#pragma once

// The descent code likes to mess with padding, so public headers should use pragma pack to
// restore expected packing!
#pragma pack(push, 8)

#include "ApiTypes.h"
#include "Core/MiniMath.h"

// ------------------------------------------------------------------

#define RT_MAX_TEXTURES  (2010)
#define RT_MAX_MATERIALS (2010)
#define RT_MAX_OBJ_BITMAPS (210) // mirrored from the game to avoid dependency on game headers 
#define RT_MAX_SEGMENTS (9000) // mirrored from the game to avoid dependency on game headers
#define RT_SIDES_PER_SEGMENT (6)
#define RT_TRIANGLES_PER_SIDE (2)
#define RT_MAX_MATERIAL_EDGES (RT_MAX_SEGMENTS*RT_SIDES_PER_SEGMENT)
#define RT_MAX_TRIANGLES (RT_MAX_SEGMENTS*RT_SIDES_PER_SEGMENT*RT_TRIANGLES_PER_SIDE)
#define RT_MAX_LIGHTS (100)

// OR this in for triangle->material_edge_index for poly objects and have them
// skip the material edges array...
#define RT_TRIANGLE_HOLDS_MATERIAL_EDGE (1 << 15)
#define RT_TRIANGLE_MATERIAL_INSTANCE_OVERRIDE (0xFFFF)

// Some built in materials to use
#define RT_MATERIAL_FLAT_WHITE       (RT_MAX_MATERIALS - 1)
#define RT_MATERIAL_EMISSIVE_WHITE   (RT_MAX_MATERIALS - 2)
#define RT_MATERIAL_ENDLEVEL_TERRAIN (RT_MAX_MATERIALS - 3)

typedef struct RT_Arena RT_Arena;

typedef struct RT_MaterialEdge
{
	uint16_t mat1; // equivalent to tmap_num1
	uint16_t mat2; // equivalent to tmap_num2
} RT_MaterialEdge;

typedef struct RT_Camera
{
	RT_Vec3 position;
	RT_Vec3 up;
	RT_Vec3 forward;
	RT_Vec3 right;
	float vfov; // degrees
	float near_plane;
	float far_plane;
} RT_Camera;

typedef struct RT_RendererInitParams
{
	RT_Arena* arena;
	void* window_handle;
} RT_RendererInitParams;

enum RT_RenderMeshFlags
{
	RT_RenderMeshFlags_ReverseCulling = (1 << 0)
};

typedef struct RT_RenderMeshParams
{
	RT_ResourceHandle mesh_handle;
	const RT_Mat4* transform;
	const RT_Mat4* prev_transform;
	uint32_t color;
	uint16_t material_override;
	uint32_t flags;
} RT_RenderMeshParams;

// Volatile: Must match common.hlsl
typedef enum RT_DebugRenderMode
{
	RT_DebugRenderMode_None,
	RT_DebugRenderMode_Normals,
	RT_DebugRenderMode_Depth,
	RT_DebugRenderMode_Albedo,
	RT_DebugRenderMode_Emissive,
	RT_DebugRenderMode_Diffuse,
	RT_DebugRenderMode_Specular,
	RT_DebugRenderMode_Motion,
	RT_DebugRenderMode_MetallicRoughness,
	RT_DebugRenderMode_HistoryLength,
	RT_DebugRenderMode_Materials,
	RT_DebugRenderMode_FirstMoment,
	RT_DebugRenderMode_SecondMoment,
	RT_DebugRenderMode_Variance,
	RT_DebugRenderMode_Bloom0,
	RT_DebugRenderMode_Bloom1,
	RT_DebugRenderMode_Bloom2,
	RT_DebugRenderMode_Bloom3,
	RT_DebugRenderMode_Bloom4,
	RT_DebugRenderMode_Bloom5,
	RT_DebugRenderMode_Bloom6,
	RT_DebugRenderMode_Bloom7,
	RT_DebugRenderMode_COUNT,
} RT_DebugRenderMode;

typedef struct RT_RendererIO
{
	// in:
	bool scene_transition; // set to true on a frame where there is a jumpcut or scene transition to avoid ghosting
	bool debug_line_depth_enabled;
	RT_Vec4 screen_overlay_color;

	// in/out:
	int debug_render_mode;

	// out:
	bool frame_frozen;
} RT_RendererIO;

