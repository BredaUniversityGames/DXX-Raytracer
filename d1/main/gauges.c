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
 * Inferno gauge drivers
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "inferno.h"
#include "game.h"
#include "screens.h"
#include "gauges.h"
#include "physics.h"
#include "dxxerror.h"
#include "menu.h"
#include "collide.h"
#include "newdemo.h"
#include "player.h"
#include "gamefont.h"
#include "hostage.h"
#include "bm.h"
#include "text.h"
#include "powerup.h"
#include "sounds.h"
#include "multi.h"
#include "endlevel.h"
#include "wall.h"
#include "text.h"
#include "render.h"
#include "piggy.h"
#include "laser.h"
#include "playsave.h"
#include "rle.h"
#include "byteswap.h"
#include "cntrlcen.h"
#include "logger.h"
#ifdef OGL
#include "ogl_init.h"
#endif
#include "globvars.h"
#include "internal.h"
#include "net_udp.h"

#ifdef RT_DX12
#include "dx12.h"
#include "Core/Arena.h"
#include "grdef.h"
#include "RTgr.h"
#endif

//bitmap numbers for gauges
#define GAUGE_SHIELDS			0		//0..9, in decreasing order (100%,90%...0%)
#define GAUGE_INVULNERABLE		10		//10..19
#define N_INVULNERABLE_FRAMES		10
#define GAUGE_SPEED		   	20		//unused
#define GAUGE_ENERGY_LEFT		21
#define GAUGE_ENERGY_RIGHT		22
#define GAUGE_NUMERICAL			23
#define GAUGE_BLUE_KEY			24
#define GAUGE_GOLD_KEY			25
#define GAUGE_RED_KEY			26
#define GAUGE_BLUE_KEY_OFF		27
#define GAUGE_GOLD_KEY_OFF		28
#define GAUGE_RED_KEY_OFF		29
#define SB_GAUGE_BLUE_KEY		30
#define SB_GAUGE_GOLD_KEY		31
#define SB_GAUGE_RED_KEY		32
#define SB_GAUGE_BLUE_KEY_OFF		33
#define SB_GAUGE_GOLD_KEY_OFF		34
#define SB_GAUGE_RED_KEY_OFF		35
#define SB_GAUGE_ENERGY			36
#define GAUGE_LIVES			37
#define GAUGE_SHIPS			38
#define GAUGE_SHIPS_LAST		45
#define RETICLE_CROSS			46
#define RETICLE_PRIMARY			48
#define RETICLE_SECONDARY		51
#define RETICLE_LAST			55
#define GAUGE_HOMING_WARNING_ON	56
#define GAUGE_HOMING_WARNING_OFF	57
#define SML_RETICLE_CROSS		58
#define SML_RETICLE_PRIMARY		60
#define SML_RETICLE_SECONDARY		63
#define SML_RETICLE_LAST		67
#define KEY_ICON_BLUE			68
#define KEY_ICON_YELLOW			69
#define KEY_ICON_RED			70

//Coordinats for gauges
// cockpit keys

#define GAUGE_BLUE_KEY_X_L		45
#define GAUGE_BLUE_KEY_Y_L		152
#define GAUGE_BLUE_KEY_X_H		91
#define GAUGE_BLUE_KEY_Y_H		374
#define GAUGE_BLUE_KEY_X		(HIRESMODE?GAUGE_BLUE_KEY_X_H:GAUGE_BLUE_KEY_X_L)
#define GAUGE_BLUE_KEY_Y		(HIRESMODE?GAUGE_BLUE_KEY_Y_H:GAUGE_BLUE_KEY_Y_L)

#define GAUGE_GOLD_KEY_X_L		44
#define GAUGE_GOLD_KEY_Y_L		162
#define GAUGE_GOLD_KEY_X_H		89
#define GAUGE_GOLD_KEY_Y_H		395
#define GAUGE_GOLD_KEY_X		(HIRESMODE?GAUGE_GOLD_KEY_X_H:GAUGE_GOLD_KEY_X_L)
#define GAUGE_GOLD_KEY_Y		(HIRESMODE?GAUGE_GOLD_KEY_Y_H:GAUGE_GOLD_KEY_Y_L)

#define GAUGE_RED_KEY_X_L		43
#define GAUGE_RED_KEY_Y_L		172
#define GAUGE_RED_KEY_X_H		87
#define GAUGE_RED_KEY_Y_H		417
#define GAUGE_RED_KEY_X			(HIRESMODE?GAUGE_RED_KEY_X_H:GAUGE_RED_KEY_X_L)
#define GAUGE_RED_KEY_Y			(HIRESMODE?GAUGE_RED_KEY_Y_H:GAUGE_RED_KEY_Y_L)

// status bar keys

#define SB_GAUGE_KEYS_X_L		11
#define SB_GAUGE_KEYS_X_H		26
#define SB_GAUGE_KEYS_X			(HIRESMODE?SB_GAUGE_KEYS_X_H:SB_GAUGE_KEYS_X_L)

#define SB_GAUGE_BLUE_KEY_Y_L		153
#define SB_GAUGE_GOLD_KEY_Y_L		169
#define SB_GAUGE_RED_KEY_Y_L		185

#define SB_GAUGE_BLUE_KEY_Y_H		390
#define SB_GAUGE_GOLD_KEY_Y_H		422
#define SB_GAUGE_RED_KEY_Y_H		454

#define SB_GAUGE_BLUE_KEY_Y		(HIRESMODE?SB_GAUGE_BLUE_KEY_Y_H:SB_GAUGE_BLUE_KEY_Y_L)
#define SB_GAUGE_GOLD_KEY_Y		(HIRESMODE?SB_GAUGE_GOLD_KEY_Y_H:SB_GAUGE_GOLD_KEY_Y_L)
#define SB_GAUGE_RED_KEY_Y		(HIRESMODE?SB_GAUGE_RED_KEY_Y_H:SB_GAUGE_RED_KEY_Y_L)

// cockpit enery gauges

#define LEFT_ENERGY_GAUGE_X_L 	70
#define LEFT_ENERGY_GAUGE_Y_L 	131
#define LEFT_ENERGY_GAUGE_W_L 	64
#define LEFT_ENERGY_GAUGE_H_L 	8

#define LEFT_ENERGY_GAUGE_X_H 	137
#define LEFT_ENERGY_GAUGE_Y_H 	314
#define LEFT_ENERGY_GAUGE_W_H 	133
#define LEFT_ENERGY_GAUGE_H_H 	21

#define LEFT_ENERGY_GAUGE_X 	(HIRESMODE?LEFT_ENERGY_GAUGE_X_H:LEFT_ENERGY_GAUGE_X_L)
#define LEFT_ENERGY_GAUGE_Y 	(HIRESMODE?LEFT_ENERGY_GAUGE_Y_H:LEFT_ENERGY_GAUGE_Y_L)
#define LEFT_ENERGY_GAUGE_W 	(HIRESMODE?LEFT_ENERGY_GAUGE_W_H:LEFT_ENERGY_GAUGE_W_L)
#define LEFT_ENERGY_GAUGE_H 	(HIRESMODE?LEFT_ENERGY_GAUGE_H_H:LEFT_ENERGY_GAUGE_H_L)

#define RIGHT_ENERGY_GAUGE_X 	(HIRESMODE?380:190)
#define RIGHT_ENERGY_GAUGE_Y 	(HIRESMODE?314:131)
#define RIGHT_ENERGY_GAUGE_W 	(HIRESMODE?133:64)
#define RIGHT_ENERGY_GAUGE_H 	(HIRESMODE?21:8)

// sb energy gauge

#define SB_ENERGY_GAUGE_X 		(HIRESMODE?196:98)
#define SB_ENERGY_GAUGE_Y 		(HIRESMODE?390:155)
#define SB_ENERGY_GAUGE_W 		(HIRESMODE?32:16)
#define SB_ENERGY_GAUGE_H 		(HIRESMODE?82:41)

#define SB_ENERGY_NUM_X 		(SB_ENERGY_GAUGE_X+(HIRESMODE?4:2))
#define SB_ENERGY_NUM_Y 		(HIRESMODE?457:190)

#define SHIELD_GAUGE_X 			(HIRESMODE?292:146)
#define SHIELD_GAUGE_Y			(HIRESMODE?374:155)
#define SHIELD_GAUGE_W 			(HIRESMODE?70:35)
#define SHIELD_GAUGE_H			(HIRESMODE?77:32)

#define SHIP_GAUGE_X 			(SHIELD_GAUGE_X+(HIRESMODE?11:5))
#define SHIP_GAUGE_Y			(SHIELD_GAUGE_Y+(HIRESMODE?10:5))

#define SB_SHIELD_GAUGE_X 		(HIRESMODE?247:123)		//139
#define SB_SHIELD_GAUGE_Y 		(HIRESMODE?395:163)

#define SB_SHIP_GAUGE_X 		(SB_SHIELD_GAUGE_X+(HIRESMODE?11:5))
#define SB_SHIP_GAUGE_Y 		(SB_SHIELD_GAUGE_Y+(HIRESMODE?10:5))

#define SB_SHIELD_NUM_X 		(SB_SHIELD_GAUGE_X+(HIRESMODE?21:12))	//151
#define SB_SHIELD_NUM_Y 		(SB_SHIELD_GAUGE_Y-(HIRESMODE?16:7))			//156 -- MWA used to be hard coded to 156

#define NUMERICAL_GAUGE_X		(HIRESMODE?308:154)
#define NUMERICAL_GAUGE_Y		(HIRESMODE?316:130)
#define NUMERICAL_GAUGE_W		(HIRESMODE?38:19)
#define NUMERICAL_GAUGE_H		(HIRESMODE?55:22)

#define PRIMARY_W_PIC_X			(HIRESMODE?135:64)
#define PRIMARY_W_PIC_Y			(HIRESMODE?370:154)
#define PRIMARY_W_TEXT_X		HUD_SCALE_X(HIRESMODE?182:87)
#define PRIMARY_W_TEXT_Y		HUD_SCALE_Y(HIRESMODE?400:157)
#define PRIMARY_AMMO_X			HUD_SCALE_X(HIRESMODE?186:93)
#define PRIMARY_AMMO_Y			HUD_SCALE_Y(HIRESMODE?420:171)

#define SECONDARY_W_PIC_X		(HIRESMODE?405:234)
#define SECONDARY_W_PIC_Y		(HIRESMODE?370:154)
#define SECONDARY_W_TEXT_X		HUD_SCALE_X(HIRESMODE?462:207)
#define SECONDARY_W_TEXT_Y		HUD_SCALE_Y(HIRESMODE?400:157)
#define SECONDARY_AMMO_X		HUD_SCALE_X(HIRESMODE?475:213)
#define SECONDARY_AMMO_Y		HUD_SCALE_Y(HIRESMODE?425:171)

#define SB_LIVES_X			(HIRESMODE?550:266)
#define SB_LIVES_Y			(HIRESMODE?450:185)
#define SB_LIVES_LABEL_X		(HIRESMODE?475:237)
#define SB_LIVES_LABEL_Y		(SB_LIVES_Y+1)

#define SB_SCORE_RIGHT_L		301
#define SB_SCORE_RIGHT_H		605
#define SB_SCORE_RIGHT			(HIRESMODE?SB_SCORE_RIGHT_H:SB_SCORE_RIGHT_L)

#define SB_SCORE_Y			(HIRESMODE?398:158)
#define SB_SCORE_LABEL_X		(HIRESMODE?475:237)

#define SB_SCORE_ADDED_RIGHT		(HIRESMODE?SB_SCORE_RIGHT_H:SB_SCORE_RIGHT_L)
#define SB_SCORE_ADDED_Y		(HIRESMODE?413:165)

#define HOMING_WARNING_X		HUD_SCALE_X(HIRESMODE?13:7)
#define HOMING_WARNING_Y		HUD_SCALE_Y(HIRESMODE?416:171)

#define BOMB_COUNT_X			(HIRESMODE?468:210)
#define BOMB_COUNT_Y			(HIRESMODE?445:186)
#define SB_BOMB_COUNT_X			(HIRESMODE?342:171)
#define SB_BOMB_COUNT_Y			(HIRESMODE?458:191)

// defining box boundries for weapon pictures
#define PRIMARY_W_BOX_LEFT_L		63
#define PRIMARY_W_BOX_TOP_L		151		//154
#define PRIMARY_W_BOX_RIGHT_L		(PRIMARY_W_BOX_LEFT_L+58)
#define PRIMARY_W_BOX_BOT_L		(PRIMARY_W_BOX_TOP_L+42)

#define PRIMARY_W_BOX_LEFT_H		121
#define PRIMARY_W_BOX_TOP_H		364
#define PRIMARY_W_BOX_RIGHT_H		241
#define PRIMARY_W_BOX_BOT_H		(PRIMARY_W_BOX_TOP_H+106)		//470

#define PRIMARY_W_BOX_LEFT		(HIRESMODE?PRIMARY_W_BOX_LEFT_H:PRIMARY_W_BOX_LEFT_L)
#define PRIMARY_W_BOX_TOP		(HIRESMODE?PRIMARY_W_BOX_TOP_H:PRIMARY_W_BOX_TOP_L)
#define PRIMARY_W_BOX_RIGHT		(HIRESMODE?PRIMARY_W_BOX_RIGHT_H:PRIMARY_W_BOX_RIGHT_L)
#define PRIMARY_W_BOX_BOT		(HIRESMODE?PRIMARY_W_BOX_BOT_H:PRIMARY_W_BOX_BOT_L)

#define SECONDARY_W_BOX_LEFT_L		202	//207
#define SECONDARY_W_BOX_TOP_L		151
#define SECONDARY_W_BOX_RIGHT_L		264	//(SECONDARY_W_BOX_LEFT+54)
#define SECONDARY_W_BOX_BOT_L		(SECONDARY_W_BOX_TOP_L+42)

#define SECONDARY_W_BOX_LEFT_H		403
#define SECONDARY_W_BOX_TOP_H		364
#define SECONDARY_W_BOX_RIGHT_H		531
#define SECONDARY_W_BOX_BOT_H		(SECONDARY_W_BOX_TOP_H+106)		//470

#define SECONDARY_W_BOX_LEFT		(HIRESMODE?SECONDARY_W_BOX_LEFT_H:SECONDARY_W_BOX_LEFT_L)
#define SECONDARY_W_BOX_TOP		(HIRESMODE?SECONDARY_W_BOX_TOP_H:SECONDARY_W_BOX_TOP_L)
#define SECONDARY_W_BOX_RIGHT		(HIRESMODE?SECONDARY_W_BOX_RIGHT_H:SECONDARY_W_BOX_RIGHT_L)
#define SECONDARY_W_BOX_BOT		(HIRESMODE?SECONDARY_W_BOX_BOT_H:SECONDARY_W_BOX_BOT_L)

#define SB_PRIMARY_W_BOX_LEFT_L		34		//50
#define SB_PRIMARY_W_BOX_TOP_L		154
#define SB_PRIMARY_W_BOX_RIGHT_L	(SB_PRIMARY_W_BOX_LEFT_L+55)
#define SB_PRIMARY_W_BOX_BOT_L		(195)

#define SB_PRIMARY_W_BOX_LEFT_H		68
#define SB_PRIMARY_W_BOX_TOP_H		381
#define SB_PRIMARY_W_BOX_RIGHT_H	179
#define SB_PRIMARY_W_BOX_BOT_H		473

#define SB_PRIMARY_W_BOX_LEFT		(HIRESMODE?SB_PRIMARY_W_BOX_LEFT_H:SB_PRIMARY_W_BOX_LEFT_L)
#define SB_PRIMARY_W_BOX_TOP		(HIRESMODE?SB_PRIMARY_W_BOX_TOP_H:SB_PRIMARY_W_BOX_TOP_L)
#define SB_PRIMARY_W_BOX_RIGHT		(HIRESMODE?SB_PRIMARY_W_BOX_RIGHT_H:SB_PRIMARY_W_BOX_RIGHT_L)
#define SB_PRIMARY_W_BOX_BOT		(HIRESMODE?SB_PRIMARY_W_BOX_BOT_H:SB_PRIMARY_W_BOX_BOT_L)

#define SB_SECONDARY_W_BOX_LEFT_L	169
#define SB_SECONDARY_W_BOX_TOP_L	154
#define SB_SECONDARY_W_BOX_RIGHT_L	(SB_SECONDARY_W_BOX_LEFT_L+54)
#define SB_SECONDARY_W_BOX_BOT_L	(153+42)

#define SB_SECONDARY_W_BOX_LEFT_H	338
#define SB_SECONDARY_W_BOX_TOP_H	381
#define SB_SECONDARY_W_BOX_RIGHT_H	449
#define SB_SECONDARY_W_BOX_BOT_H	473

#define SB_SECONDARY_W_BOX_LEFT		(HIRESMODE?SB_SECONDARY_W_BOX_LEFT_H:SB_SECONDARY_W_BOX_LEFT_L)	//210
#define SB_SECONDARY_W_BOX_TOP		(HIRESMODE?SB_SECONDARY_W_BOX_TOP_H:SB_SECONDARY_W_BOX_TOP_L)
#define SB_SECONDARY_W_BOX_RIGHT	(HIRESMODE?SB_SECONDARY_W_BOX_RIGHT_H:SB_SECONDARY_W_BOX_RIGHT_L)
#define SB_SECONDARY_W_BOX_BOT		(HIRESMODE?SB_SECONDARY_W_BOX_BOT_H:SB_SECONDARY_W_BOX_BOT_L)

