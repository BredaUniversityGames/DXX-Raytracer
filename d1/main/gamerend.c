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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Stuff for rendering the HUD
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "timer.h"
#include "pstypes.h"
#include "console.h"
#include "inferno.h"
#include "gr.h"
#include "palette.h"
#include "bm.h"
#include "player.h"
#include "render.h"
#include "menu.h"
#include "newmenu.h"
#include "screens.h"
#include "fix.h"
#include "robot.h"
#include "game.h"
#include "gauges.h"
#include "gamefont.h"
#include "newdemo.h"
#include "text.h"
#include "multi.h"
#include "endlevel.h"
#include "cntrlcen.h"
#include "fuelcen.h"
#include "powerup.h"
#include "laser.h"
#include "playsave.h"
#include "automap.h"
#include "mission.h"
#include "gameseq.h"
#include "args.h"
#include "globvars.h"

#ifdef OGL
#include "ogl_init.h"
#endif

#ifdef RT_DX12
#include "dx12.h"
#include "GLTFLoader.h"
#include "Core/Arena.h"
#include "Core/MiniMath.h"
#endif

int netplayerinfo_on=0;

#ifdef NETWORK
void game_draw_multi_message()
{

	if ( (Game_mode&GM_MULTI) && (multi_sending_message[Player_num]))	{
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(0,63,0),-1);
		gr_printf(0x8000, (LINE_SPACING*5)+FSPACY(1), "%s: %s_", TXT_MESSAGE, Network_message );
	}

	if ( (Game_mode&GM_MULTI) && (multi_defining_message))	{
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(0,63,0),-1);
		gr_printf(0x8000, (LINE_SPACING*5)+FSPACY(1), "%s #%d: %s_", TXT_MACRO, multi_defining_message, Network_message );
	}
}
#endif

void show_framerate()
{
	static int fps_count = 0, fps_rate = 0;
	int y = GHEIGHT;
	static fix64 fps_time = 0;

	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(BM_XRGB(0,31,0),-1);

	if (PlayerCfg.CockpitMode[1] == CM_FULL_SCREEN) {
		if ((Game_mode & GM_MULTI) || (Newdemo_state == ND_STATE_PLAYBACK && Newdemo_game_mode & GM_MULTI))
			y -= LINE_SPACING * 10;
		else
			y -= LINE_SPACING * 4;
	} else if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR) {
		if ((Game_mode & GM_MULTI) || (Newdemo_state == ND_STATE_PLAYBACK && Newdemo_game_mode & GM_MULTI))
			y -= LINE_SPACING * 6;
		else
			y -= LINE_SPACING * 1;
	} else {
		if ((Game_mode & GM_MULTI) || (Newdemo_state == ND_STATE_PLAYBACK && Newdemo_game_mode & GM_MULTI))
			y -= LINE_SPACING * 7;
		else
			y -= LINE_SPACING * 2;
	}

	fps_count++;
	if (timer_query() >= fps_time + F1_0)
	{
		fps_rate = fps_count;
		fps_count = 0;
		fps_time = timer_query();
	}
	gr_printf(SWIDTH-(GameArg.SysMaxFPS>999?FSPACX(43):FSPACX(37)),y,"FPS: %i",fps_rate);
}

void show_observers() {
	if(Netgame.max_numobservers == 0) {
		return;
	}

	int y = GHEIGHT;

	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(BM_XRGB(8,8,32),-1);

	if (PlayerCfg.CockpitMode[1] == CM_FULL_SCREEN) {
		if ((Game_mode & GM_MULTI) || (Newdemo_state == ND_STATE_PLAYBACK && Newdemo_game_mode & GM_MULTI))
			y -= LINE_SPACING * 10;
		else
			y -= LINE_SPACING * 4;
	} else if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR) {
		if ((Game_mode & GM_MULTI) || (Newdemo_state == ND_STATE_PLAYBACK && Newdemo_game_mode & GM_MULTI))
			y -= LINE_SPACING * 6;
		else
			y -= LINE_SPACING * 1;
	} else {
		if ((Game_mode & GM_MULTI) || (Newdemo_state == ND_STATE_PLAYBACK && Newdemo_game_mode & GM_MULTI))
			y -= LINE_SPACING * 7;
		else
			y -= LINE_SPACING * 2;
	}

	y -= LINE_SPACING*2; 

	for(int i = 0; i < Netgame.numobservers; i++) {
		gr_printf(SWIDTH-FSPACX(strlen(Netgame.observers[i].callsign)*5 + 5),y,"%s",Netgame.observers[i].callsign);
		y -= LINE_SPACING; 
	}

	gr_set_fontcolor(BM_XRGB(8,8,32),-1);
	gr_printf(SWIDTH-FSPACX(37+15),y,"Observers:");
	y -= LINE_SPACING; 	
}

