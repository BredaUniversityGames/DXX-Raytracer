#include "physfs.h"

#include "Lights.h"
#include "textures.h"
#include "dx12.h"
#include "gr.h"
#include "RTgr.h"
#include "RTmaterials.h"

#include "Core/Arena.h"
#include "Core/Config.h"
#include "Core/String.h"

float g_light_multiplier = 1.0;
float g_light_multiplier_default = 1.0;
const float FLT_MAX = 3.402823466e+38F;

typedef struct RT_SettingsNotification
{
	float timer;
	ImVec4 color;
	char *message;
} RT_SettingsNotification;

static inline void RT_SetSettingsNotification(RT_SettingsNotification *notif, char *name, ImVec4 color)
{
	notif->timer = 2.0f;
	notif->color = color;
	notif->message = name;
}

static inline void RT_ShowSettingsNotification(RT_SettingsNotification *notif)
{
	if (notif->timer > 0.0f)
	{
		ImVec4 color = notif->color;
		color.w = notif->timer / 2.0f;

		igPushStyleColor_Vec4(ImGuiCol_Text, color);
		igText(notif->message);
		igPopStyleColor(1);

		notif->timer -= 1.0f / 60.0f; // hardcoded nonsense
	}
	else
	{
		igDummy((ImVec2){0.0f, igGetFontSize()});
	}
}

static RT_SettingsNotification g_lights_notification;
static RT_SettingsNotification g_headlights_notification;

RT_LightDefinition g_light_definitions[] =
{
	// White light fixtures
	{
		.name = "ceil002",
		.kind = RT_LightKind_Area_Rect,
		.emission = {10.f,10.f,0.f},
		.radius = 0.5f,
		.spot_angle = 0.3f,
		.spot_softness = 0.2f,
	},
	{
		.name = "ceil003",
		.kind = RT_LightKind_Area_Rect,
		.emission = {10.f,10.f,0.f},
		.radius = 0.5f,
		.spot_angle = 0.3f,
		.spot_softness = 0.2f,
	},
	{
		.name = "ceil008",
		.kind = RT_LightKind_Area_Rect,
		.emission = {10.f,10.f,0.f},
		.radius = 0.5f,
		.spot_angle = 0.3f,
		.spot_softness = 0.2f,
	},
	{
		.name = "ceil020",
		.kind = RT_LightKind_Area_Rect,
		.emission = {8.f,8.f,8.f},
		.radius = 0.3f,
		.spot_angle = 0.3f,
		.spot_softness = 0.2f,
	},
	// White light fixtures
	{
		.name = "ceil021",
		.kind = RT_LightKind_Area_Rect,
		.emission = {8.f,8.f,8.f},
		.radius = 0.5f,
		.spot_angle = 0.3f,
		.spot_softness = 0.2f,
	},
	{
		.name = "ceil023",
		.kind = RT_LightKind_Area_Rect,
		.emission = {8.f,8.f,8.f},
		.radius = 0.5f,
		.spot_angle = 0.3f,
		.spot_softness = 0.2f,
	},
	{
		.name = "ceil024",
		.kind = RT_LightKind_Area_Rect,
		.emission = {8.f,8.f,8.f},
		.size = {1.0, 1.0},
		.spot_angle = 0.3f,
		.spot_softness = 0.2f,
	},
	{
		.name = "ceil025",
		.kind = RT_LightKind_Area_Rect,
		.emission = {8.f,8.f,8.f},
		.radius = 0.3f,
		.spot_angle = 0.3f,
		.spot_softness = 0.2f,
	},
	{
		.name = "ceil026",
		.kind = RT_LightKind_Area_Rect,
		.emission = {8.f,8.f,8.f},
		.radius = 0.3f,
		.spot_angle = 0.3f,
		.spot_softness = 0.2f,
	},
	{
		.name = "ceil027",
		.kind = RT_LightKind_Area_Rect,
		.emission = {8.f,8.f,8.f},
		.radius = 0.3f,
		.spot_angle = 0.3f,
		.spot_softness = 0.2f,
	},
	{
		.name = "ceil028",
		.kind = RT_LightKind_Area_Rect,
		.emission = {8.f,8.f,8.f},
		.radius = 0.4f,
		.spot_angle = 0.3f,
		.spot_softness = 0.2f,
	},
	{
		.name = "ceil029",
		.kind = RT_LightKind_Area_Rect,
		.emission = {8.f,8.f,8.f},
		.radius = 0.8f,
		.spot_angle = 0.3f,
		.spot_softness = 0.2f,
	},
	{
		.name = "ceil030",
		.kind = RT_LightKind_Area_Rect,
		.emission = {8.f,8.f,8.f},
		.radius = 0.8f,
		.spot_angle = 0.3f,
		.spot_softness = 0.2f,
	},
	{
		.name = "ceil031",
		.kind = RT_LightKind_Area_Rect,
		.emission = {8.f,8.f,8.f},
		.radius = 0.8f,
		.spot_angle = 0.3f,
		.spot_softness = 0.2f,
	},
	{
		.name = "ceil034",
		.kind = RT_LightKind_Area_Rect,
		.emission = {8.f,8.f,8.f},
		.radius = 0.8f,
		.spot_angle = 0.3f,
		.spot_softness = 0.2f,
	},
	{
		.name = "ceil035",
		.kind = RT_LightKind_Area_Rect,
		.emission = {8.f,8.f,8.f},
		.radius = 0.8f,
		.spot_angle = 0.3f,
		.spot_softness = 0.2f,
	},
	{
		.name = "ceil036",
		.kind = RT_LightKind_Area_Rect,
		.emission = {8.f,8.f,8.f},
		.radius = 0.8f,
		.spot_angle = 0.3f,
		.spot_softness = 0.2f,
	},
};