#define SB_PRIMARY_W_PIC_X		(SB_PRIMARY_W_BOX_LEFT+1)	//51
#define SB_PRIMARY_W_PIC_Y		(HIRESMODE?382:154)
#define SB_PRIMARY_W_TEXT_X		HUD_SCALE_X(SB_PRIMARY_W_BOX_LEFT+(HIRESMODE?50:24))	//(51+23)
#define SB_PRIMARY_W_TEXT_Y		HUD_SCALE_Y(HIRESMODE?390:157)
#define SB_PRIMARY_AMMO_X		HUD_SCALE_X(SB_PRIMARY_W_BOX_LEFT+(HIRESMODE?58:30))	//((SB_PRIMARY_W_BOX_LEFT+33)-3)	//(51+32)
#define SB_PRIMARY_AMMO_Y		HUD_SCALE_Y(HIRESMODE?410:171)

#define SB_SECONDARY_W_PIC_X		(HIRESMODE?385:(SB_SECONDARY_W_BOX_LEFT+27))	//(212+27)
#define SB_SECONDARY_W_PIC_Y		(HIRESMODE?382:154)
#define SB_SECONDARY_W_TEXT_X		HUD_SCALE_X(SB_SECONDARY_W_BOX_LEFT+2)	//212
#define SB_SECONDARY_W_TEXT_Y		HUD_SCALE_Y(HIRESMODE?390:157)
#define SB_SECONDARY_AMMO_X		HUD_SCALE_X(SB_SECONDARY_W_BOX_LEFT+(HIRESMODE?(14):11))	//(212+9)
#define SB_SECONDARY_AMMO_Y		HUD_SCALE_Y(HIRESMODE?414:171)

#define WS_SET				0		//in correct state
#define WS_FADING_OUT			1
#define WS_FADING_IN			2
#define FADE_SCALE			(2*i2f(GR_FADE_LEVELS)/REARM_TIME)		// fade out and back in REARM_TIME, in fade levels per seconds (int)
#define MAX_SHOWN_LIVES 4

#define COCKPIT_PRIMARY_BOX		(!HIRESMODE?0:4)
#define COCKPIT_SECONDARY_BOX		(!HIRESMODE?1:5)
#define SB_PRIMARY_BOX			(!HIRESMODE?2:6)
#define SB_SECONDARY_BOX		(!HIRESMODE?3:7)

// Let's define all the UI's UV locations here; they're all in the range from 0 to 1023
#ifdef RT_DX12
const float pos_red_key[4] = { 829, 22, 928, 96 };
const float pos_gold_key[4] = { 720, 21, 815, 96 };
const float pos_blue_key[4] = { 604, 21, 705, 95 };
const float pos_energy_left[4] = { 175, 120, 1002, 220 };
const float pos_energy_right[4] = { 175, 231, 1002, 331 };
const float pos_energy_left_x[2] = { 175, 1002 };
const float pos_energy_right_x[2] = { 1002, 175 };
const float pos_energy_left_y[2] = { 120, 220 };
const float pos_energy_right_y[2] = { 231, 331 };
const int pos_homing_text[2] = { 214, 13 };
const RT_Vec2 pos_ship_center = { 227, 806 };
const float scl_ship = 100;
const float scl_shield = 144;
const float scale_text_homing[2] = { 18, 18 };
const float scale_text_misc[2] = { 8, 16 };
const float pos_shield_text[2] = { 226, 532 };
const float pos_energy_text[2] = { 92, 40 };
const float pos_text_weapon_primary[2] = { 549, 412 };
const float pos_text_weapon_secondary[2] = { 701, 764 };
#endif

// scaling gauges
#define BASE_WIDTH (HIRESMODE? 640 : 320)
#define BASE_HEIGHT	(HIRESMODE? 480 : 200)
#if defined(OGL) || defined(RT_DX12)
#define HUD_SCALE_X(x)		((int) ((double) (x) * ((double)grd_curscreen->sc_w/BASE_WIDTH) + 0.5))
#define HUD_SCALE_Y(y)		((int) ((double) (y) * ((double)grd_curscreen->sc_h/BASE_HEIGHT) + 0.5))
#define HUD_SCALE_X_1024(x)		((int) ((double) (x) * ((double)grd_curscreen->sc_w) + 0.5))
#define HUD_SCALE_Y_1024(y)		((int) ((double) (y) * ((double)grd_curscreen->sc_h) + 0.5))
#define HUD_SCALE_X_AR(x)	(HUD_SCALE_X(100) > HUD_SCALE_Y(100) ? HUD_SCALE_Y(x) : HUD_SCALE_X(x))
#define HUD_SCALE_Y_AR(y)	(HUD_SCALE_Y(100) > HUD_SCALE_X(100) ? HUD_SCALE_X(y) : HUD_SCALE_Y(y))
#else
#define HUD_SCALE_X(x)		(x)
#define HUD_SCALE_Y(y)		(y)
#define HUD_SCALE_X_AR(x)	(x)
#define HUD_SCALE_Y_AR(y)	(y)
#endif

bitmap_index Gauges[MAX_GAUGE_BMS_MAC]; // Array of all gauge bitmaps.
grs_bitmap deccpt;
grs_bitmap *WinBoxOverlay[2] = { NULL, NULL }; // Overlay subbitmaps for both weapon boxes

static int score_display;
static fix score_time;
static int old_weapon[2]		= {-1,-1};
static int invulnerable_frame = 0;
int weapon_box_states[2];
fix weapon_box_fade_values[2];
int	Color_0_31_0 = -1;
extern fix ThisLevelTime;
extern fix Cruise_speed;

typedef struct gauge_box {
	int left,top;
	int right,bot;		//maximal box
} gauge_box;

//first two are primary & secondary
//seconds two are the same for the status bar
gauge_box gauge_boxes[] = {

// primary left/right low res
		{PRIMARY_W_BOX_LEFT_L,PRIMARY_W_BOX_TOP_L,PRIMARY_W_BOX_RIGHT_L,PRIMARY_W_BOX_BOT_L},
		{SECONDARY_W_BOX_LEFT_L,SECONDARY_W_BOX_TOP_L,SECONDARY_W_BOX_RIGHT_L,SECONDARY_W_BOX_BOT_L},

//sb left/right low res
		{SB_PRIMARY_W_BOX_LEFT_L,SB_PRIMARY_W_BOX_TOP_L,SB_PRIMARY_W_BOX_RIGHT_L,SB_PRIMARY_W_BOX_BOT_L},
		{SB_SECONDARY_W_BOX_LEFT_L,SB_SECONDARY_W_BOX_TOP_L,SB_SECONDARY_W_BOX_RIGHT_L,SB_SECONDARY_W_BOX_BOT_L},

// primary left/right hires
		{PRIMARY_W_BOX_LEFT_H,PRIMARY_W_BOX_TOP_H,PRIMARY_W_BOX_RIGHT_H,PRIMARY_W_BOX_BOT_H},
		{SECONDARY_W_BOX_LEFT_H,SECONDARY_W_BOX_TOP_H,SECONDARY_W_BOX_RIGHT_H,SECONDARY_W_BOX_BOT_H},

// sb left/right hires
		{SB_PRIMARY_W_BOX_LEFT_H,SB_PRIMARY_W_BOX_TOP_H,SB_PRIMARY_W_BOX_RIGHT_H,SB_PRIMARY_W_BOX_BOT_H},
		{SB_SECONDARY_W_BOX_LEFT_H,SB_SECONDARY_W_BOX_TOP_H,SB_SECONDARY_W_BOX_RIGHT_H,SB_SECONDARY_W_BOX_BOT_H},
	};

//store delta x values from left of box
span weapon_window_left[] = {
	{71,114},
	{69,116},
	{68,117},
	{66,118},
	{66,119},
	{66,119},
	{65,119},
	{65,119},
	{65,119},
	{65,119},
	{65,119},
	{65,119},
	{65,119},
	{65,119},
	{65,119},
	{64,119},
	{64,119},
	{64,119},
	{64,119},
	{64,119},
	{64,119},
	{64,119},
	{64,119},
	{64,119},
	{63,119},
	{63,118},
	{63,118},
	{63,118},
	{63,118},
	{63,118},
	{63,118},
	{63,118},
	{63,118},
	{63,118},
	{63,118},
	{63,118},
	{63,118},
	{63,117},
	{63,117},
	{64,116},
	{65,115},
	{66,113},
	{68,111},
};


//store delta x values from left of box
span weapon_window_right[] = {
	{208,255},
	{206,257},
	{205,258},
	{204,259},
	{203,260},
	{203,260},
	{203,260},
	{203,260},
	{203,260},
	{203,261},
	{203,261},
	{203,261},
	{203,261},
	{203,261},
	{203,261},
	{203,261},
	{203,261},
	{203,261},
	{203,262},
	{203,262},
	{203,262},
	{203,262},
	{203,262},
	{203,262},
	{203,262},
	{203,262},
	{204,263},
	{204,263},
	{204,263},
	{204,263},
	{204,263},
	{204,263},
	{204,263},
	{204,263},
	{204,263},
	{204,263},
	{204,263},
	{204,263},
	{205,263},
	{206,262},
	{207,261},
	{208,260},
	{211,255},
};


//store delta x values from left of box
span weapon_window_left_hires[] = {
	{141,231},
	{139,234},
	{137,235},
	{136,237},
	{135,238},
	{134,239},
	{133,240},
	{132,240},
	{131,241},
	{131,241},
	{130,242},
	{129,242},
	{129,242},
	{129,243},
	{128,243},
	{128,243},
	{128,243},
	{128,243},
	{128,243},
	{127,243},
	{127,243},
	{127,243},
	{127,243},
	{127,243},
	{127,243},
	{127,243},
	{127,243},
	{127,243},
	{127,243},
	{126,243},
	{126,243},
	{126,243},
	{126,243},
	{126,242},
	{126,242},
	{126,242},
	{126,242},
	{126,242},
	{126,242},
	{125,242},
	{125,242},
	{125,242},
	{125,242},
	{125,242},
	{125,242},
	{125,242},
	{125,242},
	{125,242},
	{125,242},
	{124,242},
	{124,242},
	{124,241},
	{124,241},
	{124,241},
	{124,241},
	{124,241},
	{124,241},
	{124,241},
	{124,241},
	{124,241},
	{123,241},
	{123,241},
	{123,241},
	{123,241},
	{123,241},
	{123,241},
	{123,241},
	{123,241},
	{123,241},
	{123,241},
	{123,241},
	{123,241},
	{122,241},
	{122,241},
	{122,240},
	{122,240},
	{122,240},
	{122,240},
	{122,240},
	{122,240},
	{122,240},
	{122,240},
	{121,240},
	{121,240},
	{121,240},
	{121,240},
	{121,240},
	{121,240},
	{121,240},
	{121,239},
	{121,239},
	{121,239},
	{121,238},
	{121,238},
	{121,238},
	{122,237},
	{122,237},
	{123,236},
	{123,235},
	{124,234},
	{125,233},
	{126,232},
	{126,231},
	{128,230},
	{130,228},
	{131,226},
	{133,223},
};


//store delta x values from left of box
span weapon_window_right_hires[] = {
	{416,509},
	{413,511},
	{412,513},
	{410,514},
	{409,515},
	{408,516},
	{407,517},
	{407,518},
	{406,519},
	{406,519},
	{405,520},
	{405,521},
	{405,521},
	{404,521},
	{404,522},
	{404,522},
	{404,522},
	{404,522},
	{404,522},
	{404,523},
	{404,523},
	{404,523},
	{404,523},
	{404,523},
	{404,523},
	{404,523},
	{404,523},
	{404,523},
	{404,523},
	{404,524},
	{404,524},
	{404,524},
	{404,524},
	{405,524},
	{405,524},
	{405,524},
	{405,524},
	{405,524},
	{405,524},
	{405,525},
	{405,525},
	{405,525},
	{405,525},
	{405,525},
	{405,525},
	{405,525},
	{405,525},
	{405,525},
	{405,525},
	{405,526},
	{405,526},
	{406,526},
	{406,526},
	{406,526},
	{406,526},
	{406,526},
	{406,526},
	{406,526},
	{406,526},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,528},
	{406,528},
	{407,528},
	{407,528},
	{407,528},
	{407,528},
	{407,528},
	{407,528},
	{407,528},
	{407,529},
	{407,529},
	{407,529},
	{407,529},
	{407,529},
	{407,529},
	{407,529},
	{407,529},
	{408,529},
	{408,529},
	{408,529},
	{409,529},
	{409,529},
	{409,529},
	{410,529},
	{410,528},
	{411,527},
	{412,527},
	{413,526},
	{414,525},
	{415,524},
	{416,524},
	{417,522},
	{419,521},
	{422,519},
	{424,518},
};

static inline void hud_bitblt_free (int x, int y, int w, int h, grs_bitmap *bm)
{
#ifdef OGL
	ogl_ubitmapm_cs (x,y,w,h,bm,-1,F1_0);
#elif RT_DX12
	dx12_ubitmapm_cs(x, y, w, h, bm, -1, F1_0);
#else
	gr_ubitmapm(x, y, bm);
#endif
}

static inline void hud_bitblt (int x, int y, grs_bitmap *bm)
{
#ifdef OGL
	ogl_ubitmapm_cs (x,y,HUD_SCALE_X (bm->bm_w),HUD_SCALE_Y (bm->bm_h),bm,-1,F1_0);
#elif RT_DX12
	dx12_ubitmapm_cs(x, y, HUD_SCALE_X(bm->bm_w), HUD_SCALE_Y(bm->bm_h), bm, -1, F1_0);
#else
	gr_ubitmapm(x, y, bm);
#endif
}

void hud_show_score()
{
	char	score_str[20];
	int	w, h, aw;

	if (HUD_toolong)
		return;

	gr_set_curfont( GAME_FONT );

	if ( (Game_mode & GM_MULTI) && !((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_MULTI_ROBOTS)) ) {
		sprintf(score_str, "%s: %5d", TXT_KILLS, Players[Player_num].net_kills_total);
	} else {
		sprintf(score_str, "%s: %5d", TXT_SCORE, Players[Player_num].score);
  	}

	gr_get_string_size(score_str, &w, &h, &aw );

	if (Color_0_31_0 == -1)
		Color_0_31_0 = BM_XRGB(0,31,0);
	gr_set_fontcolor(Color_0_31_0, -1);

	gr_string(grd_curcanv->cv_bitmap.bm_w-w-FSPACX(1), FSPACY(1), score_str);
}

void hud_show_timer_count()
{
#ifdef NETWORK
	char	score_str[20];
	int	w, h, aw,i;
	fix timevar=0;
#endif

	if (HUD_toolong)
		return;

#ifdef NETWORK
	if ((Game_mode & GM_NETWORK) && Netgame.PlayTimeAllowed)
	{
		timevar=i2f (Netgame.PlayTimeAllowed*5*60);
		i=f2i(timevar-ThisLevelTime);
		i++;

		sprintf(score_str, "T - %5d", i);
		gr_get_string_size(score_str, &w, &h, &aw );

		if (Color_0_31_0 == -1)
			Color_0_31_0 = BM_XRGB(0,31,0);

		gr_set_fontcolor(Color_0_31_0, -1);

		if (i>-1 && !Control_center_destroyed)
			gr_string(grd_curcanv->cv_bitmap.bm_w-w-FSPACX(12), LINE_SPACING+FSPACY(1), score_str);
	}
#endif
}

void hud_show_score_added()
{
	int	color;
	int	w, h, aw;
	char	score_str[20];

	if ( (Game_mode & GM_MULTI) && !((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_MULTI_ROBOTS)) )
		return;

	if (score_display == 0)
		return;

	gr_set_curfont( GAME_FONT );

	score_time -= FrameTime;
	if (score_time > 0) {
		color = f2i(score_time * 20) + 12;

		if (color < 10) color = 12;
		if (color > 31) color = 30;

		color = color - (color % 4);

		if (cheats.enabled)
			sprintf(score_str, "%s", TXT_CHEATER);
		else
			sprintf(score_str, "%5d", score_display);

		gr_get_string_size(score_str, &w, &h, &aw );
		gr_set_fontcolor(BM_XRGB(0, color, 0),-1 );
		gr_string(grd_curcanv->cv_bitmap.bm_w-w-FSPACX(1), LINE_SPACING+FSPACY(1), score_str);
	} else {
		score_time = 0;
		score_display = 0;
	}
}

