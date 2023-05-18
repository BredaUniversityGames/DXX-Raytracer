/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * inferno.c: Entry point of program (main procedure)
 *
 * After main initializes everything, most of the time is spent in the loop
 * while (window_get_front())
 * In this loop, the main menu is brought up first.
 *
 * main() for Inferno
 *
 */

char copyright[] = "DESCENT   COPYRIGHT (C) 1994,1995 PARALLAX SOFTWARE CORPORATION";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <SDL/SDL.h>

#ifdef __unix__
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "pstypes.h"
#include "strutil.h"
#include "console.h"
#include "gr.h"
#include "key.h"
#include "3d.h"
#include "bm.h"
#include "inferno.h"
#include "game.h"
#include "segment.h"		//for Side_to_verts
#include "u_mem.h"
#include "screens.h"
#include "texmerge.h"
#include "menu.h"
#include "digi.h"
#include "palette.h"
#include "args.h"
#include "titles.h"
#include "text.h"
#include "gauges.h"
#include "gamefont.h"
#include "kconfig.h"
#include "newmenu.h"
#include "config.h"
#include "multi.h"
#include "songs.h"
#include "gameseq.h"
#include "playsave.h"
#include "collide.h"
#include "newdemo.h"
#include "mission.h"
#include "joy.h"
#include "../texmap/scanline.h" //for select_tmap -MM
#include "event.h"
#include "rbaudio.h"
#ifndef __LINUX__
#include "messagebox.h"
#include "logger.h"
#endif
#ifdef EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#include "ui.h"
#endif
#include "vers_id.h"
#ifdef USE_UDP
#include "net_udp.h"
#endif

#if defined(RT_DX12)
#include "Core/Arena.h"
#endif

const char g_descent_build_datetime[21] = __DATE__ " " __TIME__;

int Screen_mode=-1;					//game screen or editor screen?
int descent_critical_error = 0;
unsigned int descent_critical_deverror = 0;
unsigned int descent_critical_errcode = 0;

int HiresGFXAvailable = 0;
int MacHog = 0;	// using a Mac hogfile?

extern void arch_init(void);


