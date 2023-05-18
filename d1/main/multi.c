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
 * Multiplayer code for network play.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "game.h"
#include "multi.h"
#include "object.h"
#include "laser.h"
#include "fuelcen.h"
#include "scores.h"
#include "gauges.h"
#include "collide.h"
#include "dxxerror.h"
#include "fireball.h"
#include "newmenu.h"
#include "console.h"
#include "wall.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "polyobj.h"
#include "bm.h"
#include "endlevel.h"
#include "key.h"
#include "playsave.h"
#include "timer.h"
#include "digi.h"
#include "sounds.h"
#include "newdemo.h"
#include "text.h"
#include "kmatrix.h"
#include "multibot.h"
#include "gameseq.h"
#include "physics.h"
#include "config.h"
#include "hudmsg.h"
#include "ctype.h"  // for isalpha
#include "vers_id.h"
#include "byteswap.h"
#include "pstypes.h"
#include "strutil.h"
#include "u_mem.h"
#include "state.h"
#ifdef USE_UDP
#include "net_udp.h"
#endif
#include "args.h"
#include "logger.h"

//
// Local macros and prototypes
//

// LOCALIZE ME!!

#define vm_angvec_zero(v) (v)->p=(v)->b=(v)->h=0

void reset_player_object(void); // In object.c but not in object.h
void multi_reset_object_texture(object *objp);
void drop_player_eggs(object *playerobj); // from collide.c
void drop_player_eggs_remote(object *playerobj, ubyte remote); // from collide.c
void multi_do_heartbeat(const ubyte *buf);
void multi_send_heartbeat();
void multi_do_kill_goal_counts(const ubyte *buf);
void multi_powcap_cap_objects();
void multi_powcap_adjust_remote_cap(int pnum);
void multi_send_ranking();
void multi_new_bounty_target( int pnum );
void multi_do_bounty( const ubyte *buf );
void multi_save_game(ubyte slot, uint id, char *desc);
void multi_restore_game(ubyte slot, uint id);
void multi_do_save_game(const ubyte *buf);
void multi_do_restore_game(const ubyte *buf);
void multi_do_msgsend_state(const ubyte *buf);
void multi_send_msgsend_state(int state);
void multi_send_gmode_update();
void multi_do_gmode_update(const ubyte *buf);

//
// Global variables
//

int multi_protocol=0; // set and determinate used protocol
int imulti_new_game=0; // to prep stuff for level only when starting new game

int who_killed_controlcen = -1;  // -1 = noone

//do we draw the kill list on the HUD?
int Show_kill_list = 1;
int Show_network_stats = 1; 
int Show_reticle_name = 1;
fix Show_kill_list_timer = 0;

sbyte PKilledFlags[MAX_PLAYERS];
int Bounty_target = 0;

int multi_sending_message[MAX_PLAYERS] = { 0,0,0,0,0,0,0,0 };
int multi_defining_message = 0;
int multi_message_index = 0;

ubyte multibuf[MAX_MULTI_MESSAGE_LEN+4];		// This is where multiplayer message are built
unsigned char multibuf2[MAX_MULTI_MESSAGE_LEN+4];

short remote_to_local[MAX_PLAYERS][MAX_OBJECTS];  // Remote object number for each local object
short local_to_remote[MAX_OBJECTS];
sbyte object_owner[MAX_OBJECTS];   // Who created each object in my universe, -1 = loaded at start
int early_resp[MAX_PLAYERS]; // HACK in case we ger REAPPEAR packet before EXPLODE

int   Net_create_objnums[MAX_NET_CREATE_OBJECTS]; // For tracking object creation that will be sent to remote
int   Net_create_loc = 0;       // pointer into previous array
int   Network_status = 0;
char  Network_message[MAX_MESSAGE_LEN];
int   Network_message_reciever=-1;
int   sorted_kills[MAX_PLAYERS];
short kill_matrix[MAX_PLAYERS][MAX_PLAYERS];
int   multi_goto_secret = 0;
short team_kills[2];
int   multi_quit_game = 0;
const char GMNames[MULTI_GAME_TYPE_COUNT][MULTI_GAME_NAME_LENGTH]={
	"Anarchy",
	"Team Anarchy",
	"Robo Anarchy",
	"Cooperative",
	"Unknown",
	"Unknown",
	"Unknown",
	"Bounty"
};
const char GMNamesShrt[MULTI_GAME_TYPE_COUNT][8]={
	"ANRCHY",
	"TEAM",
	"ROBO",
	"COOP",
	"UNKNOWN",
	"UNKNOWN",
	"UNKNOWN",
	"BOUNTY"
};

int Current_obs_player = OBSERVER_PLAYER_ID; // Current player being observed. Defaults to the observer player ID.
bool Obs_at_distance = 0; // True if you're viewing the player from a cube back.

// For rejoin object syncing (used here and all protocols - globally)

int	Network_send_objects = 0;  // Are we in the process of sending objects to a player?
int	Network_send_object_mode = 0; // What type of objects are we sending, static or dynamic?
int 	Network_send_objnum = -1;   // What object are we sending next?
int     Network_rejoined = 0;       // Did WE rejoin this game?
int     Network_sending_extras=0;
int     VerifyPlayerJoined=-1;      // Player (num) to enter game before any ingame/extra stuff is being sent
int     Player_joining_extras=-1;  // This is so we know who to send 'latecomer' packets to.
int     Network_player_added = 0;   // Is this a new player or a returning player?

ushort          my_segments_checksum = 0;

netgame_info Netgame;

bitmap_index multi_player_textures[MAX_PLAYERS][N_PLAYER_SHIP_TEXTURES];

// Globals for protocol-bound Refuse-functions
char RefuseThisPlayer=0,WaitForRefuseAnswer=0,RefuseTeam,RefusePlayerName[12];
fix64 RefuseTimeLimit=0;

char PowerupsInMine[MAX_POWERUP_TYPES],MaxPowerupsAllowed[MAX_POWERUP_TYPES];
extern fix ThisLevelTime;
extern void init_player_stats_new_ship(ubyte pnum);

static const int message_length[] = {
#define define_message_length(NAME,SIZE)	(SIZE),
	for_each_multiplayer_command(, define_message_length, )
};

void multi_reset_player_object(object *objp);
void multi_set_robot_ai(void);
void multi_add_lifetime_killed();
void multi_add_lifetime_kills();

char *RankStrings[]={"(unpatched) ","Cadet ","Ensign ","Lieutenant ","Lt.Commander ",
                     "Commander ","Captain ","Vice Admiral ","Admiral ","Demigod "};

int multi_allow_powerup_mask[MAX_POWERUP_TYPES] =
{ NETFLAG_DOINVUL, 0, 0, NETFLAG_DOLASER, 0, 0, 0, 0, 0, 0, 0, 0, NETFLAG_DOQUAD,
  NETFLAG_DOVULCAN, NETFLAG_DOSPREAD, NETFLAG_DOPLASMA, NETFLAG_DOFUSION,
  NETFLAG_DOPROXIM, NETFLAG_DOHOMING, NETFLAG_DOHOMING, NETFLAG_DOSMART,
  NETFLAG_DOMEGA, NETFLAG_DOVULCANAMMO, NETFLAG_DOCLOAK, 0, NETFLAG_DOINVUL, 0, 0, 0 };

char *multi_allow_powerup_text[MULTI_ALLOW_POWERUP_MAX] =
{
#define define_netflag_string(NAME,STR)	STR,
	for_each_netflag_value(define_netflag_string)
};

int GetMyNetRanking()
{
	int rank, eff;

	if (PlayerCfg.NetlifeKills+PlayerCfg.NetlifeKilled==0)
		return (1);

	rank=(int) (((float)PlayerCfg.NetlifeKills/3000.0)*8.0);

	eff=(int)((float)((float)PlayerCfg.NetlifeKills/((float)PlayerCfg.NetlifeKilled+(float)PlayerCfg.NetlifeKills))*100.0);

	if (rank>8)
		rank=8;

	if (eff<0)
		eff=0;

	if (eff<60)
		rank-=((59-eff)/10);

	if (rank<0)
		rank=0;
	if (rank>8)
		rank=8;

	return (rank+1);
}

void ClipRank (ubyte *rank)
{
	// This function insures no crashes when dealing with D2 1.0
	if (*rank > 9)
		*rank = 0;
}  

//
//  Functions that replace what used to be macros
//

int objnum_remote_to_local(int remote_objnum, int owner)
{
	// Map a remote object number from owner to a local object number

	int result;

	if ((owner >= N_players) || (owner < -1)) {
		Int3(); // Illegal!
		return(remote_objnum);
	}

	if (owner == -1)
		return(remote_objnum);

	if ((remote_objnum < 0) || (remote_objnum >= MAX_OBJECTS))
		return(-1);

	result = remote_to_local[owner][remote_objnum];

	if (result < 0)
	{
		return(-1);
	}

	return(result);
}

int objnum_local_to_remote(int local_objnum, sbyte *owner)
{
	// Map a local object number to a remote + owner

	int result;

	if ((local_objnum < 0) || (local_objnum > Highest_object_index)) {
		*owner = -1;
		return(-1);
	}

	*owner = object_owner[local_objnum];

	if (*owner == -1)
		return(local_objnum);

	if ((*owner >= N_players) || (*owner < -1)) {
		Int3(); // Illegal!
		*owner = -1;
		return local_objnum;
	}

	result = local_to_remote[local_objnum];

	if (result < 0)
	{
		Int3(); // See Rob, object has no remote number!
	}

	return(result);
}

void
map_objnum_local_to_remote(int local_objnum, int remote_objnum, int owner)
{
	// Add a mapping from a network remote object number to a local one

	Assert(local_objnum > -1);
	Assert(remote_objnum > -1);
	Assert(owner > -1);
	Assert(owner != Player_num);
	Assert(local_objnum < MAX_OBJECTS);
	Assert(remote_objnum < MAX_OBJECTS);

	object_owner[local_objnum] = owner;

	remote_to_local[owner][remote_objnum] = local_objnum;
	local_to_remote[local_objnum] = remote_objnum;

	return;
}

void
map_objnum_local_to_local(int local_objnum)
{
	// Add a mapping for our locally created objects

	Assert(local_objnum > -1);
	Assert(local_objnum < MAX_OBJECTS);

	object_owner[local_objnum] = Player_num;
	remote_to_local[Player_num][local_objnum] = local_objnum;
	local_to_remote[local_objnum] = local_objnum;

	return;
}

void reset_network_objects()
{
	memset(local_to_remote, -1, MAX_OBJECTS*sizeof(short));
	memset(remote_to_local, -1, MAX_PLAYERS*MAX_OBJECTS*sizeof(short));
	memset(object_owner, -1, MAX_OBJECTS);
}

int multi_objnum_is_past(int objnum)
{
	switch (multi_protocol)
	{
		case MULTI_PROTO_UDP:
#ifdef USE_UDP
			return net_udp_objnum_is_past(objnum);
			break;
#endif
		default:
			RT_LOG(RT_LOGSERVERITY_HIGH, "Protocol handling missing in multi_objnum_is_past\n");
			break;
	}
}

//
// Part 1 : functions whose main purpose in life is to divert the flow
//          of execution to either network  specific code based
//          on the curretn Game_mode value.
//

// Show a score list to end of net players
void multi_endlevel_score(void)
{
	int i, old_connect=0, game_wind_visible = 0;

	// If there still is a Game_wind and it's suspended (usually both shoudl be the case), bring it up again so host can still take actions of the game
	if (Game_wind)
	{
		if (!window_is_visible(Game_wind))
		{
			game_wind_visible = 1;
			window_set_visible(Game_wind, 1);
		}
	}

	// Save connect state and change to new connect state
#ifdef NETWORK
	if (Game_mode & GM_NETWORK)
	{
		old_connect = Players[Player_num].connected;
		if (Players[Player_num].connected!=CONNECT_DIED_IN_MINE)
			Players[Player_num].connected = CONNECT_END_MENU;
	}
#endif

#ifdef NETWORK
	Network_status = NETSTAT_ENDLEVEL;
#endif

	kmatrix_view(Game_mode & GM_NETWORK);

	// Restore connect state
	if (Game_mode & GM_NETWORK)
		Players[Player_num].connected = old_connect;

	if (Game_mode & GM_MULTI_COOP)
	{
		for (i = 0; i < Netgame.max_numplayers; i++)
			// Reset keys
			Players[i].flags &= ~(PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY);
	}

	for (i=0;i<MAX_PLAYERS;i++)
		Players[i].KillGoalCount=0;

	for(i=0; i < 2; i++) {
		Netgame.TeamKillGoalCount[i] = 0; 
	}

	for (i=0;i<MAX_POWERUP_TYPES;i++)
	{
		MaxPowerupsAllowed[i]=0;
		PowerupsInMine[i]=0;
	}

	// hide Game_wind again if we brought it up
	if (Game_wind && game_wind_visible)
		window_set_visible(Game_wind, 0);
}

int
get_team(int pnum)
{
	if (Netgame.team_vector & (1 << pnum))
		return 1;
	else
		return 0;
}

void
multi_new_game(void)
{
	int i;

	// Reset variables for a new net game

	for (i = 0; i < MAX_PLAYERS; i++)
		init_player_stats_game(i);

	memset(kill_matrix, 0, sizeof(kill_matrix)); // Clear kill matrix

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		sorted_kills[i] = i;
		Players[i].connected = CONNECT_DISCONNECTED;

		if (Current_obs_player == i) {
			reset_obs();
		}

		Players[i].net_killed_total = 0;
		Players[i].net_kills_total = 0;
		Players[i].flags = 0;
		Players[i].KillGoalCount=0;
		multi_sending_message[i] = 0;
	}

	for(i=0; i < 2; i++) {
		Netgame.TeamKillGoalCount[i] = 0; 
	}

	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
	{
		robot_controlled[i] = -1;
		robot_agitation[i] = 0;
		robot_fired[i] = 0;
	}

	for (i=0;i<MAX_POWERUP_TYPES;i++)
	{
		MaxPowerupsAllowed[i]=0;
		PowerupsInMine[i]=0;
	}

	team_kills[0] = team_kills[1] = 0;
	imulti_new_game=1;
	multi_quit_game = 0;
	Show_kill_list = 1;
	game_disable_cheats();
	multi_received_objects = 0; 
}

void
multi_make_player_ghost(int playernum)
{
	object *obj;

//	Assert(playernum != Player_num);
//	Assert(playernum < MAX_PLAYERS);

	if ((playernum == Player_num) || (playernum >= MAX_PLAYERS) || (playernum < 0))
	{
		Int3(); // Non-terminal, see Rob
		return;
	}

	obj = &Objects[Players[playernum].objnum];

	obj->type = OBJ_GHOST;
	obj->render_type = RT_NONE;
	obj->movement_type = MT_NONE;
	multi_reset_player_object(obj);

	if (Game_mode & GM_MULTI_ROBOTS)
		multi_strip_robots(playernum);
}

void
multi_make_ghost_player(int playernum)
{
	object *obj;

//	Assert(playernum != Player_num);
// Assert(playernum < MAX_PLAYERS);

	if ((playernum == Player_num) || (playernum >= MAX_PLAYERS))
	{
		Int3(); // Non-terminal, see rob
		return;
	}

	obj = &Objects[Players[playernum].objnum];

	obj->type = OBJ_PLAYER;
	obj->movement_type = MT_PHYSICS;
	multi_reset_player_object(obj);
	if (playernum != Player_num)
		init_player_stats_new_ship(playernum);
}

int multi_get_kill_list(int *plist)
{
	// Returns the number of active net players and their
	// sorted order of kills
	int i;
	int n = 0;

	for (i = 0; i < N_players; i++)
		//if (Players[sorted_kills[i]].connected)
		plist[n++] = sorted_kills[i];

	if (n == 0)
		Int3(); // SEE ROB OR MATT

	//memcpy(plist, sorted_kills, N_players*sizeof(int));

	return(n);
}

void
multi_sort_kill_list(void)
{
	// Sort the kills list each time a new kill is added

	int kills[MAX_PLAYERS];
	int i;
	int changed = 1;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if ((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_MULTI_ROBOTS))
			kills[i] = Players[i].score;
		else
			kills[i] = Players[i].net_kills_total;
	}

	while (changed)
	{
		changed = 0;
		for (i = 0; i < N_players-1; i++)
		{
			if (kills[sorted_kills[i]] < kills[sorted_kills[i+1]])
			{
				changed = sorted_kills[i];
				sorted_kills[i] = sorted_kills[i+1];
				sorted_kills[i+1] = changed;
				changed = 1;
			}
		}
	}
}

void robo_anarchy_suicide_penalty() {
	if(Game_mode & GM_MULTI_ROBOTS) {
		if(Players[Player_num].score > 1000) {
			Players[Player_num].score -= 1000;
		} else {
			Players[Player_num].score = 0; 
		}
		
		multi_send_score();
	}
}

extern object *obj_find_first_of_type (int);

