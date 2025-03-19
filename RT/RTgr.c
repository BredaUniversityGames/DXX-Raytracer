#include "gr.h"
#include "internal.h"
#include "RTgr.h"
#include "u_mem.h"
#include "config.h"
#include "args.h"
#include "palette.h"
#include "3d.h"
#include "segment.h"
#include "maths.h"
#include "dxxerror.h"
#include "polyobj.h"
#include "logger.h"
#include "maths.h"
#include "hudmsg.h"
#include "text.h"
#include "render.h"
#include "gamefont.h"
#include "piggy.h"
#include "playsave.h"

#include "Core/Arena.h"
#include "Core/MiniMath.h"
#include "Core/Config.h"
#include "Core/String.h"

#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>

#include "dx12.h"
#include "globvars.h"
#include "GLTFLoader.h"
#include "rle.h"
#include "textures.h"
#include "object.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ImageReadWrite.h"
#include "RTmaterials.h"
#include "polymodel_viewer.h"
#include "material_viewer.h"
#include "Game/Lights.h"

#include "vers_id.h"
#include "Core/Config.h"
#include "Core/String.h"

#pragma warning(error: 4431) // default-int (variables)
#pragma warning(error: 4013) // default-int (function returns)

int sdl_video_flags = 0;
bool g_rt_enable_debug_menu;

CockpitSettings g_rt_cockpit_settings = {
	.front_cockpit_rotation = {0.247f, -RT_PI32, 0.000f},
	.front_cockpit_offset = {0.000f, -0.033f, -2.411f},
	.front_cockpit_scale = {1.182f, 1.000f, 1.000f},
	.back_cockpit_rotation = {-0.159f, -RT_PI32, 0.000f},
	.back_cockpit_offset = {0.000f, -0.760f, -2.488f},
	.back_cockpit_scale = {1.000f, 1.000f, 1.000f},
};

RT_GLTFNode* g_rt_cockpit_gltf;
RT_FreeCamInfo g_rt_free_cam_info = { 0 };
int light_culling_heuristic = 1;
int max_rec_depth = 32;
float max_distance = 600;
float max_seg_distance = 500;

static RT_DynamicLightInfo   g_rt_default_dynamic_light_info;
static RT_WeaponLightAdjusts g_rt_default_light_adjusts[RT_LIGHT_ADJUST_ARRAY_SIZE];

uint64_t g_rt_frame_index;

// #define RT_DUMP_GAME_BITMAPS

#define BM_RTDX12 5 //whatever value, replaces BM_OGL.

RT_ResourceHandle mesh_handles[MAX_POLYGON_MODELS];

RT_Mat4 old_poly_matrix[MAX_OBJECTS];

#define OP_EOF          0   //eof
#define OP_DEFPOINTS    1   //defpoints
#define OP_FLATPOLY     2   //flat-shaded polygon
#define OP_TMAPPOLY     3   //texture-mapped polygon
#define OP_SORTNORM     4   //sort by normal
#define OP_RODBM        5   //rod bitmap
#define OP_SUBCALL      6   //call a subobject
#define OP_DEFP_START   7   //defpoints with start
#define OP_GLOW         8   //glow value for next poly

#define MAX_POINTS_PER_POLY 25
//Some jank inherited from interp.c, might change it but likely not.
g3s_point* point_list[MAX_POINTS_PER_POLY];


#define w(p)  (*((short *) (p)))
#define wp(p)  ((short *) (p))
#define fp(p)  ((fix *) (p))
#define vp(p)  ((vms_vector *) (p))

//some defines
int gr_installed = 0;
int gl_initialized = 0;
int linedotscale = 1; // scalar of glLinewidth and glPointSize - only calculated once when resolution changes
int sdl_no_modeswitch = 0;

RT_Arena g_arena = { 0 };

int gr_list_modes(u_int32_t gsmodes[])
{
	SDL_Rect** modes;
	int i = 0, modesnum = 0;

	int sdl_check_flags = SDL_FULLSCREEN; // always use Fullscreen as lead.

	modes = SDL_ListModes(NULL, sdl_check_flags);

	if (modes == (SDL_Rect**)0) // check if we get any modes - if not, return 0
		return 0;


	if (modes == (SDL_Rect**)-1)
	{
		return 0; // can obviously use any resolution... strange!
	}
	else
	{
		for (i = 0; modes[i]; ++i)
		{
			if (modes[i]->w > 0xFFF0 || modes[i]->h > 0xFFF0 // resolutions saved in 32bits. so skip bigger ones (unrealistic in 2010) (changed to 0xFFF0 to fix warning)
				|| modes[i]->w < 320 || modes[i]->h < 200) // also skip everything smaller than 320x200
				continue;
			gsmodes[modesnum] = SM(modes[i]->w, modes[i]->h);
			modesnum++;
			if (modesnum >= 50) // that really seems to be enough big boy.
				break;
		}
		return modesnum;
	}
}

int gr_check_mode(u_int32_t mode)
{
	unsigned int w, h;

	w = SM_W(mode);
	h = SM_H(mode);

	if (sdl_no_modeswitch == 0) {
		return SDL_VideoModeOK(w, h, GameArg.DbgBpp, 0);
	}
	else {
		// just tell the caller that any mode is valid...
		return 32;
	}
}

int gr_set_mode(u_int32_t mode)
{
	unsigned int w, h;
	char* gr_bm_data;

	if (mode <= 0)
		return 0;

	w = SM_W(mode);
	h = SM_H(mode);

	GameCfg.ResolutionX = w;
	GameCfg.ResolutionY = h;

	if (!gr_check_mode(mode))
	{
		RT_LOGF(RT_LOGSERVERITY_HIGH, "Cannot set %ix%i. Fallback to 640x480\n", w, h);
		w = 640;
		h = 480;
		Game_screen_mode = mode = SM(w, h);
	}

	gr_bm_data = (char*)grd_curscreen->sc_canvas.cv_bitmap.bm_data;//since we use realloc, we want to keep this pointer around.
	memset(grd_curscreen, 0, sizeof(grs_screen));
	grd_curscreen->sc_mode = mode;
	grd_curscreen->sc_w = w;
	grd_curscreen->sc_h = h;
	grd_curscreen->sc_aspect = fixdiv(grd_curscreen->sc_w * GameCfg.AspectX, grd_curscreen->sc_h * GameCfg.AspectY);
	gr_init_canvas(&grd_curscreen->sc_canvas, d_realloc(gr_bm_data, w * h), BM_RTDX12, w, h);
	gr_set_current_canvas(NULL);

	if (w != last_width || h != last_height)
	{
		last_width = w;
		last_height = h;
	}

	//Set the video or resize it. The renderer handles the resize event.
	SDL_SetVideoMode(w, h, 32, sdl_video_flags);
	RT_RasterSetViewport(0.0f, 0.0f, w, h);
	gamefont_choose_game_font(w, h);

	return 0;
}

/*
 * 2d Sprites (Fireaballs, powerups, explosions). NOT hostages
 */
bool g3_draw_bitmap_full(vms_vector* pos, fix width, fix height, grs_bitmap* bm, float r, float g, float b)
{
	// NOTE(daniel): Unfortunate!
	uint16_t material_index = (uint16_t)(bm - GameBitmaps);

	RT_Vec2 rt_dim = { f2fl(width), f2fl(height) };
	RT_Vec3 rt_pos = RT_Vec3Fromvms_vector(pos);

	RT_RaytraceBillboardColored(material_index, RT_Vec3Make(r, g, b), rt_dim, rt_pos, rt_pos);

	return 0;
}

