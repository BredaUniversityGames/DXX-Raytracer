/*
 *
 * OGL video functions. - Added 9/15/99 Matthew Mueller
 *
 */

#define DECLARE_VARS

#ifdef RPI
// extra libraries for the Raspberry Pi
#include  "bcm_host.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _MSC_VER
#include <windows.h>
#endif

#if !defined(_MSC_VER) && !defined(macintosh)
#include <unistd.h>
#endif
#if !defined(macintosh)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include <errno.h>
#include <SDL/SDL.h>
#include "hudmsg.h"
#include "game.h"
#include "text.h"
#include "gr.h"
#include "gamefont.h"
#include "grdef.h"
#include "palette.h"
#include "u_mem.h"
#include "inferno.h"
#include "screens.h"
#include "strutil.h"
#include "args.h"
#include "key.h"
#include "physfsx.h"
#include "playsave.h"
#include "internal.h"
#include "render.h"
#include "console.h"
#include "config.h"
#include "vers_id.h"
#include "game.h"
#include "logger.h"

#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/glu.h>
#else
#ifdef OGLES
#include <EGL/egl.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <SDL/SDL_syswm.h>
#else
#include <GL/glu.h>
#endif
#endif

#ifdef OGLES
int sdl_video_flags = 0;

#ifdef RPI
static EGL_DISPMANX_WINDOW_T nativewindow;
static DISPMANX_ELEMENT_HANDLE_T dispman_element=DISPMANX_NO_HANDLE;
static DISPMANX_DISPLAY_HANDLE_T dispman_display=DISPMANX_NO_HANDLE;
#endif

#else
int sdl_video_flags = SDL_OPENGL;
#endif
int gr_installed = 0;
int gl_initialized=0;
int linedotscale=1; // scalar of glLinewidth and glPointSize - only calculated once when resolution changes
int sdl_no_modeswitch=0;

#ifdef OGLES
EGLDisplay eglDisplay=EGL_NO_DISPLAY;
EGLConfig eglConfig;
EGLSurface eglSurface=EGL_NO_SURFACE;
EGLContext eglContext=EGL_NO_CONTEXT;

bool TestEGLError(char* pszLocation)
{
	/*
	 * eglGetError returns the last error that has happened using egl,
	 * not the status of the last called function. The user has to
	 * check after every single egl call or at least once every frame.
	*/
	EGLint iErr = eglGetError();
	if (iErr != EGL_SUCCESS)
	{
		RT_LOGF(RT_LOGSERVERITY_HIGH, "%s failed (%d).\n", pszLocation, iErr);
		return 0;
	}
	
	return 1;
}
#endif

void ogl_swap_buffers_internal(void)
{
#ifdef OGLES
	eglSwapBuffers(eglDisplay, eglSurface);
#else
	SDL_GL_SwapBuffers();
#endif
}

#ifdef RPI

// MH: I got the following constants for vc_dispmanx_element_change_attributes() from:
//     http://qt.gitorious.org/qt/qtbase/commit/5933205cfcd73481cb0645fa6183103063fe3e0d
//     I do not know where they got them from, but OTOH, they are quite obvious.

// these constants are not in any headers (yet)
#define ELEMENT_CHANGE_LAYER          (1<<0)
#define ELEMENT_CHANGE_OPACITY        (1<<1)
#define ELEMENT_CHANGE_DEST_RECT      (1<<2)
#define ELEMENT_CHANGE_SRC_RECT       (1<<3)
#define ELEMENT_CHANGE_MASK_RESOURCE  (1<<4)
#define ELEMENT_CHANGE_TRANSFORM      (1<<5)

void rpi_destroy_element(void)
{
	if (dispman_element != DISPMANX_NO_HANDLE) {
		DISPMANX_UPDATE_HANDLE_T dispman_update;
		RT_LOG(RT_LOGSERVERITY_INFO, "RPi: destroying display manager element\n");
		dispman_update = vc_dispmanx_update_start( 0 );
		if (vc_dispmanx_element_remove( dispman_update, dispman_element)) {
			RT_LOG(RT_LOGSERVERITY_HIGH, "RPi: failed to remove dispmanx element!\n");
		}
		vc_dispmanx_update_submit_sync( dispman_update );
		dispman_element = DISPMANX_NO_HANDLE;
	}
}

