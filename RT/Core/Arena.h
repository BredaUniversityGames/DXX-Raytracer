#pragma once

#include "Common.h"

#pragma pack(push, 8)

#if defined(DEBUG)
#define RT_ARENA_DEBUG
#endif

typedef struct RT_ArenaAllocationInfo
{
	struct RT_ArenaAllocationInfo *next;
	struct RT_ArenaAllocationInfo *prev;

	const char *file;
	int line;

	void *base;
	size_t size;
} RT_ArenaAllocationInfo;

typedef struct RT_Arena
{
	char *committed;
	char *end;
	char *at;
	char *buffer;

	bool owns_memory;

#if defined(RT_ARENA_DEBUG)
	struct RT_Arena *next;
	struct RT_Arena *prev;
	RT_ArenaAllocationInfo *first_alloc_info;
	RT_ArenaAllocationInfo *last_alloc_info;
#endif
} RT_Arena;

typedef struct RT_ArenaList
{
    RT_Arena *first, *last;
} RT_ArenaList;

typedef struct RT_ArenaMarker
{
	RT_Arena *arena;
	char *at;
} RT_ArenaMarker;

#if defined(RT_ARENA_DEBUG)
#define RT_ARENA_DEBUG_PARAMS const char *file, int line, 
#define RT_ARENA_DEBUG_ARGS __FILE__, __LINE__,
#define RT_ARENA_DEBUG_FWD file, line, 
#else
#define RT_ARENA_DEBUG_PARAMS 
#define RT_ARENA_DEBUG_ARGS
#define RT_ARENA_DEBUG_FWD
#endif

RT_API RT_ArenaList RT_ArenaDebugGetArenaList(void);
RT_API RT_ArenaAllocationInfo *RT_ArenaDebugGetAllocations(RT_Arena *arena);

// How much address space to reserve for an arena
#define RT_ARENA_DEFAULT_CAPACITY (16ull << 30) // 16 GiB
// How much memory to commit at a time when we need to grow
#define RT_ARENA_COMMIT_CHUNK_SIZE (16ull << 10) // 16 KiB
// How much memory to keep committed when asking the arena to reset and decommit
#define RT_ARENA_DEFAULT_COMMIT_PRESERVE_THRESHOLD RT_ARENA_COMMIT_CHUNK_SIZE

// Initialize the arena with existing memory instead. That stops it from being a virtual memory backed arena, so that memory shoud be regular, committed memory.
RT_API void RT_ArenaInitWithMemory(RT_Arena *arena, void *memory, size_t size);

// Don't call these directly unless you know what you're doing
RT_API void *RT_ArenaAlloc_(RT_ARENA_DEBUG_PARAMS RT_Arena *arena, size_t size, size_t align);
RT_API void *RT_ArenaAllocNoZero_(RT_ARENA_DEBUG_PARAMS RT_Arena *arena, size_t size, size_t align);
RT_API void *RT_ArenaCopy_(RT_ARENA_DEBUG_PARAMS RT_Arena *arena, const void *src, size_t size, size_t align);
RT_API void *RT_ArenaResize_(RT_ARENA_DEBUG_PARAMS RT_Arena *arena, void *pointer, size_t old_size, size_t new_size, size_t align);
RT_API char *RT_ArenaPrintF_(RT_ARENA_DEBUG_PARAMS RT_Arena *arena, const char *format_string, ...);
RT_API char *RT_ArenaPrintFV_(RT_ARENA_DEBUG_PARAMS RT_Arena *arena, const char *format_string, va_list args);

// Allocates memory from the arena and memsets it to 0
#define RT_ArenaAlloc(arena, size, align) RT_ArenaAlloc_(RT_ARENA_DEBUG_ARGS arena, size, align)
// Allocates memory from the arena and doesn't memset it to 0
#define RT_ArenaAllocNoZero(arena, size, align) RT_ArenaAllocNoZero_(RT_ARENA_DEBUG_ARGS arena, size, align)
// Allocates memory from the arena and then copies the passed in buffer into that memory
#define RT_ArenaCopy(arena, src, size, align) RT_ArenaCopy_(RT_ARENA_DEBUG_ARGS arena, src, size, align)
// Realloc style API, will try to grow in place if possible
#define RT_ArenaResize(arena, pointer, old_size, new_size, align) RT_ArenaResize_(RT_ARENA_DEBUG_ARGS arena, pointer, old_size, new_size, align)

#define RT_ArenaAllocStruct(arena, type) (type *)RT_ArenaAlloc(arena, sizeof(type), alignof(type))
#define RT_ArenaAllocArray(arena, count, type) (type *)RT_ArenaAlloc(arena, (count)*sizeof(type), alignof(type))

#define RT_ArenaAllocStructNoZero(arena, type) (type *)RT_ArenaAllocNoZero(arena, sizeof(type), alignof(type))
#define RT_ArenaAllocArrayNoZero(arena, count, type) (type *)RT_ArenaAllocNoZero(arena, (count)*sizeof(type), alignof(type))

#define RT_ArenaCopyArray(arena, arr, count) RT_ArenaCopy(arena, arr, sizeof((arr)[0])*(count), 16)

RT_API size_t RT_ArenaSizeUsed(const RT_Arena *arena);
RT_API size_t RT_ArenaSizeCommitted(const RT_Arena *arena);
RT_API size_t RT_ArenaSizeRemaining(const RT_Arena *arena);
RT_API size_t RT_ArenaSizeRemainingForAlign(const RT_Arena *arena, size_t align);

RT_API void RT_ArenaReset(RT_Arena *arena);
RT_API void RT_ArenaDecommitExcess(RT_Arena *arena);
RT_API void RT_ArenaResetAndDecommit(RT_Arena *arena);
RT_API void RT_ArenaRelease(RT_Arena *arena);

RT_API RT_ArenaMarker RT_ArenaGetMarker(RT_Arena *arena);
RT_API void RT_ArenaResetToMarker(RT_Arena *arena, RT_ArenaMarker marker);

// Helpful in C code, some insane-o-style macro
#define RT_ArenaMemoryScope(Arena) \
	for (RT_ArenaMarker arena_marker__ = RT_ArenaGetMarker(Arena); arena_marker__.arena; RT_ArenaResetToMarker(Arena, arena_marker__), arena_marker__.arena = NULL)

// Thread local arena for cool temporary memory shenanigans!
RT_API thread_local RT_Arena g_thread_arena;

// Nice convenience functions for printing strings into arenas
// TODO: Version that returns a string view
#define RT_ArenaPrintF(arena, format_string, ...) RT_ArenaPrintF_(RT_ARENA_DEBUG_ARGS arena, format_string, ##__VA_ARGS__)
#define RT_ArenaPrintFV(arena, format_string, args) RT_ArenaPrintFV_(RT_ARENA_DEBUG_ARGS arena, format_string, args)

#pragma pack(pop)
