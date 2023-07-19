#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "Core/Config.h"
#include "Core/Hash.h"
#include "Core/Arena.h"
#include "Core/MemoryScope.hpp"
#include "Core/String.h"

static inline void ConfigError(RT_Config *cfg, char *error)
{
	RT_StringNode *error_node = RT_ArenaAllocStruct(cfg->arena, RT_StringNode);
	error_node->string.count = strlen(error); // booo...
	error_node->string.bytes = error; // I'm just going to require this to be memory with a suitable lifetime
	RT_SLL_PUSH(cfg->first_error, error_node);
}

void RT_InitializeConfig(RT_Config *cfg, RT_Arena *arena)
{
	memset(cfg, 0, sizeof(*cfg));
	cfg->arena = arena;
	cfg->last_modified_time = RT_GetHighResTime().value;
}

bool RT_DeserializeConfigFromFile(RT_Config *cfg, const char *file_name)
{
	FILE *f = fopen(file_name, "rb");

	if (!f)
	{
		ConfigError(cfg, RT_ArenaPrintF(cfg->arena, "Failed to open file '%s'.", file_name));
		return false;
	}

	defer { fclose(f); };

	fseek(f, 0, SEEK_END);
	size_t file_size = ftell(f);
	fseek(f, 0, SEEK_SET);

	RT_String file;
	file.bytes = (char *)RT_ArenaAllocNoZero(cfg->arena, file_size + 1, 16); // NOTE(daniel): This could just use the thread arena but there's nuances here if the arena passed in is the thread arena...
	file.count = file_size;

	// Null terminate for good measure
	file.bytes[file.count] = 0;

	size_t bytes_read = fread(file.bytes, 1, file_size, f);
	if (bytes_read != file_size)
	{
		ConfigError(cfg, RT_ArenaPrintF(cfg->arena, "Read a weird amount of bytes from file: Expected %zu, got %zu.", file_size, bytes_read));
		return false;
	}

	RT_DeserializeConfigFromString(cfg, file);
	return true;
}

void RT_DeserializeConfigFromString(RT_Config *cfg, RT_String string)
{
	while (string.count > 0)
	{
		RT_String line;
		string = RT_StringSplitLine(string, &line);

		line = RT_StringTrim(line);

		if (line.count > 0 && line.bytes[0] == '#')
			continue;

		RT_String key;
		line = RT_StringSplitAroundChar(line, '=', &key);

		key = RT_StringTrim(key);

		RT_String value;
		line = RT_StringSplitAroundChar(line, '#', &value);

		value = RT_StringTrim(value);
		value = RT_StringUnquoteString(value);

		RT_ConfigWriteString(cfg, key, value);
	}
}

RT_ConfigKeyValue *RT_ConfigFindKeyValue(RT_Config *cfg, RT_String key)
{
	uint32_t hash = RT_Murmur3(key.bytes, (uint32_t)key.count, 0xBEEFD00D);
	uint32_t slot = hash % RT_ARRAY_COUNT(cfg->table);

	RT_ConfigKeyValue *kv;
	for (kv = cfg->table[slot]; kv; kv = kv->next)
	{
		if (kv->hash == hash && 
			strncmp(key.bytes, kv->key, RT_MIN(key.count, kv->key_count)) == 0)
		{
			break;
		}
	}
	return kv;
}

RT_ConfigKeyValue *RT_ConfigFindOrCreateKeyValue(RT_Config *cfg, RT_String key)
{
	uint32_t hash = RT_Murmur3(key.bytes, (uint32_t)key.count, 0xBEEFD00D);
	uint32_t slot = hash % RT_ARRAY_COUNT(cfg->table);

	RT_ConfigKeyValue *kv;
	for (kv = cfg->table[slot]; kv; kv = kv->next)
	{
		if (kv->hash == hash && 
			strncmp(key.bytes, kv->key, RT_MIN(key.count, kv->key_count)) == 0)
		{
			break;
		}
	}

	if (!kv)
	{
		if (!cfg->first_free_key_value)
		{
			cfg->first_free_key_value = RT_ArenaAllocStructNoZero(cfg->arena, RT_ConfigKeyValue);
			cfg->first_free_key_value->next = NULL;
		}

		kv = RT_SLL_POP(cfg->first_free_key_value);
		memset(kv, 0, sizeof(*kv));

		kv->hash = hash;
		kv->key_count = key.count;
		if (kv->key_count > sizeof(kv->key) - 1)
		{
			kv->key_count = sizeof(kv->key) - 1;
			ConfigError(cfg, RT_ArenaPrintF(cfg->arena, "Truncated key '%.*s', it was longer than 255 characters (actual key stored: '%.*s')",
											(int)key.count, key.bytes, 255, key.bytes));
		}
		RT_CopyStringToBufferNullTerm(key, sizeof(kv->key), kv->key);

		kv->next = cfg->table[slot];
		cfg->table[slot] = kv;
		cfg->kv_count++;
	}

	return kv;
}