static RT_LightDefinition g_default_light_definitions[RT_ARRAY_COUNT(g_light_definitions)];

static RT_HeadlightSettings g_default_headlights = {
	.pos_offset_horz = 3.0f,
	.pos_offset_vert = -2.0f,
	.skew_horz       = 0.1f,
	.skew_vert       = 0.06f,
	.radius          = 0.05f,
	.brightness      = 1.5f,
	.spot_angle      = 0.05f,
	.spot_softness   = 0.05f,
};

void RT_InitLightStuff()
{
	memcpy(&g_headlights, &g_default_headlights, sizeof(g_headlights));
	memcpy(g_default_light_definitions, g_light_definitions, sizeof(g_default_light_definitions));

	RT_LoadLightSettings();
	g_lights_notification.timer = 0.0f;

	RT_LoadHeadLightSettings();
	g_headlights_notification.timer = 0.0f;
}

void RT_ResetLightSettings()
{
	g_light_multiplier_default = g_light_multiplier = 1.0f;

	memcpy(g_light_definitions, g_default_light_definitions, sizeof(g_default_light_definitions));
	g_pending_light_update = true;

	RT_SetSettingsNotification(&g_lights_notification, "Reset Light Settings", (ImVec4){0.5f, 0.7f, 1.0f, 1.0f});
}

int RT_IsLight(int tmap) 
{
	//TODO: If this starts to become a performance bottleneck, add a hashmap so the lookups can be done in O(1)
	for (int i = 0; i < RT_ARRAY_COUNT(g_light_definitions); i++)
	{
		//NOTE (sam)
		//Local char array to avoid write access violation on string literals.
		char lChar[128] = { 0 };
		size_t sSize = strlen(g_light_definitions[i].name);
		if (sSize > 127)
		{
			sSize = 127;
			RT_LOG(RT_LOGSERVERITY_ASSERT, "Hashtable search has a string that is bigger then 127 bytes.");
		}

		memcpy(lChar, g_light_definitions[i].name, sSize);

		bitmap_index game_texture = piggy_find_bitmap(lChar);
		if (game_texture.index == Textures[tmap].index && game_texture.index > 0) {
			return i;
		}
	}

	return -1;
}

