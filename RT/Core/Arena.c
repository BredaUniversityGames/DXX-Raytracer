#include <string.h>
#include <stdio.h>

// ------------------------------------------------------------------

#include "Arena.h"
#include "VirtualMemory.h"

thread_local RT_Arena g_thread_arena;

static thread_local RT_ArenaList g_debug_arena_list;

static bool RT_ArenaOwnsPointer(RT_Arena *arena, void *pointer)
{
    char *p = pointer;
    return p >= arena->buffer || p < arena->end;
}

static RT_ArenaAllocationInfo *RT_ArenaGetAllocationInfoHeader(RT_Arena *arena, void *pointer)
{
	RT_ArenaAllocationInfo *result = NULL;

    if (ALWAYS(RT_ArenaOwnsPointer(arena, pointer)))
    {
        result = ((RT_ArenaAllocationInfo *)pointer) - 1;
    }

    return result;
}

static void RT_ArenaValidate(RT_Arena *arena)
{
    RT_ASSERT(arena->committed || !arena->buffer);
    RT_ASSERT(arena->committed >= arena->at);
    RT_ASSERT(arena->at >= arena->buffer && arena->at <= arena->end);
    if (!arena->owns_memory)
    {
        RT_ASSERT(arena->committed == arena->end);
    }
}

static void RT_ArenaValidateInfos(RT_Arena *arena)
{
    (void)arena;

#if defined(RT_ARENA_DEBUG)
    if (arena->first_alloc_info)
    {
        RT_ASSERT((char *)arena->first_alloc_info >= arena->buffer && (char *)(arena->first_alloc_info + 1) <= arena->at);
        RT_ASSERT((char *)arena->last_alloc_info  >= arena->buffer && (char *)(arena->last_alloc_info  + 1) <= arena->at);
    }
#endif
}

RT_ArenaList RT_ArenaDebugGetArenaList(void)
{
    return g_debug_arena_list;
}

RT_ArenaAllocationInfo *RT_ArenaDebugGetAllocations(RT_Arena *arena)
{
    (void)arena;

	RT_ArenaAllocationInfo *result = NULL;
#if defined(RT_ARENA_DEBUG)
    result = arena->first_alloc_info;
#endif
    return result;
}

void RT_ArenaInitWithMemory(RT_Arena *arena, void *memory, size_t size)
{
    RT_ASSERT(arena->buffer == NULL);
    
    arena->buffer    = memory;
    arena->at        = arena->buffer;
    arena->end       = arena->buffer + size;
    arena->committed = arena->end;

    arena->owns_memory = false;
}

static void RT_ArenaSetAt(RT_Arena *arena, char *new_at)
{
    if (ALWAYS(new_at >= arena->buffer && new_at < arena->end))
    {
		if (new_at > arena->committed)
		{
			size_t to_commit = RT_ALIGN_POW2(new_at - arena->committed, RT_ARENA_COMMIT_CHUNK_SIZE);
			RT_CommitVirtualMemory(arena->committed, to_commit);

			arena->committed += to_commit;
		}

		arena->at = new_at;
    }
}

void *RT_ArenaAllocNoZero_(RT_ARENA_DEBUG_PARAMS RT_Arena *arena, size_t size, size_t align)
{
    if (!arena->buffer)
    {
        arena->buffer    = RT_ReserveVirtualMemory(RT_ARENA_DEFAULT_CAPACITY);
        arena->at        = arena->buffer;
        arena->end       = arena->buffer + RT_ARENA_DEFAULT_CAPACITY;
        arena->committed = arena->buffer;

        arena->owns_memory = true;
    }

#if defined(DEBUG)
    {
		RT_ArenaValidate(arena);

#if 0
        // TODO(daniel): I screwed up and this list ends up looping on itself. Gotta find out why.
        // Or not, who even uses this.

		bool arena_is_in_debug_list = false;

		for (RT_Arena *debug_arena = g_debug_arena_list.first;
			 debug_arena;
			 debug_arena = debug_arena->next)
		{
            if (debug_arena == arena)
            {
                arena_is_in_debug_list = true;
                break;
            }
		}

        if (!arena_is_in_debug_list)
        {
			RT_DLL_PUSH_BACK(g_debug_arena_list.first, g_debug_arena_list.last, arena);
        }
#endif
    }
#endif

    char *result = NULL;

    if (ALWAYS(RT_ArenaSizeRemainingForAlign(arena, align) >= size))
    {
#if defined(RT_ARENA_DEBUG)
        {
			RT_ArenaValidateInfos(arena);

			RT_ArenaAllocationInfo *info = (RT_ArenaAllocationInfo *)RT_ALIGN_POW2(arena->at, alignof(RT_ArenaAllocationInfo));
			RT_ArenaSetAt(arena, (char *)(info + 1));

			info->next = info->prev = NULL;

			info->file = file;
			info->line = line;
			info->base = (char *)RT_ALIGN_POW2(arena->at, align);
			info->size = size;

			RT_DLL_PUSH_BACK(arena->first_alloc_info, arena->last_alloc_info, info);
			RT_ArenaValidateInfos(arena);
        }
#endif

        result = (char *)RT_ALIGN_POW2(arena->at, align);
        RT_ArenaSetAt(arena, result + size);
    }

    return result;
}

void *RT_ArenaAlloc_(RT_ARENA_DEBUG_PARAMS RT_Arena *arena, size_t size, size_t align)
{
    void *result = RT_ArenaAllocNoZero_(RT_ARENA_DEBUG_FWD arena, size, align);
    memset(result, 0, size);

    return result;
}