int rpi_setup_element(int x, int y, Uint32 video_flags, int update)
{
	// this code is based on the work of Ben O'Steen
	// http://benosteen.wordpress.com/2012/04/27/using-opengl-es-2-0-on-the-raspberry-pi-without-x-windows/
	// https://github.com/benosteen/opengles-book-samples/tree/master/Raspi
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;
	VC_DISPMANX_ALPHA_T alpha_descriptor;

	uint32_t rpi_display_device=DISPMANX_ID_MAIN_LCD;
	uint32_t display_width;
	uint32_t display_height;
	int success;

	success = graphics_get_display_size(rpi_display_device, &display_width, &display_height);
	if ( success < 0 ) {
		RT_LOG(RT_LOGSERVERITY_HIGH, "Could not get RPi display size, assuming 640x480\n");
		display_width=640;
		display_height=480;
	}

	if ((uint32_t)x > display_width) {
		RT_LOGF(RT_LOGSERVERITY_HIGH, "RPi: Requested width %d exceeds display width %u, scaling down!\n",
			   x, display_width);
		x=(int)display_width;
	}
	if ((uint32_t)y > display_height) {
		RT_LOGF(RT_LOGSERVERITY_HIGH, "RPi: Requested height %d exceeds display height %u, scaling down!\n",
				y, display_height);
		y=(int)display_height;
	}

	RT_LOGF(RT_LOGSERVERITY_INFO, "RPi: display resolution %ux%u, game resolution: %dx%d (%s)\n", display_width, display_height, x, y, (video_flags & SDL_FULLSCREEN) ? "fullscreen" : "windowed");
	if (video_flags & SDL_FULLSCREEN) {
		/* scale to the full display size... */
		dst_rect.x = 0;
		dst_rect.y = 0;
		dst_rect.width = display_width;
		dst_rect.height= display_height;
	} else {
		/* TODO: we could query the position of the X11 window here
		   and try to place the ovelray exactly above that...,
		   we would have to track window movements, though ... */
		dst_rect.x = 0;
		dst_rect.y = 0;
		dst_rect.width = (uint32_t)x;
		dst_rect.height= (uint32_t)y;
	}

	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = ((uint32_t)x)<< 16;
	src_rect.height =((uint32_t)y)<< 16;

	/* we do not want our overlay to be blended against the background */
	alpha_descriptor.flags=DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS;
	alpha_descriptor.opacity=0xffffffff;
	alpha_descriptor.mask=0;

	// open display, if we do not already have one ...
	if (dispman_display == DISPMANX_NO_HANDLE) {
		RT_LOGF(RT_LOGSERVERITY_INFO, "RPi: opening display: %u\n", rpi_display_device);
		dispman_display = vc_dispmanx_display_open(rpi_display_device);
		if (dispman_display == DISPMANX_NO_HANDLE) {
			RT_LOGF(RT_LOGSERVERITY_HIGH, "RPi: failed to open display: %u\n", rpi_display_device);
		}
	}

	if (dispman_element != DISPMANX_NO_HANDLE) {
		if (!update) {
			// if the element already exists, and we cannot update it, so recreate it
			rpi_destroy_element();
		}
	} else {
		// if the element does not exist, we cannot do an update
		update=0;
	}

	dispman_update = vc_dispmanx_update_start( 0 );

	if (update) {
		RT_LOG(RT_LOGSERVERITY_INFO, "RPi: updating display manager element\n");
		vc_dispmanx_element_change_attributes ( dispman_update, nativewindow.element,
							ELEMENT_CHANGE_DEST_RECT | ELEMENT_CHANGE_SRC_RECT,
							0 /*layer*/, 0 /*opacity*/,
							&dst_rect, &src_rect,
							0 /*mask*/, VC_IMAGE_ROT0 /*transform*/);
	} else {
		// create a new element
		RT_LOG(RT_LOGSERVERITY_INFO, "RPi: creating display manager element\n");
		dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display,
								0 /*layer*/, &dst_rect, 0 /*src*/,
								&src_rect, DISPMANX_PROTECTION_NONE,
								&alpha_descriptor, NULL /*clamp*/,
								VC_IMAGE_ROT0 /*transform*/);
		if (dispman_element == DISPMANX_NO_HANDLE) {
			RT_LOG(RT_LOGSERVERITY_HIGH, "RPi: failed to creat display manager element\n");
		}
		nativewindow.element = dispman_element;
	}
	nativewindow.width = display_width;
	nativewindow.height = display_height;
	vc_dispmanx_update_submit_sync( dispman_update );

	return 0;
}

#endif // RPI

#ifdef OGLES
void ogles_destroy(void)
{
	if( eglDisplay != EGL_NO_DISPLAY ) {
		eglMakeCurrent(eglDisplay, NULL, NULL, EGL_NO_CONTEXT);
	}

	if (eglContext != EGL_NO_CONTEXT) {
		RT_LOG(RT_LOGSERVERITY_INFO, "EGL: destroyig context\n");
		eglDestroyContext(eglDisplay, eglContext);
		eglContext = EGL_NO_CONTEXT;
	}

	if (eglSurface != EGL_NO_SURFACE) {
		RT_LOG(RT_LOGSERVERITY_INFO, "EGL: destroyig surface\n");
		eglDestroySurface(eglDisplay, eglSurface);
		eglSurface = EGL_NO_SURFACE;
	}

	if (eglDisplay != EGL_NO_DISPLAY) {
		RT_LOG(RT_LOGSERVERITY_INFO, "EGL: terminating\n");
		eglTerminate(eglDisplay);
		eglDisplay = EGL_NO_DISPLAY;
	}
}
#endif