void multi_compute_kill(int killer, int killed)
{
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.

	int killed_pnum, killed_type;
	int killer_pnum, killer_type;
	int TheGoal;
	char killed_name[(CALLSIGN_LEN*2)+4];
	char killer_name[(CALLSIGN_LEN*2)+4];

	// Both object numbers are localized already!

	if ((killed < 0) || (killed > Highest_object_index) || (killer < 0) || (killer > Highest_object_index))
	{
		Int3(); // See Rob, illegal value passed to compute_kill;
		return;
	}

	killed_type = Objects[killed].type;
	killer_type = Objects[killer].type;

	if ((killed_type != OBJ_PLAYER) && (killed_type != OBJ_GHOST))
	{
		Int3(); // compute_kill passed non-player object!
		return;
	}

	killed_pnum = Objects[killed].id;

	Assert ((killed_pnum >= 0) && (killed_pnum < N_players));

	if (Game_mode & GM_TEAM)
		sprintf(killed_name, "%s (%s)", Players[killed_pnum].callsign, Netgame.team_name[get_team(killed_pnum)]);
	else
		sprintf(killed_name, "%s", Players[killed_pnum].callsign);

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_death(killed_pnum);

	digi_play_sample( SOUND_HUD_KILL, F3_0 );

	if (killer_type == OBJ_CNTRLCEN)
	{
		Players[killed_pnum].net_killed_total++;
		Players[killed_pnum].net_kills_total--;

		if (Game_mode & GM_TEAM)
		{
				team_kills[get_team(killed_pnum)] -= 1;
		}

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_kill(killed_pnum, -1);

		if (killed_pnum == Player_num)
		{
			HUD_init_message(HM_MULTI, "%s %s.", TXT_YOU_WERE, TXT_KILLED_BY_NONPLAY);
			multi_add_lifetime_killed ();

			robo_anarchy_suicide_penalty();
		}
		else
			HUD_init_message(HM_MULTI, "%s %s %s.", killed_name, TXT_WAS, TXT_KILLED_BY_NONPLAY );
		return;
	}

	else if ((killer_type != OBJ_PLAYER) && (killer_type != OBJ_GHOST))
	{
			if (killed_pnum == Player_num)
			{
				HUD_init_message(HM_MULTI, "%s %s.", TXT_YOU_WERE, TXT_KILLED_BY_ROBOT);
				multi_add_lifetime_killed();

				robo_anarchy_suicide_penalty();
			}
			else
				HUD_init_message(HM_MULTI, "%s %s %s.", killed_name, TXT_WAS, TXT_KILLED_BY_ROBOT );
		Players[killed_pnum].net_killed_total++;
		return;
	}

	killer_pnum = Objects[killer].id;

	if (Game_mode & GM_TEAM)
		sprintf(killer_name, "%s (%s)", Players[killer_pnum].callsign, Netgame.team_name[get_team(killer_pnum)]);
	else
		sprintf(killer_name, "%s", Players[killer_pnum].callsign);

	// Beyond this point, it was definitely a player-player kill situation

	if ((killer_pnum < 0) || (killer_pnum >= N_players))
		Int3(); // See rob, tracking down bug with kill HUD messages
	if ((killed_pnum < 0) || (killed_pnum >= N_players))
		Int3(); // See rob, tracking down bug with kill HUD messages

	if (killer_pnum == killed_pnum)
	{
		if (Game_mode & GM_TEAM)
		{
			team_kills[get_team(killed_pnum)] -= 1;
			Netgame.TeamKillGoalCount[get_team(killed_pnum)] -= 1; 
		}

		robo_anarchy_suicide_penalty();				

		Players[killed_pnum].net_killed_total += 1;
		Players[killed_pnum].net_kills_total -= 1;
		Players[killer_pnum].KillGoalCount -=1; // Suicides count against kill goal

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_kill(killed_pnum, -1);

		kill_matrix[killed_pnum][killed_pnum] += 1; // # of suicides
		if (killer_pnum == Player_num)
		{
			HUD_init_message(HM_MULTI, "%s %s %s!", TXT_YOU, TXT_KILLED, TXT_YOURSELF );
			multi_add_lifetime_killed();
		}
		else
			HUD_init_message(HM_MULTI, "%s %s", killed_name, TXT_SUICIDE);

		/* Bounty mode needs some lovin' */
		if( Game_mode & GM_BOUNTY && killed_pnum == Bounty_target && multi_i_am_master() )
		{
			/* Select a random number */
			int new = d_rand() % MAX_PLAYERS;
			
			/* Make sure they're valid: Don't check against kill flags,
			* just in case everyone's dead! */
			while( !Players[new].connected )
				new = d_rand() % MAX_PLAYERS;
			
			/* Select new target  - it will be sent later when we're done with this function */
			multi_new_bounty_target( new );
		}
	}

	else
	{
		if (Game_mode & GM_TEAM)
		{
			if (get_team(killed_pnum) == get_team(killer_pnum))
			{
				team_kills[get_team(killed_pnum)] -= 1;
				Players[killer_pnum].net_kills_total -= 1;
				Netgame.TeamKillGoalCount[get_team(killer_pnum)] -= 1; 
			}
			else
			{
				team_kills[get_team(killer_pnum)] += 1;
				Players[killer_pnum].net_kills_total += 1;
				Players[killer_pnum].KillGoalCount +=1;
				Netgame.TeamKillGoalCount[get_team(killer_pnum)] += 1; 
			}
		}
		else if( Game_mode & GM_BOUNTY )
		{
			/* Did the target die?  Did the target get a kill? */
			if( killed_pnum == Bounty_target || killer_pnum == Bounty_target )
			{
				/* Increment kill counts */
				Players[killer_pnum].net_kills_total++;
				Players[killer_pnum].KillGoalCount++;
				
				/* Record the kill in a demo */
				if( Newdemo_state == ND_STATE_RECORDING )
					newdemo_record_multi_kill( killer_pnum, 1 );
				
				/* If the target died, the new one is set! */
				if( killed_pnum == Bounty_target )
					multi_new_bounty_target( killer_pnum );
			}
		}
		else
		{
			Players[killer_pnum].net_kills_total += 1;
			Players[killer_pnum].KillGoalCount+=1;
		}
		
			if (Newdemo_state == ND_STATE_RECORDING && !( Game_mode & GM_BOUNTY ) )
				newdemo_record_multi_kill(killer_pnum, 1);

		Players[killed_pnum].net_killed_total += 1;
		kill_matrix[killer_pnum][killed_pnum] += 1;

		if (killer_pnum == Player_num) {
			HUD_init_message(HM_MULTI, "%s %s %s!", TXT_YOU, TXT_KILLED, killed_name);
			multi_add_lifetime_kills();
			if ((Game_mode & GM_MULTI_COOP) && (Players[Player_num].score >= 1000))
				add_points_to_score(-1000);

			if (Game_mode & GM_MULTI_ROBOTS)
				add_points_to_score(10000);
		}
		else if (killed_pnum == Player_num)
		{
			HUD_init_message(HM_MULTI, "%s %s %s!", killer_name, TXT_KILLED, TXT_YOU);
			multi_add_lifetime_killed();
		}
		else
			HUD_init_message(HM_MULTI, "%s %s %s!", killer_name, TXT_KILLED, killed_name);
	}

	TheGoal=Netgame.KillGoal*10;

	if (Netgame.KillGoal>0)
	{
		int someone_won = 0; 

		if (Game_mode & GM_TEAM)
		{
			if(Netgame.TeamKillGoalCount[get_team(killer_pnum)] >= TheGoal) {
				HUD_init_message(HM_MULTI, "Kill goal reached by %s!",Netgame.team_name[get_team(killer_pnum)]);
				someone_won = 1; 
			} 
		} else {
			if (Players[killer_pnum].KillGoalCount>=TheGoal)
			{
				if (killer_pnum==Player_num)
				{
					HUD_init_message_literal(HM_MULTI, "You reached the kill goal!");
					//Players[Player_num].shields=i2f(200);
				}
				else
					HUD_init_message(HM_MULTI, "%s has reached the kill goal!",Players[killer_pnum].callsign);

				someone_won = 1; 
			}
		}

		if(someone_won) {
			HUD_init_message_literal(HM_MULTI, "The control center has been destroyed!");
			net_destroy_controlcen (obj_find_first_of_type (OBJ_CNTRLCEN));
		}
	}

	multi_sort_kill_list();
	multi_show_player_list();
}

void multi_do_protocol_frame(int force, int listen)
{
	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			net_udp_do_frame(force, listen);
			break;
#endif
		default:
			RT_LOG(RT_LOGSERVERITY_HIGH, "Protocol handling missing in multi_do_protocol_frame\n");
			break;
	}
}

void multi_do_frame(void)
{
	static int lasttime=0;
	static fix64 last_update_time = 0;
	int i;

	if (!(Game_mode & GM_MULTI) || Newdemo_state == ND_STATE_PLAYBACK)
	{
		Int3();
		return;
	}

	if ((Game_mode & GM_NETWORK) && Netgame.PlayTimeAllowed && lasttime!=f2i (ThisLevelTime))
	{
		for (i=0;i<N_players;i++)
			if (Players[i].connected)
			{
				if (i==Player_num)
				{
					multi_send_heartbeat();
					lasttime=f2i(ThisLevelTime);
				}
				break;
			}
	}

	// Send update about our game mode-specific variables every 2 secs (to keep in sync since delayed kills can invalidate these infos on Clients)
	if (multi_i_am_master() && timer_query() >= last_update_time + (F1_0*2))
	{
		multi_send_gmode_update();
		last_update_time = timer_query();
	}

	multi_send_message(); // Send any waiting messages

	if (Game_mode & GM_MULTI_ROBOTS)
	{
		multi_check_robot_timeout();
	}

	multi_do_protocol_frame(0, 1);

	if (multi_quit_game)
	{
		multi_quit_game = 0;
		if (Game_wind)
			window_close(Game_wind);
	}
}

void
multi_send_data(unsigned char *buf, int len, int priority)
{
	if (len != message_length[(int)buf[0]]) {
		//RT_LOGF(RT_LOGSERVERITY_HIGH, "multi_send_data: Packet type %i length: %i, expected: %i\n", buf[0], len, message_length[(int)buf[0]]);
		RT_LOGF(RT_LOGSERVERITY_MEDIUM, "multi_send_data: Packet type %i length: %i priority %i, expected: %i\n", buf[0], len, priority, message_length[(int)buf[0]]);
		for(int i = 0; i < len; i++) {
			RT_LOGF(RT_LOGSERVERITY_MEDIUM, "    %d: %d\n", i, buf[i]);
		}
		return;
	}
	if (buf[0] >= sizeof(message_length) / sizeof(message_length[0])) {
		RT_LOGF(RT_LOGSERVERITY_MEDIUM, "multi_send_data: Illegal packet type %i\n", buf[0]);
		return;
	}

	if (Game_mode & GM_NETWORK)
	{
		switch (multi_protocol)
		{
#ifdef USE_UDP
			case MULTI_PROTO_UDP:
				net_udp_send_data(buf, len, priority);
				break;
#endif
			default:
				RT_LOG(RT_LOGSERVERITY_HIGH, "Protocol handling missing in multi_send_data\n");
				break;
		}
	}
}

void multi_send_data_direct(const ubyte *buf, int len, int pnum, int priority)
{
	if (len != message_length[(int)buf[0]])
		RT_LOGF(RT_LOGSERVERITY_HIGH, "multi_send_data_direct: Packet type %i length: %i, expected: %i\n", buf[0], len, message_length[(int)buf[0]]);
	if (buf[0] >= sizeof(message_length) / sizeof(message_length[0]))
		RT_LOGF(RT_LOGSERVERITY_HIGH, "multi_send_data_direct: Illegal packet type %i\n", buf[0]);
	if (pnum < 0 || pnum > MAX_PLAYERS)
		RT_LOGF(RT_LOGSERVERITY_HIGH, "multi_send_data_direct: Illegal player num: %i\n", pnum);

	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			net_udp_send_mdata_direct(multibuf, len, pnum, priority);
			break;
#endif
		default:
			RT_LOG(RT_LOGSERVERITY_HIGH, "Protocol handling missing in multi_send_data_direct\n");
			break;
	}
}

void
multi_leave_game(void)
{

	if (!(Game_mode & GM_MULTI))
		return;

	if (Game_mode & GM_NETWORK)
	{
		Net_create_loc = 0;
		multi_send_position(Players[Player_num].objnum);
		multi_powcap_cap_objects();
		if (!Player_eggs_dropped)
		{
			drop_player_eggs(ConsoleObject);
			Player_eggs_dropped = 1;
		}
		multi_send_player_explode(MULTI_PLAYER_DROP);
	}

	multi_send_quit(MULTI_QUIT);

	if (Game_mode & GM_NETWORK)
	{
		switch (multi_protocol)
		{
#ifdef USE_UDP
			case MULTI_PROTO_UDP:
				net_udp_leave_game();
				break;
#endif
			default:
				RT_LOG(RT_LOGSERVERITY_HIGH, "Protocol handling missing in multi_leave_game\n");
				break;
		}
	}

	plyr_save_stats();
}
		
void 
multi_show_player_list()
{
	if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_COOP))
		return;

	if (Show_kill_list)
		return;

	Show_kill_list_timer = F1_0*5; // 5 second timer
	Show_kill_list = 1;
}

int
multi_endlevel(int *secret)
{
	int result = 0;

	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			result = net_udp_endlevel(secret);
			break;
#endif
		default:
			RT_LOG(RT_LOGSERVERITY_HIGH, "Protocol handling missing in multi_endlevel\n");
			break;
	}

	return(result);
}

int multi_endlevel_poll1( newmenu *menu, d_event *event, void *userdata )
{
	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			return net_udp_kmatrix_poll1( menu, event, userdata );
			break;
#endif
		default:
			RT_LOG(RT_LOGSERVERITY_HIGH, "Protocol handling missing in multi_endlevel_poll1\n");
			break;
	}
	
	return 0;	// kill warning
}

int multi_endlevel_poll2( newmenu *menu, d_event *event, void *userdata )
{
	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			return net_udp_kmatrix_poll2( menu, event, userdata );
			break;
#endif
		default:
			RT_LOG(RT_LOGSERVERITY_HIGH, "Protocol handling missing in multi_endlevel_poll2\n");
			break;
	}
	
	return 0;
}

void multi_send_endlevel_packet()
{
	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			net_udp_send_endlevel_packet();
			break;
#endif
		default:
			RT_LOG(RT_LOGSERVERITY_HIGH, "Protocol handling missing in multi_send_endlevel_packet\n");
			break;
	}
}

//
// Part 2 : functions that act on network messages and change the
//          the state of the game in some way.
//

void
multi_define_macro(int key)
{
	if (!(Game_mode & GM_MULTI))
		return;

	key &= (~KEY_SHIFTED);

	switch(key)
	{
		case KEY_F9:
			multi_defining_message = 1; break;
		case KEY_F10:
			multi_defining_message = 2; break;
		case KEY_F11:
			multi_defining_message = 3; break;
		case KEY_F12:
			multi_defining_message = 4; break;
		default:
			Int3();
	}

	if (multi_defining_message)     {
		key_toggle_repeat(1);
		multi_message_index = 0;
		Network_message[multi_message_index] = 0;
	}

}

char feedback_result[200];

void
multi_message_feedback(void)
{
	char *colon;
	int found = 0;
	int i;

	if (!( ((colon = strstr(Network_message, ": ")) == NULL) || (colon-Network_message < 1) || (colon-Network_message > CALLSIGN_LEN) ))
	{
		sprintf(feedback_result, "%s ", TXT_MESSAGE_SENT_TO);
		if ((Game_mode & GM_TEAM) && (atoi(Network_message) > 0) && (atoi(Network_message) < 3))
		{
			sprintf(feedback_result+strlen(feedback_result), "%s '%s'", TXT_TEAM, Netgame.team_name[atoi(Network_message)-1]);
			found = 1;
		}
		if (Game_mode & GM_TEAM)
		{
			for (i = 0; i < N_players; i++)
			{
				if (!d_strnicmp(Netgame.team_name[i], Network_message, colon-Network_message))
				{
					if (found)
						strcat(feedback_result, ", ");
					found++;
					if (!(found % 4))
						strcat(feedback_result, "\n");
					sprintf(feedback_result+strlen(feedback_result), "%s '%s'", TXT_TEAM, Netgame.team_name[i]);
				}
			}
		}
		for (i = 0; i < N_players; i++)
		{
			if ((!d_strnicmp(Players[i].callsign, Network_message, colon-Network_message)) && (i != Player_num) && (Players[i].connected))
			{
				if (found)
					strcat(feedback_result, ", ");
				found++;
				if (!(found % 4))
					strcat(feedback_result, "\n");
				sprintf(feedback_result+strlen(feedback_result), "%s", Players[i].callsign);
			}
		}
		if (!found)
			strcat(feedback_result, TXT_NOBODY);
		else
			strcat(feedback_result, ".");

		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

		Assert(strlen(feedback_result) < 200);

		HUD_init_message_literal(HM_MULTI, feedback_result);
	}
}

//added/moved on 11/10/98 by Victor Rachels to declare before this function
void multi_send_message_end();
//end this section change - VR

void
multi_send_macro(int key)
{

	if(Game_mode & GM_OBSERVER) { return; }

	if (! (Game_mode & GM_MULTI) )
		return;

	switch(key)
	{
		case KEY_F9:
			key = 0; break;
		case KEY_F10:
			key = 1; break;
		case KEY_F11:
			key = 2; break;
		case KEY_F12:
			key = 3; break;
		default:
			Int3();
	}

	if (!PlayerCfg.NetworkMessageMacro[key][0])
	{
		HUD_init_message_literal(HM_MULTI, TXT_NO_MACRO);
		return;
	}

	strcpy(Network_message, PlayerCfg.NetworkMessageMacro[key]);
	Network_message_reciever = 100;

	HUD_init_message(HM_MULTI, "%s '%s'", TXT_SENDING, Network_message);
	multi_message_feedback();

}


void
multi_send_message_start()
{

	if(Game_mode & GM_OBSERVER) { return; }

	if (Game_mode&GM_MULTI) {
		multi_sending_message[Player_num] = 1;
		multi_send_msgsend_state(1);
		multi_message_index = 0;
		Network_message[multi_message_index] = 0;
		key_toggle_repeat(1);
	}
}

extern fix StartingShields;

void multi_send_message_end()
{

	if(Game_mode & GM_OBSERVER) { return; }

	char *mytempbuf;
	int i,t;

  multi_message_index = 0;
  multi_sending_message[Player_num] = 0;
  multi_send_msgsend_state(0);
  key_toggle_repeat(0);

	if (!d_strnicmp (Network_message,"/Handicap: ",11))
	{
		mytempbuf=&Network_message[11];
		StartingShields=atol (mytempbuf);
		if (StartingShields<10)
			StartingShields=10;
		if (StartingShields>100)
		{
			sprintf (Network_message,"%s has tried to cheat!",Players[Player_num].callsign);
			StartingShields=100;
		}
		else
			sprintf (Network_message,"%s handicap is now %d",Players[Player_num].callsign,StartingShields);

		HUD_init_message(HM_MULTI, "Telling others of your handicap of %d!",StartingShields);
		StartingShields=i2f(StartingShields);
	}
	else if (!d_strnicmp (Network_message,"/move: ",7))
	{
		if ((Game_mode & GM_NETWORK) && (Game_mode & GM_TEAM))
		{
			int name_index=7;
			if (strlen(Network_message) > 7)
				while (Network_message[name_index] == ' ')
					name_index++;

			if (!multi_i_am_master())
			{
				HUD_init_message(HM_MULTI, "Only %s can move players!",Players[multi_who_is_master()].callsign);
				return;
			}

			if (strlen(Network_message)<=name_index)
			{
				HUD_init_message_literal(HM_MULTI, "You must specify a name to move");
				return;
			}

			for (i = 0; i < N_players; i++)
				if ((!d_strnicmp(Players[i].callsign, &Network_message[name_index], strlen(Network_message)-name_index)) && (Players[i].connected))
				{
					if (Netgame.team_vector & (1<<i))
						Netgame.team_vector&=(~(1<<i));
					else
						Netgame.team_vector|=(1<<i);

					for (t=0;t<N_players;t++)
						if (Players[t].connected)
							multi_reset_object_texture (&Objects[Players[t].objnum]);
					reset_cockpit();

					multi_send_gmode_update();

					sprintf (Network_message,"%s has changed teams!",Players[i].callsign);
					if (i==Player_num)
					{
						HUD_init_message_literal(HM_MULTI, "You have changed teams!");
						reset_cockpit();
					}
					else
						HUD_init_message(HM_MULTI, "Moving %s to other team.",Players[i].callsign);
					break;
				}
		}
	}
	else if (!d_strnicmp (Network_message,"/kick: ",7) && (Game_mode & GM_NETWORK))
	{
		int name_index=7;
		if (strlen(Network_message) > 7)
			while (Network_message[name_index] == ' ')
				name_index++;

		if (!multi_i_am_master())
		{
			HUD_init_message(HM_MULTI, "Only %s can kick others out!",Players[multi_who_is_master()].callsign);
			multi_message_index = 0;
			multi_sending_message[Player_num] = 0;
			return;
		}
		if (strlen(Network_message)<=name_index)
		{
			HUD_init_message_literal(HM_MULTI, "You must specify a name to kick");
			multi_message_index = 0;
			multi_sending_message[Player_num] = 0;
			return;
		}

		if (Network_message[name_index] == '#' && isdigit(Network_message[name_index+1])) {
			int players[MAX_PLAYERS];
			int listpos = Network_message[name_index+1] - '0';

			if (Show_kill_list==1 || Show_kill_list==2) {
				if (listpos == 0 || listpos >= N_players) {
					HUD_init_message_literal(HM_MULTI, "Invalid player number for kick.");
					multi_message_index = 0;
					multi_sending_message[Player_num] = 0;
					return;
				}
				multi_get_kill_list(players);
				i = players[listpos];
				if ((i != Player_num) && (Players[i].connected))
					goto kick_player;
			}
			else HUD_init_message_literal(HM_MULTI, "You cannot use # kicking with in team display.");


		    multi_message_index = 0;
		    multi_sending_message[Player_num] = 0;
			return;
		}


		for (i = 0; i < N_players; i++)
		if ((!d_strnicmp(Players[i].callsign, &Network_message[name_index], strlen(Network_message)-name_index)) && (i != Player_num) && (Players[i].connected)) {
			kick_player:;
				switch (multi_protocol)
				{
#ifdef USE_UDP
					case MULTI_PROTO_UDP:
						net_udp_dump_player(Netgame.players[i].protocol.udp.addr, 0, DUMP_KICKED);
						break;
#endif
					default:
						RT_LOG(RT_LOGSERVERITY_HIGH, "Protocol handling missing in multi_send_message_end\n");
						break;
				}

				HUD_init_message(HM_MULTI, "Dumping %s...",Players[i].callsign);
				multi_message_index = 0;
				multi_sending_message[Player_num] = 0;
				return;
			}
	}
	
	else if (!d_strnicmp (Network_message,"/killreactor",12) && (Game_mode & GM_NETWORK) && !Control_center_destroyed)
	{
		if (!multi_i_am_master())
			HUD_init_message(HM_MULTI, "Only %s can kill the reactor this way!",Players[multi_who_is_master()].callsign);
		else
		{
			net_destroy_controlcen(NULL);
			multi_send_destroy_controlcen(-1,Player_num);
		}
		multi_message_index = 0;
		multi_sending_message[Player_num] = 0;
		return;
	}

	else if (!d_strnicmp (Network_message,"/noobs",6) && (Game_mode & GM_NETWORK) )
	{
		if (!multi_i_am_master())
			HUD_init_message(HM_MULTI, "Only %s can disconnect observers!",Players[multi_who_is_master()].callsign);
		else
		{			
			for(int i = 0; i < Netgame.numobservers; i++) {
				net_udp_dump_player(Netgame.observers[i].protocol.udp.addr, 0, DUMP_KICKED);
			}
			Netgame.numobservers = 0; 
			HUD_init_message(HM_MULTI, "All observers disconnected.");
			multi_send_obs_update(1, 0); 
		}
		multi_message_index = 0;
		multi_sending_message[Player_num] = 0;
		return; 
	}	

	Network_message_reciever = 100;
	HUD_init_message(HM_MULTI, "%s '%s'", TXT_SENDING, Network_message);
	multi_send_message();
	multi_message_feedback();
	game_flush_inputs();
}