bool RT_ConfigReadString(RT_Config *cfg, RT_String key, RT_String *value)
{
	bool result = false;

	RT_ConfigKeyValue *kv = RT_ConfigFindKeyValue(cfg, key);

	if (kv)
	{
		result = true;

		if (value)
		{
			value->bytes = kv->value;
			value->count = kv->value_count;
		}
	}

	return result;
}

bool RT_ConfigReadFloat(RT_Config *cfg, RT_String key, float *value)
{
	bool result = false;

	RT_String string;
	if (RT_ConfigReadString(cfg, key, &string))
	{
		RT_ParseFloatResult parse = RT_ParseFloat(string);
		if (parse.success)
		{
			result = true;
			*value = parse.value;
		}
	}

	return result;
}

bool RT_ConfigReadInt(RT_Config *cfg, RT_String key, int *value)
{
	bool result = false;

	RT_String string;
	if (RT_ConfigReadString(cfg, key, &string))
	{
		RT_ParseIntResult parse = RT_ParseInt(string, 10);
		if (parse.success)
		{
			result = true;
			*value = parse.value;
		}
	}

	return result;
}

bool RT_ConfigReadVec2(RT_Config *cfg, RT_String key, RT_Vec2 *value)
{
	bool result = false;

	RT_String string;
	if (RT_ConfigReadString(cfg, key, &string))
	{
		RT_ParseFloatResult parse_x = RT_ParseFloat(string);
		string = RT_StringAdvance(string, parse_x.advance);
		string = RT_StringAdvance(string, RT_StringFindChar(string, ',') + 1);
		string = RT_StringAdvance(string, RT_StringFindFirstNonWhitespace(string));
		RT_ParseFloatResult parse_y = RT_ParseFloat(string);

		result = (parse_x.success &&
				  parse_y.success);

		if (result)
		{
			value->x = parse_x.value;
			value->y = parse_y.value;
		}
	}

	return result;
}

bool RT_ConfigReadVec3(RT_Config *cfg, RT_String key, RT_Vec3 *value)
{
	bool result = false;

	RT_String string;
	if (RT_ConfigReadString(cfg, key, &string))
	{
		RT_ParseFloatResult parse_x = RT_ParseFloat(string);
		string = RT_StringAdvance(string, parse_x.advance);
		string = RT_StringAdvance(string, RT_StringFindChar(string, ',') + 1);
		string = RT_StringAdvance(string, RT_StringFindFirstNonWhitespace(string));
		RT_ParseFloatResult parse_y = RT_ParseFloat(string);
		string = RT_StringAdvance(string, parse_y.advance);
		string = RT_StringAdvance(string, RT_StringFindChar(string, ',') + 1);
		string = RT_StringAdvance(string, RT_StringFindFirstNonWhitespace(string));
		RT_ParseFloatResult parse_z = RT_ParseFloat(string);

		result = (parse_x.success &&
				  parse_y.success &&
				  parse_z.success);

		if (result)
		{
			value->x = parse_x.value;
			value->y = parse_y.value;
			value->z = parse_z.value;
		}
	}

	return result;
}

void RT_ConfigWriteString(RT_Config *cfg, RT_String key, RT_String value)
{
	RT_ConfigKeyValue *kv = RT_ConfigFindOrCreateKeyValue(cfg, key);

	kv->value_count = value.count;
	if (kv->value_count > sizeof(kv->value) - 1)
	{
		kv->value_count = sizeof(kv->value) - 1;
		ConfigError(cfg, RT_ArenaPrintF(cfg->arena, "Truncated value '%.*s', it was longer than 255 characters (actual value stored: '%.*s')",
										(int)value.count, value.bytes, 255, value.bytes));
	}
	RT_CopyStringToBufferNullTerm(value, sizeof(kv->value), kv->value);

	cfg->last_modified_time = RT_GetHighResTime().value;
}

void RT_ConfigWriteFloat(RT_Config *cfg, RT_String key, float value)
{
	RT_ConfigKeyValue *kv = RT_ConfigFindOrCreateKeyValue(cfg, key);
	kv->value_count = snprintf(kv->value, sizeof(kv->value), "%f", value);
	cfg->last_modified_time = RT_GetHighResTime().value;
}

void RT_ConfigWriteInt(RT_Config *cfg, RT_String key, int value)
{
	RT_ConfigKeyValue *kv = RT_ConfigFindOrCreateKeyValue(cfg, key);
	kv->value_count = snprintf(kv->value, sizeof(kv->value), "%d", value);
	cfg->last_modified_time = RT_GetHighResTime().value;
}

