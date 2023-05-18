// TODO: I don't want to include Windows.h all the time

#include <windows.h>

// ------------------------------------------------------------------

#include "VirtualMemory.h"

void *RT_ReserveVirtualMemory(size_t size)
{
	void *result = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
	return result;
}

bool RT_CommitVirtualMemory(void *address, size_t size)
{
	void *result = VirtualAlloc(address, size, MEM_COMMIT, PAGE_READWRITE);
	return !!result;
}

void RT_DecommitVirtualMemory(void *address, size_t size)
{
	VirtualFree(address, size, MEM_DECOMMIT);
}

void RT_ReleaseVirtualMemory(void *address)
{
	VirtualFree(address, 0, MEM_RELEASE);
}