void multi_define_macro_end()
{
	Assert( multi_defining_message > 0 );

	strcpy( PlayerCfg.NetworkMessageMacro[multi_defining_message-1], Network_message );
	write_player_file();

	multi_message_index = 0;
	multi_defining_message = 0;
	key_toggle_repeat(0);
	game_flush_inputs();
}

int multi_message_input_sub(int key)
{
	switch( key )
	{
		case KEY_F8:
		case KEY_ESC:
			multi_sending_message[Player_num] = 0;
			multi_send_msgsend_state(0);
			multi_defining_message = 0;
			key_toggle_repeat(0);
			game_flush_inputs();
			return 1;
		case KEY_LEFT:
		case KEY_BACKSP:
		case KEY_PAD4:
			if (multi_message_index > 0)
				multi_message_index--;
			Network_message[multi_message_index] = 0;
			return 1;
		case KEY_ENTER:
			if ( multi_sending_message[Player_num] )
				multi_send_message_end();
			else if ( multi_defining_message )
				multi_define_macro_end();
			game_flush_inputs();
			return 1;
		default:
		{
			int ascii = key_ascii();
			if ( ascii < 255 ) {
				if (multi_message_index < MAX_MESSAGE_LEN-2 ) {
					Network_message[multi_message_index++] = ascii;
					Network_message[multi_message_index] = 0;
				} else if ( multi_sending_message[Player_num] ) {
					int i;
					char * ptext, * pcolon;
					ptext = NULL;
					Network_message[multi_message_index++] = ascii;
					Network_message[multi_message_index] = 0;
					for (i=multi_message_index-1; i>=0; i-- ) {
						if ( Network_message[i]==32 ) {
							ptext = &Network_message[i+1];
							Network_message[i] = 0;
							break;
						}
					}
					multi_send_message_end();
					if ( ptext ) {
						multi_sending_message[Player_num] = 1;
						multi_send_msgsend_state(1);
						pcolon = strstr( Network_message, ": " );
						if ( pcolon )
							strcpy( pcolon+1, ptext );
						else
							strcpy( Network_message, ptext );
						multi_message_index = strlen( Network_message );
					}
				}
			}
		}
	}
	
	return 0;
}

void
multi_send_message_dialog(void)
{
	newmenu_item m[1];
	int choice;

	if (!(Game_mode&GM_MULTI))
		return;

	Network_message[0] = 0;             // Get rid of old contents

	m[0].type=NM_TYPE_INPUT; m[0].text = Network_message; m[0].text_len = MAX_MESSAGE_LEN-1;
	choice = newmenu_do( NULL, TXT_SEND_MESSAGE, 1, m, NULL, NULL );

	if ((choice > -1) && (strlen(Network_message) > 0)) {
		Network_message_reciever = 100;
		multi_message_feedback();
	}
}



void
multi_do_death(int objnum)
{
	// Do any miscellaneous stuff for a new network player after death

	objnum = objnum;

	if (!(Game_mode & GM_MULTI_COOP))
	{
		Players[Player_num].flags |= (PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_GOLD_KEY);
	}
}



void
multi_do_fire(const ubyte *buf)
{
	ubyte weapon;
	int pnum;
	sbyte flags;
	fix save_charge = Fusion_charge;
    
	// Act out the actual shooting
	pnum = buf[1];
	weapon = (int)buf[2];
	flags = buf[4];
	Network_laser_track = GET_INTEL_SHORT(buf + 6);

	/* CED sniperpackets */
	vms_vector shot_orientation;
	shot_orientation.x = (fix) GET_INTEL_INT(buf + 8); 
	shot_orientation.y = (fix) GET_INTEL_INT(buf + 12); 
	shot_orientation.z = (fix) GET_INTEL_INT(buf + 16); 


	Assert (pnum < N_players);

	if (Objects[Players[pnum].objnum].type == OBJ_GHOST)
		multi_make_ghost_player(pnum);

	if (weapon >= MISSILE_ADJUST) 
		/* CED sniperpackets */
		net_missile_firing(pnum, weapon, (int)buf[4], shot_orientation); 
	else {
		if (weapon == FUSION_INDEX) {
			Fusion_charge = buf[4] << 12;
		}
		if (weapon == LASER_INDEX) {
			if (flags & LASER_QUAD)
				Players[pnum].flags |= PLAYER_FLAGS_QUAD_LASERS;
			else
				Players[pnum].flags &= ~PLAYER_FLAGS_QUAD_LASERS;
		}

		/* CED sniperpackets */
		do_laser_firing(Players[pnum].objnum, weapon, (int)buf[3], flags, (int)buf[5], shot_orientation);

		if (weapon == FUSION_INDEX)
			Fusion_charge = save_charge;
	}
}

void
multi_do_message(const ubyte *cbuf)
{
	const char *buf = (const char*)cbuf;
	char *colon,mesbuf[100];
	int t;

	int loc = 2;

	if(Netgame.FairColors)
		selected_player_rgb = player_rgb_all_blue; 
	else if(Netgame.BlackAndWhitePyros) 
		selected_player_rgb = player_rgb_alt; 
	else
		selected_player_rgb = player_rgb;

	if (((colon = strstr(buf+loc, ": ")) == NULL) || (colon-(buf+loc) < 1) || (colon-(buf+loc) > CALLSIGN_LEN))
	{
		int color = 0;
		mesbuf[0] = CC_COLOR;
		if (Game_mode & GM_TEAM)
			color = get_team((int)buf[1]);
		else
			color = Netgame.players[(int)buf[1]].color; //(int)buf[1];
		mesbuf[1] = BM_XRGB(selected_player_rgb[color].r,selected_player_rgb[color].g,selected_player_rgb[color].b);
		strcpy(&mesbuf[2], Players[(int)buf[1]].callsign);
		t = strlen(mesbuf);
		mesbuf[t] = ':';
		mesbuf[t+1] = CC_COLOR;
		mesbuf[t+2] = BM_XRGB(0, 31, 0);
		mesbuf[t+3] = 0;

		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
		HUD_init_message(HM_MULTI, "%s %s", mesbuf, buf+2);
		multi_sending_message[(int)buf[1]] = 0;
	}
	else if ( (!d_strnicmp(Players[Player_num].callsign, buf+loc, colon-(buf+loc))) ||
			  ((Game_mode & GM_TEAM) && ( (get_team(Player_num) == atoi(buf+loc)-1) || !d_strnicmp(Netgame.team_name[get_team(Player_num)], buf+loc, colon-(buf+loc)))) )
	{
		int color = 0;
		mesbuf[0] = CC_COLOR;
		if (Game_mode & GM_TEAM)
			color = get_team((int)buf[1]);
		else
			color = (int)buf[1];
		mesbuf[1] = BM_XRGB(selected_player_rgb[color].r,selected_player_rgb[color].g,selected_player_rgb[color].b);
		strcpy(&mesbuf[2], Players[(int)buf[1]].callsign);
		t = strlen(mesbuf);
		mesbuf[t] = ':';
		mesbuf[t+1] = CC_COLOR;
		mesbuf[t+2] = BM_XRGB(0, 31, 0);
		mesbuf[t+3] = 0;

		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
		HUD_init_message(HM_MULTI, "%s %s", mesbuf, colon+2);
		multi_sending_message[(int)buf[1]] = 0;
	}
}

void
multi_do_position(const ubyte *buf)
{
	ubyte pnum = 0;
#ifdef WORDS_BIGENDIAN
	shortpos sp;
#endif

	pnum = buf[1];

#ifndef WORDS_BIGENDIAN
	extract_shortpos(&Objects[Players[pnum].objnum], (shortpos *)(buf + 2),0);
#else
	memcpy((ubyte *)(sp.bytemat), (ubyte *)(buf + 2), 9);
	memcpy((ubyte *)&(sp.xo), (ubyte *)(buf + 11), 14);
	extract_shortpos(&Objects[Players[pnum].objnum], &sp, 1);
#endif

	if (Objects[Players[pnum].objnum].movement_type == MT_PHYSICS)
		set_thrust_from_velocity(&Objects[Players[pnum].objnum]);
}

void
multi_do_reappear(const ubyte *buf)
{
	short objnum;
	ubyte pnum = buf[1];

	objnum = GET_INTEL_SHORT(buf + 2);

	Assert(objnum >= 0);
	if (pnum != Objects[objnum].id)
		return;

	// CED -- Not helping -- makes other bug worse -- coopfix
	//if (PKilledFlags[pnum]<=0) // player was not reported dead, so do not accept this packet
	//{
	//	PKilledFlags[pnum]--;
	//	return;
	//}

	multi_make_ghost_player(Objects[objnum].id);
	create_player_appearance_effect(&Objects[objnum]);
	PKilledFlags[pnum]=0;
}

void
multi_do_player_explode(const ubyte *buf)
{
	// Only call this for players, not robots.  pnum is player number, not
	// Object number.

	object *objp;
	int count;
	int pnum;
	int i;
	char remote_created;

	pnum = buf[1];

#ifdef NDEBUG
	if ((pnum < 0) || (pnum >= N_players))
		return;
#else
	Assert(pnum >= 0);
	Assert(pnum < N_players);
#endif

#ifdef NETWORK
	// If we are in the process of sending objects to a new player, reset that process
	if (Network_send_objects)
	{
		Network_send_objnum = -1;
	}
#endif

	// Stuff the Players structure to prepare for the explosion

	count = 2;
	Players[pnum].primary_weapon_flags = buf[count];				count++;
	Players[pnum].secondary_weapon_flags = buf[count];				count++;
	Players[pnum].laser_level = buf[count];                                                 count++;
	Players[pnum].secondary_ammo[HOMING_INDEX] = buf[count];                count++;
	Players[pnum].secondary_ammo[CONCUSSION_INDEX] = buf[count];count++;
	Players[pnum].secondary_ammo[SMART_INDEX] = buf[count];         count++;
	Players[pnum].secondary_ammo[MEGA_INDEX] = buf[count];          count++;
	Players[pnum].secondary_ammo[PROXIMITY_INDEX] = buf[count]; count++;
	Players[pnum].primary_ammo[VULCAN_INDEX] = GET_INTEL_SHORT(buf + count); count += 2;
	Players[pnum].flags = GET_INTEL_INT(buf + count);               count += 4;

	multi_powcap_adjust_remote_cap (pnum);

	objp = Objects+Players[pnum].objnum;

	//      objp->phys_info.velocity = *(vms_vector *)(buf+16); // 12 bytes
	//      objp->pos = *(vms_vector *)(buf+28);                // 12 bytes

	remote_created = buf[count++]; // How many did the other guy create?

	Net_create_loc = 0;

	drop_player_eggs_remote(objp, 1);

	// Create mapping from remote to local numbering system

	// We now handle this situation gracefully, Int3 not required
	//      if (Net_create_loc != remote_created)
	//              Int3(); // Probably out of object array space, see Rob

	for (i = 0; i < remote_created; i++)
	{
		short s;

		s = GET_INTEL_SHORT(buf + count);
		if ((i < Net_create_loc) && (s > 0) &&
		    (Net_create_objnums[i] > 0))
			map_objnum_local_to_remote((short)Net_create_objnums[i], s, pnum);
		count += 2;
	}
	for (i = remote_created; i < Net_create_loc; i++) {
		Objects[Net_create_objnums[i]].flags |= OF_SHOULD_BE_DEAD;
	}

	if (buf[0] == MULTI_PLAYER_EXPLODE)
	{
		explode_badass_player(objp);

		objp->flags &= ~OF_SHOULD_BE_DEAD;              //don't really kill player
		multi_make_player_ghost(pnum);
	}
	else
	{
		create_player_appearance_effect(objp);
	}

	Players[pnum].flags &= ~(PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE);
	Players[pnum].cloak_time = 0;

	PKilledFlags[pnum]++;
	if (PKilledFlags[pnum] < 1) // seems we got reappear already so make him player again!
	{
		multi_make_ghost_player(Objects[Players[pnum].objnum].id);
		create_player_appearance_effect(&Objects[Players[pnum].objnum]);
		PKilledFlags[pnum] = 0;
	}
}

/*
 * Process can compute a kill. If I am a Client this might be my own one (see multi_send_kill()) but with more specific data so I can compute my kill correctly.
 */
void
multi_do_kill(const ubyte *buf)
{
	int killer, killed;
	int count = 1;
	int pnum = (int)(buf[count]);
	int type = (int)(buf[0]);

	if (multi_i_am_master() && type != MULTI_KILL_CLIENT)
		return;
	if (!multi_i_am_master() && type != MULTI_KILL_HOST)
		return;

	if ((pnum < 0) || (pnum >= N_players))
	{
		Int3(); // Invalid player number killed
		return;
	}

	// I am host, I know what's going on so take this packet, add game_mode related info which might be necessary for kill computation and send it to everyone so they can compute their kills correctly
	if (multi_i_am_master())
	{
		memcpy(multibuf, buf, 5);
		multibuf[0] = MULTI_KILL_HOST;
		multibuf[5] = Netgame.team_vector;
		multibuf[6] = Bounty_target;

		multi_send_data(multibuf, 7, 2);
	}

	killed = Players[pnum].objnum;
	count += 1;
	killer = GET_INTEL_SHORT(buf + count);
	if (killer > 0)
		killer = objnum_remote_to_local(killer, (sbyte)buf[count+2]);
	if (!multi_i_am_master())
	{
		Netgame.team_vector = buf[5];
		Bounty_target = buf[6];
	}

	multi_compute_kill(killer, killed);

	if (Game_mode & GM_BOUNTY && multi_i_am_master()) // update in case if needed... we could attach this to this packet but... meh...
		multi_send_bounty();
}


//      Changed by MK on 10/20/94 to send NULL as object to net_destroy_controlcen if it got -1
// which means not a controlcen object, but contained in another object
void multi_do_controlcen_destroy(const ubyte *buf)
{
	sbyte who;
	short objnum;

	objnum = GET_INTEL_SHORT(buf + 1);
	who = buf[3];

	if (Control_center_destroyed != 1)
	{
		if ((who < N_players) && (who != Player_num)) {
			HUD_init_message(HM_MULTI, "%s %s", Players[who].callsign, TXT_HAS_DEST_CONTROL);
		}
		else if (who == Player_num)
			HUD_init_message_literal(HM_MULTI, TXT_YOU_DEST_CONTROL);
		else
			HUD_init_message_literal(HM_MULTI, TXT_CONTROL_DESTROYED);

		if (objnum != -1)
			net_destroy_controlcen(Objects+objnum);
		else
			net_destroy_controlcen(NULL);
	}
}

void
multi_do_escape(const ubyte *buf)
{
	int objnum;

	objnum = Players[(int)buf[1]].objnum;

	if (buf[2] == 0)
	{
                digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
		HUD_init_message(HM_MULTI, "%s %s", Players[(int)buf[1]].callsign, TXT_HAS_ESCAPED);

		if (Game_mode & GM_NETWORK) {
			Players[(int)buf[1]].connected = CONNECT_ESCAPE_TUNNEL;

			if (Current_obs_player == (int)buf[1]) {
				reset_obs();
			}
		}

		if (!multi_goto_secret)
			multi_goto_secret = 2;
	}
	else if (buf[2] == 1)
	{
                digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
		HUD_init_message(HM_MULTI, "%s %s", Players[(int)buf[1]].callsign, TXT_HAS_FOUND_SECRET);

		if (Game_mode & GM_NETWORK) {
			Players[(int)buf[1]].connected = CONNECT_FOUND_SECRET;

			if (Current_obs_player == (int)buf[1]) {
				reset_obs();
			}
		}

		if (!multi_goto_secret)
			multi_goto_secret = 1;
	}

	create_player_appearance_effect(&Objects[objnum]);
	multi_make_player_ghost(buf[1]);
}

#define MAX_PACKETS 200 // Memory's cheap ;)
int is_recent_duplicate(const ubyte *buf) {
	const fix64 timeout = F1_0*10; 
	static ubyte received_packets[MAX_PACKETS*5]; // old pickup packets
	static fix64 rxtime[MAX_PACKETS];
	static ubyte num_waiting = 0;

	ubyte num_now_waiting = 0; 

	fix64 now = timer_query(); 


	// Clear out old ones
	for(int i = 0; i < num_waiting; i++) {
		if(now - rxtime[i] <= timeout) {
			if(num_now_waiting != i) {
				memcpy(received_packets + num_now_waiting*5, received_packets + i*5, 5); 
				rxtime[num_now_waiting] = rxtime[i];
			}

			num_now_waiting++; 
		} 
	}

	num_waiting = num_now_waiting; 

	// Search for dups
	for(int i = 0; i < num_waiting; i++) {
		if(! memcmp(received_packets + i*5, buf, 5)) {			
			return 1; 
		} 
	}

	// Not a dup, hold on to this one
	if(num_waiting < MAX_PACKETS) {
		memcpy(received_packets + num_waiting*5, buf, 5);
		rxtime[num_waiting] = now; 
		num_waiting++; 
	} 

	return 0; 
}

void
multi_do_remobj(const ubyte *buf)
{
	short objnum; // which object to remove
	short local_objnum;
	sbyte obj_owner; // which remote list is it entered in

	objnum = GET_INTEL_SHORT(buf + 1);
	obj_owner = buf[3];
	//ubyte counter = buf[4]; 

	Assert(objnum >= 0);

	if(is_recent_duplicate(buf)) {
		return; 
	}

	if (objnum < 1)
		return;

	local_objnum = objnum_remote_to_local(objnum, obj_owner); // translate to local objnum

	if (local_objnum < 0)
	{
		return;
	}

	if ((Objects[local_objnum].type != OBJ_POWERUP) && (Objects[local_objnum].type != OBJ_HOSTAGE))
	{
		return;
	}

	if (Network_send_objects && multi_objnum_is_past(local_objnum))
	{
		Network_send_objnum = -1;
	}

	if (Objects[local_objnum].type==OBJ_POWERUP)
		if (Game_mode & GM_NETWORK)
		{
			if (multi_powerup_is_4pack (Objects[local_objnum].id))
			{
				if (PowerupsInMine[Objects[local_objnum].id-1]-4<0)
					PowerupsInMine[Objects[local_objnum].id-1]=0;
				else
					PowerupsInMine[Objects[local_objnum].id-1]-=4;
			}
			else
			{
				if (PowerupsInMine[Objects[local_objnum].id]>0)
					PowerupsInMine[Objects[local_objnum].id]--;
			}
		}

	Objects[local_objnum].flags |= OF_SHOULD_BE_DEAD; // quick and painless

}

