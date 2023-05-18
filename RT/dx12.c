#include "dx12.h"
#include "Core/Arena.h"
#include "Core/MiniMath.h"

#include "internal.h"
#include "config.h"
#include "multi.h"
#include "rle.h"
#include "gamefont.h"
#include "grdef.h"
#include "globvars.h"

// ------------------------------------------------------------------

static RT_Arena g_arena;

int last_width = -1, last_height = -1;
int filter_blueship_wing = 0;
extern int linedotscale;

unsigned char decodebuf[1024 * 1024];

// An unbelievably horrible awful wretched hack
unsigned char blackpyro_tex1[8] = { 60, 59, 31, 31, 31, 31, 23, 60 };
unsigned char blackpyro_tex2[8] = { 255, 168, 255, 168, 226, 168, 224, 255 };

unsigned char whitepyro_tex1[8] = { 60, 59, 27, 27, 27, 27, 23, 60 };
unsigned char whitepyro_tex2[8] = { 255, 144, 255, 144, 226, 144, 224, 255 };

const float M_PI = 3.14159265f;

RT_Mat4 projection_matrix;

void dx12_start_frame()
{
	// Flush rasterizer
	RT_RasterRender();

	// Set viewport to current canvas
	RT_RasterSetViewport(grd_curcanv->cv_bitmap.bm_x, grd_curcanv->cv_bitmap.bm_y, Canvas_width, Canvas_height);

	projection_matrix = RT_Mat4Perspective(RT_RadiansFromDegrees(90.0f), 1.0f, 0.1f, 5000.0f);
}

void dx12_end_frame()
{
	// Flush rasterizer
	RT_RasterRender();

	// Reset viewport to default
	RT_RasterSetViewport(0.0f, 0.0f, grd_curscreen->sc_w, grd_curscreen->sc_h);
}

void dx12_init_texture(grs_bitmap* bm)
{
	bm->dxtexture = RT_ArenaAllocStruct(&g_arena, dx_texture);

	bm->dxtexture->handle = RT_RESOURCE_HANDLE_NULL;
	bm->dxtexture->lw = bm->dxtexture->w = bm->dxtexture->tw = bm->bm_w;
	bm->dxtexture->h = bm->dxtexture->th = bm->bm_h;

	//calculate u/v values that would make the resulting texture correctly sized
	bm->dxtexture->u = (float)((double)bm->dxtexture->w / (double)bm->dxtexture->tw);
	bm->dxtexture->v = (float)((double)bm->dxtexture->h / (double)bm->dxtexture->th);
}