int ogl_init_window(int x, int y)
{
	int use_x,use_y,use_bpp;
	Uint32 use_flags;

#ifdef OGLES
	SDL_SysWMinfo info;
	Window    x11Window = 0;
	Display*  x11Display = 0;
	EGLint    ver_maj, ver_min;
	EGLint configAttribs[] =
	{
		EGL_RED_SIZE, 5,
		EGL_GREEN_SIZE, 6,
		EGL_BLUE_SIZE, 5,
		EGL_DEPTH_SIZE, 16,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
		EGL_NONE, EGL_NONE
	};

	// explicitely request an OpenGL ES 1.x context
        EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 1, EGL_NONE, EGL_NONE };
	// explicitely request a doublebuffering window
        EGLint winAttribs[] = { EGL_RENDER_BUFFER, EGL_BACK_BUFFER, EGL_NONE, EGL_NONE };

	int iConfigs;
#endif // OGLES

	if (gl_initialized)
		ogl_smash_texture_list_internal();//if we are or were fullscreen, changing vid mode will invalidate current textures

	SDL_WM_SetCaption(DESCENT_VERSION, "Descent");
	SDL_WM_SetIcon( SDL_LoadBMP( "d1x-rebirth.bmp" ), NULL );

	use_x=x;
	use_y=y;
	use_bpp=GameArg.DbgBpp;
	use_flags=sdl_video_flags;
	if (sdl_no_modeswitch) {
		const SDL_VideoInfo *vinfo=SDL_GetVideoInfo();
		if (vinfo) {	
			use_x=vinfo->current_w;
			use_y=vinfo->current_h;
			use_bpp=vinfo->vfmt->BitsPerPixel;
			use_flags=SDL_SWSURFACE | SDL_ANYFORMAT;
		} else {
			RT_LOG(RT_LOGSERVERITY_HIGH, "Could not query video info\n");
		}
	}
	
	if (!SDL_SetVideoMode(use_x, use_y, use_bpp, use_flags))
	{
#ifdef RPI
		RT_LOGF(RT_LOGSERVERITY_HIGH, "Could not set %dx%dx%d opengl video mode: %s\n (Ignored for RPI)",
			   x, y, GameArg.DbgBpp, SDL_GetError());
#else
		RT_LOGF(RT_LOGSERVERITY_HIGH, "Could not set %dx%dx%d opengl video mode: %s\n", x, y, GameArg.DbgBpp, SDL_GetError());
#endif
	}
	SDL_Surface t_Surf;

#ifdef OGLES
#ifndef RPI
	// NOTE: on the RPi, the EGL stuff is not connected to the X11 window,
	//       so there is no need to destroy and recreate this
	ogles_destroy();
#endif

	SDL_VERSION(&info.version);
	
	if (SDL_GetWMInfo(&info) > 0) {
		if (info.subsystem == SDL_SYSWM_X11) {
			x11Display = info.info.x11.display;
			x11Window = info.info.x11.window;
			RT_LOGF(RT_LOGSERVERITY_INFO, "Display: %p, Window: %i ===\n", (void *)x11Display, (int)x11Window);
		}
	}

	if (eglDisplay == EGL_NO_DISPLAY) {
#ifdef RPI
		eglDisplay = eglGetDisplay((EGLNativeDisplayType)EGL_DEFAULT_DISPLAY);
#else
		eglDisplay = eglGetDisplay((EGLNativeDisplayType)x11Display);
#endif
		if (eglDisplay == EGL_NO_DISPLAY) {
			RT_LOG(RT_LOGSERVERITY_HIGH, "EGL: Error querying EGL Display\n");
		}

		if (!eglInitialize(eglDisplay, &ver_maj, &ver_min)) {
			RT_LOG(RT_LOGSERVERITY_HIGH, "EGL: Error initializing EGL\n");
		} else {
			RT_LOGF(RT_LOGSERVERITY_INFO, "EGL: Initialized, version: major %i minor %i\n", ver_maj, ver_min);
		}
	}

	
#ifdef RPI
	if (rpi_setup_element(x,y,sdl_video_flags,1)) {
		RT_LOGF(RT_LOGSERVERITY_HIGH, "RPi: Could not set up a %dx%d element\n", x, y);
	}
#endif

	if (eglSurface == EGL_NO_SURFACE) {
		if (!eglChooseConfig(eglDisplay, configAttribs, &eglConfig, 1, &iConfigs) || (iConfigs != 1)) {
			RT_LOG(RT_LOGSERVERITY_HIGH, "EGL: Error choosing config\n");
		} else {
			RT_LOG(RT_LOGSERVERITY_INFO, "EGL: config chosen\n");
		}

#ifdef RPI
		eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, (EGLNativeWindowType)&nativewindow, winAttribs);
#else
		eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, (NativeWindowType)x11Window, winAttribs);
