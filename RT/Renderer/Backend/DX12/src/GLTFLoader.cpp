#define CGLTF_IMPLEMENTATION
#pragma warning(push, 0)
#include "../CGLTF/cgltf.h"
#pragma pop

// ------------------------------------------------------------------

#include "GLTFLoader.h"

#include "Core/Common.h"
#include "Core/Arena.h"
#include "Core/MemoryScope.hpp" 
#include "Core/MiniMath.h"

#include "Renderer.h"
#include "ImageReadWrite.h"

using namespace RT;

// ------------------------------------------------------------------

static uint16_t running_material_index;
static uint32_t running_image_index;

template <typename T>
static T *GetBufferViewPointer(const cgltf_buffer_view *buffer_view)
{
	char *base = static_cast<char *>(buffer_view->buffer->data);
	base += buffer_view->offset;

	return reinterpret_cast<T *>(base);
}

template <typename T = void>
static T *GetAccessorPointer(const cgltf_accessor *accessor)
{
	cgltf_buffer_view *buffer_view = accessor->buffer_view;

	char *base = static_cast<char *>(buffer_view->buffer->data);
	base += buffer_view->offset;
	base += accessor->offset;

	return reinterpret_cast<T *>(base);
}

static char *GetDirectory(RT_Arena *arena, const char *path)
{
	size_t path_count = strlen(path);
	char* result = (char *)RT_ArenaCopy(arena, path, path_count, 1);

    char *last_slash = nullptr;
    for (char *c = result; *c; c++)
    {
        if (*c == '\\')
            last_slash = c;
    }
    if (last_slash)
        last_slash[1] = 0;
    return result;
}

static char *CreatePathFromUri(RT_Arena *arena, const char *base, const char *uri)
{
	char* result = RT_ArenaAllocArrayNoZero(arena, strlen(base) + strlen(uri), char);

    cgltf_combine_paths(result, base, uri);
    cgltf_decode_uri(result + strlen(result) - strlen(uri));

    return result;
}

static int GLTFMeshIndex(const cgltf_data *gltf, const cgltf_mesh *mesh)
{
	return (int)(mesh - gltf->meshes);
}

static int GLTFImageIndex(const cgltf_data *gltf, const cgltf_image *image)
{
	return (int)(image - gltf->images);
}

static int GLTFNodeIndex(const cgltf_data *gltf, const cgltf_node *node)
{
	return (int)(node - gltf->nodes);
}


// ------------------------------------------------------------------

