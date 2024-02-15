#include <time.h>

// ------------------------------------------------------------------

#include "piggy.h"

// ------------------------------------------------------------------

#include "ApiTypes.h"
#include "Renderer.h"
#include "ImageReadWrite.h"

#include "Core/Arena.h"
#include "Core/Config.h"
#include "Core/String.h"

#include "dx12.h"
#include "rle.h"

// ------------------------------------------------------------------

#include "RTmaterials.h"

#pragma pack(push, 8)

// #define RT_DUMP_GAME_BITMAPS

// NOTE(daniel): These warnings, the second one especially, are really useful when writing C.
// They prevent you from accidentally using functions from headers you never included and such other hilarious jokes C gets up to.
#pragma warning(error: 4431) // default-int (variables)
#pragma warning(error: 4013) // default-int (function returns)

RT_TextureFormat g_rt_material_texture_slot_formats[RT_MaterialTextureSlot_COUNT] =
{
	[RT_MaterialTextureSlot_Albedo]    = RT_TextureFormat_SRGBA8,
	[RT_MaterialTextureSlot_Normal]    = RT_TextureFormat_RGBA8,
	[RT_MaterialTextureSlot_Metalness] = RT_TextureFormat_R8,
	[RT_MaterialTextureSlot_Roughness] = RT_TextureFormat_R8,
	[RT_MaterialTextureSlot_Emissive]  = RT_TextureFormat_SRGBA8, // TODO(daniel): Double check emissive textures should be sRGB encoded.
};

char *g_rt_texture_slot_names[RT_MaterialTextureSlot_COUNT] =
{
	[RT_MaterialTextureSlot_Albedo]    = "basecolor",
	[RT_MaterialTextureSlot_Normal]    = "normal",
	[RT_MaterialTextureSlot_Metalness] = "metalness",
	[RT_MaterialTextureSlot_Roughness] = "roughness",
	[RT_MaterialTextureSlot_Emissive]  = "emissive",
};

RT_Material      g_rt_materials             [RT_MAX_TEXTURES];
RT_MaterialPaths g_rt_material_paths        [RT_MAX_TEXTURES];
RT_Material      g_rt_materials_default     [RT_MAX_TEXTURES];
RT_MaterialPaths g_rt_material_paths_default[RT_MAX_TEXTURES];

static int g_rt_last_texture_update_time = 0;

static void RT_ParseMaterialDefinitionFile(int bm_index, RT_Material *material, RT_MaterialPaths *paths, 
										   // NOTE(daniel): These are dumb, but I have to set metalness/roughness to 1 by default
										   // if there exists a metalness/roughness map, but if the file changed the metalness/roughness
										   // factor then I don't want to override it...
										   bool *default_metalness, 
										   bool *default_roughness) 
{
	if (default_metalness) *default_metalness = true;
	if (default_roughness) *default_roughness = true;

	RT_ArenaMemoryScope(&g_thread_arena)
	{
		char bitmap_name[13];
		piggy_get_bitmap_name(bm_index, bitmap_name);

		// NOTE(daniel): Substance Designer does not like hashtags in names and
		// turns them into underscores.
		for (char *c = bitmap_name; *c; c++)
		{
			if (*c == '#')
			{
				*c = '_';
			}
		}

		// ------------------------------------------------------------------
		// - IMPORTANT ------------------------------------------------------
		// ------------------------------------------------------------------

		// Don't initialize any defaults for the material or material paths
		// in this function. Don't do it, I tell you! Just things read from
		// the material definition file!!

		char *material_file = RT_ArenaPrintF(&g_thread_arena, "assets/textures/%s.material", bitmap_name);

		RT_Config cfg;
		RT_InitializeConfig(&cfg, &g_thread_arena);

		if (RT_DeserializeConfigFromFile(&cfg, material_file))
		{
			RT_String string;

			if (RT_ConfigReadString(&cfg, RT_StringLiteral("albedo_texture"), &string))
			{
				RT_CopyStringToBufferNullTerm(string, sizeof(paths->albedo_texture), paths->albedo_texture);
			}

			if (RT_ConfigReadString(&cfg, RT_StringLiteral("normal_texture"), &string))
			{
				RT_CopyStringToBufferNullTerm(string, sizeof(paths->normal_texture), paths->normal_texture);
			}

			if (RT_ConfigReadString(&cfg, RT_StringLiteral("metalness_texture"), &string))
			{
				RT_CopyStringToBufferNullTerm(string, sizeof(paths->metalness_texture), paths->metalness_texture);
			}

			if (RT_ConfigReadString(&cfg, RT_StringLiteral("roughness_texture"), &string))
			{
				RT_CopyStringToBufferNullTerm(string, sizeof(paths->roughness_texture), paths->roughness_texture);
			}

			if (RT_ConfigReadString(&cfg, RT_StringLiteral("emissive_texture"), &string))
			{
				RT_CopyStringToBufferNullTerm(string, sizeof(paths->emissive_texture), paths->emissive_texture);
			}

			bool has_roughness = RT_ConfigReadFloat(&cfg, RT_StringLiteral("roughness"), &material->roughness);
			bool has_metalness = RT_ConfigReadFloat(&cfg, RT_StringLiteral("metalness"), &material->metalness);
			RT_ConfigReadVec3(&cfg, RT_StringLiteral("emissive_color"), &material->emissive_color);
			RT_ConfigReadFloat(&cfg, RT_StringLiteral("emissive_strength"), &material->emissive_strength);

			int blackbody = 0;
			RT_ConfigReadInt(&cfg, RT_StringLiteral("blackbody"), &blackbody);
			material->flags = RT_SET_FLAG(material->flags, RT_MaterialFlag_BlackbodyRadiator, blackbody);

			int no_casting_shadow = 0;
			RT_ConfigReadInt(&cfg, RT_StringLiteral("no_casting_shadow"), &no_casting_shadow);
			material->flags = RT_SET_FLAG(material->flags, RT_MaterialFlag_NoCastingShadow, no_casting_shadow);

			int is_light = 0;
			RT_ConfigReadInt(&cfg, RT_StringLiteral("is_light"), &is_light);
			material->flags = RT_SET_FLAG(material->flags, RT_MaterialFlag_Light, is_light);

			int fsr2_reactive_mask = 0;
			RT_ConfigReadInt(&cfg, RT_StringLiteral("fsr2_reactive_mask"), &fsr2_reactive_mask);
			material->flags = RT_SET_FLAG(material->flags, RT_MaterialFlag_Fsr2ReactiveMask, fsr2_reactive_mask);

			if (default_roughness) *default_roughness = !has_roughness;
			if (default_metalness) *default_metalness = !has_metalness;
		}
	}
}

