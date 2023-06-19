#pragma once

#include "ApiTypes.h" 

#pragma pack(push, 8)
typedef struct RT_Arena RT_Arena;
typedef struct RT_Material RT_Material;

typedef struct RT_GLTFModel
{
	RT_ResourceHandle handle;
} RT_GLTFModel;

typedef struct RT_MaterialOverride {
	uint16_t material_index;
	char* name;
} RT_MaterialOverride;

typedef struct RT_GLTFNode
{
	char *name;

	struct RT_GLTFNode *parent;
	struct RT_GLTFNode **children;
	size_t children_count;

	RT_GLTFModel *model;

	RT_Mat4 transform;
} RT_GLTFNode;

RT_API RT_GLTFNode *RT_LoadGLTF(RT_Arena* arena, const char* path, RT_MaterialOverride* material_override);
#pragma pack(pop)