void dx12_loadbmtexture_f(grs_bitmap* bm, int texfilt)
{
	unsigned char* buf;
#ifdef HAVE_LIBPNG
	char* bitmapname;
#endif

	while (bm->bm_parent)
		bm = bm->bm_parent;
	if (bm->dxtexture && RT_RESOURCE_HANDLE_VALID(bm->dxtexture->handle))
		return;
	buf = bm->bm_data;
#ifdef HAVE_LIBPNG
	if ((bitmapname = piggy_game_bitmap_name(bm)))
	{
		char filename[64];
		png_data pdata;

		sprintf(filename, "textures/%s.png", bitmapname);
		if (read_png(filename, &pdata))
		{
			RT_LOGF(RT_LOGSERVERITY_INFO, "%s: %ux%ux%i p=%i(%i) c=%i a=%i chans=%i\n", filename, pdata.width, pdata.height, pdata.depth, pdata.paletted, pdata.num_palette, pdata.color, pdata.alpha, pdata.channels);
			if (pdata.depth == 8 && pdata.color)
			{
				if (bm->gltexture == NULL)
					ogl_init_texture(bm->gltexture = ogl_get_free_texture(), pdata.width, pdata.height, flags | ((pdata.alpha || bm->bm_flags & BM_FLAG_TRANSPARENT) ? OGL_FLAG_ALPHA : 0));
				ogl_loadtexture(pdata.data, 0, 0, bm->gltexture, bm->bm_flags, pdata.paletted ? 0 : pdata.channels, texfilt);
				free(pdata.data);
				if (pdata.palette)
					free(pdata.palette);
				return;
			}
			else
			{
				RT_LOGF(RT_LOGSERVERITY_INFO, "%s: unsupported texture format: must be rgb, rgba, or paletted, and depth 8\n", filename);
				free(pdata.data);
				if (pdata.palette)
					free(pdata.palette);
			}
		}
	}
#endif
	if (!bm->dxtexture)
	{
		dx12_init_texture(bm);
	}
	else
	{
		if (RT_RESOURCE_HANDLE_VALID(bm->dxtexture->handle))
			return;
		if (bm->dxtexture->w == 0)
		{
			bm->dxtexture->lw = bm->bm_w;
			bm->dxtexture->w = bm->bm_w;
			bm->dxtexture->h = bm->bm_h;
		}
	}

	
	if (bm->bm_flags & BM_FLAG_RLE) {
		unsigned char* dbits;
		unsigned char* sbits;
		int i, data_offset;

		data_offset = 1;
		if (bm->bm_flags & BM_FLAG_RLE_BIG)
			data_offset = 2;

		sbits = &bm->bm_data[4 + (bm->bm_h * data_offset)];
		dbits = decodebuf;

		for (i = 0; i < bm->bm_h; i++) {
			// RT_LOGF(RT_LOGSERVERITY_MEDIUM, "RLE decoding bitmap %d\n", bm->bm_handle);
			gr_rle_decode(sbits, dbits);
			if (bm->bm_flags & BM_FLAG_RLE_BIG)
				sbits += (int)INTEL_SHORT(*((short*)&(bm->bm_data[4 + (i * data_offset)])));
			else
				sbits += (int)bm->bm_data[4 + i];
			dbits += bm->bm_w;
		}
		buf = decodebuf;

		if (Game_mode & GM_MULTI && Netgame.BlackAndWhitePyros) {
			char is_black_tex1 = 1;
			char is_black_tex2 = 1;
			for (i = 0; i < 8; i++) {
				if (bm->bm_data[4 + i] != blackpyro_tex1[i]) {
					is_black_tex1 = 0;
					break;
				}
			}

			for (i = 0; i < 8; i++) {
				if (bm->bm_data[4 + 64 + i] != blackpyro_tex2[i]) {
					is_black_tex2 = 0;
					break;
				}
			}

			if (is_black_tex1 || is_black_tex2) {
				for (i = 0; i < bm->bm_h * bm->bm_w; i++) {
					ubyte r = gr_current_pal[buf[i] * 3];
					ubyte g = gr_current_pal[buf[i] * 3 + 1];
					ubyte b = gr_current_pal[buf[i] * 3 + 2];

					ubyte max = r;
					if (g > max) { max = g; }
					if (b > max) { max = b; }

					if (r > g && g > b) {
						int replace = gr_find_closest_color(max / 4, max / 10, max / 3);
						buf[i] = replace;
					}
				}
			}

			char is_white_tex1 = 1;
			char is_white_tex2 = 1;
			for (i = 0; i < 8; i++) {
				if (bm->bm_data[4 + i] != whitepyro_tex1[i]) {
					is_white_tex1 = 0;
					break;
				}
			}

			for (i = 0; i < 8; i++) {
				if (bm->bm_data[4 + 64 + i] != whitepyro_tex2[i]) {
					is_white_tex2 = 0;
					break;
				}
			}

			if (is_white_tex1 || is_white_tex2) {
				for (i = 0; i < bm->bm_h * bm->bm_w; i++) {
					ubyte r = gr_current_pal[buf[i] * 3];
					ubyte g = gr_current_pal[buf[i] * 3 + 1];
					ubyte b = gr_current_pal[buf[i] * 3 + 2];

					ubyte max = r;
					if (g > max) { max = g; }
					if (b > max) { max = b; }

					if (g > r && g > b) {
						int replace = gr_find_closest_color(max, max, max);
						buf[i] = replace;
					}


				}
			}

			int lower_bound[24] = { 28,27,26,25,24,23,22,21,20,19,19,18,17,16,15,14,13,13,12,11,10,9,8 }; //bos
			int upper_bound[24] = { 57,55,54,52,50,49,48,47,45,44,42,41,39,38,36,35,33,32,30,29,27,25,23 }; // fos
			if (filter_blueship_wing && bm->bm_h == 64 && bm->bm_w == 64) {
				for (i = 0; i < bm->bm_h * bm->bm_w; i++) {
					int r = i / bm->bm_w;
					int c = i % bm->bm_w;

					int in_filter_area_1 = 0;
					int in_filter_area_2 = 0;

					if (r >= 2 && r <= 6) {
						in_filter_area_1 = 1;
					}

					if (r >= 36 && r <= 58) {
						if (lower_bound[r - 36] <= c && c <= upper_bound[r - 36]) {
							in_filter_area_2 = 1;
						}
					}

					if (in_filter_area_1) {
						ubyte b = gr_current_pal[buf[i] * 3 + 2];
						int replace = gr_find_closest_color(b / 2, b / 2, b);
						buf[i] = replace;
					}

					if (in_filter_area_2) {
						ubyte b = gr_current_pal[buf[i] * 3 + 2];
						int replace = gr_find_closest_color(b, b, b * 2);
						buf[i] = replace;
					}
				}
				filter_blueship_wing = 0;
			}
		}

	}

	RT_ArenaMemoryScope(&g_thread_arena)
	{
		// Upload texture
		uint32_t* pixels = dx12_load_bitmap_pixel_data(&g_thread_arena, bm);

		RT_UploadTextureParams tex_upload_params = {
			.format = RT_TextureFormat_RGBA8,
			.width = bm->dxtexture->w,
			.height = bm->dxtexture->h,
			.pixels = pixels,
			.name = "UI texture"
		};

		bm->dxtexture->handle = RT_UploadTexture(&tex_upload_params);
	}
}

int pow2ize(int x) {
	int i;
	for (i = 2; i < x; i *= 2) {};
	return i;
}

#define BITS_TO_BYTES(x)    (((x)+7)>>3)

#define FONTSCALE_X(x) ((float)(x)*(FNTScaleX))
#define FONTSCALE_Y(x) ((float)(x)*(FNTScaleY))

//takes the character AFTER being offset into font
#define INFONT(_c) ((_c >= 0) && (_c <= grd_curcanv->cv_font->ft_maxchar-grd_curcanv->cv_font->ft_minchar))

