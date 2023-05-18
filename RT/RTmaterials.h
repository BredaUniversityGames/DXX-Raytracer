#pragma once

#include "ApiTypes.h"
#include "Renderer.h"

#pragma pack(push, 8)

RT_API RT_TextureFormat g_rt_material_texture_slot_formats[RT_MaterialTextureSlot_COUNT];
RT_API char *g_rt_texture_slot_names[RT_MaterialTextureSlot_COUNT];

typedef struct RT_MaterialPaths
{
    union
    {
        struct
        {
			char albedo_texture   [256];
			char normal_texture   [256];
			char metalness_texture[256];
			char roughness_texture[256];
			char emissive_texture [256];
        };
        char textures[RT_MaterialTextureSlot_COUNT][256];
    };
} RT_MaterialPaths;

RT_API void RT_InitAllBitmaps(void);
RT_API int RT_ReloadMaterials(void); // Returns how many textures were reloaded

RT_API RT_Material      g_rt_materials     [RT_MAX_TEXTURES];
RT_API RT_MaterialPaths g_rt_material_paths[RT_MAX_TEXTURES];

// NOTE(daniel): These are just for keeping track of what the default initialized values for these things are.
// The defaults may later change in code and that might want to be reflected in materials that had non-default
// values set, but not for all their properties.
RT_API RT_Material      g_rt_materials_default     [RT_MAX_TEXTURES];
RT_API RT_MaterialPaths g_rt_material_paths_default[RT_MAX_TEXTURES];

#pragma pack(pop)