RT_Light RT_InitLight(RT_LightDefinition definition, RT_Vertex* vertices, RT_Vec3 normal)
{
	RT_Light light = { 0 };

	light.kind     = definition.kind;
	light.emission = RT_PackRGBE(definition.emission); 

	light.spot_angle    = RT_Uint8FromFloat(definition.spot_angle);
	light.spot_softness = RT_Uint8FromFloat(definition.spot_softness);

	RT_Vec3 center = vertices[0].pos;
	center = RT_Vec3Add(center, vertices[1].pos);
	center = RT_Vec3Add(center, vertices[2].pos);
	center = RT_Vec3Add(center, vertices[3].pos);
	center = RT_Vec3Muls(center, 0.25f);

	// Offset position with normal to prevent light clipping with walls.
	RT_Vec3 position = RT_Vec3MulsAdd(center, normal, 0.015f);

	switch (definition.kind)
	{
		case RT_LightKind_Area_Sphere:
		{
			float r = definition.radius;
			RT_Mat34 transform = 
			{
				.e = 
				{
					r, 0, 0, position.x,
					0, r, 0, position.y,
					0, 0, r, position.z,
				}
			};
			light.transform = transform;
		} break;

		case RT_LightKind_Area_Rect:
		{
			RT_Vec3   tangent = RT_Vec3Muls(RT_Vec3Sub(vertices[1].pos, vertices[0].pos), 0.5f);
			RT_Vec3 bitangent = RT_Vec3Muls(RT_Vec3Sub(vertices[3].pos, vertices[0].pos), 0.5f);
			light.transform = RT_Mat34FromColumns(tangent, normal, bitangent, position);
		} break;
	}

	return light;
}


void RT_ShowLightMenu()
{
	igBegin("Light Explorer", NULL, ImGuiWindowFlags_AlwaysAutoResize);

	igText("Headlights");
	igPushID_Str("Headlights");
	{
		if (igButton("Load Settings", (ImVec2){0, 0}))
		{
			RT_LoadHeadLightSettings();
		}

		igSameLine(0.0f, -1.0f);

		if (igButton("Save Settings", (ImVec2){0, 0}))
		{
			RT_SaveHeadLightSettings();
		}

		igSameLine(0.0f, -1.0f);

		if (igButton("Reset Settings", (ImVec2){0, 0}))
		{
			RT_ResetHeadLightSettings();
		}

		RT_ShowSettingsNotification(&g_headlights_notification);

		RT_HeadlightSettings *h = &g_headlights;
		igSliderFloat("Horizontal Position Offset", &h->pos_offset_horz, 0.0f, 15.0f, "%.02f", 0);
		igSliderFloat("Vertical Position Offset", &h->pos_offset_vert, -5.0f, 5.0f, "%.02f", 0);
		igSliderFloat("Horizontal Skew", &h->skew_horz, 0.0f, 0.4f, "%.02f", 0);
		igSliderFloat("Vertical Skew", &h->skew_vert, -0.2f, 0.2f, "%.02f", 0);
		igSliderFloat("Light Radius", &h->radius, 0.01f, 0.4f, "%.02f", 0);
		igSliderFloat("Brightness", &h->brightness, 0.0f, 5.0f, "%.02f", 0);
		igSliderFloat("Spot Angle", &h->spot_angle, 0.01f, 0.12f, "%.02f", 0);
		igSliderFloat("Spot Softness", &h->spot_softness, 0.01f, 0.12f, "%.02f", 0);
		igDummy((ImVec2){0.0f, igGetFontSize()});
	}
	igPopID(1);

	igText("Level Lights");

	if (igButton("Load Settings", (ImVec2){0, 0}))
	{
		RT_LoadLightSettings();
	}

	igSameLine(0.0f, -1.0f);

	if (igButton("Save Settings", (ImVec2){0, 0}))
	{
		RT_SaveLightSettings();
	}

	igSameLine(0.0f, -1.0f);

	if (igButton("Reset Settings", (ImVec2){0, 0}))
	{
		RT_ResetLightSettings();
	}

	RT_ShowSettingsNotification(&g_lights_notification);

	if (igSliderFloat("Global brightness", &g_light_multiplier_default, 0.000001f, 15.0f, "%f", ImGuiSliderFlags_Logarithmic)) {
		g_light_multiplier = g_light_multiplier_default;
		g_pending_light_update = true;
	}
	
	igText("Current active light count: %i / %i", g_active_lights, RT_MAX_LIGHTS);
	
	if (igCollapsingHeader_TreeNodeFlags("Light definitions", ImGuiTreeNodeFlags_None)) {
		int light_definitions_count = sizeof(g_light_definitions) / sizeof(RT_LightDefinition);
		igIndent(10.5f);

		for (int i = 0; i < light_definitions_count; i++)
		{
			igPushID_Int(i);
			RT_LightDefinition* light = &g_light_definitions[i];

			char light_name[128];
			strncpy(light_name, light->name, 128);

			ushort texture_index = piggy_find_bitmap(light_name).index;

			if (texture_index > 0){
				char* light_name_buffer = RT_ArenaPrintF(&g_thread_arena, "Light: %s", light->name);
				if (igCollapsingHeader_TreeNodeFlags(light_name_buffer, ImGuiTreeNodeFlags_None)) {

					igText("Texture index: %i", texture_index);

					RT_Material *def = &g_rt_materials[texture_index];
					RT_RenderImGuiTexture(def->albedo_texture, 64.0, 64.0);

					igText("Emission: {%f, %f, %f}", light->emission.x, light->emission.y, light->emission.z);

					if (igColorEdit3("Emission", &light->emission, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR)){
						g_pending_light_update = true;
					}

					igSliderFloat("Spot Angle", &light->spot_angle, 0.0f, 1.0f, "%f", ImGuiSliderFlags_None);
					igSliderFloat("Light Radius", &light->spot_softness, 0.0f, 1.0f, "%f", ImGuiSliderFlags_None);

					if (light->kind == RT_LightKind_Area_Sphere){
						igText("Radius: %f", light->radius);
						igSliderFloat("Light Radius", &light->radius, 0.0f, 10.0f, "%f", ImGuiSliderFlags_None);
						g_pending_light_update = true;
					}
				}
			}
			igPopID();
		}
	}

	if (igSmallButton("Toggle light debugging"))
	{
		g_light_visual_debug = !g_light_visual_debug;
	}

	igEnd();
}

