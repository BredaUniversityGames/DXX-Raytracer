#pragma once

#include <stdlib.h>
#include <string.h>

#include "String.h"

// ------------------------------------------------------------------

RT_API void RT_CopyStringToBuffer(RT_String string, size_t buffer_size, char *buffer)
{
	size_t copy_size = string.count;

	if (copy_size > buffer_size)
		copy_size = buffer_size;

	memcpy(buffer, string.bytes, copy_size);
}

RT_API void RT_CopyStringToBufferNullTerm(RT_String string, size_t buffer_size, char *buffer)
{
	size_t copy_size = string.count;

	if (copy_size > buffer_size - 1)
		copy_size = buffer_size - 1;

	memcpy(buffer, string.bytes, copy_size);
	buffer[copy_size] = 0;
}

RT_API RT_ParseFloatResult RT_ParseFloat(RT_String string)
{
	RT_ParseFloatResult result = {0};

	char *end = string.bytes;
	float value = (float)strtod(string.bytes, &end);

	if (end != string.bytes)
	{
		result.success = true;
		result.value = value;
		result.advance = end - string.bytes;
	}

	return result;
}

RT_API RT_ParseIntResult RT_ParseInt(RT_String string, int base)
{
	RT_ParseIntResult result = {0};

	char *end = string.bytes;
	int value = (int)strtol(string.bytes, &end, base);

	if (end != string.bytes)
	{
		result.success = true;
		result.value = value;
		result.advance = end - string.bytes;
	}

	return result;
}
