// Holds the main init and de-init functions for arch-related program parts

#include <SDL/SDL.h>
#include "songs.h"
#include "key.h"
#include "digi.h"
#include "mouse.h"
#include "joy.h"
#include "gr.h"
#include "text.h"
#include "args.h"
#include "config.h"
#include "logger.h"

void arch_close(void)
{
	songs_uninit();

	gr_close();

	if (!GameArg.CtlNoJoystick)
		joy_close();

	mouse_close();

	if (!GameArg.SndNoSound)
	{
		digi_close();
	}

	key_close();

	SDL_Quit();
}

void arch_init(void)
{
	int t;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		RT_LOGF(RT_LOGSERVERITY_HIGH, "SDL library initialisation failed: %s.", SDL_GetError());

	key_init();

	digi_select_system( GameArg.SndDisableSdlMixer ? SDLAUDIO_SYSTEM : SDLMIXER_SYSTEM );

	if (!GameArg.SndNoSound)
		digi_init();

	mouse_init();

	if (!GameArg.CtlNoJoystick)
		joy_init();

	//The raytracing build requires delayed initialization due to the HWND.
	// TODO: Will throw an error even when initialized.
	if ((t = gr_init(Game_screen_mode)) != 0)
		RT_LOGF(RT_LOGSERVERITY_HIGH, TXT_CANT_INIT_GFX, t);

	atexit(arch_close);
}

