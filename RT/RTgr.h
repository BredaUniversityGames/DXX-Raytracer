/*
 *
 * RT Extensions of the definitions for graphics lib.
 *
 */

#ifndef _RT_GR_H
#define _RT_GR_H

#include "Renderer.h"
#include "ApiTypes.h"
#include "Core/MiniMath.h"
#include "piggy.h"
#include "RTutil.h"
#include "laser.h"

#include "Core/Common.h"

#define MAX_TEXTURE_COUNT 2048

typedef struct
{
	bool explosionLights;
	float explosionBrightMod;
	float explosionRadiusMod;
	float explosionTypeBias;

	bool weaponFlareLights;
	float weaponBrightMod;
	float weaponRadiusMod;

	bool muzzleLights;
	float muzzleBrightMod;
	float muzzleRadiusMod;
} RT_DynamicLightInfo;


typedef struct RT_WeaponLightAdjusts
{
	const char* weapon_name;
	float brightMul;
	float radiusMul;
} RT_WeaponLightAdjusts;
#define RT_LIGHT_ADJUST_ARRAY_SIZE (SPREADFIRE_ID - CONCUSSION_ID + 1)

typedef struct RT_FreeCamInfo {
	int g_free_cam_obj;
	bool g_free_cam_enabled;
	bool g_free_cam_clipping_enabled;
	int g_old_cockpit;
} RT_FreeCamInfo;

typedef struct RT_GLTFNode RT_GLTFNode;

typedef struct CockpitSettings {
	RT_GLTFNode* cockpit_gltf;
	RT_ResourceHandle cockpit_hud_texture;
	RT_Vec3 front_cockpit_rotation;
	RT_Vec3 front_cockpit_offset;
	RT_Vec3 front_cockpit_scale;
	RT_Vec3 back_cockpit_rotation;
	RT_Vec3 back_cockpit_offset;
	RT_Vec3 back_cockpit_scale;
} CockpitSettings;
extern CockpitSettings g_rt_cockpit_settings;

extern bool g_rt_enable_debug_menu;
//I sure love globals
extern RT_DynamicLightInfo g_rt_dynamic_light_info;
extern RT_WeaponLightAdjusts rt_light_adjusts[];
extern RT_FreeCamInfo g_rt_free_cam_info;
RT_Camera g_cam;
RT_Camera g_free_cam;

extern uint64_t g_rt_frame_index;

// light culling variables
extern int light_culling_heuristic;
extern int max_rec_depth;
extern float max_distance;
extern float max_seg_distance;

//The imgui structs should be 8 bytes packed.
//Note: REMEMBER TO USE PUSH SAM >:( I messed up accidently.
#pragma pack(push, 8)
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
//For some reason the global include path doesn't work. Ah well then we use this!
#include "../../RT/Renderer/Backend/DX12/cimgui/cimgui.h"
#pragma pack(pop)

void RT_VertexFixToFloat_Fan(RT_TriangleBuffer *buf, int nv, g3s_point** pointlist, uint16_t texture_index, uint32_t triangle_color);

//Inits the glTF models that extend on the raytrace version.
void RT_InitglTFModels(void);
//Creates the polymodel on the index of polygonModelIndex.
void RT_InitBasePolyModel(const int polygonModelIndex, g3s_point* interp_point_list, void* model_ptr, vms_angvec* anim_angles, int first_texture);
RT_ResourceHandle RT_InitSubPolyModel(g3s_point* interp_point_list, void* model_ptr, vms_angvec* anim_angles, int first_texture);
void RT_InitPolyModelAndSubModels(int polymodel_index);

void RT_DrawPolyModel(const int meshnumber, const int objNum, ubyte object_type, const vms_vector* pos, const vms_matrix* orient);
void RT_DrawSubPolyModel(const RT_ResourceHandle submodel, const RT_Mat4* const submodel_transform, RT_RenderKey key);
void RT_DrawPolyModelTree(const int meshnumber, const int objNum, ubyte object_type, const vms_vector* pos, const vms_matrix* orient, vms_angvec* anim_angles);
void RT_DrawGLTF(const RT_GLTFNode* basenode, RT_Mat4 transform, RT_Mat4 prev_transform);

void RT_EnableFreeCam();
void RT_DisableFreeCam();

void RT_ResetLightEmission();

void RT_UpdateMaterialEdges(void);
void RT_UpdateMaterialIndices(void);

void RT_StartImGuiFrame(void);
void RT_EndImguiFrame(void);

#endif //_RT_GR_H