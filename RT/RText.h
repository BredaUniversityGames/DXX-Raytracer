/*
 *
 * RT standard Extensions of the descent engine.
 *
 */

#ifndef _RT_EXT_H
#define _RT_EXT_H

#ifdef RT_DX12
#define _RT_DRAW_POLY const int objNum, ubyte object_type,
#define _RT_DRAW_POLY_SEND obj->objNum, obj->type,
#define _RT_DRAW_POLY_SEND_NULL 0, OBJ_NONE,
#else
#define _RT_DRAW_POLY
#define _RT_DRAW_POLY_SEND
#define _RT_DRAW_POLY_SEND_NULL
#endif

 //Pauses the game without showing a window like do_pause does in gamecntl.c
 //TODO, maybe still resume the renderer if that is blocking it.
int RT_DoRealPause();

#endif //_RT_EXT_H