typedef enum RT_TextureFormat
{
	RT_TextureFormat_RGBA8,
	RT_TextureFormat_SRGBA8,
	RT_TextureFormat_R8,
} RT_TextureFormat;

static char g_rt_texture_format_bpp[] =
{
	4,
	4,
	1,
};

typedef struct RT_UploadTextureParams
{
	RT_TextureFormat format;

	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t* pixels;

	const char *name; // Optional, used for enhanced graphics debugging
} RT_UploadTextureParams;

typedef struct RT_Triangle
{
	// NOTE(daniel): I go through the effort of making these unions mostly to indicate
	// that there is code that depends on these members being packed together in order.
	union
	{
		struct
		{
			RT_Vec3 pos0;
			RT_Vec3 pos1;
			RT_Vec3 pos2;
		};
		RT_Vec3 positions[3];
	};

	union
	{
		struct
		{
			RT_Vec3 normal0;
			RT_Vec3 normal1;
			RT_Vec3 normal2;
		};
		RT_Vec3 normals[3];
	};

	union
	{
		struct
		{
			RT_Vec4 tangent0;
			RT_Vec4 tangent1;
			RT_Vec4 tangent2;
		};
		RT_Vec4 tangents[3];
	};

	union
	{
		struct
		{
			RT_Vec2 uv0;
			RT_Vec2 uv1;
			RT_Vec2 uv2;
		};
		RT_Vec2 uvs[3];
	};

	uint32_t color;
	uint32_t material_edge_index;
} RT_Triangle;

typedef struct RT_UploadMeshParams
{
	size_t triangle_count;
	RT_Triangle* triangles;

	const char* name; // Optional, used for enhanced graphics debugging
} RT_UploadMeshParams;

typedef struct RT_DoRendererDebugMenuParams
{
	bool ui_has_cursor_focus;
} RT_DoRendererDebugMenuParams;

typedef enum RT_MaterialFlags
{
	RT_MaterialFlag_BlackbodyRadiator = 0x1, // things like lava, basically just treats the albedo as an emissive map and skips all shading
} RT_MaterialFlags;

typedef enum RT_MaterialTextureSlot
{
    RT_MaterialTextureSlot_Albedo,
    RT_MaterialTextureSlot_Normal,
    RT_MaterialTextureSlot_Metalness,
    RT_MaterialTextureSlot_Roughness,
    RT_MaterialTextureSlot_Emissive,
    RT_MaterialTextureSlot_COUNT
} RT_MaterialTextureSlot;

typedef struct RT_Material
{
	union
	{
		struct
		{
			RT_ResourceHandle albedo_texture;
			RT_ResourceHandle normal_texture;
			RT_ResourceHandle metalness_texture;
			RT_ResourceHandle roughness_texture;
			RT_ResourceHandle emissive_texture;
		};
		RT_ResourceHandle textures[RT_MaterialTextureSlot_COUNT];
	};
	float metalness;
	float roughness;
	RT_Vec3 emissive_color;
	float emissive_strength;
	uint32_t flags;
} RT_Material;

typedef struct RT_SceneSettings
{
	RT_Camera* camera;
	uint32_t render_width_override;
	uint32_t render_height_override;
} RT_SceneSettings;

typedef struct RT_RasterTrianglesParams
{
	RT_ResourceHandle texture_handle;
	RT_RasterTriVertex* vertices;
	uint32_t num_vertices;
} RT_RasterTrianglesParams;

typedef struct RT_RasterLinesParams
{
	RT_RasterLineVertex* vertices;
	uint32_t num_vertices;
} RT_RasterLinesParams;

// ------------------------------------------------------------------

RT_API void RT_RendererInit(const RT_RendererInitParams* renderInitParams);
RT_API RT_RendererIO *RT_GetRendererIO(void);
RT_API void RT_RendererExit(void);

RT_API void RT_BeginFrame(void);
RT_API void RT_BeginScene(const RT_SceneSettings* scene_settings);
RT_API void RT_EndScene(void);
RT_API void RT_EndFrame(void);
RT_API void RT_SwapBuffers(void);

RT_API RT_MaterialEdge *RT_GetMaterialEdgesArray(void);
RT_API uint16_t        *RT_GetMaterialIndicesArray(void);

// Needs to be called in a place where Dear ImGui can be used.
RT_API void RT_DoRendererDebugMenus(const RT_DoRendererDebugMenuParams *params);

