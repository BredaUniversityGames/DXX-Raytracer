#include "RText.h"
#include "Window.h"
#include "songs.h"
#include "screens.h"
#include "gr.h"
#include "key.h"
#include "game.h"

//Pauses the game without showing a window
//msg must be null since it doesn't get deleted and it doesn't do anything for this handler.
//TODO, maybe still resume the renderer if that is blocking it.
int RT_PauseHandler(window* wind, d_event* event, char* msg)
{
	int key;
	switch (event->type)
	{
	case EVENT_WINDOW_ACTIVATED:
		game_flush_inputs();
		break;

	case EVENT_KEY_COMMAND:
		key = event_key_get(event);

		switch (key)
		{
		case 0:
			break;
		case KEY_ESC:
			window_close(wind);
			return 1;
		case KEY_PAUSE:
			window_close(wind);
			return 1;
		default:
			break;
		}
		break;

	case EVENT_IDLE:
		timer_delay2(50);
		break;

	case EVENT_WINDOW_CLOSE:
		songs_resume();
		break;

	default:
		break;
	}

	return 0;
}

extern int netplayerinfo_on;

int RT_DoRealPause()
{
#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		netplayerinfo_on = !netplayerinfo_on;
		return(KEY_PAUSE);
	}
#endif

	songs_pause();
	set_screen_mode(SCREEN_MENU);
	window_create(&grd_curscreen->sc_canvas, 0, 0, 0, 0, (int (*)(window*, d_event*, void*))RT_PauseHandler, NULL);
	return 0;
}