void draw_object_tmap_rod(object *obj, bitmap_index bmi, int lighted)
{
	float size = f2fl(obj->size);

	vms_vector delta;
	vm_vec_copy_scale(&delta,&obj->orient.uvec,obj->size);

	vms_vector top_v;
	vm_vec_add(&top_v,&obj->pos,&delta);

	vms_vector bot_v;
	vm_vec_sub(&bot_v,&obj->pos,&delta);

	RT_Vec3 bot_p = RT_Vec3Fromvms_vector(&bot_v);
	RT_Vec3 top_p = RT_Vec3Fromvms_vector(&top_v);

	RT_RaytraceRod(bmi.index, bot_p, top_p, f2fl(obj->size));
}

bool g3_draw_bitmap_colorwarp(vms_vector* pos, fix width, fix height, grs_bitmap* bm,
	float r, float g, float b)
{
	return g3_draw_bitmap_full(pos, width, height, bm, r, g, b);
}

bool g3_draw_bitmap(vms_vector *pos, fix width, fix height, grs_bitmap *bm)
{
	return g3_draw_bitmap_full(pos, width, height, bm, 1, 1, 1);
}

void gr_set_attributes(void)
{
	//Nothing for us to do here
}

void gr_close(void)
{
	RT_RendererExit();
}

void gr_palette_load(ubyte* pal)
{
	int i;

	for (i = 0; i < 768; i++)
	{
		gr_current_pal[i] = pal[i];
		if (gr_current_pal[i] > 63)
			gr_current_pal[i] = 63;
	}

	gr_palette_step_up(0, 0, 0); // make ogl_setbrightness_internal get run so that menus get brightened too.
	init_computed_colors();
}

void gr_palette_read(ubyte* pal)
{
	int i;
	for (i = 0; i < 768; i++)
	{
		pal[i] = gr_current_pal[i];
		if (pal[i] > 63)
			pal[i] = 63;
	}
}

int screen_flash_mul = 1;

void gr_palette_step_up(int r, int g, int b)
{
	RT_Vec4 screen_flash_color = { 0, 0, 0, 1 };

	r = r * screen_flash_mul;
	g = g * screen_flash_mul;
	b = b * screen_flash_mul;

	screen_flash_color.x = (float)max((r + gr_palette_gamma), 0);
	screen_flash_color.y = (float)max((g + gr_palette_gamma), 0);
	screen_flash_color.z = (float)max((b + gr_palette_gamma), 0);

	screen_flash_color.x = screen_flash_color.x / 63.0f;
	screen_flash_color.y = screen_flash_color.y / 63.0f;
	screen_flash_color.z = screen_flash_color.z / 63.0f;

	RT_RendererIO* io = RT_GetRendererIO();
	io->screen_overlay_color = screen_flash_color;
}

void RT_UpdateMaterialEdges(void)
{
	RT_MaterialEdge* g_rt_material_edges = RT_GetMaterialEdgesArray();

	for (int segment_index = 0; segment_index < Num_segments; segment_index++)
	{
		segment* seg = &Segments[segment_index];

		for (int side_index = 0; side_index < MAX_SIDES_PER_SEGMENT; side_index++)
		{
			side* sd = &seg->sides[side_index];

			int absolute_side_index = MAX_SIDES_PER_SEGMENT * segment_index + side_index;

			RT_MaterialEdge* side_edge = &g_rt_material_edges[absolute_side_index];
			side_edge->mat1 = sd->tmap_num;
			side_edge->mat2 = sd->tmap_num2;
		}
	}
}

void RT_UpdateMaterialIndices(void)
{
	uint16_t* g_rt_material_indices = RT_GetMaterialIndicesArray();

	for (int texture_index = 0; texture_index < MAX_TEXTURES; texture_index++)
	{
		g_rt_material_indices[texture_index] = Textures[texture_index].index;
	}

	for (int texture_index = 0; texture_index < MAX_OBJ_BITMAPS; texture_index++)
	{
		g_rt_material_indices[texture_index + MAX_TEXTURES] = ObjBitmaps[texture_index].index;
	}

	// The way this system was set up is really not the best, this is my hack to easily
	// use these special built in materials in other code.
	g_rt_material_indices[RT_MATERIAL_FLAT_WHITE] = RT_MATERIAL_FLAT_WHITE;
	g_rt_material_indices[RT_MATERIAL_EMISSIVE_WHITE] = RT_MATERIAL_EMISSIVE_WHITE;
}

void gr_flip(void)
{
	//flip
	RT_RasterRender();
	RT_RenderImGui();
	RT_SwapBuffers();
}

int gr_check_fullscreen(void)
{
	//Check if screen is fullscreen, add this option to the renderer itself.
	return (sdl_video_flags & SDL_FULLSCREEN) ? 1 : 0;
}

int gr_toggle_fullscreen(void)
{
	//Send call to renderer to make it full screen.
	if (sdl_video_flags & SDL_FULLSCREEN)
		sdl_video_flags &= ~SDL_FULLSCREEN;
	else
		sdl_video_flags |= SDL_FULLSCREEN;

	if (!SDL_SetVideoMode(SM_W(Game_screen_mode), SM_H(Game_screen_mode), GameArg.DbgBpp, sdl_video_flags))
	{
		// Setting video mode went wrong
	}

	GameCfg.WindowMode = (sdl_video_flags & SDL_FULLSCREEN) ? 0 : 1;
	return (sdl_video_flags & SDL_FULLSCREEN)?1:0;
}

void gr_upoly_tmap(int nverts, int* vert) 
{
	//never call this, maybe log this like DXX-Retro does in arch/ogl.c?
	RT_LOG(RT_LOGSERVERITY_HIGH, "gr_upoly_tmap: unhandled");
}

void draw_tmap_flat(grs_bitmap* bm, int nv, g3s_point** vertlist) 
{
	//never call this, maybe log this like DXX-Retro does in arch/ogl.c?
	RT_LOG(RT_LOGSERVERITY_HIGH, "draw_tmap_flat: unhandled");
}

//
// NOTE(daniel): Duplicated from Lights.c
//

typedef struct RT_SettingsNotification
{
	float timer;
	ImVec4 color;
	char *message;
} RT_SettingsNotification;

static inline void RT_SetSettingsNotification(RT_SettingsNotification *notif, char *name, ImVec4 color)
{
	notif->timer = 2.0f;
	notif->color = color;
	notif->message = name;
}

static inline void RT_ShowSettingsNotification(RT_SettingsNotification *notif)
{
	if (notif->timer > 0.0f)
	{
		ImVec4 color = notif->color;
		color.w = notif->timer / 2.0f;

		igPushStyleColor_Vec4(ImGuiCol_Text, color);
		igText(notif->message);
		igPopStyleColor(1);

		notif->timer -= 1.0f / 60.0f; // hardcoded nonsense
	}
	else
	{
		igDummy((ImVec2){0.0f, igGetFontSize()});
	}
}

//

static RT_SettingsNotification g_dynamic_lights_notification;

static void RT_LoadDynamicLightSettings(void);
static void RT_SaveDynamicLightSettings(void);
static void RT_ResetDynamicLightSettings(void);

