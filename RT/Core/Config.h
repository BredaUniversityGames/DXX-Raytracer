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

	uint64_t last_modified_time;

	size_t kv_count;
	RT_ConfigKeyValue *table[1024];
	RT_ConfigKeyValue *first_free_key_value;

	RT_StringNode *first_error;
} RT_Config;

RT_API void RT_InitializeConfig(RT_Config *cfg, RT_Arena *arena);
RT_API bool RT_DeserializeConfigFromFile(RT_Config *cfg, const char *file_name);
RT_API void RT_DeserializeConfigFromString(RT_Config *cfg, RT_String string);
RT_API bool RT_ConfigReadString(RT_Config *cfg, RT_String key, RT_String *value);
RT_API bool RT_ConfigReadFloat(RT_Config *cfg, RT_String key, float *value);
RT_API bool RT_ConfigReadInt(RT_Config *cfg, RT_String key, int *value);
RT_API bool RT_ConfigReadVec2(RT_Config *cfg, RT_String key, RT_Vec2 *value);
RT_API bool RT_ConfigReadVec3(RT_Config *cfg, RT_String key, RT_Vec3 *value);
RT_API void RT_ConfigWriteString(RT_Config *cfg, RT_String key, RT_String value);
RT_API void RT_ConfigWriteFloat(RT_Config *cfg, RT_String key, float value);
RT_API void RT_ConfigWriteInt(RT_Config *cfg, RT_String key, int value);
RT_API void RT_ConfigWriteVec2(RT_Config *cfg, RT_String key, RT_Vec2 value);
RT_API void RT_ConfigWriteVec3(RT_Config *cfg, RT_String key, RT_Vec3 value);
RT_API bool RT_ConfigEraseKey(RT_Config *cfg, RT_String key); // Returns true if the key existed
RT_API RT_String RT_SerializeConfigToString(RT_Arena *arena, RT_Config *cfg);
RT_API bool RT_SerializeConfigToFile(RT_Config *cfg, char *file_name);

// Prefer the above functions for easy interpretation of values
RT_API RT_ConfigKeyValue *RT_ConfigFindKeyValue(RT_Config *cfg, RT_String key);
RT_API RT_ConfigKeyValue *RT_ConfigFindOrCreateKeyValue(RT_Config *cfg, RT_String key);

// ------------------------------------------------------------------
// Config Iterator

// Usage:
/*
	for (RT_ConfigIterator it = RT_IterateConfig(cfg);
		 RT_ConfigIterValid(&it);
		 RT_ConfigIterNext(&it))
	{
		size_t index = it.index;
		RT_ConfigKeyValue *kv = it.at;
		
		printf("%zuth key: %s\n", index, kv->key);
		printf("%zuth value: %s\n", index, kv->value);
	}
*/

typedef struct RT_ConfigIterator
{
	RT_Config *cfg;

	size_t index; // iteration index
	size_t slot;  // table slot currently pointed at
	RT_ConfigKeyValue *at; // key-value currently pointed at
} RT_ConfigIterator;

static inline RT_ConfigIterator RT_IterateConfig(RT_Config *cfg)
{
	RT_ConfigIterator it = {0};
	it.cfg = cfg;

	for (size_t i = 0; i < RT_ARRAY_COUNT(cfg->table); i++)
	{
		RT_ConfigKeyValue *kv = cfg->table[i];
		if (kv)
		{
			it.slot = i;
			it.at   = kv;
			break;
		}
	}

	return it;
}

static inline bool RT_ConfigIterValid(RT_ConfigIterator *it)
{
	return !!it->at;
}

static inline void RT_ConfigIterNext(RT_ConfigIterator *it)
{
	if (it->at)
	{
		it->at = it->at->next;
		if (!it->at)
		{
			it->slot += 1;
			for (size_t i = it->slot; i < RT_ARRAY_COUNT(it->cfg->table); i++)
			{
				if (it->cfg->table[i])
				{
					it->slot = i;
					it->at   = it->cfg->table[i];
					break;
				}
			}
		}
		it->index += 1;
	}
}

#pragma pack(pop)