void set_font_present() { gr_set_fontcolor(BM_XRGB(25,25,25),-1); }
void set_font_absent() { gr_set_fontcolor(BM_XRGB(12,12,12),-1); }
void set_font_newline() { gr_set_fontcolor(255,-1); }
void draw_flag(char* string, int present, int x, int y) {
	if(present) { set_font_present(); }
	else        { set_font_absent();  }

	gr_printf(x,y,string); 
}
void set_font_presence(int i) { if(i) set_font_present(); else set_font_absent(); }

#ifdef NETWORK
void show_netplayerinfo()
{
	int x=0, y=0, i=0, color=0, eff=0;
	static const char *const eff_strings[]={"trashing","really hurting","seriously affecting","hurting","affecting","tarnishing"};

	gr_set_current_canvas(NULL);
	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(255,-1);

	x=(SWIDTH/2)-FSPACX(120);
	y=(SHEIGHT/2)-FSPACY(84);

	gr_settransblend(14, GR_BLEND_NORMAL);
	gr_setcolor( BM_XRGB(0,0,0) );
	gr_rect((SWIDTH/2)-FSPACX(120),(SHEIGHT/2)-FSPACY(84),(SWIDTH/2)+FSPACX(120),(SHEIGHT/2)+FSPACY(84));
	gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);

	// general game information
	y+=LINE_SPACING;
	gr_printf(0x8000,y,"%s",Netgame.game_name);
#ifndef SHAREWARE
	y+=LINE_SPACING;
	gr_printf(0x8000,y,"%s - lvl: %i",Netgame.mission_title,Netgame.levelnum);