void RT_VisualizeLight(RT_Light* light)
{
	RT_Vec3 pos = RT_TranslationFromMat34(light->transform);
	switch (light->kind)
	{
		case RT_LightKind_Area_Rect:
			RT_Vec3 normal = RT_Vec3Make(light->transform.r0.y, light->transform.r1.y, light->transform.r2.y);
			RT_RasterLineWorld(pos, RT_Vec3Add(pos, RT_Vec3Muls(normal, 10.0)), RT_Vec4Make(1.0, 0.0, 1.0, 1.0));

			RT_Vec3 tangent = RT_Vec3Make(light->transform.e[0][0],light->transform.e[1][0],light->transform.e[2][0]);
			RT_Vec3 bitangent = RT_Vec3Make(light->transform.e[0][2],light->transform.e[1][2],light->transform.e[2][2]);
			RT_Vec3 emission_unpack = RT_UnpackRGBE(light->emission);
			RT_Vec4 emission = RT_Vec4Make(emission_unpack.x, emission_unpack.y, emission_unpack.z, 1.0);

			RT_RasterLineWorld(RT_Vec3Sub(RT_Vec3Sub(pos, tangent),bitangent), RT_Vec3Sub(RT_Vec3Add(pos, tangent),bitangent), emission);
			RT_RasterLineWorld(RT_Vec3Sub(RT_Vec3Sub(pos, tangent),bitangent), RT_Vec3Sub(RT_Vec3Add(pos, bitangent),tangent), emission);
			RT_RasterLineWorld(RT_Vec3Add(RT_Vec3Add(pos, tangent),bitangent), RT_Vec3Sub(RT_Vec3Add(pos, tangent),bitangent), emission);
			RT_RasterLineWorld(RT_Vec3Add(RT_Vec3Add(pos, tangent),bitangent), RT_Vec3Sub(RT_Vec3Add(pos, bitangent),tangent), emission);

			RT_RasterLineWorld(RT_Vec3Sub(pos, tangent), RT_Vec3Add(pos, tangent), RT_Vec4Make(0.0, 1.0, 0.0, 1.0));
			RT_RasterLineWorld(RT_Vec3Sub(pos, bitangent), RT_Vec3Add(pos, bitangent), RT_Vec4Make(0.0, 0.0, 1.0, 1.0));
		break;

		case RT_LightKind_Area_Sphere:
			RT_Vec3 scale = RT_ScaleFromMat34(light->transform);
			
			RT_RasterLineWorld(pos, RT_Vec3Add(pos, RT_Vec3Make(scale.x,0.0,0.0)), RT_Vec4Make(1.0, 0.0, 0.0, 1.0));
			RT_RasterLineWorld(pos, RT_Vec3Add(pos, RT_Vec3Make(-scale.x,0.0,0.0)), RT_Vec4Make(1.0, 0.0, 0.0, 1.0));
			RT_RasterLineWorld(pos, RT_Vec3Add(pos, RT_Vec3Make(0.0,scale.y,0.0)), RT_Vec4Make(1.0, 0.0, 0.0, 1.0));
			RT_RasterLineWorld(pos, RT_Vec3Add(pos, RT_Vec3Make(0.0,-scale.y,0.0)), RT_Vec4Make(1.0, 0.0, 0.0, 1.0));
			RT_RasterLineWorld(pos, RT_Vec3Add(pos, RT_Vec3Make(0.0,0.0,scale.z)), RT_Vec4Make(1.0, 0.0, 0.0, 1.0));
			RT_RasterLineWorld(pos, RT_Vec3Add(pos, RT_Vec3Make(0.0,0.0,-scale.z)), RT_Vec4Make(1.0, 0.0, 0.0, 1.0));
		break;
	}
}

