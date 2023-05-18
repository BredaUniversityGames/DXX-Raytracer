#ifndef _DX_12_H
#define _DX_12_H

#include "ApiTypes.h"
#include "Renderer.h"

#include "gr.h"

typedef struct _dx_texture
{
	RT_ResourceHandle handle;
	int w, h, tw, th, lw;
	float u, v;
} dx_texture;

void dx12_start_frame();
void dx12_end_frame();

void dx12_urect(int left, int top, int right, int bot);

void dx12_init_texture(grs_bitmap* bm);
int dx12_internal_string(int x, int y, const char* s);
void dx12_init_font(grs_font* font);
uint32_t* dx12_load_bitmap_pixel_data(RT_Arena* arena, grs_bitmap* bitmap);
bool dx12_ubitmapm_cs(int x, int y, int dw, int dh, grs_bitmap* bm, int c, int scale);
bool dx12_ubitblt(int dw, int dh, int dx, int dy, int sw, int sh, int sx, int sy, grs_bitmap* src, grs_bitmap* dst, int texfilt);

#endif //_DX_12_H