#endif

	x+=FSPACX(8);
	y+=LINE_SPACING*2;
	unsigned gamemode = Netgame.gamemode;
	gr_printf(x,y,"game mode: %s",gamemode < (sizeof(GMNames) / sizeof(GMNames[0])) ? GMNames[gamemode] : "INVALID");

	
	int base_flags_left = SWIDTH/2 - FSPACX(15);
	int flags_x = base_flags_left + FSPACX(30);
	int letter_spacing = FSPACX(7); 
	int word_spacing = FSPACX(46); 


	if(Netgame.RetroProtocol) {
		draw_flag("RetroP2P", 1,                         						 base_flags_left + word_spacing*0, y); 
	} else if(Netgame.ShortPackets) {
		draw_flag("ShortPkt", 1,                         						 base_flags_left + word_spacing*0, y); 
	} else {
		draw_flag("LongPkt", 1,                         						 base_flags_left + word_spacing*0, y); 
	}

	char pps_string[16];
	sprintf(pps_string, "PPS %d", Netgame.PacketsPerSec); 
	draw_flag(pps_string, 1,                         						 base_flags_left + word_spacing*1, y);

	if(Netgame.SpawnStyle == SPAWN_STYLE_NO_INVUL ) {
		draw_flag("NoInvul", 1,                            base_flags_left + word_spacing*2, y); 
	} else if (Netgame.SpawnStyle == SPAWN_STYLE_SHORT_INVUL ) {
		draw_flag("ShortInv", 1, base_flags_left + word_spacing*2, y); 
	}  else if (Netgame.SpawnStyle == SPAWN_STYLE_LONG_INVUL ) {
		draw_flag("LongInv", 1,                            base_flags_left + word_spacing*2, y); 
	} else {
		draw_flag("Preview", 1,                            base_flags_left + word_spacing*2, y); 
	}
	

	set_font_newline(); 

	y+=LINE_SPACING;
	gr_printf(x,y,"difficulty: %s",MENU_DIFFICULTY_TEXT(Netgame.difficulty));

	draw_flag("ColorLgt", Netgame.AllowColoredLighting,                            base_flags_left + word_spacing*0, y); 
	draw_flag("BrtShips", Netgame.BrightPlayers,                                   base_flags_left + word_spacing*1, y); 	
	draw_flag("ConcResp", Netgame.RespawnConcs,                                    base_flags_left + word_spacing*2, y); 

	set_font_newline(); 
	y+=LINE_SPACING;
	gr_printf(x,y,"level time: %i:%02i:%02i",Players[Player_num].hours_level,f2i(Players[Player_num].time_level) / 60 % 60,f2i(Players[Player_num].time_level) % 60);

	char disp_string[16];
	sprintf(disp_string, "Guns x%d", Netgame.PrimaryDupFactor == 0 ? 1 : Netgame.PrimaryDupFactor);
	draw_flag(disp_string, Netgame.PrimaryDupFactor > 1,                           base_flags_left + word_spacing*0, y); 

	sprintf(disp_string, "Msls x%d", Netgame.SecondaryDupFactor == 0 ? 1 : Netgame.SecondaryDupFactor);
	draw_flag(disp_string, Netgame.SecondaryDupFactor > 1,                         base_flags_left + word_spacing*1, y); 	

	sprintf(disp_string, "Mcap %s", Netgame.SecondaryCapFactor == 0 ? "ALL" : (Netgame.SecondaryCapFactor == 1 ? "6" : "2"));
	draw_flag(disp_string, Netgame.SecondaryCapFactor > 0,                         base_flags_left + word_spacing*2, y); 	


	set_font_newline(); 
	y+=LINE_SPACING;
	gr_printf(x,y,"total time: %i:%02i:%02i",Players[Player_num].hours_total,f2i(Players[Player_num].time_total) / 60 % 60,f2i(Players[Player_num].time_total) % 60);




	set_font_newline(); 
	y+=LINE_SPACING;
	if (Netgame.KillGoal)
		gr_printf(x,y,"Kill goal: %d",Netgame.KillGoal*10);

	gr_printf(base_flags_left, y, "Items: "); 
	draw_flag("L", Netgame.AllowedItems & NETFLAG_DOLASER,     flags_x, y);  flags_x += letter_spacing; 
	draw_flag("Q", Netgame.AllowedItems & NETFLAG_DOQUAD,      flags_x, y);  flags_x += letter_spacing; 
	draw_flag("V", Netgame.AllowedItems & NETFLAG_DOVULCAN,    flags_x, y);  flags_x += letter_spacing; 
	draw_flag("A", Netgame.AllowedItems & NETFLAG_DOVULCANAMMO,flags_x, y);  flags_x += letter_spacing; 
	draw_flag("S", Netgame.AllowedItems & NETFLAG_DOSPREAD,    flags_x, y);  flags_x += letter_spacing; 
	draw_flag("P", Netgame.AllowedItems & NETFLAG_DOPLASMA,    flags_x, y);  flags_x += letter_spacing; 
	draw_flag("F", Netgame.AllowedItems & NETFLAG_DOFUSION,    flags_x, y);  flags_x += letter_spacing; 
	draw_flag("C", 1,                                          flags_x, y);  flags_x += letter_spacing; 
	draw_flag("H", Netgame.AllowedItems & NETFLAG_DOHOMING,    flags_x, y);  flags_x += letter_spacing; 
	draw_flag("P", Netgame.AllowedItems & NETFLAG_DOPROXIM,    flags_x, y);  flags_x += letter_spacing; 
	draw_flag("S", Netgame.AllowedItems & NETFLAG_DOSMART,     flags_x, y);  flags_x += letter_spacing; 
	draw_flag("M", Netgame.AllowedItems & NETFLAG_DOMEGA,      flags_x, y);  flags_x += letter_spacing; 
	draw_flag("C", Netgame.AllowedItems & NETFLAG_DOCLOAK,     flags_x, y);  flags_x += letter_spacing; 
	draw_flag("I", Netgame.AllowedItems & NETFLAG_DOINVUL,     flags_x, y);  flags_x += letter_spacing; 

	// player information (name, kills, ping, game efficiency)
	set_font_newline(); 	
	y+=LINE_SPACING*3;
	gr_printf(x,y,"player");
	if (Game_mode & GM_MULTI_COOP)
		gr_printf(x+FSPACX(8)*7,y,"score");
	else
	{
		gr_printf(x+FSPACX(8)*7,y,"kills");
		gr_printf(x+FSPACX(8)*12,y,"deaths");
	}
	gr_printf(x+FSPACX(8)*18,y,"ping");
	gr_printf(x+FSPACX(8)*23,y,"efficiency");

	if(Netgame.FairColors)
		selected_player_rgb = player_rgb_all_blue; 
	else if(Netgame.BlackAndWhitePyros) 
		selected_player_rgb = player_rgb_alt; 
	else
		selected_player_rgb = player_rgb;

	// process players table
	for (i=0; i<MAX_PLAYERS; i++)
	{
		if (!Players[i].connected)
			continue;

		y+=LINE_SPACING;

		//if (Game_mode & GM_TEAM)
		//	color=get_team(i);
		//else
		//	color=Netgame.players[i].color;//i;
		color = get_color_for_player(i, 0); 
		gr_set_fontcolor( BM_XRGB(selected_player_rgb[color].r,selected_player_rgb[color].g,selected_player_rgb[color].b),-1 );
		gr_printf(x,y,"%s\n",Players[i].callsign);
		if (Game_mode & GM_MULTI_COOP)
			gr_printf(x+FSPACX(8)*7,y,"%-6d",Players[i].score);
		else
		{
			gr_printf(x+FSPACX(8)*7,y,"%-6d",Players[i].net_kills_total);
			gr_printf(x+FSPACX(8)*12,y,"%-6d",Players[i].net_killed_total);
		}

		gr_printf(x+FSPACX(8)*18,y,"%-6d",Netgame.players[i].ping + Netgame.players[Player_num].ping);
		if (i != Player_num)
			gr_printf(x+FSPACX(8)*23,y,"%d/%d",kill_matrix[Player_num][i],kill_matrix[i][Player_num]);
	}

	y+=LINE_SPACING*2+(LINE_SPACING*(MAX_PLAYERS-N_players));

	// printf team scores
	if (Game_mode & GM_TEAM)
	{
		gr_set_fontcolor(255,-1);
		gr_printf(x,y,"team");
		gr_printf(x+FSPACX(8)*8,y,"score");
		y+=LINE_SPACING;
		gr_set_fontcolor(BM_XRGB(selected_player_rgb[0].r,selected_player_rgb[0].g,selected_player_rgb[0].b),-1 );
		gr_printf(x,y,"%s:",Netgame.team_name[0]);
		gr_printf(x+FSPACX(8)*8,y,"%i",team_kills[0]);
		y+=LINE_SPACING;
		gr_set_fontcolor(BM_XRGB(selected_player_rgb[1].r,selected_player_rgb[1].g,selected_player_rgb[1].b),-1 );
		gr_printf(x,y,"%s:",Netgame.team_name[1]);
		gr_printf(x+FSPACX(8)*8,y,"%i",team_kills[1]);
		y+=LINE_SPACING*2;
	}
	else
		y+=LINE_SPACING*4;

	gr_set_fontcolor(255,-1);

	// additional information about game - ranking
	eff=(int)((float)((float)PlayerCfg.NetlifeKills/((float)PlayerCfg.NetlifeKilled+(float)PlayerCfg.NetlifeKills))*100.0);
	if (eff<0)
		eff=0;

	if (!PlayerCfg.NoRankings)
	{
		gr_printf(0x8000,y,"Your lifetime efficiency of %d%% (%d/%d)",eff,PlayerCfg.NetlifeKills,PlayerCfg.NetlifeKilled);
		y+=LINE_SPACING;
		if (eff<60)
			gr_printf(0x8000,y,"is %s your ranking.",eff_strings[eff/10]);
		else
			gr_printf(0x8000,y,"is serving you well.");
		y+=LINE_SPACING;
		gr_printf(0x8000,y,"your rank is: %s",RankStrings[GetMyNetRanking()]);
	}
}
#endif

