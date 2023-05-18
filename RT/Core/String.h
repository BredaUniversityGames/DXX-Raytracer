#pragma once

// ------------------------------------------------------------------

#include "ApiTypes.h"
#include "Arena.h"

// ------------------------------------------------------------------

#define RT_String_NPOS SIZE_MAX

#ifdef __cplusplus
#define RT_StringLiteral(s) RT_String { s, sizeof(s) - 1 }
#else
#define RT_StringLiteral(s) (RT_String) { s, sizeof(s) - 1 }
#endif

// NOTE(daniel): Very odd macro, specifically used for using the %.*s printf formatter
// to print counted strings.
#define RT_ExpandString(s) (int)(s).count, (s).bytes

static inline RT_String RT_StringFromCString(const char *c_string)
{
	size_t count = 0;
	for (const char *c = c_string; *c; c++)
	{
		count++;
	}

	RT_String result;
	result.bytes = (char *)c_string; // const begone
	result.count = count;
	return result;
}

RT_API void RT_CopyStringToBuffer(RT_String string, size_t buffer_size, char *buffer);
RT_API void RT_CopyStringToBufferNullTerm(RT_String string, size_t buffer_size, char *buffer);

static inline RT_String RT_CopyString(RT_Arena *arena, RT_String string)
{
	RT_String result;
	result.bytes = (char *)RT_ArenaCopyArray(arena, string.bytes, string.count);
	result.count = string.count;
	return result;
}

typedef struct RT_ParseFloatResult
{
	bool success;
	float value;
	size_t advance;
} RT_ParseFloatResult;

RT_API RT_ParseFloatResult RT_ParseFloat(RT_String string);

typedef struct RT_ParseIntResult
{
	bool success;
	int value;
	size_t advance;
} RT_ParseIntResult;

RT_API RT_ParseIntResult RT_ParseInt(RT_String string, int base);

static inline size_t RT_StringFindChar(RT_String string, char c)
{
	for (size_t i = 0; i < string.count; i++)
	{
		if (string.bytes[i] == c)
		{
			return i;
		}
	}
	return RT_String_NPOS;
}

static inline size_t RT_StringFindFirstNonWhitespace(RT_String string)
{
	for (size_t i = 0; i < string.count; i++)
	{
		if (string.bytes[i] != ' ' &&
			string.bytes[i] != '\t' &&
			string.bytes[i] != '\n' &&
			string.bytes[i] != '\r')
		{
			return i;
		}
	}
	return RT_String_NPOS;
}

static inline RT_String RT_StringAdvance(RT_String string, size_t advance)
{
	if (advance > string.count)
		advance = string.count;

	string.bytes += advance;
	string.count -= advance;
	return string;
}
