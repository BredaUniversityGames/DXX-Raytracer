#include "ImageReadWrite.h"

#include "Core/Arena.h"
#include "Core/String.h"
#include "Core/FileIO.h"

// TODO(daniel): These external libraries are in a weird place... RT/Renderer/Backend/DX12? This has nothing to do with DX12!!!!!!!!

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

#include "../DirectXTK12/src/DDSc.h"

RT_Image RT_LoadImageFromDisk(RT_Arena *arena, const char *path_c, int required_channel_count, bool is_srgb)
{
	RT_Image result = {};

	RT_ArenaMarker marker = RT_ArenaGetMarker(&g_thread_arena);

	RT_String path = RT_StringFromCString(path_c);
	RT_String ext  = RT_StringFindExtension(path);

	if (RT_StringsAreEqualNoCase(ext, RT_StringLiteral(".dds")))
	{
		RT_String memory;
		if (RT_ReadEntireFile(arena, path, &memory))
		{
			result = RT_LoadDDSFromMemory(memory);
		}
	}
	else
	{
		int w, h, channel_count;
		result.pixels = stbi_load(path_c, &w, &h, &channel_count, required_channel_count);

		if (result.pixels)
		{
			result.width           = (uint32_t)w;
			result.height          = (uint32_t)h;
			result.pitch           = required_channel_count*result.width;
			result.mip_count       = 1;

			switch (required_channel_count)
			{
				case 4: result.format = RT_TextureFormat_RGBA8; break;
				case 1: result.format = RT_TextureFormat_R8;    break;
				RT_INVALID_DEFAULT_CASE;
			}

			if (arena != &g_thread_arena)
			{
				// stbi_load will always use the thread arena, but if we wanted the result to be in a different arena
				// we'll just copy it. This means that the destination arena isn't polluted by intermediate allocations
				// that stbi_load did to load the image.
				result.pixels = RT_ArenaCopy(arena, result.pixels, w*h*required_channel_count, 16);
				// and then we know we don't need the thread arena stuff anymore so free it
				RT_ArenaResetToMarker(&g_thread_arena, marker);
			}
		}
	}

	if (is_srgb)
	{
		result.format = RT_TextureFormatToSRGB(result.format);
	}

	return result;
}

RT_Image RT_LoadImageFromMemory(RT_Arena *arena, const void *memory, size_t memory_size, int required_channel_count, bool is_srgb)
{
	RT_Image result = {};

	RT_ArenaMarker marker = RT_ArenaGetMarker(&g_thread_arena);

	int w, h, channel_count;
	result.pixels = stbi_load_from_memory((const stbi_uc *)memory, memory_size, &w, &h, &channel_count, required_channel_count);

	result.width     = (uint32_t)w;
	result.height    = (uint32_t)h;
	result.pitch     = required_channel_count*result.width;
	result.mip_count = 1;

	switch (required_channel_count)
	{
		case 4: result.format = RT_TextureFormat_RGBA8; break;
		case 1: result.format = RT_TextureFormat_R8;    break;
		RT_INVALID_DEFAULT_CASE;
	}

	if (is_srgb)
	{
		result.format = RT_TextureFormatToSRGB(result.format);
	}

	if (arena != &g_thread_arena)
	{
		result.pixels = RT_ArenaCopy(arena, result.pixels, w*h*required_channel_count, 16);
		RT_ArenaResetToMarker(&g_thread_arena, marker);
	}

	return result;
}

void RT_WritePNGToDisk(const char *path, int w, int h, int channel_count, const void *pixels, int stride_in_bytes)
{
	RT_ArenaMemoryScope(&g_thread_arena)
	{
		int result = stbi_write_png(path, w, h, channel_count, pixels, stride_in_bytes);

		if (!result)
		{
			fprintf(stderr, "[RT_WritePNGToDisk]: ERROR: Failed to write png: '%s'\n", path);
		}
	}
}