void RT_LoadLightSettings()
{
	PHYSFS_mkdir("lights");

	bool success = true;

	// ------------------------------------------------------------------
	// Read global light settings

	RT_ArenaMemoryScope(&g_thread_arena)
	{
		RT_Config *cfg = RT_ArenaAllocStruct(&g_thread_arena, RT_Config);
		RT_InitializeConfig(cfg, &g_thread_arena);

		if (RT_DeserializeConfigFromFile(cfg, "lights/global_lights.vars"))
		{
			RT_ConfigReadFloat(cfg, RT_StringLiteral("global_light_multiplier"), &g_light_multiplier_default);
			g_light_multiplier = g_light_multiplier_default;
		}
		else
		{
			success = false;
		}
	}

	// ------------------------------------------------------------------
	// Read all da rest

	for (size_t light_index = 0; light_index < RT_ARRAY_COUNT(g_light_definitions); light_index++)
	{
		RT_ArenaMemoryScope(&g_thread_arena)
		{
			RT_LightDefinition *def = &g_light_definitions[light_index];

			RT_Config *cfg = RT_ArenaAllocStruct(&g_thread_arena, RT_Config);
			RT_InitializeConfig(cfg, &g_thread_arena);

			char *file_name = RT_ArenaPrintF(&g_thread_arena, "lights/%s.vars", def->name);
			if (RT_DeserializeConfigFromFile(cfg, file_name))
			{
				RT_ConfigReadInt(cfg, RT_StringLiteral("kind"), &def->kind);
				RT_ConfigReadVec3(cfg, RT_StringLiteral("emission"), &def->emission);
				RT_ConfigReadFloat(cfg, RT_StringLiteral("radius"), &def->radius);
				RT_ConfigReadVec2(cfg, RT_StringLiteral("size"), &def->size);
				RT_ConfigReadFloat(cfg, RT_StringLiteral("spot_angle"), &def->spot_angle);
				RT_ConfigReadFloat(cfg, RT_StringLiteral("spot_softness"), &def->spot_softness);
			}
			else
			{
				success = false;
			}
		}
	}

	if (success)
	{
		RT_SetSettingsNotification(&g_lights_notification, "Successfully Loaded Light Settings", (ImVec4){0.5f, 1.0f, 0.7f, 1.0f});
	}
	else 
	{
		RT_SetSettingsNotification(&g_lights_notification, "Encountered Problems Loading Light Settings!", (ImVec4){1.0f, 0.2f, 0.2f, 1.0f});
	}

	g_pending_light_update = true;
}

