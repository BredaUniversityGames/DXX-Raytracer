#include "FileIO.h"

#include "Core/Arena.h"
#include "Core/String.h"

#include <stdio.h>

bool RT_ReadEntireFile(RT_Arena *arena, RT_String path, RT_String *result)
{
	const char *path_c = RT_CopyStringNullTerm(&g_thread_arena, path);

	FILE *f = fopen(path_c, "rb");

	if (!f)
	{
		// fprintf(stderr, "Failed to open file '%s'.\n", path_c);
		return false;
	}

	defer { fclose(f); };

	fseek(f, 0, SEEK_END);
	size_t file_size = ftell(f);
	fseek(f, 0, SEEK_SET);

	RT_String file;
	file.bytes = (char *)RT_ArenaAllocNoZero(arena, file_size + 1, 16);
	file.count = file_size;

	size_t bytes_read = fread(file.bytes, 1, file_size, f);

	if (bytes_read != file_size)
	{
		return false;
	}

	// Null terminate for good measure
	file.bytes[file.count] = 0;

	*result = file;

	return true;
}