void sb_show_score()
{
	char	score_str[20];
	int x,y;
	int	w, h, aw;

	gr_set_curfont( GAME_FONT );
	gr_set_fontcolor(BM_XRGB(0,20,0),-1 );

	if ( (Game_mode & GM_MULTI) && !((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_MULTI_ROBOTS)) )
		gr_printf(HUD_SCALE_X(SB_SCORE_LABEL_X),HUD_SCALE_Y(SB_SCORE_Y),"%s:", TXT_KILLS);
	else
		gr_printf(HUD_SCALE_X(SB_SCORE_LABEL_X),HUD_SCALE_Y(SB_SCORE_Y),"%s:", TXT_SCORE);

	gr_set_curfont( GAME_FONT );
	if ( (Game_mode & GM_MULTI) && !( (Game_mode & GM_MULTI_COOP) || (Game_mode & GM_MULTI_ROBOTS) ) )
		sprintf(score_str, "%5d", Players[Player_num].net_kills_total);
	else
		sprintf(score_str, "%5d", Players[Player_num].score);
	gr_get_string_size(score_str, &w, &h, &aw );

	x = HUD_SCALE_X(SB_SCORE_RIGHT)-w-FSPACX(1);
	y = HUD_SCALE_Y(SB_SCORE_Y);

	//erase old score
	gr_setcolor(BM_XRGB(0,0,0));
	gr_rect(x,y,HUD_SCALE_X(SB_SCORE_RIGHT),y+LINE_SPACING);

	if ( (Game_mode & GM_MULTI) && !((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_MULTI_ROBOTS)) )
		gr_set_fontcolor(BM_XRGB(0,20,0),-1 );
	else
		gr_set_fontcolor(BM_XRGB(0,31,0),-1 );

	gr_string(x,y,score_str);
}

void sb_show_score_added()
{
	int	color;
	int w, h, aw;
	char	score_str[32];
	static int x;
	static	int last_score_display = -1;

	if ( (Game_mode & GM_MULTI) && !((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_MULTI_ROBOTS)) )
		return;

	if (score_display == 0)
		return;

	gr_set_curfont( GAME_FONT );

	score_time -= FrameTime;
	if (score_time > 0) {
		if (score_display != last_score_display)
			last_score_display = score_display;

		color = f2i(score_time * 20) + 10;

		if (color < 10) color = 10;
		if (color > 31) color = 31;

		if (cheats.enabled)
			sprintf(score_str, "%s", TXT_CHEATER);
		else
			sprintf(score_str, "%5d", score_display);

		gr_get_string_size(score_str, &w, &h, &aw );
		x = HUD_SCALE_X(SB_SCORE_ADDED_RIGHT)-w-FSPACX(1);
		gr_set_fontcolor(BM_XRGB(0, color, 0),-1 );
		gr_string(x, HUD_SCALE_Y(SB_SCORE_ADDED_Y), score_str);
	} else {
		//erase old score
		gr_setcolor(BM_XRGB(0,0,0));
		gr_rect(x,HUD_SCALE_Y(SB_SCORE_ADDED_Y),HUD_SCALE_X(SB_SCORE_ADDED_RIGHT),HUD_SCALE_Y(SB_SCORE_ADDED_Y)+LINE_SPACING);
		score_time = 0;
		score_display = 0;
	}
}

//	-----------------------------------------------------------------------------
void play_homing_warning(void)
{
	fix beep_delay;
	static fix64 Last_warning_beep_time = 0; // Time we last played homing missile warning beep.

	if (Endlevel_sequence || Player_is_dead)
		return;

	if (Players[Player_num].homing_object_dist >= 0) {
		beep_delay = Players[Player_num].homing_object_dist/128;
		if (beep_delay > F1_0)
			beep_delay = F1_0;
		else if (beep_delay < F1_0/8)
			beep_delay = F1_0/8;

		if (GameTime64 - Last_warning_beep_time > beep_delay/2 || Last_warning_beep_time > GameTime64) {
			digi_play_sample( SOUND_HOMING_WARNING, F1_0 );
			Last_warning_beep_time = GameTime64;
		}
	}
}

//	-----------------------------------------------------------------------------
void show_homing_warning(void)
{
	if (Endlevel_sequence)
	{
		PIGGY_PAGE_IN( Gauges[GAUGE_HOMING_WARNING_OFF] );
		hud_bitblt( HOMING_WARNING_X, HOMING_WARNING_Y, &GameBitmaps[Gauges[GAUGE_HOMING_WARNING_OFF].index]);
		return;
	}

	gr_set_current_canvas( NULL );

	if (Players[Player_num].homing_object_dist >= 0)
	{
		if (GameTime64 & 0x4000)
		{
			PIGGY_PAGE_IN(Gauges[GAUGE_HOMING_WARNING_ON]);
			hud_bitblt( HOMING_WARNING_X, HOMING_WARNING_Y, &GameBitmaps[Gauges[GAUGE_HOMING_WARNING_ON].index]);
		}
		else
		{
			PIGGY_PAGE_IN(Gauges[GAUGE_HOMING_WARNING_OFF]);
			hud_bitblt( HOMING_WARNING_X, HOMING_WARNING_Y, &GameBitmaps[Gauges[GAUGE_HOMING_WARNING_OFF].index]);
		}
	}
	else
	{
		PIGGY_PAGE_IN(Gauges[GAUGE_HOMING_WARNING_OFF]);
		hud_bitblt( HOMING_WARNING_X, HOMING_WARNING_Y, &GameBitmaps[Gauges[GAUGE_HOMING_WARNING_OFF].index]);
	}
}

void hud_show_homing_warning(void)
{
	if (Players[Player_num].homing_object_dist >= 0) {
		if (GameTime64 & 0x4000) {
			gr_set_curfont( GAME_FONT );
			gr_set_fontcolor(BM_XRGB(0,31,0),-1 );
			gr_string(0x8000, grd_curcanv->cv_bitmap.bm_h-LINE_SPACING,TXT_LOCK);
		}
	}
}

void hud_show_keys(void)
{
	grs_bitmap *blue,*yellow,*red;
	int y=HUD_SCALE_Y_AR(GameBitmaps[Gauges[GAUGE_LIVES].index].bm_h+2)+FSPACY(1);

	PIGGY_PAGE_IN(Gauges[KEY_ICON_BLUE]);
	PIGGY_PAGE_IN(Gauges[KEY_ICON_YELLOW]);
	PIGGY_PAGE_IN(Gauges[KEY_ICON_RED]);

	blue=&GameBitmaps[Gauges[KEY_ICON_BLUE].index];
	yellow=&GameBitmaps[Gauges[KEY_ICON_YELLOW].index];
	red=&GameBitmaps[Gauges[KEY_ICON_RED].index];

	if (Players[Player_num].flags & PLAYER_FLAGS_BLUE_KEY)
		hud_bitblt_free(FSPACX(2),y,HUD_SCALE_X_AR(blue->bm_w),HUD_SCALE_Y_AR(blue->bm_h),blue);

	if (Players[Player_num].flags & PLAYER_FLAGS_GOLD_KEY)
		hud_bitblt_free(FSPACX(2)+HUD_SCALE_X_AR(blue->bm_w+3),y,HUD_SCALE_X_AR(yellow->bm_w),HUD_SCALE_Y_AR(yellow->bm_h),yellow);

	if (Players[Player_num].flags & PLAYER_FLAGS_RED_KEY)
		hud_bitblt_free(FSPACX(2)+HUD_SCALE_X_AR(blue->bm_w+yellow->bm_w+6),y,HUD_SCALE_X_AR(red->bm_w),HUD_SCALE_Y_AR(red->bm_h),red);
}

void hud_show_energy(void)
{
	if (PlayerCfg.HudMode<2) {
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(BM_XRGB(0,31,0),-1 );
		if (Game_mode & GM_MULTI)
		     gr_printf(FSPACX(1), (grd_curcanv->cv_bitmap.bm_h-(LINE_SPACING*5)),"%s: %i", TXT_ENERGY, f2ir(Players[Player_num].energy));
		else
		     gr_printf(FSPACX(1), (grd_curcanv->cv_bitmap.bm_h-LINE_SPACING),"%s: %i", TXT_ENERGY, f2ir(Players[Player_num].energy));
	}

	if (Newdemo_state==ND_STATE_RECORDING )
		newdemo_record_player_energy(f2ir(Players[Player_num].energy));
}

static inline const char *SECONDARY_WEAPON_NAMES_VERY_SHORT(const unsigned u)
{
	switch(u)
	{
		default:
			Int3();
		case CONCUSSION_INDEX:	return TXT_CONCUSSION;
		case HOMING_INDEX:		return TXT_HOMING;
		case PROXIMITY_INDEX:	return TXT_PROXBOMB;
		case SMART_INDEX:		return TXT_SMART;
		case MEGA_INDEX:		return TXT_MEGA;
	}
}

void show_bomb_count(int x,int y,int bg_color,int always_show,int right_align)
{
	int bomb,count,w=0,h=0,aw=0;
	char txt[5],*t;

	if (!PlayerCfg.BombGauge)
		return;

	bomb = which_bomb();
	count = Players[Player_num].secondary_ammo[bomb];

	count = min(count,99);	//only have room for 2 digits - cheating give 200

	if (always_show && count == 0)		//no bombs, draw nothing on HUD
		return;

	if (count)
		gr_set_fontcolor(gr_find_closest_color(55,0,0),bg_color);
	else
		gr_set_fontcolor(bg_color,bg_color);	//erase by drawing in background color

	snprintf(txt, sizeof(txt), "B:%02d", count);

	while ((t=strchr(txt,'1')) != NULL)
		*t = '\x84';	//convert to wide '1'

	if (right_align)
		gr_get_string_size(txt, &w, &h, &aw );

	gr_string(x-w,y,txt);
}

void hud_show_weapons_mode(int type,int vertical,int x,int y){
	int i,w,h,aw;
	char weapon_str[10];
	if (vertical){
		y=y+(LINE_SPACING*4);
	}
	if (type==0){
		for (i=4;i>=0;i--){
			if (Primary_weapon==i)
				gr_set_fontcolor(BM_XRGB(20,0,0),-1);
			else{
				if (player_has_weapon(i,0) & HAS_WEAPON_FLAG)
					gr_set_fontcolor(BM_XRGB(0,15,0),-1);
				else
					gr_set_fontcolor(BM_XRGB(3,3,3),-1);
			}
			switch(i){
				case 0:
					sprintf(weapon_str,"%c%i",
						(Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)?'Q':'L',
						Players[Player_num].laser_level+1);
					break;
				case 1:
				if (PlayerCfg.CockpitMode[1]==CM_FULL_SCREEN)
					sprintf(weapon_str,"V%i", f2i(Players[Player_num].primary_ammo[1] * VULCAN_AMMO_SCALE));
				else
					sprintf(weapon_str,"V%i", f2i(Players[Player_num].primary_ammo[1] * VULCAN_AMMO_SCALE));
					break;
				case 2:
					sprintf(weapon_str,"S");break;
				case 3:
					sprintf(weapon_str,"P");break;
				case 4:
					sprintf(weapon_str,"F");break;

			}
			gr_get_string_size(weapon_str, &w, &h, &aw );
			if (vertical){
				y-=h+FSPACX(2);
			}else
			     x-=w+FSPACY(3);
			gr_string(x, y, weapon_str);
		}
	} else {
		for (i=4;i>=0;i--){
			if (Secondary_weapon==i)
				gr_set_fontcolor(BM_XRGB(20,0,0),-1);
			else{
				if (Players[Player_num].secondary_ammo[i]>0)
					gr_set_fontcolor(BM_XRGB(0,15,0),-1);
				else
					gr_set_fontcolor(BM_XRGB(0,6,0),-1);
			}
			sprintf(weapon_str,"%i",Players[Player_num].secondary_ammo[i]);
			gr_get_string_size(weapon_str, &w, &h, &aw );
			if (vertical){
				y-=h+FSPACX(2);
			}else
			     x-=w+FSPACY(3);
			gr_string(x, y, weapon_str);
		}
	}
	gr_set_fontcolor(BM_XRGB(0,31,0),-1 );
}

void hud_show_weapons(void)
{
	int	y;

	gr_set_curfont( GAME_FONT );
	gr_set_fontcolor(BM_XRGB(0,31,0),-1 );

	y = grd_curcanv->cv_bitmap.bm_h;

	if (Game_mode & GM_MULTI)
		y -= LINE_SPACING*4;

	if (PlayerCfg.HudMode==1){
		hud_show_weapons_mode(0,0,grd_curcanv->cv_bitmap.bm_w,y-(LINE_SPACING*2));
		hud_show_weapons_mode(1,0,grd_curcanv->cv_bitmap.bm_w,y-LINE_SPACING);
	}
	else if (PlayerCfg.HudMode==2){
		int x1,x2;
		int w, aw;
		gr_get_string_size("V1000", &w, &x1, &aw );
		gr_get_string_size("0 ", &x2, &x1, &aw);
		y=grd_curcanv->cv_bitmap.bm_h/1.75;
		x1=grd_curcanv->cv_bitmap.bm_w/2.1-(FSPACX(40)+w);
		x2=grd_curcanv->cv_bitmap.bm_w/1.9+(FSPACX(42)+x2);
		hud_show_weapons_mode(0,1,x1,y);
		hud_show_weapons_mode(1,1,x2,y);
		gr_set_fontcolor(BM_XRGB(14,14,23),-1 );
		gr_printf(x2, y-(LINE_SPACING*4),"%i", f2ir(Players[Player_num].shields));
		gr_set_fontcolor(BM_XRGB(25,18,6),-1 );
		gr_printf(x1, y-(LINE_SPACING*4),"%i", f2ir(Players[Player_num].energy));
	}
	else
	{
		const char *disp_primary_weapon_name;
		char    weapon_str[32];
		int	w, h, aw;

		switch (Primary_weapon) {
			case 0:
				if (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)
					sprintf(weapon_str, "%s %s %i", TXT_QUAD, TXT_LASER, Players[Player_num].laser_level+1);
				else
					sprintf(weapon_str, "%s %i", TXT_LASER, Players[Player_num].laser_level+1);
				disp_primary_weapon_name = weapon_str;
				break;
			case 1:
				sprintf(weapon_str, "%s: %i", TXT_W_VULCAN_S, f2i(Players[Player_num].primary_ammo[Primary_weapon] * VULCAN_AMMO_SCALE));
				disp_primary_weapon_name = weapon_str;
				break;
			case 2:
				disp_primary_weapon_name = TXT_W_SPREADFIRE_S;
				break;
			case 3:
				disp_primary_weapon_name = TXT_W_PLASMA_S;
				break;
			case 4:
				disp_primary_weapon_name = TXT_W_FUSION_S;
				break;
			default:
				Int3();
				disp_primary_weapon_name = "";
				break;
		}

		gr_get_string_size(disp_primary_weapon_name, &w, &h, &aw );
		gr_string(grd_curcanv->cv_bitmap.bm_w-w-FSPACX(1), y-(LINE_SPACING*2), disp_primary_weapon_name);//originally y-8

		snprintf(weapon_str, sizeof(weapon_str), "%s %d",SECONDARY_WEAPON_NAMES_VERY_SHORT(Secondary_weapon),Players[Player_num].secondary_ammo[Secondary_weapon]);
		gr_get_string_size(weapon_str, &w, &h, &aw );
		gr_string(grd_curcanv->cv_bitmap.bm_w-w-FSPACX(1), y-LINE_SPACING, weapon_str);

		show_bomb_count(grd_curcanv->cv_bitmap.bm_w-FSPACX(1), y-(LINE_SPACING*3),-1,1, 1);
	}

	if (Primary_weapon == VULCAN_INDEX)
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_primary_ammo(Players[Player_num].primary_ammo[Primary_weapon]);

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_secondary_ammo(Players[Player_num].secondary_ammo[Secondary_weapon]);
}

void hud_show_cloak_invuln(void)
{
	gr_set_fontcolor(BM_XRGB(0,31,0),-1 );

	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
		int	y = grd_curcanv->cv_bitmap.bm_h;

		if (Game_mode & GM_MULTI)
			y -= LINE_SPACING*8;
		else
			y -= LINE_SPACING*4;

		if (Players[Player_num].cloak_time+CLOAK_TIME_MAX-GameTime64 > F1_0*3 || GameTime64 & 0x8000)
		{
			gr_printf(FSPACX(1), y, "%s", TXT_CLOAKED);
		}
	}

	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) {
		int	y = grd_curcanv->cv_bitmap.bm_h;

		if (Game_mode & GM_MULTI)
			y -= LINE_SPACING*9;
		else
			y -= LINE_SPACING*5;

		if (Players[Player_num].invulnerable_time+INVULNERABLE_TIME_MAX-GameTime64 > F1_0*4 || GameTime64 & 0x8000)
		{
			gr_printf(FSPACX(1), y, "%s", TXT_INVULNERABLE);
		}
	}

}

void hud_show_shield(void)
{
	if (PlayerCfg.HudMode<2) {
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(BM_XRGB(0,31,0),-1 );
		if ( Players[Player_num].shields >= 0 )	{
			if (Game_mode & GM_MULTI)
				gr_printf(FSPACX(1), (grd_curcanv->cv_bitmap.bm_h-(LINE_SPACING*6)),"%s: %i", TXT_SHIELD, f2ir(Players[Player_num].shields));
			else
				gr_printf(FSPACX(1), (grd_curcanv->cv_bitmap.bm_h-(LINE_SPACING*2)),"%s: %i", TXT_SHIELD, f2ir(Players[Player_num].shields));
		} else {
			if (Game_mode & GM_MULTI)
				gr_printf(FSPACX(1), (grd_curcanv->cv_bitmap.bm_h-(LINE_SPACING*6)),"%s: 0", TXT_SHIELD );
			else
				gr_printf(FSPACX(1), (grd_curcanv->cv_bitmap.bm_h-(LINE_SPACING*2)),"%s: 0", TXT_SHIELD );
		}
	}

	if (Newdemo_state==ND_STATE_RECORDING )
		newdemo_record_player_shields(f2ir(Players[Player_num].shields));
}

