#pragma once

#include "ApiTypes.h"

#pragma pack(push, 8)

typedef struct RT_Arena RT_Arena;

RT_API RT_Image RT_LoadImageFromDisk(RT_Arena *arena, const char *path, int required_channel_count, bool is_srgb);
RT_API RT_Image RT_LoadImageFromMemory(RT_Arena *arena, const void *memory, size_t memory_size, int required_channel_count, bool is_srgb);
RT_API RT_Image RT_LoadDDSFromDisk(RT_Arena *arena, RT_String path);
RT_API RT_Image RT_LoadDDSFromMemory(RT_String memory);

RT_API void RT_WritePNGToDisk(const char *path, int w, int h, int channel_count, const void *pixels, int stride_in_bytes);

#pragma pack(pop)