void multi_disconnect_player(int pnum)
{
	int i, n = 0;

	if (!(Game_mode & GM_NETWORK))
		return;
	if (Players[pnum].connected == CONNECT_DISCONNECTED)
		return;

	if (Players[pnum].connected == CONNECT_PLAYING)
	{
		digi_play_sample( SOUND_HUD_MESSAGE, F1_0 );
		HUD_init_message(HM_MULTI,  "%s %s", Players[pnum].callsign, TXT_HAS_LEFT_THE_GAME);

		multi_sending_message[pnum] = 0;

		if (Network_status == NETSTAT_PLAYING)
		{
			multi_make_player_ghost(pnum);
			multi_strip_robots(pnum);
		}

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_disconnect(pnum);

		// Bounty target left - select a new one
		if( Game_mode & GM_BOUNTY && pnum == Bounty_target && multi_i_am_master() )
		{
			/* Select a random number */
			int new = d_rand() % MAX_PLAYERS;
			
			/* Make sure they're valid: Don't check against kill flags,
				* just in case everyone's dead! */
			while( !Players[new].connected )
				new = d_rand() % MAX_PLAYERS;
			
			/* Select new target */
			multi_new_bounty_target( new );
			
			/* Send this new data */
			multi_send_bounty();
		}
	}

	Players[pnum].connected = CONNECT_DISCONNECTED;
	if (Current_obs_player == pnum) {
		reset_obs();
	}
	Netgame.players[pnum].connected = CONNECT_DISCONNECTED;
	PKilledFlags[pnum] = 1;

	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			net_udp_disconnect_player(pnum);
			break;
#endif
		default:
			RT_LOG(RT_LOGSERVERITY_HIGH, "Protocol handling missing in multi_disconnect_player\n");
			break;
	}

	if (pnum == multi_who_is_master()) // Host has left - Quit game!
	{
		if (Network_status==NETSTAT_PLAYING)
			multi_leave_game();
		if (Game_wind)
			window_set_visible(Game_wind, 0);
		nm_messagebox(NULL, 1, TXT_OK, "Host left the game!");
		if (Game_wind)
			window_set_visible(Game_wind, 1);
		multi_quit_game = 1;
		game_leave_menus();
		multi_reset_stuff();
		return;
	}

	for (i = 0; i < N_players; i++)
		if (Players[i].connected) n++;
	if (n == 1)
	{
		HUD_init_message_literal(HM_MULTI, "You are the only person remaining in this netgame");
	}
}

void
multi_do_quit(const ubyte *buf)
{

	if (!(Game_mode & GM_NETWORK))
		return;
	multi_disconnect_player((int)buf[1]);
}

void
multi_do_cloak(const ubyte *buf)
{
	int pnum;

	pnum = buf[1];

	Assert(pnum < N_players);

	Players[pnum].flags |= PLAYER_FLAGS_CLOAKED;
	Players[pnum].cloak_time = GameTime64;
	ai_do_cloak_stuff();

	if (Game_mode & GM_MULTI_ROBOTS)
		multi_strip_robots(pnum);

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_cloak(pnum);
}

void
multi_do_decloak(const ubyte *buf)
{
	int pnum;

	pnum = buf[1];

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_decloak(pnum);

}

void
multi_do_door_open(const ubyte *buf)
{
	int segnum;
	short side;
	segment *seg;
	wall *w;

	segnum = GET_INTEL_SHORT(buf + 1);
	side = buf[3];
	
	if ((segnum < 0) || (segnum > Highest_segment_index) || (side < 0) || (side > 5))
	{
		Int3();
		return;
	}

	seg = &Segments[segnum];

	if (seg->sides[side].wall_num == -1) {  //Opening door on illegal wall
		Int3();
		return;
	}

	w = &Walls[seg->sides[side].wall_num];

	if (w->type == WALL_BLASTABLE)
	{
		if (!(w->flags & WALL_BLASTED))
		{
			wall_destroy(seg, side);
		}
		return;
	}
	else if (w->state != WALL_DOOR_OPENING)
	{
		wall_open_door(seg, side);
	}
}

void
multi_do_create_explosion(const ubyte *buf)
{
	int pnum;
	int count = 1;

	pnum = buf[count++];

	create_small_fireball_on_object(&Objects[Players[pnum].objnum], F1_0, 1);
}

void
multi_do_controlcen_fire(const ubyte *buf)
{
	vms_vector to_target;
	int gun_num;
	short objnum;
	int count = 1;

	memcpy(&to_target, buf+count, 12);          count += 12;
#ifdef WORDS_BIGENDIAN  // swap the vector to_target
	to_target.x = (fix)INTEL_INT((int)to_target.x);
	to_target.y = (fix)INTEL_INT((int)to_target.y);
	to_target.z = (fix)INTEL_INT((int)to_target.z);
#endif
	gun_num = buf[count];                       count += 1;
	objnum = GET_INTEL_SHORT(buf + count);      count += 2;

 	Laser_create_new_easy(&to_target, &Objects[objnum].ctype.reactor_info.gun_pos[gun_num], objnum, CONTROLCEN_WEAPON_NUM, 1);
}

void
multi_do_create_powerup(const ubyte *buf)
{
	short segnum;
	short objnum;
	int my_objnum;
	int pnum;
	int count = 1;
	vms_vector new_pos;
	char powerup_type;

	if (Endlevel_sequence || Control_center_destroyed)
		return;

	pnum = buf[count++];
	powerup_type = buf[count++];
	segnum = GET_INTEL_SHORT(buf + count); count += 2;
	objnum = GET_INTEL_SHORT(buf + count); count += 2;

	if ((segnum < 0) || (segnum > Highest_segment_index)) {
		Int3();
		return;
	}
	
	new_pos = *(vms_vector *)(buf+count); count+=sizeof(vms_vector);

#ifdef WORDS_BIGENDIAN
	new_pos.x = (fix)SWAPINT((int)new_pos.x);
	new_pos.y = (fix)SWAPINT((int)new_pos.y);
	new_pos.z = (fix)SWAPINT((int)new_pos.z);
#endif

	Net_create_loc = 0;
	my_objnum = call_object_create_egg(&Objects[Players[pnum].objnum], 1, OBJ_POWERUP, powerup_type);

	if (my_objnum < 0) {
		return;
	}

	if (Network_send_objects && multi_objnum_is_past(my_objnum))
	{
		Network_send_objnum = -1;
	}

	Objects[my_objnum].pos = new_pos;

	vm_vec_zero(&Objects[my_objnum].mtype.phys_info.velocity);

	obj_relink(my_objnum, segnum);

	map_objnum_local_to_remote(my_objnum, objnum, pnum);

	object_create_explosion(segnum, &new_pos, i2f(5), VCLIP_POWERUP_DISAPPEARANCE);

	if (Game_mode & GM_NETWORK)
	{
		if (multi_powerup_is_4pack((int)powerup_type))
			PowerupsInMine[(int)(powerup_type-1)]+=4;
		else
			PowerupsInMine[(int)powerup_type]++;
	}
}

void
multi_do_play_sound(const ubyte *buf)
{
	int pnum = buf[1];
	int sound_num = buf[2];
	fix volume = buf[3] << 12;

	if (!Players[pnum].connected)
		return;

	Assert(Players[pnum].objnum >= 0);
	Assert(Players[pnum].objnum <= Highest_object_index);

	digi_link_sound_to_object( sound_num, Players[pnum].objnum, 0, volume);
}

void
multi_do_score(const ubyte *buf)
{
	int pnum = buf[1];

	if ((pnum < 0) || (pnum >= N_players))
	{
		Int3(); // Non-terminal, see rob
		return;
	}

	if (Newdemo_state == ND_STATE_RECORDING) {
		int score;
		score = GET_INTEL_INT(buf + 2);
		newdemo_record_multi_score(pnum, score);
	}

	Players[pnum].score = GET_INTEL_INT(buf + 2);

	multi_sort_kill_list();
}

void
multi_do_trigger(const ubyte *buf)
{
	int pnum = buf[1];
	int trigger = buf[2];

	if ((pnum < 0) || (pnum >= N_players) || (pnum == Player_num))
	{
		Int3(); // Got trigger from illegal playernum
		return;
	}
	if ((trigger < 0) || (trigger >= Num_triggers))
	{
		Int3(); // Illegal trigger number in multiplayer
		return;
	}
	check_trigger_sub(trigger, pnum, 0);
}

void multi_do_hostage_door_status(const ubyte *buf)
{
	// Update hit point status of a door

	int count = 1;
	int wallnum;
	fix hps;

	wallnum = GET_INTEL_SHORT(buf + count);     count += 2;
	hps = GET_INTEL_INT(buf + count);           count += 4;

	if ((wallnum < 0) || (wallnum > Num_walls) || (hps < 0) || (Walls[wallnum].type != WALL_BLASTABLE))
	{
		Int3(); // Non-terminal, see Rob
		return;
	}

	if (hps < Walls[wallnum].hps)
		wall_damage(&Segments[Walls[wallnum].segnum], Walls[wallnum].sidenum, Walls[wallnum].hps - hps);
}

void
multi_reset_stuff(void)
{
	// A generic, emergency function to solve problems that crop up
	// when a player exits quick-out from the game because of a
	// connection loss.  Fixes several weird bugs!

	dead_player_end();
	Players[Player_num].homing_object_dist = -F1_0; // Turn off homing sound.
	reset_rear_view();
}

void
multi_reset_player_object(object *objp)
{
	int i;

	//Init physics for a non-console player

	Assert(objp >= Objects);
	Assert(objp <= Objects+Highest_object_index);
	Assert((objp->type == OBJ_PLAYER) || (objp->type == OBJ_GHOST));

	vm_vec_zero(&objp->mtype.phys_info.velocity);
	vm_vec_zero(&objp->mtype.phys_info.thrust);
	vm_vec_zero(&objp->mtype.phys_info.rotvel);
	vm_vec_zero(&objp->mtype.phys_info.rotthrust);
	objp->mtype.phys_info.brakes = objp->mtype.phys_info.turnroll = 0;
	objp->mtype.phys_info.mass = Player_ship->mass;
	objp->mtype.phys_info.drag = Player_ship->drag;
	//if (objp->type == OBJ_PLAYER) {
	//	objp->mtype.phys_info.flags |= PF_TURNROLL | PF_WIGGLE;
	//	objp->mtype.phys_info.flags &= ~PF_LEVELLING;
	//} else {
		objp->mtype.phys_info.flags &= ~(PF_TURNROLL | PF_LEVELLING | PF_WIGGLE);
	//}

	//Init render info

	objp->render_type = RT_POLYOBJ;
	objp->rtype.pobj_info.model_num = Player_ship->model_num;               //what model is this?
	objp->rtype.pobj_info.subobj_flags = 0;         //zero the flags
	for (i=0;i<MAX_SUBMODELS;i++)
		vm_angvec_zero(&objp->rtype.pobj_info.anim_angles[i]);

	//reset textures for this, if not player 0

	multi_reset_object_texture (objp);

	// Clear misc

	objp->flags = 0;

	if (objp->type == OBJ_GHOST)
		objp->render_type = RT_NONE;

}

void disable_faircolors_if_3_connected() {
	if(Game_mode & GM_MULTI && Netgame.FairColors) {
		int num_connected = 0; 
		for(int i = 0; i < MAX_PLAYERS; i++) {
			if(Players[i].connected != CONNECT_DISCONNECTED) {
				num_connected++;
			}
		}

		if(num_connected > 2) {
			Netgame.FairColors = 0; 
		}
	}
}

int get_color_for_first_team_player(int team, int missile) {
	for(int i = 0; i < MAX_PLAYERS; i++) {
		if(get_team(i) == team) {
			if(missile) { return Netgame.players[i].missilecolor; }
			return Netgame.players[i].color;
		}
	}
	RT_LOGF(RT_LOGSERVERITY_MEDIUM, "Couldn't find team color for team %d\n", team);
	return team; 
} 

int get_color_for_player(int player, int missile) {
	int color = 0; 

	if((! PlayerCfg.ShowCustomColors) || (! Netgame.AllowPreferredColors)) {
		if(Game_mode & GM_TEAM) {
			color = get_team(player);
		} else {
			color = player; 
		}
	} else {
		if(Game_mode & GM_TEAM) {
			color = get_color_for_first_team_player(get_team(player), missile);
		} else {
			if(missile) { color = Netgame.players[player].missilecolor; }
			else        { color = Netgame.players[player].color;  }
		}
	}

	if (Game_mode & GM_TEAM) {
		return(color); 
	}

	if(Game_mode & GM_MULTI && Netgame.FairColors && ! (Game_mode & GM_OBSERVER)) {
		return 0;
	}

	return(color); 
}

int get_color_for_team(int team, int missile) {
	for(int i = 0; i < MAX_PLAYERS; i++) {
		if(get_team(i) == team) {
			return get_color_for_player(i, missile);
		}
	}

	return team; 
}

void multi_reset_object_texture (object *objp)
{
	disable_faircolors_if_3_connected();

	int wid = get_color_for_player(objp->id, 0);
	int mid = get_color_for_player(objp->id, 1);

	// RT_LOGF(RT_LOGSERVERITY_MEDIUM, "Custom color for player %d is %d,%d\n", objp->id, wid, mid);

	int id; 
	if (Game_mode & GM_TEAM)
		id = get_team(objp->id);
	else
		id = objp->id;

	if(id == 0) {
		if(wid == 0 && mid == 0) {
			objp->rtype.pobj_info.alt_textures=0;
		} else {
			objp->rtype.pobj_info.alt_textures=8;

			// Initialize the other textures
			for(int i = 0; i<Polygon_models[objp->rtype.pobj_info.model_num].n_textures; i++) {
				multi_player_textures[7][i] = ObjBitmaps[ObjBitmapPtrs[Polygon_models[objp->rtype.pobj_info.model_num].first_texture+i]];
			}
			multi_player_textures[7][4] = ObjBitmaps[ObjBitmapPtrs[First_multi_bitmap_num+(mid-1)*2]];
			multi_player_textures[7][5] = ObjBitmaps[ObjBitmapPtrs[First_multi_bitmap_num+(wid-1)*2+1]];
		}
	} else {
		if (N_PLAYER_SHIP_TEXTURES < Polygon_models[objp->rtype.pobj_info.model_num].n_textures)
			RT_LOG(RT_LOGSERVERITY_HIGH, "Too many player ship textures!\n");

		for (int i=0;i<Polygon_models[objp->rtype.pobj_info.model_num].n_textures;i++)
			multi_player_textures[id-1][i] = ObjBitmaps[ObjBitmapPtrs[Polygon_models[objp->rtype.pobj_info.model_num].first_texture+i]];

		multi_player_textures[id-1][4] = ObjBitmaps[ObjBitmapPtrs[First_multi_bitmap_num+(mid-1)*2]];
		multi_player_textures[id-1][5] = ObjBitmaps[ObjBitmapPtrs[First_multi_bitmap_num+(wid-1)*2+1]];

		objp->rtype.pobj_info.alt_textures = id;
	}
}

void
multi_process_bigdata(const ubyte *buf, unsigned len)
{
	// Takes a bunch of messages, check them for validity,
	// and pass them to multi_process_data. 

	unsigned type, sub_len, bytes_processed = 0;

	while( bytes_processed < len )	{
		type = buf[bytes_processed];

		if ( (type>= sizeof(message_length)/sizeof(message_length[0])))	{
			RT_LOGF(RT_LOGSERVERITY_INFO, "multi_process_bigdata: Invalid packet type %d!\n", type);
			return;
		}

		sub_len = message_length[type];

		Assert(sub_len > 0);

		if ( (bytes_processed+sub_len) > len )  {
			RT_LOGF(RT_LOGSERVERITY_INFO, "multi_process_bigdata: packet type %d too short (%d>%d)!\n", type, (bytes_processed + sub_len), len);
			Int3();
			return;
		}

		multi_process_data(&buf[bytes_processed], sub_len);
		bytes_processed += sub_len;
	}
}

//
// Part 2 : Functions that send communication messages to inform the other
//          players of something we did.
//

void multi_send_fire(int laser_gun, int laser_level, int laser_flags, int laser_fired, short laser_track)
{
	if(Game_mode & GM_OBSERVER) { return; }

	multi_do_protocol_frame(1, 0); // provoke positional update if possible

	multibuf[0] = (char)MULTI_FIRE;
	multibuf[1] = (char)Player_num;
	multibuf[2] = (char)laser_gun;
	multibuf[3] = (char)laser_level;
	multibuf[4] = (char)laser_flags;
	multibuf[5] = (char)laser_fired;
	PUT_INTEL_SHORT(multibuf+6, laser_track);

	/* CED sniperpackets */
	object* ownship = Objects + Players[Player_num].objnum;
	PUT_INTEL_INT(multibuf+8 , ownship->orient.fvec.x);
	PUT_INTEL_INT(multibuf+12, ownship->orient.fvec.y);
	PUT_INTEL_INT(multibuf+16, ownship->orient.fvec.z);

	//multi_send_data(multibuf, 8, 1);
	multi_send_data(multibuf, 20, 1);
}


void
multi_send_destroy_controlcen(int objnum, int player)
{
	if(Game_mode & GM_OBSERVER) { return; }

	if (player == Player_num)
		HUD_init_message_literal(HM_MULTI, TXT_YOU_DEST_CONTROL);
	else if ((player > 0) && (player < N_players))
		HUD_init_message(HM_MULTI, "%s %s", Players[player].callsign, TXT_HAS_DEST_CONTROL);
	else
		HUD_init_message_literal(HM_MULTI, TXT_CONTROL_DESTROYED);

	multibuf[0] = (char)MULTI_CONTROLCEN;
	PUT_INTEL_SHORT(multibuf+1, objnum);
	multibuf[3] = player;
	multi_send_data(multibuf, 4, 2);
}

void 
multi_send_endlevel_start(int secret)
{
	if(Game_mode & GM_OBSERVER) { return; }

	multibuf[0] = (char)MULTI_ENDLEVEL_START;
	multibuf[1] = Player_num;
	multibuf[2] = (char)secret;

	if ((secret) && !multi_goto_secret)
		multi_goto_secret = 1;
	else if (!multi_goto_secret)
		multi_goto_secret = 2;

	multi_send_data(multibuf, 3, 2);
	if (Game_mode & GM_NETWORK)
	{
		Players[Player_num].connected = CONNECT_ESCAPE_TUNNEL;

		switch (multi_protocol)
		{
#ifdef USE_UDP
			case MULTI_PROTO_UDP:
				net_udp_send_endlevel_packet();
				break;
#endif
			default:
				RT_LOG(RT_LOGSERVERITY_HIGH, "Protocol handling missing in multi_send_endlevel_start\n");
				break;
		}
	}
}