RT_API RT_ResourceHandle RT_UploadTexture(const RT_UploadTextureParams* params);
// Updates the material on the GPU at the given index, so long as it is less than RT_MAX_MATERIALS.
// Returns the material_index you passed in, or UINT16_MAX if it was out of bounds.
RT_API uint16_t RT_UpdateMaterial(uint16_t material_index, const RT_Material *material);
RT_API RT_ResourceHandle RT_UploadMesh(const RT_UploadMeshParams* params);
RT_API void RT_ReleaseMesh(const RT_ResourceHandle mesh_handle);
RT_API bool RT_GenerateTangents(RT_Triangle *triangles, size_t triangle_count); // Will modify triangles in-place to add tangent vectors

RT_API RT_ResourceHandle RT_GetDefaultWhiteTexture(void);
RT_API RT_ResourceHandle RT_GetDefaultBlackTexture(void);

RT_API int RT_CheckWindowMinimized(void);

// -------------------------------------------------------------------------------
// Raytracing functions

RT_API uint32_t RT_RaytraceSetRenderFlagsOverride(uint32_t flags);
RT_API void RT_RaytraceMeshEx(RT_RenderMeshParams* render_mesh_params);
RT_API void RT_RaytraceMeshColor(RT_ResourceHandle mesh, RT_Vec4 color, const RT_Mat4* transform, const RT_Mat4* prev_transform);
RT_API void RT_RaytraceMesh(RT_ResourceHandle mesh, const RT_Mat4* transform, const RT_Mat4* prev_transform);
RT_API void RT_RaytraceMeshOverrideMaterial(RT_ResourceHandle mesh, uint16_t material_override, const RT_Mat4* transform, const RT_Mat4* prev_transform);
// NOTE(daniel): I'd prefer it if these functions were not part of the "core" renderer, but instead the core call was just
// RT_RenderQuad or something and then that can just be used to draw a billboard. But drawing a billboard requires knowledge
// of the camera, and right now the camera isn't that well organized, so meh. Imma figure out the billboards in RenderBackend.cpp
// first.
RT_API void RT_RaytraceBillboard(uint16_t material_index, RT_Vec2 dim, RT_Vec3 pos, RT_Vec3 prev_pos);
RT_API void RT_RaytraceBillboardColored(uint16_t material_index, RT_Vec3 color, RT_Vec2 dim, RT_Vec3 pos, RT_Vec3 prev_pos);
RT_API void RT_RaytraceRod(uint16_t material_index, RT_Vec3 bot_p, RT_Vec3 top_p, float width);
RT_API void RT_RaytraceRender();
RT_API void RT_RaytraceSubmitLights(size_t light_count, const RT_Light *lights); 
static inline void RT_RaytraceSubmitLight(RT_Light light)
{
	RT_RaytraceSubmitLights(1, &light);
}
RT_API void RT_RaytraceSetVerticalOffset(float new_offset);
RT_API float RT_RaytraceGetVerticalOffset();
RT_API void RT_RaytraceSetSkyColors(RT_Vec3 sky_top, RT_Vec3 sky_bottom);

// -------------------------------------------------------------------------------
// Rasterizer functions

// This sets the rasterizers viewport (UI)
RT_API void RT_RasterSetViewport(float x, float y, float width, float height);
// Used to rasterize UI triangles
RT_API void RT_RasterTriangles(RT_RasterTrianglesParams* params, uint32_t num_params);
// Used to rasterize UI lines
RT_API void RT_RasterLines(RT_RasterLineVertex* vertices, uint32_t num_vertices);
// Used to rasterize debug lines in the actual 3D world
RT_API void RT_DrawLineWorld(RT_Vec3 a, RT_Vec3 b, RT_Vec4 color);
RT_API void RT_RasterLinesWorld(RT_RasterLineVertex* vertices, uint32_t num_vertices);
RT_API void RT_RasterRender();

// -------------------------------------------------------------------------------
// Dear ImGui functions

RT_API void RT_RenderImGuiTexture(RT_ResourceHandle texture_handle, float width, float height);
RT_API void RT_RenderImGui();

// -------------------------------------------------------------------------------
// Utility functions

RT_API void RT_QueueScreenshot(const char *file_name);

// Don't forget to pop.
#pragma pack(pop)