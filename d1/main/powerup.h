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
 * Powerup header file.
 *
 */

#ifndef _POWERUP_H
#define _POWERUP_H

#include "vclip.h"
#include "player.h"

enum powerup_type_t
{
	POW_EXTRA_LIFE = 0,
	POW_ENERGY = 1,
	POW_SHIELD_BOOST = 2,
	POW_LASER = 3,

	POW_KEY_BLUE = 4,
	POW_KEY_RED = 5,
	POW_KEY_GOLD = 6,

	POW_RADAR_ROBOTS = 7,
	POW_RADAR_POWERUPS = 8,
	POW_FULL_MAP = 9,

	POW_MISSILE_1 = 10,
	POW_MISSILE_4 = 11,      // 4-pack MUST follow single missile

	POW_QUAD_FIRE = 12,

	POW_VULCAN_WEAPON = 13,
	POW_SPREADFIRE_WEAPON = 14,
	POW_PLASMA_WEAPON = 15,
	POW_FUSION_WEAPON = 16,
	POW_PROXIMITY_WEAPON = 17,
	POW_HOMING_AMMO_1 = 18,
	POW_HOMING_AMMO_4 = 19,
	POW_SMARTBOMB_WEAPON = 20,
	POW_MEGA_WEAPON = 21,
	POW_VULCAN_AMMO = 22,
	POW_CLOAK = 23,
	POW_TURBO = 24,
	POW_INVULNERABILITY = 25,
	POW_HEADLIGHT = 26,
	POW_MEGAWOW = 27,
};

#define VULCAN_AMMO_MAX             (392*2)
#define VULCAN_WEAPON_AMMO_AMOUNT   196
#define VULCAN_AMMO_AMOUNT          (49*2)

// What I picked up        What it said I picked up
// ----------------        ------------------------
// vulcan ammo             4 homing missiles
// mega missile            1 homing missile
// smart missile           vulcan ammo
// 4 homing missiles       mega missile
// 1 homing missile        smart missile
// 
// The rest were correct.  I can help you with this whenever you're free.

#define MAX_POWERUP_TYPES			29

#define POWERUP_NAME_LENGTH 16      // Length of a robot or powerup name.
extern char Powerup_names[MAX_POWERUP_TYPES][POWERUP_NAME_LENGTH];

typedef struct powerup_type_info {
	int vclip_num;
	int hit_sound;
	fix size;       // 3d size of longest dimension
	fix light;      // amount of light cast by this powerup, set in bitmaps.tbl
} __pack__ powerup_type_info;

extern int N_powerup_types;
extern powerup_type_info Powerup_info[MAX_POWERUP_TYPES];

void draw_powerup(object *obj);

//returns true if powerup consumed
int do_powerup(object *obj);

//process (animate) a powerup for one frame
void do_powerup_frame(object *obj);

// Diminish shields and energy towards max in case they exceeded it.
extern void diminish_towards_max(void);

extern void do_megawow_powerup(int quantity);

extern void powerup_basic(int redadd, int greenadd, int blueadd, int score, char *format, ...);

// may this powerup be added to the level?
// returns number of powerups left if true, otherwise 0.
extern int may_create_powerup(int powerup);

/*
 * reads n powerup_type_info structs from a PHYSFS_file
 */
extern int powerup_type_info_read_n(powerup_type_info *pti, int n, PHYSFS_file *fp);

#endif /* _POWERUP_H */

