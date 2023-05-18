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
 * Multiplayer robot code
 *
 */

#ifdef NETWORK
#ifndef SHAREWARE

#include <string.h>
#include <stdlib.h>

#include "vecmat.h"
#include "game.h"
#include "multi.h"
#include "object.h"
#include "laser.h"
#include "dxxerror.h"
#include "timer.h"
#include "text.h"
#include "ai.h"
#include "fireball.h"
#include "aistruct.h"
#include "robot.h"
#include "powerup.h"
#include "scores.h"
#include "gauges.h"
#include "fuelcen.h"
#include "morph.h"
#include "digi.h"
#include "sounds.h"
#include "effects.h"
#include "physics.h" 
#include "multibot.h"
#include "byteswap.h"
#include "wall.h"

void multi_delete_controlled_robot(int objnum);
void multi_send_robot_position_sub(int objnum, int now);
void multi_send_release_robot(int objnum);
int multi_add_controlled_robot(int objnum, int agitation);

//
// Code for controlling robots in multiplayer games
//

#define STANDARD_EXPL_DELAY (F1_0/4)
#define MIN_CONTROL_TIME	F1_0*2
#define ROBOT_TIMEOUT		F1_0*3

#define MIN_TO_ADD	60
#define MAX_TO_DELETE	67

int robot_controlled[MAX_ROBOTS_CONTROLLED];
int robot_agitation[MAX_ROBOTS_CONTROLLED];
fix64 robot_controlled_time[MAX_ROBOTS_CONTROLLED];
fix64 robot_last_send_time[MAX_ROBOTS_CONTROLLED];
fix64 robot_last_message_time[MAX_ROBOTS_CONTROLLED];
int robot_send_pending[MAX_ROBOTS_CONTROLLED];
int robot_fired[MAX_ROBOTS_CONTROLLED];
ubyte robot_fire_buf[MAX_ROBOTS_CONTROLLED][18+3];

#define MULTI_ROBOT_PRIORITY(objnum, pnum) (((objnum % 4) + pnum) % N_players)

//#define MULTI_ROBOT_PRIORITY(objnum, pnum) multi_robot_priority(objnum, pnum)
//int multi_robot_priority(int objnum, int pnum)
//{
//	return( ((objnum % 4) + pnum) % N_players);
//}

int
multi_can_move_robot(int objnum, int agitation)
{
	// Determine whether or not I am allowed to move this robot.
	int rval;

	// Claim robot if necessary.

	if (Player_exploded)
		return 0;

#ifndef NDEBUG
	if ((objnum < 0) || (objnum > Highest_object_index))
	{	
		Int3();
		rval = 0;
	}

	else if (Objects[objnum].type != OBJ_ROBOT)
	{
		Int3();
		rval = 0;
	}
#endif

	else if ((Robot_info[Objects[objnum].id].boss_flag) && (Boss_dying == 1))
		return 0;

	else if (Objects[objnum].ctype.ai_info.REMOTE_OWNER == Player_num) // Already my robot!
	{
		int slot_num = Objects[objnum].ctype.ai_info.REMOTE_SLOT_NUM;
		Assert((slot_num > -1) && (slot_num < MAX_ROBOTS_CONTROLLED));

		if (robot_fired[slot_num]) {
			rval = 0;
		}
		else {
			robot_agitation[slot_num] = agitation;
			robot_last_message_time[slot_num] = GameTime64;
			rval = 1;
		}
	}

	else if ((Objects[objnum].ctype.ai_info.REMOTE_OWNER != -1) || (agitation < MIN_TO_ADD))
	{
		if (agitation == ROBOT_FIRE_AGITATION) // Special case for firing at non-player
		{
			rval = 1; // Try to fire at player even tho we're not in control!
		}
		else
			rval = 0;
	}
	else
		rval = multi_add_controlled_robot(objnum, agitation);

	return(rval);
}

void
multi_check_robot_timeout(void)
{
	static fix64 lastcheck = 0;
	int i;

	if (GameTime64 > lastcheck + F1_0 || lastcheck > GameTime64)
	{
		lastcheck = GameTime64;
		for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++) 
		{
			if ((robot_controlled[i] != -1) && (robot_last_send_time[i] + ROBOT_TIMEOUT < GameTime64)) 
			{
				if (Objects[robot_controlled[i]].ctype.ai_info.REMOTE_OWNER != Player_num)
				{		
					robot_controlled[i] = -1;
					Int3(); // Non-terminal but Rob is interesting, step over please...
					return;
				}
//				Assert(Objects[robot_controlled[i]].ctype.ai_info.REMOTE_OWNER == Player_num);
				if (robot_send_pending[i])
					multi_send_robot_position(robot_controlled[i], 1);
				multi_send_release_robot(robot_controlled[i]);
//				multi_delete_controlled_robot(robot_controlled[i]);
//				robot_controlled[i] = -1;
			}
		}
	}			
}

void
multi_strip_robots(int playernum)
{
	// Grab all robots away from a player 
	// (player died or exited the game)

	int i;

	if (Game_mode & GM_MULTI_ROBOTS) {
	
		if (playernum == Player_num)
			for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
				multi_delete_controlled_robot(robot_controlled[i]);

		for (i = 1; i <= Highest_object_index; i++)
			if ((Objects[i].type == OBJ_ROBOT) && (Objects[i].ctype.ai_info.REMOTE_OWNER == playernum)) {
				Assert((Objects[i].control_type == CT_AI) || (Objects[i].control_type == CT_NONE) || (Objects[i].control_type == CT_MORPH));
				Objects[i].ctype.ai_info.REMOTE_OWNER = -1;
				if (playernum == Player_num)
					Objects[i].ctype.ai_info.REMOTE_SLOT_NUM = 4;
				else
					Objects[i].ctype.ai_info.REMOTE_SLOT_NUM = 0;
	  		}
	}
	// Note -- only call this with playernum == Player_num if all other players
	// already know that we are clearing house.  This does not send a release
	// message for each controlled robot!!

}

void
multi_dump_robots(void)
{
	// Dump robot control info for debug purposes

	if (!(Game_mode & GM_MULTI_ROBOTS))
		return;
}