void RT_ConfigWriteVec2(RT_Config *cfg, RT_String key, RT_Vec2 value)
{
	RT_ConfigKeyValue *kv = RT_ConfigFindOrCreateKeyValue(cfg, key);
	kv->value_count = snprintf(kv->value, sizeof(kv->value), "%f, %f", value.x, value.y);
	cfg->last_modified_time = RT_GetHighResTime().value;
}

void RT_ConfigWriteVec3(RT_Config *cfg, RT_String key, RT_Vec3 value)
{
	RT_ConfigKeyValue *kv = RT_ConfigFindOrCreateKeyValue(cfg, key);
	kv->value_count = snprintf(kv->value, sizeof(kv->value), "%f, %f, %f", value.x, value.y, value.z);
	cfg->last_modified_time = RT_GetHighResTime().value;
}

bool RT_ConfigEraseKey(RT_Config *cfg, RT_String key)
{
	bool result = false;

	uint32_t hash = RT_Murmur3(key.bytes, (uint32_t)key.count, 0xBEEFD00D);
	uint32_t slot = hash % RT_ARRAY_COUNT(cfg->table);

	for (RT_ConfigKeyValue **kv_at = &cfg->table[slot]; *kv_at; kv_at = &(*kv_at)->next)
	{
		RT_ConfigKeyValue *kv = *kv_at;

		if (kv->hash == hash && 
			strncmp(key.bytes, kv->key, RT_MIN(key.count, kv->key_count)) == 0)
		{
			*kv_at = (*kv_at)->next;

			// Add kv to freelist
			kv->next = cfg->first_free_key_value;
			cfg->first_free_key_value = kv;

			if (ALWAYS(cfg->kv_count > 0))
				cfg->kv_count -= 1;

			result = true;
			cfg->last_modified_time = RT_GetHighResTime().value;

			break;
		}
	}

	return result;
}

RT_String RT_SerializeConfigToString(RT_Arena *arena, RT_Config *cfg)
{
	// NOTE(daniel): Because I don't have a flexible string builder of some kind, I am precariously
	// pre-calculating the upper bound of storage required for the string.

	size_t per_kv_storage = RT_CONFIG_MAX_KEY_SIZE + RT_CONFIG_MAX_VALUE_SIZE + 4; // + 4 for ' = ' and newline
	size_t storage_upper_bound = cfg->kv_count*per_kv_storage;

	if (cfg->first_error)
	{
		// IMPORTANT: Must match PrintToBuffer call down below.
		storage_upper_bound += sizeof("# This config had logged errors:\n");
		for (RT_StringNode *error = cfg->first_error; error; error = error->next)
		{
			storage_upper_bound += error->string.count + 3; // # [error]\n	
		}
	}

	char *buffer = RT_ArenaAllocArrayNoZero(arena, storage_upper_bound, char);

	char  *buffer_at   = buffer;
	size_t buffer_left = storage_upper_bound;

	auto PrintToBuffer = [&buffer_at, &buffer_left](const char *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);

		size_t print_count = vsnprintf(buffer_at, buffer_left, fmt, args);

		if (NEVER(print_count > buffer_left))
			print_count = buffer_left;

		buffer_at   += print_count;
		buffer_left -= print_count;

		va_end(args);
	};

	if (cfg->first_error)
	{
		PrintToBuffer("# This config had logged errors:\n");

		for (RT_StringNode *error = cfg->first_error; error; error = error->next)
		{
			PrintToBuffer("# %.*s\n", RT_ExpandString(error->string));
		}
	}

	for (size_t i = 0; i < RT_ARRAY_COUNT(cfg->table); i++)
	{
		for (RT_ConfigKeyValue *kv = cfg->table[i]; kv; kv = kv->next)
		{
			PrintToBuffer("%s = %s\n", kv->key, kv->value);
		}
	}

	size_t total_print_count = buffer_at - buffer;

	RT_String result;
	result.bytes = buffer;
	result.count = total_print_count;
	return result;
}

bool RT_SerializeConfigToFile(RT_Config *cfg, char *file_name)
{
	FILE *f = fopen(file_name, "wb");
	if (!f)
	{
		ConfigError(cfg, RT_ArenaPrintF(cfg->arena, "Failed to open file '%s' for writing.", file_name));
		return false;
	}
	defer { fclose(f); };

	bool result = false;

	RT::MemoryScope temp;

	RT_String serialized = RT_SerializeConfigToString(temp, cfg);
	if (serialized.count > 0)
	{
		size_t bytes_written = fwrite(serialized.bytes, 1, serialized.count, f);
		if (bytes_written == serialized.count)
		{
			result = true;
		}
		else
		{
			ConfigError(cfg, RT_ArenaPrintF(cfg->arena, "Wrote a weird amount of bytes: Tried to write %zu, but wrote %zu.", serialized.count, bytes_written));
		}
	}

	return result;
}