#endif
		if ((!TestEGLError("eglCreateWindowSurface")) || eglSurface == EGL_NO_SURFACE) {
			RT_LOG(RT_LOGSERVERITY_HIGH, "EGL: Error creating window surface\n");
		} else {
			RT_LOG(RT_LOGSERVERITY_INFO, "EGL: Created window surface\n");
		}
	}
	
	if (eglContext == EGL_NO_CONTEXT) {
		eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttribs);
		if ((!TestEGLError("eglCreateContext")) || eglContext == EGL_NO_CONTEXT) {
			RT_LOG(RT_LOGSERVERITY_HIGH, "EGL: Error creating context\n");
		} else {
			RT_LOG(RT_LOGSERVERITY_INFO, "EGL: Created context\n");
		}
	}
	
	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
	if (!TestEGLError("eglMakeCurrent")) {
		RT_LOG(RT_LOGSERVERITY_HIGH, "EGL: Error making current\n");
	} else {
		RT_LOG(RT_LOGSERVERITY_INFO, "EGL: made context current\n");
	}
#endif

	linedotscale = ((x/640<y/480?x/640:y/480)<1?1:(x/640<y/480?x/640:y/480));

	gl_initialized=1;
	return 0;
}

int gr_check_fullscreen(void)
{
	return (sdl_video_flags & SDL_FULLSCREEN)?1:0;
}

int gr_toggle_fullscreen(void)
{
	if (sdl_video_flags & SDL_FULLSCREEN)
		sdl_video_flags &= ~SDL_FULLSCREEN;
	else
		sdl_video_flags |= SDL_FULLSCREEN;

	if (gl_initialized)
	{
		if (sdl_no_modeswitch == 0) {
			if (!SDL_VideoModeOK(SM_W(Game_screen_mode), SM_H(Game_screen_mode), GameArg.DbgBpp, sdl_video_flags))
			{
				RT_LOGF(RT_LOGSERVERITY_HIGH, "Cannot set %ix%i. Fallback to 640x480\n", SM_W(Game_screen_mode), SM_H(Game_screen_mode));
				Game_screen_mode=SM(640,480);
			}
			if (!SDL_SetVideoMode(SM_W(Game_screen_mode), SM_H(Game_screen_mode), GameArg.DbgBpp, sdl_video_flags))
			{
				RT_LOGF(RT_LOGSERVERITY_HIGH, "Could not set %dx%dx%d opengl video mode: %s\n", SM_W(Game_screen_mode), SM_H(Game_screen_mode), GameArg.DbgBpp, SDL_GetError());
			}
		}
#ifdef RPI
		if (rpi_setup_element(SM_W(Game_screen_mode), SM_H(Game_screen_mode), sdl_video_flags, 1)) {
			RT_LOGF(RT_LOGSERVERITY_HIGH, "RPi: Could not set up %dx%d element\n", SM_W(Game_screen_mode), SM_H(Game_screen_mode));
		}
#endif
	}

	if (gl_initialized) // update viewing values for menus
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
#ifdef OGLES
		glOrthof(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
#else
 		glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
#endif
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();//clear matrix
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		ogl_smash_texture_list_internal();//if we are or were fullscreen, changing vid mode will invalidate current textures
	}
	GameCfg.WindowMode = (sdl_video_flags & SDL_FULLSCREEN)?0:1;
	return (sdl_video_flags & SDL_FULLSCREEN)?1:0;
}

