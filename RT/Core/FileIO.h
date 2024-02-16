#pragma once

#include "ApiTypes.h"

// The descent code likes to mess with padding, so public headers should use pragma pack to
// restore expected packing!
#pragma pack(push, 8)

RT_API bool RT_ReadEntireFile(RT_Arena *arena, RT_String path, RT_String *result);

// Don't forget to pop.
#pragma pack(pop)