extern int gr_message_color_level;
#define CHECK_EMBEDDED_COLORS() if ((*text_ptr >= 0x01) && (*text_ptr <= 0x02)) { \
		text_ptr++; \
		if (*text_ptr){ \
			if (gr_message_color_level >= *(text_ptr-1)) \
				grd_curcanv->cv_font_fg_color = (unsigned char)*text_ptr; \
			text_ptr++; \
		} \
	} \
	else if (*text_ptr == 0x03) \
	{ \
		underline = 1; \
		text_ptr++; \
	} \
	else if ((*text_ptr >= 0x04) && (*text_ptr <= 0x06)){ \
		if (gr_message_color_level >= *text_ptr - 3) \
			grd_curcanv->cv_font_fg_color=(unsigned char)orig_color; \
		text_ptr++; \
	}

int get_font_total_width(grs_font* font) {
	if (font->ft_flags & FT_PROPORTIONAL) {
		int i, w = 0, c = font->ft_minchar;
		for (i = 0; c <= font->ft_maxchar; i++, c++) {
			if (font->ft_widths[i] < 0)
				RT_LOG(RT_LOGSERVERITY_HIGH, "heh?\n");
			w += font->ft_widths[i];
		}
		return w;
	}
	else {
		return font->ft_w * (font->ft_maxchar - font->ft_minchar + 1);
	}
}

void dx12_font_choose_size(grs_font* font, int gap, int* rw, int* rh) {
	int	nchars = font->ft_maxchar - font->ft_minchar + 1;
	int r, x, y, nc = 0, smallest = 999999, smallr = -1, tries;
	int smallprop = 10000;
	int h, w;
	for (h = 32; h <= 256; h *= 2) {
		//		h=pow2ize(font->ft_h*rows+gap*(rows-1));
		if (font->ft_h > h)continue;
		r = (h / (font->ft_h + gap));
		w = pow2ize((get_font_total_width(font) + (nchars - r) * gap) / r);
		tries = 0;
		do {
			if (tries)
				w = pow2ize(w + 1);
			if (tries > 3) {
				break;
			}
			nc = 0;
			y = 0;
			while (y + font->ft_h <= h) {
				x = 0;
				while (x < w) {
					if (nc == nchars)
						break;
					if (font->ft_flags & FT_PROPORTIONAL) {
						if (x + font->ft_widths[nc] + gap > w)break;
						x += font->ft_widths[nc++] + gap;
					}
					else {
						if (x + font->ft_w + gap > w)break;
						x += font->ft_w + gap;
						nc++;
					}
				}
				if (nc == nchars)
					break;
				y += font->ft_h + gap;
			}

			tries++;
		} while (nc != nchars);
		if (nc != nchars)
			continue;

		if (w * h == smallest) {//this gives squarer sizes priority (ie, 128x128 would be better than 512*32)
			if (w >= h) {
				if (w / h < smallprop) {
					smallprop = w / h;
					smallest++;//hack
				}
			}
			else {
				if (h / w < smallprop) {
					smallprop = h / w;
					smallest++;//hack
				}
			}
		}
		if (w * h < smallest) {
			smallr = 1;
			smallest = w * h;
			*rw = w;
			*rh = h;
		}
	}
	if (smallr <= 0)
		RT_LOG(RT_LOGSERVERITY_HIGH, "couldn't fit font?\n");
}

