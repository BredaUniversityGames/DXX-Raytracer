// ------------------------------------------------------------------
// Game-code includes

#include "Level.h"
#include "segment.h"
#include "textures.h"
#include "gameseq.h"
#include "wall.h"
#include "automap.h"
#include "render.h"

// ------------------------------------------------------------------
// RT includes

#include "Core/MiniMath.h"
#include "Core/Arena.h"
#include "RTgr.h"
#include "Renderer.h"
#include "Lights.h"

// ------------------------------------------------------------------

// Current active level
RT_ResourceHandle g_level_resource = { 0 };
int g_active_level = 0;

int m_light_count = 1;
RT_Light m_lights[1024] = {0};
int m_lights_definitions[1024] = {0};
side* m_extracted_light_sides[1024] = {0};

void RT_ExtractLightsFromSide(side *side, RT_Vertex *vertices, RT_Vec3 normal);

RT_Triangle RT_TriangleFromIndices(RT_Vertex* verts, int vert_offset, int v0, int v1, int v2, int tmap) 
{
	RT_Triangle triangle = { 0 };

	triangle.material_edge_index = tmap;

	RT_Vec3 pos0 = verts[vert_offset + v0].pos;
	RT_Vec3 pos1 = verts[vert_offset + v1].pos;
	RT_Vec3 pos2 = verts[vert_offset + v2].pos;

	RT_Vec3 p10 = RT_Vec3Normalize(RT_Vec3Sub(pos1, pos0));
	RT_Vec3 p20 = RT_Vec3Normalize(RT_Vec3Sub(pos2, pos0));

	RT_Vec3 normal = RT_Vec3Normalize(RT_Vec3Cross(p10, p20));

	triangle.pos0 = pos0;
	triangle.pos1 = pos1;
	triangle.pos2 = pos2;
	triangle.normal0 = normal;
	triangle.normal1 = normal;
	triangle.normal2 = normal;
	triangle.uv0 = verts[vert_offset + v0].uv;
	triangle.uv1 = verts[vert_offset + v1].uv;
	triangle.uv2 = verts[vert_offset + v2].uv;
	triangle.color = 0xFFFFFFFF;

	return triangle;
}

RT_ResourceHandle RT_UploadLevelGeometry()
{
	RT_ResourceHandle level_handle = {0};

	RT_ArenaMemoryScope(&g_thread_arena)
	{
		RT_Vertex* verts = RT_ArenaAllocArray(&g_thread_arena, Num_segments * 6 * 4, RT_Vertex);
		RT_Triangle* triangles = RT_ArenaAllocArray(&g_thread_arena, Num_segments * 6 * 2, RT_Triangle);

		int num_verts = 0;
		int num_indices = 0; 
		int num_triangles = 0;

		int num_mesh = 0;
		for (int seg_id = 0; seg_id < Num_segments; seg_id++)
		{
			segment *seg = &Segments[seg_id];

			for (int side_index = 0; side_index < MAX_SIDES_PER_SEGMENT; side_index++)
			{
				const int vertex_offset = num_verts;
				const int indices_offset = num_indices;

				side *s = &seg->sides[side_index];
				int vertnum_list[4];
				get_side_verts(&vertnum_list, seg_id, side_index);

				int vert_ids[4];

				for (int v = 0; v < 4; v++)
				{
					// Extract Vertex Data
					const int vertex_id = vertnum_list[v];
					vms_vector raw_vertex = Vertices[vertex_id];
					vms_vector raw_normal = s->normals[0]; // if quadrilateral use this as normal.

					RT_Vertex vert =
					{
						f2fl(raw_vertex.x),
						f2fl(raw_vertex.y),
						f2fl(raw_vertex.z),
						f2fl(s->uvls[v].u),
						f2fl(s->uvls[v].v),
						f2fl(raw_normal.x),
						f2fl(raw_normal.y),
						f2fl(raw_normal.z)
					};
					verts[vertex_offset + v] = vert;
					num_verts++;

					assert(vert.uv.x >= -10.0 && vert.uv.x <= 10.0);
				}

				// Ignore invisible walls
				bool should_render = false;
				if (seg->children[side_index] == -1)
				{
					should_render = true;
				}
				else if (s->wall_num != -1)
				{
					wall *w = &Walls[s->wall_num];
					// TODO(daniel): What about blastable wallls?
					if (w->type != WALL_OPEN)
					{
						should_render = true;
					}
				}

				if (!should_render) { continue; }

				int absolute_side_index = MAX_SIDES_PER_SEGMENT*seg_id + side_index;
				triangles[num_triangles++] = RT_TriangleFromIndices(verts, vertex_offset, 0, 1, 2, absolute_side_index);
				triangles[num_triangles++] = RT_TriangleFromIndices(verts, vertex_offset, 0, 2, 3, absolute_side_index);

				RT_ExtractLightsFromSide(s, &verts[vertex_offset], triangles[num_triangles-1].normal0);
			}
		}

		// NOTE(daniel): This is a separate call, because I don't want to do something tweaky like
		// detecting whether tangents need to be calculated in RT_UploadMesh. You, the uploader, should know.
		RT_GenerateTangents(triangles, num_triangles);

		RT_UploadMeshParams params =
		{
			.triangle_count = num_triangles,
			.triangles      = triangles,
		};

		RT_LOGF(RT_LOGSERVERITY_INFO, "UPLOADING MESH >>\n");
		level_handle = RT_UploadMesh(&params);
		RT_LOGF(RT_LOGSERVERITY_INFO, "UPLOADING MESH OK\n");
	}

	return level_handle;
}