void *RT_ArenaCopy_(RT_ARENA_DEBUG_PARAMS RT_Arena *arena, const void *src, size_t size, size_t align)
{
    void *result = NULL;

    if (src)
    {
		result = RT_ArenaAllocNoZero_(RT_ARENA_DEBUG_FWD arena, size, align);
		memcpy(result, src, size);
    }

    return result;
}

void *RT_ArenaResize_(RT_ARENA_DEBUG_PARAMS RT_Arena *arena, void *pointer, size_t old_size, size_t new_size, size_t align)
{
    if (new_size <= old_size)
    {
        return pointer;
    }

    if (!pointer)
    {
        return RT_ArenaAllocNoZero_(RT_ARENA_DEBUG_FWD arena, new_size, align);
    }

    char *p = pointer;
    if (NEVER(p < arena->buffer || p >= arena->end))
    {
        // This pointer doesn't belong to this arena...
        return NULL;
    }

    void *result = pointer;

    if (p >= arena->at && p + old_size >= arena->at)
    {
        // We can resize in place
        RT_ArenaSetAt(arena, p + new_size);
#if defined(RT_ARENA_DEBUG)
        RT_ArenaAllocationInfo *info = RT_ArenaGetAllocationInfoHeader(arena, p);
        info->file = file;
        info->line = line;
        info->size = new_size;
#endif
    }
    else
    {
        // We have to copy
        result = RT_ArenaAllocNoZero_(RT_ARENA_DEBUG_FWD arena, new_size, align);
        memcpy(result, pointer, old_size);
    }

    return result;
}

size_t RT_ArenaSizeUsed(const RT_Arena *arena)
{
    return arena->at - arena->buffer;
}

size_t RT_ArenaSizeCommitted(const RT_Arena *arena)
{
    return arena->committed - arena->buffer;
}

size_t RT_ArenaSizeRemaining(const RT_Arena *arena)
{
    if (!arena->buffer)
    {
        return RT_ARENA_DEFAULT_CAPACITY;
    }
    return arena->end - arena->at;
}

size_t RT_ArenaSizeRemainingForAlign(const RT_Arena *arena, size_t align)
{
    size_t result = RT_ArenaSizeRemaining(arena);
    result = RT_ALIGN_DOWN_POW2(result, align);
    return result;
}

static void RT_ArenaResetTo(RT_Arena *arena, char *at)
{
#if defined(RT_ARENA_DEBUG)
    while (arena->last_alloc_info)
    {
        RT_ArenaAllocationInfo *info = arena->last_alloc_info;
        if ((char *)info->base + info->size <= at)
        {
            break;
        }
        RT_DLL_REMOVE(arena->first_alloc_info, arena->last_alloc_info, info);
        RT_ArenaValidateInfos(arena);
    }
#endif

    if (at)
    {
		if (ALWAYS(RT_ArenaOwnsPointer(arena, at)))
        {
			arena->at = at;
        }
    }
    else
    {
        arena->at = arena->buffer;
    }
}

void RT_ArenaReset(RT_Arena *arena)
{
    RT_ArenaResetTo(arena, arena->buffer);
}

void RT_ArenaDecommitExcess(RT_Arena *arena)
{
    if (arena->owns_memory)
    {
        char  *at_aligned     = (char *)RT_ALIGN_POW2(arena->at, RT_ARENA_COMMIT_CHUNK_SIZE);
        char  *decommit_from  = RT_MAX(at_aligned, arena->buffer + RT_ARENA_DEFAULT_COMMIT_PRESERVE_THRESHOLD);
        size_t decommit_bytes = RT_MAX(0, arena->committed - decommit_from);

        if (decommit_bytes > 0)
        {
			RT_DecommitVirtualMemory(decommit_from, decommit_bytes);
			arena->committed = decommit_from;
        }
    }

#if defined(DEBUG)
    RT_ArenaValidate(arena);
#endif
}

void RT_ArenaResetAndDecommit(RT_Arena *arena)
{
    RT_ArenaReset(arena);
    RT_ArenaDecommitExcess(arena);
}

void RT_ArenaRelease(RT_Arena *arena)
{
    if (arena->owns_memory)
    {
        void *buffer = arena->buffer;
        RT_ZERO_STRUCT(arena);
        RT_ReleaseVirtualMemory(buffer);
    }
    else
    {
        RT_ArenaReset(arena);
    }
}

RT_ArenaMarker RT_ArenaGetMarker(RT_Arena *arena)
{
    RT_ArenaMarker result = 
    {
		.arena = arena,
		.at    = arena->at,
    };
    return result;
}

void RT_ArenaResetToMarker(RT_Arena *arena, RT_ArenaMarker marker)
{
    RT_ASSERT(marker.arena == arena);
    RT_ASSERT(!marker.at || RT_ArenaOwnsPointer(arena, marker.at));

    RT_ArenaResetTo(arena, marker.at);
    RT_ArenaDecommitExcess(arena);
}

char *RT_ArenaPrintF_(RT_ARENA_DEBUG_PARAMS RT_Arena *arena, const char *format_string, ...)
{
    va_list args;
    va_start(args, format_string);
    char *result = RT_ArenaPrintFV_(RT_ARENA_DEBUG_FWD arena, format_string, args);
    va_end(args);

    return result;
}

char *RT_ArenaPrintFV_(RT_ARENA_DEBUG_PARAMS RT_Arena *arena, const char *format_string, va_list args)
{
    va_list args2;
    va_copy(args2, args);

    size_t count = vsnprintf(NULL, 0, format_string, args2);

    va_end(args2);

    char *result = RT_ArenaAllocNoZero_(RT_ARENA_DEBUG_FWD arena, count + 1, 16);
    vsnprintf(result, count + 1, format_string, args);

    return result;
}