void dx12_init_font(grs_font* font)
{
	int oglflags = OGL_FLAG_ALPHA;
	int	nchars = font->ft_maxchar - font->ft_minchar + 1;
	int i, w, h, tw, th, x, y, curx = 0, cury = 0;
	unsigned char* fp;
	ubyte* data;
	int gap = 1; // x/y offset between the chars so we can filter

	dx12_font_choose_size(font, gap, &tw, &th);
	data = d_malloc(tw * th);
	memset(data, TRANSPARENCY_COLOR, tw * th); // map the whole data with transparency so we won't have borders if using gap
	gr_init_bitmap(&font->ft_parent_bitmap, BM_LINEAR, 0, 0, tw, th, tw, data);
	gr_set_transparent(&font->ft_parent_bitmap, 1);

	if (!(font->ft_flags & FT_COLOR))
		oglflags |= OGL_FLAG_NOCOLOR;

	if (font->ft_parent_bitmap.dxtexture == NULL)
		font->ft_parent_bitmap.dxtexture = RT_ArenaAllocStruct(&g_arena, dx_texture);
	dx12_init_texture(&font->ft_parent_bitmap);
	font->ft_parent_bitmap.dxtexture->h;

	font->ft_bitmaps = (grs_bitmap*)d_malloc(nchars * sizeof(grs_bitmap));
	h = font->ft_h;

	for (i = 0; i < nchars; i++)
	{
		if (font->ft_flags & FT_PROPORTIONAL)
			w = font->ft_widths[i];
		else
			w = font->ft_w;

		if (w < 1 || w>256)
			continue;

		if (curx + w + gap > tw)
		{
			cury += h + gap;
			curx = 0;
		}

		if (cury + h > th)
			RT_LOGF(RT_LOGSERVERITY_HIGH, "font doesn't really fit (%i/%i)?\n", i, nchars);

		if (font->ft_flags & FT_COLOR)
		{
			if (font->ft_flags & FT_PROPORTIONAL)
				fp = font->ft_chars[i];
			else
				fp = font->ft_data + i * w * h;
			for (y = 0; y < h; y++)
			{
				for (x = 0; x < w; x++)
				{
					font->ft_parent_bitmap.bm_data[curx + x + (cury + y) * tw] = fp[x + y * w];
					// Let's call this a HACK:
					// If we filter the fonts, the sliders will be messed up as the border pixels will have an
					// alpha value while filtering. So the slider bitmaps will not look "connected".
					// To prevent this, duplicate the first/last pixel-row with a 1-pixel offset.
					if (gap && i >= 99 && i <= 102)
					{
						// See which bitmaps need left/right shifts:
						// 99  = SLIDER_LEFT - shift RIGHT
						// 100 = SLIDER_RIGHT - shift LEFT
						// 101 = SLIDER_MIDDLE - shift LEFT+RIGHT
						// 102 = SLIDER_MARKER - shift RIGHT

						// shift left border
						if (x == 0 && i != 99 && i != 102)
							font->ft_parent_bitmap.bm_data[(curx + x + (cury + y) * tw) - 1] = fp[x + y * w];

						// shift right border
						if (x == w - 1 && i != 100)
							font->ft_parent_bitmap.bm_data[(curx + x + (cury + y) * tw) + 1] = fp[x + y * w];
					}
				}
			}
		}
		else
		{
			int BitMask, bits = 0, white = gr_find_closest_color(63, 63, 63);
			if (font->ft_flags & FT_PROPORTIONAL)
				fp = font->ft_chars[i];
			else
				fp = font->ft_data + i * BITS_TO_BYTES(w) * h;
			for (y = 0; y < h; y++) {
				BitMask = 0;
				for (x = 0; x < w; x++)
				{
					if (BitMask == 0) {
						bits = *fp++;
						BitMask = 0x80;
					}

					if (bits & BitMask)
						font->ft_parent_bitmap.bm_data[curx + x + (cury + y) * tw] = white;
					else
						font->ft_parent_bitmap.bm_data[curx + x + (cury + y) * tw] = 255;
					BitMask >>= 1;
				}
			}
		}
		gr_init_sub_bitmap(&font->ft_bitmaps[i], &font->ft_parent_bitmap, curx, cury, w, h);
		curx += w + gap;
	}
	dx12_loadbmtexture_f(&font->ft_parent_bitmap, GameCfg.TexFilt);
}

// Use: uidraw.c, menubar.c, keypad.c, icon.c, newmenu.c, kconfig.c
void dx12_urect(int left, int top, int right, int bot)
{
	float xo, yo, xf, yf, color_r, color_g, color_b, color_a;
	int c = grd_curcanv->cv_color;

	xo = (left + grd_curcanv->cv_bitmap.bm_x) / (float)last_width;
	xf = (right + 1 + grd_curcanv->cv_bitmap.bm_x) / (float)last_width;
	yo = 1.0 - (top + grd_curcanv->cv_bitmap.bm_y) / (float)last_height;
	yf = 1.0 - (bot + 1 + grd_curcanv->cv_bitmap.bm_y) / (float)last_height;

	xo = (xo - 0.5f) * 2.0f;
	xf = (xf - 0.5f) * 2.0f;
	yo = (yo - 0.5f) * 2.0f;
	yf = (yf - 0.5f) * 2.0f;

	color_r = CPAL2Tr(c);
	color_g = CPAL2Tg(c);
	color_b = CPAL2Tb(c);

	if (grd_curcanv->cv_fade_level >= GR_FADE_OFF)
		color_a = 1.0;
	else
		color_a = 1.0 - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0);

	RT_Vec4 col = RT_Vec4Make(color_r, color_g, color_b, color_a);
	RT_RasterTriVertex vertices[6] = {
		{.pos = { xf, yo, 0.0f }, .uv = { 1.0f, 0.0f }, .color = col, .texture_index = 0 },
		{.pos = { xf, yf, 0.0f }, .uv = { 1.0f, 1.0f }, .color = col, .texture_index = 0 },
		{.pos = { xo, yf, 0.0f }, .uv = { 0.0f, 1.0f }, .color = col, .texture_index = 0 },
		{.pos = { xf, yo, 0.0f }, .uv = { 1.0f, 0.0f }, .color = col, .texture_index = 0 },
		{.pos = { xo, yf, 0.0f }, .uv = { 0.0f, 1.0f }, .color = col, .texture_index = 0 },
		{.pos = { xo, yo, 0.0f }, .uv = { 0.0f, 0.0f }, .color = col, .texture_index = 0 }
	};

	RT_RasterTrianglesParams raster_tri_params = {0};
	raster_tri_params.texture_handle = RT_RESOURCE_HANDLE_NULL;
	raster_tri_params.num_vertices = 6;
	raster_tri_params.vertices = vertices;

	RT_RasterTriangles(&raster_tri_params, 1);
}

