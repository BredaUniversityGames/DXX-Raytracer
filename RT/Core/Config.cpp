#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "Core/Config.h"
#include "Core/Hash.h"
#include "Core/Arena.h"
#include "Core/MemoryScope.hpp"
#include "Core/String.h"

static void ConfigError(RT_Config *cfg, char *error)
{
	RT_StringNode *error_node = RT_ArenaAllocStruct(cfg->arena, RT_StringNode);
	error_node->string.count = strlen(error); // booo...
	error_node->string.bytes = error; // I'm just going to require this to be memory with a suitable lifetime
	RT_SLL_PUSH(cfg->first_error, error_node);
}

typedef enum RT_ConfigTokenKind
{
	/* ascii ... */
	RT_ConfigToken_Identifier = 128,
	RT_ConfigToken_Number,
	RT_ConfigToken_String,
	RT_ConfigToken_Error,
	RT_ConfigToken_EOF,
} RT_ConfigTokenKind;

typedef struct RT_ConfigToken
{
	RT_ConfigTokenKind kind;
	RT_String string;
} RT_ConfigToken;

typedef struct RT_ConfigTokenizer
{
	char *start;
	char *end;
	char *at;

	RT_ConfigToken token;
	RT_Config *cfg;
} RT_ConfigTokenizer;

static void TokenizeError(RT_ConfigTokenizer *tok, const char *error)
{
	int line = 1;
	
	char *at = tok->start;
	char *end = tok->end;

	while (at < end)
	{
		if (*at++ == '\n')
		{
			line++;
		}
	}

	ConfigError(tok->cfg, RT_ArenaPrintF(tok->cfg->arena, "line %d: %s", line, error));
}

static void SkipWhitespace(RT_ConfigTokenizer *tok)
{
	char *at = tok->at;
	char *end = tok->end;

	while (at < end)
	{
		switch (*at)
		{
			case '#':
			{
				while (at < end && *at != '\n') at++;
			} break;

			default:
			{
				if (!isspace(*at))
					goto done;
				at++;
			} break;
		}
	}
done:

	tok->at = at; 
	tok->end = end;
}

static bool NextToken(RT_ConfigTokenizer *tok)
{
	SkipWhitespace(tok);

	char *at = tok->at;
	char *end = tok->end;

	char *token_start = tok->at;
	tok->token.kind = RT_ConfigToken_EOF;

	bool result = false;

	if (at < end)
	{
		result = true;

		tok->token.kind = RT_ConfigToken_Error;

		char c = *at++;
		switch (c)
		{
			case '=':
			{
				tok->token.kind = (RT_ConfigTokenKind)'=';
			} break;

			case '"':
			{
				tok->token.kind = RT_ConfigToken_String;
				while (at < end && *at != '"')
				{
					if (*at == '\n')
					{
						TokenizeError(tok, "Unterminated string (newline).");
						break;
					}
					at++;
				}

				if (at[-1] != '"')
					TokenizeError(tok, "Unterminated string (eof).");
			} break;

			case '0':
			case '1': case '2': case '3':
			case '4': case '5': case '6':
			case '7': case '8': case '9':
			{
				tok->token.kind = RT_ConfigToken_Number;

				// NOTE: fairly unrobust float parsing
				while (at < end && isalnum(*at)) at++;
				if (*at == '.') at++;
				while (at < end && isalnum(*at)) at++;
				if (*at == 'f') at++;
			} break;

			default:
			{
				if (isalpha(c) || c == '_')
				{
					tok->token.kind = RT_ConfigToken_Identifier;
					while (at < end && isalnum(*at) || *at == '_') at++;
				}
			} break;
		}
	}

	char *token_end = at;

	tok->token.string.bytes = token_start;
	tok->token.string.count = token_end - token_start;

	tok->at = at;
	tok->end = end;

	return result;
}

static bool ExpectKey(RT_ConfigTokenizer *tok, RT_ConfigToken *token)
{
	if (tok->token.kind != RT_ConfigToken_Identifier)
	{
		TokenizeError(tok, "Expected key.");
		return false;
	}

	*token = tok->token;
	NextToken(tok);

	return true;
}

static bool ExpectOperator(RT_ConfigTokenizer *tok, char op, RT_ConfigToken *token)
{
	if (tok->token.kind != op)
	{
		TokenizeError(tok, RT_ArenaPrintF(tok->cfg->arena, "Expected character '%c' after key.", op));
		return false;
	}

	*token = tok->token;
	NextToken(tok);

	return true;
}

static bool ExpectValue(RT_ConfigTokenizer *tok, RT_ConfigToken *token)
{
	if (tok->token.kind != RT_ConfigToken_Identifier &&
		tok->token.kind != RT_ConfigToken_String &&
		tok->token.kind != RT_ConfigToken_Number)
	{
		TokenizeError(tok, "Expected valid value.");
		return false;
	}

	*token = tok->token;
	NextToken(tok);

	return true;
}

static void InitTokenizer(RT_ConfigTokenizer *tok, RT_Config *cfg, RT_String string)
{
	tok->start = string.bytes;
	tok->at = tok->start;
	tok->end = tok->start + string.count;
	tok->cfg = cfg;

	NextToken(tok);
}

void RT_InitializeConfig(RT_Config *cfg, RT_Arena *arena)
{
	memset(cfg, 0, sizeof(*cfg));
	cfg->arena = arena;
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
	RT_ConfigTokenizer tok = {0};
	InitTokenizer(&tok, cfg, string);

	while (tok.token.kind != RT_ConfigToken_EOF)
	{
		RT_ConfigToken token;

		if (!ExpectKey(&tok, &token))
			break;

		RT_String key = token.string;

		if (!ExpectOperator(&tok, '=', &token))
			break;

		if (!ExpectValue(&tok, &token))
			break;

		RT_String value = token.string;

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
		kv = RT_ArenaAllocStruct(cfg->arena, RT_ConfigKeyValue);
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

bool RT_ConfigReadVec3(RT_Config *cfg, RT_String key, RT_Vec3 *value)
{
	bool result = false;

	RT_String string;
	if (RT_ConfigReadString(cfg, key, &string))
	{
		RT_ParseFloatResult parse_x = RT_ParseFloat(string);
		string = RT_StringAdvance(string, parse_x.advance);
		string = RT_StringAdvance(string, RT_StringFindChar(string, ','));
		string = RT_StringAdvance(string, RT_StringFindFirstNonWhitespace(string));
		RT_ParseFloatResult parse_y = RT_ParseFloat(string);
		string = RT_StringAdvance(string, parse_y.advance);
		string = RT_StringAdvance(string, RT_StringFindChar(string, ','));
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
}

void RT_ConfigWriteFloat(RT_Config *cfg, RT_String key, float value)
{
	RT_ConfigKeyValue *kv = RT_ConfigFindOrCreateKeyValue(cfg, key);
	kv->value_count = snprintf(kv->value, sizeof(kv->value), "%f", value);
}

void RT_ConfigWriteInt(RT_Config *cfg, RT_String key, int value)
{
	RT_ConfigKeyValue *kv = RT_ConfigFindOrCreateKeyValue(cfg, key);
	kv->value_count = snprintf(kv->value, sizeof(kv->value), "%d", value);
}

void RT_ConfigWriteVec3(RT_Config *cfg, RT_String key, RT_Vec3 value)
{
	RT_ConfigKeyValue *kv = RT_ConfigFindOrCreateKeyValue(cfg, key);
	kv->value_count = snprintf(kv->value, sizeof(kv->value), "%f, %f, %f", value.x, value.y, value.z);
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