static int RT_LoadMaterialTexturesFromPaths(uint16_t bm_index, RT_Material *material, RT_MaterialPaths *paths, uint32_t load_mask)
{
	// TODO(daniel): Free existing textures if they're being overwritten.
	// TODO(daniel): Handle original game bitmaps in here as well.

	int textures_reloaded = 0;

	for (size_t i = 0; i < RT_MaterialTextureSlot_COUNT; i++)
	{
		if (load_mask & RT_BITN(i))
		{
			
			// first try loading the compressed dds version of the file if it exists
			bool ddsLoaded = false;
			RT_ArenaMemoryScope(&g_thread_arena)
			{
				char* ddsFile = RT_ArenaPrintF(&g_thread_arena, "assets/textures/%s.dds", paths->textures[i]);
				
				uint8_t* ddsData = NULL;  // this is pointer to whole file including header
				const struct DDS_HEADER* header = NULL;	// pointer to the header (in same buffer as ddsData)
				const uint8_t* bitData = NULL;		// pointer to the compressed pixel data (in same buffer as ddsData)
				size_t bitSize = 0;					// size of the compressed pixel data
				
				bool fileLoaded = RT_LoadDDSImageFromDisk(&g_thread_arena, ddsFile, &ddsData, &header, &bitData, &bitSize);

				if (fileLoaded && ddsData)
				{
					ddsLoaded = true;

					material->textures[i] = RT_UploadTextureDDS(&(RT_UploadTextureParamsDDS) {
						.header = header,
						.ddsData = ddsData,
						.bitData = bitData,
						.bitSize = bitSize,
						.sRGB = i == 0 || i == 4,	// set sRGB to true if basecolor or emissive
						.name = RT_ArenaPrintF(&g_thread_arena, "Game Texture %hu:%s (source: %s)",
								bm_index, g_rt_texture_slot_names[i], ddsFile)
					});

					textures_reloaded += 1;
				}

			}

			if (!ddsLoaded)	// dds file not loaded try png
			{
				RT_ArenaMemoryScope(&g_thread_arena)
				{
					char* file = RT_ArenaPrintF(&g_thread_arena, "assets/textures/%s.png", paths->textures[i]);

					RT_TextureFormat format = g_rt_material_texture_slot_formats[i];

					int bpp = g_rt_texture_format_bpp[format];

					int w = 0, h = 0, c = 0;
					unsigned char* pixels = RT_LoadImageFromDisk(&g_thread_arena, file, &w, &h, &c, bpp);

					if (i == RT_MaterialTextureSlot_Emissive)
					{
						// Premultiply emissive by alpha to avoid white transparent backgrounds from showing...

						uint32_t* at = (uint32_t*)pixels;
						for (size_t i = 0; i < w * h; i++)
						{
							RT_Vec4 color = RT_UnpackRGBA(*at);
							color.xyz = RT_Vec3Muls(color.xyz, color.w);
							*at++ = RT_PackRGBA(color);
						}
					}

					if (pixels)
					{
						material->textures[i] = RT_UploadTexture(&(RT_UploadTextureParams) {
							.format = format,
								.width = w,
								.height = h,
								.pixels = pixels,
								.pitch = bpp * w,
								.name = RT_ArenaPrintF(&g_thread_arena, "Game Texture %hu:%s (source: %s)",
									bm_index, g_rt_texture_slot_names[i], file),
						});

						textures_reloaded += 1;
					}
				}
			}
		}
	}

	return textures_reloaded;
}

