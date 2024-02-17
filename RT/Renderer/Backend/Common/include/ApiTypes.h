#pragma once

// The descent code likes to mess with padding, so public headers should use pragma pack to
// restore expected packing!
#pragma pack(push, 8)

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

// ------------------------------------------------------------------
// C-specific stuff

#ifndef __cplusplus

#define RT_API extern 
#define RT_EXPORT extern __declspec(dllexport)

// C doesn't have a built-in bool type, but it does have _Bool (which is the same thing). This awesome header defines bool as a macro. Cool.
#include <stdbool.h>

// Much like the above, there is a puny header for defining _Alignof to be alignof, but it wasn't included with Visual Studio until very
// recently, so I am just going to do it myself!!!!
#ifndef alignof
#define alignof(x) _Alignof(x)
#endif

#ifndef thread_local
// MSVC does not implement _Thread_local for C even though they have __declspec(thread) which is the same thing!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define thread_local __declspec(thread)
#endif

#else

#define RT_API extern "C" 
#define RT_EXPORT extern "C" __declspec(dllexport)

#endif

// ------------------------------------------------------------------

typedef struct RT_Arena RT_Arena;

typedef struct RT_String
{
	char  *bytes;
	size_t count;
} RT_String;

typedef struct RT_StringNode
{
	struct RT_StringNode *next;
	RT_String string;
} RT_StringNode;

typedef struct RT_Vec2i
{
	int x, y;
} RT_Vec2i;

typedef struct RT_Vec2
{
	float x, y;
} RT_Vec2;

typedef struct RT_Vec3i
{
	int x, y, z;
} RT_Vec3i;

typedef struct RT_Vec3
{
	float x, y, z;
} RT_Vec3;

typedef struct RT_Vec4
{
	union
	{
		struct
		{
			float x, y, z, w;
		};

		RT_Vec3 xyz;
		RT_Vec2 xy;
	};
} RT_Vec4;

// Typical gamedev 3x4 matrix where the missing 4th row
// is presumed to be 0, 0, 0, 1
typedef struct RT_Mat34
{
	union
	{
		float e[3][4];
		struct
		{
			RT_Vec4 r0;
			RT_Vec4 r1;
			RT_Vec4 r2;
		};
	};
} RT_Mat34;

typedef struct RT_Mat4
{
	union
	{
		float e[4][4];
		struct
		{
			RT_Vec4 r0;
			RT_Vec4 r1;
			RT_Vec4 r2;
			RT_Vec4 r3;
		};
		RT_Mat34 mat34;
	};
} RT_Mat4;

typedef struct RT_Quat
{
	union
	{
		struct
		{
			float x, y, z, w;
		};
		float e[4];
		RT_Vec3 xyz;
		RT_Vec4 xyzw;
	};
} RT_Quat;

typedef enum RT_LightKind
{
	RT_LightKind_Area_Sphere = 0,
	RT_LightKind_Area_Rect	 = 1,
} RT_LightKind;

#define RT_LIGHT_SCALE (1000.0f)

typedef struct RT_Light
{
	uint8_t kind;
	uint8_t spot_angle;
	uint8_t spot_softness;
	uint8_t spot_vignette;
	uint32_t emission;
	RT_Mat34 transform;
} RT_Light;

typedef struct RT_Vertex
{
	RT_Vec3 pos;
	RT_Vec2 uv;
	RT_Vec3 normal;
	RT_Vec3 tangent;
	RT_Vec3 bitangent;
} RT_Vertex;

typedef struct RT_RasterTriVertex
{
	RT_Vec3 pos;
	RT_Vec2 uv;
	RT_Vec4 color;
	uint32_t texture_index;
} RT_RasterTriVertex;

typedef struct RT_RasterLineVertex
{
	RT_Vec3 pos;
	RT_Vec4 color;
} RT_RasterLineVertex;

typedef uint32_t RT_Index;

typedef struct RT_ResourceHandle
{
	union
	{
		struct 
		{
			uint32_t index;
			uint32_t generation;
		};
		uint64_t value;
	};
} RT_ResourceHandle;

typedef enum RT_TextureFormat
{
	RT_TextureFormat_RGBA8,
	RT_TextureFormat_RGBA8_SRGB,
	RT_TextureFormat_R8,

	// DDS formats
	RT_TextureFormat_BC1,
	RT_TextureFormat_BC1_SRGB,
	RT_TextureFormat_BC2,
	RT_TextureFormat_BC2_SRGB,
	RT_TextureFormat_BC3,
	RT_TextureFormat_BC3_SRGB,
	RT_TextureFormat_BC4,
	RT_TextureFormat_BC5,
	// Skip BC6 because there's no point in supporting HDR textures here.
	RT_TextureFormat_BC7,
	RT_TextureFormat_BC7_SRGB,
} RT_TextureFormat;

static inline RT_TextureFormat RT_TextureFormatToSRGB(RT_TextureFormat format)
{
	switch (format)
	{
		case RT_TextureFormat_RGBA8: return RT_TextureFormat_RGBA8_SRGB;
		case RT_TextureFormat_BC1: return RT_TextureFormat_BC1_SRGB;
		case RT_TextureFormat_BC2: return RT_TextureFormat_BC2_SRGB;
		case RT_TextureFormat_BC3: return RT_TextureFormat_BC3_SRGB;
		case RT_TextureFormat_BC7: return RT_TextureFormat_BC7_SRGB;
	}

	return format;
}

typedef struct RT_Image
{
	RT_TextureFormat format;
	uint32_t         width;
	uint32_t         height;
	uint32_t         pitch;
	uint32_t         mip_count;
	union
	{
		void *pixels;
		void *mips[16];
	};
} RT_Image;

#define RT_RESOURCE_HANDLE_VALID(handle) (handle.index != 0)
#define RT_RESOURCE_HANDLES_MATCH(a, b) ((a).value == (b).value)

#ifdef __cplusplus
#define RT_RESOURCE_HANDLE_NULL RT_ResourceHandle{}
#else
#define RT_RESOURCE_HANDLE_NULL (RT_ResourceHandle){0}
#endif

// Don't forget to pop.
#pragma pack(pop)