void RT_SaveLightSettings()
{
	PHYSFS_mkdir("lights");

	bool success = true;

	// ------------------------------------------------------------------
	// Write global light settings

	RT_ArenaMemoryScope(&g_thread_arena)
	{
		RT_Config *cfg = RT_ArenaAllocStruct(&g_thread_arena, RT_Config);
		RT_InitializeConfig(cfg, &g_thread_arena);

		RT_ConfigWriteFloat(cfg, RT_StringLiteral("global_light_multiplier"), g_light_multiplier_default);

		if (!RT_SerializeConfigToFile(cfg, "lights/global_lights.vars"))
		{
			success = false;

			RT_LOG(RT_LOGSERVERITY_HIGH, "Failed to serialize global_lights.vars:\n");
			for (RT_StringNode *error = cfg->first_error; error; error = error->next)
			{
				RT_LOGF(RT_LOGSERVERITY_HIGH, "    > %.*s\n", RT_ExpandString(error->string));
			}
		}
	}

	// ------------------------------------------------------------------
	// Write all da rest

	for (size_t light_index = 0; light_index < RT_ARRAY_COUNT(g_light_definitions); light_index++)
	{
		RT_ArenaMemoryScope(&g_thread_arena)
		{
			RT_LightDefinition *def = &g_light_definitions[light_index];

			RT_Config *cfg = RT_ArenaAllocStruct(&g_thread_arena, RT_Config);
			RT_InitializeConfig(cfg, &g_thread_arena);

			RT_ConfigWriteInt(cfg, RT_StringLiteral("kind"), def->kind);
			RT_ConfigWriteVec3(cfg, RT_StringLiteral("emission"), def->emission);
			RT_ConfigWriteFloat(cfg, RT_StringLiteral("radius"), def->radius);
			RT_ConfigWriteVec2(cfg, RT_StringLiteral("size"), def->size);
			RT_ConfigWriteFloat(cfg, RT_StringLiteral("spot_angle"), def->spot_angle);
			RT_ConfigWriteFloat(cfg, RT_StringLiteral("spot_softness"), def->spot_softness);

			char *file_name = RT_ArenaPrintF(&g_thread_arena, "lights/%s.vars", def->name);
			if (!RT_SerializeConfigToFile(cfg, file_name))
			{
				success = false;

				RT_LOG(RT_LOGSERVERITY_HIGH, "Failed to serialize %s:\n", file_name);
				for (RT_StringNode *error = cfg->first_error; error; error = error->next)
				{
					RT_LOGF(RT_LOGSERVERITY_HIGH, "    > %.*s\n", RT_ExpandString(error->string));
				}
			}
		}
	}

	if (success)
	{
		RT_SetSettingsNotification(&g_lights_notification, "Successfully Saved Light Settings", (ImVec4){0.7f, 1.0f, 0.5f, 1.0f});
	}
	else 
	{
		RT_SetSettingsNotification(&g_lights_notification, "Encountered Problems Saving Light Settings!", (ImVec4){1.0f, 0.2f, 0.2f, 1.0f});
	}
}

void RT_ResetHeadLightSettings()
{
	memcpy(&g_headlights, &g_default_headlights, sizeof(g_headlights));
	RT_SetSettingsNotification(&g_headlights_notification, "Reset Headlight Settings", (ImVec4){0.5f, 0.7f, 1.0f, 1.0f});
}

