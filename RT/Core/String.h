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

// Just does what strncpy does but also null terminates the string if it doesn't fit in the destination buffer.
// BECAUSE THAT'S WHAT IT SHOULD DO. WHO WROTE STRNCPY. WHAT THE HECK.
static inline char *RT_SaneStrncpy(char *dst, const char *src, size_t count)
{
	if (count > 0)
	{
		size_t src_count = strlen(src);
		size_t cpy_count = RT_MIN(src_count, count - 1);

		if (cpy_count > 0)
		{
			memcpy(dst, src, cpy_count);
		}

		// always null terminate...
		dst[cpy_count] = '\0';
	}
	return dst;
}

static inline char *RT_FormatHumanReadableBytes(RT_Arena *arena, size_t bytes)
{
	size_t log = 0;
	for (size_t x = bytes / 1024; x > 0; x /= 1024) log += 1;

	char *string = NULL;
	if (log == 0)
	{
		string = RT_ArenaPrintF(arena, "%zuB", bytes);
	}
	else if (log == 1)
	{
		string = RT_ArenaPrintF(arena, "%zuKiB", bytes / 1024);
	}
	else if (log == 2)
	{
		string = RT_ArenaPrintF(arena, "%zuMiB", bytes / 1024 / 1024);
	}
	else if (log == 3)
	{
		string = RT_ArenaPrintF(arena, "%zuGiB", bytes / 1024 / 1024 / 1024);
	}
	else if (log >= 4)
	{
		string = RT_ArenaPrintF(arena, "%zuTiB", bytes / 1024 / 1024 / 1024 / 1024);
	}
	return string;
}

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

static inline const char *RT_CopyStringNullTerm(RT_Arena *arena, RT_String string)
{
	char *result = (char *)RT_ArenaCopyArray(arena, string.bytes, string.count + 1);
	result[string.count + 1] = 0;
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

static inline char RT_CharToLower(char c)
{
	return c >= 'A' && c <= 'Z' ? c + 'a' - 'A' : c;
}

static inline bool RT_StringsAreEqual(RT_String a, RT_String b)
{
	bool result = a.count == b.count;

	if (result)
	{
		for (size_t i = 0; i < a.count; i++)
		{
			if (a.bytes[i] != b.bytes[i])
			{
				result = false;
				break;
			}
		}
	}

	return result;
}

static inline bool RT_StringsAreEqualNoCase(RT_String a, RT_String b)
{
	bool result = a.count == b.count;

	if (result)
	{
		for (size_t i = 0; i < a.count; i++)
		{
			if (RT_CharToLower(a.bytes[i]) != RT_CharToLower(b.bytes[i]))
			{
				result = false;
				break;
			}
		}
	}

	return result;
}

static inline RT_String RT_Substring(RT_String string, size_t start, size_t count)
{
	start = RT_MIN(start, string.count);
	
	if (count > start - string.count)
	{
		count = start - string.count;
	}

	RT_String result;
	result.bytes = string.bytes + start;
	result.count = count;
	return result;
}

// Returns extension including dot
// For an extension like .tar.gz it will only return '.gz'.
static inline RT_String RT_StringFindExtension(RT_String string)
{
	size_t last_dot = RT_String_NPOS;

	for (size_t i = 0; i < string.count; i++)
	{
		if (string.bytes[i] == '.')
		{
			last_dot = i;
		}
	}

	return RT_Substring(string, last_dot, RT_String_NPOS);
}

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

static inline bool RT_IsWhitespace(char c)
{
	return (c == ' '  ||
			c == '\t' ||
			c == '\n' ||
			c == '\r');
}

static inline bool RT_IsNewline(char c)
{
	return (c == '\n' || c == '\r');
}

static inline size_t RT_StringFindFirstNonWhitespace(RT_String string)
{
	for (size_t i = 0; i < string.count; i++)
	{
		if (!RT_IsWhitespace(string.bytes[i]))
		{
			return i;
		}
	}
	return RT_String_NPOS;
}

static inline size_t RT_StringFindLastNonWhitespace(RT_String string)
{
	for (size_t j = 0; j < string.count; j++)
	{
		size_t i = string.count - j - 1; // unsigned reverse for loops :))

		if (!RT_IsWhitespace(string.bytes[i]))
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

static inline RT_String RT_StringSplitLine(RT_String string, RT_String *line)
{
	char *start = string.bytes;
	char *end   = string.bytes + string.count;
	char *at    = start;

	while (at < end && !RT_IsNewline(*at))
	{
		at++;
	}

	if (line)
	{
		line->bytes = start;
		line->count = at - start;
	}

	while (at < end && RT_IsNewline(*at))
	{
		at++;
	}

	RT_String result;
	result.bytes = at;
	result.count = end - at;
	return result;
}

// Returns string to the right of the char as a return value, returns left side through lhs parameter
static inline RT_String RT_StringSplitAroundChar(RT_String string, char c, RT_String *lhs)
{
	RT_String result = {0};

	size_t pos = RT_StringFindChar(string, c);

	if (pos != RT_String_NPOS)
	{
		if (lhs)
		{
			lhs->bytes = string.bytes;
			lhs->count = pos;
		}
		result = RT_StringAdvance(string, pos + 1);
	}
	else
	{
		if (lhs)
		{
			*lhs = string;
		}
	}

	return result;
}

// If the string is in the form "This is a string", it removes the quotes: This is a string
static inline RT_String RT_StringUnquoteString(RT_String string)
{
	if (string.count >= 2)
	{
		if (string.bytes[0] == '"' &&
			string.bytes[string.count - 1] == '"')
		{
			string.bytes += 1;
			string.count -= 2;
		}
	}
	return string;
}

static inline RT_String RT_StringTrimLeft(RT_String string)
{
	while (string.count > 0 && RT_IsWhitespace(string.bytes[0]))
	{
		string.bytes++;
		string.count--;
	}
	return string;
}

static inline RT_String RT_StringTrimRight(RT_String string)
{
	while (string.count > 0 && RT_IsWhitespace(string.bytes[string.count - 1]))
	{
		string.count--;
	}
	return string;
}

static inline RT_String RT_StringTrim(RT_String string)
{
	string = RT_StringTrimLeft(string);
	string = RT_StringTrimRight(string);
	return string;
}