RT_GLTFNode *RT_LoadGLTF(RT_Arena *arena, const char *path, RT_MaterialOverride* material_override)
{
	MemoryScope temp;

    // ------------------------------------------------------------------

    cgltf_options options = {};

	options.memory.alloc_func = [](void* user, cgltf_size size)
	{
		(void)user;
		return RT_ArenaAllocNoZero(&g_thread_arena, size, 16);
	};

	options.memory.free_func = [](void* user, void* ptr)
	{
		(void)user;
		(void)ptr;
	};

    cgltf_data *data = nullptr;

	cgltf_result parse_result = cgltf_parse_file(&options, path, &data);
    if (parse_result != cgltf_result_success)
        return nullptr;

    cgltf_load_buffers(&options, data, path);

	char* gltf_directory = GetDirectory(temp, path);

	struct LoadedImage
	{
		const char* uri;
		RT_ResourceHandle handle;
		RT_ResourceHandle handle2; // if metallic_roughness texture, this is the roughness one
	};

	LoadedImage **loaded_images = RT_ArenaAllocArray(temp, data->images_count, LoadedImage *);

	// This stuff is getting hacky
	auto LoadImage = [&](cgltf_image *image, bool is_srgb, bool is_metallic_roughness) -> LoadedImage *
	{
		int index = GLTFImageIndex(data, image);

		LoadedImage *result = nullptr;

		if (loaded_images[index])
		{
			result = loaded_images[index];
		}
		else
		{
			loaded_images[index] = RT_ArenaAllocStruct(temp, LoadedImage);

			MemoryScope image_temp;
			
			RT_Image loaded_image;

			if (image->buffer_view)
			{
				unsigned char* src = GetBufferViewPointer<unsigned char>(image->buffer_view);
				loaded_image = RT_LoadImageFromMemory(image_temp, src, image->buffer_view->size, 4, is_srgb);
			}
			else
			{
				char* image_path = CreatePathFromUri(image_temp, gltf_directory, image->uri);
				loaded_image = RT_LoadImageFromDisk(image_temp, image_path, 4, is_srgb);
			}

			if (loaded_image.pixels)
			{
				loaded_images[index]->uri = image->uri;

				uint32_t w = loaded_image.width;
				uint32_t h = loaded_image.height;

				if (is_metallic_roughness)
				{
					if (loaded_image.format >= RT_TextureFormat_BC1 &&
						loaded_image.format <= RT_TextureFormat_BC7_SRGB)
					{
						// Unfortunately, due to the combining below, we can't support DXT compressed textures for metallic roughness textures in GLTFs.
						RT_FATAL_ERROR("DXT compressed textures are not supported for metallic roughness textures in GLTF models, for stupid reasons");
					}

					uint8_t *metalness = RT_ArenaAllocArrayNoZero(image_temp, w*h, uint8_t);
					uint8_t *roughness = RT_ArenaAllocArrayNoZero(image_temp, w*h, uint8_t);

					uint32_t *src = (uint32_t *)loaded_image.pixels;
					uint8_t *metalness_dst = metalness;
					uint8_t *roughness_dst = roughness;

					for (int i = 0; i < w*h; i++)
					{
						uint32_t src_pixel = *src++;
						*metalness_dst++ = (src_pixel >> 16) & 0xFF;
						*roughness_dst++ = (src_pixel >>  8) & 0xFF;
					}

					{
						RT_UploadTextureParams params = {};
						params.image.format = RT_TextureFormat_R8;
						params.image.width  = (uint32_t)w;
						params.image.height = (uint32_t)h;
						params.image.pixels = metalness;
						params.name   = RT_ArenaPrintF(image_temp, "%s (metalness)", image->uri);
						loaded_images[index]->handle = RT_UploadTexture(&params);
					}

					{
						RT_UploadTextureParams params = {};
						params.image.format = RT_TextureFormat_R8;
						params.image.width  = (uint32_t)w;
						params.image.height = (uint32_t)h;
						params.image.pixels = roughness;
						params.name   = RT_ArenaPrintF(image_temp, "%s (roughness)", image->uri);
						loaded_images[index]->handle2 = RT_UploadTexture(&params);
					}
				}
				else
				{
					RT_UploadTextureParams params = {};
					params.image = loaded_image;
					params.name   = image->uri ? image->uri : RT_ArenaPrintF(image_temp, "GLTF Texture #%d", running_image_index++);
					loaded_images[index]->handle = RT_UploadTexture(&params);
				}

				result = loaded_images[index];
			}
		}

		return result;
	};

	RT_GLTFModel *models = RT_ArenaAllocArray(arena, data->meshes_count, RT_GLTFModel);

	for (size_t mesh_index = 0; mesh_index < data->meshes_count; mesh_index++)
	{
		cgltf_mesh *mesh = &data->meshes[mesh_index];

		size_t mesh_triangle_count = 0;
		for (size_t primitive_index = 0; primitive_index < mesh->primitives_count; primitive_index++)
		{
			cgltf_primitive* primitive = &mesh->primitives[primitive_index];

			RT_ASSERT(primitive->indices->count % 3 == 0);
			mesh_triangle_count += primitive->indices->count / 3;
		}

		RT_Triangle *mesh_triangles = RT_ArenaAllocArray(temp, mesh_triangle_count, RT_Triangle);
		RT_Triangle *prim_triangles = mesh_triangles;

		for (size_t primitive_index = 0; primitive_index < mesh->primitives_count; primitive_index++)
		{
			cgltf_primitive* primitive = &mesh->primitives[primitive_index];

			RT_ASSERT(primitive->type == cgltf_primitive_type_triangles);

			size_t prim_triangle_count = primitive->indices->count / 3;

			RT_Material material = {};
			uint16_t material_index;

			// note(lily): quick ugly hack to make the cockpit no longer cast shadows, but it works so there ya go
			if (strcmp(path, "assets/cockpit_prototype.gltf") == 0) {
				material.flags |= RT_MaterialFlag_NoCastingShadow;
			}

			if (material_override != nullptr && strcmp(primitive->material->name, material_override->name) == 0) {
				material_index = material_override->material_index;
			}
			else {
				if (primitive->material->pbr_metallic_roughness.base_color_texture.texture)
				{
					LoadedImage* loaded_image = LoadImage(primitive->material->pbr_metallic_roughness.base_color_texture.texture->image, true, false);
					if (loaded_image)
					{
						material.albedo_texture = loaded_image->handle;
					}
				}

				if (primitive->material->normal_texture.texture)
				{
					LoadedImage* loaded_image = LoadImage(primitive->material->normal_texture.texture->image, false, false);
					if (loaded_image)
					{
						material.normal_texture = loaded_image->handle;
					}
				}

				if (primitive->material->pbr_metallic_roughness.metallic_roughness_texture.texture)
				{
					LoadedImage* loaded_image = LoadImage(primitive->material->pbr_metallic_roughness.metallic_roughness_texture.texture->image, false, true);
					if (loaded_image)
					{
						material.metalness_texture = loaded_image->handle;
						material.roughness_texture = loaded_image->handle2;
					}
				}
				material.metalness = primitive->material->pbr_metallic_roughness.metallic_factor;
				material.roughness = primitive->material->pbr_metallic_roughness.roughness_factor;

				if (primitive->material->emissive_texture.texture)
				{
					LoadedImage* loaded_image = LoadImage(primitive->material->emissive_texture.texture->image, true, false);
					if (loaded_image)
					{
						material.emissive_texture = loaded_image->handle;
					}
				}
				material.emissive_color = RT_Vec3Make(primitive->material->emissive_factor[0],
					primitive->material->emissive_factor[1],
					primitive->material->emissive_factor[2]);
				material.emissive_strength = 1.0f;
				if (primitive->material->has_emissive_strength)
				{
					material.emissive_strength = primitive->material->emissive_strength.emissive_strength;
				}
				material_index = RT_UpdateMaterial(RT_EXTRA_BITMAPS_START + running_material_index++, &material);
			};

			RT_ASSERT(material_index != UINT16_MAX);

			// All the triangles point into the material edge
			for (size_t triangle_index = 0; triangle_index < prim_triangle_count; triangle_index++)
			{
				prim_triangles[triangle_index].material_edge_index = material_index|RT_TRIANGLE_HOLDS_MATERIAL_INDEX;
			}

			size_t    index_count = primitive->indices->count;
			uint32_t *indices     = nullptr;

			if (primitive->indices->component_type == cgltf_component_type_r_32u)
			{
				indices = GetAccessorPointer<uint32_t>(primitive->indices);
			}
			else
			{
				RT_ASSERT(primitive->indices->component_type == cgltf_component_type_r_16u);
				uint16_t *indices16 = GetAccessorPointer<uint16_t>(primitive->indices);

				indices = RT_ArenaAllocArrayNoZero(temp, index_count, uint32_t);
				for (size_t i = 0; i < index_count; i++)
				{
					indices[i] = indices16[i];
				}
			}

			bool calculate_tangents = true;

			for (size_t k = 0; k < primitive->attributes_count; k++)
			{
				cgltf_attribute *attribute = &primitive->attributes[k];

				switch (attribute->type)
				{
					case cgltf_attribute_type_position:
					{
						RT_ASSERT(attribute->data->type == cgltf_type_vec3);

						RT_Vec3 *positions = GetAccessorPointer<RT_Vec3>(attribute->data);

						for (size_t triangle_index = 0; triangle_index < index_count / 3; triangle_index++)
						{
							prim_triangles[triangle_index].pos0 = positions[indices[triangle_index * 3 + 0]];
							prim_triangles[triangle_index].pos1 = positions[indices[triangle_index * 3 + 1]];
							prim_triangles[triangle_index].pos2 = positions[indices[triangle_index * 3 + 2]];
						}
					} break;

					case cgltf_attribute_type_texcoord:
					{
						RT_ASSERT(attribute->data->type == cgltf_type_vec2);

						RT_Vec2 *texcoords = GetAccessorPointer<RT_Vec2>(attribute->data);

						for (size_t triangle_index = 0; triangle_index < index_count / 3; triangle_index++)
						{
							prim_triangles[triangle_index].uv0 = texcoords[indices[triangle_index * 3 + 0]];
							prim_triangles[triangle_index].uv1 = texcoords[indices[triangle_index * 3 + 1]];
							prim_triangles[triangle_index].uv2 = texcoords[indices[triangle_index * 3 + 2]];
						}
					} break;

					case cgltf_attribute_type_normal:
					{
						RT_ASSERT(attribute->data->type == cgltf_type_vec3);

						RT_Vec3 *normals = GetAccessorPointer<RT_Vec3>(attribute->data);

						for (size_t triangle_index = 0; triangle_index < index_count / 3; triangle_index++)
						{
							prim_triangles[triangle_index].normal0 = normals[indices[triangle_index * 3 + 0]];
							prim_triangles[triangle_index].normal1 = normals[indices[triangle_index * 3 + 1]];
							prim_triangles[triangle_index].normal2 = normals[indices[triangle_index * 3 + 2]];
						}
					} break;

					case cgltf_attribute_type_tangent:
					{
						RT_ASSERT(attribute->data->type == cgltf_type_vec4);

						RT_Vec4* tangents = GetAccessorPointer<RT_Vec4>(attribute->data);

						for (size_t triangle_index = 0; triangle_index < index_count / 3; triangle_index++)
						{
							prim_triangles[triangle_index].tangent0 = tangents[indices[triangle_index * 3 + 0]];
							prim_triangles[triangle_index].tangent1 = tangents[indices[triangle_index * 3 + 1]];
							prim_triangles[triangle_index].tangent2 = tangents[indices[triangle_index * 3 + 2]];
						}

						calculate_tangents = false;
					} break;
				}
			}

			if (calculate_tangents)
			{
				RT_GenerateTangents(prim_triangles, prim_triangle_count);
			}

			for (size_t triangle_index = 0; triangle_index < prim_triangle_count; triangle_index++)
			{
				prim_triangles[triangle_index].color = 0xFFFFFFFF;
			}

			prim_triangles += prim_triangle_count;
		}

		RT_UploadMeshParams upload_mesh = {};
		upload_mesh.triangle_count = mesh_triangle_count;
		upload_mesh.triangles = mesh_triangles;
		upload_mesh.name = RT_ArenaPrintF(temp, "%s: Primitive %zu", path);

		models[mesh_index].handle = RT_UploadMesh(&upload_mesh);
	}

	RT_GLTFNode *root = RT_ArenaAllocStruct(arena, RT_GLTFNode);
	root->transform = RT_MakeMat4(-1, 0, 0, 0,
								   0, 1, 0, 0,
								   0, 0, 1, 0,
								   0, 0, 0, 1);

	if (data->scenes_count > 0)
	{
		cgltf_scene *scene = &data->scenes[0];

		// ------------------------------------------------------------------
		// Load nodes

		RT_GLTFNode *dst_nodes = RT_ArenaAllocArray(arena, data->nodes_count, RT_GLTFNode);
		{
			for (size_t node_index = 0; node_index < data->nodes_count; node_index++)
			{
				cgltf_node  *src_node = &data->nodes[node_index];
				RT_GLTFNode *dst_node = &dst_nodes[node_index];

				if (src_node->parent)
				{
					dst_node->parent = dst_nodes + GLTFNodeIndex(data, src_node->parent);
				}

				if (src_node->name)
				{
					size_t name_length = strlen(src_node->name);
					dst_node->name = (char *)RT_ArenaCopy(arena, src_node->name, name_length, 4);
				}

				if (src_node->mesh)
				{
					dst_node->model = models + GLTFMeshIndex(data, src_node->mesh);
				}

				if (src_node->has_matrix)
				{
					memcpy(&dst_node->transform, src_node->matrix, sizeof(RT_Mat4));
					dst_node->transform = RT_Mat4Transpose(dst_node->transform);
				}
				else
				{
					RT_Vec3 translation = RT_Vec3FromScalar(0);

					if (src_node->has_translation)
						translation = RT_Vec3FromFloats(src_node->translation);

					RT_Quat rotation = RT_QuatIdentity();

					if (src_node->has_rotation)
						rotation = RT_QuatFromFloats(src_node->rotation);

					RT_Vec3 scale = RT_Vec3FromScalar(1);

					if (src_node->has_scale)
						scale = RT_Vec3FromFloats(src_node->scale);

					dst_node->transform = RT_Mat4FromTRS(translation, rotation, scale);
				}

				dst_node->children_count = src_node->children_count;
				dst_node->children = RT_ArenaAllocArray(arena, dst_node->children_count, RT_GLTFNode *);

				for (size_t child_index = 0; child_index < dst_node->children_count; child_index++)
				{
					dst_node->children[child_index] = dst_nodes + GLTFNodeIndex(data, src_node->children[child_index]);
				}
			}
		}

		// ------------------------------------------------------------------
		// Add scene nodes to root node

		root->children_count = scene->nodes_count;
		root->children = RT_ArenaAllocArray(arena, root->children_count, RT_GLTFNode *);

		for (size_t node_index = 0; node_index < scene->nodes_count; node_index++)
		{
			root->children[node_index] = dst_nodes + GLTFNodeIndex(data, scene->nodes[node_index]);
		}
	}
	else
	{
		// ------------------------------------------------------------------
		// If there is no scene, just create a root node that has all the
		// models.

		root->children_count = data->meshes_count;
		root->children = RT_ArenaAllocArray(arena, data->meshes_count, RT_GLTFNode *);

		for (size_t mesh_index = 0; mesh_index < data->meshes_count; mesh_index++)
		{
			RT_GLTFNode *node = root->children[mesh_index] = RT_ArenaAllocStruct(arena, RT_GLTFNode);
			node->transform = RT_Mat4Identity();
			node->model     = models + mesh_index;
		}
	}

	return root;
}