int gr_init(int mode)
{
	SDL_WM_SetCaption(DESCENT_VERSION, "Descent");
	//Note SAM: Maybe we can create our own bmp later on, would be cool.
	SDL_WM_SetIcon(SDL_LoadBMP("dxx-raytracer.bmp"), NULL);

	if (!GameCfg.WindowMode && !GameArg.SysWindow)
		sdl_video_flags |= SDL_FULLSCREEN;

	if (GameArg.SysNoBorders)
		sdl_video_flags |= SDL_NOFRAME;

	g_rt_free_cam_info.g_free_cam_enabled = false;
	g_rt_free_cam_info.g_free_cam_clipping_enabled = false;
	g_rt_free_cam_info.g_free_cam_obj = 0;
	g_rt_free_cam_info.g_old_cockpit = 0;

	MALLOC(grd_curscreen, grs_screen, 1);
	memset(grd_curscreen, 0, sizeof(grs_screen));
	grd_curscreen->sc_canvas.cv_bitmap.bm_data = NULL;

	grd_curscreen->sc_canvas.cv_color = 0;
	grd_curscreen->sc_canvas.cv_fade_level = GR_FADE_OFF;
	grd_curscreen->sc_canvas.cv_blend_func = GR_BLEND_NORMAL;
	grd_curscreen->sc_canvas.cv_drawmode = 0;
	grd_curscreen->sc_canvas.cv_font = NULL;
	grd_curscreen->sc_canvas.cv_font_fg_color = 0;
	grd_curscreen->sc_canvas.cv_font_bg_color = 0;
	gr_set_current_canvas(&grd_curscreen->sc_canvas);

	unsigned int w, h;
	w = SM_W(mode);
	h = SM_H(mode);

	//Init video here, sadly it's wrong but it will be resized in gr_set_mode.
	SDL_Surface* surf = SDL_SetVideoMode(w, h, 32, SDL_DOUBLEBUF | SDL_HWSURFACE | SDL_ANYFORMAT);
	
	SDL_EventState(SDL_IGNORE, NULL);
	RT_RendererInitParams initParams;
	SDL_SysWMinfo info;
	
	SDL_VERSION(&info.version);
	SDL_GetWMInfo(&info);

	initParams.arena = &g_arena;
	initParams.window_handle = info.window;

	igCreateContext(NULL);

	ImGuiIO *io = igGetIO();
	io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io->Fonts->Flags |= ImFontAtlasFlags_NoBakedLines;

	igInitWin32(info.window);
	RT_RendererInit(&initParams);

	float fov;
	RT_ConfigReadFloat(RT_GetRendererIO()->config, RT_StringLiteral("fov"), &fov);
	g_cam.vfov = g_free_cam.vfov = fov;

	// Awkward place to do this, but I need to do it somewhere
	memcpy(&g_rt_default_dynamic_light_info, &g_rt_dynamic_light_info, sizeof(g_rt_dynamic_light_info));
	memcpy(g_rt_default_light_adjusts, rt_light_adjusts, sizeof(RT_WeaponLightAdjusts)*RT_LIGHT_ADJUST_ARRAY_SIZE);

	RT_LoadDynamicLightSettings();
	g_dynamic_lights_notification.timer = 0.0f;

	return 0;
}

void RT_VertexFixToFloat_Fan(RT_TriangleBuffer *buf, int nv, g3s_point** pointlist, uint32_t texture_id, uint32_t triangle_color)
{
	RT_Triangle first_triangle = {0};

	first_triangle.pos0.x = f2fl(pointlist[0]->p3_vec.x);
	first_triangle.pos0.y = f2fl(pointlist[0]->p3_vec.y);
	first_triangle.pos0.z = f2fl(pointlist[0]->p3_vec.z);

	first_triangle.uv0.x = f2fl(pointlist[0]->p3_u);
	first_triangle.uv0.y = f2fl(pointlist[0]->p3_v);

	first_triangle.color = triangle_color;

	RT_ASSERT(texture_id & RT_TRIANGLE_HOLDS_MATERIAL_EDGE);

    first_triangle.material_edge_index = texture_id;

	int start_count = buf->count;

	for (int point_index = 1; point_index + 1 < nv; point_index++)
	{
		RT_Triangle triangle = first_triangle;

		triangle.pos1.x = f2fl(pointlist[point_index]->p3_vec.x);
		triangle.pos1.y = f2fl(pointlist[point_index]->p3_vec.y);
		triangle.pos1.z = f2fl(pointlist[point_index]->p3_vec.z);

		triangle.uv1.x = f2fl(pointlist[point_index]->p3_u);
		triangle.uv1.y = f2fl(pointlist[point_index]->p3_v);

		triangle.pos2.x = f2fl(pointlist[point_index + 1]->p3_vec.x);
		triangle.pos2.y = f2fl(pointlist[point_index + 1]->p3_vec.y);
		triangle.pos2.z = f2fl(pointlist[point_index + 1]->p3_vec.z);

		triangle.uv2.x = f2fl(pointlist[point_index + 1]->p3_u);
		triangle.uv2.y = f2fl(pointlist[point_index + 1]->p3_v);

        triangle.material_edge_index = texture_id;
		
		//Now do the normals
		RT_Vec3 p10 = RT_Vec3Normalize(RT_Vec3Sub(triangle.pos1, triangle.pos0));
		RT_Vec3 p20 = RT_Vec3Normalize(RT_Vec3Sub(triangle.pos2, triangle.pos0));

		RT_Vec3 normal = RT_Vec3Normalize(RT_Vec3Cross(p10, p20));

		triangle.normal0 = normal;
		triangle.normal1 = normal;
		triangle.normal2 = normal;

		RT_PushTriangle(buf, triangle);
	}

	int end_count = buf->count;

	// NOTE(daniel): This is a separate call, because I don't want to do something tweaky like
	// detecting whether tangents need to be calculated in RT_UploadMesh. You, the uploader, should know.
	RT_GenerateTangents(&buf->triangles[start_count], end_count - start_count);
}

void RT_SetPointList(g3s_point* dest, vms_vector* src, int n)
{
	// NOTE(daniel): Ok so for submodels I guess we're relying on behaviour related
	// to the view matrix and position being set. Very janky intermingling of code
	// meant for rasterization, but I guess that's that. So we need to rotate the
	// points.

	while (n--)
		g3_rotate_point(dest++, src++);
}

vms_angvec zero_angles;

g3s_point* point_list[MAX_POINTS_PER_POLY];

// NOTE(daniel): Here for hardcoding certain materials for flat polys
// for different meshes. It's not the best.
static int jank_currently_loading_poly_model;