void
multi_send_player_explode(char type)
{
	if(Game_mode & GM_OBSERVER) { return; }

	int count = 0;
	int i;

	Assert( (type == MULTI_PLAYER_DROP) || (type == MULTI_PLAYER_EXPLODE) );

	if (Network_send_objects)
	{
		Network_send_objnum = -1;
	}

	multi_send_position(Players[Player_num].objnum);

	multibuf[count++] = type;
	multibuf[count++] = Player_num;
	multibuf[count++] = (char)Players[Player_num].primary_weapon_flags;
	multibuf[count++] = (char)Players[Player_num].secondary_weapon_flags;
	multibuf[count++] = (char)Players[Player_num].laser_level;
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[HOMING_INDEX];
	if(Netgame.RespawnConcs) {
		multibuf[count++] = (char)RespawningConcussions[Player_num];
	} else {
		multibuf[count++] = (char)Players[Player_num].secondary_ammo[CONCUSSION_INDEX];
	}
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[SMART_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[MEGA_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[PROXIMITY_INDEX];
	PUT_INTEL_SHORT(multibuf+count, Players[Player_num].primary_ammo[VULCAN_INDEX] );
	count += 2;
	PUT_INTEL_INT(multibuf+count, Players[Player_num].flags );
	count += 4;

	multibuf[count++] = Net_create_loc;

	Assert(Net_create_loc <= MAX_NET_CREATE_OBJECTS);

	memset(multibuf+count, -1, MAX_NET_CREATE_OBJECTS*sizeof(short));

	for (i = 0; i < Net_create_loc; i++)
	{
		if (Net_create_objnums[i] <= 0) {
#if 0 
// Now legal, happens if there are too much powerups in mine
			Int3(); // Illegal value in created egg object numbers
#endif
			continue;
		}

		PUT_INTEL_SHORT(multibuf+count, Net_create_objnums[i]); count += 2;

		// We created these objs so our local number = the network number
		map_objnum_local_to_local((short)Net_create_objnums[i]);
	}

	Net_create_loc = 0;

	if (count > message_length[MULTI_PLAYER_EXPLODE])
	{
		Int3(); // See Rob
	}

	multi_send_data(multibuf, message_length[MULTI_PLAYER_EXPLODE], 2);
	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)
		multi_send_decloak();

	multi_strip_robots(Player_num);
}

extern int Proximity_dropped;

/*
 * Powerup capping: Keep track of how many powerups are in level and kill these which would exceed initial limit.
 */

// Count the initial amount of Powerups in the level
void multi_powcap_count_powerups_in_mine(void)
{
	int i;

	for (i=0;i<MAX_POWERUP_TYPES;i++)
		PowerupsInMine[i]=0;
		
	for (i=0;i<=Highest_object_index;i++) 
	{
		if (Objects[i].type==OBJ_POWERUP)
		{
			if (multi_powerup_is_4pack(Objects[i].id))
				PowerupsInMine[Objects[i].id-1]+=4;
			else
				PowerupsInMine[Objects[i].id]++;
		}
	}
}

// We want to drop something. Kill every Powerup which exceeds the level limit
void multi_powcap_cap_objects()
{
	char type;
	int index;

	if (!(Game_mode & GM_NETWORK))
		return;

	// Bad interaction with savegames -- CED -- coopfix
	if(Game_mode & GM_MULTI_COOP) {
		return;
	}

	// Don't even try.
	if(Netgame.PrimaryDupFactor > 1 || Netgame.SecondaryDupFactor > 1 || Netgame.SecondaryCapFactor > 1 ) {
		return;
	}


	Players[Player_num].secondary_ammo[PROXIMITY_INDEX]+=Proximity_dropped;
	Proximity_dropped=0;

	for (index=0;index<MAX_PRIMARY_WEAPONS;index++)
	{
		type=Primary_weapon_to_powerup[index];
		if (PowerupsInMine[(int)type]>=MaxPowerupsAllowed[(int)type])
			if(Players[Player_num].primary_weapon_flags & (1 << index))
			{
				RT_LOGF(RT_LOGSERVERITY_INFO, "PIM=%d MPA=%d\n", PowerupsInMine[(int)type], MaxPowerupsAllowed[(int)type]);
				RT_LOGF(RT_LOGSERVERITY_INFO, "Killing a primary cuz there's too many! (%d)\n", type);
				Players[Player_num].primary_weapon_flags&=(~(1 << index));
			}
	}


	Players[Player_num].secondary_ammo[2]/=4;

	for (index=0;index<MAX_SECONDARY_WEAPONS;index++)
	{
		type=Secondary_weapon_to_powerup[index];

		if ((Players[Player_num].secondary_ammo[index]+PowerupsInMine[(int)type])>MaxPowerupsAllowed[(int)type])
		{
			if (MaxPowerupsAllowed[(int)type]-PowerupsInMine[(int)type]<0)
				Players[Player_num].secondary_ammo[index]=0;
			else
				Players[Player_num].secondary_ammo[index]=(MaxPowerupsAllowed[(int)type]-PowerupsInMine[(int)type]);
			RT_LOGF(RT_LOGSERVERITY_INFO, "Hey! I killed secondary type %d because PIM=%d MPA=%d\n", type, PowerupsInMine[(int)type], MaxPowerupsAllowed[(int)type]);
		}
	}

	Players[Player_num].secondary_ammo[2]*=4;

	if (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)
		if (PowerupsInMine[POW_QUAD_FIRE]+1 > MaxPowerupsAllowed[POW_QUAD_FIRE])
			Players[Player_num].flags&=(~PLAYER_FLAGS_QUAD_LASERS);

	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)
		if (PowerupsInMine[POW_CLOAK]+1 > MaxPowerupsAllowed[POW_CLOAK])
			Players[Player_num].flags&=(~PLAYER_FLAGS_CLOAKED);
}

// Adds players inventory to multi cap
void multi_powcap_adjust_cap_for_player(int pnum)
{
	char type;

	int index;

	if (!(Game_mode & GM_NETWORK))
		return;

	for (index=0;index<MAX_PRIMARY_WEAPONS;index++)
	{
		type=Primary_weapon_to_powerup[index];
		if (Players[pnum].primary_weapon_flags & (1 << index))
		    MaxPowerupsAllowed[(int)type]++;
	}

	for (index=0;index<MAX_SECONDARY_WEAPONS;index++)
	{
		type=Secondary_weapon_to_powerup[index];
		MaxPowerupsAllowed[(int)type]+=Players[pnum].secondary_ammo[index];
	}

	if (Players[pnum].flags & PLAYER_FLAGS_QUAD_LASERS)
		MaxPowerupsAllowed[POW_QUAD_FIRE]++;

	if (Players[pnum].flags & PLAYER_FLAGS_CLOAKED)
		MaxPowerupsAllowed[POW_CLOAK]++;
}

void multi_powcap_adjust_remote_cap(int pnum)
{
	char type;

	int index;

	if (!(Game_mode & GM_NETWORK))
		return;

	for (index=0;index<MAX_PRIMARY_WEAPONS;index++)
	{
		type=Primary_weapon_to_powerup[index];
		if (Players[pnum].primary_weapon_flags & (1 << index))
		    PowerupsInMine[(int)type]++;
	}

	for (index=0;index<MAX_SECONDARY_WEAPONS;index++)
	{
		type=Secondary_weapon_to_powerup[index];

		if (index==2) // PROX? Those bastards...
			PowerupsInMine[(int)type]+=(Players[pnum].secondary_ammo[index]/4);
		else
			PowerupsInMine[(int)type]+=Players[pnum].secondary_ammo[index];

	}

	if (Players[pnum].flags & PLAYER_FLAGS_QUAD_LASERS)
		PowerupsInMine[POW_QUAD_FIRE]++;

	if (Players[pnum].flags & PLAYER_FLAGS_CLOAKED)
		PowerupsInMine[POW_CLOAK]++;
}

void
multi_send_message(void)
{
	int loc = 0;
	if (Network_message_reciever != -1)
	{
		multibuf[loc] = (char)MULTI_MESSAGE;            loc += 1;
		multibuf[loc] = (char)Player_num;                       loc += 1;
		strncpy((char*)multibuf+loc, Network_message, MAX_MESSAGE_LEN); loc += MAX_MESSAGE_LEN;
		multibuf[loc-1] = '\0';
		multi_send_data(multibuf, loc, 0);
		Network_message_reciever = -1;
	}
}

void
multi_send_reappear()
{
	if(Game_mode & GM_OBSERVER) { return; }

	multi_send_position(Players[Player_num].objnum);
	
	multibuf[0] = (char)MULTI_REAPPEAR;
	multibuf[1] = (char)Player_num;
	PUT_INTEL_SHORT(multibuf+2, Players[Player_num].objnum);

	multi_send_data(multibuf, 4, 2);
	PKilledFlags[Player_num]=0;
}

void
multi_send_position(int objnum)
{
	if(Game_mode & GM_OBSERVER) { return; }

#ifdef WORDS_BIGENDIAN
	shortpos sp;
#endif
	int count=0;

	multibuf[count++] = (char)MULTI_POSITION;
	multibuf[count++] = (char)Player_num;
#ifndef WORDS_BIGENDIAN
	create_shortpos((shortpos *)(multibuf+count), Objects+objnum,0);
	count += sizeof(shortpos);
#else
	create_shortpos(&sp, Objects+objnum, 1);
	memcpy(&(multibuf[count]), (ubyte *)(sp.bytemat), 9);
	count += 9;
	memcpy(&(multibuf[count]), (ubyte *)&(sp.xo), 14);
	count += 14;
#endif
	if(Netgame.RetroProtocol) {
		multi_send_data(multibuf, count, 0);
	} else {
		// send twice while first has priority so the next one will be attached to the next bigdata packet
		multi_send_data(multibuf, count, 2);
		multi_send_data(multibuf, count, 0);
	}
}

/* 
 * I was killed. If I am host, send this info to everyone and compute kill. If I am just a Client I'll only send the kill but not compute it for me. I (Client) will wait for Host to send me my kill back together with updated game_mode related variables which are important for me to compute consistent kill.
 */
void
multi_send_kill(int objnum)
{
	if(Game_mode & GM_OBSERVER) { return; }

	// I died, tell the world.

	int killer_objnum;
	int count = 0;

	Assert(Objects[objnum].id == Player_num);
	killer_objnum = Players[Player_num].killer_objnum;

	if (multi_i_am_master())
		multibuf[count] = (char)MULTI_KILL_HOST;
	else
		multibuf[count] = (char)MULTI_KILL_CLIENT;
							count += 1;
	multibuf[count] = Player_num;			count += 1;

	if (killer_objnum > -1)
	{
		short s = (short)objnum_local_to_remote(killer_objnum, (sbyte *)&multibuf[count+2]); // do it with variable since INTEL_SHORT won't work on return val from function.
		PUT_INTEL_SHORT(multibuf+count, s);
	}
	else
	{
		PUT_INTEL_SHORT(multibuf+count, -1);
		multibuf[count+2] = (char)-1;
	}
	count += 3;
	// I am host - I know what's going on so attach game_mode related info which might be vital for correct kill computation
	if (multi_i_am_master())
	{
		multibuf[count] = Netgame.team_vector;	count += 1;
		multibuf[count] = Bounty_target;	count += 1;
	}

	if (multi_i_am_master())
	{
		multi_send_data(multibuf, count, 2);
		multi_compute_kill(killer_objnum, objnum); // THIS TRASHES THE MULTIBUF!!!
	}
	else
		multi_send_data_direct((ubyte*)multibuf, count, multi_who_is_master(), 2); // I am just a client so I'll only send my kill but not compute it, yet. I'll get response from host so I can compute it correctly

	if (Game_mode & GM_MULTI_ROBOTS)
		multi_strip_robots(Player_num);

	if (Game_mode & GM_BOUNTY && multi_i_am_master()) // update in case if needed... we could attach this to this packet but... meh...
		multi_send_bounty();
}

void
multi_send_remobj(int objnum)
{
	// Tell the other guy to remove an object from his list
	if(Game_mode & GM_OBSERVER) { return; }

	sbyte obj_owner;
	short remote_objnum;

	static char remove_obj_counter = 0; 

	if (Objects[objnum].type==OBJ_POWERUP && (Game_mode & GM_NETWORK))
	{
		if (multi_powerup_is_4pack (Objects[objnum].id))
		{
			if (PowerupsInMine[Objects[objnum].id-1]-4<0)
				PowerupsInMine[Objects[objnum].id-1]=0;
			else
				PowerupsInMine[Objects[objnum].id-1]-=4;
		}
		else
		{
			if (PowerupsInMine[Objects[objnum].id]>0)
				PowerupsInMine[Objects[objnum].id]--;
		}
	}

	multibuf[0] = (char)MULTI_REMOVE_OBJECT;

	remote_objnum = objnum_local_to_remote((short)objnum, &obj_owner);

	PUT_INTEL_SHORT(multibuf+1, remote_objnum); // Map to network objnums

	multibuf[3] = obj_owner;
	multibuf[4] = remove_obj_counter++; 

	if(Netgame.RetroProtocol) {
		int plr_count = 0;
		for(int i = 0; i < MAX_PLAYERS; i++) {
			if(Players[i].connected == CONNECT_PLAYING ) {
				plr_count += 1;
			}
		}
		
		// In a two player game, we can get both speed and packet loss prevention
		if(plr_count <= 2) {
			multi_send_data(multibuf, 5, 2); 
		} else {
			// Otherwise, send via both paths -- dup will be dropped
			multi_send_data(multibuf, 5, 1); 
			multi_send_data(multibuf, 5, 2); 
		}
	} else {
		multi_send_data(multibuf, 5, 2);
	}

	if (Network_send_objects && multi_objnum_is_past(objnum))
	{
		Network_send_objnum = -1;
	}
}

void
multi_send_quit(int why)
{
	// I am quitting the game, tell the other guy the bad news.
	if(Game_mode & GM_OBSERVER) { return; }

	Assert (why == MULTI_QUIT);

	multibuf[0] = (char)why;
	multibuf[1] = Player_num;
	multi_send_data(multibuf, 2, 2);
}

void
multi_send_cloak(void)
{
	// Broadcast a change in our pflags (made to support cloaking)
	if(Game_mode & GM_OBSERVER) { return; }

	multibuf[0] = MULTI_CLOAK;
	multibuf[1] = (char)Player_num;

	if(Netgame.RetroProtocol) {
		multi_send_data(multibuf, 2, 1);
	}
	multi_send_data(multibuf, 2, 2);

	if (Game_mode & GM_MULTI_ROBOTS)
		multi_strip_robots(Player_num);
}

void
multi_send_decloak(void)
{
	// Broadcast a change in our pflags (made to support cloaking)
	if(Game_mode & GM_OBSERVER) { return; }

	multibuf[0] = MULTI_DECLOAK;
	multibuf[1] = (char)Player_num;

	if(Netgame.RetroProtocol) {
		multi_send_data(multibuf, 2, 1);
	}
	multi_send_data(multibuf, 2, 2);
}

void multi_send_door_open(int segnum, int side, ubyte flag)
{
	if(Game_mode & GM_OBSERVER) { return; }

	(void)flag;
	multibuf[0] = MULTI_DOOR_OPEN;
	PUT_INTEL_SHORT(multibuf+1, segnum );
	multibuf[3] = (sbyte)side;
	if(Netgame.RetroProtocol) {
		multi_send_data(multibuf, 4, 1);
	} else {
		multi_send_data(multibuf, 4, 2);
	}
}

//
// Part 3 : Functions that change or prepare the game for multiplayer use.
//          Not including functions needed to syncronize or start the
//          particular type of multiplayer game.  Includes preparing the
//                      mines, player structures, etc.

void
multi_send_create_explosion(int pnum)
{
	// Send all data needed to create a remote explosion
	if(Game_mode & GM_OBSERVER) { return; }

	int count = 0;

	multibuf[count] = MULTI_CREATE_EXPLOSION;       count += 1;
	multibuf[count] = (sbyte)pnum;                  count += 1;
	//                                                                                                      -----------
	//                                                                                                      Total size = 2

	multi_send_data(multibuf, count, 0);
}

void
multi_send_controlcen_fire(vms_vector *to_goal, int best_gun_num, int objnum)
{
	if(Game_mode & GM_OBSERVER) { return; }

#ifdef WORDS_BIGENDIAN
	vms_vector swapped_vec;
#endif
	int count = 0;

	multibuf[count] = MULTI_CONTROLCEN_FIRE;                count +=  1;
#ifndef WORDS_BIGENDIAN
	memcpy(multibuf+count, to_goal, 12);                    count += 12;
#else
	swapped_vec.x = (fix)INTEL_INT( (int)to_goal->x );
	swapped_vec.y = (fix)INTEL_INT( (int)to_goal->y );
	swapped_vec.z = (fix)INTEL_INT( (int)to_goal->z );
	memcpy(multibuf+count, &swapped_vec, 12);				count += 12;
#endif
	multibuf[count] = (char)best_gun_num;                   count +=  1;
	PUT_INTEL_SHORT(multibuf+count, objnum );     count +=  2;
	//                                                                                                                      ------------
	//                                                                                                                      Total  = 16
	multi_send_data(multibuf, count, 0);
}

void
multi_send_create_powerup(int powerup_type, int segnum, int objnum, vms_vector *pos)
{
	if(Game_mode & GM_OBSERVER) { return; }

	// Create a powerup on a remote machine, used for remote
	// placement of used powerups like missiles and cloaking
	// powerups.

#ifdef WORDS_BIGENDIAN
	vms_vector swapped_vec;
#endif
	int count = 0;

	// CED -- this makes no sense
	//multi_send_position(Players[Player_num].objnum);

	if (Game_mode & GM_NETWORK)
	{
		if (multi_powerup_is_4pack(powerup_type))
			PowerupsInMine[powerup_type-1]+=4;
		else
			PowerupsInMine[powerup_type]++;
	}

	multibuf[count] = MULTI_CREATE_POWERUP;         count += 1;
	multibuf[count] = Player_num;                                      count += 1;
	multibuf[count] = powerup_type;                                 count += 1;
	PUT_INTEL_SHORT(multibuf+count, segnum );     count += 2;
	PUT_INTEL_SHORT(multibuf+count, objnum );     count += 2;
#ifndef WORDS_BIGENDIAN
	memcpy(multibuf+count, pos, sizeof(vms_vector));  count += sizeof(vms_vector);
#else
	swapped_vec.x = (fix)INTEL_INT( (int)pos->x );
	swapped_vec.y = (fix)INTEL_INT( (int)pos->y );
	swapped_vec.z = (fix)INTEL_INT( (int)pos->z );
	memcpy(multibuf+count, &swapped_vec, 12);				count += 12;
#endif
	//                                                                                                            -----------
	//       
	multi_send_data(multibuf, count, 2);

	if (Network_send_objects && multi_objnum_is_past(objnum))
	{
		Network_send_objnum = -1;
	}

	map_objnum_local_to_local(objnum);
}

void
multi_send_play_sound(int sound_num, fix volume)
{
	if(Game_mode & GM_OBSERVER) { return; }

	int count = 0;

	multibuf[count] = MULTI_PLAY_SOUND;                     count += 1;
	multibuf[count] = Player_num;                                   count += 1;
	multibuf[count] = (char)sound_num;                      count += 1;
	multibuf[count] = (char)(volume >> 12); count += 1;
	//                                                                                                         -----------
	//                                                                                                         Total = 4
	multi_send_data(multibuf, count, 0);
}

void
multi_send_audio_taunt(int taunt_num)
{
#ifdef AUDIO_TAUNTS
	int audio_taunts[4] = {
		// Begin addition by GF
		SOUND_CONTROL_CENTER_WARNING_SIREN,
		SOUND_HOMING_WARNING,
		SOUND_CONTROL_CENTER_DESTROYED,
		SOUND_MINE_BLEW_UP
		// End addition by GF
	};


	Assert(taunt_num >= 0);
	Assert(taunt_num < 4);

	digi_play_sample( audio_taunts[taunt_num], F1_0 );
	multi_send_play_sound(audio_taunts[taunt_num], F1_0);
#endif
}

void
multi_send_score(void)
{
	if(Game_mode & GM_OBSERVER) { return; }

	// Send my current score to all other players so it will remain
	// synced.
	int count = 0;

	if ((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_MULTI_ROBOTS)) {
		multi_sort_kill_list();
		multibuf[count] = MULTI_SCORE;                  count += 1;
		multibuf[count] = Player_num;                           count += 1;
		PUT_INTEL_INT(multibuf+count, Players[Player_num].score);  count += 4;
		multi_send_data(multibuf, count, 0);
	}
}

void
multi_send_trigger(int triggernum)
{
	// Send an event to trigger something in the mine
	if(Game_mode & GM_OBSERVER) { return; }

	int count = 0;

	multibuf[count] = MULTI_TRIGGER;                                count += 1;
	multibuf[count] = Player_num;                                   count += 1;
	multibuf[count] = (ubyte)triggernum;            count += 1;

	multi_send_data(multibuf, count, 2);
}

void
multi_send_hostage_door_status(int wallnum)
{
	// Tell the other player what the hit point status of a hostage door
	// should be

	if(Game_mode & GM_OBSERVER) { return; }

	int count = 0;

	Assert(Walls[wallnum].type == WALL_BLASTABLE);

	multibuf[count] = MULTI_HOSTAGE_DOOR;           count += 1;
	PUT_INTEL_SHORT(multibuf+count, wallnum );           count += 2;
	PUT_INTEL_INT(multibuf+count, Walls[wallnum].hps );  count += 4;

	multi_send_data(multibuf, count, 0);
}

