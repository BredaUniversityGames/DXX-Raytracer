
/*
 *
 * Player Stuff
 *
 */

#include "player.h"
#include "multi.h"
#include "hudmsg.h"
#include "byteswap.h"

int RespawningConcussions[MAX_PLAYERS]; 

void player_rw_swap(player_rw *p, int swap)
{
	int i;

	if (!swap)
		return;

	p->objnum = SWAPINT(p->objnum);
	p->n_packets_got = SWAPINT(p->n_packets_got);
	p->n_packets_sent = SWAPINT(p->n_packets_sent);
	p->flags = SWAPINT(p->flags);
	p->energy = SWAPINT(p->energy);
	p->shields = SWAPINT(p->shields);
	p->killer_objnum = SWAPSHORT(p->killer_objnum);
	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		p->primary_ammo[i] = SWAPSHORT(p->primary_ammo[i]);
	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
		p->secondary_ammo[i] = SWAPSHORT(p->secondary_ammo[i]);
	p->last_score = SWAPINT(p->last_score);
	p->score = SWAPINT(p->score);
	p->time_level = SWAPINT(p->time_level);
	p->time_total = SWAPINT(p->time_total);
	p->cloak_time = SWAPINT(p->cloak_time);
	p->invulnerable_time = SWAPINT(p->invulnerable_time);
	p->net_killed_total = SWAPSHORT(p->net_killed_total);
	p->net_kills_total = SWAPSHORT(p->net_kills_total);
	p->num_kills_level = SWAPSHORT(p->num_kills_level);
	p->num_kills_total = SWAPSHORT(p->num_kills_total);
	p->num_robots_level = SWAPSHORT(p->num_robots_level);
	p->num_robots_total = SWAPSHORT(p->num_robots_total);
	p->hostages_rescued_total = SWAPSHORT(p->hostages_rescued_total);
	p->hostages_total = SWAPSHORT(p->hostages_total);
	p->homing_object_dist = SWAPINT(p->homing_object_dist);
}

void player_ship_read(player_ship *ps, PHYSFS_file *fp)
{
	int i;
	ps->model_num = PHYSFSX_readInt(fp);
	ps->expl_vclip_num = PHYSFSX_readInt(fp);
	ps->mass = PHYSFSX_readFix(fp);
	ps->drag = PHYSFSX_readFix(fp);
	ps->max_thrust = PHYSFSX_readFix(fp);
	ps->reverse_thrust = PHYSFSX_readFix(fp);
	ps->brakes = PHYSFSX_readFix(fp);
	ps->wiggle = PHYSFSX_readFix(fp);
	ps->max_rotthrust = PHYSFSX_readFix(fp);
	for (i = 0; i < N_PLAYER_GUNS; i++)
		PHYSFSX_readVector(&ps->gun_points[i], fp);
}

void reset_obs() {
	if (Current_obs_player == OBSERVER_PLAYER_ID)
		return;
	
	Current_obs_player = OBSERVER_PLAYER_ID;
	Objects[Players[OBSERVER_PLAYER_ID].objnum].pos = Last_pos;
	Objects[Players[OBSERVER_PLAYER_ID].objnum].orient = Last_orient;
	ConsoleObject->pos = Last_pos;
	ConsoleObject->orient = Last_orient;
	HUD_init_message_literal(HM_MULTI, "Observing freely.");
}

void set_obs(int pnum) {
	if (Current_obs_player == OBSERVER_PLAYER_ID) {
		Last_pos = Objects[Players[OBSERVER_PLAYER_ID].objnum].pos;
		Last_orient = Objects[Players[OBSERVER_PLAYER_ID].objnum].orient;
	}

	if (Players[pnum].connected == CONNECT_PLAYING) {
		if (Current_obs_player != pnum) {
			HUD_init_message(HM_DEFAULT, "Observing %s!", Players[pnum].callsign);
		}
		Current_obs_player = pnum;
	} else {
		reset_obs();
	}
}
