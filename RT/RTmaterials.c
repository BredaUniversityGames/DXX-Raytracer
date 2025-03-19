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
	// NOTE(daniel): These formats are kind of misleading with DDS textures, for them we look at them just to know if it's sRGB or not.
	[RT_MaterialTextureSlot_Albedo]    = RT_TextureFormat_RGBA8_SRGB,
	[RT_MaterialTextureSlot_Normal]    = RT_TextureFormat_RGBA8,
	[RT_MaterialTextureSlot_Metalness] = RT_TextureFormat_R8,
	[RT_MaterialTextureSlot_Roughness] = RT_TextureFormat_R8,
	[RT_MaterialTextureSlot_Emissive]  = RT_TextureFormat_RGBA8_SRGB, // TODO(daniel): Double check emissive textures should be sRGB encoded.
};

char *g_rt_texture_slot_names[RT_MaterialTextureSlot_COUNT] =
{
	[RT_MaterialTextureSlot_Albedo]    = "basecolor",
	[RT_MaterialTextureSlot_Normal]    = "normal",
	[RT_MaterialTextureSlot_Metalness] = "metalness",
	[RT_MaterialTextureSlot_Roughness] = "roughness",
	[RT_MaterialTextureSlot_Emissive]  = "emissive",
	[RT_MaterialTextureSlot_Height] = "height",
};