//read help from a file & print to screen
void print_commandline_help()
{
	printf( "\n System Options:\n\n");
	printf( "  -nonicefps                    Don't free CPU-cycles\n");
	printf( "  -maxfps <n>                   Set maximum framerate to <n>\n\t\t\t\t(default: %i, availble: 1-%i)\n", MAXIMUM_FPS, MAXIMUM_FPS);
	printf( "  -hogdir <s>                   set shared data directory to <s>\n");
	printf( "  -nohogdir                     don't try to use shared data directory\n");
	printf( "  -use_players_dir              put player files and saved games in Players subdirectory\n");
	printf( "  -lowmem                       Lowers animation detail for better performance with\n\t\t\t\tlow memory\n");
	printf( "  -pilot <s>                    Select pilot <s> automatically\n");
	printf( "  -autodemo                     Start in demo mode\n");
	printf( "  -window                       Run the game in a window\n");
	printf( "  -noborders                    Do not show borders in window mode\n");
	printf( "  -notitles                     Skip title screens\n");

	printf( "\n Controls:\n\n");
	printf( "  -nocursor                     Hide mouse cursor\n");
	printf( "  -nomouse                      Deactivate mouse\n");
	printf( "  -nojoystick                   Deactivate joystick\n");
	printf( "  -nostickykeys                 Make CapsLock and NumLock non-sticky\n");

	printf( "\n Sound:\n\n");
	printf( "  -nosound                      Disables sound output\n");
	printf( "  -nomusic                      Disables music output\n");
#ifdef    USE_SDLMIXER
	printf( "  -nosdlmixer                   Disable Sound output via SDL_mixer\n");
#endif // USE SDLMIXER

	printf( "\n Graphics:\n\n");
	printf( "  -lowresfont                   Force to use LowRes fonts\n");
#ifdef    OGL
	printf( "  -gl_fixedfont                 Do not scale fonts to current resolution\n");
#endif // OGL

#if defined(USE_UDP)
	printf( "\n Multiplayer:\n\n");
	printf( "  -udp_hostaddr <s>             Use IP address/Hostname <s> for manual game joining\n\t\t\t\t(default: %s)\n", UDP_MANUAL_ADDR_DEFAULT);
	printf( "  -udp_hostport <n>             Use UDP port <n> for manual game joining (default: %i)\n", UDP_PORT_DEFAULT);
	printf( "  -udp_myport <n>               Set my own UDP port to <n> (default: %i)\n", UDP_PORT_DEFAULT);
#ifdef USE_TRACKER
	printf( "  -tracker_hostaddr <n>         Address of Tracker server to register/query games to/from\n\t\t\t\t(default: %s)\n", TRACKER_ADDR_DEFAULT);
	printf( "  -tracker_hostport <n>         Port of Tracker server to register/query games to/from\n\t\t\t\t(default: %i)\n", TRACKER_PORT_DEFAULT);
#endif // USE_TRACKER
#endif // defined(USE_UDP)

#ifdef    EDITOR
	printf( "\n Editor:\n\n");
	printf( "  -nobm                         Don't load BITMAPS.TBL and BITMAPS.BIN - use internal data\n");
#endif // EDITOR

	printf( "\n Debug (use only if you know what you're doing):\n\n");
	printf( "  -debug                        Enable debugging output.\n");
	printf( "  -verbose                      Enable verbose output.\n");
	printf( "  -safelog                      Write gamelog.txt unbuffered.\n\t\t\t\tUse to keep helpful output to trace program crashes.\n");
	printf( "  -norun                        Bail out after initialization\n");
	printf( "  -renderstats                  Enable renderstats info by default\n");
	printf( "  -text <s>                     Specify alternate .tex file\n");
	printf( "  -tmap <s>                     Select texmapper <s> to use\n\t\t\t\t(default: c, available: c, fp, quad, i386)\n");
	printf( "  -showmeminfo                  Show memory statistics\n");
	printf( "  -nodoublebuffer               Disable Doublebuffering\n");
	printf( "  -bigpig                       Use uncompressed RLE bitmaps\n");
	printf( "  -16bpp                        Use 16Bpp instead of 32Bpp\n");
#ifdef    OGL
	printf( "  -gl_oldtexmerge               Use old texmerge, uses more ram, but might be faster\n");
	printf( "  -gl_intensity4_ok <n>         Override DbgGlIntensity4Ok (default: 1)\n");
	printf( "  -gl_luminance4_alpha4_ok <n>  Override DbgGlLuminance4Alpha4Ok (default: 1)\n");
	printf( "  -gl_rgba2_ok <n>              Override DbgGlRGBA2Ok (default: 1)\n");
	printf( "  -gl_readpixels_ok <n>         Override DbgGlReadPixelsOk (default: 1)\n");
	printf( "  -gl_gettexlevelparam_ok <n>   Override DbgGlGetTexLevelParamOk (default: 1)\n");
#else
	printf( "  -hwsurface                    Use SDL HW Surface\n");
	printf( "  -asyncblit                    Use queued blits over SDL. Can speed up rendering\n");
#endif // OGL

	printf( "\n Help:\n\n");
	printf( "  -help, -h, -?, ?             View this help screen\n");
	printf( "\n\n");
}

int Quitting = 0;

