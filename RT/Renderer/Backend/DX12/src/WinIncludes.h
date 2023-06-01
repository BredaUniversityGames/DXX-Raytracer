#pragma once

#define NONEARFAR
#include <Windows.h>
//#include <shellapi.h>
#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>

#include "..\DXC\inc\dxcapi.h"
//#include "..\D3DX\d3dx12.h"

#if defined(CreateWindow)
#undef CreateWindow
#endif

#if defined(LoadImage)
#undef LoadImage
#endif

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#if defined(near)
#undef near
#endif

#if defined(far)
#undef far
#endif

#include <assert.h>
#include <stdint.h>

#include "Core/Common.h"
#include "Core/Arena.h"

inline void ExplainHRESULT(HRESULT hr, char *title, char *file, int line)
{
    char *message = nullptr;

    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER| 
                   FORMAT_MESSAGE_FROM_SYSTEM|
                   FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL,
                   hr,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (char *)&message,
                   0, NULL);

    char *formatted = RT_ArenaPrintF(&g_thread_arena, "\nError code: 0x%X\n\n%s", (uint32_t)hr, message);
    RT_FATAL_ERROR_(formatted, title, file, line);
}

#define DX_CALL(hr_) \
    do {                                  \
		HRESULT dx_call_hr = hr_;         \
		if (FAILED(dx_call_hr))           \
		{                                 \
			ExplainHRESULT(dx_call_hr, "Fatal DirectX Error", __FILE__, __LINE__); \
		}                                 \
	}                                     \
	while (false)

#define SafeRelease(object) ((object) ? (object)->Release(), (object) = nullptr : (void)0)
#define DeferRelease(object) defer { SafeRelease(object); };