void multi_consistency_error(int reset)
{
	static int count = 0;

	if (reset)
		count = 0;

	if (++count < 10)
		return;

	if (Game_wind)
		window_set_visible(Game_wind, 0);
	nm_messagebox(NULL, 1, TXT_OK, TXT_CONSISTENCY_ERROR);
	if (Game_wind)
		window_set_visible(Game_wind, 1);
	count = 0;
	multi_quit_game = 1;
	game_leave_menus();
	multi_reset_stuff();
}

int is_dupable_primary(int id) {
	switch(id) {
		case POW_LASER:
		case POW_QUAD_FIRE:
		case POW_VULCAN_WEAPON: 
		case POW_VULCAN_AMMO: 
		case POW_SPREADFIRE_WEAPON: 
		case POW_PLASMA_WEAPON:
		case POW_FUSION_WEAPON:
			return 1;
	}

	return 0; 

}

int is_dupable_secondary(int id) {
	switch(id) {
		case POW_MISSILE_1:
		case POW_MISSILE_4:
		case POW_HOMING_AMMO_1:
		case POW_HOMING_AMMO_4:
		case POW_PROXIMITY_WEAPON:
		case POW_SMARTBOMB_WEAPON:
		case POW_MEGA_WEAPON: 
			return 1;
	}
	return 0;
}

int multi_received_objects = 0; 

char original_object_types[MAX_OBJECTS];
void save_original_objects() {
	for (int i=0; i<=MAX_OBJECTS; i++) {
		original_object_types[i] = Objects[i].type; 
	}
}

int was_original_object(int i) {
	if(original_object_types[i] != OBJ_NONE) {
		return 1;
	}

	return 0; 
}

void
multi_prep_level(void)
{
	// Do any special stuff to the level required for games
	// before we begin playing in it.

	// Player_num MUST be set before calling this procedure.

	// This function must be called before checksuming the Object array,
	// since the resulting checksum with depend on the value of Player_num
	// at the time this is called.

	int i;
	int     cloak_count, inv_count;

	Assert(Game_mode & GM_MULTI);

	Assert(NumNetPlayerPositions > 0);

	Bounty_target = 0;

	multi_consistency_error(1);

	for (i=0;i<MAX_PLAYERS;i++)
	{
		PKilledFlags[i]=1;
		multi_sending_message[i] = 0;
		if (imulti_new_game)
			init_player_stats_new_ship(i);
	}

	for (i = 0; i < NumNetPlayerPositions; i++)
	{
		if (i != Player_num)
			Objects[Players[i].objnum].control_type = CT_REMOTE;
		Objects[Players[i].objnum].movement_type = MT_PHYSICS;
		multi_reset_player_object(&Objects[Players[i].objnum]);
		Netgame.players[i].LastPacketTime = 0;
	}

	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
	{
		robot_controlled[i] = -1;
		robot_agitation[i] = 0;
		robot_fired[i] = 0;
	}

	Viewer = ConsoleObject = &Objects[Players[Player_num].objnum];

	if (!(Game_mode & GM_MULTI_COOP))
	{
		multi_delete_extra_objects(); // Removes monsters from level
	}

	if (Game_mode & GM_MULTI_ROBOTS)
	{
		multi_set_robot_ai(); // Set all Robot AI to types we can cope with
	}

	if (Game_mode & GM_NETWORK)
	{
		multi_powcap_adjust_cap_for_player(Player_num);
		multi_send_powcap_update();
	}

	inv_count = 0;
	cloak_count = 0;
	for (i=0; i<=Highest_object_index; i++)
	{
		int objnum;

		if ((Objects[i].type == OBJ_HOSTAGE) && !(Game_mode & GM_MULTI_COOP))
		{
			objnum = obj_create(OBJ_POWERUP, POW_SHIELD_BOOST, Objects[i].segnum, &Objects[i].pos, &vmd_identity_matrix, Powerup_info[POW_SHIELD_BOOST].size, CT_POWERUP, MT_PHYSICS, RT_POWERUP);
			obj_delete(i);
			if (objnum != -1)
			{
				Objects[objnum].rtype.vclip_info.vclip_num = Powerup_info[POW_SHIELD_BOOST].vclip_num;
				Objects[objnum].rtype.vclip_info.frametime = Vclip[Objects[objnum].rtype.vclip_info.vclip_num].frame_time;
				Objects[objnum].rtype.vclip_info.framenum = 0;
				Objects[objnum].mtype.phys_info.drag = 512;     //1024;
				Objects[objnum].mtype.phys_info.mass = F1_0;
				vm_vec_zero(&Objects[objnum].mtype.phys_info.velocity);
			}
			continue;
		}

		if (Objects[i].type == OBJ_POWERUP)
		{
			if (Objects[i].id == POW_EXTRA_LIFE)
			{
				if (!(Netgame.AllowedItems & NETFLAG_DOINVUL))
				{
					Objects[i].id = POW_SHIELD_BOOST;
					Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
					Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
				}
				else
				{
					Objects[i].id = POW_INVULNERABILITY;
					Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
					Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
				}

			}

			if (!(Game_mode & GM_MULTI_COOP))
				if ((Objects[i].id >= POW_KEY_BLUE) && (Objects[i].id <= POW_KEY_GOLD))
				{
					Objects[i].id = POW_SHIELD_BOOST;
					Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
					Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
				}

			if (Objects[i].id == POW_INVULNERABILITY) {
				if (inv_count >= 3 || (!(Netgame.AllowedItems & NETFLAG_DOINVUL))) {
					Objects[i].id = POW_SHIELD_BOOST;
					Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
					Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
				} else
					inv_count++;
			}

			if (Objects[i].id == POW_CLOAK) {
				if (cloak_count >= 3 || (!(Netgame.AllowedItems & NETFLAG_DOCLOAK))) {
					Objects[i].id = POW_SHIELD_BOOST;
					Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
					Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
				} else
					cloak_count++;
			}

			if (Objects[i].id == POW_FUSION_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOFUSION))
				bash_to_shield (i,"fusion");
			if (Objects[i].id == POW_MEGA_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOMEGA))
				bash_to_shield (i,"mega");
			if (Objects[i].id == POW_SMARTBOMB_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOSMART))
				bash_to_shield (i,"smartmissile");
			if (Objects[i].id == POW_VULCAN_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOVULCAN))
				bash_to_shield (i,"vulcan");
			if (Objects[i].id == POW_VULCAN_WEAPON && Netgame.LowVulcan)
				Objects[i].ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT/2; //1250		
			if (Objects[i].id == POW_PLASMA_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOPLASMA))
				bash_to_shield (i,"plasma");
			if (Objects[i].id == POW_PROXIMITY_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOPROXIM))
				bash_to_shield (i,"proximity");
			if (Objects[i].id==POW_VULCAN_AMMO && 
				( (!(Netgame.AllowedItems & NETFLAG_DOVULCANAMMO)) ||
				  Netgame.LowVulcan 
				)
			   )
				bash_to_shield(i,"vulcan ammo");
			if (Objects[i].id == POW_SPREADFIRE_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOSPREAD))
				bash_to_shield (i,"spread");
			if (Objects[i].id == POW_LASER && !(Netgame.AllowedItems & NETFLAG_DOLASER))
				bash_to_shield (i,"Laser powerup");
			if (Objects[i].id == POW_HOMING_AMMO_1 && !(Netgame.AllowedItems & NETFLAG_DOHOMING))
				bash_to_shield (i,"Homing");
			if (Objects[i].id == POW_HOMING_AMMO_4 && !(Netgame.AllowedItems & NETFLAG_DOHOMING))
				bash_to_shield (i,"Homing");
			if (Objects[i].id == POW_QUAD_FIRE && !(Netgame.AllowedItems & NETFLAG_DOQUAD))
				bash_to_shield (i,"Quad Lasers");
		}
	}

	if(! multi_received_objects ) {
		int old_highest_object = Highest_object_index;
		save_original_objects(); 

		for (i=0; i<=old_highest_object; i++)
		{
			if(! was_original_object(i)) { continue; }			
			int objnum;

			if ((Objects[i].type == OBJ_HOSTAGE) && !(Game_mode & GM_MULTI_COOP))
			{
				objnum = obj_create(OBJ_POWERUP, POW_SHIELD_BOOST, Objects[i].segnum, &Objects[i].pos, &vmd_identity_matrix, Powerup_info[POW_SHIELD_BOOST].size, CT_POWERUP, MT_PHYSICS, RT_POWERUP);
				obj_delete(i);
				if (objnum != -1)
				{
					Objects[objnum].rtype.vclip_info.vclip_num = Powerup_info[POW_SHIELD_BOOST].vclip_num;
					Objects[objnum].rtype.vclip_info.frametime = Vclip[Objects[objnum].rtype.vclip_info.vclip_num].frame_time;
					Objects[objnum].rtype.vclip_info.framenum = 0;
					Objects[objnum].mtype.phys_info.drag = 512;     //1024;
					Objects[objnum].mtype.phys_info.mass = F1_0;
					vm_vec_zero(&Objects[objnum].mtype.phys_info.velocity);
				}
				continue;
			}

			if (Objects[i].type == OBJ_POWERUP)
			{
				if(Netgame.PrimaryDupFactor > 1) {
					if(is_dupable_primary(Objects[i].id)) {
						for(int dup = 0; dup < Netgame.PrimaryDupFactor - 1; dup++) {
							objnum = obj_create(OBJ_POWERUP, Objects[i].id, Objects[i].segnum, &Objects[i].pos, &vmd_identity_matrix, Powerup_info[Objects[i].id].size, CT_POWERUP, MT_PHYSICS, RT_POWERUP);
							// RT_LOGF(RT_LOGSERVERITY_MEDIUM, "Duped %d (%d) as %d\n", i, objnum, Objects[i].id);
							if (objnum != -1)
							{
								Objects[objnum].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
								Objects[objnum].rtype.vclip_info.frametime = Vclip[Objects[objnum].rtype.vclip_info.vclip_num].frame_time;
								Objects[objnum].rtype.vclip_info.framenum = 0;
								Objects[objnum].mtype.phys_info.drag = 512;     //1024;
								Objects[objnum].mtype.phys_info.mass = F1_0;
								vm_vec_zero(&Objects[objnum].mtype.phys_info.velocity);
							}
						}
					
					}
				}

				if(Netgame.SecondaryDupFactor > 1) {
					if(is_dupable_secondary(Objects[i].id)) {					
						for(int dup = 0; dup < Netgame.SecondaryDupFactor - 1; dup++) {
							objnum = obj_create(OBJ_POWERUP, Objects[i].id, Objects[i].segnum, &Objects[i].pos, &vmd_identity_matrix, Powerup_info[Objects[i].id].size, CT_POWERUP, MT_PHYSICS, RT_POWERUP);

							if (objnum != -1)
							{
								Objects[objnum].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
								Objects[objnum].rtype.vclip_info.frametime = Vclip[Objects[objnum].rtype.vclip_info.vclip_num].frame_time;
								Objects[objnum].rtype.vclip_info.framenum = 0;
								Objects[objnum].mtype.phys_info.drag = 512;     //1024;
								Objects[objnum].mtype.phys_info.mass = F1_0;
								vm_vec_zero(&Objects[objnum].mtype.phys_info.velocity);
							}
						}
					
					}
				}			
				
			}
		}

		if(Netgame.SecondaryCapFactor > 0) {
			int max_homers = Netgame.SecondaryCapFactor == 1 ? 6 : 2; 
			int max_smarts = Netgame.SecondaryCapFactor == 1 ? 6 : 2; 

			int num_homers = 0;
			int num_smarts = 0; 
			for (i=0; i<=Highest_object_index; i++)
			{
				if(Objects[i].id == POW_HOMING_AMMO_1) {
					if(num_homers < max_homers) {
						num_homers++;
					} else {
						bash_to_shield (i,"Homing");
					}
				}

				if(Objects[i].id == POW_HOMING_AMMO_4) {
					if(num_homers + 4 <= max_homers) {
						num_homers += 4;
					} else if(num_homers + 1 <= max_homers) {
						Objects[i].id = POW_HOMING_AMMO_1;
						Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
						Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;

						num_homers += 1; 
					} else {
						bash_to_shield (i,"Homing");
					}
				}

				if(Objects[i].id == POW_SMARTBOMB_WEAPON) {
					if(num_smarts < max_smarts) {
						num_smarts++;
					} else {
						bash_to_shield (i,"smartmissile");
					}
				}
			}
		}
	}

	multi_sort_kill_list();

	multi_show_player_list();

	ConsoleObject->control_type = CT_FLYING;

	reset_player_object();

	imulti_new_game=0;
}

int multi_level_sync(void)
{
	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			return net_udp_level_sync();
			break;
#endif
		default:
			RT_LOG(RT_LOGSERVERITY_HIGH, "Protocol handling missing in multi_level_sync\n");
			break;
	}
}

void multi_set_robot_ai(void)
{
	// Go through the objects array looking for robots and setting
	// them to certain supported types of NET AI behavior.

//	int i;
//
//	for (i = 0; i <= Highest_object_index; i++)
//	{
//		if (Objects[i].type == OBJ_ROBOT) {
//			Objects[i].ai_info.REMOTE_OWNER = -1;
//			if (Objects[i].ai_info.behavior == AIB_STATION)
//				Objects[i].ai_info.behavior = AIB_NORMAL;
//		}
//	}
}

int multi_delete_extra_objects()
{
	int i;
	int nnp=0;
	object *objp;
	
	// Go through the object list and remove any objects not used in
	// 'Anarchy!' games.

	// This function also prints the total number of available multiplayer
	// positions in this level, even though this should always be 8 or more!

	objp = Objects;
	for (i=0;i<=Highest_object_index;i++) {
		if ((objp->type==OBJ_PLAYER) || (objp->type==OBJ_GHOST)) 
			nnp++;
		else if ((objp->type==OBJ_ROBOT) && (Game_mode & GM_MULTI_ROBOTS))
			;
		else if ( (objp->type!=OBJ_NONE) && (objp->type!=OBJ_PLAYER) && (objp->type!=OBJ_POWERUP) && (objp->type!=OBJ_CNTRLCEN) && (objp->type!=OBJ_HOSTAGE) )
			obj_delete(i);
		objp++;
	}

	return nnp;
}

// Returns 1 if player is Master/Host of this game
int multi_i_am_master(void)
{
	return (Player_num == 0);
}

// Returns the Player_num of Master/Host of this game
int multi_who_is_master(void)
{
	return 0;
}

void change_playernum_to( int new_Player_num )
{
// 	if (Player_num > -1)
// 		memcpy( Players[new_Player_num].callsign, Players[Player_num].callsign, CALLSIGN_LEN+1 );
	if (Player_num > -1)
	{
		char *buf;
		MALLOC(buf,char,CALLSIGN_LEN+1);
		memcpy( buf, Players[Player_num].callsign, CALLSIGN_LEN+1 );
		strcpy(Players[new_Player_num].callsign,buf);
		d_free(buf);
	}

	Player_num = new_Player_num;
}

int multi_all_players_alive()
{
	int i;
	for (i=0;i<N_players;i++)
	{
		if (PKilledFlags[i] && Players[i].connected)
			return (0);
	}
	return (1);
}

void multi_send_powcap_update ()
{
	int i;

	multibuf[0]=MULTI_POWCAP_UPDATE;
	for (i=0;i<MAX_POWERUP_TYPES;i++)
		multibuf[i+1]=MaxPowerupsAllowed[i];

	multi_send_data(multibuf, MAX_POWERUP_TYPES+1, 2);
}

void multi_do_powcap_update (const ubyte *buf)
{
	int i;

	for (i=0;i<MAX_POWERUP_TYPES;i++)
		if (buf[i+1]>MaxPowerupsAllowed[i])
			MaxPowerupsAllowed[i]=buf[i+1];
}

#define POWERUPADJUSTS 2
int PowerupAdjustMapping[]={11,19};

int multi_powerup_is_4pack (int id)
{
	int i;

	for (i=0;i<POWERUPADJUSTS;i++)
		if (id==PowerupAdjustMapping[i])
			return (1);
	return (0);
}

void multi_send_kill_goal_counts()
{
	int i,count=1;
	multibuf[0]=MULTI_KILLGOALS;

	for (i=0;i<MAX_PLAYERS;i++)
	{
		*(char *)(multibuf+count)=(char)Players[i].KillGoalCount;
		count++;
	}

	for(i=0; i < 2; i++) {
		*(char *)(multibuf+count)=(char)Netgame.TeamKillGoalCount[i];
		count++;
	}

	multi_send_data(multibuf, count, 2);
}

void multi_do_kill_goal_counts(const ubyte *buf)
{
	int i,count=1;

	for (i=0;i<MAX_PLAYERS;i++)
	{
		Players[i].KillGoalCount=*(char *)(buf+count);
		count++;
	}

	for (i=0;i<2;i++)
	{
		Netgame.TeamKillGoalCount[i]=*(char *)(buf+count);
		count++;
	}

}

void multi_send_heartbeat ()
{
	if (!Netgame.PlayTimeAllowed)
		return;

	multibuf[0]=MULTI_HEARTBEAT;
	PUT_INTEL_INT(multibuf+1, ThisLevelTime);
	multi_send_data(multibuf, 5, 0);
}

void multi_do_heartbeat (const ubyte *buf)
{
	fix num;

	num = GET_INTEL_INT(buf + 1);

	ThisLevelTime=num;
}


void multi_check_for_killgoal_winner ()
{
	int i,best=0,bestnum=0;
	object *objp;

	if (Control_center_destroyed)
		return;


	if (Game_mode & GM_TEAM)
	{
		int winner = 0;
		if(Netgame.TeamKillGoalCount[1] > Netgame.TeamKillGoalCount[0]) {
			winner = 1; 
		}

		HUD_init_message(HM_MULTI, "The winner is %s, with the most kills!",Netgame.team_name[winner]);

	} else {
		for (i=0;i<N_players;i++)
		{
			if (Players[i].KillGoalCount>best)
			{
				best=Players[i].KillGoalCount;
				bestnum=i;
			}
		}

		if (bestnum==Player_num)
		{
			HUD_init_message(HM_MULTI, "You have the best score at %d kills!",best);
			//Players[Player_num].shields=i2f(200);
		}
		else

			HUD_init_message(HM_MULTI, "%s has the best score with %d kills!",Players[bestnum].callsign,best);
	}



	HUD_init_message_literal(HM_MULTI, "The control center has been destroyed!");

	objp=obj_find_first_of_type (OBJ_CNTRLCEN);
	net_destroy_controlcen (objp);
}

extern char *RankStrings[];

void multi_add_lifetime_kills ()
{
	// This function adds a kill to lifetime stats of this player, and possibly
	// gives a promotion.  If so, it will tell everyone else

	int oldrank;

	if (!(Game_mode & GM_NETWORK))
		return;

	oldrank=GetMyNetRanking();

	PlayerCfg.NetlifeKills++;

	if (oldrank!=GetMyNetRanking())
	{
		multi_send_ranking();
		if (!PlayerCfg.NoRankings)
		{
			HUD_init_message(HM_MULTI, "You have been promoted to %s!",RankStrings[GetMyNetRanking()]);
			digi_play_sample (SOUND_CONTROL_CENTER_WARNING_SIREN,F1_0*2);
			Netgame.players[Player_num].rank=GetMyNetRanking();
		}
	}
}

void multi_add_lifetime_killed ()
{
	// This function adds a "killed" to lifetime stats of this player, and possibly
	// gives a demotion.  If so, it will tell everyone else

	int oldrank;

	if (!(Game_mode & GM_NETWORK))
		return;

	oldrank=GetMyNetRanking();

	PlayerCfg.NetlifeKilled++;

	if (oldrank!=GetMyNetRanking())
	{
		multi_send_ranking();
		Netgame.players[Player_num].rank=GetMyNetRanking();

		if (!PlayerCfg.NoRankings)
			HUD_init_message(HM_MULTI, "You have been demoted to %s!",RankStrings[GetMyNetRanking()]);

	}
}