//draw the icons for number of lives
void hud_show_lives()
{
	int x;

	if (HUD_toolong)
		return;

	if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT)
		x = HUD_SCALE_X(7);
	else
		x = FSPACX(2);

	if (Game_mode & GM_MULTI) {
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(BM_XRGB(0,31,0),-1 );
		gr_printf(x, FSPACY(1), "%s: %d", TXT_DEATHS, Players[Player_num].net_killed_total);
	}
	else if (Players[Player_num].lives > 1)  {
		grs_bitmap *bm;
		PIGGY_PAGE_IN(Gauges[GAUGE_LIVES]);
		bm=&GameBitmaps[Gauges[GAUGE_LIVES].index];
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(BM_XRGB(0,20,0),-1 );
		hud_bitblt_free(x,FSPACY(1),HUD_SCALE_X_AR(bm->bm_w),HUD_SCALE_Y_AR(bm->bm_h),bm);
		gr_printf(HUD_SCALE_X_AR(bm->bm_w)+x, FSPACY(1), " x %d", Players[Player_num].lives-1);
	}
}

void sb_show_lives()
{
	int x,y;
	grs_bitmap * bm = &GameBitmaps[Gauges[GAUGE_LIVES].index];
	x = SB_LIVES_X;
	y = SB_LIVES_Y;

	gr_set_curfont( GAME_FONT );
	gr_set_fontcolor(BM_XRGB(0,20,0),-1 );
	if (Game_mode & GM_MULTI)
		gr_printf(HUD_SCALE_X(SB_LIVES_LABEL_X),HUD_SCALE_Y(y),"%s:", TXT_DEATHS);
	else
		gr_printf(HUD_SCALE_X(SB_LIVES_LABEL_X),HUD_SCALE_Y(y),"%s:", TXT_LIVES);

	if (Game_mode & GM_MULTI)
	{
		char killed_str[20];
		int w, h, aw;
		static int last_x[4] = {SB_SCORE_RIGHT_L,SB_SCORE_RIGHT_L,SB_SCORE_RIGHT_H,SB_SCORE_RIGHT_H};
		int x;

		sprintf(killed_str, "%5d", Players[Player_num].net_killed_total);
		gr_get_string_size(killed_str, &w, &h, &aw);
		gr_setcolor(BM_XRGB(0,0,0));
		gr_rect(last_x[HIRESMODE], HUD_SCALE_Y(y), HUD_SCALE_X(SB_SCORE_RIGHT), HUD_SCALE_Y(y)+LINE_SPACING);
		gr_set_fontcolor(BM_XRGB(0,20,0),-1);
		x = HUD_SCALE_X(SB_SCORE_RIGHT)-w-FSPACX(1);
		gr_string(x, HUD_SCALE_Y(y), killed_str);
		last_x[HIRESMODE] = x;
		return;
	}

	//erase old icons
	gr_setcolor(BM_XRGB(0,0,0));
	gr_rect(HUD_SCALE_X(x), HUD_SCALE_Y(y), HUD_SCALE_X(SB_SCORE_RIGHT), HUD_SCALE_Y(y+bm->bm_h));

	if (Players[Player_num].lives-1 > 0) {
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(BM_XRGB(0,20,0),-1 );
		PIGGY_PAGE_IN(Gauges[GAUGE_LIVES]);
		hud_bitblt_free(HUD_SCALE_X(x),HUD_SCALE_Y(y),HUD_SCALE_X_AR(bm->bm_w),HUD_SCALE_Y_AR(bm->bm_h),bm);
		gr_printf(HUD_SCALE_X(x)+HUD_SCALE_X_AR(bm->bm_w), HUD_SCALE_Y(y), " x %d", Players[Player_num].lives-1);
	}
}

#ifndef RELEASE

extern int Piggy_bitmap_cache_next;

void show_time()
{
	int secs = f2i(Players[Player_num].time_level) % 60;
	int mins = f2i(Players[Player_num].time_level) / 60;

	gr_set_curfont( GAME_FONT );

	if (Color_0_31_0 == -1)
		Color_0_31_0 = BM_XRGB(0,31,0);
	gr_set_fontcolor(Color_0_31_0, -1 );

	gr_printf(SWIDTH-FSPACX(30),GHEIGHT-(LINE_SPACING*11),"%d:%02d", mins, secs);
}
#endif

#define EXTRA_SHIP_SCORE	50000		//get new ship every this many points

void add_points_to_score(int points)
{
	int prev_score;

	score_time += f1_0*2;
	score_display += points;
	if (score_time > f1_0*4) score_time = f1_0*4;

	if (points == 0 || cheats.enabled)
		return;

	if ((Game_mode & GM_MULTI) && !( (Game_mode & GM_MULTI_COOP) || (Game_mode & GM_MULTI_ROBOTS) ))
		return;

	prev_score=Players[Player_num].score;

	Players[Player_num].score += points;

#ifndef SHAREWARE
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_player_score(points);
#endif

#ifndef SHAREWARE
#ifdef NETWORK
	if ((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_MULTI_ROBOTS))
		multi_send_score();
#endif
#endif

	if (Game_mode & GM_MULTI)
		return;

	if (Players[Player_num].score/EXTRA_SHIP_SCORE != prev_score/EXTRA_SHIP_SCORE) {
		int snd;
		Players[Player_num].lives += Players[Player_num].score/EXTRA_SHIP_SCORE - prev_score/EXTRA_SHIP_SCORE;
		powerup_basic(20, 20, 20, 0, TXT_EXTRA_LIFE);
		if ((snd=Powerup_info[POW_EXTRA_LIFE].hit_sound) > -1 )
			digi_play_sample( snd, F1_0 );
	}
}

void add_bonus_points_to_score(int points)
{
	int prev_score;

	if (points == 0 || cheats.enabled)
		return;

	prev_score=Players[Player_num].score;

	Players[Player_num].score += points;


#ifndef SHAREWARE
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_player_score(points);
#endif

	if (Game_mode & GM_MULTI)
		return;

	if (Players[Player_num].score/EXTRA_SHIP_SCORE != prev_score/EXTRA_SHIP_SCORE) {
		int snd;
		Players[Player_num].lives += Players[Player_num].score/EXTRA_SHIP_SCORE - prev_score/EXTRA_SHIP_SCORE;
		if ((snd=Powerup_info[POW_EXTRA_LIFE].hit_sound) > -1 )
			digi_play_sample( snd, F1_0 );
	}
}

// Decode cockpit bitmap to deccpt and add alpha fields to weapon boxes (as it should have always been) so we later can render sub bitmaps over the window canvases
void cockpit_decode_alpha(grs_bitmap *bm)
{

	int i=0,x=0,y=0;
	static unsigned char *cur=NULL;
	static short cur_w=0, cur_h=0;
	static unsigned char cockpitbuf[1024*1024];

	// check if we processed this bitmap already
	if (cur==bm->bm_data && cur_w == bm->bm_w && cur_h == bm->bm_h)
		return;

	memset(cockpitbuf,0,1024*1024);

	// decode the bitmap
	if (bm->bm_flags & BM_FLAG_RLE){
		unsigned char * dbits;
		unsigned char * sbits;
		int i, data_offset;

		data_offset = 1;
		if (bm->bm_flags & BM_FLAG_RLE_BIG)
			data_offset = 2;

		sbits = &bm->bm_data[4 + (bm->bm_h * data_offset)];
		dbits = cockpitbuf;

		for (i=0; i < bm->bm_h; i++ )    {
			gr_rle_decode(sbits,dbits);
			if ( bm->bm_flags & BM_FLAG_RLE_BIG )
				sbits += (int)INTEL_SHORT(*((short *)&(bm->bm_data[4+(i*data_offset)])));
			else
				sbits += (int)bm->bm_data[4+i];
			dbits += bm->bm_w;
		}
	}
	else
	{
		memcpy(&cockpitbuf, bm->bm_data, sizeof(unsigned char)*(bm->bm_w*bm->bm_h));
	}

	// add alpha color to the pixels which are inside the window box spans
	for (y=0;y<bm->bm_h;y++)
	{
		for (x=0;x<bm->bm_w;x++)
		{
			if (y >= (HIRESMODE?364:151) && y <= (HIRESMODE?469:193) && ((x >= WinBoxLeft[y-(HIRESMODE?364:151)].l && x <= WinBoxLeft[y-(HIRESMODE?364:151)].r) ||  (x >=WinBoxRight[y-(HIRESMODE?364:151)].l && x <= WinBoxRight[y-(HIRESMODE?364:151)].r)))
				cockpitbuf[i]=TRANSPARENCY_COLOR;
			i++;
		}
	}
#ifdef OGL
	ogl_freebmtexture(bm);
#endif
	gr_init_bitmap (&deccpt, 0, 0, 0, bm->bm_w, bm->bm_h, bm->bm_w, cockpitbuf);
	gr_set_transparent(&deccpt,1);
#ifdef OGL
	ogl_ubitmapm_cs (0, 0, -1, -1, &deccpt, 255, F1_0); // render one time to init the texture
#elif RT_DX12
	dx12_ubitmapm_cs(0, 0, -1, -1, &deccpt, 255, F1_0);
#endif
	if (WinBoxOverlay[0] != NULL)
		gr_free_sub_bitmap(WinBoxOverlay[0]);
	if (WinBoxOverlay[1] != NULL)
		gr_free_sub_bitmap(WinBoxOverlay[1]);
	WinBoxOverlay[0] = gr_create_sub_bitmap(&deccpt,(PRIMARY_W_BOX_LEFT)-2,(PRIMARY_W_BOX_TOP)-2,(PRIMARY_W_BOX_RIGHT-PRIMARY_W_BOX_LEFT+4),(PRIMARY_W_BOX_BOT-PRIMARY_W_BOX_TOP+4));
	WinBoxOverlay[1] = gr_create_sub_bitmap(&deccpt,(SECONDARY_W_BOX_LEFT)-2,(SECONDARY_W_BOX_TOP)-2,(SECONDARY_W_BOX_RIGHT-SECONDARY_W_BOX_LEFT)+4,(SECONDARY_W_BOX_BOT-SECONDARY_W_BOX_TOP)+4);

	cur = bm->bm_data;
	cur_w = bm->bm_w;
	cur_h = bm->bm_h;
}

void draw_wbu_overlay()
{
	grs_bitmap * bm = &GameBitmaps[cockpit_bitmap[PlayerCfg.CockpitMode[1]].index];

	PIGGY_PAGE_IN(cockpit_bitmap[PlayerCfg.CockpitMode[1]]);
	cockpit_decode_alpha(bm);

	if (WinBoxOverlay[0] != NULL)
		hud_bitblt(HUD_SCALE_X(PRIMARY_W_BOX_LEFT-2),HUD_SCALE_Y(PRIMARY_W_BOX_TOP-2),WinBoxOverlay[0]);
	if (WinBoxOverlay[1] != NULL)
		hud_bitblt(HUD_SCALE_X(SECONDARY_W_BOX_LEFT-2),HUD_SCALE_Y(SECONDARY_W_BOX_TOP-2),WinBoxOverlay[1]);
}

void close_gauges()
{
	if (WinBoxOverlay[0] != NULL)
		gr_free_sub_bitmap(WinBoxOverlay[0]);
	if (WinBoxOverlay[1] != NULL)
		gr_free_sub_bitmap(WinBoxOverlay[1]);
	WinBoxOverlay[0] = NULL;
	WinBoxOverlay[1] = NULL;
}

void init_gauges()
{
	old_weapon[0] = old_weapon[1] = -1;
}

void draw_energy_bar(int energy)
{
	int x1, x2, y;
	int not_energy = (HIRESMODE?(HUD_SCALE_X(125 - (energy*125)/100)):(HUD_SCALE_X(63 - (energy*63)/100)));
	double aplitscale=((double)(HUD_SCALE_X(65)/HUD_SCALE_Y(8))/(65/8)); //scale aplitude of energy bar to current resolution aspect

	// Draw left energy bar
	PIGGY_PAGE_IN(Gauges[GAUGE_ENERGY_LEFT]);
	hud_bitblt (HUD_SCALE_X(LEFT_ENERGY_GAUGE_X), HUD_SCALE_Y(LEFT_ENERGY_GAUGE_Y), &GameBitmaps[Gauges[GAUGE_ENERGY_LEFT].index]);

	gr_setcolor(BM_XRGB(0,0,0));

	if (energy < 100)
		for (y=0; y < HUD_SCALE_Y(LEFT_ENERGY_GAUGE_H); y++) {
			x1 = HUD_SCALE_X(LEFT_ENERGY_GAUGE_H - 2) - y*(aplitscale);
			x2 = HUD_SCALE_X(LEFT_ENERGY_GAUGE_H - 2) - y*(aplitscale) + not_energy;

			if (x2 > HUD_SCALE_X(LEFT_ENERGY_GAUGE_W) - (y*aplitscale)/3)
				x2 = HUD_SCALE_X(LEFT_ENERGY_GAUGE_W) - (y*aplitscale)/3;

			if (x2 > x1) gr_uline( i2f(x1+HUD_SCALE_X(LEFT_ENERGY_GAUGE_X)), i2f(y+HUD_SCALE_Y(LEFT_ENERGY_GAUGE_Y)), i2f(x2+HUD_SCALE_X(LEFT_ENERGY_GAUGE_X)), i2f(y+HUD_SCALE_Y(LEFT_ENERGY_GAUGE_Y)) );
		}

	gr_set_current_canvas( NULL );

	// Draw right energy bar
	PIGGY_PAGE_IN(Gauges[GAUGE_ENERGY_RIGHT]);
	hud_bitblt (HUD_SCALE_X(RIGHT_ENERGY_GAUGE_X), HUD_SCALE_Y(RIGHT_ENERGY_GAUGE_Y), &GameBitmaps[Gauges[GAUGE_ENERGY_RIGHT].index]);

	if (energy < 100)
		for (y=0; y < HUD_SCALE_Y(RIGHT_ENERGY_GAUGE_H); y++) {
			x1 = HUD_SCALE_X(RIGHT_ENERGY_GAUGE_W - RIGHT_ENERGY_GAUGE_H + 2 ) + y*(aplitscale) - not_energy;
			x2 = HUD_SCALE_X(RIGHT_ENERGY_GAUGE_W - RIGHT_ENERGY_GAUGE_H + 2 ) + y*(aplitscale);

			if (x1 < (y*aplitscale)/3)
				x1 = (y*aplitscale)/3;

			if (x2 > x1) gr_uline( i2f(x1+HUD_SCALE_X(RIGHT_ENERGY_GAUGE_X)), i2f(y+HUD_SCALE_Y(RIGHT_ENERGY_GAUGE_Y)), i2f(x2+HUD_SCALE_X(RIGHT_ENERGY_GAUGE_X)), i2f(y+HUD_SCALE_Y(RIGHT_ENERGY_GAUGE_Y)) );
		}

	gr_set_current_canvas( NULL );
}

void draw_shield_bar(int shield)
{
	int bm_num = shield>=100?9:(shield / 10);

	PIGGY_PAGE_IN(Gauges[GAUGE_SHIELDS+9-bm_num]);
	hud_bitblt (HUD_SCALE_X(SHIELD_GAUGE_X), HUD_SCALE_Y(SHIELD_GAUGE_Y), &GameBitmaps[Gauges[GAUGE_SHIELDS+9-bm_num].index]);
}

#define CLOAK_FADE_WAIT_TIME  0x400

