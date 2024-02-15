//--------------------------------------------------------------------------------------
// DDS.h
//
// This header defines constants and structures that are useful when parsing
// DDS files.  DDS files were originally designed to use several structures
// and constants that are native to DirectDraw and are defined in ddraw.h,
// such as DDSURFACEDESC2 and DDSCAPS2.  This file defines similar
// (compatible) constants and structures so that one can use DDS files
// without needing to include ddraw.h.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#pragma once

#ifndef __DDSC_H__
#define __DDSC_H__

//#include <cstdint>
#include <dxgiformat.h>

//namespace DirectX
//{

#pragma pack(push,1)


    struct DDS_PIXELFORMAT
    {
        uint32_t    size;
        uint32_t    flags;
        uint32_t    fourCC;
        uint32_t    RGBBitCount;
        uint32_t    RBitMask;
        uint32_t    GBitMask;
        uint32_t    BBitMask;
        uint32_t    ABitMask;
    };

#define DDS_FOURCC      0x00000004  // DDPF_FOURCC
#define DDS_RGB         0x00000040  // DDPF_RGB
#define DDS_RGBA        0x00000041  // DDPF_RGB | DDPF_ALPHAPIXELS
#define DDS_LUMINANCE   0x00020000  // DDPF_LUMINANCE
#define DDS_LUMINANCEA  0x00020001  // DDPF_LUMINANCE | DDPF_ALPHAPIXELS
#define DDS_ALPHAPIXELS 0x00000001  // DDPF_ALPHAPIXELS
#define DDS_ALPHA       0x00000002  // DDPF_ALPHA
#define DDS_PAL8        0x00000020  // DDPF_PALETTEINDEXED8
#define DDS_PAL8A       0x00000021  // DDPF_PALETTEINDEXED8 | DDPF_ALPHAPIXELS
#define DDS_BUMPDUDV    0x00080000  // DDPF_BUMPDUDV
    // DDS_BUMPLUMINANCE 0x00040000

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
                ((uint32_t)((uint8_t)(ch0)) \
                | ((uint32_t)((uint8_t)(ch1)) << 8) \
                | ((uint32_t)((uint8_t)(ch2)) << 16) \
                | ((uint32_t)((uint8_t)(ch3)) << 24))
#endif /* MAKEFOURCC */