// gauges.c, uidraw.c, box.c
void dx12_ulinec(int left, int top, int right, int bot, int c)
{
	float xo, yo, xf, yf;

	xo = (left + grd_curcanv->cv_bitmap.bm_x + 0.5) / (float)last_width;
	xf = (right + grd_curcanv->cv_bitmap.bm_x + 1.0) / (float)last_width;
	yo = 1.0 - (top + grd_curcanv->cv_bitmap.bm_y + 0.5) / (float)last_height;
	yf = 1.0 - (bot + grd_curcanv->cv_bitmap.bm_y + 1.0) / (float)last_height;

	xo = (xo - 0.5f) * 2.0f;
	xf = (xf - 0.5f) * 2.0f;
	yo = (yo - 0.5f) * 2.0f;
	yf = (yf - 0.5f) * 2.0f;

	RT_Vec4 col = RT_Vec4Make(CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), (grd_curcanv->cv_fade_level >= GR_FADE_OFF) ? 1.0 : 1.0 - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0));
	RT_RasterLineVertex vertices[] = {
		{ .pos = RT_Vec3Make(xo, yo, 0.0f), .color = col },
		{ .pos = RT_Vec3Make(xf, yf, 0.0f), .color = col },
	};

	RT_RasterLines(vertices, 2);
}

// Use: render.c, terrain.c, automap.c, meddraw.c, draw.c
bool g3_draw_line(g3s_point* p0, g3s_point* p1)
{
	int c;
	c = grd_curcanv->cv_color;

	RT_Vec4 vp0 = RT_Vec4Make(f2fl(p0->p3_vec.x), f2fl(p0->p3_vec.y), f2fl(-p0->p3_vec.z), 1.0f);
	RT_Vec4 vp1 = RT_Vec4Make(f2fl(p1->p3_vec.x), f2fl(p1->p3_vec.y), f2fl(-p1->p3_vec.z), 1.0f);
	RT_Vec4 col = RT_Vec4Make(
		(float)((gr_palette[c * 3]) / 63.0),
		(float)((gr_palette[c * 3 + 1]) / 63.0),
		(float)((gr_palette[c * 3 + 2]) / 63.0),
		1.0f
	);

	vp0 = RT_Mat4TransformVec4(projection_matrix, vp0);
	vp1 = RT_Mat4TransformVec4(projection_matrix, vp1);

	vp0 = RT_Vec4Divs(vp0, vp0.w);
	vp1 = RT_Vec4Divs(vp1, vp1.w);

	RT_RasterLineVertex vertices[2] = {
		{.pos = vp0.xyz, .color = col },
		{.pos = vp1.xyz, .color = col },
	};

	RT_RasterLines(vertices, 2);
	return 1;
}

// This has not been tested/verified yet
void dx12_upixelc(int x, int y, int c)
{
	RT_Vec4 col = RT_Vec4Make(CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), 1.0);

	float xOff = linedotscale * 0.5f;
	float yOff = linedotscale * 0.5f;

	RT_RasterLineVertex vertices[] = {
		{ .pos = RT_Vec3Make((x + grd_curcanv->cv_bitmap.bm_x + xOff) / (float)last_width, 1.0 - (y + grd_curcanv->cv_bitmap.bm_y + yOff) / (float)last_height, 0.0f), .color = col },
		{ .pos = RT_Vec3Make((x + grd_curcanv->cv_bitmap.bm_x - xOff) / (float)last_width, 1.0 - (y + grd_curcanv->cv_bitmap.bm_y - yOff) / (float)last_height, 0.0f), .color = col },
	};

	RT_RasterLines(vertices, 2);
}

//unsigned char dx12_ugpixel(grs_bitmap* bitmap, int x, int y)
//{
//	GLint gl_draw_buffer;
//	ubyte buf[4];
//
//#ifndef OGLES
//	glGetIntegerv(GL_DRAW_BUFFER, &gl_draw_buffer);
//	glReadBuffer(gl_draw_buffer);
//#endif
//
//	glReadPixels(bitmap->bm_x + x, SHEIGHT - bitmap->bm_y - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buf);
//
//	return gr_find_closest_color(buf[0] / 4, buf[1] / 4, buf[2] / 4);
//}

void circle_array_init(RT_RasterTriVertex* circle_vertices, int nsides)
{
	float ang, next_ang, cos_ang, sin_ang, cos_next_ang, sin_next_ang;

	for (int i = 0; i < nsides; ++i) {
		int next_side = (i + 1) % nsides;

		ang = 2.0 * M_PI * i / nsides;
		next_ang = 2.0 * M_PI * (next_side) / nsides;

		cos_ang = cosf(ang);
		sin_ang = sinf(ang);
		cos_next_ang = cosf(next_ang);
		sin_next_ang = sinf(next_ang);

		circle_vertices[i * 3].pos = RT_Vec3Make(0.0f, 0.0f, 1.0f);
		circle_vertices[i * 3 + 1].pos = RT_Vec3Make(cos_next_ang, sin_next_ang, 1.0f);
		circle_vertices[i * 3 + 2].pos = RT_Vec3Make(cos_ang, sin_ang, 1.0f);
	}
}