void draw_player_ship(int cloak_state,int x, int y)
{
	static fix cloak_fade_timer=0;
	static int cloak_fade_value=GR_FADE_LEVELS-1;
	grs_bitmap *bm = NULL;

#ifdef NETWORK
	if (Game_mode & GM_TEAM)
	{
		PIGGY_PAGE_IN(Gauges[GAUGE_SHIPS+get_team(Player_num)]);
		bm = &GameBitmaps[Gauges[GAUGE_SHIPS+get_team(Player_num)].index];
	}
	else
#endif
	{
		int color = Netgame.players[Player_num].color; 
		
		PIGGY_PAGE_IN(Gauges[GAUGE_SHIPS+color]);
		bm = &GameBitmaps[Gauges[GAUGE_SHIPS+color].index];
	}

	// for(int i = 0; i < 8; i++) {
	//	RT_LOGF(RT_LOGSERVERITY_MEDIUM, "Player ship %d %d\n", i, Gauges[GAUGE_SHIPS+i].index);
	// }

	if (cloak_state)
	{
		static int step = 0;

		if (GameTime64-Players[Player_num].cloak_time < F1_0)
		{
			step = -2;
		}
		else if (Players[Player_num].cloak_time+CLOAK_TIME_MAX-GameTime64 <= F1_0*3)
		{
			if (cloak_fade_value >= (GR_FADE_LEVELS-1))
			{
				step = -2;
			}
			else if (cloak_fade_value <= 0)
			{
				step = 2;
			}
		}
		else
		{
			step = 0;
			cloak_fade_value = 0;
		}

		cloak_fade_timer -= FrameTime;

		while (cloak_fade_timer < 0)
		{
			cloak_fade_timer += CLOAK_FADE_WAIT_TIME;
			cloak_fade_value += step;
		}

		if (cloak_fade_value > (GR_FADE_LEVELS-1))
			cloak_fade_value = (GR_FADE_LEVELS-1);
		if (cloak_fade_value <= 0)
			cloak_fade_value = 0;
	}
	else
	{
		cloak_fade_timer = 0;
		cloak_fade_value = GR_FADE_LEVELS-1;
	}

	gr_set_current_canvas(NULL);
	hud_bitblt( HUD_SCALE_X(x), HUD_SCALE_Y(y), bm);
	gr_settransblend(cloak_fade_value, GR_BLEND_NORMAL);
	gr_rect(HUD_SCALE_X(x-3), HUD_SCALE_Y(y-3), HUD_SCALE_X(x+bm->bm_w+3), HUD_SCALE_Y(y+bm->bm_h+3));
	gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);
	gr_set_current_canvas( NULL );
}

#define INV_FRAME_TIME	(f1_0/10)		//how long for each frame

void draw_numerical_display(int shield, int energy)
{
	int sw,sh,saw,ew,eh,eaw;

	gr_set_curfont( GAME_FONT );
#ifndef OGL
	PIGGY_PAGE_IN(Gauges[GAUGE_NUMERICAL]);
	hud_bitblt (HUD_SCALE_X(NUMERICAL_GAUGE_X), HUD_SCALE_Y(NUMERICAL_GAUGE_Y), &GameBitmaps[Gauges[GAUGE_NUMERICAL].index]);
#endif
	// cockpit is not 100% geometric so we need to divide shield and energy X position by 1.951 which should be most accurate
	// gr_get_string_size is used so we can get the numbers finally in the correct position with sw and ew
	gr_set_fontcolor(BM_XRGB(14,14,23),-1 );
	gr_get_string_size((shield>199)?"200":(shield>99)?"100":(shield>9)?"00":"0",&sw,&sh,&saw);
	gr_printf(	(grd_curscreen->sc_w/1.951)-(sw/2),
			(grd_curscreen->sc_h/1.365),"%d",shield);

	gr_set_fontcolor(BM_XRGB(25,18,6),-1 );
	gr_get_string_size((energy>199)?"200":(energy>99)?"100":(energy>9)?"00":"0",&ew,&eh,&eaw);
	gr_printf(	(grd_curscreen->sc_w/1.951)-(ew/2),
			(grd_curscreen->sc_h/1.5),"%d",energy);

	gr_set_current_canvas( NULL );
}


void draw_keys()
{
	gr_set_current_canvas( NULL );

	if (Players[Player_num].flags & PLAYER_FLAGS_BLUE_KEY )	{
		PIGGY_PAGE_IN(Gauges[GAUGE_BLUE_KEY]);
                hud_bitblt( HUD_SCALE_X(GAUGE_BLUE_KEY_X), HUD_SCALE_Y(GAUGE_BLUE_KEY_Y), &GameBitmaps[Gauges[GAUGE_BLUE_KEY].index]);
	} else {
		PIGGY_PAGE_IN(Gauges[GAUGE_BLUE_KEY_OFF]);
		hud_bitblt( HUD_SCALE_X(GAUGE_BLUE_KEY_X), HUD_SCALE_Y(GAUGE_BLUE_KEY_Y), &GameBitmaps[Gauges[GAUGE_BLUE_KEY_OFF].index]);
	}

	if (Players[Player_num].flags & PLAYER_FLAGS_GOLD_KEY)	{
		PIGGY_PAGE_IN(Gauges[GAUGE_GOLD_KEY]);
		hud_bitblt( HUD_SCALE_X(GAUGE_GOLD_KEY_X), HUD_SCALE_Y(GAUGE_GOLD_KEY_Y), &GameBitmaps[Gauges[GAUGE_GOLD_KEY].index]);
	} else {
		PIGGY_PAGE_IN(Gauges[GAUGE_GOLD_KEY_OFF]);
		hud_bitblt( HUD_SCALE_X(GAUGE_GOLD_KEY_X), HUD_SCALE_Y(GAUGE_GOLD_KEY_Y), &GameBitmaps[Gauges[GAUGE_GOLD_KEY_OFF].index]);
	}

	if (Players[Player_num].flags & PLAYER_FLAGS_RED_KEY)	{
		PIGGY_PAGE_IN( Gauges[GAUGE_RED_KEY] );
		hud_bitblt( HUD_SCALE_X(GAUGE_RED_KEY_X), HUD_SCALE_Y(GAUGE_RED_KEY_Y), &GameBitmaps[Gauges[GAUGE_RED_KEY].index]);
	} else {
		PIGGY_PAGE_IN(Gauges[GAUGE_RED_KEY_OFF]);
		hud_bitblt( HUD_SCALE_X(GAUGE_RED_KEY_X), HUD_SCALE_Y(GAUGE_RED_KEY_Y), &GameBitmaps[Gauges[GAUGE_RED_KEY_OFF].index]);
	}
}

#ifdef RT_DX12
void render_ui_bitmap(bitmap_index bitmap, float x1, float y1, float x2, float y2) {
	grs_bitmap* bm = &GameBitmaps[bitmap.index];
	dx12_ubitmapm_cs(x1, y1, (x2 - x1), (y2 - y1), bm, -1, F1_0);
}
#endif

void draw_weapon_info_sub(int info_index,gauge_box *box,int pic_x,int pic_y,char *name,int text_x,int text_y)
{
	grs_bitmap *bm;

	//clear the window
	if (PlayerCfg.CockpitMode[1] != CM_MODEL_3D) {
		gr_setcolor(BM_XRGB(0, 0, 0));
		gr_rect(HUD_SCALE_X(box->left), HUD_SCALE_Y(box->top), HUD_SCALE_X(box->right), HUD_SCALE_Y(box->bot + 1));
	}

	bm=&GameBitmaps[Weapon_info[info_index].picture.index];
	Assert(bm != NULL);

	PIGGY_PAGE_IN( Weapon_info[info_index].picture );
	if (PlayerCfg.CockpitMode[1] == CM_MODEL_3D) {
#ifdef RT_DX12
		render_ui_bitmap(Weapon_info[info_index].picture, pic_x, pic_y, pic_x + bm->bm_w * 6, pic_y + bm->bm_h * 6);
#endif
	}
	else
        hud_bitblt(HUD_SCALE_X(pic_x), HUD_SCALE_Y(pic_y), bm);

	if (PlayerCfg.HudMode == 0)
	{
		gr_set_fontcolor(BM_XRGB(0,20,0),-1 );

		gr_string(text_x,text_y,name);

		//	For laser, show level and quadness
		if (info_index == LASER_INDEX)
		{
			gr_printf(text_x,text_y+LINE_SPACING, "%s: %i", TXT_LVL, Players[Player_num].laser_level+1);
			if (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)
				gr_string(text_x,text_y+(LINE_SPACING*2), TXT_QUAD);
		}
	}
}

void draw_weapon_info(int weapon_type,int weapon_num)
{
	int x,y;

#ifdef SHAREWARE
	if (Newdemo_state==ND_STATE_RECORDING )
		newdemo_record_player_weapon(weapon_type, weapon_num);
#endif

	if (weapon_type == 0)
	{
		if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR)
		{
			draw_weapon_info_sub(Primary_weapon_to_weapon_info[weapon_num],&gauge_boxes[SB_PRIMARY_BOX],SB_PRIMARY_W_PIC_X,SB_PRIMARY_W_PIC_Y,PRIMARY_WEAPON_NAMES_SHORT(weapon_num),SB_PRIMARY_W_TEXT_X,SB_PRIMARY_W_TEXT_Y);
			x=SB_PRIMARY_AMMO_X;
			y=SB_PRIMARY_AMMO_Y;
		}
#ifdef RT_DX12
		else if (PlayerCfg.CockpitMode[1] == CM_MODEL_3D) {
			int temp = grd_curcanv->cv_fade_level;
			int fade_value = f2i(weapon_box_fade_values[0]);
			if (weapon_box_states[0] == WS_SET)
				gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);
			else
				gr_settransblend(fade_value, GR_BLEND_NORMAL);
			draw_weapon_info_sub(Primary_weapon_to_weapon_info[weapon_num], &gauge_boxes[COCKPIT_PRIMARY_BOX], 404, 405, PRIMARY_WEAPON_NAMES_SHORT(weapon_num), pos_text_weapon_primary[0], pos_text_weapon_primary[1]);
			x = pos_text_weapon_primary[0] + 36;
			y = pos_text_weapon_primary[1] + 75;
			gr_settransblend(temp, GR_BLEND_NORMAL);
		}
#endif
		else
		{
			draw_weapon_info_sub(Primary_weapon_to_weapon_info[weapon_num],&gauge_boxes[COCKPIT_PRIMARY_BOX],PRIMARY_W_PIC_X,PRIMARY_W_PIC_Y, PRIMARY_WEAPON_NAMES_SHORT(weapon_num),PRIMARY_W_TEXT_X,PRIMARY_W_TEXT_Y);
			x=PRIMARY_AMMO_X;
			y=PRIMARY_AMMO_Y;
		}
	}
	else
	{
		if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR)
		{
			draw_weapon_info_sub(Secondary_weapon_to_weapon_info[weapon_num],&gauge_boxes[SB_SECONDARY_BOX],SB_SECONDARY_W_PIC_X,SB_SECONDARY_W_PIC_Y,SECONDARY_WEAPON_NAMES_SHORT(weapon_num),SB_SECONDARY_W_TEXT_X,SB_SECONDARY_W_TEXT_Y);
			x=SB_SECONDARY_AMMO_X;
			y=SB_SECONDARY_AMMO_Y;
		}
#ifdef RT_DX12
		else if (PlayerCfg.CockpitMode[1] == CM_MODEL_3D) {
			int temp = grd_curcanv->cv_fade_level;
			int fade_value = f2i(weapon_box_fade_values[1]);
			if (weapon_box_states[1] == WS_SET)
			    gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);
			else
			    gr_settransblend(fade_value, GR_BLEND_NORMAL);
			draw_weapon_info_sub(Secondary_weapon_to_weapon_info[weapon_num], &gauge_boxes[SB_SECONDARY_BOX], 849, 747, SECONDARY_WEAPON_NAMES_SHORT(weapon_num), pos_text_weapon_secondary[0], pos_text_weapon_secondary[1]);
			x = pos_text_weapon_secondary[0] + 36;
			y = pos_text_weapon_secondary[1] + 75;
			gr_settransblend(temp, GR_BLEND_NORMAL);
		}
#endif
		else
		{
			draw_weapon_info_sub(Secondary_weapon_to_weapon_info[weapon_num],&gauge_boxes[COCKPIT_SECONDARY_BOX],SECONDARY_W_PIC_X,SECONDARY_W_PIC_Y,SECONDARY_WEAPON_NAMES_SHORT(weapon_num),SECONDARY_W_TEXT_X,SECONDARY_W_TEXT_Y);
			x=SECONDARY_AMMO_X;
			y=SECONDARY_AMMO_Y;
		}
	}

	if (PlayerCfg.HudMode!=0)
		hud_show_weapons_mode(weapon_type,1,x,y);
}

void draw_ammo_info(int x,int y,int ammo_count,int primary)
{
	if (PlayerCfg.HudMode!=0)
		hud_show_weapons_mode(!primary,1,x,y);
	else
	{
		gr_setcolor(BM_XRGB(0,0,0));
		gr_set_fontcolor(BM_XRGB(20,0,0),-1 );
		gr_printf(x,y,"%03d",ammo_count);
	}
}

void draw_primary_ammo_info(int ammo_count)
{
	if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR)
		draw_ammo_info(SB_PRIMARY_AMMO_X,SB_PRIMARY_AMMO_Y,ammo_count,1);
	else if (PlayerCfg.CockpitMode[1] == CM_MODEL_3D)
		draw_ammo_info(549 + 36, 412 + 75, ammo_count, 1);
	else
		draw_ammo_info(PRIMARY_AMMO_X,PRIMARY_AMMO_Y,ammo_count,1);
}

void draw_secondary_ammo_info(int ammo_count)
{
	if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR)
		draw_ammo_info(SB_SECONDARY_AMMO_X,SB_SECONDARY_AMMO_Y,ammo_count,0);
	else if (PlayerCfg.CockpitMode[1] == CM_MODEL_3D)
		draw_ammo_info(701 + 36, 764 + 75, ammo_count, 0);
	else
		draw_ammo_info(SECONDARY_AMMO_X,SECONDARY_AMMO_Y,ammo_count,0);
}

void draw_weapon_box_hud_3d_cockpit(int weapon_type, int weapon_num)
{
	if (weapon_num != old_weapon[weapon_type] && weapon_box_states[weapon_type] == WS_SET && (old_weapon[weapon_type] != -1) && !PlayerCfg.HudMode)
	{
		weapon_box_states[weapon_type] = WS_FADING_OUT;
		weapon_box_fade_values[weapon_type] = i2f(GR_FADE_LEVELS - 1);
	}

	if (old_weapon[weapon_type] == -1)
	{
		//draw_weapon_info(weapon_type, weapon_num);
		old_weapon[weapon_type] = weapon_num;
		weapon_box_states[weapon_type] = WS_SET;
	}

	if (weapon_box_states[weapon_type] == WS_FADING_OUT) {
		//draw_weapon_info(weapon_type, old_weapon[weapon_type]);
		weapon_box_fade_values[weapon_type] -= FrameTime * FADE_SCALE;
		if (weapon_box_fade_values[weapon_type] <= 0)
		{
			weapon_box_states[weapon_type] = WS_FADING_IN;
			old_weapon[weapon_type] = weapon_num;
			old_weapon[weapon_type] = weapon_num;
			weapon_box_fade_values[weapon_type] = 0;
		}
	}
	else if (weapon_box_states[weapon_type] == WS_FADING_IN)
	{
		if (weapon_num != old_weapon[weapon_type])
		{
			weapon_box_states[weapon_type] = WS_FADING_OUT;
		}
		else
		{
			//draw_weapon_info(weapon_type, weapon_num);
			weapon_box_fade_values[weapon_type] += FrameTime * FADE_SCALE;
			if (weapon_box_fade_values[weapon_type] >= i2f(GR_FADE_LEVELS - 1)) {
				weapon_box_states[weapon_type] = WS_SET;
				old_weapon[weapon_type] = -1;
			}
		}
	}
	else
	{
		draw_weapon_info(weapon_type, weapon_num);
		old_weapon[weapon_type] = weapon_num;
	}
}

void draw_weapon_box(int weapon_type,int weapon_num)
{
	gr_set_current_canvas(NULL);
	gr_set_curfont(GAME_FONT);


	if (weapon_num != old_weapon[weapon_type] && weapon_box_states[weapon_type] == WS_SET && (old_weapon[weapon_type] != -1) && !PlayerCfg.HudMode)
	{
		weapon_box_states[weapon_type] = WS_FADING_OUT;
		weapon_box_fade_values[weapon_type] = i2f(GR_FADE_LEVELS - 1);
	}

	if (old_weapon[weapon_type] == -1)
	{
		draw_weapon_info(weapon_type, weapon_num);
		old_weapon[weapon_type] = weapon_num;
		weapon_box_states[weapon_type] = WS_SET;
	}

	if (weapon_box_states[weapon_type] == WS_FADING_OUT) {
		draw_weapon_info(weapon_type, old_weapon[weapon_type]);
		weapon_box_fade_values[weapon_type] -= FrameTime * FADE_SCALE;
		if (weapon_box_fade_values[weapon_type] <= 0)
		{
			weapon_box_states[weapon_type] = WS_FADING_IN;
			old_weapon[weapon_type] = weapon_num;
			old_weapon[weapon_type] = weapon_num;
			weapon_box_fade_values[weapon_type] = 0;
		}
	}
	else if (weapon_box_states[weapon_type] == WS_FADING_IN)
	{
		if (weapon_num != old_weapon[weapon_type])
		{
			weapon_box_states[weapon_type] = WS_FADING_OUT;
		}
		else
		{
			draw_weapon_info(weapon_type, weapon_num);
			weapon_box_fade_values[weapon_type] += FrameTime * FADE_SCALE;
			if (weapon_box_fade_values[weapon_type] >= i2f(GR_FADE_LEVELS - 1)) {
				weapon_box_states[weapon_type] = WS_SET;
				old_weapon[weapon_type] = -1;
			}
		}
	}
	else
	{
		draw_weapon_info(weapon_type, weapon_num);
		old_weapon[weapon_type] = weapon_num;
	}


	if (weapon_box_states[weapon_type] != WS_SET)         //fade gauge
	{
		int fade_value = f2i(weapon_box_fade_values[weapon_type]);
		int boxofs = (PlayerCfg.CockpitMode[1]==CM_STATUS_BAR)?SB_PRIMARY_BOX:COCKPIT_PRIMARY_BOX;

		gr_settransblend(fade_value, GR_BLEND_NORMAL);
		if (PlayerCfg.CockpitMode[1] != CM_MODEL_3D)
		    gr_rect(HUD_SCALE_X(gauge_boxes[boxofs+weapon_type].left),HUD_SCALE_Y(gauge_boxes[boxofs+weapon_type].top),HUD_SCALE_X(gauge_boxes[boxofs+weapon_type].right),HUD_SCALE_Y(gauge_boxes[boxofs+weapon_type].bot));

		gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);
	}

	gr_set_current_canvas(NULL);

}