void RT_LoadHeadLightSettings(void)
{
	bool success = true;

	RT_ArenaMemoryScope(&g_thread_arena)
	{
		RT_Config *cfg = RT_ArenaAllocStruct(&g_thread_arena, RT_Config);
		RT_InitializeConfig(cfg, &g_thread_arena);

		if (RT_DeserializeConfigFromFile(cfg, "lights/headlights.vars"))
		{
			RT_ConfigReadFloat(cfg, RT_StringLiteral("pos_offset_horz"), &g_headlights.pos_offset_horz);
			RT_ConfigReadFloat(cfg, RT_StringLiteral("pos_offset_vert"), &g_headlights.pos_offset_vert);
			RT_ConfigReadFloat(cfg, RT_StringLiteral("skew_horz"), &g_headlights.skew_horz);
			RT_ConfigReadFloat(cfg, RT_StringLiteral("skew_vert"), &g_headlights.skew_vert);
			RT_ConfigReadFloat(cfg, RT_StringLiteral("radius"), &g_headlights.radius);
			RT_ConfigReadFloat(cfg, RT_StringLiteral("brightness"), &g_headlights.brightness);
			RT_ConfigReadFloat(cfg, RT_StringLiteral("spot_angle"), &g_headlights.spot_angle);
			RT_ConfigReadFloat(cfg, RT_StringLiteral("spot_softness"), &g_headlights.spot_softness);
		}
		else
		{
			success = false;
		}
	}

	if (success)
	{
		RT_SetSettingsNotification(&g_headlights_notification, "Successfully Loaded Headlight Settings", (ImVec4){0.5f, 1.0f, 0.7f, 1.0f});
	}
	else 
	{
		RT_SetSettingsNotification(&g_headlights_notification, "Encountered Problems Loading Headlight Settings!", (ImVec4){1.0f, 0.2f, 0.2f, 1.0f});
	}
}

void RT_SaveHeadLightSettings(void)
{
	PHYSFS_mkdir("lights");

	bool success = true;

	RT_ArenaMemoryScope(&g_thread_arena)
	{
		RT_Config *cfg = RT_ArenaAllocStruct(&g_thread_arena, RT_Config);
		RT_InitializeConfig(cfg, &g_thread_arena);

		RT_ConfigWriteFloat(cfg, RT_StringLiteral("pos_offset_horz"), g_headlights.pos_offset_horz);
		RT_ConfigWriteFloat(cfg, RT_StringLiteral("pos_offset_vert"), g_headlights.pos_offset_vert);
		RT_ConfigWriteFloat(cfg, RT_StringLiteral("skew_horz"), g_headlights.skew_horz);
		RT_ConfigWriteFloat(cfg, RT_StringLiteral("skew_vert"), g_headlights.skew_vert);
		RT_ConfigWriteFloat(cfg, RT_StringLiteral("radius"), g_headlights.radius);
		RT_ConfigWriteFloat(cfg, RT_StringLiteral("brightness"), g_headlights.brightness);
		RT_ConfigWriteFloat(cfg, RT_StringLiteral("spot_angle"), g_headlights.spot_angle);
		RT_ConfigWriteFloat(cfg, RT_StringLiteral("spot_softness"), g_headlights.spot_softness);

		if (!RT_SerializeConfigToFile(cfg, "lights/headlights.vars"))
		{
			success = false;

			RT_LOG(RT_LOGSERVERITY_HIGH, "Failed to serialize global_lights.vars:\n");
			for (RT_StringNode *error = cfg->first_error; error; error = error->next)
			{
				RT_LOGF(RT_LOGSERVERITY_HIGH, "    > %.*s\n", RT_ExpandString(error->string));
			}
		}
	}

	if (success)
	{
		RT_SetSettingsNotification(&g_headlights_notification, "Successfully Saved Headlight Settings", (ImVec4){0.7f, 1.0f, 0.5f, 1.0f});
	}
	else 
	{
		RT_SetSettingsNotification(&g_headlights_notification, "Encountered Problems Saving Headlight Settings!", (ImVec4){1.0f, 0.2f, 0.2f, 1.0f});
	}
}