static void ogl_init_state(void)
{
	/* select clearing (background) color   */
	glClearColor(0.0, 0.0, 0.0, 0.0);

	/* initialize viewing values */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
#ifdef OGLES
	glOrthof(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
#else
 	glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
#endif
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();//clear matrix
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gr_palette_step_up(0,0,0);//in case its left over from in game

	ogl_init_pixel_buffers(grd_curscreen->sc_w, grd_curscreen->sc_h);
}

// Set the buffer to draw to. 0 is front, 1 is back
void gr_set_draw_buffer(int buf)
{
#ifndef OGLES
	glDrawBuffer((buf == 0) ? GL_FRONT : GL_BACK);
#endif
}

const char *gl_vendor, *gl_renderer, *gl_version, *gl_extensions;

void ogl_get_verinfo(void)
{
#ifndef OGLES
	gl_vendor = (const char *) glGetString (GL_VENDOR);
	gl_renderer = (const char *) glGetString (GL_RENDERER);
	gl_version = (const char *) glGetString (GL_VERSION);
	gl_extensions = (const char *) glGetString (GL_EXTENSIONS);

	RT_LOGF(RT_LOGSERVERITY_MINOR, "OpenGL: vendor: %s\nOpenGL: renderer: %s\nOpenGL: version: %s\n", gl_vendor, gl_renderer, gl_version);

#ifdef _WIN32
	dglMultiTexCoord2fARB = (glMultiTexCoord2fARB_fp)wglGetProcAddress("glMultiTexCoord2fARB");
	dglActiveTextureARB = (glActiveTextureARB_fp)wglGetProcAddress("glActiveTextureARB");
	dglMultiTexCoord2fSGIS = (glMultiTexCoord2fSGIS_fp)wglGetProcAddress("glMultiTexCoord2fSGIS");
	dglSelectTextureSGIS = (glSelectTextureSGIS_fp)wglGetProcAddress("glSelectTextureSGIS");
#endif

	//add driver specific hacks here.  whee.
	if ((d_stricmp(gl_renderer,"Mesa NVIDIA RIVA 1.0\n")==0 || d_stricmp(gl_renderer,"Mesa NVIDIA RIVA 1.2\n")==0) && d_stricmp(gl_version,"1.2 Mesa 3.0")==0)
	{
		GameArg.DbgGlIntensity4Ok=0;//ignores alpha, always black background instead of transparent.
		GameArg.DbgGlReadPixelsOk=0;//either just returns all black, or kills the X server entirely
		GameArg.DbgGlGetTexLevelParamOk=0;//returns random data..
	}
	if (d_stricmp(gl_vendor,"Matrox Graphics Inc.")==0)
	{
		//displays garbage. reported by
		//  redomen@crcwnet.com (render="Matrox G400" version="1.1.3 5.52.015")
		//  orulz (Matrox G200)
		GameArg.DbgGlIntensity4Ok=0;
	}
#ifdef macintosh
	if (d_stricmp(gl_renderer,"3dfx Voodoo 3")==0) // strangely, includes Voodoo 2
		GameArg.DbgGlGetTexLevelParamOk=0; // Always returns 0
#endif

#ifndef NDEBUG
	RT_LOGF(RT_LOGSERVERITY_MINOR, "gl_intensity4:%i gl_luminance4_alpha4:%i gl_rgba2:%i gl_readpixels:%i gl_gettexlevelparam:%i\n", GameArg.DbgGlIntensity4Ok, GameArg.DbgGlLuminance4Alpha4Ok, GameArg.DbgGlRGBA2Ok, GameArg.DbgGlReadPixelsOk, GameArg.DbgGlGetTexLevelParamOk);
#endif
	if (!d_stricmp(gl_extensions,"GL_EXT_texture_filter_anisotropic")==0)
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &ogl_maxanisotropy);
		RT_LOGF(RT_LOGSERVERITY_MINOR, "ogl_maxanisotropy:%f\n", ogl_maxanisotropy);
	}
	else if (GameCfg.TexFilt >= 3)
		GameCfg.TexFilt = 2;
#endif
}

// returns possible (fullscreen) resolutions if any.
int gr_list_modes( u_int32_t gsmodes[] )
{
	SDL_Rect** modes;
	int i = 0, modesnum = 0;
#ifdef OGLES
	int sdl_check_flags = SDL_FULLSCREEN; // always use Fullscreen as lead.
#else
	int sdl_check_flags = SDL_OPENGL | SDL_FULLSCREEN; // always use Fullscreen as lead.
#endif

	if (sdl_no_modeswitch) {
		/* TODO: we could use the tvservice to list resolutions on the RPi */
		return 0;
	}

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
			gsmodes[modesnum] = SM(modes[i]->w,modes[i]->h);
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

	w=SM_W(mode);
	h=SM_H(mode);

	if (sdl_no_modeswitch == 0) {
		return SDL_VideoModeOK(w, h, GameArg.DbgBpp, sdl_video_flags);
	} else {
		// just tell the caller that any mode is valid...
		return 32;
	}
}

int gr_set_mode(u_int32_t mode)
{
	unsigned int w, h;
	char *gr_bm_data;

	if (mode<=0)
		return 0;

	w = SM_W(mode);
	h = SM_H(mode);

	if (!gr_check_mode(mode))
	{
		RT_LOGF(RT_LOGSERVERITY_HIGH, "Cannot set %ix%i. Fallback to 640x480\n", w, h);
		w=640;
		h=480;
		Game_screen_mode=mode=SM(w,h);
	}

	gr_bm_data=(char *)grd_curscreen->sc_canvas.cv_bitmap.bm_data;//since we use realloc, we want to keep this pointer around.
	memset( grd_curscreen, 0, sizeof(grs_screen));
	grd_curscreen->sc_mode = mode;
	grd_curscreen->sc_w = w;
	grd_curscreen->sc_h = h;
	grd_curscreen->sc_aspect = fixdiv(GameCfg.AspectX, GameCfg.AspectY);
	gr_init_canvas(&grd_curscreen->sc_canvas, d_realloc(gr_bm_data,w*h), BM_OGL, w, h);
	gr_set_current_canvas(NULL);

	ogl_init_window(w,h);//platform specific code
	ogl_get_verinfo();
	OGL_VIEWPORT(0,0,w,h);
	ogl_init_state();
	gamefont_choose_game_font(w,h);

	return 0;
}

#define GLstrcmptestr(a,b) if (d_stricmp(a,#b)==0 || d_stricmp(a,"GL_" #b)==0)return GL_ ## b;
int ogl_atotexfilti(char *a,int min)
{
	GLstrcmptestr(a,NEAREST);
	GLstrcmptestr(a,LINEAR);
	if (min)
	{//mipmaps are valid only for the min filter
		GLstrcmptestr(a,NEAREST_MIPMAP_NEAREST);
		GLstrcmptestr(a,NEAREST_MIPMAP_LINEAR);
		GLstrcmptestr(a,LINEAR_MIPMAP_NEAREST);
		GLstrcmptestr(a,LINEAR_MIPMAP_LINEAR);
	}
	RT_LOGF(RT_LOGSERVERITY_HIGH, "unknown/invalid texture filter %s\n", a);
}

