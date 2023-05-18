#pragma once

#include "ApiTypes.h"

#pragma pack(push, 8)

typedef struct RT_Arena RT_Arena;

RT_API unsigned char *RT_LoadImageFromDisk(RT_Arena *arena, const char *path, int *w, int *h, int *channel_count, int required_channel_count);
RT_API unsigned char *RT_LoadImageFromMemory(RT_Arena *arena, const void *memory, size_t memory_size, int *w, int *h, int *channel_count, int required_channel_count);
RT_API void RT_WritePNGToDisk(const char *path, int w, int h, int channel_count, const void *pixels, int stride_in_bytes);

#pragma pack(pop)