#ifndef DDSGLOBALCONST
#if defined(__GNUC__) && !defined(__MINGW32__)
#define DDSGLOBALCONST extern const __attribute__((weak))
#else
#define DDSGLOBALCONST extern const __declspec(selectany)
#endif
#endif /* DDSGLOBALCONST */

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_DXT1 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','1'), 0, 0, 0, 0, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_DXT2 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','2'), 0, 0, 0, 0, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_DXT3 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','3'), 0, 0, 0, 0, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_DXT4 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','4'), 0, 0, 0, 0, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_DXT5 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','5'), 0, 0, 0, 0, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_BC4_UNORM =
    { sizeof(struct DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('B','C','4','U'), 0, 0, 0, 0, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_BC4_SNORM =
    { sizeof(struct DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('B','C','4','S'), 0, 0, 0, 0, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_BC5_UNORM =
    { sizeof(struct DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('B','C','5','U'), 0, 0, 0, 0, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_BC5_SNORM =
    { sizeof(struct DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('B','C','5','S'), 0, 0, 0, 0, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_R8G8_B8G8 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('R','G','B','G'), 0, 0, 0, 0, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_G8R8_G8B8 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('G','R','G','B'), 0, 0, 0, 0, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_YUY2 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('Y','U','Y','2'), 0, 0, 0, 0, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_UYVY =
    { sizeof(struct DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('U','Y','V','Y'), 0, 0, 0, 0, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_A8R8G8B8 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_RGBA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_X8R8G8B8 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_RGB,  0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_A8B8G8R8 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_RGBA, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_X8B8G8R8 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_RGB,  0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_G16R16 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_RGB,  0, 32, 0x0000ffff, 0xffff0000, 0, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_R5G6B5 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_RGB, 0, 16, 0xf800, 0x07e0, 0x001f, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_A1R5G5B5 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_RGBA, 0, 16, 0x7c00, 0x03e0, 0x001f, 0x8000 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_X1R5G5B5 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_RGB, 0, 16, 0x7c00, 0x03e0, 0x001f, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_A4R4G4B4 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_RGBA, 0, 16, 0x0f00, 0x00f0, 0x000f, 0xf000 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_X4R4G4B4 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_RGB, 0, 16, 0x0f00, 0x00f0, 0x000f, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_R8G8B8 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_A8R3G3B2 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_RGBA, 0, 16, 0x00e0, 0x001c, 0x0003, 0xff00 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_R3G3B2 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_RGB, 0, 8, 0xe0, 0x1c, 0x03, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_A4L4 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_LUMINANCEA, 0, 8, 0x0f, 0, 0, 0xf0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_L8 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_LUMINANCE, 0, 8, 0xff, 0, 0, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_L16 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_LUMINANCE, 0, 16, 0xffff, 0, 0, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_A8L8 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_LUMINANCEA, 0, 16, 0x00ff, 0, 0, 0xff00 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_A8L8_ALT =
    { sizeof(struct DDS_PIXELFORMAT), DDS_LUMINANCEA, 0, 8, 0x00ff, 0, 0, 0xff00 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_L8_NVTT1 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_RGB, 0, 8, 0xff, 0, 0, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_L16_NVTT1 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_RGB, 0, 16, 0xffff, 0, 0, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_A8L8_NVTT1 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_RGBA, 0, 16, 0x00ff, 0, 0, 0xff00 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_A8 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_ALPHA, 0, 8, 0, 0, 0, 0xff };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_V8U8 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_BUMPDUDV, 0, 16, 0x00ff, 0xff00, 0, 0 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_Q8W8V8U8 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_BUMPDUDV, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 };

    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_V16U16 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_BUMPDUDV, 0, 32, 0x0000ffff, 0xffff0000, 0, 0 };

    // D3DFMT_A2R10G10B10/D3DFMT_A2B10G10R10 should be written using DX10 extension to avoid D3DX 10:10:10:2 reversal issue
    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_A2R10G10B10 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_RGBA, 0, 32, 0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000 };
    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_A2B10G10R10 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_RGBA, 0, 32, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000 };

    // We do not support the following legacy Direct3D 9 formats:
    // DDSPF_A2W10V10U10 = { sizeof(DDS_PIXELFORMAT), DDS_BUMPDUDV, 0, 32, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000 };
    // DDSPF_L6V5U5 = { sizeof(DDS_PIXELFORMAT), DDS_BUMPLUMINANCE, 0, 16, 0x001f, 0x03e0, 0xfc00, 0 };
    // DDSPF_X8L8V8U8 = { sizeof(DDS_PIXELFORMAT), DDS_BUMPLUMINANCE, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0 };

    // This indicates the DDS_HEADER_DXT10 extension is present (the format is in dxgiFormat)
    DDSGLOBALCONST struct DDS_PIXELFORMAT DDSPF_DX10 =
    { sizeof(struct DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','1','0'), 0, 0, 0, 0, 0 };

#define DDS_HEADER_FLAGS_TEXTURE        0x00001007  // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT
#define DDS_HEADER_FLAGS_MIPMAP         0x00020000  // DDSD_MIPMAPCOUNT
#define DDS_HEADER_FLAGS_VOLUME         0x00800000  // DDSD_DEPTH
#define DDS_HEADER_FLAGS_PITCH          0x00000008  // DDSD_PITCH
#define DDS_HEADER_FLAGS_LINEARSIZE     0x00080000  // DDSD_LINEARSIZE

#define DDS_HEIGHT 0x00000002 // DDSD_HEIGHT
#define DDS_WIDTH  0x00000004 // DDSD_WIDTH

#define DDS_SURFACE_FLAGS_TEXTURE 0x00001000 // DDSCAPS_TEXTURE
#define DDS_SURFACE_FLAGS_MIPMAP  0x00400008 // DDSCAPS_COMPLEX | DDSCAPS_MIPMAP
#define DDS_SURFACE_FLAGS_CUBEMAP 0x00000008 // DDSCAPS_COMPLEX

#define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES ( DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
                               DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
                               DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ )

#define DDS_CUBEMAP 0x00000200 // DDSCAPS2_CUBEMAP

#define DDS_FLAGS_VOLUME 0x00200000 // DDSCAPS2_VOLUME

    // Subset here matches D3D10_RESOURCE_DIMENSION and D3D11_RESOURCE_DIMENSION
    enum DDS_RESOURCE_DIMENSION //: uint32_t
    {
        DDS_DIMENSION_TEXTURE1D = 2,
        DDS_DIMENSION_TEXTURE2D = 3,
        DDS_DIMENSION_TEXTURE3D = 4,
    };

    // Subset here matches D3D10_RESOURCE_MISC_FLAG and D3D11_RESOURCE_MISC_FLAG
    enum DDS_RESOURCE_MISC_FLAG //: uint32_t
    {
        DDS_RESOURCE_MISC_TEXTURECUBE = 0x4L,
    };

    enum DDS_MISC_FLAGS2 //: uint32_t
    {
        DDS_MISC_FLAGS2_ALPHA_MODE_MASK = 0x7L,
    };

#ifndef DDS_ALPHA_MODE_DEFINED
#define DDS_ALPHA_MODE_DEFINED
    enum DDS_ALPHA_MODE //: uint32_t
    {
        DDS_ALPHA_MODE_UNKNOWN = 0,
        DDS_ALPHA_MODE_STRAIGHT = 1,
        DDS_ALPHA_MODE_PREMULTIPLIED = 2,
        DDS_ALPHA_MODE_OPAQUE = 3,
        DDS_ALPHA_MODE_CUSTOM = 4,
    };
#endif

    struct DDS_HEADER
    {
        uint32_t        size;
        uint32_t        flags;
        uint32_t        height;
        uint32_t        width;
        uint32_t        pitchOrLinearSize;
        uint32_t        depth; // only if DDS_HEADER_FLAGS_VOLUME is set in flags
        uint32_t        mipMapCount;
        uint32_t        reserved1[11];
        struct DDS_PIXELFORMAT ddspf;
        uint32_t        caps;
        uint32_t        caps2;
        uint32_t        caps3;
        uint32_t        caps4;
        uint32_t        reserved2;
    };

    struct DDS_HEADER_DXT10
    {
        enum DXGI_FORMAT     dxgiFormat;
        //uint8_t        dxgiFormat;
        uint32_t        resourceDimension;
        uint32_t        miscFlag; // see D3D11_RESOURCE_MISC_FLAG
        uint32_t        arraySize;
        uint32_t        miscFlags2; // see DDS_MISC_FLAGS2
    };

    //--------------------------------------------------------------------------------------
        // Return the BPP for a particular format
        //--------------------------------------------------------------------------------------
    inline size_t DDSBitsPerPixel(_In_ DXGI_FORMAT fmt)
    {
        switch (fmt)
        {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return 128;

        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
            return 96;

        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        case DXGI_FORMAT_Y416:
        case DXGI_FORMAT_Y210:
        case DXGI_FORMAT_Y216:
            return 64;

        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_AYUV:
        case DXGI_FORMAT_Y410:
        case DXGI_FORMAT_YUY2:
#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
        case DXGI_FORMAT_R10G10B10_7E3_A2_FLOAT:
        case DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT:
        case DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM:
#endif
            return 32;

        case DXGI_FORMAT_P010:
        case DXGI_FORMAT_P016:
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
        case DXGI_FORMAT_V408:
#endif
#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
        case DXGI_FORMAT_D16_UNORM_S8_UINT:
        case DXGI_FORMAT_R16_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X16_TYPELESS_G8_UINT:
#endif
            return 24;

        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_B5G6R5_UNORM:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
        case DXGI_FORMAT_A8P8:
        case DXGI_FORMAT_B4G4R4A4_UNORM:
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
        case DXGI_FORMAT_P208:
        case DXGI_FORMAT_V208:
#endif
            return 16;

        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_420_OPAQUE:
        case DXGI_FORMAT_NV11:
            return 12;

        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_A8_UNORM:
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
        case DXGI_FORMAT_AI44:
        case DXGI_FORMAT_IA44:
        case DXGI_FORMAT_P8:
#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
        case DXGI_FORMAT_R4G4_UNORM:
#endif
            return 8;

        case DXGI_FORMAT_R1_UNORM:
            return 1;

        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            return 4;

        case DXGI_FORMAT_UNKNOWN:
        case DXGI_FORMAT_FORCE_UINT:
        default:
            return 0;
        }
    }

//#define ISBITMASK( r,g,b,a ) ( ddpf.RBitMask == r && ddpf.GBitMask == g && ddpf.BBitMask == b && ddpf.ABitMask == a )
    inline bool ISBITMASK(const struct DDS_PIXELFORMAT* ddpf, uint32_t r, uint32_t g, uint32_t b, uint32_t a) { return ddpf->RBitMask == r && ddpf->GBitMask == g && ddpf->BBitMask == b && ddpf->ABitMask == a; }

    inline DXGI_FORMAT GetDXGIFormat(const struct DDS_PIXELFORMAT* ddpf)
    {
        if (ddpf->flags & DDS_RGB)
        {
            // Note that sRGB formats are written using the "DX10" extended header

            switch (ddpf->RGBBitCount)
            {
            case 32:
                if (ISBITMASK(ddpf, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
                {
                    return DXGI_FORMAT_R8G8B8A8_UNORM;
                }

                if (ISBITMASK(ddpf, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
                {
                    return DXGI_FORMAT_B8G8R8A8_UNORM;
                }

                if (ISBITMASK(ddpf, 0x00ff0000, 0x0000ff00, 0x000000ff, 0))
                {
                    return DXGI_FORMAT_B8G8R8X8_UNORM;
                }

                // No DXGI format maps to ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0) aka D3DFMT_X8B8G8R8

                // Note that many common DDS reader/writers (including D3DX) swap the
                // the RED/BLUE masks for 10:10:10:2 formats. We assume
                // below that the 'backwards' header mask is being used since it is most
                // likely written by D3DX. The more robust solution is to use the 'DX10'
                // header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

                // For 'correct' writers, this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
                if (ISBITMASK(ddpf, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
                {
                    return DXGI_FORMAT_R10G10B10A2_UNORM;
                }

                // No DXGI format maps to ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) aka D3DFMT_A2R10G10B10

                if (ISBITMASK(ddpf, 0x0000ffff, 0xffff0000, 0, 0))
                {
                    return DXGI_FORMAT_R16G16_UNORM;
                }

                if (ISBITMASK(ddpf, 0xffffffff, 0, 0, 0))
                {
                    // Only 32-bit color channel format in D3D9 was R32F
                    return DXGI_FORMAT_R32_FLOAT; // D3DX writes this out as a FourCC of 114
                }
                break;

            case 24:
                // No 24bpp DXGI formats aka D3DFMT_R8G8B8
                break;

            case 16:
                if (ISBITMASK(ddpf, 0x7c00, 0x03e0, 0x001f, 0x8000))
                {
                    return DXGI_FORMAT_B5G5R5A1_UNORM;
                }
                if (ISBITMASK(ddpf, 0xf800, 0x07e0, 0x001f, 0))
                {
                    return DXGI_FORMAT_B5G6R5_UNORM;
                }

                // No DXGI format maps to ISBITMASK(0x7c00,0x03e0,0x001f,0) aka D3DFMT_X1R5G5B5

                if (ISBITMASK(ddpf, 0x0f00, 0x00f0, 0x000f, 0xf000))
                {
                    return DXGI_FORMAT_B4G4R4A4_UNORM;
                }

                // NVTT versions 1.x wrote this as RGB instead of LUMINANCE
                if (ISBITMASK(ddpf, 0x00ff, 0, 0, 0xff00))
                {
                    return DXGI_FORMAT_R8G8_UNORM;
                }
                if (ISBITMASK(ddpf, 0xffff, 0, 0, 0))
                {
                    return DXGI_FORMAT_R16_UNORM;
                }

                // No DXGI format maps to ISBITMASK(0x0f00,0x00f0,0x000f,0) aka D3DFMT_X4R4G4B4

                // No 3:3:2:8 or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_A8P8, etc.
                break;

            case 8:
                // NVTT versions 1.x wrote this as RGB instead of LUMINANCE
                if (ISBITMASK(ddpf, 0xff, 0, 0, 0))
                {
                    return DXGI_FORMAT_R8_UNORM;
                }

                // No 3:3:2 or paletted DXGI formats aka D3DFMT_R3G3B2, D3DFMT_P8
                break;
            }
        }
        else if (ddpf->flags & DDS_LUMINANCE)
        {
            switch (ddpf->RGBBitCount)
            {
            case 16:
                if (ISBITMASK(ddpf, 0xffff, 0, 0, 0))
                {
                    return DXGI_FORMAT_R16_UNORM; // D3DX10/11 writes this out as DX10 extension
                }
                if (ISBITMASK(ddpf, 0x00ff, 0, 0, 0xff00))
                {
                    return DXGI_FORMAT_R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension
                }
                break;

            case 8:
                if (ISBITMASK(ddpf, 0xff, 0, 0, 0))
                {
                    return DXGI_FORMAT_R8_UNORM; // D3DX10/11 writes this out as DX10 extension
                }

                // No DXGI format maps to ISBITMASK(0x0f,0,0,0xf0) aka D3DFMT_A4L4

                if (ISBITMASK(ddpf, 0x00ff, 0, 0, 0xff00))
                {
                    return DXGI_FORMAT_R8G8_UNORM; // Some DDS writers assume the bitcount should be 8 instead of 16
                }
                break;
            }
        }
        else if (ddpf->flags & DDS_ALPHA)
        {
            if (8 == ddpf->RGBBitCount)
            {
                return DXGI_FORMAT_A8_UNORM;
            }
        }
        else if (ddpf->flags & DDS_BUMPDUDV)
        {
            switch (ddpf->RGBBitCount)
            {
            case 32:
                if (ISBITMASK(ddpf, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
                {
                    return DXGI_FORMAT_R8G8B8A8_SNORM; // D3DX10/11 writes this out as DX10 extension
                }
                if (ISBITMASK(ddpf, 0x0000ffff, 0xffff0000, 0, 0))
                {
                    return DXGI_FORMAT_R16G16_SNORM; // D3DX10/11 writes this out as DX10 extension
                }

                // No DXGI format maps to ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000) aka D3DFMT_A2W10V10U10
                break;

            case 16:
                if (ISBITMASK(ddpf, 0x00ff, 0xff00, 0, 0))
                {
                    return DXGI_FORMAT_R8G8_SNORM; // D3DX10/11 writes this out as DX10 extension
                }
                break;
            }

            // No DXGI format maps to DDPF_BUMPLUMINANCE aka D3DFMT_L6V5U5, D3DFMT_X8L8V8U8
        }
        else if (ddpf->flags & DDS_FOURCC)
        {
            if (MAKEFOURCC('D', 'X', 'T', '1') == ddpf->fourCC)
            {
                return DXGI_FORMAT_BC1_UNORM;
            }
            if (MAKEFOURCC('D', 'X', 'T', '3') == ddpf->fourCC)
            {
                return DXGI_FORMAT_BC2_UNORM;
            }
            if (MAKEFOURCC('D', 'X', 'T', '5') == ddpf->fourCC)
            {
                return DXGI_FORMAT_BC3_UNORM;
            }

            // While pre-multiplied alpha isn't directly supported by the DXGI formats,
            // they are basically the same as these BC formats so they can be mapped
            if (MAKEFOURCC('D', 'X', 'T', '2') == ddpf->fourCC)
            {
                return DXGI_FORMAT_BC2_UNORM;
            }
            if (MAKEFOURCC('D', 'X', 'T', '4') == ddpf->fourCC)
            {
                return DXGI_FORMAT_BC3_UNORM;
            }

            if (MAKEFOURCC('A', 'T', 'I', '1') == ddpf->fourCC)
            {
                return DXGI_FORMAT_BC4_UNORM;
            }
            if (MAKEFOURCC('B', 'C', '4', 'U') == ddpf->fourCC)
            {
                return DXGI_FORMAT_BC4_UNORM;
            }
            if (MAKEFOURCC('B', 'C', '4', 'S') == ddpf->fourCC)
            {
                return DXGI_FORMAT_BC4_SNORM;
            }

            if (MAKEFOURCC('A', 'T', 'I', '2') == ddpf->fourCC)
            {
                return DXGI_FORMAT_BC5_UNORM;
            }
            if (MAKEFOURCC('B', 'C', '5', 'U') == ddpf->fourCC)
            {
                return DXGI_FORMAT_BC5_UNORM;
            }
            if (MAKEFOURCC('B', 'C', '5', 'S') == ddpf->fourCC)
            {
                return DXGI_FORMAT_BC5_SNORM;
            }

            // BC6H and BC7 are written using the "DX10" extended header

            if (MAKEFOURCC('R', 'G', 'B', 'G') == ddpf->fourCC)
            {
                return DXGI_FORMAT_R8G8_B8G8_UNORM;
            }
            if (MAKEFOURCC('G', 'R', 'G', 'B') == ddpf->fourCC)
            {
                return DXGI_FORMAT_G8R8_G8B8_UNORM;
            }

            if (MAKEFOURCC('Y', 'U', 'Y', '2') == ddpf->fourCC)
            {
                return DXGI_FORMAT_YUY2;
            }

            // Check for D3DFORMAT enums being set here
            switch (ddpf->fourCC)
            {
            case 36: // D3DFMT_A16B16G16R16
                return DXGI_FORMAT_R16G16B16A16_UNORM;

            case 110: // D3DFMT_Q16W16V16U16
                return DXGI_FORMAT_R16G16B16A16_SNORM;

            case 111: // D3DFMT_R16F
                return DXGI_FORMAT_R16_FLOAT;

            case 112: // D3DFMT_G16R16F
                return DXGI_FORMAT_R16G16_FLOAT;

            case 113: // D3DFMT_A16B16G16R16F
                return DXGI_FORMAT_R16G16B16A16_FLOAT;

            case 114: // D3DFMT_R32F
                return DXGI_FORMAT_R32_FLOAT;

            case 115: // D3DFMT_G32R32F
                return DXGI_FORMAT_R32G32_FLOAT;

            case 116: // D3DFMT_A32B32G32R32F
                return DXGI_FORMAT_R32G32B32A32_FLOAT;

                // No DXGI format maps to D3DFMT_CxV8U8
            }
        }

        return DXGI_FORMAT_UNKNOWN;
    }

    inline DXGI_FORMAT MakeSRGB( DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

        case DXGI_FORMAT_BC1_UNORM:
            return DXGI_FORMAT_BC1_UNORM_SRGB;

        case DXGI_FORMAT_BC2_UNORM:
            return DXGI_FORMAT_BC2_UNORM_SRGB;

        case DXGI_FORMAT_BC3_UNORM:
            return DXGI_FORMAT_BC3_UNORM_SRGB;

        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

        case DXGI_FORMAT_B8G8R8X8_UNORM:
            return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;

        case DXGI_FORMAT_BC7_UNORM:
            return DXGI_FORMAT_BC7_UNORM_SRGB;

        default:
            return format;
        }
    }

#pragma pack(pop)

    static_assert(sizeof(struct DDS_PIXELFORMAT) == 32, "DDS pixel format size mismatch");
    static_assert(sizeof(struct DDS_HEADER) == 124, "DDS Header size mismatch");
    static_assert(sizeof(struct DDS_HEADER_DXT10) == 20, "DDS DX10 Extended Header size mismatch");

//} // namespace

#endif