void draw_weapon_boxes()
{
	draw_weapon_box(0,Primary_weapon);

	if (weapon_box_states[0] == WS_SET)
		if (Primary_weapon == VULCAN_INDEX)
		{
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_primary_ammo(Players[Player_num].primary_ammo[Primary_weapon]);
			draw_primary_ammo_info(f2i(VULCAN_AMMO_SCALE * Players[Player_num].primary_ammo[Primary_weapon]));
		}

	draw_weapon_box(1,Secondary_weapon);

	if (weapon_box_states[1] == WS_SET)
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_secondary_ammo(Players[Player_num].secondary_ammo[Secondary_weapon]);

	draw_secondary_ammo_info(Players[Player_num].secondary_ammo[Secondary_weapon]);

	if(PlayerCfg.HudMode!=0)
	{
		draw_primary_ammo_info(f2i(VULCAN_AMMO_SCALE * Players[Player_num].primary_ammo[Primary_weapon]));
		draw_secondary_ammo_info(Players[Player_num].secondary_ammo[Secondary_weapon]);
	}
}

void sb_draw_energy_bar(int energy)
{
	int erase_height,i;
	int ew, eh, eaw;

	PIGGY_PAGE_IN(Gauges[SB_GAUGE_ENERGY]);
	hud_bitblt( HUD_SCALE_X(SB_ENERGY_GAUGE_X), HUD_SCALE_Y(SB_ENERGY_GAUGE_Y), &GameBitmaps[Gauges[SB_GAUGE_ENERGY].index]);

	erase_height = HUD_SCALE_Y((100 - energy) * SB_ENERGY_GAUGE_H / 100);
	gr_setcolor( 0 );
	for (i=0;i<erase_height;i++)
		gr_uline( i2f(HUD_SCALE_X(SB_ENERGY_GAUGE_X-1)), i2f(HUD_SCALE_Y(SB_ENERGY_GAUGE_Y)+i), i2f(HUD_SCALE_X(SB_ENERGY_GAUGE_X+(SB_ENERGY_GAUGE_W))), i2f(HUD_SCALE_Y(SB_ENERGY_GAUGE_Y)+i) );

	gr_set_current_canvas( NULL );

	//draw numbers
	gr_set_fontcolor(BM_XRGB(25,18,6),-1 );
	gr_get_string_size((energy>199)?"200":(energy>99)?"100":(energy>9)?"00":"0",&ew,&eh,&eaw);
	gr_printf((grd_curscreen->sc_w/3)-(ew/2),HUD_SCALE_Y(SB_ENERGY_NUM_Y),"%d",energy);
}

void sb_draw_shield_num(int shield)
{
	grs_bitmap *bm = &GameBitmaps[cockpit_bitmap[PlayerCfg.CockpitMode[1]].index];
	int sw, sh, saw;

	//draw numbers
	gr_set_curfont( GAME_FONT );
	gr_set_fontcolor(BM_XRGB(14,14,23),-1 );

	//erase old one
	PIGGY_PAGE_IN( cockpit_bitmap[PlayerCfg.CockpitMode[1]] );
	gr_setcolor(gr_gpixel(bm,SB_SHIELD_NUM_X,SB_SHIELD_NUM_Y-(SM_H(Game_screen_mode)-bm->bm_h)));
	gr_get_string_size((shield>199)?"200":(shield>99)?"100":(shield>9)?"00":"0",&sw,&sh,&saw);
	gr_printf((grd_curscreen->sc_w/2.267)-(sw/2),HUD_SCALE_Y(SB_SHIELD_NUM_Y),"%d",shield);
}

void sb_draw_shield_bar(int shield)
{
	int bm_num = shield>=100?9:(shield / 10);

	gr_set_current_canvas(NULL);

	PIGGY_PAGE_IN( Gauges[GAUGE_SHIELDS+9-bm_num] );
	hud_bitblt( HUD_SCALE_X(SB_SHIELD_GAUGE_X), HUD_SCALE_Y(SB_SHIELD_GAUGE_Y), &GameBitmaps[Gauges[GAUGE_SHIELDS+9-bm_num].index]);
}

void sb_draw_keys()
{
	grs_bitmap * bm;
	int flags = Players[Player_num].flags;

	gr_set_current_canvas(NULL);
	bm = &GameBitmaps[Gauges[(flags&PLAYER_FLAGS_BLUE_KEY)?SB_GAUGE_BLUE_KEY:SB_GAUGE_BLUE_KEY_OFF].index];
	PIGGY_PAGE_IN(Gauges[(flags&PLAYER_FLAGS_BLUE_KEY)?SB_GAUGE_BLUE_KEY:SB_GAUGE_BLUE_KEY_OFF]);
	hud_bitblt( HUD_SCALE_X(SB_GAUGE_KEYS_X), HUD_SCALE_Y(SB_GAUGE_BLUE_KEY_Y), bm);
	bm = &GameBitmaps[Gauges[(flags&PLAYER_FLAGS_GOLD_KEY)?SB_GAUGE_GOLD_KEY:SB_GAUGE_GOLD_KEY_OFF].index];
	PIGGY_PAGE_IN(Gauges[(flags&PLAYER_FLAGS_GOLD_KEY)?SB_GAUGE_GOLD_KEY:SB_GAUGE_GOLD_KEY_OFF]);
	hud_bitblt( HUD_SCALE_X(SB_GAUGE_KEYS_X), HUD_SCALE_Y(SB_GAUGE_GOLD_KEY_Y), bm);
	bm = &GameBitmaps[Gauges[(flags&PLAYER_FLAGS_RED_KEY)?SB_GAUGE_RED_KEY:SB_GAUGE_RED_KEY_OFF].index];
	PIGGY_PAGE_IN(Gauges[(flags&PLAYER_FLAGS_RED_KEY)?SB_GAUGE_RED_KEY:SB_GAUGE_RED_KEY_OFF]);
	hud_bitblt( HUD_SCALE_X(SB_GAUGE_KEYS_X), HUD_SCALE_Y(SB_GAUGE_RED_KEY_Y), bm);
}

//	Draws invulnerable ship, or maybe the flashing ship, depending on invulnerability time left.
void draw_invulnerable_ship()
{
	static fix time=0;

	gr_set_current_canvas(NULL);

	if (Players[Player_num].invulnerable_time+INVULNERABLE_TIME_MAX-GameTime64 > F1_0*4 || GameTime64 & 0x8000)
	{

		if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR)	{
			PIGGY_PAGE_IN(Gauges[GAUGE_INVULNERABLE+invulnerable_frame]);
			hud_bitblt( HUD_SCALE_X(SB_SHIELD_GAUGE_X), HUD_SCALE_Y(SB_SHIELD_GAUGE_Y), &GameBitmaps[Gauges[GAUGE_INVULNERABLE+invulnerable_frame].index]);
		} else {
			PIGGY_PAGE_IN(Gauges[GAUGE_INVULNERABLE+invulnerable_frame]);
			hud_bitblt( HUD_SCALE_X(SHIELD_GAUGE_X), HUD_SCALE_Y(SHIELD_GAUGE_Y), &GameBitmaps[Gauges[GAUGE_INVULNERABLE+invulnerable_frame].index]);
		}

		time += FrameTime;

		while (time > INV_FRAME_TIME) {
			time -= INV_FRAME_TIME;
			if (++invulnerable_frame == N_INVULNERABLE_FRAMES)
				invulnerable_frame=0;
		}
	} else if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR)
		sb_draw_shield_bar(f2ir(Players[Player_num].shields));
	else
		draw_shield_bar(f2ir(Players[Player_num].shields));
}

extern int Missile_gun;
extern int allowed_to_fire_laser(void);
extern int allowed_to_fire_missile(void);

// CED -- shipcolor fix
const rgb player_rgb[] = {	//{0,0,0}, // black
							//{4,4,12}, // indigo
							{15,15,23},    // 0x7878B8 ---> blue
							//{23, 23, 23}, // white							
							{27,0,0},      // 0xD80000 ---> red
							{0,23,0},      // 0x00B800 ---> green
							{30,11,31},    // 0xF058F8 ---> PINK
							{31,16,0},     // 0xF88000 ---> orange
							//{0,0,0}, // black
							//{23, 23, 23}, // white
							{24,17,6},     // 0xC08830 ---> light orange (PROBLEM)
							{14,21,12},    // 0x70A860 ---> light green (PROBLEM)
			                {29,29,0}, };  // 0xE8E800 ---> YELLOW

const rgb player_rgb_alt[] = {
	
							{15,15,23},    // 0x7878B8 ---> blue
							{27,0,0},      // 0xD80000 ---> red
							{0,23,0},      // 0x00B800 ---> green
							{30,11,31},    // 0xF058F8 ---> PINK
							{31,16,0},     // 0xF88000 ---> orange
							//{4,4,12}, 		// indigo													
							{12,4,20}, 		// purple
							{23, 23, 23}, 	// white
							{29,29,0}, };  // 0xE8E800 ---> YELLOW		

const rgb player_rgb_all_blue[] = {
	
							{15,15,23},    // 0x7878B8 ---> blue
							{15,15,23},     
							{15,15,23},    
							{15,15,23},   
							{15,15,23},     												
							{15,15,23}, 	
							{15,15,23}, 	
							{15,15,23}, };  							                

const rgb* selected_player_rgb;

typedef struct {
	sbyte x, y;
} xy;

//offsets for reticle parts: high-big  high-sml  low-big  low-sml
static const xy cross_offsets[4] = 		{ {-8,-5},	{-4,-2},	{-4,-2}, {-2,-1} };
static const xy primary_offsets[4] = 	{ {-30,14}, {-16,6},	{-15,6}, {-8, 2} };
static const xy secondary_offsets[4] =	{ {-24,2},	{-12,0}, {-12,1}, {-6,-2} };