int
multi_add_controlled_robot(int objnum, int agitation)
{
	int i;
	int lowest_agitation = 0x7fffffff; // MAX POSITIVE INT
	int lowest_agitated_bot = -1;
	int first_free_robot = -1;

	// Try to add a new robot to the controlled list, return 1 if added, 0 if not.

	if (Objects[objnum].ctype.ai_info.REMOTE_SLOT_NUM > 0)
	{
		Objects[objnum].ctype.ai_info.REMOTE_SLOT_NUM -= 1;
		return 0;
	}

	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
	{
		if ((robot_controlled[i] == -1) || (Objects[robot_controlled[i]].type != OBJ_ROBOT)) {
			first_free_robot = i;
			break;
		}

		if (robot_last_message_time[i] + ROBOT_TIMEOUT < GameTime64) {
			if (robot_send_pending[i])
				multi_send_robot_position(robot_controlled[i], 1);
			multi_send_release_robot(robot_controlled[i]);
			first_free_robot = i;
			break;
		}

		if ((robot_controlled[i] != -1) && (robot_agitation[i] < lowest_agitation) && (robot_controlled_time[i] + MIN_CONTROL_TIME < GameTime64))
		{
			lowest_agitation = robot_agitation[i];
			lowest_agitated_bot = i;
		}
	}

	if (first_free_robot != -1)  // Room left for new robots
		i = first_free_robot;

	else if ((agitation > lowest_agitation) && (lowest_agitation <= MAX_TO_DELETE)) // Replace some old robot with a more agitated one
	{
		if (robot_send_pending[lowest_agitated_bot])
			multi_send_robot_position(robot_controlled[lowest_agitated_bot], 1);
		multi_send_release_robot(robot_controlled[lowest_agitated_bot]);

		i = lowest_agitated_bot;
	}
	else {
		return(0); // Sorry, can't squeeze him in!
	}

	multi_send_claim_robot(objnum);
	robot_controlled[i] = objnum;
	robot_agitation[i] = agitation;
	Objects[objnum].ctype.ai_info.REMOTE_OWNER = Player_num;
	Objects[objnum].ctype.ai_info.REMOTE_SLOT_NUM = i;
	robot_controlled_time[i] = GameTime64;
	robot_last_send_time[i] = robot_last_message_time[i] = GameTime64;
	return(1);
}	

void
multi_delete_controlled_robot(int objnum)
{
	int i;

	// Delete robot object number objnum from list of controlled robots because it is dead

	if ( (objnum<0) || (objnum>Highest_object_index))	{
		return;
	}

	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
		if (robot_controlled[i] == objnum)
			break;

	if (i == MAX_ROBOTS_CONTROLLED)
		return;

	Assert(Objects[objnum].ctype.ai_info.REMOTE_SLOT_NUM == i);

	Objects[objnum].ctype.ai_info.REMOTE_OWNER = -1;
	Objects[objnum].ctype.ai_info.REMOTE_SLOT_NUM = 0;
	robot_controlled[i] = -1;
	robot_send_pending[i] = 0;
	robot_fired[i] = 0;
}

void
multi_send_claim_robot(int objnum)
{
	short s;
	
	if ((objnum < 0) || (objnum > Highest_object_index))
	{
		Int3(); // See rob
		return;
	}

	if (Objects[objnum].type != OBJ_ROBOT)
	{
		Int3(); // See rob
		return;
	}

	// The AI tells us we should take control of this robot. 

	multibuf[0] = (char)MULTI_ROBOT_CLAIM;
	multibuf[1] = Player_num;
	s = objnum_local_to_remote(objnum, (sbyte *)&multibuf[4]);
	PUT_INTEL_SHORT(multibuf+2, s);

	multi_send_data(multibuf, 5, 2);
}

void
multi_send_release_robot(int objnum)
{
	short s;
	
	if ((objnum < 0) || (objnum > Highest_object_index))
	{
		Int3(); // See rob
		return;
	}

	if (Objects[objnum].type != OBJ_ROBOT)
	{
		Int3(); // See rob
		return;
	}

	multi_delete_controlled_robot(objnum);

	multibuf[0] = (char)MULTI_ROBOT_RELEASE;
	multibuf[1] = Player_num;
	s = objnum_local_to_remote(objnum, (sbyte *)&multibuf[4]);
	PUT_INTEL_SHORT(multibuf+2, s);

	multi_send_data(multibuf, 5, 2);
}

#define MIN_ROBOT_COM_GAP F1_0/12

int
multi_send_robot_frame(int sent)
{
	static int last_sent = 0;

	int i;
	int rval = 0;

	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
	{
		int sending = (last_sent+1+i)%MAX_ROBOTS_CONTROLLED;
		if ( (robot_controlled[sending] != -1) && ((robot_send_pending[sending] > sent) || (robot_fired[sending] > sent)) )
		{
			if (robot_send_pending[sending])
			{
				multi_send_robot_position_sub(robot_controlled[sending], (robot_send_pending[sending]>1)?1:0);
				robot_send_pending[sending] = 0;
			}

			if (robot_fired[sending])
			{
				robot_fired[sending] = 0;
				multi_send_data(robot_fire_buf[sending], 18, 1);
			}

			if (!(Game_mode & GM_NETWORK))
				sent += 1;

			last_sent = sending;
			rval++;
		}
	}
	Assert((last_sent >= 0) && (last_sent <= MAX_ROBOTS_CONTROLLED));
	return(rval);
}

void
multi_send_robot_position_sub(int objnum, int now)
{
	int loc = 0;
	short s;
#ifdef WORDS_BIGENDIAN
	shortpos sp;
#endif

	multibuf[loc] = MULTI_ROBOT_POSITION;  								loc += 1;
	multibuf[loc] = Player_num;											loc += 1;
	s = objnum_local_to_remote(objnum, (sbyte *)&multibuf[loc+2]);
	PUT_INTEL_SHORT(multibuf+loc, s);

																		loc += 3;
#ifndef WORDS_BIGENDIAN
	create_shortpos((shortpos *)(multibuf+loc), Objects+objnum,0);		loc += sizeof(shortpos);
#else
	create_shortpos(&sp, Objects+objnum, 1);
	memcpy(&(multibuf[loc]), (ubyte *)(sp.bytemat), 9);
	loc += 9;
	memcpy(&(multibuf[loc]), (ubyte *)&(sp.xo), 14);
	loc += 14;
#endif
	multi_send_data(multibuf, loc, now?1:0);
}