// Default event handler for everything except the editor
int standard_handler(d_event *event)
{
	int key;

	if (Quitting)
	{
		window *wind = window_get_front();
		if (!wind)
			return 0;
	
		if (wind == Game_wind)
		{
			int choice;
			Quitting = 0;
			choice=nm_messagebox( NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME );
			if (choice != 0)
				return 0;
			else
			{
				GameArg.SysAutoDemo = 0;
				Quitting = 1;
			}
		}
		
		// Close front window, let the code flow continue until all windows closed or quit cancelled
		if (!window_close(wind))
			Quitting = 0;
		
		return 1;
	}

	switch (event->type)
	{
		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
			// No window selecting
			// We stay with the current one until it's closed/hidden or another one is made
			// Not the case for the editor
			break;

		case EVENT_KEY_COMMAND:
			key = event_key_get(event);

			switch (key)
			{
#ifdef macintosh
				case KEY_COMMAND + KEY_SHIFTED + KEY_3:
#endif
				case KEY_PRINT_SCREEN:
				{
					gr_set_current_canvas(NULL);
					save_screen_shot(0);
					return 1;
				}

				case KEY_ALTED+KEY_ENTER:
				case KEY_ALTED+KEY_PADENTER:
					if (Game_wind)
						if (Game_wind == window_get_front())
							return 0;
					gr_toggle_fullscreen();
					return 1;

#ifndef NDEBUG
				case KEY_BACKSP:
					Int3();
					return 1;
#endif

#if defined(__APPLE__) || defined(macintosh)
				case KEY_COMMAND+KEY_Q:
					// Alt-F4 already taken, too bad
					Quitting = 1;
					return 1;
#endif
				case KEY_SHIFTED + KEY_ESC:
					con_showup();
					return 1;
			}
			break;

		case EVENT_WINDOW_DRAW:
		case EVENT_IDLE:
			//see if redbook song needs to be restarted
			RBACheckFinishedHook();
			return 1;

		case EVENT_QUIT:
#ifdef EDITOR
			if (SafetyCheck())
#endif
				Quitting = 1;
			return 1;

		default:
			break;
	}

	return 0;
}

jmp_buf LeaveEvents;
#define PROGNAME argv[0]

//	DESCENT by Parallax Software
//		Descent Main