#ifndef NDEBUG

fix Show_view_text_timer = -1;

void draw_window_label()
{
	if ( Show_view_text_timer > 0 )
	{
		char *viewer_name,*control_name;

		Show_view_text_timer -= FrameTime;

		switch( Viewer->type )
		{
			case OBJ_FIREBALL:	viewer_name = "Fireball"; break;
			case OBJ_ROBOT:		viewer_name = "Robot"; break;
			case OBJ_HOSTAGE:		viewer_name = "Hostage"; break;
			case OBJ_PLAYER:		viewer_name = "Player"; break;
			case OBJ_WEAPON:		viewer_name = "Weapon"; break;
			case OBJ_CAMERA:		viewer_name = "Camera"; break;
			case OBJ_POWERUP:	viewer_name = "Powerup"; break;
			case OBJ_DEBRIS:		viewer_name = "Debris"; break;
			case OBJ_CNTRLCEN:	viewer_name = "Control Center"; break;
			default:					viewer_name = "Unknown"; break;
		}

		switch ( Viewer->control_type) {
			case CT_NONE:			control_name = "Stopped"; break;
			case CT_AI:				control_name = "AI"; break;
			case CT_FLYING:		control_name = "Flying"; break;
			case CT_SLEW:			control_name = "Slew"; break;
			case CT_FLYTHROUGH:	control_name = "Flythrough"; break;
			case CT_MORPH:			control_name = "Morphing"; break;
			default:					control_name = "Unknown"; break;
		}
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(31,0,0),-1);
		gr_printf( 0x8000, (SHEIGHT/10), "%s View - %s",viewer_name,control_name );
	}
}
#endif

