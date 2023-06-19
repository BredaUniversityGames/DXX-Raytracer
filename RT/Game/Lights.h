
#ifndef _RT_LIGHTS_H
#define _RT_LIGHTS_H

#include "RTgr.h"
#include "Core/MiniMath.h"
#include "Renderer.h"

bool g_pending_light_update;
bool g_light_visual_debug;
int  g_active_lights;
// Light multiplier to tweak brightness of all the lights.
// Because this might change in runtime (end of level sequence), the ImGui windows should make use of the g_light_multiplier_default.
float g_light_multiplier;
// Default light multiplier (not changed during runtime)
float g_light_multiplier_default;

typedef struct RT_LightDefinition
{
	// TODO(daniel): These would ideally not be parameterized by the tmap index.
	// GameBitmap indices are the canonical indices for the _actual_ textures/bitmaps/materials.
	const char*  name;
	RT_LightKind kind;
	RT_Vec3      emission;
	float        radius;
	RT_Vec2		 size;
	float		 spot_angle;
	float		 spot_softness;
} RT_LightDefinition;

extern RT_LightDefinition g_light_definitions[];

// Returns g_light_definition index if light, otherwise -1.
int RT_IsLight(int tmap);
RT_Light RT_InitLight(RT_LightDefinition definition, RT_Vertex* vertices, RT_Vec3 normal);
void RT_ShowLightMenu();
void RT_VisualizeLight(RT_Light* light);

#endif