void
multi_send_robot_position(int objnum, int force)
{
	// Send robot position to other player(s).  Includes a byte
	// value describing whether or not they fired a weapon

	if (!(Game_mode & GM_MULTI))
		return;

	if ((objnum < 0) || (objnum > Highest_object_index))
	{
		Int3(); // See rob
		return;
	}
	if (Objects[objnum].type != OBJ_ROBOT)
	{
		Int3(); // See rob
		return;
	}

	if (Objects[objnum].ctype.ai_info.REMOTE_OWNER != Player_num)
		return;

//	Objects[objnum].phys_info.drag = Robot_info[Objects[objnum].id].drag; // Set drag to normal

	robot_last_send_time[Objects[objnum].ctype.ai_info.REMOTE_SLOT_NUM] = GameTime64;

	robot_send_pending[Objects[objnum].ctype.ai_info.REMOTE_SLOT_NUM] = 1+force;

	return;
}

void
multi_send_robot_fire(int objnum, int gun_num, vms_vector *fire)
{
	// Send robot fire event
	int loc = 0;
	short s;
#ifdef WORDS_BIGENDIAN
	vms_vector swapped_vec;
#endif

	multibuf[loc] = MULTI_ROBOT_FIRE;						loc += 1;
	multibuf[loc] = Player_num;								loc += 1;
	s = objnum_local_to_remote(objnum, (sbyte *)&multibuf[loc+2]);
	PUT_INTEL_SHORT(multibuf+loc, s);
																		loc += 3;
	multibuf[loc] = gun_num;									loc += 1;
#ifndef WORDS_BIGENDIAN
	memcpy(multibuf+loc, fire, sizeof(vms_vector));         loc += sizeof(vms_vector); // 12
	// --------------------------
	//      Total = 18
#else
	swapped_vec.x = (fix)INTEL_INT((int)fire->x);
	swapped_vec.y = (fix)INTEL_INT((int)fire->y);
	swapped_vec.z = (fix)INTEL_INT((int)fire->z);
	memcpy(multibuf+loc, &swapped_vec, sizeof(vms_vector)); loc += sizeof(vms_vector);
#endif

	if (Objects[objnum].ctype.ai_info.REMOTE_OWNER == Player_num)
	{
		int slot = Objects[objnum].ctype.ai_info.REMOTE_SLOT_NUM;
		Assert((slot >= 0) && (slot < MAX_ROBOTS_CONTROLLED));
		if (robot_fired[slot] != 0)
			Int3(); // ROB!
		memcpy(robot_fire_buf[slot], multibuf, loc);
		robot_fired[slot] = 1;
	}
	else
		multi_send_data(multibuf, loc, 0); // Not our robot, send ASAP
}

void
multi_send_robot_explode(int objnum, int killer,char isthief)
{
	// Send robot explosion event to the other players

	int loc = 0;
	short s;

	multibuf[loc] = MULTI_ROBOT_EXPLODE;					loc += 1;
	multibuf[loc] = Player_num;								loc += 1;
	s = (short)objnum_local_to_remote(killer, (sbyte *)&multibuf[loc+2]);
	PUT_INTEL_SHORT(multibuf+loc, s);                       loc += 3;

	s = (short)objnum_local_to_remote(objnum, (sbyte *)&multibuf[loc+2]);
	PUT_INTEL_SHORT(multibuf+loc, s);                       loc += 3;

	(void)isthief;
	multi_send_data(multibuf, loc, 2);

	multi_delete_controlled_robot(objnum);
}

void
multi_send_create_robot(int station, int objnum, int type)
{
	// Send create robot information

	int loc = 0;

	multibuf[loc] = MULTI_CREATE_ROBOT;						loc += 1;
	multibuf[loc] = Player_num;								loc += 1;
	multibuf[loc] = (sbyte)station;                         loc += 1;
	PUT_INTEL_SHORT(multibuf+loc, objnum);                  loc += 2;
	multibuf[loc] = type;									loc += 1;

	map_objnum_local_to_local((short)objnum);

	multi_send_data(multibuf, loc, 2);
}

void
multi_send_boss_actions(int bossobjnum, int action, int secondary, int objnum)
{
	// Send special boss behavior information

	int loc = 0;
	
	multibuf[loc] = MULTI_BOSS_ACTIONS;						loc += 1;
	multibuf[loc] = Player_num;								loc += 1; // Which player is controlling the boss
	PUT_INTEL_SHORT(multibuf+loc, bossobjnum);              loc += 2; // We won't network map this objnum since it's the boss
	multibuf[loc] = (sbyte)action;                          loc += 1; // What is the boss doing?
	multibuf[loc] = (sbyte)secondary;                       loc += 1; // More info for what he is doing
	PUT_INTEL_SHORT(multibuf+loc, objnum);                  loc += 2; // Objnum of object created by gate-in action
	if (action == 3) {
		PUT_INTEL_SHORT(multibuf+loc, Objects[objnum].segnum); loc += 2; // Segment number object created in (for gate only)
	}
	else
																		loc += 2; // Dummy

	if (action == 1) { // Teleport releases robot
		// Boss is up for grabs after teleporting
		Assert((Objects[bossobjnum].ctype.ai_info.REMOTE_SLOT_NUM >= 0) && (Objects[bossobjnum].ctype.ai_info.REMOTE_SLOT_NUM < MAX_ROBOTS_CONTROLLED));
		multi_delete_controlled_robot(bossobjnum);
//		robot_controlled[Objects[bossobjnum].ctype.ai_info.REMOTE_SLOT_NUM] = -1;
//		Objects[bossobjnum].ctype.ai_info.REMOTE_OWNER = -1;
		Objects[bossobjnum].ctype.ai_info.REMOTE_SLOT_NUM = 5; // Hands-off period!
	}
	multi_send_data(multibuf, loc, 2);
}
			