void render_countdown_gauge()
{
	if (!Endlevel_sequence && Control_center_destroyed  && (Countdown_seconds_left>-1) && (Countdown_seconds_left<127))	{
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(0,63,0),-1);
		gr_printf(0x8000, (LINE_SPACING*6)+FSPACY(1), "T-%d s", Countdown_seconds_left );
	}
}

void game_draw_hud_stuff()
{
#ifdef CROSSHAIR
	if ( Viewer->type == OBJ_PLAYER )
		laser_do_crosshair(Viewer);
#endif
	
#ifndef NDEBUG
	draw_window_label();
#endif

#ifdef NETWORK
	game_draw_multi_message();
#endif

	if ((Newdemo_state == ND_STATE_PLAYBACK) || (Newdemo_state == ND_STATE_RECORDING)) {
		char message[128];
		int y;

		if (Newdemo_state == ND_STATE_PLAYBACK) {
			if (Newdemo_show_percentage) {
			  	sprintf(message, "%s (%d%% %s)", TXT_DEMO_PLAYBACK, newdemo_get_percent_done(), TXT_DONE);
			} else {
				sprintf (message, " ");
			}
		} else {
			//extern int Newdemo_num_written;
			//sprintf (message, "%s (%dK)", TXT_DEMO_RECORDING, (Newdemo_num_written / 1024));
			sprintf (message, "%s", TXT_DEMO_RECORDING);
		}

		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor( BM_XRGB(27,0,0), -1 );

		y = GHEIGHT-(LINE_SPACING*2);

		if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT)
			y = grd_curcanv->cv_bitmap.bm_h / 1.2 ;
		if (PlayerCfg.CockpitMode[1] != CM_REAR_VIEW)
			gr_string(0x8000, y, message );
	}

	render_countdown_gauge();

	if (GameCfg.FPSIndicator && PlayerCfg.CockpitMode[1] != CM_REAR_VIEW)
		show_framerate();

	if ( (Game_mode & GM_MULTI) && (PlayerCfg.ObsShowObs)) {
		show_observers(); 
	}

	if (Newdemo_state == ND_STATE_PLAYBACK)
		Game_mode = Newdemo_game_mode;

	draw_hud();

	if (Newdemo_state == ND_STATE_PLAYBACK)
		Game_mode = GM_NORMAL | (Game_mode & GM_OBSERVER);

	if ( Player_is_dead )
		player_dead_message();
}

extern int gr_bitblt_dest_step_shift;
extern int gr_bitblt_double;
extern int force_cockpit_redraw;
void update_cockpits();

//render a frame for the game
void game_render_frame_mono(int flip)
{
#ifdef RT_DX12
	if (RT_CheckWindowMinimized())
		return;
#endif

	gr_set_current_canvas(&Screen_3d_window);
	
	render_frame(0);

	update_cockpits();

	if (Newdemo_state == ND_STATE_PLAYBACK)
		Game_mode = Newdemo_game_mode;

	if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT || PlayerCfg.CockpitMode[1] == CM_STATUS_BAR)
		render_gauges();
#ifdef RT_DX12
	if (PlayerCfg.CockpitMode[1] == CM_MODEL_3D)
		render_gauges();
#endif

	if (Newdemo_state == ND_STATE_PLAYBACK)
		Game_mode = GM_NORMAL | (Game_mode & GM_OBSERVER);

	gr_set_current_canvas(&Screen_3d_window);
	game_draw_hud_stuff();