void multi_send_ranking ()
{
	multibuf[0]=(char)MULTI_RANK;
	multibuf[1]=(char)Player_num;
	multibuf[2]=(char)GetMyNetRanking();

	multi_send_data (multibuf,3,2);
}

void multi_do_ranking (char *buf)
{
	char rankstr[20];
	char pnum=buf[1];
	char rank=buf[2];

	if (Netgame.players[(int)pnum].rank<rank)
		strcpy (rankstr,"promoted");
	else if (Netgame.players[(int)pnum].rank>rank)
		strcpy (rankstr,"demoted");
	else
		return;

	Netgame.players[(int)pnum].rank=rank;

	if (!PlayerCfg.NoRankings)
		HUD_init_message(HM_MULTI, "%s has been %s to %s!",Players[(int)pnum].callsign,rankstr,RankStrings[(int)rank]);
}

// Decide if fire from "killer" is friendly. If yes return 1 (no harm to me) otherwise 0 (damage me)
int multi_maybe_disable_friendly_fire(object *killer)
{
	if (!(Game_mode & GM_NETWORK)) // no Multiplayer game -> always harm me!
		return 0;
	if (!Netgame.NoFriendlyFire) // friendly fire is activated -> harm me!
		return 0;
	if (killer == NULL) // no actual killer -> harm me!
		return 0;
	if (killer->type != OBJ_PLAYER) // not a player -> harm me!
		return 0;
	if (Game_mode & GM_MULTI_COOP) // coop mode -> don't harm me!
		return 1;
	else if (Game_mode & GM_TEAM) // team mode - find out if killer is in my team
	{
		if (get_team(Player_num) == get_team(killer->id)) // in my team -> don't harm me!
			return 1;
		else // opposite team -> harm me!
			return 0;
	}
	return 0; // all other cases -> harm me!
}

void multi_send_damage(fix damage, fix shields, ubyte killer_type, ubyte killer_id, ubyte damage_type, object* source)
{
	if (Game_mode & GM_OBSERVER) { return; }

	// Sending damage to the host isn't interesting if there cannot be any observers.
	if (Netgame.max_numobservers == 0) { return; }

	// Calculate new shields amount.
	if (shields < damage)
		shields = 0;
	else
		shields -= damage;

	// Setup damage packet.
	multibuf[0] = MULTI_DAMAGE;
	multibuf[1] = Player_num;
	multibuf[2] = (damage >> 24) & 0xFF;
	multibuf[3] = (damage >> 16) & 0xFF;
	multibuf[4] = (damage >> 8) & 0xFF;
	multibuf[5] = damage & 0xFF;
	multibuf[6] = (shields >> 24) & 0xFF;
	multibuf[7] = (shields >> 16) & 0xFF;
	multibuf[8] = (shields >> 8) & 0xFF;
	multibuf[9] = shields & 0xFF;
	multibuf[10] = killer_type;
	multibuf[11] = killer_id;
	multibuf[12] = damage_type;
	if (source == NULL)
	{
		multibuf[13] = 0;
		multibuf[14] = 0;
	}
	else if (source->type == OBJ_WEAPON)
	{
		multibuf[13] = OBJ_WEAPON;
		multibuf[14] = 0;
	}
	else if (source->type == OBJ_PLAYER)
	{
		multibuf[13] = OBJ_WEAPON;
		multibuf[14] = source->id;
	}
	else
	{
		multibuf[13] = 0;
		multibuf[14] = 0;
	}

	if (multi_i_am_master())
		multi_do_damage( multibuf );

	multi_send_data_direct( multibuf, 15, multi_who_is_master(), 2 );
}

void multi_do_damage( const ubyte *buf )
{
	if (Game_mode & GM_OBSERVER)
	{
		Players[buf[1]].shields = ((fix)buf[6] << 24) + ((fix)buf[7] << 16) + ((fix)buf[8] << 8) + (fix)buf[9];
		if (Players[Player_num].hours_total - Players[buf[1]].shields_time_hours > 1 || Players[Player_num].hours_total - Players[buf[1]].shields_time_hours == 1 && i2f(3600) + Players[Player_num].time_total - Players[buf[1]].shields_time > i2f(2) || Players[Player_num].time_total - Players[buf[1]].shields_time > i2f(2)) {
			Players[buf[1]].shields_delta = 0;
		}
		Players[buf[1]].shields_delta -= ((fix)buf[2] << 24) + ((fix)buf[3] << 16) + ((fix)buf[4] << 8) + (fix)buf[5];
		Players[buf[1]].shields_time = Players[Player_num].time_total;
		Players[buf[1]].shields_time_hours = Players[Player_num].hours_total;
	}
}

void multi_send_repair(fix repair, fix shields, ubyte sourcetype)
{
	if (Game_mode & GM_OBSERVER) { return; }

	// Sending damage to the host isn't interesting if there cannot be any observers.
	if (Netgame.max_numobservers == 0) { return; }

	// Calculate new shields amount.
	if (shields + repair > MAX_SHIELDS)
		shields = MAX_SHIELDS;
	else
		shields += repair;
	
	// Setup repair packet.
	multibuf[0] = MULTI_REPAIR;
	multibuf[1] = Player_num;
	multibuf[2] = (repair >> 24) & 0xFF;
	multibuf[3] = (repair >> 16) & 0xFF;
	multibuf[4] = (repair >> 8) & 0xFF;
	multibuf[5] = repair & 0xFF;
	multibuf[6] = (shields >> 24) & 0xFF;
	multibuf[7] = (shields >> 16) & 0xFF;
	multibuf[8] = (shields >> 8) & 0xFF;
	multibuf[9] = shields & 0xFF;
	multibuf[10] = sourcetype;

	if (multi_i_am_master())
		multi_do_repair( multibuf );
	
	multi_send_data_direct( multibuf, 11, multi_who_is_master(), 2);
}

void multi_do_repair( const ubyte *buf )
{
	if (Game_mode & GM_OBSERVER)
	{
		Players[buf[1]].shields = ((fix)buf[6] << 24) + ((fix)buf[7] << 16) + ((fix)buf[8] << 8) + (fix)buf[9];
		if (Players[Player_num].hours_total - Players[buf[1]].shields_time_hours > 1 || Players[Player_num].hours_total - Players[buf[1]].shields_time_hours == 1 && i2f(3600) + Players[Player_num].time_total - Players[buf[1]].shields_time > i2f(2) || Players[Player_num].time_total - Players[buf[1]].shields_time > i2f(2)) {
			Players[buf[1]].shields_delta = 0;
		}
		Players[buf[1]].shields_delta += ((fix)buf[2] << 24) + ((fix)buf[3] << 16) + ((fix)buf[4] << 8) + (fix)buf[5];
		Players[buf[1]].shields_time = Players[Player_num].time_total;
		Players[buf[1]].shields_time_hours = Players[Player_num].hours_total;
	}
}

/* Bounty packer sender and handler */
void multi_send_bounty( void )
{
	if(Game_mode & GM_OBSERVER) { return; }

	/* Test game mode */
	if( !( Game_mode & GM_BOUNTY ) )
		return;
	if ( !multi_i_am_master() )
		return;
	
	/* Add opcode, target ID and how often we re-assigned */
	multibuf[0] = MULTI_DO_BOUNTY;
	multibuf[1] = (char)Bounty_target;
	
	/* Send data */
	multi_send_data( multibuf, 2, 2 );
}

void multi_do_bounty( const ubyte *buf )
{
	if ( multi_i_am_master() )
		return;
	
	multi_new_bounty_target( buf[1] );
}

void multi_send_obs_update(ubyte event, ubyte event_data) {
	if(! multi_i_am_master()) { return; }

	multibuf[0] = MULTI_OBS_UPDATE; 
	multibuf[1] = event;
	multibuf[2] = event_data;
	multibuf[3] = Netgame.numobservers;

	for(int i = 0; i < Netgame.numobservers; i++) {
		memcpy(&multibuf[4 + i*8], &Netgame.observers[i].callsign, 8);
	}

	multi_send_data( multibuf, 4 + 8*MAX_OBSERVERS, 2 );
}

void multi_do_obs_update(const ubyte *buf) {
	if(multi_i_am_master()) { return; }

	if(Netgame.max_numobservers < Netgame.numobservers) {
		Netgame.max_numobservers = Netgame.numobservers;
	}
	Netgame.numobservers = buf[3];
	for(int i = 0; i < Netgame.numobservers; i++) {
		memcpy(&Netgame.observers[i].callsign, &buf[4+i*8], 8); 
	}

	// Someone joined
	if(buf[1] == 0) {
		char who_joined[9];
		strncpy(who_joined, (char*) &buf[4 + buf[2]*8], 8); 
		HUD_init_message(HM_MULTI, "%s is now observing.", who_joined);
	}
}

void multi_new_bounty_target( int pnum )
{
	/* If it's already the same, don't do it */
	if( Bounty_target == pnum )
		return;
	
	/* Set the target */
	Bounty_target = pnum;
	
	if(Netgame.FairColors)
		selected_player_rgb = player_rgb_all_blue; 
	else if(Netgame.BlackAndWhitePyros) 
		selected_player_rgb = player_rgb_alt; 
	else
		selected_player_rgb = player_rgb;

	int color = Netgame.players[pnum].color;

	/* Send a message */
	HUD_init_message( HM_MULTI, "%c%c%s is the new target!", CC_COLOR,
		BM_XRGB( selected_player_rgb[color].r, selected_player_rgb[color].g, selected_player_rgb[color].b ),
		Players[Bounty_target].callsign );

	digi_play_sample( SOUND_CONTROL_CENTER_WARNING_SIREN, F1_0 * 3 );
}

void multi_do_save_game(const ubyte *buf)
{
	int count = 1;
	ubyte slot;
	uint id;
	char desc[25];

	slot = *(ubyte *)(buf+count);			count += 1;
	id = GET_INTEL_INT(buf+count);			count += 4;
	memcpy( desc, &buf[count], 20 );		count += 20;

	multi_save_game( slot, id, desc );
}

void multi_do_restore_game(const ubyte *buf)
{
	int count = 1;
	ubyte slot;
	uint id;

	slot = *(ubyte *)(buf+count);			count += 1;
	id = GET_INTEL_INT(buf+count);			count += 4;

	multi_restore_game( slot, id );
}

void multi_send_save_game(ubyte slot, uint id, char * desc)
{
	if(Game_mode & GM_OBSERVER) { return; }

	int count = 0;
	
	multibuf[count] = MULTI_SAVE_GAME;		count += 1;
	multibuf[count] = slot;				count += 1; // Save slot=0
	PUT_INTEL_INT( multibuf+count, id );		count += 4; // Save id
	memcpy( &multibuf[count], desc, 20 );		count += 20;
	multi_send_data(multibuf, count, 2);
}

void multi_send_restore_game(ubyte slot, uint id)
{
	int count = 0;
	
	multibuf[count] = MULTI_RESTORE_GAME;		count += 1;
	multibuf[count] = slot;				count += 1; // Save slot=0
	PUT_INTEL_INT( multibuf+count, id );		count += 4; // Save id
	multi_send_data(multibuf, count, 2);
}

void multi_initiate_save_game()
{
	fix game_id = 0;
	int i, j, slot;
	char filename[PATH_MAX];
	char desc[24];

	if ((Endlevel_sequence) || (Control_center_destroyed))
		return;

	if (!multi_i_am_master())
	{
		HUD_init_message_literal(HM_MULTI, "Only host is allowed to save a game!");
		return;
	}
	if (!multi_all_players_alive())
	{
		HUD_init_message_literal(HM_MULTI, "Can't save! All players must be alive!");
		return;
	}
	for (i = 0; i < N_players; i++)
	{
		for (j = 0; j < N_players; j++)
		{
			if (i != j && !d_stricmp(Players[i].callsign, Players[j].callsign))
			{
				HUD_init_message_literal(HM_MULTI, "Can't save! Multiple players with same callsign!");
				return;
			}
		}
	}

	memset(&filename, '\0', PATH_MAX);
	memset(&desc, '\0', 24);
	slot = state_get_save_file(filename, desc, 0 );
	if (!slot)
		return;
	slot--;

	// Make a unique game id
	game_id = ((fix)timer_query());
	game_id ^= N_players<<4;
	for (i = 0; i < N_players; i++ )
	{
		fix call2i;
		memcpy(&call2i, Players[i].callsign, sizeof(fix));
		game_id ^= call2i;
	}
	if ( game_id == 0 )
		game_id = 1; // 0 is invalid

	multi_send_save_game( slot, game_id, desc );
	multi_do_frame();
	multi_save_game( slot,game_id, desc );
}

extern int state_get_game_id(char *);

void multi_initiate_restore_game()
{
	int i, j, slot;
	char filename[PATH_MAX];

	if ((Endlevel_sequence) || (Control_center_destroyed))
		return;

	if (!multi_i_am_master())
	{
		HUD_init_message_literal(HM_MULTI, "Only host is allowed to load a game!");
		return;
	}
	if (!multi_all_players_alive())
	{
		HUD_init_message_literal(HM_MULTI, "Can't load! All players must be alive!");
		return;
	}
	for (i = 0; i < N_players; i++)
	{
		for (j = 0; j < N_players; j++)
		{
			if (i != j && !d_stricmp(Players[i].callsign, Players[j].callsign))
			{
				HUD_init_message_literal(HM_MULTI, "Can't load! Multiple players with same callsign!");
				return;
			}
		}
	}
	slot = state_get_restore_file(filename);
	if (!slot)
		return;
	state_game_id = state_get_game_id(filename);
	if (!state_game_id)
		return;
	slot--;
	multi_send_restore_game(slot,state_game_id);
	multi_do_frame();
	multi_restore_game(slot,state_game_id);
}

void multi_save_game(ubyte slot, uint id, char *desc)
{
	char filename[PATH_MAX];

	if ((Endlevel_sequence) || (Control_center_destroyed))
		return;

	snprintf(filename, PATH_MAX, GameArg.SysUsePlayersDir? "Players/%s.mg%d" : "%s.mg%d", Players[Player_num].callsign, slot);
	HUD_init_message(HM_MULTI,  "Saving game #%d, '%s'", slot, desc);
	stop_time();
	state_game_id = id;
	state_save_all_sub(filename, desc );
}

void multi_restore_game(ubyte slot, uint id)
{
	char filename[PATH_MAX];
	int i;
	int thisid;

	if ((Endlevel_sequence) || (Control_center_destroyed))
		return;

	snprintf(filename, PATH_MAX, GameArg.SysUsePlayersDir? "Players/%s.mg%d" : "%s.mg%d", Players[Player_num].callsign, slot);
   
	for (i = 0; i < N_players; i++)
		multi_strip_robots(i);
	if (multi_i_am_master()) // put all players to wait-state again so we can sync up properly
		for (i = 0; i < MAX_PLAYERS; i++)
			if (Players[i].connected == CONNECT_PLAYING && i != Player_num) {
				Players[i].connected = CONNECT_WAITING;

				if (Current_obs_player == i) {
					reset_obs();
				}
			}
   
	thisid=state_get_game_id(filename);
	if (thisid!=id)
	{
		nm_messagebox(NULL, 1, TXT_OK, "A multi-save game was restored\nthat you are missing or does not\nmatch that of the others.\nYou must rejoin if you wish to\ncontinue.");
		return;
	}
  
	state_restore_all_sub( filename );
	multi_send_score(); // send my restored scores. I sent 0 when I loaded the level anyways...
}

void multi_do_msgsend_state(const ubyte *buf)
{
	multi_sending_message[(int)buf[1]] = (int)buf[2];
}

void multi_send_msgsend_state(int state)
{
	if(Game_mode & GM_OBSERVER) { return; }

	multibuf[0] = (char)MULTI_TYPING_STATE;
	multibuf[1] = Player_num;
	multibuf[2] = (char)state;
	
	multi_send_data(multibuf, 3, 2);
}

// Specific variables related to our game mode we want the clients to know about
void multi_send_gmode_update()
{
	if (!multi_i_am_master())
		return;
	if (!(Game_mode & GM_TEAM || Game_mode & GM_BOUNTY)) // expand if necessary
		return;
	multibuf[0] = (char)MULTI_GMODE_UPDATE;
	multibuf[1] = Netgame.team_vector;
	multibuf[2] = Bounty_target;
	
	multi_send_data(multibuf, 3, 0);
}

void multi_do_gmode_update(const ubyte *buf)
{
	if (multi_i_am_master())
		return;
	if (Game_mode & GM_TEAM)
	{
		if (buf[1] != Netgame.team_vector)
		{
			int t;
			Netgame.team_vector = buf[1];
			for (t=0;t<N_players;t++)
				if (Players[t].connected)
					multi_reset_object_texture (&Objects[Players[t].objnum]);
			reset_cockpit();
		}
	}
	if (Game_mode & GM_BOUNTY)
	{
		Bounty_target = buf[2]; // accept silently - message about change we SHOULD have gotten due to kill computation
	}
}

void
multi_process_data(const ubyte *buf, int len)
{
	// Take an entire message (that has already been checked for validity,
	// if necessary) and act on it.  

	int type;
	len = len;

	type = buf[0];
	
	if (type >= sizeof(message_length) / sizeof(message_length[0]))
	{
		Int3();
		return;
	}

	switch(type)
	{
		case MULTI_POSITION:
			if (!Endlevel_sequence) multi_do_position(buf); break;
		case MULTI_REAPPEAR:
			if (!Endlevel_sequence) multi_do_reappear(buf); break;
		case MULTI_FIRE:
			if (!Endlevel_sequence) multi_do_fire(buf); break;
		case MULTI_KILL:
			multi_do_kill(buf); break;
		case MULTI_REMOVE_OBJECT:
			if (!Endlevel_sequence) multi_do_remobj(buf); break;
		case MULTI_PLAYER_DROP:
		case MULTI_PLAYER_EXPLODE:
			if (!Endlevel_sequence) multi_do_player_explode(buf); break;
		case MULTI_MESSAGE:
			if (!Endlevel_sequence) multi_do_message(buf); break;
		case MULTI_QUIT:
			if (!Endlevel_sequence) multi_do_quit(buf); break;
		case MULTI_BEGIN_SYNC:
		   break;
		case MULTI_CONTROLCEN:
			if (!Endlevel_sequence) multi_do_controlcen_destroy(buf); break;
		case MULTI_ENDLEVEL_START:
			if (!Endlevel_sequence) multi_do_escape(buf); break;
		case MULTI_END_SYNC:
			break;
		case MULTI_CLOAK:
			if (!Endlevel_sequence) multi_do_cloak(buf); break;
		case MULTI_DECLOAK:
			if (!Endlevel_sequence) multi_do_decloak(buf); break;
		case MULTI_DOOR_OPEN:
			if (!Endlevel_sequence) multi_do_door_open(buf); break;
		case MULTI_CREATE_EXPLOSION:
			if (!Endlevel_sequence) multi_do_create_explosion(buf); break;
		case MULTI_CONTROLCEN_FIRE:
			if (!Endlevel_sequence) multi_do_controlcen_fire(buf); break;
		case MULTI_CREATE_POWERUP:
			if (!Endlevel_sequence) multi_do_create_powerup(buf); break;
		case MULTI_PLAY_SOUND:
			if (!Endlevel_sequence) multi_do_play_sound(buf); break;
		case MULTI_ROBOT_CLAIM:
			if (!Endlevel_sequence) multi_do_claim_robot(buf); break;
		case MULTI_ROBOT_POSITION:
			if (!Endlevel_sequence) multi_do_robot_position(buf); break;
		case MULTI_ROBOT_EXPLODE:
			if (!Endlevel_sequence) multi_do_robot_explode(buf); break;
		case MULTI_ROBOT_RELEASE:
			if (!Endlevel_sequence) multi_do_release_robot(buf); break;
		case MULTI_ROBOT_FIRE:
			if (!Endlevel_sequence) multi_do_robot_fire(buf); break;
		case MULTI_RESPAWN_ROBOT:
			if(! Endlevel_sequence) multi_do_respawn_robot(buf); break;
		case MULTI_SCORE:
			if (!Endlevel_sequence) multi_do_score(buf); break;
		case MULTI_CREATE_ROBOT:
			if (!Endlevel_sequence) multi_do_create_robot(buf); break;
		case MULTI_TRIGGER:
			if (!Endlevel_sequence) multi_do_trigger(buf); break;
		case MULTI_BOSS_ACTIONS:
			if (!Endlevel_sequence) multi_do_boss_actions(buf); break;
		case MULTI_CREATE_ROBOT_POWERUPS:
			if (!Endlevel_sequence) multi_do_create_robot_powerups(buf); break;
		case MULTI_HOSTAGE_DOOR:
			if (!Endlevel_sequence) multi_do_hostage_door_status(buf); break;
		case MULTI_SAVE_GAME:
			if (!Endlevel_sequence) multi_do_save_game(buf); break;
		case MULTI_RESTORE_GAME:
			if (!Endlevel_sequence) multi_do_restore_game(buf); break;
		case MULTI_POWCAP_UPDATE:
			if (!Endlevel_sequence) multi_do_powcap_update(buf); break;
		case MULTI_HEARTBEAT:
			if (!Endlevel_sequence) multi_do_heartbeat (buf); break;
		case MULTI_KILLGOALS:
			if (!Endlevel_sequence) multi_do_kill_goal_counts (buf); break;
		case MULTI_DO_BOUNTY:
			if( !Endlevel_sequence ) multi_do_bounty( buf ); break;
		case MULTI_TYPING_STATE:
			multi_do_msgsend_state( buf ); break;
		case MULTI_GMODE_UPDATE:
			multi_do_gmode_update( buf ); break;
		case MULTI_KILL_HOST:
			multi_do_kill(buf); break;
		case MULTI_KILL_CLIENT:
			multi_do_kill(buf); break;
		case MULTI_OBS_UPDATE:
			multi_do_obs_update(buf); break;
		case MULTI_DAMAGE:
			multi_do_damage(buf); break;
		case MULTI_REPAIR:
			multi_do_repair(buf); break;
		default:
			Int3();
	}
}