#ifdef _WIN32
char *OglLibPath="opengl32.dll";

int ogl_rt_loaded=0;
int ogl_init_load_library(void)
{
	int retcode=0;
	if (!ogl_rt_loaded)
	{
		retcode = OpenGL_LoadLibrary(true);
		if(retcode)
		{
			if(!glEnd)
			{
				RT_LOG(RT_LOGSERVERITY_HIGH, "Opengl: Functions not imported\n");
			}
		}
		else
		{
			RT_LOGF(RT_LOGSERVERITY_HIGH, "Opengl: error loading %s\n", OglLibPath);
		}
		ogl_rt_loaded=1;
	}
	return retcode;
}
#endif

void gr_set_attributes(void)
{
#ifndef OGLES
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL,GameCfg.VSync);
	if (GameCfg.Multisample)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	}
	else
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	}
#endif
	ogl_smash_texture_list_internal();
}

int gr_init(int mode)
{
#ifdef RPI
	char sdl_driver[32];
	char *sdl_driver_ret;
#endif

	int retcode;

	// Only do this function once!
	if (gr_installed==1)
		return -1;

#ifdef RPI
	// Initialize the broadcom host library
	// we have to call this before we can create an OpenGL ES context
	bcm_host_init();

	// Check if we are running with SDL directfb driver ...
	sdl_driver_ret=SDL_VideoDriverName(sdl_driver,32);
	if (sdl_driver_ret) {
		if (strcmp(sdl_driver_ret,"x11")) {
			RT_LOG(RT_LOGSERVERITY_HIGH, "RPi: activating hack for console driver\n");
			sdl_no_modeswitch=1;
		}
	}
#endif

#ifdef _WIN32
	ogl_init_load_library();
#endif

	if (!GameCfg.WindowMode && !GameArg.SysWindow)
		sdl_video_flags|=SDL_FULLSCREEN;

	if (GameArg.SysNoBorders)
		sdl_video_flags|=SDL_NOFRAME;

	gr_set_attributes();

	ogl_init_texture_list_internal();

	MALLOC( grd_curscreen,grs_screen,1 );
	memset( grd_curscreen, 0, sizeof(grs_screen));
	grd_curscreen->sc_canvas.cv_bitmap.bm_data = NULL;

	// Set the mode.
	if ((retcode=gr_set_mode(mode)))
		return retcode;

	grd_curscreen->sc_canvas.cv_color = 0;
	grd_curscreen->sc_canvas.cv_fade_level = GR_FADE_OFF;
	grd_curscreen->sc_canvas.cv_blend_func = GR_BLEND_NORMAL;
	grd_curscreen->sc_canvas.cv_drawmode = 0;
	grd_curscreen->sc_canvas.cv_font = NULL;
	grd_curscreen->sc_canvas.cv_font_fg_color = 0;
	grd_curscreen->sc_canvas.cv_font_bg_color = 0;
	gr_set_current_canvas( &grd_curscreen->sc_canvas );

	ogl_init_pixel_buffers(256, 128);       // for gamefont_init

	gr_installed = 1;

	return 0;
}

//NOTE (sam)
//I moved this code here to remove some very unneeded externals in internal.h no reason for them to be there.
int ogl_brightness_ok = 0;
int ogl_brightness_r = 0, ogl_brightness_g = 0, ogl_brightness_b = 0;
static int old_b_r = 0, old_b_g = 0, old_b_b = 0;
void gr_close()
{
	ogl_brightness_r = ogl_brightness_g = ogl_brightness_b = 0;

	if (gl_initialized)
	{
		ogl_smash_texture_list_internal();
	}

	if (grd_curscreen)
	{
		if (grd_curscreen->sc_canvas.cv_bitmap.bm_data)
			d_free(grd_curscreen->sc_canvas.cv_bitmap.bm_data);
		d_free(grd_curscreen);
	}
	ogl_close_pixel_buffers();
#ifdef _WIN32
	if (ogl_rt_loaded)
		OpenGL_LoadLibrary(false);
#endif

#ifdef OGLES
	ogles_destroy();
#ifdef RPI
	RT_LOG(RT_LOGSERVERITY_INFO, "RPi: cleanuing up\n");
	if (dispman_display != DISPMANX_NO_HANDLE) {
		rpi_destroy_element();
		RT_LOG(RT_LOGSERVERITY_INFO, "RPi: closing display\n");
		vc_dispmanx_display_close(dispman_display);
		dispman_display = DISPMANX_NO_HANDLE;
	}
#endif
#endif
}

