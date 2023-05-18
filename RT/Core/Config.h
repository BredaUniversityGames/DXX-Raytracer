#pragma once

#include "Common.h"

#pragma pack(push, 8)

#define RT_CONFIG_MAX_KEY_SIZE 256
#define RT_CONFIG_MAX_VALUE_SIZE 256

// memory cheap, computer fast, me dum dum
typedef struct RT_ConfigKeyValue
{
	struct RT_ConfigKeyValue *next;

	uint32_t hash;

	size_t key_count;
	char key[RT_CONFIG_MAX_KEY_SIZE];

	size_t value_count;
	char value[RT_CONFIG_MAX_VALUE_SIZE];
} RT_ConfigKeyValue;

typedef struct RT_Config
{
	RT_Arena *arena;

	size_t kv_count;
	RT_ConfigKeyValue *table[1024];

	RT_StringNode *first_error;
} RT_Config;

RT_API void RT_InitializeConfig(RT_Config *cfg, RT_Arena *arena);
RT_API bool RT_DeserializeConfigFromFile(RT_Config *cfg, const char *file_name);
RT_API void RT_DeserializeConfigFromString(RT_Config *cfg, RT_String string);
RT_API bool RT_ConfigReadString(RT_Config *cfg, RT_String key, RT_String *value);
RT_API bool RT_ConfigReadFloat(RT_Config *cfg, RT_String key, float *value);
RT_API bool RT_ConfigReadInt(RT_Config *cfg, RT_String key, int *value);
RT_API bool RT_ConfigReadVec3(RT_Config *cfg, RT_String key, RT_Vec3 *value);
RT_API void RT_ConfigWriteString(RT_Config *cfg, RT_String key, RT_String value);
RT_API void RT_ConfigWriteFloat(RT_Config *cfg, RT_String key, float value);
RT_API void RT_ConfigWriteInt(RT_Config *cfg, RT_String key, int value);
RT_API void RT_ConfigWriteVec3(RT_Config *cfg, RT_String key, RT_Vec3 value);
RT_API RT_String RT_SerializeConfigToString(RT_Arena *arena, RT_Config *cfg);
RT_API bool RT_SerializeConfigToFile(RT_Config *cfg, char *file_name);

// Prefer the above functions for easy interpretation of values
RT_API RT_ConfigKeyValue *RT_ConfigFindKeyValue(RT_Config *cfg, RT_String key);
RT_API RT_ConfigKeyValue *RT_ConfigFindOrCreateKeyValue(RT_Config *cfg, RT_String key);

#pragma pack(pop)