// Use: meddraw.c (draw_seg_objects), automap.c (draw_player, draw_automap), endlevel.c (draw_stars)
int g3_draw_sphere(g3s_point* pnt, fix rad)
{
	int c = grd_curcanv->cv_color;
	float scale = ((float)grd_curcanv->cv_bitmap.bm_w / grd_curcanv->cv_bitmap.bm_h);

	RT_Vec4 col = RT_Vec4Make(CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), 1.0f);
	RT_Vec3 translation = RT_Vec3Make(f2fl(pnt->p3_vec.x), f2fl(pnt->p3_vec.y), -f2fl(pnt->p3_vec.z));
	RT_Vec3 sc;

	if (scale >= 1)
	{
		rad /= scale;
		sc = RT_Vec3Make(f2fl(rad), f2fl(rad * scale), f2fl(rad));
	}
	else
	{
		rad *= scale;
		sc = RT_Vec3Make(f2fl(rad / scale), f2fl(rad), f2fl(rad));
	}

	RT_Mat4 transform = RT_Mat4FromTranslation(translation);
	transform = RT_Mat4Mul(transform, RT_Mat4FromScale(sc));

	RT_RasterTrianglesParams raster_tri_params = { 0 };
	raster_tri_params.texture_handle = RT_RESOURCE_HANDLE_NULL;
	raster_tri_params.num_vertices = 63;
	raster_tri_params.vertices = RT_ArenaAllocArray(&g_thread_arena, raster_tri_params.num_vertices, RT_RasterTriVertex);

	circle_array_init(raster_tri_params.vertices, 20);

	for (size_t s = 0; s < 20; ++s)
	{
		for (size_t v = 0; v < 3; ++v)
		{
			RT_Vec4 vPos = RT_Mat4TransformVec4(transform, RT_Vec4Make(raster_tri_params.vertices[s * 3 + v].pos.x, raster_tri_params.vertices[s * 3 + v].pos.y, 0.0f, 1.0f));
			vPos = RT_Mat4TransformVec4(projection_matrix, vPos);
			vPos = RT_Vec4Divs(vPos, vPos.w);

			raster_tri_params.vertices[s * 3 + v].pos = RT_Vec3Make(vPos.x, vPos.y, vPos.z);
			raster_tri_params.vertices[s * 3 + v].color = col;
			raster_tri_params.vertices[s * 3 + v].uv = RT_Vec2Make(0.5f, 0.5f);
			raster_tri_params.vertices[s * 3 + v].texture_index = 0;
		}
	}
	
	// Need one last triangle to make it a full circle
	raster_tri_params.vertices[60] = raster_tri_params.vertices[0];
	raster_tri_params.vertices[61] = raster_tri_params.vertices[1];
	raster_tri_params.vertices[62] = raster_tri_params.vertices[58];

	RT_RasterTriangles(&raster_tri_params, 1);

	return 0;
}

void dx12_drawcircle(int nsides, RT_Mat4* transform, RT_Vec4* col)
{
	RT_RasterTrianglesParams raster_tri_params = { 0 };
	raster_tri_params.texture_handle = RT_RESOURCE_HANDLE_NULL;
	raster_tri_params.num_vertices = nsides * 3 + 3;
	raster_tri_params.vertices = RT_ArenaAllocArray(&g_thread_arena, raster_tri_params.num_vertices, RT_RasterTriVertex);

	circle_array_init(raster_tri_params.vertices, nsides);

	for (size_t s = 0; s < nsides; ++s)
	{
		for (size_t v = 0; v < 3; ++v)
		{
			RT_Vec4 vPos = RT_Mat4TransformVec4(*transform, RT_Vec4Make(raster_tri_params.vertices[s * 3 + v].pos.x, raster_tri_params.vertices[s * 3 + v].pos.y, 0.0f, 1.0f));
			raster_tri_params.vertices[s * 3 + v].pos = RT_Vec3Make(vPos.x * 2.0f - 1.0f, vPos.y * 2.0f - 1.0f, vPos.z * 2.0f - 1.0f);

			raster_tri_params.vertices[s * 3 + v].color = *col;
			raster_tri_params.vertices[s * 3 + v].uv = RT_Vec2Make(0.5f, 0.5f);
			raster_tri_params.vertices[s * 3 + v].texture_index = 0;
		}
	}

	// Need one last triangle to make it a full circle
	raster_tri_params.vertices[raster_tri_params.num_vertices - 3] = raster_tri_params.vertices[0];
	raster_tri_params.vertices[raster_tri_params.num_vertices - 2] = raster_tri_params.vertices[1];
	raster_tri_params.vertices[raster_tri_params.num_vertices - 1] = raster_tri_params.vertices[nsides - 2];
	
	RT_RasterTriangles(&raster_tri_params, 1);
}

// Use: gauges.c (show_reticle)
int gr_ucircle(fix xc1, fix yc1, fix r1)
{
	int c, nsides;
	c = grd_curcanv->cv_color;

	RT_Vec4 col = RT_Vec4Make(CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), (grd_curcanv->cv_fade_level >= GR_FADE_OFF) ? 1.0 : 1.0 - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0));
	RT_Vec3 translation = RT_Vec3Make((f2fl(xc1) + grd_curcanv->cv_bitmap.bm_x + 0.5) / (float)last_width, 1.0 - (f2fl(yc1) + grd_curcanv->cv_bitmap.bm_y + 0.5) / (float)last_height, 0);
	RT_Vec3 scale = RT_Vec3Make(f2fl(r1) / last_width, f2fl(r1) / last_height, 1.0);

	RT_Mat4 transform = RT_Mat4FromTranslation(translation);
	transform = RT_Mat4Mul(transform, RT_Mat4FromScale(scale));

	nsides = 10 + 2 * (int)(M_PI * f2fl(r1) / 19);
	dx12_drawcircle(nsides, &transform, &col);

	return 0;
}

// Not used anywhere.
int gr_circle(fix xc1, fix yc1, fix r1) {
	return gr_ucircle(xc1, yc1, r1);
}