#define MAX_ROBOT_POWERUPS 4

void
multi_send_create_robot_powerups(object *del_obj)
{
	// Send create robot information

	int loc = 0;
	int i;
#ifdef WORDS_BIGENDIAN
	vms_vector swapped_vec;
#endif

	multibuf[loc] = MULTI_CREATE_ROBOT_POWERUPS;				loc += 1;
	multibuf[loc] = Player_num;									loc += 1;
	multibuf[loc] = del_obj->contains_count;					loc += 1;
	multibuf[loc] = del_obj->contains_type; 					loc += 1;
	multibuf[loc] = del_obj->contains_id;						loc += 1;
	PUT_INTEL_SHORT(multibuf+loc, del_obj->segnum);		        loc += 2;
#ifndef WORDS_BIGENDIAN
	memcpy(multibuf+loc, &del_obj->pos, sizeof(vms_vector));	loc += 12;
#else
	swapped_vec.x = (fix)INTEL_INT((int)del_obj->pos.x);
	swapped_vec.y = (fix)INTEL_INT((int)del_obj->pos.y);
	swapped_vec.z = (fix)INTEL_INT((int)del_obj->pos.z);
	memcpy(multibuf+loc, &swapped_vec, sizeof(vms_vector));     loc += 12;
#endif

	memset(multibuf+loc, -1, MAX_ROBOT_POWERUPS*sizeof(short));

	if ((Net_create_loc > MAX_ROBOT_POWERUPS) || (Net_create_loc < 1))
	{
		Int3(); // See Rob
	}
	for (i = 0; i < Net_create_loc; i++)
	{
		PUT_INTEL_SHORT(multibuf+loc, Net_create_objnums[i]);
		loc += 2;
		map_objnum_local_to_local(Net_create_objnums[i]);
	}

	Net_create_loc = 0;

	multi_send_data(multibuf, 27, 2);
}

void
multi_do_claim_robot(const ubyte *buf)
{
	short botnum;
	short remote_botnum;
	char pnum;

	pnum = buf[1];

	remote_botnum = GET_INTEL_SHORT(buf + 2);
	botnum = objnum_remote_to_local(remote_botnum, (sbyte)buf[4]);

	if ((botnum > Highest_object_index) || (botnum < 0)) {
		return;
	}

	if (Objects[botnum].type != OBJ_ROBOT) {
		return;
	}
	
	if (Objects[botnum].ctype.ai_info.REMOTE_OWNER != -1)
	{
		if (MULTI_ROBOT_PRIORITY(remote_botnum, pnum) <= MULTI_ROBOT_PRIORITY(remote_botnum, Objects[botnum].ctype.ai_info.REMOTE_OWNER))
			return;
	}
	
	// Perform the requested change

	if (Objects[botnum].ctype.ai_info.REMOTE_OWNER == Player_num)
	{
		multi_delete_controlled_robot(botnum);
	}

	Objects[botnum].ctype.ai_info.REMOTE_OWNER = pnum;
	Objects[botnum].ctype.ai_info.REMOTE_SLOT_NUM = 0;
}

void
multi_do_release_robot(const ubyte *buf)
{
	short botnum, remote_botnum;
	char pnum;

	pnum = buf[1];

	remote_botnum = GET_INTEL_SHORT(buf + 2);
	botnum = objnum_remote_to_local(remote_botnum, (sbyte)buf[4]);

	if ((botnum < 0) || (botnum > Highest_object_index)) {
		return;
	}

	if (Objects[botnum].type != OBJ_ROBOT) {
		return;
	}
	
	if (Objects[botnum].ctype.ai_info.REMOTE_OWNER != pnum)
	{
		return;
	}
	
	// Perform the requested change

	Objects[botnum].ctype.ai_info.REMOTE_OWNER = -1;
	Objects[botnum].ctype.ai_info.REMOTE_SLOT_NUM = 0;
}

void
multi_do_robot_position(const ubyte *buf)
{
	// Process robot movement sent by another player

	short botnum, remote_botnum;
	char pnum;
	int loc = 1;
#ifdef WORDS_BIGENDIAN
	shortpos sp;
#endif

	pnum = buf[loc];										loc += 1;

	remote_botnum = GET_INTEL_SHORT(buf + loc);
	botnum = objnum_remote_to_local(remote_botnum, (sbyte)buf[loc+2]); loc += 3;

	if ((botnum < 0) || (botnum > Highest_object_index)) {
		return;
	}

	if ((Objects[botnum].type != OBJ_ROBOT) || (Objects[botnum].flags & OF_EXPLODING)) {
		return;
	}
		
	if (Objects[botnum].ctype.ai_info.REMOTE_OWNER != pnum)
	{	
		if (Objects[botnum].ctype.ai_info.REMOTE_OWNER == -1)
		{
			// Robot claim packet must have gotten lost, let this player claim it.
			if (Objects[botnum].ctype.ai_info.REMOTE_SLOT_NUM > 3) {
				Objects[botnum].ctype.ai_info.REMOTE_OWNER = pnum;
				Objects[botnum].ctype.ai_info.REMOTE_SLOT_NUM = 0;
			}
			else
				Objects[botnum].ctype.ai_info.REMOTE_SLOT_NUM++;
		}
		else
		{
			return;
		}
	}

	set_thrust_from_velocity(&Objects[botnum]); // Try to smooth out movement
//	Objects[botnum].phys_info.drag = Robot_info[Objects[botnum].id].drag >> 4; // Set drag to low

#ifndef WORDS_BIGENDIAN
	extract_shortpos(&Objects[botnum], (shortpos *)(buf+loc), 0);
#else
	memcpy((ubyte *)(sp.bytemat), (ubyte *)(buf + loc), 9);		loc += 9;
	memcpy((ubyte *)&(sp.xo), (ubyte *)(buf + loc), 14);
	extract_shortpos(&Objects[botnum], &sp, 1);
#endif
}