static void RT_GetPolyData(RT_TriangleBuffer *buf,
						   g3s_point *interp_point_list,
						   void *model_ptr,
						   vms_angvec *anim_angles,
						   int first_texture)
{
	ubyte* p = model_ptr;

	while (w(p) != OP_EOF)
	{
		switch (w(p)) 
		{
			case OP_DEFPOINTS: 
			{
				int n = w(p + 2);

				RT_SetPointList(interp_point_list, vp(p + 4), n);
				p += n*sizeof(struct vms_vector) + 4;
			} break;

			case OP_DEFP_START: 
			{
				int n = w(p + 2);
				int s = w(p + 4);

				RT_SetPointList(&interp_point_list[s], vp(p + 8), n);
				p += n*sizeof(struct vms_vector) + 8;
			} break;

			case OP_FLATPOLY: 
			{
				int nv = w(p + 2);

				for (int i = 0; i < nv; i++)
				{
					point_list[i] = interp_point_list + wp(p + 30)[i];
				}

				short color = w(p + 28);

				// NOTE(daniel): I hope the palette is initialized correctly!
				ubyte r = gr_palette[color*3 + 0]*4;
				ubyte g = gr_palette[color*3 + 1]*4;
				ubyte b = gr_palette[color*3 + 2]*4;
				ubyte a = 255;

				// RGBA!
				uint32_t color_packed = (r << 0)|(g << 8)|(b << 16)|(a << 24);

				uint32_t material_index = RT_MATERIAL_FLAT_WHITE|RT_TRIANGLE_HOLDS_MATERIAL_EDGE;

				// wow. good code.

				bool should_be_emissive = false;

				if (PCSharePig)
				{
					// pointy melee guy
					if (jank_currently_loading_poly_model == 2) should_be_emissive = true;
					if (jank_currently_loading_poly_model == 3) should_be_emissive = true;

					if (jank_currently_loading_poly_model == 8) should_be_emissive = true;

					if ((jank_currently_loading_poly_model >= 28 &&
						 jank_currently_loading_poly_model <= 43) ||
						(jank_currently_loading_poly_model >= 46 &&
						 jank_currently_loading_poly_model <= 47) ||
						(jank_currently_loading_poly_model >= 52 &&
						 jank_currently_loading_poly_model <= 55))

					{
						should_be_emissive = true;
					}

				}
				else
				{
					// pointy melee guy
					if (jank_currently_loading_poly_model == 2) should_be_emissive = true;
					// pointy blue-purple guy
					if (jank_currently_loading_poly_model == 7) should_be_emissive = true;

					if ((jank_currently_loading_poly_model >= 46 &&
						 jank_currently_loading_poly_model <= 61) ||
						(jank_currently_loading_poly_model >= 63 &&
						 jank_currently_loading_poly_model <= 67) ||
						(jank_currently_loading_poly_model >= 73 &&
						 jank_currently_loading_poly_model <= 76))
					{
						// Laser.
						should_be_emissive = true;
					}
				}

				if (should_be_emissive)
				{
					material_index = RT_MATERIAL_EMISSIVE_WHITE|RT_TRIANGLE_HOLDS_MATERIAL_EDGE;
				}

				RT_VertexFixToFloat_Fan(buf, nv, point_list, material_index, color_packed);

				p += 30 + ((nv & ~1) + 1) * 2;
			} break;

			case OP_TMAPPOLY: 
			{
				int nv = w(p + 2);

				Assert(nv < MAX_POINTS_PER_POLY);

				// Lights were done here previously, no needed now.
				// NOTE(daniel): Again, that's nice, but do we know that we don't care about those light values?
				// Are they used anywhere, or do we just ignore them entirely? (We might, and that might be
				// totally valid).

				// Get the UV coordinates (allegedly) // NOTE(daniel): Why allegedly?
				g3s_uvl* uvl_list = (g3s_uvl*)(p + 30 + ((nv & ~1) + 1) * 2);

				for (int i = 0; i < nv; i++) 
				{
					point_list[i] = interp_point_list + wp(p + 30)[i];
					point_list[i]->p3_u = uvl_list[i].u;
					point_list[i]->p3_v = uvl_list[i].v;
				}

				// Get texture index - p+28 would be the magic offset for the texture id relative to first_texture
				short texture_index = w(p + 28) + first_texture;

				// NOTE(daniel): For poly objects, RT_TRIANGLE_HOLDS_MATERIAL_EDGE is or'd into the material index
				// assigned to the triangle to indicate to the renderer to skip the double indirection
				// through the material edge array, because it's only needed for segments.
				uint32_t material_index = (ObjBitmapPtrs[texture_index] + MAX_TEXTURES) | RT_TRIANGLE_HOLDS_MATERIAL_EDGE;
				RT_VertexFixToFloat_Fan(buf, nv, point_list, material_index, 0xFFFFFFFF);

				p += 30 + ((nv & ~1) + 1) * 2 + nv * 12;
			} break;

			case OP_SORTNORM:
			{
				if (g3_check_normal_facing(vp(p + 16), vp(p + 4)) > 0) // facing
				{		
					// draw back then front
					// NOTE(daniel): This is about sorting polys according to the view matrix, what does this
					// have to do with loading models ahead of time?
					RT_GetPolyData(buf, interp_point_list, p + w(p + 30), anim_angles, first_texture);
					RT_GetPolyData(buf, interp_point_list, p + w(p + 28), anim_angles, first_texture);

				}
				else												   // not facing.  draw front then back
				{			
					RT_GetPolyData(buf, interp_point_list, p + w(p + 28), anim_angles, first_texture);
					RT_GetPolyData(buf, interp_point_list, p + w(p + 30), anim_angles, first_texture);
				}

				p += 32;
			} break;

			case OP_RODBM: 
			{
				//Not needed here
				p += 36;
			} break;

			case OP_SUBCALL: 
			{
				typedef struct RT_SubcallStruct 
				{
					uint16_t opcode; // = OP_SUBCALL
					uint16_t anim_angles_index;
					vms_vector position_offset;
					uint16_t model_ptr;
					uint16_t unknown7; // always 0?
				} RT_SubcallStruct;

				RT_SubcallStruct *command_data = (RT_SubcallStruct *)p;

				// Not needed here
				// NOTE(daniel): If it's not needed here, why is it still here?
				vms_angvec* a;

				if (anim_angles)
				{
					a = &anim_angles[command_data->anim_angles_index];
				}
				else
				{
					a = &zero_angles;
				}

				g3_start_instance_angles(&command_data->position_offset, a);

				RT_GetPolyData(buf, interp_point_list, p + command_data->model_ptr, anim_angles, first_texture);

				g3_done_instance();
				p += 20;
			} break;

			case OP_GLOW:
			{
				// We aint doing glow here
				// NOTE(daniel): Ok, but then where are the glow values going? When are they used?
				// Should we care?
				p += 4;
			} break;

			default:
			{
				/* ... */
			} break;
		}
	}
}