// Following functions convert object to object_rw and back.
// turn object to object_rw for sending
void multi_object_to_object_rw(object *obj, object_rw *obj_rw)
{
	obj_rw->signature     = obj->signature;
	obj_rw->type          = obj->type;
	obj_rw->id            = obj->id;
	obj_rw->next          = obj->next;
	obj_rw->prev          = obj->prev;
	obj_rw->control_type  = obj->control_type;
	obj_rw->movement_type = obj->movement_type;
	obj_rw->render_type   = obj->render_type;
	obj_rw->flags         = obj->flags;
	obj_rw->segnum        = obj->segnum;
	obj_rw->attached_obj  = obj->attached_obj;
	obj_rw->pos.x         = obj->pos.x;
	obj_rw->pos.y         = obj->pos.y;
	obj_rw->pos.z         = obj->pos.z;
	obj_rw->orient.rvec.x = obj->orient.rvec.x;
	obj_rw->orient.rvec.y = obj->orient.rvec.y;
	obj_rw->orient.rvec.z = obj->orient.rvec.z;
	obj_rw->orient.fvec.x = obj->orient.fvec.x;
	obj_rw->orient.fvec.y = obj->orient.fvec.y;
	obj_rw->orient.fvec.z = obj->orient.fvec.z;
	obj_rw->orient.uvec.x = obj->orient.uvec.x;
	obj_rw->orient.uvec.y = obj->orient.uvec.y;
	obj_rw->orient.uvec.z = obj->orient.uvec.z;
	obj_rw->size          = obj->size;
	obj_rw->shields       = obj->shields;
	obj_rw->last_pos.x    = obj->last_pos.x;
	obj_rw->last_pos.y    = obj->last_pos.y;
	obj_rw->last_pos.z    = obj->last_pos.z;
	obj_rw->contains_type = obj->contains_type;
	obj_rw->contains_id   = obj->contains_id;
	obj_rw->contains_count= obj->contains_count;
	obj_rw->matcen_creator= obj->matcen_creator;
	obj_rw->lifeleft      = obj->lifeleft;
	
	switch (obj_rw->movement_type)
	{
		case MT_PHYSICS:
			obj_rw->mtype.phys_info.velocity.x  = obj->mtype.phys_info.velocity.x;
			obj_rw->mtype.phys_info.velocity.y  = obj->mtype.phys_info.velocity.y;
			obj_rw->mtype.phys_info.velocity.z  = obj->mtype.phys_info.velocity.z;
			obj_rw->mtype.phys_info.thrust.x    = obj->mtype.phys_info.thrust.x;
			obj_rw->mtype.phys_info.thrust.y    = obj->mtype.phys_info.thrust.y;
			obj_rw->mtype.phys_info.thrust.z    = obj->mtype.phys_info.thrust.z;
			obj_rw->mtype.phys_info.mass        = obj->mtype.phys_info.mass;
			obj_rw->mtype.phys_info.drag        = obj->mtype.phys_info.drag;
			obj_rw->mtype.phys_info.brakes      = obj->mtype.phys_info.brakes;
			obj_rw->mtype.phys_info.rotvel.x    = obj->mtype.phys_info.rotvel.x;
			obj_rw->mtype.phys_info.rotvel.y    = obj->mtype.phys_info.rotvel.y;
			obj_rw->mtype.phys_info.rotvel.z    = obj->mtype.phys_info.rotvel.z;
			obj_rw->mtype.phys_info.rotthrust.x = obj->mtype.phys_info.rotthrust.x;
			obj_rw->mtype.phys_info.rotthrust.y = obj->mtype.phys_info.rotthrust.y;
			obj_rw->mtype.phys_info.rotthrust.z = obj->mtype.phys_info.rotthrust.z;
			obj_rw->mtype.phys_info.turnroll    = obj->mtype.phys_info.turnroll;
			obj_rw->mtype.phys_info.flags       = obj->mtype.phys_info.flags;
			break;
			
		case MT_SPINNING:
			obj_rw->mtype.spin_rate.x = obj->mtype.spin_rate.x;
			obj_rw->mtype.spin_rate.y = obj->mtype.spin_rate.y;
			obj_rw->mtype.spin_rate.z = obj->mtype.spin_rate.z;
			break;
	}
	
	switch (obj_rw->control_type)
	{
		case CT_WEAPON:
			obj_rw->ctype.laser_info.parent_type      = obj->ctype.laser_info.parent_type;
			obj_rw->ctype.laser_info.parent_num       = obj->ctype.laser_info.parent_num;
			obj_rw->ctype.laser_info.parent_signature = obj->ctype.laser_info.parent_signature;
			if (obj->ctype.laser_info.creation_time - GameTime64 < F1_0*(-18000))
				obj_rw->ctype.laser_info.creation_time = F1_0*(-18000);
			else
				obj_rw->ctype.laser_info.creation_time = obj->ctype.laser_info.creation_time - GameTime64;
			obj_rw->ctype.laser_info.last_hitobj      = obj->ctype.laser_info.last_hitobj;
			obj_rw->ctype.laser_info.track_goal       = obj->ctype.laser_info.track_goal;
			obj_rw->ctype.laser_info.multiplier       = obj->ctype.laser_info.multiplier;
			break;
			
		case CT_EXPLOSION:
			obj_rw->ctype.expl_info.spawn_time    = obj->ctype.expl_info.spawn_time;
			obj_rw->ctype.expl_info.delete_time   = obj->ctype.expl_info.delete_time;
			obj_rw->ctype.expl_info.delete_objnum = obj->ctype.expl_info.delete_objnum;
			obj_rw->ctype.expl_info.attach_parent = obj->ctype.expl_info.attach_parent;
			obj_rw->ctype.expl_info.prev_attach   = obj->ctype.expl_info.prev_attach;
			obj_rw->ctype.expl_info.next_attach   = obj->ctype.expl_info.next_attach;
			break;
			
		case CT_AI:
		{
			int i;
			obj_rw->ctype.ai_info.behavior               = obj->ctype.ai_info.behavior; 
			for (i = 0; i < MAX_AI_FLAGS; i++)
				obj_rw->ctype.ai_info.flags[i]       = obj->ctype.ai_info.flags[i]; 
			obj_rw->ctype.ai_info.hide_segment           = obj->ctype.ai_info.hide_segment;
			obj_rw->ctype.ai_info.hide_index             = obj->ctype.ai_info.hide_index;
			obj_rw->ctype.ai_info.path_length            = obj->ctype.ai_info.path_length;
			obj_rw->ctype.ai_info.cur_path_index         = obj->ctype.ai_info.cur_path_index;
			obj_rw->ctype.ai_info.follow_path_start_seg  = obj->ctype.ai_info.follow_path_start_seg;
			obj_rw->ctype.ai_info.follow_path_end_seg    = obj->ctype.ai_info.follow_path_end_seg;
			obj_rw->ctype.ai_info.danger_laser_signature = obj->ctype.ai_info.danger_laser_signature;
			obj_rw->ctype.ai_info.danger_laser_num       = obj->ctype.ai_info.danger_laser_num;
			break;
		}
			
		case CT_LIGHT:
			obj_rw->ctype.light_info.intensity = obj->ctype.light_info.intensity;
			break;
			
		case CT_POWERUP:
			obj_rw->ctype.powerup_info.count         = obj->ctype.powerup_info.count;
			break;
	}
	
	switch (obj_rw->render_type)
	{
		case RT_MORPH:
		case RT_POLYOBJ:
		case RT_NONE: // HACK below
		{
			int i;
			if (obj->render_type == RT_NONE && obj->type != OBJ_GHOST) // HACK: when a player is dead or not connected yet, clients still expect to get polyobj data - even if render_type == RT_NONE at this time.
				break;
			obj_rw->rtype.pobj_info.model_num                = obj->rtype.pobj_info.model_num;
			for (i=0;i<MAX_SUBMODELS;i++)
			{
				obj_rw->rtype.pobj_info.anim_angles[i].p = obj->rtype.pobj_info.anim_angles[i].p;
				obj_rw->rtype.pobj_info.anim_angles[i].b = obj->rtype.pobj_info.anim_angles[i].b;
				obj_rw->rtype.pobj_info.anim_angles[i].h = obj->rtype.pobj_info.anim_angles[i].h;
			}
			obj_rw->rtype.pobj_info.subobj_flags             = obj->rtype.pobj_info.subobj_flags;
			obj_rw->rtype.pobj_info.tmap_override            = obj->rtype.pobj_info.tmap_override;
			obj_rw->rtype.pobj_info.alt_textures             = obj->rtype.pobj_info.alt_textures;
			break;
		}
			
		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:
			obj_rw->rtype.vclip_info.vclip_num = obj->rtype.vclip_info.vclip_num;
			obj_rw->rtype.vclip_info.frametime = obj->rtype.vclip_info.frametime;
			obj_rw->rtype.vclip_info.framenum  = obj->rtype.vclip_info.framenum;
			break;
			
		case RT_LASER:
			break;
			
	}
}

// turn object_rw to object after receiving
void multi_object_rw_to_object(object_rw *obj_rw, object *obj)
{
	obj->signature     = obj_rw->signature;
	obj->type          = obj_rw->type;
	obj->id            = obj_rw->id;
	obj->next          = obj_rw->next;
	obj->prev          = obj_rw->prev;
	obj->control_type  = obj_rw->control_type;
	obj->movement_type = obj_rw->movement_type;
	obj->render_type   = obj_rw->render_type;
	obj->flags         = obj_rw->flags;
	obj->segnum        = obj_rw->segnum;
	obj->attached_obj  = obj_rw->attached_obj;
	obj->pos.x         = obj_rw->pos.x;
	obj->pos.y         = obj_rw->pos.y;
	obj->pos.z         = obj_rw->pos.z;
	obj->orient.rvec.x = obj_rw->orient.rvec.x;
	obj->orient.rvec.y = obj_rw->orient.rvec.y;
	obj->orient.rvec.z = obj_rw->orient.rvec.z;
	obj->orient.fvec.x = obj_rw->orient.fvec.x;
	obj->orient.fvec.y = obj_rw->orient.fvec.y;
	obj->orient.fvec.z = obj_rw->orient.fvec.z;
	obj->orient.uvec.x = obj_rw->orient.uvec.x;
	obj->orient.uvec.y = obj_rw->orient.uvec.y;
	obj->orient.uvec.z = obj_rw->orient.uvec.z;
	obj->size          = obj_rw->size;
	obj->shields       = obj_rw->shields;
	obj->last_pos.x    = obj_rw->last_pos.x;
	obj->last_pos.y    = obj_rw->last_pos.y;
	obj->last_pos.z    = obj_rw->last_pos.z;
	obj->contains_type = obj_rw->contains_type;
	obj->contains_id   = obj_rw->contains_id;
	obj->contains_count= obj_rw->contains_count;
	obj->matcen_creator= obj_rw->matcen_creator;
	obj->lifeleft      = obj_rw->lifeleft;
	
	switch (obj->movement_type)
	{
		case MT_PHYSICS:
			obj->mtype.phys_info.velocity.x  = obj_rw->mtype.phys_info.velocity.x;
			obj->mtype.phys_info.velocity.y  = obj_rw->mtype.phys_info.velocity.y;
			obj->mtype.phys_info.velocity.z  = obj_rw->mtype.phys_info.velocity.z;
			obj->mtype.phys_info.thrust.x    = obj_rw->mtype.phys_info.thrust.x;
			obj->mtype.phys_info.thrust.y    = obj_rw->mtype.phys_info.thrust.y;
			obj->mtype.phys_info.thrust.z    = obj_rw->mtype.phys_info.thrust.z;
			obj->mtype.phys_info.mass        = obj_rw->mtype.phys_info.mass;
			obj->mtype.phys_info.drag        = obj_rw->mtype.phys_info.drag;
			obj->mtype.phys_info.brakes      = obj_rw->mtype.phys_info.brakes;
			obj->mtype.phys_info.rotvel.x    = obj_rw->mtype.phys_info.rotvel.x;
			obj->mtype.phys_info.rotvel.y    = obj_rw->mtype.phys_info.rotvel.y;
			obj->mtype.phys_info.rotvel.z    = obj_rw->mtype.phys_info.rotvel.z;
			obj->mtype.phys_info.rotthrust.x = obj_rw->mtype.phys_info.rotthrust.x;
			obj->mtype.phys_info.rotthrust.y = obj_rw->mtype.phys_info.rotthrust.y;
			obj->mtype.phys_info.rotthrust.z = obj_rw->mtype.phys_info.rotthrust.z;
			obj->mtype.phys_info.turnroll    = obj_rw->mtype.phys_info.turnroll;
			obj->mtype.phys_info.flags       = obj_rw->mtype.phys_info.flags;
			break;
			
		case MT_SPINNING:
			obj->mtype.spin_rate.x = obj_rw->mtype.spin_rate.x;
			obj->mtype.spin_rate.y = obj_rw->mtype.spin_rate.y;
			obj->mtype.spin_rate.z = obj_rw->mtype.spin_rate.z;
			break;
	}
	
	switch (obj->control_type)
	{
		case CT_WEAPON:
			obj->ctype.laser_info.parent_type      = obj_rw->ctype.laser_info.parent_type;
			obj->ctype.laser_info.parent_num       = obj_rw->ctype.laser_info.parent_num;
			obj->ctype.laser_info.parent_signature = obj_rw->ctype.laser_info.parent_signature;
			obj->ctype.laser_info.creation_time    = obj_rw->ctype.laser_info.creation_time;
			obj->ctype.laser_info.last_hitobj      = obj_rw->ctype.laser_info.last_hitobj;
			obj->ctype.laser_info.track_goal       = obj_rw->ctype.laser_info.track_goal;
			obj->ctype.laser_info.multiplier       = obj_rw->ctype.laser_info.multiplier;
			break;
			
		case CT_EXPLOSION:
			obj->ctype.expl_info.spawn_time    = obj_rw->ctype.expl_info.spawn_time;
			obj->ctype.expl_info.delete_time   = obj_rw->ctype.expl_info.delete_time;
			obj->ctype.expl_info.delete_objnum = obj_rw->ctype.expl_info.delete_objnum;
			obj->ctype.expl_info.attach_parent = obj_rw->ctype.expl_info.attach_parent;
			obj->ctype.expl_info.prev_attach   = obj_rw->ctype.expl_info.prev_attach;
			obj->ctype.expl_info.next_attach   = obj_rw->ctype.expl_info.next_attach;
			break;
			
		case CT_AI:
		{
			int i;
			obj->ctype.ai_info.behavior               = obj_rw->ctype.ai_info.behavior; 
			for (i = 0; i < MAX_AI_FLAGS; i++)
				obj->ctype.ai_info.flags[i]       = obj_rw->ctype.ai_info.flags[i]; 
			obj->ctype.ai_info.hide_segment           = obj_rw->ctype.ai_info.hide_segment;
			obj->ctype.ai_info.hide_index             = obj_rw->ctype.ai_info.hide_index;
			obj->ctype.ai_info.path_length            = obj_rw->ctype.ai_info.path_length;
			obj->ctype.ai_info.cur_path_index         = obj_rw->ctype.ai_info.cur_path_index;
			obj->ctype.ai_info.follow_path_start_seg  = obj_rw->ctype.ai_info.follow_path_start_seg;
			obj->ctype.ai_info.follow_path_end_seg    = obj_rw->ctype.ai_info.follow_path_end_seg;
			obj->ctype.ai_info.danger_laser_signature = obj_rw->ctype.ai_info.danger_laser_signature;
			obj->ctype.ai_info.danger_laser_num       = obj_rw->ctype.ai_info.danger_laser_num;
			break;
		}
			
		case CT_LIGHT:
			obj->ctype.light_info.intensity = obj_rw->ctype.light_info.intensity;
			break;
			
		case CT_POWERUP:
			obj->ctype.powerup_info.count         = obj_rw->ctype.powerup_info.count;
			break;
		case CT_CNTRLCEN:
		{
			// gun points of reactor now part of the object but of course not saved in object_rw. Let's just recompute them.
			int i = 0;
			reactor *reactor = get_reactor_definition(obj->id);
			for (i=0; i<reactor->n_guns; i++)
				calc_controlcen_gun_point(reactor, obj, i);
			break;
		}
	}
	
	switch (obj->render_type)
	{
		case RT_MORPH:
		case RT_POLYOBJ:
		case RT_NONE: // HACK below
		{
			int i;
			if (obj->render_type == RT_NONE && obj->type != OBJ_GHOST) // HACK: when a player is dead or not connected yet, clients still expect to get polyobj data - even if render_type == RT_NONE at this time.
				break;
			obj->rtype.pobj_info.model_num                = obj_rw->rtype.pobj_info.model_num;
			for (i=0;i<MAX_SUBMODELS;i++)
			{
				obj->rtype.pobj_info.anim_angles[i].p = obj_rw->rtype.pobj_info.anim_angles[i].p;
				obj->rtype.pobj_info.anim_angles[i].b = obj_rw->rtype.pobj_info.anim_angles[i].b;
				obj->rtype.pobj_info.anim_angles[i].h = obj_rw->rtype.pobj_info.anim_angles[i].h;
			}
			obj->rtype.pobj_info.subobj_flags             = obj_rw->rtype.pobj_info.subobj_flags;
			obj->rtype.pobj_info.tmap_override            = obj_rw->rtype.pobj_info.tmap_override;
			obj->rtype.pobj_info.alt_textures             = obj_rw->rtype.pobj_info.alt_textures;
			break;
		}
			
		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:
			obj->rtype.vclip_info.vclip_num = obj_rw->rtype.vclip_info.vclip_num;
			obj->rtype.vclip_info.frametime = obj_rw->rtype.vclip_info.frametime;
			obj->rtype.vclip_info.framenum  = obj_rw->rtype.vclip_info.framenum;
			break;
			
		case RT_LASER:
			break;
			
	}
}