int main(int argc, char* argv[])
{
	mem_init();
#if defined(__LINUX__) || defined(__APPLE__)
	error_init(NULL);
#else
	//error_init(msgbox_error);
	//set_warn_func(msgbox_warning);
#endif
	PHYSFSX_init(argc, argv);
	RT_LOG_INIT(RT_FILTERFLAG_ALL);
	con_init();  // Initialise the console

	setbuf(stdout, NULL); // unbuffered output via printf
#ifndef SHIPPING_BUILD
#ifdef _WIN32 
	freopen("CON", "w", stdout);
	freopen("CON", "w", stderr);
#endif
#endif //SHIPPING_BUILD
	if (GameArg.SysShowCmdHelp) {
		print_commandline_help();

		return(0);
	}

	printf("\nType %s -help' for a list of command-line options.\n\n", PROGNAME);

	PHYSFSX_listSearchPathContent();

	if (!PHYSFSX_checkSupportedArchiveTypes())
		return(0);

	if (!PHYSFSX_contfile_init("descent.hog", 1))
#define DXX_NAME_NUMBER	"1"
#define DXX_HOGFILE_NAMES	"descent.hog"
#if defined(__unix__) && !defined(__APPLE__)
#define DXX_HOGFILE_PROGRAM_DATA_DIRECTORY	\
			      "\t$HOME/.d" DXX_NAME_NUMBER "x-raytracer\n"	\
			      "\t" SHAREPATH "\n"
#else
#define DXX_HOGFILE_PROGRAM_DATA_DIRECTORY	\
				  "\tDirectory containing D" DXX_NAME_NUMBER "X\n"
#endif
#if (defined(__APPLE__) && defined(__MACH__)) || defined(macintosh)
#define DXX_HOGFILE_APPLICATION_BUNDLE	\
				  "\tIn 'Resources' inside the application bundle\n"
#else
#define DXX_HOGFILE_APPLICATION_BUNDLE	""
#endif
#define DXX_MISSING_HOGFILE_ERROR_TEXT	\
		"Could not find a valid hog file (" DXX_HOGFILE_NAMES ")\nPossible locations are:\n"	\
		DXX_HOGFILE_PROGRAM_DATA_DIRECTORY	\
		"\tIn a subdirectory called 'Data'\n"	\
		DXX_HOGFILE_APPLICATION_BUNDLE	\
		"Or use the -hogdir option to specify an alternate location."
		RT_LOG(RT_LOGSERVERITY_ASSERT, DXX_MISSING_HOGFILE_ERROR_TEXT);

	switch (PHYSFSX_fsize("descent.hog"))
	{
	case D1_MAC_SHARE_MISSION_HOGSIZE:
	case D1_MAC_MISSION_HOGSIZE:
		MacHog = 1;	// used for fonts and the Automap
		break;
	}

	load_text();

	//print out the banner title
	RT_LOGF(RT_LOGSERVERITY_INFO, "%s  %s\n", DESCENT_VERSION, g_descent_build_datetime); // D1X version
	RT_LOGF(RT_LOGSERVERITY_INFO, "This is a MODIFIED version of Descent, based on %s.\n", BASED_VERSION);
	RT_LOGF(RT_LOGSERVERITY_INFO, "%s\n%s\n", TXT_COPYRIGHT, TXT_TRADEMARK);
	RT_LOG(RT_LOGSERVERITY_INFO, "Copyright (C) 2005-2011 Christian Beckhaeuser\n\n");

	if (GameArg.DbgVerbose)
		RT_LOGF(RT_LOGSERVERITY_INFO, "%s%s", TXT_VERBOSE_1, "\n");

	ReadConfigFile();

	PHYSFSX_addArchiveContent();

	arch_init();

	select_tmap(GameArg.DbgTexMap);

	RT_LOG(RT_LOGSERVERITY_INFO, "Going into graphics mode...\n");

	gr_set_mode(Game_screen_mode);

	// Load the palette stuff. Returns non-zero if error.
	RT_LOG(RT_LOGSERVERITY_INFO, "Initializing palette system...\n");
	gr_use_palette_table("PALETTE.256");

	RT_LOG(RT_LOGSERVERITY_INFO, "Initializing font system...\n");
	gamefont_init();	// must load after palette data loaded.

	set_default_handler(standard_handler);

#ifndef QUICK_START
	show_titles();
#endif
	set_screen_mode(SCREEN_MENU);

	RT_LOG(RT_LOGSERVERITY_INFO, "\nDoing gamedata_init...");
	gamedata_init();

	if (GameArg.DbgNoRun)
		return(0);

	RT_LOG(RT_LOGSERVERITY_INFO, "\nInitializing texture caching system...");
	texmerge_init(10);		// 10 cache bitmaps

	RT_LOG(RT_LOGSERVERITY_INFO, "\nRunning game...\n");
	init_game();

	Players[Player_num].callsign[0] = '\0';

	key_flush();

	if (GameArg.SysPilot)
	{
		char filename[32] = "";
		int j;

		if (GameArg.SysUsePlayersDir)
			strcpy(filename, "Players/");
		strncat(filename, GameArg.SysPilot, 12);
		filename[8 + 12] = '\0';	// unfortunately strncat doesn't put the terminating 0 on the end if it reaches 'n'
		for (j = GameArg.SysUsePlayersDir ? 8 : 0; filename[j] != '\0'; j++) {
			switch (filename[j]) {
			case ' ':
				filename[j] = '\0';
			}
		}
		if (!strstr(filename, ".plr")) // if player hasn't specified .plr extension in argument, add it
			strcat(filename, ".plr");
		if (PHYSFSX_exists(filename, 0))
		{
			strcpy(strstr(filename, ".plr"), "\0");
			strcpy(Players[Player_num].callsign, GameArg.SysUsePlayersDir ? &filename[8] : filename);
			read_player_file();
			WriteConfigFile();
		}
	}


	Game_mode = GM_GAME_OVER;
	DoMenu();

#ifdef QUICK_START
	select_mission(0, "New Game\n\nSelect mission", do_new_game_menu);
#endif //QUICK_START

	setjmp(LeaveEvents);
	// Send events to windows and the default handler
	while (window_get_front())
		event_process();

	// Tidy up - avoids a crash on exit
	{
		window* wind;

		show_menus();
		while ((wind = window_get_front()))
			window_close(wind);
	}

	WriteConfigFile();
	show_order_form();
	RT_LOG(RT_LOGSERVERITY_INFO, "\nCleanup...\n");
	close_game();
	texmerge_close();
	gamedata_close();
	gamefont_close();
	free_text();
	args_exit();
	newmenu_free_background();
	free_mission();
	PHYSFSX_removeArchiveContent();

	return(0);		//presumably successful exit
}