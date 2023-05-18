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

inline void ExplainHRESULT(HRESULT hr)
{
    wchar_t *message = nullptr;

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER| 
                   FORMAT_MESSAGE_FROM_SYSTEM|
                   FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL,
                   hr,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (wchar_t *)&message,
                   0, NULL);

    OutputDebugStringW(L"HRESULT FAILED:\n");
    OutputDebugStringW(message);
    OutputDebugStringW(L"\n");

    LocalFree(message);
}

#define DX_CALL(hr_) \
    do {                                  \
		HRESULT dx_call_hr = hr_;         \
		if (FAILED(dx_call_hr))           \
		{                                 \
			OutputDebugStringA("DX_CALL failed at " __FILE__ ":" RT_STRINGIFY(__LINE__) "\n"); \
			ExplainHRESULT(dx_call_hr);   \
			RT_ASSERT(!"HRESULT FAILED"); \
		}                                 \
	}                                     \
	while (false)

#define SafeRelease(object) ((object) ? (object)->Release(), (object) = nullptr : (void)0)
#define DeferRelease(object) defer { SafeRelease(object); };