// Use: draw.c, gauges.c
int gr_disk(fix x, fix y, fix r)
{
	int c, nsides;
	c = grd_curcanv->cv_color;

	RT_Vec4 col = RT_Vec4Make(CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), (grd_curcanv->cv_fade_level >= GR_FADE_OFF) ? 1.0 : 1.0 - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0));
	RT_Vec3 translation = RT_Vec3Make((f2fl(x) + grd_curcanv->cv_bitmap.bm_x + 0.5) / (float)last_width,
		1.0 - (f2fl(y) + grd_curcanv->cv_bitmap.bm_y + 0.5) / (float)last_height, 0);
	RT_Vec3 scale = RT_Vec3Make(f2fl(r) / last_width, f2fl(r) / last_height, 1.0);
	RT_Mat4 transform = RT_Mat4FromTranslation(translation);
	transform = RT_Mat4Mul(transform, RT_Mat4FromScale(scale));

	nsides = 10 + 2 * (int)(M_PI * f2fl(r) / 19);
	dx12_drawcircle(nsides, &transform, &col);

	return 0;
}

int dx12_internal_string(int x, int y, const char* s)
{
	const char* text_ptr, * next_row, * text_ptr1;
	int width, spacing, letter;
	int xx, yy;
	int orig_color = grd_curcanv->cv_font_bg_color;
	int underline;

	next_row = s;
	yy = y;

	if (grd_curscreen->sc_canvas.cv_bitmap.bm_type != BM_OGL)
		RT_LOG(RT_LOGSERVERITY_HIGH, "carp.\n");
	while (next_row != NULL)
	{
		text_ptr1 = next_row;
		next_row = NULL;

		text_ptr = text_ptr1;
		xx = x;

		if (xx == 0x8000)
			xx = get_centered_x(text_ptr);

		while (*text_ptr)
		{
			int ft_w;

			if (*text_ptr == '\n')
			{
				next_row = &text_ptr[1];
				yy += FONTSCALE_Y(grd_curcanv->cv_font->ft_h) + FSPACY(1);
				break;
			}

			letter = (unsigned char)*text_ptr - grd_curcanv->cv_font->ft_minchar;
			get_char_width(text_ptr[0], text_ptr[1], &width, &spacing);

			underline = 0;
			if (!INFONT(letter) || (unsigned char)*text_ptr <= 0x06)
			{
				CHECK_EMBEDDED_COLORS() else {
					xx += spacing;
					text_ptr++;
				}

				if (underline)
				{
					ubyte save_c = (unsigned char)COLOR;

					gr_setcolor(grd_curcanv->cv_font_fg_color);
					gr_rect(xx, yy + grd_curcanv->cv_font->ft_baseline + 2, xx + grd_curcanv->cv_font->ft_w, yy + grd_curcanv->cv_font->ft_baseline + 3);
					gr_setcolor(save_c);
				}

				continue;
			}

			if (grd_curcanv->cv_font->ft_flags & FT_PROPORTIONAL)
				ft_w = grd_curcanv->cv_font->ft_widths[letter];
			else
				ft_w = grd_curcanv->cv_font->ft_w;

			if (grd_curcanv->cv_font->ft_flags & FT_COLOR)
				dx12_ubitmapm_cs(xx, yy, FONTSCALE_X(ft_w), FONTSCALE_Y(grd_curcanv->cv_font->ft_h), &grd_curcanv->cv_font->ft_bitmaps[letter], -1, F1_0);
			else {
				if (grd_curcanv->cv_bitmap.bm_type == BM_OGL)
					dx12_ubitmapm_cs(xx, yy, ft_w * (FONTSCALE_X(grd_curcanv->cv_font->ft_w) / grd_curcanv->cv_font->ft_w), FONTSCALE_Y(grd_curcanv->cv_font->ft_h), &grd_curcanv->cv_font->ft_bitmaps[letter], grd_curcanv->cv_font_fg_color, F1_0);
				else
					RT_LOG(RT_LOGSERVERITY_HIGH, "ogl_internal_string: non-color string to non-ogl dest\n");
			}

			xx += spacing;

			text_ptr++;
		}
	}

	return 0;
}