extern int r_upixelc;
void ogl_upixelc(int x, int y, int c)
{
	GLfloat vertex_array[] = { (x+grd_curcanv->cv_bitmap.bm_x)/(float)last_width, 1.0-(y+grd_curcanv->cv_bitmap.bm_y)/(float)last_height };
	GLfloat color_array[] = { CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), 1.0, CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), 1.0, CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), 1.0, CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), 1.0 };

	r_upixelc++;
	OGL_DISABLE(TEXTURE_2D);
	glPointSize(linedotscale);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glDrawArrays(GL_POINTS, 0, 1);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

unsigned char ogl_ugpixel( grs_bitmap * bitmap, int x, int y )
{
	GLint gl_draw_buffer;
	ubyte buf[4];

#ifndef OGLES
	glGetIntegerv(GL_DRAW_BUFFER, &gl_draw_buffer);
	glReadBuffer(gl_draw_buffer);
#endif

	glReadPixels(bitmap->bm_x + x, SHEIGHT - bitmap->bm_y - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buf);
	
	return gr_find_closest_color(buf[0]/4, buf[1]/4, buf[2]/4);
}

void ogl_urect(int left,int top,int right,int bot)
{
	GLfloat xo, yo, xf, yf, color_r, color_g, color_b, color_a;
	GLfloat color_array[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	GLfloat vertex_array[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	int c=COLOR;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	xo=(left+grd_curcanv->cv_bitmap.bm_x)/(float)last_width;
	xf = (right + 1 + grd_curcanv->cv_bitmap.bm_x) / (float)last_width;
	yo=1.0-(top+grd_curcanv->cv_bitmap.bm_y)/(float)last_height;
	yf = 1.0 - (bot + 1 + grd_curcanv->cv_bitmap.bm_y) / (float)last_height;

	OGL_DISABLE(TEXTURE_2D);

	color_r = CPAL2Tr(c);
	color_g = CPAL2Tg(c);
	color_b = CPAL2Tb(c);

	if (grd_curcanv->cv_fade_level >= GR_FADE_OFF)
		color_a = 1.0;
	else
		color_a = 1.0 - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0);

	color_array[0] = color_array[4] = color_array[8] = color_array[12] = color_r;
	color_array[1] = color_array[5] = color_array[9] = color_array[13] = color_g;
	color_array[2] = color_array[6] = color_array[10] = color_array[14] = color_b;
	color_array[3] = color_array[7] = color_array[11] = color_array[15] = color_a;

	vertex_array[0] = xo;
	vertex_array[1] = yo;
	vertex_array[2] = xo;
	vertex_array[3] = yf;
	vertex_array[4] = xf;
	vertex_array[5] = yf;
	vertex_array[6] = xf;
	vertex_array[7] = yo;
	
	glVertexPointer(2, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);//replaced GL_QUADS
	glDisableClientState(GL_VERTEX_ARRAY);
}

void ogl_ulinec(int left,int top,int right,int bot,int c)
{
	GLfloat xo,yo,xf,yf;
	GLfloat color_array[] = { CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), (grd_curcanv->cv_fade_level >= GR_FADE_OFF)?1.0:1.0 - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0), CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), (grd_curcanv->cv_fade_level >= GR_FADE_OFF)?1.0:1.0 - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0), CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), 1.0, CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), (grd_curcanv->cv_fade_level >= GR_FADE_OFF)?1.0:1.0 - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0) };
	GLfloat vertex_array[] = { 0.0, 0.0, 0.0, 0.0 };

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
	xo = (left + grd_curcanv->cv_bitmap.bm_x + 0.5) / (float)last_width;
	xf = (right + grd_curcanv->cv_bitmap.bm_x + 1.0) / (float)last_width;
	yo = 1.0 - (top + grd_curcanv->cv_bitmap.bm_y + 0.5) / (float)last_height;
	yf = 1.0 - (bot + grd_curcanv->cv_bitmap.bm_y + 1.0) / (float)last_height;
 
	OGL_DISABLE(TEXTURE_2D);

	vertex_array[0] = xo;
	vertex_array[1] = yo;
	vertex_array[2] = xf;
	vertex_array[3] = yf;

	glVertexPointer(2, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glDrawArrays(GL_LINES, 0, 2);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

GLfloat last_r=0, last_g=0, last_b=0;
int do_pal_step=0;

void ogl_do_palfx(void)
{
	GLfloat color_array[] = { last_r, last_g, last_b, 1.0, last_r, last_g, last_b, 1.0, last_r, last_g, last_b, 1.0, last_r, last_g, last_b, 1.0 };
	GLfloat vertex_array[] = { 0,0,0,1,1,1,1,0 };

	OGL_DISABLE(TEXTURE_2D);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
 
	if (do_pal_step)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE,GL_ONE);
	}
	else
		return;
 
	glVertexPointer(2, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);//replaced GL_QUADS
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void gr_palette_step_up(int r, int g, int b)
{
	old_b_r = ogl_brightness_r;
	old_b_g = ogl_brightness_g;
	old_b_b = ogl_brightness_b;

	ogl_brightness_r = max(r + gr_palette_gamma, 0);
	ogl_brightness_g = max(g + gr_palette_gamma, 0);
	ogl_brightness_b = max(b + gr_palette_gamma, 0);

	if (!ogl_brightness_ok)
	{
		last_r = ogl_brightness_r / 63.0;
		last_g = ogl_brightness_g / 63.0;
		last_b = ogl_brightness_b / 63.0;

		do_pal_step = (r || g || b || gr_palette_gamma);
	}
	else
	{
		do_pal_step = 0;
	}
}