void RT_InitglTFModels(void)
{
	g_rt_cockpit_settings.cockpit_hud_texture = RT_UploadTexture(&(RT_UploadTextureParams) {
		.image.width  = 1024,
		.image.height = 1024,
		.image.format = RT_TextureFormat_RGBA8_SRGB,
		.image.pixels = RT_ArenaAllocArray(&g_thread_arena, 1024 * 1024, uint32_t),
		.name   = "Cockpit UI",
	});

	RT_Material cockpit_material = { 0 };
	cockpit_material.albedo_texture = RT_GetDefaultBlackTexture();
	cockpit_material.emissive_texture = g_rt_cockpit_settings.cockpit_hud_texture;
	cockpit_material.emissive_strength = 1.0f;
	cockpit_material.emissive_color.x = 1.0f;
	cockpit_material.emissive_color.y = 1.0f;
	cockpit_material.emissive_color.z = 1.0f;
	cockpit_material.flags = RT_MaterialFlag_NoCastingShadow;
	RT_UpdateMaterial(RT_MATERIAL_COCKPIT_UI, &cockpit_material);

	RT_MaterialOverride material_override = {
		RT_MATERIAL_COCKPIT_UI,
		"Screens_and_keycard_buttons"
    };

	g_rt_cockpit_settings.cockpit_gltf = RT_LoadGLTF(&g_arena, "assets/cockpit_prototype.gltf", &material_override);

	RT_Config cfg;
	RT_InitializeConfig(&cfg, &g_thread_arena);
	if (RT_DeserializeConfigFromFile(&cfg, "cockpit_3d.cfg")) {
		RT_ConfigReadVec3(&cfg, RT_StringLiteral("front_cockpit_rotation"), &g_rt_cockpit_settings.front_cockpit_rotation);
		RT_ConfigReadVec3(&cfg, RT_StringLiteral("front_cockpit_offset"), &g_rt_cockpit_settings.front_cockpit_offset);
		RT_ConfigReadVec3(&cfg, RT_StringLiteral("front_cockpit_scale"), &g_rt_cockpit_settings.front_cockpit_scale);
		RT_ConfigReadVec3(&cfg, RT_StringLiteral("back_cockpit_rotation"), &g_rt_cockpit_settings.back_cockpit_rotation);
		RT_ConfigReadVec3(&cfg, RT_StringLiteral("back_cockpit_offset"), &g_rt_cockpit_settings.back_cockpit_offset);
		RT_ConfigReadVec3(&cfg, RT_StringLiteral("back_cockpit_scale"), &g_rt_cockpit_settings.back_cockpit_scale);
	}
}

void RT_InitBasePolyModel(const int polymodel_index, g3s_point* interp_point_list, void* model_ptr, vms_angvec* anim_angles, int first_texture)
{
	RT_ArenaMemoryScope(&g_thread_arena)
	{
		int triangle_buffer_size = 2048;

		RT_TriangleBuffer triangles =
		{
			.triangles = RT_ArenaAllocArray(&g_thread_arena, triangle_buffer_size, RT_Triangle),
			.count     = 0,
			.capacity  = triangle_buffer_size,
		};

		View_position = vmd_zero_vector;
		View_matrix   = vmd_identity_matrix;

		RT_GetPolyData(&triangles, interp_point_list, model_ptr, anim_angles, first_texture);

		if (triangles.count > 0) 
		{
			mesh_handles[polymodel_index] = RT_UploadMesh(&(RT_UploadMeshParams){
				.triangles      = triangles.triangles,
				.triangle_count = triangles.count,
				.name           = RT_ArenaPrintF(&g_thread_arena, "Polymodel %d", polymodel_index),
			});

			// NOTE(daniel): If we're going to keep the triangles around for some hacking, we should 
			// make a copy to a permanent arena.
			RT_UploadMeshParams hacky_params = 
			{
				.triangles      = RT_ArenaCopyArray(&g_arena, triangles.triangles, triangles.count),
				.triangle_count = triangles.count,
			};
			meshVerticesRawHack[polymodel_index] = hacky_params;
		}
	}
}

RT_ResourceHandle RT_InitSubPolyModel(g3s_point* interp_point_list, void* model_ptr, vms_angvec* anim_angles, int first_texture)
{
	RT_ResourceHandle result = RT_RESOURCE_HANDLE_NULL;

	RT_ArenaMemoryScope(&g_thread_arena)
	{
		int triangle_buffer_size = 2048;

		RT_TriangleBuffer triangles =
		{
			.triangles = RT_ArenaAllocArray(&g_thread_arena, triangle_buffer_size, RT_Triangle),
			.count     = 0,
			.capacity  = triangle_buffer_size,
		};

		View_position = vmd_zero_vector;
		View_matrix   = vmd_identity_matrix;

		RT_GetPolyData(&triangles, interp_point_list, model_ptr, anim_angles, first_texture);

		if (triangles.count > 0) 
		{
			result = RT_UploadMesh(&(RT_UploadMeshParams){
				.triangles      = triangles.triangles,
				.triangle_count = triangles.count,
				.name           = "Sub-Polymodel",
			});
		}
	}

	return result;
}

void RT_InitPolyModelAndSubModels(int pm_index)
{
	polymodel *pm = &Polygon_models[pm_index];

	// TODO(daniel): Figure out something better
	jank_currently_loading_poly_model = pm_index;

	RT_InitBasePolyModel(pm_index, robot_points, pm->model_data, NULL, pm->first_texture);
	for (size_t i = 0; i < pm->n_models; i++)
	{
		int t_ModelPtr = pm->submodel_ptrs[i];
		pm->submodel[i] = RT_InitSubPolyModel(robot_points, pm->model_data + t_ModelPtr, NULL, pm->first_texture);
	}

	// TODO(daniel): Figure out something better
	jank_currently_loading_poly_model = 0;

    // Find tree structure of submodels
    memset(pm->model_tree, 0, sizeof(pm->model_tree));

    for (int i = 0; i < pm->n_models; ++i) 
	{
        // Parent
        pm->model_tree[i].parent_index = pm->submodel_parents[i];

        // Add this model's index to the list of child indices of the parent
        if (pm->submodel_parents[i] != 255) 
		{
            RT_ModelTree *parent = &pm->model_tree[pm->submodel_parents[i]];
            parent->child_indices[parent->n_children] = i;
            parent->n_children++;
        }
    }
}

void RT_InitAllPolyModels(void)
{
	for (int pm_index = 0; pm_index < N_polygon_models; pm_index++)
	{
		RT_InitPolyModelAndSubModels(pm_index);
	}
}

void RT_DrawPolyModel(const int meshnumber, const int signature, ubyte object_type, const vms_vector* pos, const vms_matrix* orient)
{
	// NOTE(daniel): This is never used, is it.

	if (!RT_RESOURCE_HANDLE_VALID(mesh_handles[meshnumber]))
	{
		RT_InitPolyModelAndSubModels(meshnumber);
	}

	if (RT_RESOURCE_HANDLE_VALID(mesh_handles[meshnumber]))
	{
        // Create model matrix
		RT_Mat4 mat = RT_Mat4Identity();

        // Apply translation
		mat = RT_Mat4Mul(mat, RT_Mat4FromTranslation(RT_Vec3Fromvms_vector(pos)));

        // Apply rotation
        RT_Mat4 rot = RT_Mat4Fromvms_matrix(orient);
        mat = RT_Mat4Mul(mat, RT_Mat4Fromvms_matrix(orient));

		// assert(objNum > 0 || objNum < MAX_OBJECTS);

		RT_ResourceHandle handle = mesh_handles[meshnumber];

        // Render mesh
		// RT_RaytraceMesh(handle, &mat, &old_poly_matrix[objNum]);

		RT_RenderMeshParams params = 
		{
			.key         = signature,
			.mesh_handle = handle,
			.transform   = &mat,
			.color       = 0xFFFFFFFF,
		};
		RT_RaytraceMeshEx(&params);
	}
}

void RT_DrawSubPolyModel(RT_ResourceHandle submodel, const RT_Mat4* const submodel_transform, RT_RenderKey key)
{
	if (RT_RESOURCE_HANDLE_VALID(submodel))
	{
		float component = 1.0f;
		float alpha = 1.0f;
		if (grd_curcanv->cv_fade_level < GR_FADE_OFF)
		{
			component = 0.0f;	// cloaked ships should have black materials
			alpha = 1.0f - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0f);
		}

		RT_Vec4 color = { component, component, component, alpha };

		RT_RenderMeshParams params =
		{
			.key = key,
			.mesh_handle = submodel,
			.transform = submodel_transform,
			.prev_transform = submodel_transform,
			.color = RT_PackRGBA(color),
		};
		RT_RaytraceMeshEx(&params);
	}
}