#ifdef NETWORK
	if (netplayerinfo_on && Game_mode & GM_MULTI)
		show_netplayerinfo();
#endif
}

void toggle_cockpit()
{
	int new_mode=CM_FULL_SCREEN;

	if (Rear_view || Player_is_dead)
		return;

	switch (PlayerCfg.CockpitMode[1])
	{
		case CM_FULL_COCKPIT:
			new_mode = CM_STATUS_BAR;
			break;
        case CM_STATUS_BAR:
#ifdef RT_DX12
            new_mode = CM_MODEL_3D;
            break;
        case CM_MODEL_3D:
#endif
            new_mode = CM_FULL_SCREEN;
            break;
		case CM_FULL_SCREEN:
			new_mode = CM_FULL_COCKPIT;
			if(PlayerCfg.DisableCockpit) {
				new_mode = CM_STATUS_BAR; 
			}
			break;
	}

	select_cockpit(new_mode);
	HUD_clear_messages();
	PlayerCfg.CockpitMode[0] = new_mode;
	write_player_file();
}

int last_drawn_cockpit = -1;
extern void ogl_loadbmtexture(grs_bitmap *bm);

// This actually renders the new cockpit onto the screen.
#ifdef OGL
#define UBITMAPM ogl_ubitmapm_cs
#elif RT_DX12
#define UBITMAPM dx12_ubitmapm_cs
#else
#define UBITMAPM(x, y, dw, dh, bm, c, scale) gr_ubitmapm(x, y, bm)
#endif
void update_cockpits()
{
	grs_bitmap *bm;
	PIGGY_PAGE_IN(cockpit_bitmap[PlayerCfg.CockpitMode[1]]);
	bm = &GameBitmaps[cockpit_bitmap[PlayerCfg.CockpitMode[1]].index];

	switch( PlayerCfg.CockpitMode[1] )	{
		case CM_FULL_COCKPIT:
			gr_set_current_canvas(NULL);
			UBITMAPM(0, 0, -1, grd_curcanv->cv_bitmap.bm_h, bm,255, F1_0);
			break;
		case CM_REAR_VIEW:
			gr_set_current_canvas(NULL);
			UBITMAPM(0, 0, -1, grd_curcanv->cv_bitmap.bm_h, bm,255, F1_0);
			break;
		case CM_FULL_SCREEN:
			break;
		case CM_STATUS_BAR:
			gr_set_current_canvas(NULL);
			UBITMAPM(0, (HIRESMODE?(SHEIGHT*2)/2.6:(SHEIGHT*2)/2.72), -1, ((int) ((double) (bm->bm_h) * (HIRESMODE?(double)SHEIGHT/480:(double)SHEIGHT/200) + 0.5)), bm,255, F1_0);
			break;
		case CM_LETTERBOX:
			gr_set_current_canvas(NULL);
			break;
#ifdef RT_DX12
	    case CM_MODEL_3D:
            break;
#endif
	}

	gr_set_current_canvas(NULL);

	if (PlayerCfg.CockpitMode[1] != last_drawn_cockpit)
		last_drawn_cockpit = PlayerCfg.CockpitMode[1];
	else
		return;

	if (PlayerCfg.CockpitMode[1]==CM_FULL_COCKPIT || PlayerCfg.CockpitMode[1]==CM_STATUS_BAR)
		init_gauges();
}

void game_render_frame()
{
	set_screen_mode(SCREEN_GAME);
	play_homing_warning();
	game_render_frame_mono(GameArg.DbgUseDoubleBuffer);
}

//show a message in a nice little box
void show_boxed_message(char *msg, int RenderFlag)
{
	int w,h,aw;
	int x,y;
	
	gr_set_current_canvas(NULL);
	gr_set_curfont( MEDIUM1_FONT );
	gr_set_fontcolor(BM_XRGB(31, 31, 31), -1);
	gr_get_string_size(msg,&w,&h,&aw);
	
	x = (SWIDTH-w)/2;
	y = (SHEIGHT-h)/2;
	
	nm_draw_background(x-BORDERX,y-BORDERY,x+w+BORDERX,y+h+BORDERY);
	
	gr_string( 0x8000, y, msg );
	
	// If we haven't drawn behind it, need to flip
	if (!RenderFlag)
		gr_flip();
}