void
multi_do_robot_fire(const ubyte *buf)
{
	// Send robot fire event
	int loc = 1;
	int botnum;
	short remote_botnum;
	int gun_num;
	vms_vector fire, gun_point;
	robot_info *robptr;

	loc += 1; // pnum
	remote_botnum = GET_INTEL_SHORT(buf + loc);
	botnum = objnum_remote_to_local(remote_botnum, (sbyte)buf[loc+2]); loc += 3;
	gun_num = (sbyte)buf[loc];                                      loc += 1;
	memcpy(&fire, buf+loc, sizeof(vms_vector));
	fire.x = (fix)INTEL_INT((int)fire.x);
	fire.y = (fix)INTEL_INT((int)fire.y);
	fire.z = (fix)INTEL_INT((int)fire.z);

	if ((botnum < 0) || (botnum > Highest_object_index) || (Objects[botnum].type != OBJ_ROBOT) || (Objects[botnum].flags & OF_EXPLODING))
	{
		return;
	}
	
	// Do the firing
	
	if (gun_num == -1)
	{
		// Drop proximity bombs
		vm_vec_add(&gun_point, &Objects[botnum].pos, &fire);
		Laser_create_new_easy( &fire, &gun_point, botnum, PROXIMITY_ID, 1);
	}
	else 
	{
		calc_gun_point(&gun_point, &Objects[botnum], gun_num);
		robptr = &Robot_info[Objects[botnum].id];
		Laser_create_new_easy( &fire, &gun_point, botnum, robptr->weapon_type, 1);
	}
}

int
multi_explode_robot_sub(int botnum, int killer,char isthief)
{
	object *robot;

	killer = killer;

	if ((botnum < 0) || (botnum > Highest_object_index)) { // Objnum in range?
		Int3(); // See rob
		return 0;
	}

	if (Objects[botnum].type != OBJ_ROBOT) { // Object is robot?
		return 0;
	}

	if (Objects[botnum].flags & OF_EXPLODING) { // Object not already exploding
		return 0;
	}

	// Data seems valid, explode the sucker

	if (Network_send_objects && multi_objnum_is_past(botnum))
	{
		Network_send_objnum = -1;
	}

	robot = &Objects[botnum];

	// Drop non-random KEY powerups locally only!
	if ((robot->contains_count > 0) && (robot->contains_type == OBJ_POWERUP) && (Game_mode & GM_MULTI_COOP) && (robot->contains_id >= POW_KEY_BLUE) && (robot->contains_id <= POW_KEY_GOLD))
	{
		object_create_egg(robot);
	}
	else if (robot->ctype.ai_info.REMOTE_OWNER == Player_num) 
	{
		multi_drop_robot_powerups(robot-Objects);
		multi_delete_controlled_robot(robot-Objects);
	}
	else if (robot->ctype.ai_info.REMOTE_OWNER == -1 && multi_i_am_master())
	{
		multi_drop_robot_powerups(robot-Objects);
	}
	(void)isthief;

	if (Robot_info[robot->id].boss_flag) {
		if (!Boss_dying)
			start_boss_death_sequence(robot);
		else
			return 0;
	} else
		explode_object(robot, STANDARD_EXPL_DELAY);
	return 1;
}

void
multi_do_robot_explode(const ubyte *buf)
{
	// Explode robot controlled by other player

	int botnum;
	short remote_botnum;
	int loc = 1;
	short killer, remote_killer;
	int rval;
	char thief;

	loc += 1; // pnum
	remote_killer = GET_INTEL_SHORT(buf + loc);
	killer = objnum_remote_to_local(remote_killer, (sbyte)buf[loc+2]); loc += 3;
	remote_botnum = GET_INTEL_SHORT(buf + loc);
	botnum = objnum_remote_to_local(remote_botnum, (sbyte)buf[loc+2]); loc += 3;
	thief = 0;

	if ((botnum < 0) || (botnum > Highest_object_index)) {
		return;
	}

	rval = multi_explode_robot_sub(botnum, killer,thief);

	if (rval && (killer == Players[Player_num].objnum))
		add_points_to_score(Robot_info[Objects[botnum].id].score_value);

	if(multi_i_am_master() && (Game_mode & GM_MULTI_ROBOTS)) {
	    kill_respawnable_robot(Objects + botnum); 
	}
			    
}

extern fix EnergyToCreateOneRobot; // From fuelcen.c 
extern object *create_morph_robot(segment *segp, vms_vector *object_pos, int object_id); // from fuelcen.c

void
multi_do_create_robot(const ubyte *buf)
{
	
	int fuelcen_num = buf[2];
	int pnum = buf[1];
	short objnum;
	int type = buf[5];

	FuelCenter *robotcen;
	vms_vector cur_object_loc, direction;
	object *obj;

	objnum = GET_INTEL_SHORT(buf + 3);

	if ((pnum < 0) || (objnum < 0) || (fuelcen_num < 0) || (fuelcen_num >= Num_fuelcenters) || (pnum >= N_players))
	{
		Int3(); // Bogus data
		return;
	}

	robotcen = &Station[fuelcen_num];

	// Play effect and sound

	compute_segment_center(&cur_object_loc, &Segments[robotcen->segnum]);
	obj = object_create_explosion(robotcen->segnum, &cur_object_loc, i2f(10), VCLIP_MORPHING_ROBOT);
	if (obj)
		extract_orient_from_segment(&obj->orient, &Segments[robotcen->segnum]);
	if (Vclip[VCLIP_MORPHING_ROBOT].sound_num > -1)
		digi_link_sound_to_pos( Vclip[VCLIP_MORPHING_ROBOT].sound_num, robotcen->segnum, 0, &cur_object_loc, 0, F1_0 );

	// Set robot center flags, in case we become the master for the next one

	robotcen->Flag = 0;
	robotcen->Capacity -= EnergyToCreateOneRobot;
	robotcen->Timer = 0;

	obj = create_morph_robot(&Segments[robotcen->segnum], &cur_object_loc, type);
	if (obj == NULL)
		return; // Cannot create object!
	
	obj->matcen_creator = (robotcen-Station) | 0x80;
//	extract_orient_from_segment(&obj->orient, &Segments[robotcen->segnum]);
	vm_vec_sub( &direction, &ConsoleObject->pos, &obj->pos );
	vm_vector_2_matrix( &obj->orient, &direction, &obj->orient.uvec, NULL);
	morph_start( obj );

	map_objnum_local_to_remote(obj-Objects, objnum, pnum);

	Assert(obj->ctype.ai_info.REMOTE_OWNER == -1);
}