void RT_DrawPolySubModelTree(const polymodel* model, const vms_angvec* const anim_angles, int submodel_index, const int signature, const RT_Mat4 submodel_transform)
{
	RT_RenderKey key =
	{
		.signature      = signature,
		.submodel_index = submodel_index,
	};

	// Draw the submodel
	RT_DrawSubPolyModel(model->submodel[submodel_index], &submodel_transform, key);

	// Traverse tree structure
	for (int i = 0; i < model->model_tree[submodel_index].n_children; ++i) {
		// anim_angles is an array, where the indices into that array allegedly correspond directly to the child indices :D
		const int child_index = model->model_tree[submodel_index].child_indices[i];

		vms_angvec anim_angles_final;
		if (anim_angles) {
			anim_angles_final.p = anim_angles[child_index].p;
			anim_angles_final.b = anim_angles[child_index].b;
			anim_angles_final.h = anim_angles[child_index].h;
		}
		else {
			anim_angles_final.p = zero_angles.p;
			anim_angles_final.b = zero_angles.b;
			anim_angles_final.h = zero_angles.h;
		}

		// Get matrix from local position offset
		const vms_vector offset_vms = model->submodel_offsets[child_index];
		const RT_Vec3 offset_vec3 = RT_Vec3Fromvms_vector(&offset_vms);
		RT_Mat4 offset_mat4 = RT_Mat4FromTranslation(offset_vec3);
		offset_mat4 = RT_Mat4Mul(submodel_transform, offset_mat4);

		// Get matrix from rotation offset
		vms_matrix rotation_vms;
		vm_angles_2_matrix(&rotation_vms, &anim_angles_final);
		RT_Mat4 rotation_mat4 = RT_Mat4Fromvms_matrix(&rotation_vms);

		// Combine them into one big matrix
		RT_Mat4 combined_matrix = RT_Mat4Mul(offset_mat4, rotation_mat4);

		RT_DrawPolySubModelTree(model, anim_angles, child_index, signature, combined_matrix);
	}
}

void RT_DrawPolyModelTree(const int meshnumber, const int signature, ubyte object_type, const vms_vector* pos, const vms_matrix* orient, vms_angvec* anim_angles) {
	if (!RT_RESOURCE_HANDLE_VALID(mesh_handles[meshnumber]))
	{
		RT_InitPolyModelAndSubModels(meshnumber);
	}

    // Get model to render
    polymodel* model = &Polygon_models[meshnumber];

    // Get matrix from local position offset
    const vms_vector offset_vms = *pos;
    const RT_Vec3 offset_vec3 = RT_Vec3Fromvms_vector(&offset_vms);
    RT_Mat4 offset_mat4 = RT_Mat4FromTranslation(offset_vec3);

    // Get matrix from rotation offset
    RT_Mat4 rotation_mat4 = RT_Mat4Fromvms_matrix(orient);

    // Combine them into one big matrix
    RT_Mat4 combined_matrix = RT_Mat4Mul(offset_mat4, rotation_mat4);

    RT_DrawPolySubModelTree(model, anim_angles, 0, signature, combined_matrix);
}

void RT_DrawGLTF(const RT_GLTFNode* node, RT_Mat4 transform, RT_Mat4 prev_transform)
{
	if (!node)
		return;

	transform = RT_Mat4Mul(transform, node->transform);
	prev_transform = RT_Mat4Mul(prev_transform, node->transform);

	if (node->model)
	{
		RT_GLTFModel* model = node->model;
		RT_RaytraceMesh(model->handle, &transform, &prev_transform);
	}

	for (size_t i = 0; i < node->children_count; i++)
	{
		RT_GLTFNode* child = node->children[i];
		RT_DrawGLTF(child, transform, prev_transform);
	}
}

void RT_EnableFreeCam()
{
	g_rt_free_cam_info.g_free_cam_enabled = true;
	g_rt_free_cam_info.g_old_cockpit = PlayerCfg.CockpitMode[1];

	g_rt_free_cam_info.g_free_cam_obj = obj_create(OBJ_CAMERA, 0,
		ConsoleObject->segnum, &ConsoleObject->pos, &ConsoleObject->orient, 0,
		CT_FLYING, MT_PHYSICS, RT_NONE);

	Objects[g_rt_free_cam_info.g_free_cam_obj].mtype.phys_info.drag = 2162;
	Objects[g_rt_free_cam_info.g_free_cam_obj].mtype.phys_info.mass = 262144;
	Objects[g_rt_free_cam_info.g_free_cam_obj].mtype.phys_info.flags = PF_USES_THRUST;
	Objects[g_rt_free_cam_info.g_free_cam_obj].size = 310325.0F;

	CollisionResult[OBJ_CAMERA][OBJ_WALL] = RESULT_CHECK; CollisionResult[OBJ_CAMERA][OBJ_WALL] = RESULT_CHECK;

	Viewer = &Objects[g_rt_free_cam_info.g_free_cam_obj];

	PlayerCfg.HudMode = 3; // NO HUD mode
	select_cockpit(CM_FULL_SCREEN);
	PlayerCfg.CockpitMode[0] = CM_FULL_SCREEN;
}

void RT_DisableFreeCam()
{
	g_rt_free_cam_info.g_free_cam_enabled = false;

	Viewer = ConsoleObject;

	CollisionResult[OBJ_CAMERA][OBJ_WALL] = RESULT_NOTHING; CollisionResult[OBJ_CAMERA][OBJ_WALL] = RESULT_NOTHING;

	PlayerCfg.HudMode = 0; // Standard HUD mode
	select_cockpit(g_rt_free_cam_info.g_old_cockpit);
	PlayerCfg.CockpitMode[0] = g_rt_free_cam_info.g_old_cockpit;

	obj_delete(g_rt_free_cam_info.g_free_cam_obj);
}

void RT_ResetLightEmission()
{
	int lightTexture = PCSharePig ? 774 : 997;
	RT_Material* material = &g_rt_materials[lightTexture];
	material->emissive_strength = 3.5f;
	RT_UpdateMaterial(lightTexture, material);
}