void RT_ExtractLightsFromSide(side *side, RT_Vertex *vertices, RT_Vec3 normal)
{
	int light_index = RT_IsLight(side->tmap_num2);
	if (light_index > -1)
	{
		RT_Vec2 uv_min = RT_Vec2Make(INFINITY, INFINITY);
		RT_Vec2 uv_max = RT_Vec2Make(-INFINITY, -INFINITY);
		for (int i = 0; i < 4; i++)
		{
			RT_Vec2 uv = RT_Vec2Make((f2fl(side->uvls[i].u)),(f2fl(side->uvls[i].v)));
			
			uv_min = RT_Vec2Min(uv, uv_min);
			uv_max = RT_Vec2Max(uv, uv_max);
		}

		bool multiple_lights = false;
		if(uv_min.x < -1.0 || uv_min.y < -1.0 || uv_max.x > 1.0 || uv_max.y > 1.0)
		{
			RT_Vec2 uv = RT_Vec2Sub(uv_max, uv_min);
			RT_Vec2 light_size = g_light_definitions[light_index].size;

			int num_x = max((int)(uv.x / light_size.x),1);
			int num_y = max((int)(uv.y / light_size.y),1);

			RT_LOGF(RT_LOGSERVERITY_INFO, "Creating lights in the following directions. {X: %i, Y: %i}", num_x, num_y);
			if(num_x > 1 && num_y > 1)
			{
				RT_LOGF(RT_LOGSERVERITY_INFO, "Multiple lights created!");
				multiple_lights = true;
			}
		}

		if (!multiple_lights) 
		{
			if (ALWAYS(m_light_count < RT_ARRAY_COUNT(m_lights)))
			{
				m_lights[m_light_count] = RT_InitLight(g_light_definitions[light_index], vertices, normal);
				m_lights_definitions[m_light_count] = light_index;
				m_extracted_light_sides[m_light_count] = side;
				m_light_count++;
			}
		}
	}
}

bool RT_LoadLevel() 
{
	assert(!RT_RESOURCE_HANDLE_VALID(g_level_resource));
	// Load level geometry
	g_level_resource = RT_UploadLevelGeometry();
	g_active_level = Current_level_num;

	return RT_RESOURCE_HANDLE_VALID(g_level_resource);
}