#define MAX_ROBOTS 1000
fix NextRespawnWave=0;
fix RespawnWaveLength=F1_0*30;
fix RobotRechargeTime=F1_0*60;
object respawnable_bots[MAX_ROBOTS]; 
short roboid_to_objid[MAX_ROBOTS];
fix robo_death_time[MAX_ROBOTS];
short num_respawnable_bots = 0; 
void reset_respawnable_bots() {
	num_respawnable_bots = 0; 
	NextRespawnWave=0;
	for(int i = 0; i < MAX_OBJECTS && num_respawnable_bots < MAX_ROBOTS; i++) {
		if (Objects[i].type == OBJ_ROBOT) {
			roboid_to_objid[num_respawnable_bots] = i;
			robo_death_time[num_respawnable_bots] = 0;
			respawnable_bots[num_respawnable_bots++] = Objects[i];
		}
	}
}

void kill_respawnable_robot(object *bot) {
	short id = bot - Objects;
	short roboid = -1;
	for(int i = 0; i < num_respawnable_bots; i++) {
		if(roboid_to_objid[i] == id) {
			roboid = i;
			break;
		}
	}

	if(roboid == -1)
		return;

	robo_death_time[roboid] = GameTime64;
	//respawn_robot(roboid); 
} 

short master_respawn_robot(short robo_id);

void check_robot_respawns() { 
	if(! multi_i_am_master()) { return; } 
	if(! (Game_mode & GM_MULTI_ROBOTS)) { return; }
	if( Game_mode & GM_MULTI_COOP ) { return; }

	if(GameTime64 > NextRespawnWave) {
		NextRespawnWave = GameTime64 + RespawnWaveLength; 
		for(int i = 0; i < num_respawnable_bots; i++) {
			if(robo_death_time[i] == 0) { continue; } // Still alive
			if(GameTime64 - robo_death_time[i] < RobotRechargeTime) { continue; }


			object *robo = respawnable_bots + i; 
			int player_too_close = 0; 
			for(int j = 0; j < MAX_PLAYERS; j++) {
				if(Players[j].connected) {
					fix dist = find_connected_distance(
						&Objects[Players[j].objnum].pos, 
						Objects[Players[j].objnum].segnum, 
						&robo->pos,
						robo->segnum, 
						15, WID_FLY_FLAG
						);

					if(dist < F1_0 * 100) {
						player_too_close = 1; 
						break;
					}
				}
			}

			if(! player_too_close) {
				short objnum = master_respawn_robot(i);

				// Tell everyone else to do the same
				multi_send_respawn_robot(objnum);
			}
		}
	}
 } 

void multi_send_respawn_robot(short objnum) {
	assert(multi_i_am_master()); 

	object *bot = Objects + objnum; 

	int loc = 0;

	multibuf[loc] = MULTI_RESPAWN_ROBOT;					loc += 1;
	multibuf[loc] = bot->id;								loc += 1;
	PUT_INTEL_SHORT(multibuf + loc, bot->segnum);           loc += 2; 
	PUT_INTEL_INT(multibuf + loc, bot->pos.x);              loc += 4;
	PUT_INTEL_INT(multibuf + loc, bot->pos.y);              loc += 4;
	PUT_INTEL_INT(multibuf + loc, bot->pos.z);              loc += 4;
	PUT_INTEL_INT(multibuf + loc, bot->orient.rvec.x);      loc += 4;
	PUT_INTEL_INT(multibuf + loc, bot->orient.rvec.y);      loc += 4;
	PUT_INTEL_INT(multibuf + loc, bot->orient.rvec.z);      loc += 4;
	PUT_INTEL_INT(multibuf + loc, bot->orient.uvec.x);      loc += 4;
	PUT_INTEL_INT(multibuf + loc, bot->orient.uvec.y);      loc += 4;
	PUT_INTEL_INT(multibuf + loc, bot->orient.uvec.z);      loc += 4;	
	PUT_INTEL_INT(multibuf + loc, bot->orient.fvec.x);      loc += 4;
	PUT_INTEL_INT(multibuf + loc, bot->orient.fvec.y);      loc += 4;
	PUT_INTEL_INT(multibuf + loc, bot->orient.fvec.z);      loc += 4;	
	PUT_INTEL_INT(multibuf + loc, bot->size);               loc += 4;
	multibuf[loc] = 0;                                      loc += 1; // player num
	PUT_INTEL_SHORT(multibuf + loc, objnum);                loc += 2; 
	multibuf[loc] = bot->ctype.ai_info.behavior;        	loc += 1; 

	multi_send_data(multibuf, loc, 2);
}

void multi_do_respawn_robot(const ubyte *buf) {

	int loc = 1; 
	short segnum; 
	vms_vector pos; 
	vms_matrix orient; 
	fix size;
	ubyte behavior; 

	ubyte id      = buf[loc];                      loc += 1; 
	segnum        = GET_INTEL_SHORT(buf + loc);    loc += 2; 
	pos.x         = GET_INTEL_INT(buf + loc);      loc += 4;
	pos.y         = GET_INTEL_INT(buf + loc);      loc += 4;
	pos.z         = GET_INTEL_INT(buf + loc);      loc += 4;
	orient.rvec.x = GET_INTEL_INT(buf + loc);      loc += 4;
	orient.rvec.y = GET_INTEL_INT(buf + loc);      loc += 4;
	orient.rvec.z = GET_INTEL_INT(buf + loc);      loc += 4;
	orient.uvec.x = GET_INTEL_INT(buf + loc);      loc += 4;
	orient.uvec.y = GET_INTEL_INT(buf + loc);      loc += 4;
	orient.uvec.z = GET_INTEL_INT(buf + loc);      loc += 4;
	orient.fvec.x = GET_INTEL_INT(buf + loc);      loc += 4;
	orient.fvec.y = GET_INTEL_INT(buf + loc);      loc += 4;
	orient.fvec.z = GET_INTEL_INT(buf + loc);      loc += 4;	
	size          = GET_INTEL_INT(buf + loc);      loc += 4;
	ubyte rplr    = buf[loc];                      loc += 1; 
	short robjnum = GET_INTEL_SHORT(buf + loc);    loc += 2; 
	behavior      = buf[loc];                      loc += 1; 

	short objnum = respawn_robot(id, segnum, pos, orient, size, behavior); 

	map_objnum_local_to_remote(objnum, robjnum, rplr); 
}