static void RT_DoDynamicLightEditorMenus(void)
{
	if (igBegin("Dynamic Light Editor", NULL, 0))
	{
		igPushID_Str("Dynamic Lights");

		if (igButton("Load Settings", (ImVec2){0, 0}))
		{
			RT_LoadDynamicLightSettings();
		}

		igSameLine(0.0f, -1.0f);

		if (igButton("Save Settings", (ImVec2){0, 0}))
		{
			RT_SaveDynamicLightSettings();
		}

		igSameLine(0.0f, -1.0f);

		if (igButton("Reset Settings", (ImVec2){0, 0}))
		{
			RT_ResetDynamicLightSettings();
		}

		RT_ShowSettingsNotification(&g_dynamic_lights_notification);

		igSeparator();
		igText("Weapon Lights");
		igSeparator();

		igPushID_Int(2);
		igCheckbox("Enable Weapon and flare lights", &g_rt_dynamic_light_info.weaponFlareLights);
		igSliderFloat("Weapon Brightness modifier", &g_rt_dynamic_light_info.weaponBrightMod, 0, 1000.f, "%.3f", 0);
		igSliderFloat("Radius  modifier", &g_rt_dynamic_light_info.weaponRadiusMod, 0, 4.f, "%.3f", 0);
		for (size_t i = 0; i < RT_LIGHT_ADJUST_ARRAY_SIZE; i++)
		{
			RT_WeaponLightAdjusts* adj = &rt_light_adjusts[i];
			if (igTreeNode_Str(adj->weapon_name))
			{
				igSliderFloat("Brightness", &adj->brightMul, 0, 100.f, "%.3f", 0);
				igSliderFloat("Radius", &adj->radiusMul, 0, 10.f, "%.3f", 0);
				igTreePop();
			}
		}
		igPopID();

		igDummy((ImVec2){0.0f, igGetFontSize()});

		igSeparator();
		igText("Explosion Lights");
		igSeparator();

		igPushID_Int(3);
		igCheckbox("Enable explosion lights", &g_rt_dynamic_light_info.explosionLights);
		igSliderFloat("Brightness modifier", &g_rt_dynamic_light_info.explosionBrightMod, 0, 1000.f, "%.3f", 0);
		igSliderFloat("Radius modifier", &g_rt_dynamic_light_info.explosionRadiusMod, 0, 4.f, "%.3f", 0);
		igSliderFloat("Type bias modifier", &g_rt_dynamic_light_info.explosionTypeBias, 0.10f, 10.f, "%.3f", 0);
		igPopID();

		igDummy((ImVec2){0.0f, igGetFontSize()});

		igSeparator();
		igText("Muzzle Flare Lights");
		igSeparator();

		igPushID_Int(4);
		igCheckbox("Enable muzzle flare lights", &g_rt_dynamic_light_info.muzzleLights);
		igSliderFloat("Brightness modifier", &g_rt_dynamic_light_info.muzzleBrightMod, 0, 1000.f, "%.3f", 0);
		igSliderFloat("Radius modifier", &g_rt_dynamic_light_info.muzzleRadiusMod, 0, 4.f, "%.3f", 0);
		igPopID();

		igPopID();
	} igEnd();
}

// Bad thing ahead:
static RT_String RT_FormatString(RT_Arena *arena, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	const char *formatted = RT_ArenaPrintF(arena, fmt, args);

	va_end(args);

	RT_String result = 
	{
		.bytes = formatted,
		.count = strlen(formatted), // <--- specifically, this is the bad thing.
	};

	return result;
}

void RT_LoadDynamicLightSettings(void)
{
	bool success = true;

	RT_DynamicLightInfo *info = &g_rt_dynamic_light_info;

	RT_ArenaMemoryScope(&g_thread_arena)
	{
		RT_Config *cfg = RT_ArenaAllocStruct(&g_thread_arena, RT_Config);
		RT_InitializeConfig(cfg, &g_thread_arena);

		if (RT_DeserializeConfigFromFile(cfg, "lights/dynamic_lights.vars"))
		{
			RT_ConfigReadInt(cfg, RT_StringLiteral("weapon_flare_lights"), &info->weaponFlareLights);
			RT_ConfigReadFloat(cfg, RT_StringLiteral("weapon_brightness"), &info->weaponBrightMod);
			RT_ConfigReadFloat(cfg, RT_StringLiteral("weapon_radius"), &info->weaponRadiusMod);

			for (size_t i = 0; i < RT_LIGHT_ADJUST_ARRAY_SIZE; i++)
			{
				RT_WeaponLightAdjusts *adj = &rt_light_adjusts[i];

				RT_ConfigReadFloat(cfg, RT_FormatString(&g_thread_arena, "%s_brightness", adj->weapon_name), &adj->brightMul);
				RT_ConfigReadFloat(cfg, RT_FormatString(&g_thread_arena, "%s_radius", adj->weapon_name), &adj->radiusMul);
			}

			RT_ConfigReadInt(cfg, RT_StringLiteral("explosion_lights"), &info->explosionLights);
			RT_ConfigReadFloat(cfg, RT_StringLiteral("explosion_brightness"), &info->explosionBrightMod);
			RT_ConfigReadFloat(cfg, RT_StringLiteral("explosion_radius"), &info->explosionRadiusMod);
			RT_ConfigReadFloat(cfg, RT_StringLiteral("explosion_type_bias"), &info->explosionTypeBias);

			RT_ConfigReadInt(cfg, RT_StringLiteral("muzzle_lights"), &info->muzzleLights);
			RT_ConfigReadFloat(cfg, RT_StringLiteral("muzzle_brightness"), &info->muzzleBrightMod);
			RT_ConfigReadFloat(cfg, RT_StringLiteral("muzzle_radius"), &info->muzzleRadiusMod);
		}
		else
		{
			success = false;
		}
	}

	if (success)
	{
		RT_SetSettingsNotification(&g_dynamic_lights_notification, "Successfully loaded dynamic light settings", (ImVec4){0.5f, 1.0f, 0.7f, 1.0f});
	}
	else 
	{
		RT_SetSettingsNotification(&g_dynamic_lights_notification, "Nothing to load!", (ImVec4){1.0f, 0.2f, 0.2f, 1.0f});
	}
}

void RT_SaveDynamicLightSettings(void)
{
	PHYSFS_mkdir("lights");

	bool success = true;

	RT_DynamicLightInfo *info = &g_rt_dynamic_light_info;

	RT_ArenaMemoryScope(&g_thread_arena)
	{
		RT_Config *cfg = RT_ArenaAllocStruct(&g_thread_arena, RT_Config);
		RT_InitializeConfig(cfg, &g_thread_arena);

		RT_ConfigWriteInt(cfg, RT_StringLiteral("weapon_flare_lights"), info->weaponFlareLights);
		RT_ConfigWriteFloat(cfg, RT_StringLiteral("weapon_brightness"), info->weaponBrightMod);
		RT_ConfigWriteFloat(cfg, RT_StringLiteral("weapon_radius"), info->weaponRadiusMod);

		for (size_t i = 0; i < RT_LIGHT_ADJUST_ARRAY_SIZE; i++)
		{
			RT_WeaponLightAdjusts *adj = &rt_light_adjusts[i];

			RT_ConfigWriteFloat(cfg, RT_FormatString(&g_thread_arena, "%s_brightness", adj->weapon_name), adj->brightMul);
			RT_ConfigWriteFloat(cfg, RT_FormatString(&g_thread_arena, "%s_radius", adj->weapon_name), adj->radiusMul);
		}

		RT_ConfigWriteInt(cfg, RT_StringLiteral("explosion_lights"), info->explosionLights);
		RT_ConfigWriteFloat(cfg, RT_StringLiteral("explosion_brightness"), info->explosionBrightMod);
		RT_ConfigWriteFloat(cfg, RT_StringLiteral("explosion_radius"), info->explosionRadiusMod);
		RT_ConfigWriteFloat(cfg, RT_StringLiteral("explosion_type_bias"), info->explosionTypeBias);

		RT_ConfigWriteInt(cfg, RT_StringLiteral("muzzle_lights"), info->muzzleLights);
		RT_ConfigWriteFloat(cfg, RT_StringLiteral("muzzle_brightness"), info->muzzleBrightMod);
		RT_ConfigWriteFloat(cfg, RT_StringLiteral("muzzle_radius"), info->muzzleRadiusMod);

		if (!RT_SerializeConfigToFile(cfg, "lights/dynamic_lights.vars"))
		{
			success = false;
		}
	}

	if (success)
	{
		RT_SetSettingsNotification(&g_dynamic_lights_notification, "Successfully saved dynamic light settings", (ImVec4){0.7f, 1.0f, 0.5f, 1.0f});
	}
	else 
	{
		RT_SetSettingsNotification(&g_dynamic_lights_notification, "Encountered problems saving dynamic light settings!", (ImVec4){1.0f, 0.2f, 0.2f, 1.0f});
	}
}