void RT_RenderLevel(RT_Vec3 player_pos) 
{
	// ------------------------------------------------------------------
	// TODO(daniel): Figure out the right time to update these arrays

	RT_MaterialEdge *g_rt_material_edges   = RT_GetMaterialEdgesArray();
	uint16_t        *g_rt_material_indices = RT_GetMaterialIndicesArray();

	for (int segment_index = 0; segment_index < Num_segments; segment_index++)
	{
		segment *seg = &Segments[segment_index];

		for (int side_index = 0; side_index < MAX_SIDES_PER_SEGMENT; side_index++)
		{
			side *sd = &seg->sides[side_index];

			int absolute_side_index = MAX_SIDES_PER_SEGMENT*segment_index + side_index;

			RT_MaterialEdge *side_edge = &g_rt_material_edges[absolute_side_index];
			side_edge->mat1 = sd->tmap_num;
			side_edge->mat2 = sd->tmap_num2;
		}
	}

	for (int texture_index = 0; texture_index < MAX_TEXTURES; texture_index++)
	{
		g_rt_material_indices[texture_index] = Textures[texture_index].index;
	}

	for (int texture_index = 0; texture_index < MAX_OBJ_BITMAPS; texture_index++)
	{
		g_rt_material_indices[texture_index + MAX_TEXTURES] = ObjBitmaps[texture_index].index;
	}

	// The way this system was set up is really not the best, this is my hack to easily
	// use these special built in materials in other code.
	g_rt_material_indices[RT_MATERIAL_FLAT_WHITE]     = RT_MATERIAL_FLAT_WHITE;
	g_rt_material_indices[RT_MATERIAL_EMISSIVE_WHITE] = RT_MATERIAL_EMISSIVE_WHITE;

	// ------------------------------------------------------------------

	// Unload current level if other level becomes active.
	// TODO: Given mesh unloading is currently not supported, this function will leak memory!!
	if (g_active_level != Current_level_num)
	{
		RT_UnloadLevel();
	}

	// Load level if not loaded.
	// TODO: RT_LoadLevel should be called after menu interactions, this should be removed at some point.
	if (!RT_RESOURCE_HANDLE_VALID(g_level_resource))
	{
		RT_LoadLevel();
	}

	RT_FindAndSubmitNearbyLights(player_pos);

	RT_Mat4 mat = RT_Mat4Identity();
	RT_RaytraceMesh(g_level_resource, &mat, &mat);
}

bool RT_UnloadLevel() 
{
	// TODO: Unload mesh doesn't exsist yet, level will leak now.
	g_level_resource = RT_RESOURCE_HANDLE_NULL;

	m_light_count = 0;
	memset(m_lights, 0, sizeof(RT_Light) * 1024);

	return true;
}

void RT_UpdateLight(int index)
{
	RT_LightDefinition definition = g_light_definitions[m_lights_definitions[index]];
	RT_Light* light = &m_lights[index];
	light->kind = definition.kind;

	light->spot_angle    = RT_Uint8FromFloat(definition.spot_angle);
	light->spot_softness = RT_Uint8FromFloat(definition.spot_softness);

	light->emission = RT_PackRGBE(definition.emission);

	if(light->kind == RT_LightKind_Area_Sphere)
	{
		float r = definition.radius;
		RT_Vec3 position = RT_TranslationFromMat34(light->transform);

		RT_Mat34 transform = 
		{
			.e = 
			{
				r, 0, 0, position.x,
				0, r, 0, position.y,
				0, 0, r, position.z,
			}
		};
		light->transform = transform;
	}
	
}

void RT_FindAndSubmitNearbyLights(RT_Vec3 player_pos)
{
	// NOTE(daniel): I rewrote this to call RT_RaytraceSubmitLight because I updated that API
	// to allow you to incrementally submit lights. This way, no extra array has to be
	// made and you can just submit lights so long as you don't exceed the maximum.

	int total = 0;
	for (int i = 0; i < m_light_count; i++) 
	{
		if (g_pending_light_update)
		{
			RT_UpdateLight(i);
		}

		if (g_light_visual_debug)
		{
			RT_VisualizeLight(&m_lights[i]);
		}

		//TODO: Replace with better culling heuristic
		float distance = RT_Vec3Length(RT_Vec3Sub(RT_TranslationFromMat34(m_lights[i].transform), player_pos));
		if (distance < 200.0)
		{
			RT_RaytraceSubmitLight(m_lights[i]);
			total++;
		}

	}
	
	g_pending_light_update = false;
	g_active_lights = total;
}