short master_respawn_robot(short robo_id) {
	object *robo = respawnable_bots + robo_id; 

	short objnum = respawn_robot(robo->id, robo->segnum, robo->pos, robo->orient, robo->size, robo->ctype.ai_info.behavior ); 

	roboid_to_objid[robo_id] = objnum;
	robo_death_time[robo_id] = 0;

	map_objnum_local_to_local(objnum);

	return objnum; 
}

short respawn_robot(ubyte id, short segnum, vms_vector pos, vms_matrix orient, fix size, ubyte behavior) // robo ID 
{
	robot_info	*robodata = &Robot_info[id];

	short objnum = obj_create(OBJ_ROBOT, id, segnum, &pos, &orient, size, CT_AI, MT_PHYSICS, RT_POLYOBJ);
	if ( objnum < 0 ) {
		return objnum;
	}

	object *objp = &Objects[objnum];

	objp->rtype.pobj_info.model_num = robodata->model_num;
	objp->rtype.pobj_info.subobj_flags = 0;

	objp->mtype.phys_info.mass = robodata->mass;
	objp->mtype.phys_info.drag = robodata->drag;

	objp->mtype.phys_info.flags |= (PF_LEVELLING);

	objp->shields = robodata->strength;
	
	//int default_behavior = AIB_NORMAL;
	//if (id == 10)						//	This is a toaster guy!
	//	default_behavior = AIB_RUN_FROM;

	init_ai_object(objp-Objects, behavior, -1 );		//	Note, -1 = segment this robot goes to to hide, should probably be something useful

	object_create_explosion(segnum, &pos, i2f(10), VCLIP_MORPHING_ROBOT );
	digi_link_sound_to_pos( Vclip[VCLIP_MORPHING_ROBOT].sound_num, segnum, 0,  &pos, 0 , F1_0);
	morph_start(objp);

	Players[Player_num].num_robots_level++;
	Players[Player_num].num_robots_total++;

	return objnum;
}

void
multi_do_boss_actions(const ubyte *buf)
{
	// Code to handle remote-controlled boss actions

	object *boss_obj;
	int boss_objnum;
	int pnum;
	int action, secondary;
	int loc = 1;
	short remote_objnum, segnum;

	pnum = buf[loc]; 									loc += 1;
	boss_objnum = GET_INTEL_SHORT(buf + loc);           loc += 2;
	action = buf[loc];									loc += 1;
	secondary = buf[loc];								loc += 1;
	remote_objnum = GET_INTEL_SHORT(buf + loc);         loc += 2;
	segnum = GET_INTEL_SHORT(buf + loc);                loc += 2;
	
	if ((boss_objnum < 0) || (boss_objnum > Highest_object_index))
	{
		Int3();  // See Rob
		return;
	}

	boss_obj = &Objects[boss_objnum];

	if ((boss_obj->type != OBJ_ROBOT) || !(Robot_info[boss_obj->id].boss_flag))
	{
		Int3(); // Got boss actions for a robot who's not a boss?
		return;
	}
		
	switch(action) 
	{
		case 1: // Teleport
			{	
				int teleport_segnum;
				vms_vector boss_dir;

				if ((secondary < 0) || (secondary > Num_boss_teleport_segs))
				{
					Int3(); // Bad segnum for boss teleport, ROB!!
					return;
				}
				teleport_segnum = Boss_teleport_segs[secondary];
				if ((teleport_segnum < 0) || (teleport_segnum > Highest_segment_index))
				{
					Int3();  // See Rob
					return;
				}
				compute_segment_center(&boss_obj->pos, &Segments[teleport_segnum]);
				obj_relink(boss_obj-Objects, teleport_segnum);
				Last_teleport_time = GameTime64;
		
				vm_vec_sub(&boss_dir, &Objects[Players[pnum].objnum].pos, &boss_obj->pos);
				vm_vector_2_matrix(&boss_obj->orient, &boss_dir, NULL, NULL);

				digi_link_sound_to_pos( Vclip[VCLIP_MORPHING_ROBOT].sound_num, teleport_segnum, 0, &boss_obj->pos, 0 , F1_0);
				digi_kill_sound_linked_to_object( boss_obj-Objects);
				digi_link_sound_to_object2( SOUND_BOSS_SHARE_SEE, boss_obj-Objects, 1, F1_0, F1_0*512 );	//	F1_0*512 means play twice as loud
				Ai_local_info[boss_obj-Objects].next_fire = 0;

				if (boss_obj->ctype.ai_info.REMOTE_OWNER == Player_num)
				{
					multi_delete_controlled_robot(boss_objnum);
//					robot_controlled[boss_obj->ctype.ai_info.REMOTE_SLOT_NUM] = -1;
				}

				boss_obj->ctype.ai_info.REMOTE_OWNER = -1; // Boss is up for grabs again!
				boss_obj->ctype.ai_info.REMOTE_SLOT_NUM = 0; // Available immediately!
			}
			break;
		case 2: // Cloak
			Boss_hit_this_frame = 0;
			Boss_cloak_start_time = GameTime64;
			Boss_cloak_end_time = GameTime64 + Boss_cloak_duration;
			boss_obj->ctype.ai_info.CLOAKED = 1;
			break;
		case 3: // Gate in robots!
			{
				// Do some validity checking
				if ( (remote_objnum >= MAX_OBJECTS) || (remote_objnum < 0) || (segnum < 0) || (segnum > Highest_segment_index) )
				{
					Int3(); // See Rob, bad data in boss gate action message
					return;
				}

				// Gate one in!
				if (gate_in_robot(secondary, segnum))
					map_objnum_local_to_remote(Net_create_objnums[0], remote_objnum, pnum);
			}
			break;
		case 4: // Start effect
			restart_effect(ECLIP_NUM_BOSS);
			break;
		case 5:	// Stop effect
			stop_effect(ECLIP_NUM_BOSS);
			break;
		default:
			Int3(); // Illegal type to boss actions
	}
}