void RT_ResetDynamicLightSettings(void)
{
	memcpy(&g_rt_dynamic_light_info, &g_rt_default_dynamic_light_info, sizeof(g_rt_dynamic_light_info));
	memcpy(rt_light_adjusts, g_rt_default_light_adjusts, sizeof(RT_WeaponLightAdjusts)*RT_LIGHT_ADJUST_ARRAY_SIZE);
	RT_SetSettingsNotification(&g_dynamic_lights_notification, "Reset dynamic light settings", (ImVec4){0.5f, 0.7f, 1.0f, 1.0f});
}

void RT_StartImGuiFrame(void)
{
	igStartFrameWin32();
	igNewFrame();

	// NOTE(Justin): It is times like these you realize that you are under a lot of time pressure
	// and write code like this. There is definitely a quite simple mathematical way to solve this, but whatever.
	bool value_changed = false;
	float fov;
	RT_ConfigReadFloat(RT_GetRendererIO()->config, RT_StringLiteral("fov"), &fov);
	float fov_delta = fov - g_cam.vfov;
	if (fov_delta != 0.0)
	{
		float multiplier = 0.8 / 30.0;
		g_cam.vfov = g_free_cam.vfov = fov;
		g_rt_cockpit_settings.front_cockpit_offset.z += fov_delta * multiplier;
		g_rt_cockpit_settings.back_cockpit_offset.z -= fov_delta * multiplier;
		value_changed = true;
	}

	if (g_rt_enable_debug_menu)
	{
		RT_DoRendererDebugMenuParams params = {
			.ui_has_cursor_focus = true,
		};
		RT_DoRendererDebugMenus(&params);

		if (igBegin("HUD Texture Debug", NULL, 0)) {
			igPushID_Str("HUD Texture Debug");
			igIndent(0);
			RT_RenderImGuiTexture(g_rt_cockpit_settings.cockpit_hud_texture, 1024, 1024);
			igPopID();
		} igEnd();

		if (igBegin("3D Cockpit", NULL, 0)) {
			igPushID_Str("Dynamic Lights");
			igIndent(0);
			value_changed |= igDragFloat3("Front View Rotation", &g_rt_cockpit_settings.front_cockpit_rotation, 0.001f, -RT_PI32, +RT_PI32, "%.3f", ImGuiTreeNodeFlags_None);
			value_changed |= igDragFloat3("Front View Offset", &g_rt_cockpit_settings.front_cockpit_offset, 0.001f, -10.0, 10.0, "%.3f", ImGuiTreeNodeFlags_None);
			value_changed |= igDragFloat3("Front View Scale", &g_rt_cockpit_settings.front_cockpit_scale, 0.001f, -10.0, 10.0, "%.3f", ImGuiTreeNodeFlags_None);
			value_changed |= igDragFloat3("Rear View Rotation", &g_rt_cockpit_settings.back_cockpit_rotation, 0.001f, -RT_PI32, +RT_PI32, "%.3f", ImGuiTreeNodeFlags_None);		
			value_changed |= igDragFloat3("Rear View Offset", &g_rt_cockpit_settings.back_cockpit_offset, 0.001f, -10.0, 10.0, "%.3f", ImGuiTreeNodeFlags_None);
			value_changed |= igDragFloat3("Rear View Scale", &g_rt_cockpit_settings.back_cockpit_scale, 0.001f, -10.0, 10.0, "%.3f", ImGuiTreeNodeFlags_None);

			igPopID();
		} igEnd();

		if (igBegin("Ingame Tools", NULL, 0))
		{
			if (igCollapsingHeader_TreeNodeFlags("Light Culling", ImGuiTreeNodeFlags_None)) {
				igPushID_Int(1);
				igSliderInt("Light Culling Heuristic", &light_culling_heuristic, 0, 1, "%i", 0);
				igSliderInt("Max Segment Recursion Depth", &max_rec_depth, 0, 50, "%i segments", 0);
				//igSliderFloat("Max Segment Distance", &max_seg_distance, 0.0f, 5000.0f, "%.1f units", 0); // note(lily): this is broken so I commented it out
				igSliderFloat("Max Light Distance", &max_distance, 0.0f, 1000.0f, "%.1f units", 0);
				igPopID();
			}

			if (igCollapsingHeader_TreeNodeFlags("Miscellaneous", ImGuiTreeNodeFlags_None))
			{
				igPushID_Str("Miscellaneous");
				igIndent(0);

				igSliderInt("Flash screen multiplier", &screen_flash_mul, 1, 100, "%d", 0);

				igPopID();
				igUnindent(0);
			}

			if (igCollapsingHeader_TreeNodeFlags("Free cam settings", ImGuiTreeNodeFlags_None))
			{
				igCheckbox("Enable free cam clipping", &g_rt_free_cam_info.g_free_cam_clipping_enabled);
			}

		} igEnd();

		RT_DoDynamicLightEditorMenus();
		RT_ShowLightMenu();

		RT_DoPolymodelViewerMenus();
		RT_DoMaterialViewerMenus();
	}

	if (value_changed) {
		RT_Config cfg;
		RT_InitializeConfig(&cfg, &g_thread_arena);
		RT_ConfigWriteVec3(&cfg, RT_StringLiteral("front_cockpit_rotation"), g_rt_cockpit_settings.front_cockpit_rotation);
		RT_ConfigWriteVec3(&cfg, RT_StringLiteral("front_cockpit_offset"), g_rt_cockpit_settings.front_cockpit_offset);
		RT_ConfigWriteVec3(&cfg, RT_StringLiteral("front_cockpit_scale"), g_rt_cockpit_settings.front_cockpit_scale);
		RT_ConfigWriteVec3(&cfg, RT_StringLiteral("back_cockpit_rotation"), g_rt_cockpit_settings.back_cockpit_rotation);
		RT_ConfigWriteVec3(&cfg, RT_StringLiteral("back_cockpit_offset"), g_rt_cockpit_settings.back_cockpit_offset);
		RT_ConfigWriteVec3(&cfg, RT_StringLiteral("back_cockpit_scale"), g_rt_cockpit_settings.back_cockpit_scale);
		RT_SerializeConfigToFile(&cfg, "cockpit_3d.cfg");
	}
}

void RT_EndImguiFrame(void)
{
	igRender();
	igEndFrame();
}

void save_screen_shot(int automap_flag)
{
	static int savenum=0;
	char savename[FILENAME_LEN+sizeof(SCRNS_DIR)];

	stop_time();

	if (!PHYSFSX_exists(SCRNS_DIR,0))
		PHYSFS_mkdir(SCRNS_DIR); //try making directory

	do
	{
		sprintf(savename, "%sscrn%04d.png",SCRNS_DIR, savenum++);
	} while (PHYSFSX_exists(savename,0));

	if (!automap_flag)
		HUD_init_message(HM_DEFAULT, "%s 'scrn%04d.png'", TXT_DUMPING_SCREEN, savenum-1 );

	RT_QueueScreenshot(savename);

	start_time();
}