uint32_t* dx12_load_bitmap_pixel_data(RT_Arena* arena, grs_bitmap* bitmap)
{
	if (bitmap->bm_flags & BM_FLAG_RLE)
	{
		bitmap = rle_expand_texture(bitmap);
	}

	uint32_t* pixels = RT_ArenaAlloc(arena, sizeof(uint32_t) * bitmap->bm_w * bitmap->bm_h, alignof(uint32_t));
	unsigned char* dst_ptr = (unsigned char*)pixels;

	int data_format = 0;
	int dxo = 0, dyo = 0;
	int x, y, c, i;
	i = 0;
	for (y = 0; y < bitmap->bm_h; y++)
	{
		i = dxo + bitmap->bm_w * (y + dyo);
		for (x = 0; x < bitmap->bm_w; x++)
		{
			if (x < bitmap->bm_w && y < bitmap->bm_h)
			{
				if (data_format)
				{
					int j;

					for (j = 0; j < data_format; ++j)
						(*(dst_ptr++)) = bitmap->bm_data[i * data_format + j];
					i++;
					continue;
				}
				else
				{
					c = bitmap->bm_data[i++];
				}

#if 0
				(*(dst_ptr++)) = gr_palette[c * 3] * 4;
				(*(dst_ptr++)) = gr_palette[c * 3 + 1] * 4;
				(*(dst_ptr++)) = gr_palette[c * 3 + 2] * 4;
				(*(dst_ptr++)) = 255;
#else
				if (c == 254 && (bitmap->bm_flags & BM_FLAG_SUPER_TRANSPARENT))
				{
					(*(dst_ptr++)) = 255;
					(*(dst_ptr++)) = 255;
					(*(dst_ptr++)) = 255;
					(*(dst_ptr++)) = 0; // transparent pixel
				}
				else if ((c == 255 && (bitmap->bm_flags & BM_FLAG_TRANSPARENT)) || c == 256)
				{
					(*(dst_ptr++)) = 0;
					(*(dst_ptr++)) = 0;
					(*(dst_ptr++)) = 0;
					(*(dst_ptr++)) = 0; // transparent pixel
				}
				else
				{
					(*(dst_ptr++)) = gr_palette[c * 3] * 4;
					(*(dst_ptr++)) = gr_palette[c * 3 + 1] * 4;
					(*(dst_ptr++)) = gr_palette[c * 3 + 2] * 4;
					(*(dst_ptr++)) = 255;//not transparent
				}
#endif
			}
		}
	}

	return pixels;
}

bool dx12_ubitmapm_cs(int x, int y, int dw, int dh, grs_bitmap* bm, int c, int scale)
{
	if (!bm->dxtexture)
	{
		dx12_init_texture(bm);
		dx12_loadbmtexture_f(bm, GameCfg.TexFilt);
	}

	float xo, yo, xf, yf, u1, u2, v1, v2, color_r, color_g, color_b, h;

	x += grd_curcanv->cv_bitmap.bm_x;
	y += grd_curcanv->cv_bitmap.bm_y;

	if (dw < 0)
		dw = grd_curcanv->cv_bitmap.bm_w;
	else if (dw == 0)
		dw = bm->bm_w;
	if (dh < 0)
		dh = grd_curcanv->cv_bitmap.bm_h;
	else if (dh == 0)
		dh = bm->bm_h;

	h = (double)scale / (double)F1_0;

	xo = x / ((double)last_width * h);
	xf = (dw + x) / ((double)last_width * h);
	yo = 1.0 - y / ((double)last_height * h);
	yf = 1.0 - (dh + y) / ((double)last_height * h);

	xo = (xo - 0.5f) * 2.0f;
	xf = (xf - 0.5f) * 2.0f;
	yo = (yo - 0.5f) * 2.0f;
	yf = (yf - 0.5f) * 2.0f;

	if (bm->bm_x == 0) {
		u1 = 0;
		if (bm->bm_w == bm->dxtexture->w)
			u2 = bm->dxtexture->u;
		else
			u2 = (bm->bm_w + bm->bm_x) / (float)bm->dxtexture->tw;
	}
	else {
		u1 = bm->bm_x / (float)bm->dxtexture->tw;
		u2 = (bm->bm_w + bm->bm_x) / (float)bm->dxtexture->tw;
	}
	if (bm->bm_y == 0) {
		v1 = 0;
		if (bm->bm_h == bm->dxtexture->h)
			v2 = bm->dxtexture->v;
		else
			v2 = (bm->bm_h + bm->bm_y) / (float)bm->dxtexture->th;
	}
	else {
		v1 = bm->bm_y / (float)bm->dxtexture->th;
		v2 = (bm->bm_h + bm->bm_y) / (float)bm->dxtexture->th;
	}

	if (c < 0) {
		color_r = 1.0;
		color_g = 1.0;
		color_b = 1.0;
	}
	else {
		color_r = CPAL2Tr(c);
		color_g = CPAL2Tg(c);
		color_b = CPAL2Tb(c);
	}

	RT_Vec4 col = { color_r, color_g, color_b, 1.0f };
	RT_RasterTriVertex vertices[6] = {
		{.pos = { xf, yo, 0.0f }, .uv = { u2, v1 }, .color = col, .texture_index = 0 },
		{.pos = { xf, yf, 0.0f }, .uv = { u2, v2 }, .color = col, .texture_index = 0 },
		{.pos = { xo, yf, 0.0f }, .uv = { u1, v2 }, .color = col, .texture_index = 0 },
		{.pos = { xf, yo, 0.0f }, .uv = { u2, v1 }, .color = col, .texture_index = 0 },
		{.pos = { xo, yf, 0.0f }, .uv = { u1, v2 }, .color = col, .texture_index = 0 },
		{.pos = { xo, yo, 0.0f }, .uv = { u1, v1 }, .color = col, .texture_index = 0 }
	};

	RT_RasterTrianglesParams raster_tri_params = { 0 };
	raster_tri_params.texture_handle = bm->dxtexture->handle;
	raster_tri_params.num_vertices = 6;
	raster_tri_params.vertices = vertices;

	RT_RasterTriangles(&raster_tri_params, 1);
}

bool dx12_ubitblt(int dw, int dh, int dx, int dy, int sw, int sh, int sx, int sy, grs_bitmap* src, grs_bitmap* dst, int texfilt)
{
	// One of the use-cases of this function is to render a preview of a savegame in the load menu
}