void RT_InitAllBitmaps(void) 
{
	// This function creates all materials for all GameBitmaps. It has no insight into
	// the indirection of the Textures / ObjBitmaps / ObjBitmapPtrs arrays.
	// - Daniel 01/03/2023
	
	for (uint16_t bm_index = 1; bm_index < MAX_BITMAP_FILES; bm_index++)
	{
		grs_bitmap *bitmap = &GameBitmaps[bm_index];

		if (bitmap->bm_w == 0 ||
			bitmap->bm_h == 0)
		{
			continue;
		}

		PIGGY_PAGE_IN((bitmap_index){bm_index});

		RT_ArenaMemoryScope(&g_thread_arena)
		{
			if (bitmap->bm_flags & BM_FLAG_RLE)
			{
				bitmap = rle_expand_texture(bitmap);
			}

			char bitmap_name[13];
			piggy_get_bitmap_name(bm_index, bitmap_name);

			// NOTE(daniel): Substance Designer does not like hashtags in names and
			// turns them into underscores.
			for (char *c = bitmap_name; *c; c++)
			{
				if (*c == '#')
				{
					*c = '_';
				}
			}

            // Try to load the material definition
			RT_Material *material = &g_rt_materials[bm_index];
			RT_MaterialPaths *paths = &g_rt_material_paths[bm_index];

			// ------------------------------------------------------------------
			// Initialize all the defaults

			material->metalness = 0.0f;
			material->roughness = 0.8f;
			material->emissive_color = RT_Vec3FromScalar(1.0f);
			material->emissive_strength = 1.0f;

			if (strstr(bitmap_name, "metl"))
			{
				material->metalness = 1.0f;
				material->roughness = 0.5f;
			}

			snprintf(paths->albedo_texture, sizeof(paths->albedo_texture), "%s_basecolor", bitmap_name);
			snprintf(paths->normal_texture, sizeof(paths->normal_texture), "%s_normal", bitmap_name);
			snprintf(paths->metalness_texture, sizeof(paths->metalness_texture), "%s_metallic", bitmap_name);
			snprintf(paths->roughness_texture, sizeof(paths->roughness_texture), "%s_roughness", bitmap_name);
			snprintf(paths->emissive_texture, sizeof(paths->emissive_texture), "%s_emissive", bitmap_name);

			// ------------------------------------------------------------------

			memcpy(&g_rt_materials_default[bm_index], material, sizeof(*material));
			memcpy(&g_rt_material_paths_default[bm_index], paths, sizeof(*paths));

			bool default_metalness, default_roughness;
            RT_ParseMaterialDefinitionFile(bm_index, material, paths, &default_metalness, &default_roughness);

			RT_LoadMaterialTexturesFromPaths(bm_index, material, paths, ~0u);

			if (default_metalness && RT_RESOURCE_HANDLE_VALID(material->metalness_texture))
			{
				material->metalness = 1.0f;
				// AND ALSO make note of this quirk for the defaults, where after all that we should store the default
				// metalness and roughness if it was affected by whether or not there was a metal/roughness map:
				g_rt_materials_default[bm_index].metalness = material->metalness;
			}

			if (default_roughness && RT_RESOURCE_HANDLE_VALID(material->roughness_texture))
			{
				material->roughness = 1.0f;
				// AND ALSO make note of this quirk for the defaults, where after all that we should store the default
				// metalness and roughness if it was affected by whether or not there was a metal/roughness map:
				g_rt_materials_default[bm_index].roughness = material->roughness;
			}

			if (!RT_RESOURCE_HANDLE_VALID(material->albedo_texture))
			{
                uint32_t *pixels = dx12_load_bitmap_pixel_data(&g_thread_arena, bitmap);

                material->albedo_texture = RT_UploadTexture(&(RT_UploadTextureParams) {
                    .width  = bitmap->bm_w,
                    .height = bitmap->bm_h,
                    .pixels = pixels,
                    .name   = RT_ArenaPrintF(&g_thread_arena, "Game Texture %hu:basecolor (original)", bm_index),
                    .format = g_rt_material_texture_slot_formats[RT_MaterialTextureSlot_Albedo],
                });

#ifdef RT_DUMP_GAME_BITMAPS
				{
					const char *png_path = RT_ArenaPrintF(&g_thread_arena, "assets/texture_dump/%s.png", bitmap_name);
					RT_WritePNGToDisk(png_path, bitmap->bm_w, bitmap->bm_h, 4, pixels, 4*bitmap->bm_w);
				}
#endif
			}

			RT_UpdateMaterial(bm_index, material);
		}

	}
	piggy_bitmap_page_out_all();

	g_rt_last_texture_update_time = time(NULL);
}