#undef min
static inline int min(int x, int y) { return x < y ? x : y; }

void gr_palette_load( ubyte *pal )
{
	int i;

	for (i=0; i<768; i++ )
	{
		gr_current_pal[i] = pal[i];
		if (gr_current_pal[i] > 63)
			gr_current_pal[i] = 63;
	}

	gr_palette_step_up(0, 0, 0); // make ogl_setbrightness_internal get run so that menus get brightened too.
	init_computed_colors();
}

void gr_palette_read(ubyte * pal)
{
	int i;
	for (i=0; i<768; i++ )
	{
		pal[i]=gr_current_pal[i];
		if (pal[i] > 63)
			pal[i] = 63;
	}
}

#define GL_BGR_EXT 0x80E0

typedef struct
{
      unsigned char TGAheader[12];
      unsigned char header[6];
} TGA_header;

//writes out an uncompressed RGB .tga file
//if we got really spiffy, we could optionally link in libpng or something, and use that.
void write_bmp(char *savename,int w,int h,unsigned char *buf)
{
	PHYSFS_file* TGAFile;
	TGA_header TGA;
	GLbyte HeightH,HeightL,WidthH,WidthL;
	unsigned int pixel;
	unsigned char *rgbaBuf;

	buf = (unsigned char*)d_calloc(w*h*4,sizeof(unsigned char));

	rgbaBuf = (unsigned char*) d_calloc(w * h * 4, sizeof(unsigned char));
	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgbaBuf);
	for(pixel = 0; pixel < w * h; pixel++) {
		*(buf + pixel * 3) = *(rgbaBuf + pixel * 4 + 2);
		*(buf + pixel * 3 + 1) = *(rgbaBuf + pixel * 4 + 1);
		*(buf + pixel * 3 + 2) = *(rgbaBuf + pixel * 4);
	}
	d_free(rgbaBuf);

	if (!(TGAFile = PHYSFSX_openWriteBuffered(savename)))
	{
		RT_LOG(RT_LOGSERVERITY_HIGH, "Could not create TGA file to dump screenshot!");
		d_free(buf);
		return;
	}

	HeightH = (GLbyte)(h / 256);
	HeightL = (GLbyte)(h % 256);
	WidthH  = (GLbyte)(w / 256);
	WidthL  = (GLbyte)(w % 256);
	// Write TGA Header
	TGA.TGAheader[0] = 0;
	TGA.TGAheader[1] = 0;
	TGA.TGAheader[2] = 2;
	TGA.TGAheader[3] = 0;
	TGA.TGAheader[4] = 0;
	TGA.TGAheader[5] = 0;
	TGA.TGAheader[6] = 0;
	TGA.TGAheader[7] = 0;
	TGA.TGAheader[8] = 0;
	TGA.TGAheader[9] = 0;
	TGA.TGAheader[10] = 0;
	TGA.TGAheader[11] = 0;
	TGA.header[0] = (GLbyte) WidthL;
	TGA.header[1] = (GLbyte) WidthH;
	TGA.header[2] = (GLbyte) HeightL;
	TGA.header[3] = (GLbyte) HeightH;
	TGA.header[4] = (GLbyte) 24;
	TGA.header[5] = 0;
	PHYSFS_write(TGAFile,&TGA,sizeof(TGA_header),1);
	PHYSFS_write(TGAFile,buf,w*h*3*sizeof(unsigned char),1);
	PHYSFS_close(TGAFile);
	d_free(buf);
}

void save_screen_shot(int automap_flag)
{
	static int savenum=0;
	char savename[13+sizeof(SCRNS_DIR)];
	unsigned char *buf;

	if (!GameArg.DbgGlReadPixelsOk){
		if (!automap_flag)
			HUD_init_message_literal(HM_DEFAULT, "glReadPixels not supported on your configuration");
		return;
	}

	stop_time();

	if (!PHYSFSX_exists(SCRNS_DIR,0))
		PHYSFS_mkdir(SCRNS_DIR); //try making directory

	do
	{
		sprintf(savename, "%sscrn%04d.tga",SCRNS_DIR, savenum++);
	} while (PHYSFSX_exists(savename,0));

	if (!automap_flag)
		HUD_init_message(HM_DEFAULT, "%s 'scrn%04d.tga'", TXT_DUMPING_SCREEN, savenum-1 );

#ifndef OGLES
	glReadBuffer(GL_FRONT);
#endif

	buf = d_malloc(grd_curscreen->sc_w*grd_curscreen->sc_h*3);
	write_bmp(savename,grd_curscreen->sc_w,grd_curscreen->sc_h,buf);
	d_free(buf);

	start_time();
}
