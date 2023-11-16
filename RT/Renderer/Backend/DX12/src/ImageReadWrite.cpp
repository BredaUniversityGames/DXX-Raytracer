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

//  Code modified from DirectXTK12 : LoaderHelpers::LoadTextureDataFromFile (https://github.com/microsoft/DirectXTK12/blob/main/Src/LoaderHelpers.h)
bool RT_LoadDDSImageFromDisk(RT_Arena* arena, const char* path, const uint8_t** ddsData, const struct DDS_HEADER** header, const uint8_t** bitData, size_t* bitSize)
{
	RT_ArenaMarker marker = RT_ArenaGetMarker(&g_thread_arena);

	const uint32_t DDS_MAGIC = 0x20534444; // "DDS "

	FILE* f;

	if (0 == fopen_s(&f, path, "rb"))
	{
		long fileSize = 0;
		
		fseek(f, 0, SEEK_END); // seek to end of file
		fileSize = ftell(f); // get current file pointer
		fseek(f, 0, SEEK_SET); // seek back to beginning of file

		// success opening file
		//printf("Found and Opened DDS Texture: %s, File Size: %d\n", path, fileSize);

		// check for file too big > 4294967296 (32bit) and handle
		// Is this check necessary as fileSize is only a long?.. what happens if the file is bigger?
		if (fileSize > 4294967296)
		{
			printf("Error: file too large\n");
			return false;
		}

		// check for file too small // Need at least enough data to fill the header and magic number to be a valid DDS
		if (fileSize < (sizeof(uint32_t) + sizeof(DDS_HEADER)))
		{
			printf("Error: file too small\n");
			return false;
		}

		// read the contents of the file
		*ddsData = (uint8_t*) malloc(fileSize);  // allocate the memory to read the file
		fread((void*)*ddsData, fileSize, 1, f);	// read the file

		// close the file
		fclose(f);

		// DDS files always start with the same magic number ("DDS ") (yes there's a space there)
		if (memcmp(*ddsData, &DDS_MAGIC, 4) != 0)
		{
			printf("File did not begin with \"DDS \"\n");
			return false;
		}

		// get the header data
		*header = (DDS_HEADER*)(*ddsData + 4);

		// print header info
		//printf("Size: %d, Flags: %d, Height: %d, Width: %d, PitchOrLinearSize: %d, Depth: %d, MipMapCount: %d, PixelFormat->Size: %d, PixelFormat->Flags: %d, PixelFormat->fourCC: %d, PixelFormat->RGBBitCount: %d\n", 
		//	(*header)->size, (*header)->flags, (*header)->height, (*header)->width, (*header)->pitchOrLinearSize, (*header)->depth, (*header)->mipMapCount, (*header)->ddspf.size, (*header)->ddspf.flags, (*header)->ddspf.fourCC, (*header)->ddspf.RGBBitCount);

		// Verify header to validate DDS file
		if ((*header)->size != sizeof(DDS_HEADER) ||
			(*header)->ddspf.size != sizeof(DDS_PIXELFORMAT))
		{
			printf("Header not valid\n");
			return false;
		}

		// Check for DX10 extension
		bool bDXT10Header = false;
		if (((*header)->ddspf.flags & DDS_FOURCC) &&
			(MAKEFOURCC('D', 'X', '1', '0') == (*header)->ddspf.fourCC))
		{
			//printf("File has DX10 extension\n");

			// Must be long enough for both headers and magic value
			if (fileSize < (sizeof(uint32_t) + sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10)))
			{
				printf("File too small for DX10 extension\n");
				return false;
			}

			bDXT10Header = true;
		}

		// setup the pointers in the process request
		//*header = hdr;
		auto offset = sizeof(uint32_t) + sizeof(DDS_HEADER)
			+ (bDXT10Header ? sizeof(DDS_HEADER_DXT10) : 0u);
		*bitData = *ddsData + offset;
		*bitSize = fileSize - offset;
	}
	else 
	{
		f = 0;
	}

	if (arena != &g_thread_arena)
	{
		// stbi_load will always use the thread arena, but if we wanted the result to be in a different arena
		// we'll just copy it. This means that the destination arena isn't polluted by intermediate allocations
		// that stbi_load did to load the image.
		//result = (unsigned char*)RT_ArenaCopy(arena, result, (*w) * (*h) * required_channel_count, 16);
		// and then we know we don't need the thread arena stuff anymore so free it
		RT_ArenaResetToMarker(&g_thread_arena, marker);
	}

	return true;
}