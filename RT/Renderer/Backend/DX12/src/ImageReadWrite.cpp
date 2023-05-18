#include "ImageReadWrite.h"
#include "Core/Arena.h"

#define STBI_ASSERT(x) RT_ASSERT(x)
#define STBI_MALLOC(size) RT_ArenaAllocNoZero(&g_thread_arena, size, 16)
#define STBI_REALLOC_SIZED(pointer, old_size, new_size) RT_ArenaResize(&g_thread_arena, pointer, old_size, new_size, 16)
#define STBI_FREE(pointer) (void)(0) /* no-op */

#define STB_IMAGE_IMPLEMENTATION

#pragma warning(push, 0)
#include "../STB/stb_image.h"
#pragma pop

#define STBIW_ASSERT(x) RT_ASSERT(x)
#define STBIW_MALLOC(size) RT_ArenaAllocNoZero(&g_thread_arena, size, 16)
#define STBIW_REALLOC_SIZED(pointer, old_size, new_size) RT_ArenaResize(&g_thread_arena, pointer, old_size, new_size, 16)
#define STBIW_FREE(pointer) (void)(0) /* no-op */

#define STB_IMAGE_WRITE_IMPLEMENTATION

#pragma warning(push, 0)
#include "../STB/stb_image_write.h"
#pragma pop

unsigned char *RT_LoadImageFromDisk(RT_Arena *arena, const char *path, int *w, int *h, int *channel_count, int required_channel_count)
{
	RT_ArenaMarker marker = RT_ArenaGetMarker(&g_thread_arena);

	unsigned char *result = stbi_load(path, w, h, channel_count, required_channel_count);

	if (arena != &g_thread_arena)
	{
		// stbi_load will always use the thread arena, but if we wanted the result to be in a different arena
		// we'll just copy it. This means that the destination arena isn't polluted by intermediate allocations
		// that stbi_load did to load the image.
		result = (unsigned char *)RT_ArenaCopy(arena, result, (*w)*(*h)*required_channel_count, 16);
		// and then we know we don't need the thread arena stuff anymore so free it
		RT_ArenaResetToMarker(&g_thread_arena, marker);
	}

	return result;
}

unsigned char *RT_LoadImageFromMemory(RT_Arena *arena, const void *memory, size_t memory_size, int *w, int *h, int *channel_count, int required_channel_count)
{
	RT_ArenaMarker marker = RT_ArenaGetMarker(&g_thread_arena);

	unsigned char *result = stbi_load_from_memory((const stbi_uc *)memory, memory_size, w, h, channel_count, required_channel_count);

	if (arena != &g_thread_arena)
	{
		result = (unsigned char *)RT_ArenaCopy(arena, result, (*w)*(*h)*required_channel_count, 16);
		RT_ArenaResetToMarker(&g_thread_arena, marker);
	}

	return result;
}

void RT_WritePNGToDisk(const char *path, int w, int h, int channel_count, const void *pixels, int stride_in_bytes)
{
	RT_ArenaMemoryScope(&g_thread_arena)
	{
		int result = stbi_write_png(path, w, h, channel_count, pixels, stride_in_bytes);
		RT_ASSERT(result); // TODO: Cool error handling / return an error code at least
	}
}