static bool TextureFileIsOutdated(char *name)
{
	RT_ArenaMemoryScope(&g_thread_arena)
	{
		char *path = RT_ArenaPrintF(&g_thread_arena, "assets/textures/%s", name);

		PHYSFS_Stat stat;
		if (PHYSFS_stat(path, &stat))
		{
			if (stat.createtime > g_rt_last_texture_update_time || stat.accesstime > g_rt_last_texture_update_time)
				return true;
		}
	}
	// NOTE(daniel): Returns false if file does not exist.
	return false;
}

int RT_ReloadMaterials(void)
{
	// Function loads bitmaps changed during runtime

	int textures_reloaded = 0;

	for (uint16_t bm_index = 1; bm_index < MAX_BITMAP_FILES; bm_index++)
	{
		grs_bitmap *bitmap = &GameBitmaps[bm_index];

		if (bitmap->bm_w == 0 ||
			bitmap->bm_h == 0)
		{
			continue;
		}

		char bitmap_name[13];
		piggy_get_bitmap_name(bm_index, bitmap_name);

		char bitmap_file_name[13];
		strcpy(bitmap_file_name, bitmap_name);

		// Fix up the name because substance doesn't like hashtags...
		for (char *c = bitmap_file_name; *c; c++)
		{
			if (*c == '#') *c = '_';
		}

		RT_Material *material = &g_rt_materials[bm_index];
		RT_MaterialPaths *paths = &g_rt_material_paths[bm_index];

		uint32_t needs_reload = 0;

		char *material_definition_path = RT_ArenaPrintF(&g_thread_arena, "assets/textures/%s.material", bitmap_file_name);
		if (TextureFileIsOutdated(material_definition_path))
		{
			RT_MaterialPaths new_paths = {0};
			RT_ParseMaterialDefinitionFile(bm_index, material, &new_paths, NULL, NULL);

			for (size_t i = 0; i < RT_MaterialTextureSlot_COUNT; i++)
			{
				if (!strncmp(paths->textures[i], new_paths.textures[i], sizeof(paths->textures[i])))
				{
					needs_reload |= RT_BITN(i);
				}
			}

			memcpy(paths, &new_paths, sizeof(new_paths));
		}

		for (size_t i = 0; i < RT_MaterialTextureSlot_COUNT; i++)
		{
			if (!(needs_reload & (1u << i)))
			{
				if (!RT_RESOURCE_HANDLE_VALID(material->textures[i]) || // always try to load textures if there weren't any for this kind
					TextureFileIsOutdated(paths->textures[i]))
				{
					if (RT_RESOURCE_HANDLE_VALID(material->textures[i]))
						RT_ReleaseTexture(material->textures[i]);
					needs_reload |= (1u << i);
				}
			}
		}

		textures_reloaded += RT_LoadMaterialTexturesFromPaths(bm_index, material, paths, needs_reload);
		RT_UpdateMaterial(bm_index, material);
	}

	// Remember what time we last reloaded textures
	// NOTE(daniel): I wanted to double check: Yes, the result of time() should be
	// compatible with PHYSFS's file timestamps. They're good old unix time. It still
	// makes me feel slightly uncomfortable though, I'd prefer to only compare file times
	// provided by PHYSFS so that you know for sure it always works.
	g_rt_last_texture_update_time = time(NULL);

	return textures_reloaded;
}