RT_Image RT_LoadDDSFromMemory(RT_String memory)
{
	RT_Image result = {};

	if (memory.count > 4294967296)
	{
		fprintf(stderr, "[RT_LoadDDSFromMemory]: ERROR: File too large, what are you doing! Your texture is more than 4GB!?\n");
		return result;
	}

	if (memory.count < sizeof(uint32_t) + sizeof(DDS_HEADER))
	{
		fprintf(stderr, "[RT_LoadDDSFromMemory]: ERROR: File too small to fit a valid dds texture.\n");
		return result;
	}

	char *data = memory.bytes;

	const uint32_t dds_magic = 0x20534444; // "DDS "

	if (memcmp(data, &dds_magic, 4) != 0)
	{
		fprintf(stderr, "RT_LoadDDSFromMemory]: ERROR: File did not start with \"DDS \", which does not match my expectations...\n");
		return result;
	}

	DDS_HEADER *header = (DDS_HEADER *)(data + 4);

	if (header->size       != sizeof(DDS_HEADER) ||
		header->ddspf.size != sizeof(DDS_PIXELFORMAT))
	{
		fprintf(stderr, "[RT_LoadDDSFromMemory]: ERROR: DDS header looks wrong...\n");
		return result;
	}

	DDS_HEADER_DXT10 *header_dxt10 = nullptr;

	if ((header->ddspf.flags & DDS_FOURCC) &&
		(header->ddspf.fourCC == MAKEFOURCC('D', 'X', '1', '0')))
	{
		if (memory.count < sizeof(uint32_t) + sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10))
		{
			fprintf(stderr, "[RT_LoadDDSFromMemory]: ERROR: DDS file looks like it has DXT10 extension, but it's too small!\n");
			return result;
		}

		header_dxt10 = (DDS_HEADER_DXT10 *)(header + 1);
	}

	DXGI_FORMAT dxgi_format;
	if (header_dxt10)
	{
		dxgi_format = header_dxt10->dxgiFormat;
	}
	else
	{
		dxgi_format = GetDXGIFormat(&header->ddspf);
	}

	RT_TextureFormat format;
	switch (dxgi_format)
	{
		case DXGI_FORMAT_BC1_UNORM:      format = RT_TextureFormat_BC1;      break;
		case DXGI_FORMAT_BC1_UNORM_SRGB: format = RT_TextureFormat_BC1_SRGB; break;
		case DXGI_FORMAT_BC2_UNORM:      format = RT_TextureFormat_BC2;      break;
		case DXGI_FORMAT_BC2_UNORM_SRGB: format = RT_TextureFormat_BC2_SRGB; break;
		case DXGI_FORMAT_BC3_UNORM:      format = RT_TextureFormat_BC3;      break;
		case DXGI_FORMAT_BC3_UNORM_SRGB: format = RT_TextureFormat_BC3_SRGB; break;
		case DXGI_FORMAT_BC4_UNORM:      format = RT_TextureFormat_BC4;      break;
		case DXGI_FORMAT_BC5_UNORM:      format = RT_TextureFormat_BC5;      break;
		case DXGI_FORMAT_BC7_UNORM:      format = RT_TextureFormat_BC7;      break;
		case DXGI_FORMAT_BC7_UNORM_SRGB: format = RT_TextureFormat_BC7_SRGB; break;
		default:
		{
			fprintf(stderr, "[RT_LoadDDSFromMemory]: ERROR: Unsupported DDS format. Use one of the BC formats.\n");
			return result;
		} break;
	}

	size_t bits_per_pixel = DDSBitsPerPixel(dxgi_format);
	if (bits_per_pixel == 0)
	{
		fprintf(stderr, "[RT_LoadDDSFromMemory]: ERROR: Unknown DXGI format. Something's wrong with your texture, or you're using a format that's too new.\n");
		return result;
	}

	// Only one we care about.
	const uint32_t D3D_RESOURCE_DIMENSION_TEXTURE2D = 3;

	if (header_dxt10)
	{
		if (header_dxt10->resourceDimension != D3D_RESOURCE_DIMENSION_TEXTURE2D)
		{
			fprintf(stderr, "[RT_LoadDDSFromMemory]: ERROR: Only 2D textures are supported.\n");
			return result;
		}
	}
	else
	{
		if ((header->flags & DDS_HEADER_FLAGS_VOLUME) ||
			(header->caps2 & DDS_CUBEMAP)) // if we ever need cubemaps for something we can readdress this
		{
			fprintf(stderr, "[RT_LoadDDSFromMemory]: ERROR: Only 2D textures are supported.\n");
			return result;
		}
	}

	if (header_dxt10)
	{
		if (header_dxt10->arraySize == 0)
		{
			fprintf(stderr, "[RT_LoadDDSFromMemory]: ERROR: DDS array size is 0. That's not a valid state for it to be in, so this texture is probably corrupt.\n");
			return result;
		}

		if (header_dxt10->arraySize > 1)
		{
			fprintf(stderr, "[RT_LoadDDSFromMemory]: Warning: DDS contains a texture array. Only first texture will be used.\n");
		}
	}

	if (!RT_IS_POW2(header->width) || !RT_IS_POW2(header->height))
	{
		fprintf(stderr, "[RT_LoadDDSFromMemory]: ERROR: DDS texture dimensions are not a power of two (%ux%u). That is very much wrong.\n", header->width, header->height);
		return result;
	}

	if (header->mipMapCount > 16)
	{
		fprintf(stderr, "[RT_LoadDDSFromMemory]: ERROR: Crazy number of mips! You can't have more than 16 mips!\n");
		return result;
	}

	uint32_t expected_mip_count = RT_U32Log2(RT_MIN(header->width, header->height));
	if (header->mipMapCount > expected_mip_count)
	{
		fprintf(stderr, "[RT_LoadDDSFromMemory]: Warning: There are more mips in this DDS than I think makes sense... (Expected: %u, Has: %u)\n", expected_mip_count, header->mipMapCount);
	}

	result.format    = format;
	result.width     = header->width;
	result.height    = header->height;
	result.pitch     = header->pitchOrLinearSize;
	result.mip_count = RT_MAX(1, header->mipMapCount);

	size_t bytes_per_pixel = bits_per_pixel / 8;

	char *mip_at = data + sizeof(uint32_t) + sizeof(DDS_HEADER) + (header_dxt10 ? sizeof(DDS_HEADER_DXT10) : 0);
	uint32_t mip_width  = result.width;
	uint32_t mip_height = result.height;
	for (size_t mip_index = 0; mip_index < result.mip_count; mip_index++)
	{
		result.mips[mip_index] = mip_at;

		// NOTE(daniel): There is an assumption here that rows are tightly packed, it seems.
		mip_at += mip_width*mip_height*bytes_per_pixel;
		mip_width  /= 2;
		mip_height /= 2;

		if (mip_width == 0 || mip_height == 0)
		{
			fprintf(stderr, "[RT_LoadDDSFromMemory]: Warning: I'm confused about the number of mips. Results may be wrong.\n");
			break;
		}
	}

	return result;
}

RT_Image RT_LoadDDSFromDisk(RT_Arena *arena, RT_String path)
{
	RT_Image result = {};

	// Don't want to spam when attempting to load DDS image first
	// fprintf(stderr, "[RT_LoadDDSFromDisk]: Attempting to load image: '%.*s'\n", RT_ExpandString(path));

	RT_String memory;
	if (RT_ReadEntireFile(arena, path, &memory))
	{
		result = RT_LoadDDSFromMemory(memory);
	}

	return result;
}