void
multi_do_create_robot_powerups(const ubyte *buf)
{
	// Code to drop remote-controlled robot powerups

	int loc = 1;
	object del_obj;
	int pnum, egg_objnum, i;

	memset( &del_obj, 0, sizeof(object) );
	del_obj.type = OBJ_ROBOT;

	pnum = buf[loc];					loc += 1;
	del_obj.contains_count = buf[loc];			loc += 1;
	del_obj.contains_type = buf[loc];			loc += 1;
	del_obj.contains_id = buf[loc]; 			loc += 1;
	del_obj.segnum = GET_INTEL_SHORT(buf + loc);            loc += 2;
	memcpy(&del_obj.pos, buf+loc, sizeof(vms_vector));      loc += 12;
	
	vm_vec_zero(&del_obj.mtype.phys_info.velocity);

	del_obj.pos.x = (fix)INTEL_INT((int)del_obj.pos.x);
	del_obj.pos.y = (fix)INTEL_INT((int)del_obj.pos.y);
	del_obj.pos.z = (fix)INTEL_INT((int)del_obj.pos.z);

	Assert((pnum >= 0) && (pnum < N_players));

	Net_create_loc = 0;
	d_srand(1245L);

	egg_objnum = object_create_egg(&del_obj);

	if (egg_objnum == -1)
		return; // Object buffer full

//	Assert(egg_objnum > -1);
	Assert((Net_create_loc > 0) && (Net_create_loc <= MAX_ROBOT_POWERUPS));

	for (i = 0; i < Net_create_loc; i++)
	{
		short s;
		
		s = GET_INTEL_SHORT(buf + loc);
		if ( s != -1)
			map_objnum_local_to_remote((short)Net_create_objnums[i], s, pnum);
		else
			Objects[Net_create_objnums[i]].flags |= OF_SHOULD_BE_DEAD; // Delete objects other guy didn't create one of
		loc += 2;
	}
}

void
multi_drop_robot_powerups(int objnum)
{
	// Code to handle dropped robot powerups in network mode ONLY!

	object *del_obj;
	int egg_objnum = -1;
	robot_info	*robptr; 

	if ((objnum < 0) || (objnum > Highest_object_index))
	{
		Int3();  // See rob
		return;
	}
	
	del_obj = &Objects[objnum];

	if (del_obj->type != OBJ_ROBOT)
	{
		Int3(); // dropping powerups for non-robot, Rob's fault
		return;
	}

	robptr = &Robot_info[del_obj->id];

	if(Game_mode & GM_MULTI_ROBOTS)
		return; 

	Net_create_loc = 0;

	if (del_obj->contains_count > 0) { 
		//	If dropping a weapon that the player has, drop energy instead, unless it's vulcan, in which case drop vulcan ammo.
		if (del_obj->contains_type == OBJ_POWERUP) {
			maybe_replace_powerup_with_energy(del_obj);

			// No key drops in non-coop games!
			if (!(Game_mode & GM_MULTI_COOP)) {
				if ((del_obj->contains_id >= POW_KEY_BLUE) && (del_obj->contains_id <= POW_KEY_GOLD))
					del_obj->contains_count = 0;
			}
		}
		d_srand(1245L);
		if (del_obj->contains_count > 0)
			egg_objnum = object_create_egg(del_obj);
	}
		
	else if (del_obj->ctype.ai_info.REMOTE_OWNER == -1) // No random goodies for robots we weren't in control of
		return;

	else if (robptr->contains_count) {
		d_srand((fix)timer_query());
		if (((d_rand() * 16) >> 15) < robptr->contains_prob) {
			del_obj->contains_count = ((d_rand() * robptr->contains_count) >> 15) + 1;
			del_obj->contains_type = robptr->contains_type;
			del_obj->contains_id = robptr->contains_id;
			if (del_obj->contains_type == OBJ_POWERUP)
				maybe_replace_powerup_with_energy(del_obj);
			d_srand(1245L);
			if (del_obj->contains_count > 0)
				egg_objnum = object_create_egg(del_obj);
		}
	}

	if (egg_objnum >= 0) {
		// Transmit the object creation to the other players	 	
		multi_send_create_robot_powerups(del_obj);
	}
}

//	-----------------------------------------------------------------------------
//	Robot *robot got whacked by player player_num and requests permission to do something about it.
//	Note: This function will be called regardless of whether Game_mode is a multiplayer mode, so it
//	should quick-out if not in a multiplayer mode.  On the other hand, it only gets called when a
//	player or player weapon whacks a robot, so it happens rarely.
void multi_robot_request_change(object *robot, int player_num)
{
	int slot, remote_objnum;
	sbyte dummy;

	if (!(Game_mode & GM_MULTI_ROBOTS))
		return;
	
	if (robot->ctype.ai_info.REMOTE_OWNER != Player_num)
		return;

	slot = robot->ctype.ai_info.REMOTE_SLOT_NUM;

	if ((slot < 0) || (slot >= MAX_ROBOTS_CONTROLLED)) {
		Int3();
		return;
	}

	remote_objnum = objnum_local_to_remote(robot-Objects, &dummy);
	if (remote_objnum < 0)
		return;

	if ( (robot_agitation[slot] < 70) || (MULTI_ROBOT_PRIORITY(remote_objnum, player_num) > MULTI_ROBOT_PRIORITY(remote_objnum, Player_num)) || (d_rand() > 0x4400))
	{
		if (robot_send_pending[slot])
			multi_send_robot_position(robot_controlled[slot], -1);
		multi_send_release_robot(robot_controlled[slot]);
		robot->ctype.ai_info.REMOTE_SLOT_NUM = 5;  // Hands-off period
	}
}

#endif
#endif