//draw the reticle
void show_reticle(int reticle_type, int secondary_display)
{
	int x,y,size;
	int laser_ready,missile_ready,laser_ammo,missile_ammo;
	int cross_bm_num,primary_bm_num,secondary_bm_num;
	int use_hires_reticle,ofs;

	x = grd_curcanv->cv_bitmap.bm_w/2;
#ifdef RT_DX12
	y = grd_curcanv->cv_bitmap.bm_h/2 - grd_curcanv->cv_bitmap.bm_h * (RT_RaytraceGetVerticalOffset());
#else
	y = grd_curcanv->cv_bitmap.bm_h/2;
#endif
	size = (grd_curcanv->cv_bitmap.bm_h / (32-(PlayerCfg.ReticleSize*4)));

	laser_ready = allowed_to_fire_laser();
	missile_ready = allowed_to_fire_missile();

	laser_ammo = player_has_weapon(Primary_weapon,0);
	missile_ammo = player_has_weapon(Secondary_weapon,1);

	primary_bm_num = (laser_ready && laser_ammo==HAS_ALL);
	secondary_bm_num = (missile_ready && missile_ammo==HAS_ALL);

	if (primary_bm_num && Primary_weapon==LASER_INDEX && (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
		primary_bm_num++;

	if (Secondary_weapon!=CONCUSSION_INDEX && Secondary_weapon!=HOMING_INDEX)
		secondary_bm_num += 3;
	else if (secondary_bm_num && !(Missile_gun&1))
			secondary_bm_num++;

	cross_bm_num = ((primary_bm_num > 0) || (secondary_bm_num > 0));

	Assert(primary_bm_num <= 2);
	Assert(secondary_bm_num <= 4);
	Assert(cross_bm_num <= 1);

	gr_setcolor(BM_XRGB(PlayerCfg.ReticleRGBA[0],PlayerCfg.ReticleRGBA[1],PlayerCfg.ReticleRGBA[2]));
	gr_settransblend(PlayerCfg.ReticleRGBA[3], GR_BLEND_NORMAL);

	switch (reticle_type)
	{
		case RET_TYPE_CLASSIC:
		{
			grs_bitmap *cross, *primary, *secondary;

			use_hires_reticle = (HIRESMODE != 0);
			ofs = (use_hires_reticle?0:2);

			PIGGY_PAGE_IN(Gauges[RETICLE_CROSS + cross_bm_num]);
			cross = &GameBitmaps[Gauges[RETICLE_CROSS + cross_bm_num].index];
			hud_bitblt_free(x+HUD_SCALE_X_AR(cross_offsets[ofs].x),y+HUD_SCALE_Y_AR(cross_offsets[ofs].y), HUD_SCALE_X_AR(cross->bm_w), HUD_SCALE_Y_AR(cross->bm_h), cross);

			PIGGY_PAGE_IN(Gauges[RETICLE_PRIMARY + primary_bm_num]);
			primary = &GameBitmaps[Gauges[RETICLE_PRIMARY + primary_bm_num].index];
			hud_bitblt_free(x+HUD_SCALE_X_AR(primary_offsets[ofs].x),y+HUD_SCALE_Y_AR(primary_offsets[ofs].y), HUD_SCALE_X_AR(primary->bm_w), HUD_SCALE_Y_AR(primary->bm_h), primary);

			PIGGY_PAGE_IN(Gauges[RETICLE_SECONDARY + secondary_bm_num]);
			secondary = &GameBitmaps[Gauges[RETICLE_SECONDARY + secondary_bm_num].index];
			hud_bitblt_free(x+HUD_SCALE_X_AR(secondary_offsets[ofs].x),y+HUD_SCALE_Y_AR(secondary_offsets[ofs].y), HUD_SCALE_X_AR(secondary->bm_w), HUD_SCALE_Y_AR(secondary->bm_h), secondary);
			break;
		}
		case RET_TYPE_CLASSIC_REBOOT:
		{
#ifdef OGL
			ogl_draw_vertex_reticle(cross_bm_num,primary_bm_num,secondary_bm_num,BM_XRGB(PlayerCfg.ReticleRGBA[0],PlayerCfg.ReticleRGBA[1],PlayerCfg.ReticleRGBA[2]),PlayerCfg.ReticleRGBA[3],PlayerCfg.ReticleSize);
#endif
			break;

		}
		case RET_TYPE_X:
			gr_uline(i2f(x-(size/2)), i2f(y-(size/2)), i2f(x-(size/5)), i2f(y-(size/5))); // top-left
			gr_uline(i2f(x+(size/2)), i2f(y-(size/2)), i2f(x+(size/5)), i2f(y-(size/5))); // top-right
			gr_uline(i2f(x-(size/2)), i2f(y+(size/2)), i2f(x-(size/5)), i2f(y+(size/5))); // bottom-left
			gr_uline(i2f(x+(size/2)), i2f(y+(size/2)), i2f(x+(size/5)), i2f(y+(size/5))); // bottom-right
			if (secondary_display && secondary_bm_num == 1)
				gr_uline(i2f(x-(size/2)-(size/5)), i2f(y-(size/2)), i2f(x-(size/5)-(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 2)
				gr_uline(i2f(x+(size/2)+(size/5)), i2f(y-(size/2)), i2f(x+(size/5)+(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 4)
				gr_uline(i2f(x+(size/2)), i2f(y+(size/2)+(size/5)), i2f(x+(size/5)), i2f(y+(size/5)+(size/5)));
			break;
		case RET_TYPE_DOT:
			gr_disk(i2f(x),i2f(y),i2f(size/5));
			if (secondary_display && secondary_bm_num == 1)
				gr_uline(i2f(x-(size/2)-(size/5)), i2f(y-(size/2)), i2f(x-(size/5)-(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 2)
				gr_uline(i2f(x+(size/2)+(size/5)), i2f(y-(size/2)), i2f(x+(size/5)+(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 4)
				gr_uline(i2f(x), i2f(y+(size/2)+(size/5)), i2f(x), i2f(y+(size/5)+(size/5)));
			break;
		case RET_TYPE_CIRCLE:
			// Hack!  Something is going wrong in OGL-land with these numbers (???)
			if(size == 33 && x == 960 && y == 540) { size = 24; }
				
			gr_ucircle(i2f(x),i2f(y),i2f(size/4));
			if (secondary_display && secondary_bm_num == 1)
				gr_uline(i2f(x-(size/2)-(size/5)), i2f(y-(size/2)), i2f(x-(size/5)-(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 2)
				gr_uline(i2f(x+(size/2)+(size/5)), i2f(y-(size/2)), i2f(x+(size/5)+(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 4)
				gr_uline(i2f(x), i2f(y+(size/2)+(size/5)), i2f(x), i2f(y+(size/5)+(size/5)));
			break;
		case RET_TYPE_CROSS_V1:
			gr_uline(i2f(x),i2f(y-(size/2)),i2f(x),i2f(y+(size/2)+1)); // horiz
			gr_uline(i2f(x-(size/2)),i2f(y),i2f(x+(size/2)+1),i2f(y)); // vert
			if (secondary_display && secondary_bm_num == 1)
				gr_uline(i2f(x-(size/2)), i2f(y-(size/2)), i2f(x-(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 2)
				gr_uline(i2f(x+(size/2)), i2f(y-(size/2)), i2f(x+(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 4)
				gr_uline(i2f(x-(size/2)), i2f(y+(size/2)), i2f(x-(size/5)), i2f(y+(size/5)));
			break;
		case RET_TYPE_CROSS_V2:
			gr_uline(i2f(x), i2f(y-(size/2)), i2f(x), i2f(y-(size/6))); // vert-top
			gr_uline(i2f(x), i2f(y+(size/2)), i2f(x), i2f(y+(size/6))); // vert-bottom
			gr_uline(i2f(x-(size/2)), i2f(y), i2f(x-(size/6)), i2f(y)); // horiz-left
			gr_uline(i2f(x+(size/2)), i2f(y), i2f(x+(size/6)), i2f(y)); // horiz-right
			if (secondary_display && secondary_bm_num == 1)
				gr_uline(i2f(x-(size/2)), i2f(y-(size/2)), i2f(x-(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 2)
				gr_uline(i2f(x+(size/2)), i2f(y-(size/2)), i2f(x+(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 4)
				gr_uline(i2f(x-(size/2)), i2f(y+(size/2)), i2f(x-(size/5)), i2f(y+(size/5)));
			break;
		case RET_TYPE_ANGLE:
			gr_uline(i2f(x),i2f(y),i2f(x),i2f(y+(size/2))); // vert
			gr_uline(i2f(x),i2f(y),i2f(x+(size/2)),i2f(y)); // horiz
			if (secondary_display && secondary_bm_num == 1)
				gr_uline(i2f(x-(size/2)), i2f(y-(size/2)), i2f(x-(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 2)
				gr_uline(i2f(x+(size/2)), i2f(y-(size/2)), i2f(x+(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 4)
				gr_uline(i2f(x-(size/2)), i2f(y+(size/2)), i2f(x-(size/5)), i2f(y+(size/5)));
			break;
		case RET_TYPE_NONE:
			break;
		default:
			break;
	}
	gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);
}

void show_mousefs_indicator(int mx, int my, int mz, int x, int y, int size)
{
	int axscale = (MOUSEFS_DELTA_RANGE*2)/size, xaxpos = x+(mx/axscale), yaxpos = y+(my/axscale), zaxpos = y+(mz/axscale);

	gr_setcolor(BM_XRGB(PlayerCfg.ReticleRGBA[0],PlayerCfg.ReticleRGBA[1],PlayerCfg.ReticleRGBA[2]));
	gr_settransblend(PlayerCfg.ReticleRGBA[3], GR_BLEND_NORMAL);
	gr_uline(i2f(xaxpos), i2f(y-(size/2)), i2f(xaxpos), i2f(y-(size/4)));
	gr_uline(i2f(xaxpos), i2f(y+(size/2)), i2f(xaxpos), i2f(y+(size/4)));
	gr_uline(i2f(x-(size/2)), i2f(yaxpos), i2f(x-(size/4)), i2f(yaxpos));
	gr_uline(i2f(x+(size/2)), i2f(yaxpos), i2f(x+(size/4)), i2f(yaxpos));
	gr_uline(i2f(x+(size/2)+HUD_SCALE_X_AR(2)), i2f(y), i2f(x+(size/2)+HUD_SCALE_X_AR(2)), i2f(zaxpos));
	gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);
}

extern struct connection_status connection_statuses[]; 

void fontcolor_bad() {
	gr_set_fontcolor(BM_XRGB(25, 0, 0), -1);
}

void fontcolor_good() {
	gr_set_fontcolor(BM_XRGB(0, 18, 0), -1);
}

#ifdef NETWORK
void hud_show_kill_list()
{
	int n_players,player_list[MAX_PLAYERS];
	int n_left,i,x0,x1,y,save_y;

	if (Show_kill_list_timer > 0)
	{
		Show_kill_list_timer -= FrameTime;
		if (Show_kill_list_timer < 0)
			Show_kill_list = 0;
	}

	gr_set_curfont( GAME_FONT );

	n_players = multi_get_kill_list(player_list);

	if (Show_kill_list == 3)
		n_players = 2;

	if (n_players <= 4)
		n_left = n_players;
	else
		n_left = (n_players+1)/2;

    if(Netgame.BlackAndWhitePyros) 
		selected_player_rgb = player_rgb_alt; 
	else
		selected_player_rgb = player_rgb;

	x0 = FSPACX(1); x1 = FSPACX(43); 

	if ((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_MULTI_ROBOTS))
		x1 = FSPACX(31);

	save_y = y = grd_curcanv->cv_bitmap.bm_h - n_left*(LINE_SPACING);

	if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT) {
		save_y = y -= FSPACX(6);
		if ((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_MULTI_ROBOTS))
			x1 = FSPACX(33);
		else
			x1 = FSPACX(43);
	}

	int ox1 = x1; 
	for (i=0;i<n_players;i++) {
		int player_num;
		char name[9];
		int sw,sh,aw;

		if (i>=n_left) {
			if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT)
				x0 = grd_curcanv->cv_bitmap.bm_w - FSPACX(53);
			else
				x0 = grd_curcanv->cv_bitmap.bm_w - FSPACX(60);
			if ((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_MULTI_ROBOTS))
				x1 = grd_curcanv->cv_bitmap.bm_w - FSPACX(27);
			else
				x1 = grd_curcanv->cv_bitmap.bm_w - FSPACX(15);  // Right edge of name, change this for width problems
			if (i==n_left)
				y = save_y;

			if (Netgame.KillGoal || Netgame.PlayTimeAllowed)
				x1-=FSPACX(18);

			if(! ((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_MULTI_ROBOTS)) ) {
				x1 -= FSPACX(18);
				x0 -= FSPACX(18); 
			}

			if(Show_network_stats) {
				x1 -= FSPACX(45);
				x0 -= FSPACX(45); 

				if(Netgame.RetroProtocol) {
					x1 -= FSPACX(25);
					x0 -= FSPACX(25); 					
				}
			}

			if(Netgame.AllowPreferredColors) {
				x1 -= FSPACX(5);
				x0 -= FSPACX(5);
			}

			ox1 = x1;
		}
		else  if (Netgame.KillGoal || Netgame.PlayTimeAllowed)
		{
			x1 = FSPACX(43);
			ox1 = x1;
		}

		int lagx, loss_upx, loss_downx, cnxx; 

		lagx = x1 + FSPACX(15);
		if (Netgame.KillGoal || Netgame.PlayTimeAllowed)
				lagx+=FSPACX(18);

		loss_upx = lagx + FSPACX(15);
		if(Netgame.RetroProtocol) {
			loss_downx = loss_upx + FSPACX(25);
		} else {
			loss_downx = loss_upx; 
		}
		
		cnxx = loss_downx + FSPACX(25); 

		if (Show_kill_list == 3)
			player_num = i;
		else
			player_num = player_list[i];


		int color;

		if (Players[player_num].connected != CONNECT_PLAYING) {
			gr_set_fontcolor(BM_XRGB(12, 12, 12), -1);
		} else if (Game_mode & GM_TEAM) {
			color = get_color_for_team(player_num, 0);
			gr_set_fontcolor(BM_XRGB(selected_player_rgb[color].r,selected_player_rgb[color].g,selected_player_rgb[color].b),-1 );
		} else {
			color = get_color_for_player(player_num, 0);
			gr_set_fontcolor(BM_XRGB(selected_player_rgb[color].r,selected_player_rgb[color].g,selected_player_rgb[color].b),-1 );
		}		

		if (Show_kill_list == 3)
			strcpy(name, Netgame.team_name[i]);
		else if (Game_mode & GM_BOUNTY && player_num == Bounty_target && GameTime64&0x10000)
			strcpy(name,"[TARGET]");
		else
			strcpy(name,Players[player_num].callsign);	// Note link to above if!!
		gr_get_string_size(name,&sw,&sh,&aw);
		while (sw > (x1-x0-FSPACX(2))) {
			name[strlen(name)-1]=0;
			gr_get_string_size(name,&sw,&sh,&aw);
		}
		gr_printf(x0,y,"%s",name);


		if (Players[player_num].connected == CONNECT_PLAYING) {
			if (Game_mode & GM_TEAM) {
				color = get_color_for_team(player_num, 1);
				gr_set_fontcolor(BM_XRGB(selected_player_rgb[color].r,selected_player_rgb[color].g,selected_player_rgb[color].b),-1 );
			} else {
				color = get_color_for_player(player_num, 1);
				gr_set_fontcolor(BM_XRGB(selected_player_rgb[color].r,selected_player_rgb[color].g,selected_player_rgb[color].b),-1 );
			}	
		}

		if (Show_kill_list==2)
		{
			if (Players[player_num].net_killed_total+Players[player_num].net_kills_total==0)
				gr_string (x1,y,"NA");
			else
				gr_printf (x1,y,"%d%%",(int)((float)((float)Players[player_num].net_kills_total/((float)Players[player_num].net_killed_total+(float)Players[player_num].net_kills_total))*100.0));
		}
		else if (Show_kill_list == 3) {
			if (Netgame.PlayTimeAllowed || Netgame.KillGoal)
				gr_printf(x1,y,"%3d(%d)",team_kills[i],Netgame.TeamKillGoalCount[i]);
			else
				gr_printf(x1,y,"%3d",team_kills[i]);

			
		} else if ((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_MULTI_ROBOTS) )
			gr_printf(x1,y,"%-6d",Players[player_num].score);
		
		else {


			if (Netgame.PlayTimeAllowed || Netgame.KillGoal)
				gr_printf(x1,y,"%3d(%d)",Players[player_num].net_kills_total,Players[player_num].KillGoalCount);
			else
				gr_printf(x1,y,"%3d",Players[player_num].net_kills_total);

		}

		
		if((Show_network_stats && !(Game_mode & GM_OBSERVER)) && player_num != Player_num && Players[player_num].connected && Show_kill_list != 3) {
			int lag = -1; 
			
			if(Netgame.RetroProtocol) {
				lag = Netgame.players[player_num].ping; 
			} else {
				if(multi_i_am_master()) {
					lag = Netgame.players[player_num].ping; 
				} else if (player_num == multi_who_is_master()) {
					lag = Netgame.players[Player_num].ping;
				} else {
					lag = Netgame.players[Player_num].ping + Netgame.players[player_num].ping;
				}
			}

			if(lag != -1) {
				if(lag > 100) {
					fontcolor_bad();
				} else {
					fontcolor_good();
				}

				gr_printf(lagx,y,"% 3d", lag);
			}

			int loss_down = Netgame.players[i].loss; 
			int loss_up = Netgame.players[i].rx_loss;

			if(Netgame.RetroProtocol) {			
				if(loss_up) {
					fontcolor_bad();
				} else {
					fontcolor_good();
				}

				if(loss_up > 0 || loss_down > 0) {
					gr_printf(loss_upx,y,"% 3d%%", loss_up);
				}
			}



			if(loss_down) {
				fontcolor_bad();
			} else {
				fontcolor_good();
			}

			if(loss_up > 0 || loss_down > 0) {
				gr_printf(loss_downx,y,"% 3d%%", loss_down);
			}


			





			if(connection_statuses[player_num].type == PROXY) {
				fontcolor_bad();

				if(loss_up > 0 || loss_down > 0) {
					gr_printf(cnxx + FSPACX(5), y,"P");
				} else {
					gr_printf(loss_upx,y,"P");
				}
			}

			
		}

		y += LINE_SPACING;
	}
}
#endif

//returns true if viewer can see object
int see_object(int objnum)
{
	fvi_query fq;
	int hit_type;
	fvi_info hit_data;

	//see if we can see this player

	fq.p0 					= &Viewer->pos;
	fq.p1 					= &Objects[objnum].pos;
	fq.rad 					= 0;
	fq.thisobjnum			= Viewer - Objects;
	fq.flags 				= FQ_TRANSWALL | FQ_CHECK_OBJS | FQ_GET_SEGLIST;
	fq.startseg				= Viewer->segnum;
	fq.ignore_obj_list	= NULL;

	hit_type = find_vector_intersection(&fq, &hit_data);

	return (hit_type == HIT_OBJECT && hit_data.hit_object == objnum);
}

void show_HUD_names()
{
	int is_friend = 0, show_friend_name = 0, show_enemy_name = 0, show_name = 0, show_typing = 0, show_indi = 0, pnum = 0, objnum = 0;
	
	if(Netgame.BlackAndWhitePyros) 
		selected_player_rgb = player_rgb_alt; 
	else
		selected_player_rgb = player_rgb;

	for (pnum=0;pnum<N_players;pnum++)
	{
		if (pnum == Player_num || Players[pnum].connected != CONNECT_PLAYING)
			continue;
		// ridiculusly complex to check if we want to show something... but this is readable at least.
		is_friend = (Game_mode & GM_MULTI_COOP || (Game_mode & GM_TEAM && get_team(pnum) == get_team(Player_num)));
		show_friend_name = Show_reticle_name;
		show_enemy_name = Show_reticle_name && Netgame.ShowEnemyNames && !(Players[pnum].flags & PLAYER_FLAGS_CLOAKED);
		show_name = ((is_friend && show_friend_name) || (!is_friend && show_enemy_name)) || ((Game_mode & GM_OBSERVER) && (PlayerCfg.ObsShowNames)) ;
		show_typing = is_friend || !(Players[pnum].flags & PLAYER_FLAGS_CLOAKED);
		show_indi = ((/*(Game_mode & ( GM_CAPTURE | GM_HOARD ) && Players[pnum].flags & PLAYER_FLAGS_FLAG) || */(Game_mode & GM_BOUNTY &&  pnum == Bounty_target)) && (is_friend || !(Players[pnum].flags & PLAYER_FLAGS_CLOAKED)));

		if (Newdemo_state == ND_STATE_PLAYBACK) {
			//if this is a demo, the objnum in the player struct is wrong, so we search the object list for the objnum
			for (objnum=0;objnum<=Highest_object_index;objnum++)
				if (Objects[objnum].type==OBJ_PLAYER && Objects[objnum].id == pnum)
					break;
			if (objnum > Highest_object_index)	//not in list, thus not visible
				continue;			//..so don't show name
		}
		else
			objnum = Players[pnum].objnum;

		if ((show_name || show_typing || show_indi) && (see_object(objnum) || (Game_mode & GM_OBSERVER)))
		{
			g3s_point player_point;
			g3_rotate_point(&player_point,&Objects[objnum].pos);

			if (player_point.p3_codes == 0) //on screen
			{
				g3_project_point(&player_point);

				if (!(player_point.p3_flags & PF_OVERFLOW))
				{
					fix x,y,dx,dy;
					char s[CALLSIGN_LEN+20];
					int w, h, aw, x1, y1, color_num;

					x = player_point.p3_sx;
					y = player_point.p3_sy;
					dy = -fixmuldiv(fixmul(Objects[objnum].size,Matrix_scale.y),i2f(grd_curcanv->cv_bitmap.bm_h)/2,player_point.p3_z);
					dx = fixmul(dy,grd_curscreen->sc_aspect);
					color_num = (Game_mode & GM_TEAM)?get_team(pnum):Netgame.players[pnum].color;//pnum;
					memset(&s, '\0', CALLSIGN_LEN+20);
					/* Set the text to show */
					if( Game_mode & GM_BOUNTY && pnum == Bounty_target )
						strncpy( s, "Target", 6 );
					else if (show_name)
					{
						if (Game_mode & GM_OBSERVER)
							snprintf( s, sizeof(s), "%s (%0.1f)", Players[pnum].callsign, f2db(Players[pnum].shields) );
						else
							snprintf( s, sizeof(s), "%s", Players[pnum].callsign );
					}
					if (show_typing && multi_sending_message[pnum])
					{
						if (s[0])
							strncat( s, ", typing", 8);
						else
							strncpy( s, "Typing", 6 );
					}
					if (s[0])
					{
						gr_get_string_size(s, &w, &h, &aw);
						gr_set_fontcolor(BM_XRGB(selected_player_rgb[color_num].r,selected_player_rgb[color_num].g,selected_player_rgb[color_num].b),-1 );
						x1 = f2i(x)-w/2;
						y1 = f2i(y-dy)+FSPACY(1);
						gr_string (x1, y1, s);
					}

					if (Game_mode & GM_OBSERVER)
					{
						if (Players[pnum].shields_delta != 0 && (Players[Player_num].hours_total - Players[pnum].shields_time_hours == 1 && i2f(3600) + Players[Player_num].time_total - Players[pnum].shields_time < i2f(2) || Players[Player_num].time_total - Players[pnum].shields_time < i2f(2)))
						{
							double diff = (double)(int)Players[pnum].shields_delta / 65536;

							memset(&s, '\0', CALLSIGN_LEN+20);
							snprintf( s, sizeof(s), "%+0.1f", diff);

							gr_get_string_size(s, &w, &h, &aw);
							gr_set_fontcolor(BM_XRGB(diff < 0 ? (diff < -50 ? 32 : (diff > -10 ? 8 : 32 - (diff + 50) * 24 / 40)) : 0, diff > 0 ? 32 : 0, 0), -1);
							x1 = f2i(x)-w/2;
							y1 = f2i(y+dy)-FSPACY(6);
							gr_string (x1, y1, s);
						}
					}

					/* Draw box on HUD */
					if (show_indi)
					{
						fix w,h;

						w = dx/4;
						h = dy/4;

// 						if (Game_mode & GM_CAPTURE)
// 							gr_setcolor((get_team(pnum) == TEAM_BLUE)?BM_XRGB(31,0,0):BM_XRGB(0,0,31));
// 						else if (Game_mode & GM_HOARD)
// 						{
// 							if (Game_mode & GM_TEAM)
// 								gr_setcolor((get_team(pnum) == TEAM_RED)?BM_XRGB(31,0,0):BM_XRGB(0,0,31));
// 							else
// 								gr_setcolor(BM_XRGB(0,31,0));
// 						}
// 						else if( Game_mode & GM_BOUNTY )
							gr_setcolor( BM_XRGB( selected_player_rgb[pnum].r, selected_player_rgb[pnum].g, selected_player_rgb[pnum].b ) );

						gr_line(x+dx-w,y-dy,x+dx,y-dy);
						gr_line(x+dx,y-dy,x+dx,y-dy+h);
						gr_line(x-dx,y-dy,x-dx+w,y-dy);
						gr_line(x-dx,y-dy,x-dx,y-dy+h);
						gr_line(x+dx-w,y+dy,x+dx,y+dy);
						gr_line(x+dx,y+dy,x+dx,y+dy-h);
						gr_line(x-dx,y+dy,x-dx+w,y+dy);
						gr_line(x-dx,y+dy,x-dx,y+dy-h);
					}
				}
			}
		}
	}
}

//draw all the things on the HUD

void draw_hud()
{
	if (PlayerCfg.HudMode==3) // no hud, "immersion mode"
		return;

	// Cruise speed 
	if ( Player_num > -1 && Viewer->type==OBJ_PLAYER && Viewer->id==Player_num && PlayerCfg.CockpitMode[1] != CM_REAR_VIEW)	{
		int	x = FSPACX(1);
		int	y = grd_curcanv->cv_bitmap.bm_h;

		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor( BM_XRGB(0, 31, 0), -1 );
		if (Cruise_speed > 0) {
			if (PlayerCfg.CockpitMode[1]==CM_FULL_SCREEN) {
				if (Game_mode & GM_MULTI)
					y -= LINE_SPACING * 10;
				else
					y -= LINE_SPACING * 6;
			} else if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR) {
				if (Game_mode & GM_MULTI)
					y -= LINE_SPACING * 6;
				else
					y -= LINE_SPACING * 1;
			} else {
				if (Game_mode & GM_MULTI)
					y -= LINE_SPACING * 7;
				else
					y -= LINE_SPACING * 2;
			}

			gr_printf( x, y, "%s %2d%%", TXT_CRUISE, f2i(Cruise_speed) );
		}
	}

	//	Show score so long as not in rearview
	if ( !Rear_view && PlayerCfg.CockpitMode[1]!=CM_REAR_VIEW && PlayerCfg.CockpitMode[1]!=CM_STATUS_BAR) {
		hud_show_score();
		if (score_time)
			hud_show_score_added();
	}

	if ( !Rear_view && PlayerCfg.CockpitMode[1]!=CM_REAR_VIEW)
		hud_show_timer_count();

	//	Show other stuff if not in rearview or letterbox.
	if (!Rear_view && PlayerCfg.CockpitMode[1]!=CM_REAR_VIEW)
	{
		show_HUD_names();

		if (PlayerCfg.CockpitMode[1]==CM_STATUS_BAR || PlayerCfg.CockpitMode[1]==CM_FULL_SCREEN)
			hud_show_homing_warning();

		if (PlayerCfg.CockpitMode[1]==CM_FULL_SCREEN) {
			hud_show_energy();
			hud_show_shield();
			hud_show_weapons();
			if (!PCSharePig)
				hud_show_keys();
			hud_show_cloak_invuln();

			if (Newdemo_state==ND_STATE_RECORDING)
				newdemo_record_player_flags(Players[Player_num].flags);
		}

#ifndef RELEASE
		if (!(Game_mode&GM_MULTI && Show_kill_list))
			show_time();
#endif
		HUD_render_message_frame();

		if (PlayerCfg.CockpitMode[1]!=CM_STATUS_BAR)
			hud_show_lives();
		if (Game_mode&GM_MULTI && Show_kill_list)
			hud_show_kill_list();
		if (PlayerCfg.CockpitMode[1] != CM_LETTERBOX)
			show_reticle(PlayerCfg.ReticleType, 1);
		if (PlayerCfg.CockpitMode[1] != CM_LETTERBOX && Newdemo_state != ND_STATE_PLAYBACK && (PlayerCfg.MouseControlStyle == MOUSE_CONTROL_FLIGHT_SIM) && PlayerCfg.MouseFSIndicator) /* Old School Mouse */
			show_mousefs_indicator(Controls.raw_mouse_axis[0], Controls.raw_mouse_axis[1], Controls.raw_mouse_axis[2], GWIDTH/2, GHEIGHT/2, GHEIGHT/4);
	}

	if (Rear_view && PlayerCfg.CockpitMode[1]!=CM_REAR_VIEW) {
		HUD_render_message_frame();
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(BM_XRGB(0,31,0),-1 );
		gr_string(0x8000,GHEIGHT-LINE_SPACING,TXT_REAR_VIEW);
	}

}

//print out some player statistics
void render_gauges()
{
	int energy = f2ir(Players[Player_num].energy);
	int shields = f2ir(Players[Player_num].shields);
	int cloak = ((Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) != 0);

#ifdef RT_DX12
	Assert(PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT || PlayerCfg.CockpitMode[1] == CM_STATUS_BAR || PlayerCfg.CockpitMode[1] == CM_MODEL_3D);
#else
	Assert(PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT || PlayerCfg.CockpitMode[1] == CM_STATUS_BAR);
#endif

	if (shields < 0) shields = 0;

	gr_set_current_canvas(NULL);
	gr_set_curfont(GAME_FONT);

	if (Newdemo_state == ND_STATE_RECORDING)
		if (Players[Player_num].homing_object_dist >= 0)
			newdemo_record_homing_distance(Players[Player_num].homing_object_dist);


	if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT) {
		draw_weapon_boxes();

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_player_energy(energy);
		draw_energy_bar(energy);
		draw_numerical_display(shields, energy);
		if (!PlayerCfg.HudMode)
			show_bomb_count(HUD_SCALE_X(BOMB_COUNT_X), HUD_SCALE_Y(BOMB_COUNT_Y), gr_find_closest_color(0, 0, 0), 0, 0);
		draw_player_ship(cloak, SHIP_GAUGE_X, SHIP_GAUGE_Y);

		if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE)
			draw_invulnerable_ship();
		else
			draw_shield_bar(shields);
		draw_numerical_display(shields, energy);
		if (Newdemo_state == ND_STATE_RECORDING)
		{
			newdemo_record_player_shields(shields);
			newdemo_record_player_flags(Players[Player_num].flags);
		}
		draw_keys();

		show_homing_warning();
		draw_wbu_overlay();

	}
	else if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR) {
		draw_weapon_boxes();

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_player_energy(energy);
		sb_draw_energy_bar(energy);
		if (!PlayerCfg.HudMode)
			show_bomb_count(HUD_SCALE_X(SB_BOMB_COUNT_X), HUD_SCALE_Y(SB_BOMB_COUNT_Y), gr_find_closest_color(0, 0, 0), 0, 0);

		draw_player_ship(cloak, SB_SHIP_GAUGE_X, SB_SHIP_GAUGE_Y);

		if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE)
			draw_invulnerable_ship();
		else
			sb_draw_shield_bar(shields);
		sb_draw_shield_num(shields);

		if (Newdemo_state == ND_STATE_RECORDING)
		{
			newdemo_record_player_shields(shields);
			newdemo_record_player_flags(Players[Player_num].flags);
		}
		sb_draw_keys();

		sb_show_lives();

		if ((Game_mode & GM_MULTI) && !((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_MULTI_ROBOTS)))
		{
			sb_show_score();
		}
		else
		{
			sb_show_score();
			sb_show_score_added();
		}
	}
#ifdef RT_DX12

	// If we're using the 3d cockpit model, render every UI element to the cockpit's screen textures
	else if (PlayerCfg.CockpitMode[1] == CM_MODEL_3D) {
		// note(lily): this might be the most giga scuffed code I've ever written, but don't worry about it :D
		const float tmp_x = FNTScaleX;
		const float tmp_y = FNTScaleY;
		const auto tmp_bm_w = grd_curcanv->cv_bitmap.bm_w;
		const auto tmp_bm_h = grd_curcanv->cv_bitmap.bm_h;
		const auto tmp_last_width = last_width;
		const auto tmp_last_height = last_height;
		
		grd_curcanv->cv_bitmap.bm_w = 1024;
		grd_curcanv->cv_bitmap.bm_h = 1024;
		last_width = 1024;
		last_height = 1024;
		FNTScaleX = 6;
		FNTScaleY = 6;
		dx12_set_render_target(g_rt_cockpit_settings.cockpit_hud_texture);
		RT_RasterSetViewport(0, 0, 1024, 1024);
		draw_weapon_boxes();

		// Now define a silly little macro that expands those positions to 4 parameters
        #define PLACE(pos) pos[0], pos[1], pos[2], pos[3]

		// Render keys
		if (Players[Player_num].flags & PLAYER_FLAGS_RED_KEY) {
			render_ui_bitmap(Gauges[GAUGE_RED_KEY], PLACE(pos_red_key));
		} else {
			render_ui_bitmap(Gauges[GAUGE_RED_KEY_OFF], PLACE(pos_red_key));
		}
		if (Players[Player_num].flags & PLAYER_FLAGS_GOLD_KEY) {
			render_ui_bitmap(Gauges[GAUGE_GOLD_KEY], PLACE(pos_gold_key));
		} else {
			render_ui_bitmap(Gauges[GAUGE_GOLD_KEY_OFF], PLACE(pos_gold_key));
		}
		if (Players[Player_num].flags & PLAYER_FLAGS_BLUE_KEY) {
			render_ui_bitmap(Gauges[GAUGE_BLUE_KEY], PLACE(pos_blue_key));
		} else {
			render_ui_bitmap(Gauges[GAUGE_BLUE_KEY_OFF], PLACE(pos_blue_key));
		}

		// Render full energy bars
		render_ui_bitmap(Gauges[GAUGE_ENERGY_LEFT], PLACE(pos_energy_left));
		render_ui_bitmap(Gauges[GAUGE_ENERGY_RIGHT], PLACE(pos_energy_right));

		// Hide corners
		if (energy < 100) {
			// Erase part of energy bar to indicate the actual energy level
			const float t = (float)energy / 100.f;
			const float right_black_bar_x = RT_Lerp(pos_energy_right_x[1], pos_energy_right_x[0], t);
			const float left_black_bar_x = RT_Lerp(pos_energy_left_x[1], pos_energy_left_x[0], t);
			const float bar_skew_scalar = RT_Lerp(0.25f, 1.125f, t);
			const float right_max_h = pos_energy_right_y[1] - pos_energy_right_y[0];
			const float left_max_h = pos_energy_left_y[1] - pos_energy_left_y[0];

			const RT_RasterTriVertex verts_right[4] = {
				{.pos = {right_black_bar_x - right_max_h * bar_skew_scalar, pos_energy_right_y[0], 0.0f}, .color = {0, 0, 0, 1}},
				{.pos = {pos_energy_right_x[0], pos_energy_right_y[0], 0.0f}, .color = {0, 0, 0, 1}},
				{.pos = {pos_energy_right_x[0], pos_energy_right_y[1], 0.0f}, .color = {0, 0, 0, 1}},
				{.pos = {right_black_bar_x, pos_energy_right_y[1], 0.0f}, .color = {0, 0, 0, 1}},
			};
			const RT_RasterTriVertex verts_left[4] = {
				{.pos = {left_black_bar_x, pos_energy_left_y[1], 0.0f}, .color = {0, 0, 0, 1}},
				{.pos = {pos_energy_left_x[0], pos_energy_left_y[1], 0.0f}, .color = {0, 0, 0, 1}},
				{.pos = {pos_energy_left_x[0], pos_energy_left_y[0], 0.0f}, .color = {0, 0, 0, 1}},
				{.pos = {left_black_bar_x + left_max_h * bar_skew_scalar, pos_energy_left_y[0], 0.0f}, .color = {0, 0, 0, 1}},
			};
			RT_RasterTriVertex triangles[12] = {
				verts_right[0],verts_right[1],verts_right[2],
				verts_right[0],verts_right[2],verts_right[3],
				verts_left[0],verts_left[1],verts_left[2],
				verts_left[0],verts_left[2],verts_left[3]
			};
			for (size_t i = 0; i < 12; ++i) {
				triangles[i].pos.x /= (float)grd_curcanv->cv_bitmap.bm_w / 2.0f;
				triangles[i].pos.x -= 1.0f;
				triangles[i].pos.y /= (float)grd_curcanv->cv_bitmap.bm_h / 2.0f;
				triangles[i].pos.y -= 1.0f;
				triangles[i].pos.y *= -1.0f;
			}
			RT_RasterTriangles(&(RT_RasterTrianglesParams) { .num_vertices = 12, .vertices = triangles, .texture_handle = { 0 } }, 1);
			
		}

	    // Draw homing warning
		FNTScaleX = scale_text_homing[0];
		FNTScaleY = scale_text_homing[1];
		const bool flash_timer_is_toggled_on = (GameTime64 & 0x4000);
		const bool object_is_homing_in = (Players[Player_num].homing_object_dist >= 0);
		const bool not_in_end_level_sequence = !Endlevel_sequence;

		if (not_in_end_level_sequence && object_is_homing_in && flash_timer_is_toggled_on)
			gr_set_fontcolor(BM_XRGB(49, 35, 11), -1);
		else
			gr_set_fontcolor(BM_XRGB(15, 12, 3), -1);
		gr_printf(pos_homing_text[0], pos_homing_text[1], "LOCK");

		// Render player ship and shield
        const int color = Netgame.players[Player_num].color;
		render_ui_bitmap(Gauges[GAUGE_SHIPS + color], pos_ship_center.x - scl_ship, pos_ship_center.y - scl_ship, pos_ship_center.x + scl_ship, pos_ship_center.y + scl_ship);
        const int bm_num = shields >= 100 ? 9 : (shields / 10);
		render_ui_bitmap(Gauges[GAUGE_SHIELDS + 9 - bm_num], pos_ship_center.x - scl_shield, pos_ship_center.y - scl_shield, pos_ship_center.x + scl_shield, pos_ship_center.y + scl_shield);

		// Draw weapons
		gr_set_curfont(GAME_FONT);

		// Render shield number text
		FNTScaleX = scale_text_misc[0];
		FNTScaleY = scale_text_misc[1];
		int sw, sh, saw, ew, eh, eaw;
		gr_set_fontcolor(BM_XRGB(14, 14, 23), -1);
		gr_get_string_size((shields > 199) ? "200" : (shields > 99) ? "100" : (shields > 9) ? "00" : "0", &sw, &sh, &saw);
		gr_printf((pos_shield_text[0]) - (sw / 2),
			(pos_shield_text[1]), "%d", shields);

		// Render energy number text
		gr_set_fontcolor(BM_XRGB(25, 18, 6), -1);
		gr_get_string_size((energy > 199) ? "200" : (energy > 99) ? "100" : (energy > 9) ? "00" : "0", &ew, &eh, &eaw);
		gr_printf((pos_energy_text[0]) - (ew / 2),
			(pos_energy_text[1]), "%d", energy);

		// end of scuffed
		FNTScaleX = tmp_x;
		FNTScaleY = tmp_y;
		grd_curcanv->cv_bitmap.bm_w = tmp_bm_w;
		grd_curcanv->cv_bitmap.bm_h = tmp_bm_h;
		last_width = tmp_last_width;
		last_height = tmp_last_height;
		dx12_set_render_target(RT_RESOURCE_HANDLE_NULL);
		RT_RasterSetViewport(grd_curcanv->cv_bitmap.bm_x, grd_curcanv->cv_bitmap.bm_y, Canvas_width, Canvas_height);
    }
#endif
	else
	{
		draw_player_ship(cloak, SB_SHIP_GAUGE_X, SB_SHIP_GAUGE_Y);
		draw_weapon_boxes();
	}

#ifdef RT_DX12
	dx12_end_frame();
	dx12_start_frame();
#endif
}

//	---------------------------------------------------------------------------------------------------------
//	Call when picked up a laser powerup.
//	If laser is active, set old_weapon[0] to -1 to force redraw.
void update_laser_weapon_info(void)
{
	if (old_weapon[0] == 0)
		old_weapon[0] = -1;
}
