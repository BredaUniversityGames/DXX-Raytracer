#include "Lights.h"
#include "textures.h"
#include "Core/Arena.h"
#include "dx12.h"
#include "gr.h"
#include "RTgr.h"
#include "RTmaterials.h"

float g_light_multiplier = 1.0;
float g_light_multiplier_default = 1.0;
const float FLT_MAX = 3.402823466e+38F;

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
	igBegin("Light Explorer", false, ImGuiWindowFlags_AlwaysAutoResize);
	if(igSliderFloat("Global brightness: ", &g_light_multiplier_default, 0.000001f, 15.0f, "%f", ImGuiSliderFlags_Logarithmic)){
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