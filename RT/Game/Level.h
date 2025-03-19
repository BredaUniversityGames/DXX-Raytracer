#ifndef _RT_LEVEL_H
#define _RT_LEVEL_H

#include "ApiTypes.h"

typedef struct RT_Light RT_Light;
void RT_FindAndSubmitNearbyLights(RT_Vec3 player_pos);

bool RT_LoadLevel();
void RT_RenderLevel(RT_Vec3 player_pos);
bool RT_UnloadLevel();

bool RT_UploadLevelGeometry(RT_ResourceHandle* level_handle, RT_ResourceHandle* portals_handle);

#endif //_RT_LEVEL_H