RT_Material				g_rt_materials				[RT_MAX_TEXTURES];
RT_MaterialPaths		g_rt_material_paths			[RT_MAX_TEXTURES];
RT_MaterialPathsExist	g_rt_material_paths_exist	[RT_MAX_TEXTURES];
RT_Material				g_rt_materials_default		[RT_MAX_TEXTURES];
RT_MaterialPaths		g_rt_material_paths_default	[RT_MAX_TEXTURES];

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

		// first try to load the material definition from the vault files
		bool loaded = RT_DeserializeConfigFromVault(&cfg, material_file);

		// if fails, try from file.
		if (!loaded)
		{
			loaded = RT_DeserializeConfigFromFile(&cfg, material_file);
		}
		
		if (loaded)
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

			if (RT_ConfigReadString(&cfg, RT_StringLiteral("height_texture"), &string))
			{
				RT_CopyStringToBufferNullTerm(string, sizeof(paths->height_texture), paths->height_texture);
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

static void RT_VerifyMaterialTexturesFromPaths(uint16_t bm_index, RT_MaterialPaths* paths, RT_MaterialPathsExist* paths_exist, uint32_t load_mask)
{
	for (size_t i = 0; i < RT_MaterialTextureSlot_COUNT; i++)
	{
		if (load_mask & RT_BITN(i))
		{
			// first see if compressed dds version of the file exists
			RT_ArenaMemoryScope(&g_thread_arena)
			{
				char* dds_file = RT_ArenaPrintF(&g_thread_arena, "assets/textures/%s.dds", paths->textures[i]);
				
				RT_String file_name_string = RT_StringFromCString(dds_file);
				bool found = RT_ConfigFileExistsInVaults(&file_name_string);

				FILE* f = fopen(dds_file, "r");

				if (found || f)
				{
					paths->textures[i];
					paths_exist->textures[i] = true;
				}

				if (f)
				{
					fclose(f);
				}
			}

			if (!paths_exist->textures[i]) // dds file not found try png
			{
				RT_ArenaMemoryScope(&g_thread_arena)
				{
					char* file = RT_ArenaPrintF(&g_thread_arena, "assets/textures/%s.png", paths->textures[i]);

					RT_String file_name_string = RT_StringFromCString(file);
					bool found = RT_ConfigFileExistsInVaults(&file_name_string);

					FILE* f = fopen(file, "r");

					if (found ||  f)
					{
						
						paths->textures[i];
						paths_exist->textures[i] = true;
					}

					if (f)
					{
						fclose(f);
					}
				}
			}
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
			bool dds_loaded = false;
			RT_ArenaMemoryScope(&g_thread_arena)
			{
				RT_TextureFormat format = g_rt_material_texture_slot_formats[i];
				bool is_srgb = (format == RT_TextureFormat_RGBA8_SRGB);

				char* dds_file = NULL;

				// check for dds extension to already be part of texture path.
				if (strstr(paths->textures[i], ".dds") || strstr(paths->textures[i], ".png"))
				{
					dds_file = RT_ArenaPrintF(&g_thread_arena, "assets/textures/%s", paths->textures[i]);
				}
				else
				{
					dds_file = RT_ArenaPrintF(&g_thread_arena, "assets/textures/%s.dds", paths->textures[i]);
				}

				// TODO(daniel): It's dumb that RT_ArenaPrintF doesn't return an RT_String
				RT_Image image = RT_LoadDDSFromDisk(&g_thread_arena, RT_StringFromCString(dds_file));

				if (is_srgb)
				{
					image.format = RT_TextureFormatToSRGB(image.format);
				}

				if (image.pixels)
				{
					dds_loaded = true;

					material->textures[i] = RT_UploadTexture(&(RT_UploadTextureParams) {
						.image = image,
						.name  = RT_ArenaPrintF(&g_thread_arena, "Game Texture %hu:%s (source: %s)", bm_index, g_rt_texture_slot_names[i], dds_file),
					});

					textures_reloaded += 1;
				}
			}

			if (!dds_loaded) // dds file not loaded try png
			{
				RT_ArenaMemoryScope(&g_thread_arena)
				{
					char* file = NULL;

					// check for png extension to already be part of texture path.
					if (strstr(paths->textures[i], ".png"))
					{
						file = RT_ArenaPrintF(&g_thread_arena, "assets/textures/%s", paths->textures[i]);
					}
					else
					{
						file = RT_ArenaPrintF(&g_thread_arena, "assets/textures/%s.png", paths->textures[i]);
					}

					RT_TextureFormat format = g_rt_material_texture_slot_formats[i];

					bool is_srgb = format == RT_TextureFormat_RGBA8_SRGB;
					int  bpp     = g_rt_texture_format_bpp[format];

					RT_Image image = RT_LoadImageFromDisk(&g_thread_arena, file, bpp, is_srgb);

					// NOTE(daniel): Mildly sketchy code, that format check is checking it's an uncompressed format.
					if (i == RT_MaterialTextureSlot_Emissive && image.format == format)
					{
						// Premultiply emissive by alpha to avoid white transparent backgrounds from showing...

						uint32_t* at = (uint32_t*)image.pixels;
						for (size_t i = 0; i < image.width * image.height; i++)
						{
							RT_Vec4 color = RT_UnpackRGBA(*at);
							color.xyz = RT_Vec3Muls(color.xyz, color.w);
							*at++ = RT_PackRGBA(color);
						}
					}

					if (image.pixels)
					{
						material->textures[i] = RT_UploadTexture(&(RT_UploadTextureParams) {
							.image = image,
							.name  = RT_ArenaPrintF(&g_thread_arena, "Game Texture %hu:%s (source: %s)", bm_index, g_rt_texture_slot_names[i], file),
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

	// define a list of textures that always need to be loaded (sprites for example that can spawn at anytime)
	char texture_always_load_list[][13] = //{"none"};
	{
		"blob01#0","blob01#1","blob01#2","blob01#3","blob01#4",
		"blob02#0","blob02#1","blob02#2","blob02#3","blob02#4",
		"blob03#0","blob03#1","blob03#2","blob03#3","blob03#4",
		"blown01","blown02","blown03","blown04","blown05","blown06","blown07",
		"boss02#0","boss02#1","boss02#2","boss02#3","boss02#4","boss02#5","boss02#6","boss02#7","boss02#8","boss02#9","boss02#10","boss02#11","boss02#12","boss02#13","boss02#14",
		"cloak#0","cloak#1","cloak#2","cloak#3","cloak#4","cloak#5","cloak#6","cloak#7","cloak#8","cloak#9","cloak#10","cloak#11","cloak#12","cloak#13","cloak#14","cloak#15",
		"cmissil1#0","cmissil1#1","cmissil1#2","cmissil1#3","cmissil1#4","cmissil1#5","cmissil1#6","cmissil1#7","cmissil1#8","cmissil1#9","cmissil1#10","cmissil1#11","cmissil1#12","cmissil1#13","cmissil1#14",
		"cmissil2#0","cmissil2#1","cmissil2#2","cmissil2#3","cmissil2#4","cmissil2#5","cmissil2#6","cmissil2#7","cmissil2#8","cmissil2#9","cmissil2#10","cmissil2#11","cmissil2#12","cmissil2#13","cmissil2#14",
		"exp03#0","exp03#1","exp03#2","exp03#3","exp03#4",
		"exp06#0","exp06#1","exp06#2","exp06#3","exp06#4","exp06#5","exp06#6","exp06#7","exp06#8","exp06#9","exp06#10","exp06#11",
		"exp09#0","exp09#1","exp09#2","exp09#3","exp09#4",
		"exp10#0","exp10#1","exp10#2","exp10#3","exp10#4",
		"exp11#0","exp11#1","exp11#2","exp11#3","exp11#4",
		"exp13#0","exp13#1","exp13#2","exp13#3","exp13#4","exp13#5","exp13#6","exp13#7","exp13#8","exp13#9","exp13#10","exp13#11","exp13#12","exp13#13","exp13#14","exp13#15","exp13#16",
		"exp15#0","exp15#1","exp15#2","exp15#3","exp15#4","exp15#5","exp15#6","exp15#7","exp15#8","exp15#9","exp15#10","exp15#11","exp15#12","exp15#13","exp15#14","exp15#15","exp15#16",
		"exp17#0","exp17#1","exp17#2","exp17#3","exp17#4","exp17#5","exp17#6","exp17#7","exp17#8","exp17#9","exp17#10","exp17#11","exp17#12","exp17#13","exp17#14","exp17#15","exp17#16",
		"exp18#0","exp18#1","exp18#2","exp18#3","exp18#4","exp18#5","exp18#6","exp18#7","exp18#8",
		"exp19#0","exp19#1","exp19#2","exp19#3","exp19#4","exp19#5","exp19#6","exp19#7","exp19#8","exp19#9","exp19#10",
		"exp20#0","exp20#1","exp20#2","exp20#3","exp20#4","exp20#5","exp20#6","exp20#7","exp20#8","exp20#9",
		"exp21#0","exp21#1","exp21#2","exp21#3","exp21#4","exp21#5","exp21#6","exp21#7","exp21#8","exp21#9","exp21#10","exp21#11","exp21#12",
		"exp22#0","exp22#1","exp22#2","exp22#3","exp22#4","exp22#5","exp22#6","exp22#7","exp22#8","exp22#9",
		"eye01#0","eye01#1","eye01#2","eye01#3","eye01#4","eye01#5",
		"eye02#0","eye02#1","eye02#2","eye02#3","eye02#4","eye02#5","eye02#6","eye02#7","eye02#8","eye02#9","eye02#10",
		"eye03#0","eye03#1","eye03#2","eye03#3","eye03#4","eye03#5","eye03#6","eye03#7","eye03#8","eye03#9",
		"fusion#0","fusion#1","fusion#2","fusion#3","fusion#4","fusion#5","fusion#6","fusion#7","fusion#8","fusion#9","fusion#10","fusion#11","fusion#12","fusion#13","fusion#14",
		"gauge01#0","gauge01#1","gauge01#2","gauge01#3","gauge01#4","gauge01#5","gauge01#6","gauge01#7","gauge01#8","gauge01#9","gauge01#10","gauge01#11","gauge01#12","gauge01#13","gauge01#14","gauge01#15","gauge01#16","gauge01#17","gauge01#18","gauge01#19",
		"gauge02#0","gauge02#1","gauge02#2","gauge02#3","gauge02#4",
		"gauge03","gauge04","gauge05",
		"gauge06#0","gauge06#1","gauge06#2","gauge06#3","gauge06#4","gauge06#5","gauge06#6","gauge06#7",
		"gauge07","gauge08","gauge09","gauge10","gauge11","gauge12","gauge13","gauge14","gauge15","gauge16","gauge17",
		"gauge18#0","gauge18#1","gauge18#2",
		"hmissil1#0","hmissil1#1","hmissil1#2","hmissil1#3","hmissil1#4","hmissil1#5","hmissil1#6","hmissil1#7","hmissil1#8","hmissil1#9","hmissil1#10","hmissil1#11","hmissil1#12","hmissil1#13","hmissil1#14",
		"hmissil2#0","hmissil2#1","hmissil2#2","hmissil2#3","hmissil2#4","hmissil2#5","hmissil2#6","hmissil2#7","hmissil2#8","hmissil2#9","hmissil2#10","hmissil2#11","hmissil2#12","hmissil2#13","hmissil2#14",
		"hostage#0","hostage#1","hostage#2","hostage#3","hostage#4","hostage#5","hostage#6","hostage#7",
		"invuln#0","invuln#1","invuln#2","invuln#3","invuln#4","invuln#5","invuln#6","invuln#7","invuln#8","invuln#9","invuln#10","invuln#11","invuln#12","invuln#13","invuln#14","invuln#15",
		"key01#0","key01#1","key01#2","key01#3","key01#4","key01#5","key01#6","key01#7","key01#8","key01#9","key01#10","key01#11","key01#12","key01#13","key01#14",
		"key02#0","key02#1","key02#2","key02#3","key02#4","key02#5","key02#6","key02#7","key02#8","key02#9","key02#10","key02#11","key02#12","key02#13","key02#14",
		"key03#0","key03#1","key03#2","key03#3","key03#4","key03#5","key03#6","key03#7","key03#8","key03#9","key03#10","key03#11","key03#12","key03#13","key03#14",
		"laser#0","laser#1","laser#2","laser#3","laser#4","laser#5","laser#6","laser#7","laser#8","laser#9","laser#10","laser#11","laser#12","laser#13","laser#14",
		"life01#0","life01#1","life01#2","life01#3","life01#4","life01#5","life01#6","life01#7","life01#8","life01#9","life01#10","life01#11","life01#12","life01#13","life01#14",
		"lives",
		"misc17#0","misc17#1","misc17#2","misc17#3","misc17#4","misc17#5","misc17#6","misc17#7","misc17#8","misc17#9","misc17#10","misc17#11","misc17#12","misc17#13","misc17#14","misc17#15",
		"misc060b#0","misc060b#1","misc060b#2","misc060d#0","misc060d#1","misc060d#2",
		"misc061b#0","misc061b#1","misc061b#2","misc061d#0","misc061d#1","misc061d#2",
		"misc062b#0","misc062b#1","misc062b#2","misc062d#0","misc062d#1","misc062d#2",
		"misc065b#0","misc065b#1","misc065b#2","misc065d#0","misc065d#1","misc065d#2",
		"misc066b#0","misc066b#1","misc066b#2","misc066d#0","misc066d#1","misc066d#2","misc066d#3",
		"misc068b#0","misc068b#1","misc068b#2","misc068b#3","misc068d#0","misc068d#1","misc068d#2","misc068d#3","misc068d#4","misc068d#5","misc068d#6","misc068d#7","misc068d#8","misc068d#9",
		"missile",
		"mmissile#0","mmissile#1","mmissile#2","mmissile#3","mmissile#4","mmissile#5","mmissile#6","mmissile#7","mmissile#8","mmissile#9","mmissile#10","mmissile#11","mmissile#12","mmissile#13","mmissile#14",
		"mntr04b#0","mntr04b#1","mntr04b#2","mntr04d#0","mntr04d#1","mntr04d#2",
		"mtrl01#0","mtrl01#1","mtrl01#2","mtrl01#3","mtrl01#4","mtrl01#5","mtrl01#6","mtrl01#7","mtrl01#8","mtrl01#9","mtrl01#10","mtrl01#11","mtrl01#12","mtrl01#13","mtrl01#14","mtrl01#15","mtrl01#16","mtrl01#17",
		"mtrl02#0","mtrl02#1","mtrl01#2","mtrl02#3","mtrl02#4","mtrl02#5","mtrl02#6","mtrl02#7","mtrl02#8","mtrl02#9","mtrl02#10","mtrl02#11",
		"mtrl03#0","mtrl03#1","mtrl03#2","mtrl03#3","mtrl03#4","mtrl03#5","mtrl03#6","mtrl03#7","mtrl03#8",
		"muzl02#0","muzl02#1","muzl02#2",
		"muzl03#0","muzl03#1","muzl03#2",
		"muzl05#0","muzl05#1","muzl05#2",
		"muzl06#0","muzl06#1","muzl06#2",
		"pbomb#0","pbomb#1","pbomb#2","pbomb#3","pbomb#4","pbomb#5","pbomb#6","pbomb#7","pbomb#8","pbomb#9",
		"pbombs#0","pbombs#1","pbombs#2","pbombs#3","pbombs#4","pbombs#5","pbombs#6","pbombs#7","pbombs#8","pbombs#9",
		"plasblob#0","plasblob#1","plasblob#2","plasblob#3","plasblob#4","plasblob#5",
		"plasma#0","plasma#1","plasma#2","plasma#3","plasma#4","plasma#5","plasma#6","plasma#7","plasma#8","plasma#9","plasma#10","plasma#11","plasma#12","plasma#13","plasma#14",
		"pwr01#0","pwr01#1","pwr01#2","pwr01#3","pwr01#4","pwr01#5","pwr01#6","pwr01#7","pwr01#8","pwr01#9","pwr01#10","pwr01#11","pwr01#12","pwr01#13","pwr01#14",
		"pwr02#0","pwr02#1","pwr02#2","pwr02#3","pwr02#4","pwr02#5","pwr02#6","pwr02#7",
		"quad#0","quad#1","quad#2","quad#3","quad#4","quad#5","quad#6","quad#7","quad#8","quad#9","quad#10","quad#11","quad#12","quad#13","quad#14",
		"rbot006","rbot007","rbot008","rbot009","rbot010","rbot011","rbot012","rbot018","rbot019","rbot020","rbot021",
		"rbot031","rbot032","rbot035","rbot036","rbot037","rbot042","rbot044","rbot045","rbot046","rbot047","rbot048",
		"rbot049","rbot051","rbot052","rbot053","rbot054","rbot055","rbot056","rbot057","rbot058","rbot059","rbot060",
		"rbot061","rbot062","rbot063","rbot064","rbot065",
		"rmap01#0","rmap01#1","rmap01#2","rmap01#3","rmap01#4","rmap01#5","rmap01#6","rmap01#7",
		"rmap02#0","rmap02#1","rmap02#2",
		"rmap03#0","rmap03",
		"rmap04#0","rmap04#1","rmap04#2","rmap04#3","rmap04#4",
		"sbenergy","sbkb_off","sbkb_on","sbkr_off","sbkr_on","sbky_off","sbky_on",
		"ship1-1","ship1-2","ship1-3","ship1-4","ship1-5",
		"ship2-4","ship2-5",
		"ship3-4","ship3-5",
		"ship4-4","ship4-5",
		"ship5-4","ship5-5",
		"ship6-4","ship6-5",
		"ship7-4","ship7-5",
		"ship8-4","ship8-5",
		"smissile#0","smissile#1","smissile#2","smissile#3","smissile#4","smissile#5","smissile#6","smissile#7","smissile#8","smissile#9","smissile#10","smissile#11","smissile#12","smissile#13","smissile#14",
		"spark01#0","spark01#1","spark01#2","spark01#3","spark01#4",
		"spark02#0","spark02#1","spark02#2","spark02#3","spark02#4",
		"spark03#0","spark03#1","spark03#2","spark03#3","spark03#4",
		"spark04#0","spark04#1","spark04#2","spark04#3","spark04#4",
		"spark05#0","spark05#1","spark05#2","spark05#3","spark05#4",
		"sprdblob",
		"spread#0","spread#1","spread#2","spread#3","spread#4","spread#5","spread#6","spread#7","spread#8","spread#9","spread#10","spread#11","spread#12","spread#13","spread#14",
		"targ01#0","targ01#1",
		"targ02#0","targ02#1","targ02#2",
		"targ03#0","targ03#1","targ03#2","targ03#3","targ03#4",
		"targ04#0","targ04#1",
		"targ05#0","targ05#1","targ05#2",
		"targ06#0","targ06#1","targ06#2","targ06#3","targ06#4",
		"vammo#0","vammo#1","vammo#2","vammo#3","vammo#4","vammo#5","vammo#6","vammo#7","vammo#8","vammo#9","vammo#10","vammo#11","vammo#12","vammo#13","vammo#14",
		"vulcan#0","vulcan#1","vulcan#2","vulcan#3","vulcan#4","vulcan#5","vulcan#6","vulcan#7","vulcan#8","vulcan#9","vulcan#10","vulcan#11","vulcan#12","vulcan#13","vulcan#14",
	};

	//printf("Found %d Always Load Textures\n", (int)RT_ARRAY_COUNT(texture_always_load_list));

	// mark all the always load materials
	for (int i = 0; i < RT_ARRAY_COUNT(texture_always_load_list); i++)
	{
		bitmap_index game_texture = piggy_find_bitmap(texture_always_load_list[i]);
		if (game_texture.index > 0) 
		{
			g_rt_materials[game_texture.index].always_load_texture = true;
		}
		else
		{
			printf("Always Load Texture Not Found in Bitmap List: %s\n", texture_always_load_list[i]);
		}
	}
	
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
			RT_MaterialPathsExist* paths_exist = &g_rt_material_paths_exist[bm_index];

			// ------------------------------------------------------------------
			// Initialize all the defaults
			material->texture_load_state = RT_MaterialTextureLoadState_Unloaded;

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
			snprintf(paths->height_texture, sizeof(paths->height_texture), "%s_height", bitmap_name);

			// ------------------------------------------------------------------

			memcpy(&g_rt_materials_default[bm_index], material, sizeof(*material));
			memcpy(&g_rt_material_paths_default[bm_index], paths, sizeof(*paths));

			bool default_metalness, default_roughness;
            RT_ParseMaterialDefinitionFile(bm_index, material, paths, &default_metalness, &default_roughness);

			RT_VerifyMaterialTexturesFromPaths(bm_index, paths, paths_exist, ~0u);

			if (default_metalness && paths_exist->metalness_texture)
			{
				material->metalness = 1.0f;
				// AND ALSO make note of this quirk for the defaults, where after all that we should store the default
				// metalness and roughness if it was affected by whether or not there was a metal/roughness map:
				g_rt_materials_default[bm_index].metalness = material->metalness;
			}

			if (default_roughness && paths_exist->roughness_texture)
			{
				material->roughness = 1.0f;
				// AND ALSO make note of this quirk for the defaults, where after all that we should store the default
				// metalness and roughness if it was affected by whether or not there was a metal/roughness map:
				g_rt_materials_default[bm_index].roughness = material->roughness;
			}
		}

		RT_SyncMaterialStates();

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

void RT_SyncMaterialStates(void) 
{

	for (uint16_t bm_index = 1; bm_index < MAX_BITMAP_FILES; bm_index++)
	{
		char bitmap_name[13];
		piggy_get_bitmap_name(bm_index, bitmap_name);

		// NOTE(daniel): Substance Designer does not like hashtags in names and
		// turns them into underscores.
		for (char* c = bitmap_name; *c; c++)
		{
			if (*c == '#')
			{
				*c = '_';
			}
		}

		// Try to load the material definition
		RT_Material* material = &g_rt_materials[bm_index];
		RT_MaterialPaths* paths = &g_rt_material_paths[bm_index];

		if (material->always_load_texture || ( material->texture_load_state != material->texture_load_state_next))
		{
			if ((material->always_load_texture && material->texture_load_state == RT_MaterialTextureLoadState_Unloaded) || (material->texture_load_state == RT_MaterialTextureLoadState_Unloaded && material->texture_load_state_next == RT_MaterialTextureLoadState_Loaded ) )
			{
				// Load the material

				RT_ArenaMemoryScope(&g_thread_arena)
				{
					
					RT_LoadMaterialTexturesFromPaths(bm_index, material, paths, ~0u);
					
					material->texture_load_state = RT_MaterialTextureLoadState_Loaded;

					RT_UpdateMaterial(bm_index, material);
				}
			}
			else if (!material->always_load_texture && (material->texture_load_state == RT_MaterialTextureLoadState_Loaded && material->texture_load_state_next == RT_MaterialTextureLoadState_Unloaded) )
			{
				// Unload the material
				for (size_t i = 0; i < RT_MaterialTextureSlot_COUNT; i++)
				{
					if (RT_RESOURCE_HANDLE_VALID(material->textures[i]))
						RT_ReleaseTexture(material->textures[i]);
							
				}
				material->texture_load_state = RT_MaterialTextureLoadState_Unloaded;

				RT_UpdateMaterial(bm_index, material);
				
				// load the original low res texture (for the material viewer)
				RT_ArenaMemoryScope(&g_thread_arena)
				{
					grs_bitmap* bitmap = &GameBitmaps[bm_index];

					if (bitmap->bm_w == 0 ||
						bitmap->bm_h == 0)
					{
						continue;
					}

					PIGGY_PAGE_IN((bitmap_index) { bm_index });

					if (bitmap->bm_flags & BM_FLAG_RLE)
					{
						bitmap = rle_expand_texture(bitmap);
					}

					if (bitmap->bm_flags & BM_FLAG_RLE)
					{
						bitmap = rle_expand_texture(bitmap);
					}

					uint32_t* pixels = dx12_load_bitmap_pixel_data(&g_thread_arena, bitmap);

					material->albedo_texture = RT_UploadTexture(&(RT_UploadTextureParams) {
						.image.width = bitmap->bm_w,
							.image.height = bitmap->bm_h,
							.image.pixels = pixels,
							.image.format = g_rt_material_texture_slot_formats[RT_MaterialTextureSlot_Albedo],
							.name = RT_ArenaPrintF(&g_thread_arena, "Game Texture %hu:basecolor (original)", bm_index),
					});

#ifdef RT_DUMP_GAME_BITMAPS
					{
						const char* png_path = RT_ArenaPrintF(&g_thread_arena, "assets/texture_dump/%s.png", bitmap_name);
						RT_WritePNGToDisk(png_path, bitmap->bm_w, bitmap->bm_h, 4, pixels, 4 * bitmap->bm_w);
					}
#endif
				}

				RT_UpdateMaterial(bm_index, material);
			}
		}

		// ensure that an albedo texture is present, if not load the original texture (mainly for material viewer)
		
		if (!RT_RESOURCE_HANDLE_VALID(material->albedo_texture))
		{
			RT_ArenaMemoryScope(&g_thread_arena)
			{
				grs_bitmap* bitmap = &GameBitmaps[bm_index];

				if (bitmap->bm_w == 0 ||
					bitmap->bm_h == 0)
				{
					continue;
				}

				PIGGY_PAGE_IN((bitmap_index) { bm_index });


				if (bitmap->bm_flags & BM_FLAG_RLE)
				{
					bitmap = rle_expand_texture(bitmap);
				}

				if (bitmap->bm_flags & BM_FLAG_RLE)
				{
					bitmap = rle_expand_texture(bitmap);
				}

				uint32_t* pixels = dx12_load_bitmap_pixel_data(&g_thread_arena, bitmap);

				material->albedo_texture = RT_UploadTexture(&(RT_UploadTextureParams) {
					.image.width = bitmap->bm_w,
						.image.height = bitmap->bm_h,
						.image.pixels = pixels,
						.image.format = g_rt_material_texture_slot_formats[RT_MaterialTextureSlot_Albedo],
						.name = RT_ArenaPrintF(&g_thread_arena, "Game Texture %hu:basecolor (original)", bm_index),
				});

#ifdef RT_DUMP_GAME_BITMAPS
				{
					const char* png_path = RT_ArenaPrintF(&g_thread_arena, "assets/texture_dump/%s.png", bitmap_name);
					RT_WritePNGToDisk(png_path, bitmap->bm_w, bitmap->bm_h, 4, pixels, 4 * bitmap->bm_w);
				}
#endif
			}
			
			RT_UpdateMaterial(bm_index, material);
		}
	}

	piggy_bitmap_page_out_all();
}

