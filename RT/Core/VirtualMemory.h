#pragma once

#include "Common.h"

RT_API void *RT_ReserveVirtualMemory(size_t size);
RT_API bool  RT_CommitVirtualMemory(void *address, size_t size);
RT_API void  RT_DecommitVirtualMemory(void *address, size_t size);
RT_API void  RT_ReleaseVirtualMemory(void *address);
