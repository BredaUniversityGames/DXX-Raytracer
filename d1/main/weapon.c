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
 * Functions for weapons...
 *
 */


#include "game.h"
#include "laser.h"
#include "weapon.h"
#include "player.h"
#include "gauges.h"
#include "dxxerror.h"
#include "sounds.h"
#include "text.h"
#include "powerup.h"
#include "newdemo.h"
#include "multi.h"
#include "newmenu.h"
#include "playsave.h"
#include "fireball.h"
#include "logger.h"

//	Convert primary weapons to indices in Weapon_info array.
const ubyte Primary_weapon_to_weapon_info[MAX_PRIMARY_WEAPONS] = {0, VULCAN_ID, 12, PLASMA_ID, FUSION_ID};
const ubyte Secondary_weapon_to_weapon_info[MAX_SECONDARY_WEAPONS] = {CONCUSSION_ID, HOMING_ID, PROXIMITY_ID, SMART_ID, MEGA_ID};
//for each primary weapon, what kind of powerup gives weapon
const ubyte Primary_weapon_to_powerup[MAX_PRIMARY_WEAPONS] = {POW_LASER,POW_VULCAN_WEAPON,POW_SPREADFIRE_WEAPON,POW_PLASMA_WEAPON,POW_FUSION_WEAPON};
//for each Secondary weapon, what kind of powerup gives weapon
const ubyte Secondary_weapon_to_powerup[MAX_SECONDARY_WEAPONS] = {POW_MISSILE_1,POW_HOMING_AMMO_1,POW_PROXIMITY_WEAPON,POW_SMARTBOMB_WEAPON,POW_MEGA_WEAPON};
const int Primary_ammo_max[MAX_PRIMARY_WEAPONS] = {0, VULCAN_AMMO_MAX, 0, 0, 0};
const ubyte Secondary_ammo_max[MAX_SECONDARY_WEAPONS] = {20, 10, 10, 5, 5};
weapon_info Weapon_info[MAX_WEAPON_TYPES];
int	N_weapon_types=0;
sbyte   Primary_weapon, Secondary_weapon;
int POrderList (int num);
int SOrderList (int num);
static const ubyte DefaultPrimaryOrder[] = { 4, 3, 2, 1, 0, 255, 16 };
static const ubyte DefaultSecondaryOrder[] = { 4, 3, 1, 0, 255, 2 };
extern ubyte MenuReordering;

int player_has_weapon_lasers_not_quads(int weapon_num, int secondary_flag) {
	if(weapon_num == 16) {
		if(Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)
			return player_has_weapon(LASER_INDEX, 0); 
		else
			return 0;  	
	} else if(weapon_num == LASER_INDEX && secondary_flag == 0) {
		if(! (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
			return player_has_weapon(LASER_INDEX, 0); 
		else
			return 0; 
	} else {
		return player_has_weapon(weapon_num, secondary_flag); 
	}
}

//	------------------------------------------------------------------------------------
//	Return:
// Bits set:
//		HAS_WEAPON_FLAG
//		HAS_ENERGY_FLAG
//		HAS_AMMO_FLAG
// See weapon.h for bit values
int player_has_weapon(int weapon_num, int secondary_flag)
{
	int	return_value = 0;
	int	weapon_index;

	//	Hack! If energy goes negative, you can't fire a weapon that doesn't require energy.
	//	But energy should not go negative (but it does), so find out why it does!
	if (Players[Player_num].energy < 0)
		Players[Player_num].energy = 0;

	if (!secondary_flag) {
		if(weapon_num >= MAX_PRIMARY_WEAPONS)
		{
			switch(weapon_num-MAX_PRIMARY_WEAPONS)
			{
				case 0 : if((Players[Player_num].laser_level != 0)||(Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
					return 0;
					break;
				case 1 : if((Players[Player_num].laser_level != 1)||(Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
					return 0;
					break;
				case 2 : if((Players[Player_num].laser_level != 2)||(Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
					return 0;
					break;
				case 3 : if((Players[Player_num].laser_level != 3)||(Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
					return 0;
					break;
				case 4 : if((Players[Player_num].laser_level != 0)||!(Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
					return 0;
					break;
				case 5 : if((Players[Player_num].laser_level != 1)||!(Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
					return 0;
					break;
				case 6 : if((Players[Player_num].laser_level != 2)||!(Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
					return 0;
					break;
				case 7 : if((Players[Player_num].laser_level != 3)||!(Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
					return 0;
					break;
			}
			weapon_num = 0;
		}

		weapon_index = Primary_weapon_to_weapon_info[weapon_num];

		if (Players[Player_num].primary_weapon_flags & (1 << weapon_num))
			return_value |= HAS_WEAPON_FLAG;

		if (Weapon_info[weapon_index].ammo_usage <= Players[Player_num].primary_ammo[weapon_num])
			return_value |= HAS_AMMO_FLAG;

		//added on 1/21/99 by Victor Rachels... yet another hack
		//fusion has 0 energy usage, HAS_ENERGY_FLAG was always true
		if(weapon_num==FUSION_INDEX)
		{
			if(Players[Player_num].energy >= F1_0*2)
				return_value |= HAS_ENERGY_FLAG;
		}
		else
			//end this section addition - VR
			if (Weapon_info[weapon_index].energy_usage <= Players[Player_num].energy)
				return_value |= HAS_ENERGY_FLAG;
	} else {
		weapon_index = Secondary_weapon_to_weapon_info[weapon_num];

		if (Players[Player_num].secondary_weapon_flags & (1 << weapon_num))
			return_value |= HAS_WEAPON_FLAG;

		if (Weapon_info[weapon_index].ammo_usage <= Players[Player_num].secondary_ammo[weapon_num])
			return_value |= HAS_AMMO_FLAG;

		if (Weapon_info[weapon_index].energy_usage <= Players[Player_num].energy)
			return_value |= HAS_ENERGY_FLAG;
	}
	return return_value;
}

void InitWeaponOrdering ()
 {
  // short routine to setup default weapon priorities for new pilots

  int i;

  for (i=0;i<MAX_PRIMARY_WEAPONS+2;i++)
	PlayerCfg.PrimaryOrder[i]=DefaultPrimaryOrder[i];
  for (i=0;i<MAX_SECONDARY_WEAPONS+1;i++)
	PlayerCfg.SecondaryOrder[i]=DefaultSecondaryOrder[i];
 }

void CyclePrimary ()
{
	int cur_order_slot = POrderList(Primary_weapon);
	if(Primary_weapon == LASER_INDEX && Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS) {
		cur_order_slot = POrderList(16); 
	}

	int desired_weapon = Primary_weapon, loop=0;
	const int autoselect_order_slot = POrderList(255);
	const int use_restricted_autoselect = (cur_order_slot < autoselect_order_slot) && (1 < autoselect_order_slot) && (PlayerCfg.CycleAutoselectOnly);
	
	while (loop<(MAX_PRIMARY_WEAPONS+2))
	{
		loop++;
		cur_order_slot++; // next slot
		if (cur_order_slot >= MAX_PRIMARY_WEAPONS+2) // loop if necessary
			cur_order_slot = 0;
		if (cur_order_slot == autoselect_order_slot) // what to to with non-autoselect weapons?
		{
			if (use_restricted_autoselect)
			{
				cur_order_slot = 0; // loop over or ...
			}
			else
			{
				continue; // continue?
			}
		}
		desired_weapon = PlayerCfg.PrimaryOrder[cur_order_slot]; // now that is the weapon next to our current one
		// select the weapon if we have it
		if (player_has_weapon_lasers_not_quads(desired_weapon, 0) == HAS_ALL)
		{
			if(desired_weapon == 16) { // Quads
				desired_weapon = LASER_INDEX; 
			}
			select_weapon(desired_weapon, 0, 1, 1);
			return;
		}
	}
}

void CycleSecondary ()
{
	int cur_order_slot = SOrderList(Secondary_weapon), desired_weapon = Secondary_weapon, loop=0;
	const int autoselect_order_slot = SOrderList(255);
	const int use_restricted_autoselect = (cur_order_slot < autoselect_order_slot) && (1 < autoselect_order_slot) && (PlayerCfg.CycleAutoselectOnly);
	
	while (loop<(MAX_SECONDARY_WEAPONS+1))
	{
		loop++;
		cur_order_slot++; // next slot
		if (cur_order_slot >= MAX_SECONDARY_WEAPONS+1) // loop if necessary
			cur_order_slot = 0;
		if (cur_order_slot == autoselect_order_slot) // what to to with non-autoselect weapons?
		{
			if (use_restricted_autoselect)
			{
				cur_order_slot = 0; // loop over or ...
			}
			else
			{
				continue; // continue?
			}
		}
		desired_weapon = PlayerCfg.SecondaryOrder[cur_order_slot]; // now that is the weapon next to our current one
		// select the weapon if we have it
		if (player_has_weapon(desired_weapon, 1) == HAS_ALL)
		{
			select_weapon(desired_weapon, 1, 1, 1);
			return;
		}
	}
}

//	------------------------------------------------------------------------------------
//if message flag set, print message saying selected
void select_weapon(int weapon_num, int secondary_flag, int print_message, int wait_for_rearm)
{
	if(! secondary_flag && weapon_num == 16) {
		weapon_num = LASER_INDEX; 

		if(Primary_weapon == LASER_INDEX)
			return;
	}

	char	*weapon_name;

#ifndef SHAREWARE
	if (Newdemo_state==ND_STATE_RECORDING )
		newdemo_record_player_weapon(secondary_flag, weapon_num);
#endif

	if (!secondary_flag) {
		if (Primary_weapon != weapon_num) {
#ifndef FUSION_KEEPS_CHARGE
			//added 8/6/98 by Victor Rachels to fix fusion charge bug
                        Fusion_charge=0;
			//end edit - Victor Rachels
#endif
			if (wait_for_rearm) digi_play_sample_once( SOUND_GOOD_SELECTION_PRIMARY, F1_0 );

			if(weapon_num == VULCAN_INDEX && PlayerCfg.VulcanAmmoWarnings && Players[Player_num].primary_ammo[VULCAN_INDEX] != 0) {
				if(Players[Player_num].primary_ammo[VULCAN_INDEX] < 500/12) {
					HUD_init_message_literal(HM_MULTI, "Vulcan ammo critical!"); 
					digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
				} else if(Players[Player_num].primary_ammo[VULCAN_INDEX] < 1000/12) {
					HUD_init_message_literal(HM_MULTI, "Vulcan ammo low."); 
					digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
				}
			}
#ifdef NETWORK
			if (Game_mode & GM_MULTI)	{
				if (wait_for_rearm) multi_send_play_sound(SOUND_GOOD_SELECTION_PRIMARY, F1_0);
			}
#endif
			if (wait_for_rearm)
				Next_laser_fire_time = GameTime64 + REARM_TIME;
			else
				Next_laser_fire_time = 0;
			Global_laser_firing_count = 0;
		} else 	{
			if (wait_for_rearm) digi_play_sample( SOUND_ALREADY_SELECTED, F1_0 );
		}
		Primary_weapon = weapon_num;
		weapon_name = PRIMARY_WEAPON_NAMES(weapon_num);
	} else {

		if (Secondary_weapon != weapon_num) {
			if (wait_for_rearm) digi_play_sample_once( SOUND_GOOD_SELECTION_SECONDARY, F1_0 );
#ifdef NETWORK
			if (Game_mode & GM_MULTI)	{
				if (wait_for_rearm) multi_send_play_sound(SOUND_GOOD_SELECTION_PRIMARY, F1_0);
			}
#endif
			if (wait_for_rearm)
				Next_missile_fire_time = GameTime64 + REARM_TIME;
			else
				Next_missile_fire_time = 0;
			Global_missile_firing_count = 0;
		} else	{
			if (wait_for_rearm) digi_play_sample_once( SOUND_ALREADY_SELECTED, F1_0 );
		}
		Secondary_weapon = weapon_num;
		weapon_name = SECONDARY_WEAPON_NAMES(weapon_num);
	}

	if (print_message)
		HUD_init_message(HM_DEFAULT, "%s %s", weapon_name, TXT_SELECTED);
}

//	------------------------------------------------------------------------------------
//	Select a weapon, primary or secondary.
void do_weapon_select(int weapon_num, int secondary_flag)
{
        //added on 10/9/98 by Victor Rachels to add laser cycle
	int	oweapon = weapon_num;
        //end this section addition - Victor Rachels
	int	weapon_status = player_has_weapon(weapon_num, secondary_flag);
	char	*weapon_name;


	// do special hud msg. for picking registered weapon in shareware version.
	if (PCSharePig)
		if (weapon_num >= NUM_SHAREWARE_WEAPONS) {
			weapon_name = secondary_flag?SECONDARY_WEAPON_NAMES(weapon_num):PRIMARY_WEAPON_NAMES(weapon_num);
			HUD_init_message(HM_DEFAULT, "%s %s!", weapon_name,TXT_NOT_IN_SHAREWARE);
			digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
			return;
		}

	if (!secondary_flag) {

		if (weapon_num >= MAX_PRIMARY_WEAPONS)
			weapon_num = 0;

		weapon_name = PRIMARY_WEAPON_NAMES(weapon_num);
		if ((weapon_status & HAS_WEAPON_FLAG) == 0) {
			HUD_init_message(HM_DEFAULT, "%s %s!", TXT_DONT_HAVE, weapon_name);
			digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
			return;
		}
		else if ((weapon_status & HAS_AMMO_FLAG) == 0) {
			HUD_init_message(HM_DEFAULT, "%s %s!", TXT_DONT_HAVE_AMMO, weapon_name);
			digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
			return;
		}
	}
	else {
		weapon_name = SECONDARY_WEAPON_NAMES(weapon_num);
		if (weapon_status != HAS_ALL) {
			HUD_init_message(HM_DEFAULT, "%s %s%s",TXT_HAVE_NO, weapon_name, TXT_SX);
			digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
			return;
		}
	}

	weapon_num=oweapon;
	select_weapon(weapon_num, secondary_flag, 1, 1);
}

//	----------------------------------------------------------------------------------------
//	Automatically select best available weapon if unable to fire current weapon.
// Weapon type: 0==primary, 1==secondary
void auto_select_weapon(int weapon_type)
{
	// Can you fire your current weapon? 
	if(weapon_type == 0 && player_has_weapon(  Primary_weapon, 0) == HAS_ALL) { return; }
	if(weapon_type == 1 && player_has_weapon(Secondary_weapon, 1) == HAS_ALL) { return; }

	// Ok, no.  Let's find you a new one. 
	int selected_weapon = 0; 
	int max_weapons = weapon_type == 0 ? MAX_PRIMARY_WEAPONS + 2 : MAX_SECONDARY_WEAPONS + 1; 
	for(int i = 0; i < max_weapons; i++) {
		int next_weapon = weapon_type == 0 ? PlayerCfg.PrimaryOrder[i] : PlayerCfg.SecondaryOrder[i];

		if(next_weapon == 255) { continue; } // Breakpoint in list
		if(next_weapon == PROXIMITY_INDEX && weapon_type == 1) { continue; } // Don't autoselect proxies.  Ever.
		if(player_has_weapon_lasers_not_quads(next_weapon, weapon_type) != HAS_ALL) { continue; } // Missing weapon or ammo

		select_weapon(next_weapon, weapon_type, 0, 1); 
		selected_weapon = 1; 
		break; 
	}

	// Nothing will shoot?  Bummer.
	if(! selected_weapon ) {
		if(weapon_type == 0) {
			select_weapon(0, 0, 0, 1); // Apparently you have to select lasers because if you leave it on vulcan, you can keep shooting!
			HUD_init_message_literal(HM_DEFAULT, "No primary weapons available!");
		} else {
			HUD_init_message_literal(HM_DEFAULT, "No secondary weapons available!");
		}
	}

	// Wow.  That's an awful lot of work to do the wrong thing. --CED
	/*
	int	r;
	int cutpoint;
	int looped=0;

	if (weapon_type==0) {
		r = player_has_weapon(Primary_weapon, 0);
		if (r != HAS_ALL) {
			int	cur_weapon;
			int	try_again = 1;

			if(Primary_weapon == LASER_INDEX && Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS) {
				cur_weapon = POrderList(16);
			} else {
				cur_weapon = POrderList(Primary_weapon);
			}

			cutpoint = POrderList (255);

			while (try_again) {
				cur_weapon++;

				if (cur_weapon>=cutpoint)
				{
					if (looped)
					{
						HUD_init_message_literal(HM_DEFAULT, TXT_NO_PRIMARY);
						select_weapon(0, 0, 0, 1);
						try_again = 0;
						continue;
					}
					cur_weapon=0;
					looped=1;
				}


				if (cur_weapon==MAX_PRIMARY_WEAPONS)
					cur_weapon = 0;

				//	Hack alert!  Because the fusion uses 0 energy at the end (it's got the weird chargeup)
				//	it looks like it takes 0 to fire, but it doesn't, so never auto-select.
				// if (PlayerCfg.PrimaryOrder[cur_weapon] == FUSION_INDEX)
				//	continue;

				if (PlayerCfg.PrimaryOrder[cur_weapon] == Primary_weapon) {
					HUD_init_message_literal(HM_DEFAULT, TXT_NO_PRIMARY);
					select_weapon(0, 0, 0, 1);
					try_again = 0;			// Tried all weapons!

				} else if (PlayerCfg.PrimaryOrder[cur_weapon]!=255 && player_has_weapon_lasers_not_quads(PlayerCfg.PrimaryOrder[cur_weapon], 0) == HAS_ALL) {
					select_weapon(PlayerCfg.PrimaryOrder[cur_weapon], 0, 1, 1 );
					try_again = 0;
				}
			}
		}

	} else {

		Assert(weapon_type==1);
		r = player_has_weapon(Secondary_weapon, 1);
		if (r != HAS_ALL) {
			int	cur_weapon;
			int	try_again = 1;

			cur_weapon = SOrderList(Secondary_weapon);
			cutpoint = SOrderList (255);


			while (try_again) {
				cur_weapon++;

				if (cur_weapon>=cutpoint)
				{
					if (looped)
					{
						HUD_init_message_literal(HM_DEFAULT, "No secondary weapons selected!");
						try_again = 0;
						continue;
					}
					cur_weapon=0;
					looped=1;
				}

				if (cur_weapon==MAX_SECONDARY_WEAPONS)
					cur_weapon = 0;

				if (PlayerCfg.SecondaryOrder[cur_weapon] == Secondary_weapon) {
					HUD_init_message_literal(HM_DEFAULT, "No secondary weapons available!");
					try_again = 0;				// Tried all weapons!
				} else if (player_has_weapon(PlayerCfg.SecondaryOrder[cur_weapon], 1) == HAS_ALL) {
					select_weapon(PlayerCfg.SecondaryOrder[cur_weapon], 1, 1, 1 );
					try_again = 0;
				}
			}
		}
	}
	*/
}

int delayed_secondary_autoselect_weapon_index = -1;
//	---------------------------------------------------------------------
//called when one of these weapons is picked up
//when you pick up a secondary, you always get the weapon & ammo for it
//	Returns true if powerup picked up, else returns false.
int pick_up_secondary(int weapon_index,int count)
{
	int max;
	int	num_picked_up;
	int cutpoint;

	max = Secondary_ammo_max[weapon_index];

	if (Players[Player_num].secondary_ammo[weapon_index] >= max) {
		HUD_init_message(HM_DEFAULT|HM_REDUNDANT|HM_MAYDUPL, "%s %i %ss!", TXT_ALREADY_HAVE, Players[Player_num].secondary_ammo[weapon_index],SECONDARY_WEAPON_NAMES(weapon_index));
		return 0;
	}

	Players[Player_num].secondary_weapon_flags |= (1<<weapon_index);
	Players[Player_num].secondary_ammo[weapon_index] += count;

	num_picked_up = count;
	if (Players[Player_num].secondary_ammo[weapon_index] > max) {
		num_picked_up = count - (Players[Player_num].secondary_ammo[weapon_index] - max);
		Players[Player_num].secondary_ammo[weapon_index] = max;

		// Respawn extras
		if(weapon_index == HOMING_INDEX) {
			for(int i = 0; i < count - num_picked_up; i++) {
				maybe_drop_net_powerup(POW_HOMING_AMMO_1); 
			}
		}

		if(weapon_index == CONCUSSION_INDEX && (Game_mode & GM_MULTI) && Netgame.RespawnConcs) {
			for(int i = 0; i < count - num_picked_up; i++) {
				maybe_drop_net_powerup(POW_MISSILE_1); 
			}
		}
	}

	if(weapon_index == CONCUSSION_INDEX && (Game_mode & GM_MULTI) && Netgame.RespawnConcs) {
		RespawningConcussions[Player_num] += num_picked_up; 
	}

	if (Players[Player_num].secondary_ammo[weapon_index] == count)	// only autoselect if player didn't have any
	{
		cutpoint=SOrderList (255);


		// Picked up something better than you have, or you're dry
		if(SOrderList(weapon_index)<cutpoint && (SOrderList(weapon_index)<SOrderList(Secondary_weapon) || Players[Player_num].secondary_ammo[Secondary_weapon] == 0)) {

			// Are you firing? 
			if(Controls.fire_secondary_state) {
				// Remember what we picked up, if it's better than what's waiting now
				if(PlayerCfg.SelectAfterFire) {

					// Nothing waiting -- remember this weapon
					if(delayed_secondary_autoselect_weapon_index == -1) {
						delayed_secondary_autoselect_weapon_index = weapon_index;

					// Something waiting -- is this better? 
					} else {
						if(SOrderList(weapon_index) < SOrderList(delayed_secondary_autoselect_weapon_index)) {					
							delayed_secondary_autoselect_weapon_index = weapon_index;
						}
					}
				} else if (! PlayerCfg.NoFireAutoselect) {		
					select_weapon(weapon_index,1, 0, 1);
				}
			} else {	
				select_weapon(weapon_index,1, 0, 1);
			}
		}
		


		//cutpoint=SOrderList (255);
		//if (((Controls.fire_secondary_state && PlayerCfg.NoFireAutoselect)?0:1) && SOrderList (weapon_index)<cutpoint && ((SOrderList (weapon_index) < SOrderList(Secondary_weapon)) || (Players[Player_num].secondary_ammo[Secondary_weapon] == 0))   )
		//	select_weapon(weapon_index,1, 0, 1);
	}

	if (num_picked_up>1) {
		PALETTE_FLASH_ADD(15,15,15);
		HUD_init_message(HM_DEFAULT, "%d %s%s",num_picked_up,SECONDARY_WEAPON_NAMES(weapon_index), TXT_SX);
	}
	else {
		PALETTE_FLASH_ADD(10,10,10);
		HUD_init_message(HM_DEFAULT, "%s!",SECONDARY_WEAPON_NAMES(weapon_index));
	}

	return 1;
}

void ReorderPrimary ()
{
	newmenu_item m[MAX_PRIMARY_WEAPONS+2];
	int i;

	for (i=0;i<MAX_PRIMARY_WEAPONS+2;i++)
	{
		m[i].type=NM_TYPE_MENU;
		if (PlayerCfg.PrimaryOrder[i]==255)
			m[i].text="--- Never Autoselect below ---";
		else if (PlayerCfg.PrimaryOrder[i]==16)
			m[i].text="Quad Lasers";
		else
			m[i].text=(char *)PRIMARY_WEAPON_NAMES(PlayerCfg.PrimaryOrder[i]);
		m[i].value=PlayerCfg.PrimaryOrder[i];
	}
	i = newmenu_doreorder("Reorder Primary","Shift+Up/Down arrow to move item", i, m, NULL, NULL);
	for (i=0;i<MAX_PRIMARY_WEAPONS+2;i++)
		PlayerCfg.PrimaryOrder[i]=m[i].value;
}

void ReorderSecondary ()
{
	newmenu_item m[MAX_SECONDARY_WEAPONS+1];
	int i;

	for (i=0;i<MAX_SECONDARY_WEAPONS+1;i++)
	{
		m[i].type=NM_TYPE_MENU;
		if (PlayerCfg.SecondaryOrder[i]==255)
			m[i].text="--- Never Autoselect below ---";
		else
			m[i].text=(char *)SECONDARY_WEAPON_NAMES(PlayerCfg.SecondaryOrder[i]);
		m[i].value=PlayerCfg.SecondaryOrder[i];
	}
	i = newmenu_doreorder("Reorder Secondary","Shift+Up/Down arrow to move item", i, m, NULL, NULL);
	for (i=0;i<MAX_SECONDARY_WEAPONS+1;i++)
		PlayerCfg.SecondaryOrder[i]=m[i].value;
}

int POrderList (int num)
{
	int i;

	for (i=0;i<MAX_PRIMARY_WEAPONS+2;i++)
	if (PlayerCfg.PrimaryOrder[i]==num)
	{
		return (i);
	}
	//Error ("Primary Weapon is not in order list!!!");
	InitWeaponOrdering();
	for (i=0;i<MAX_PRIMARY_WEAPONS+2;i++)
	if (DefaultPrimaryOrder[i]==num)
	{
		return (i);
	}
	RT_LOG(RT_LOGSERVERITY_HIGH , "Primary Weapon is not in order list!!!");
}

int SOrderList (int num)
{
	int i;

	for (i=0;i<MAX_SECONDARY_WEAPONS+1;i++)
		if (PlayerCfg.SecondaryOrder[i]==num)
		{
			return (i);
		}
	RT_LOG(RT_LOGSERVERITY_HIGH, "Secondary Weapon is not in order list!!!");
}

int pick_up_primary_helper(int weapon_index, int is_quads);
int pick_up_quads()
{
	return pick_up_primary_helper(LASER_INDEX, 1); 
}

//called when a primary weapon is picked up
//returns true if actually picked up
int pick_up_primary(int weapon_index) {
	return pick_up_primary_helper(weapon_index, 0); 
}

int delayed_primary_autoselect_weapon_index = -1; 
int pick_up_primary_helper(int weapon_index, int is_quads)
{

	int cutpoint;
	ubyte flag = 1<<weapon_index;

	if (weapon_index!=LASER_INDEX && Players[Player_num].primary_weapon_flags & flag) {		//already have
		HUD_init_message(HM_DEFAULT|HM_REDUNDANT|HM_MAYDUPL, "%s %s!", TXT_ALREADY_HAVE_THE, PRIMARY_WEAPON_NAMES(weapon_index));
		return 0;
	}

	Players[Player_num].primary_weapon_flags |= flag;

	cutpoint=POrderList (255);

	if(is_quads) {
		weapon_index = 16; 
	}

	int primary_weapon_index = Primary_weapon;
	if(Primary_weapon == LASER_INDEX && (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)) {
		primary_weapon_index = 16; 
	}

	// Picked up something better than you have
	if(POrderList(weapon_index)<cutpoint && POrderList(weapon_index)<POrderList(primary_weapon_index)) {

		// Are you firing? 
		if(Controls.fire_primary_state) {
			// Remember what we picked up, if it's better than what's waiting now
			if(PlayerCfg.SelectAfterFire) {

				// Nothing waiting -- remember this weapon
				if(delayed_primary_autoselect_weapon_index == -1) {
					delayed_primary_autoselect_weapon_index = weapon_index;

				// Something waiting -- is this better? 
				} else {
					if(POrderList(weapon_index) < POrderList(delayed_primary_autoselect_weapon_index)) {					
						delayed_primary_autoselect_weapon_index = weapon_index;
					}
				}
			} else if (! PlayerCfg.NoFireAutoselect) {		
				select_weapon(weapon_index,0,0,1);
			}
		} else {	
			select_weapon(weapon_index,0,0,1);
		}
	}

	//if (((Controls.fire_primary_state && PlayerCfg.NoFireAutoselect)?0:1) && POrderList(weapon_index)<cutpoint && POrderList(weapon_index)<POrderList(Primary_weapon)) {
	//	select_weapon(weapon_index,0,0,1);
	//}

	PALETTE_FLASH_ADD(7,14,21);

   if (weapon_index!=LASER_INDEX)
		HUD_init_message(HM_DEFAULT, "%s!",PRIMARY_WEAPON_NAMES(weapon_index));

	return 1;
}

/* SelectAfterFire */ 
void delayed_autoselect() {
	if(delayed_primary_autoselect_weapon_index > -1 && ! Controls.fire_primary_state) {
		select_weapon(delayed_primary_autoselect_weapon_index, 0, 0, 1); 
		delayed_primary_autoselect_weapon_index = -1; 
	}

	if(delayed_secondary_autoselect_weapon_index > -1 && ! Controls.fire_secondary_state) {
		select_weapon(delayed_secondary_autoselect_weapon_index, 1, 0, 1); 
		delayed_secondary_autoselect_weapon_index = -1; 
	}	
}

//called when ammo (for the vulcan cannon) is picked up
//Return true if ammo picked up, else return false.
int pick_up_ammo(int class_flag,int weapon_index,int ammo_count)
{
	int max;
	int cutpoint;
	int old_ammo=class_flag;		//kill warning

	Assert(class_flag==CLASS_PRIMARY && weapon_index==VULCAN_INDEX);

	max = Primary_ammo_max[weapon_index];
	if (Players[Player_num].primary_ammo[weapon_index] == max)
		return 0;

	old_ammo = Players[Player_num].primary_ammo[weapon_index];

	Players[Player_num].primary_ammo[weapon_index] += ammo_count;

	if (Players[Player_num].primary_ammo[weapon_index] > max) {
		ammo_count += (max - Players[Player_num].primary_ammo[weapon_index]);
		Players[Player_num].primary_ammo[weapon_index] = max;
	}
	cutpoint=POrderList (255);

	int primary_weapon_index = Primary_weapon;
	if(Primary_weapon == LASER_INDEX && (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)) {
		primary_weapon_index = 16; 
	}

	if ( Players[Player_num].primary_weapon_flags&(1<<weapon_index)  && old_ammo==0 &&
		POrderList(weapon_index)<cutpoint && POrderList(weapon_index)<POrderList(primary_weapon_index)) {

		// Are you firing? 
		if(Controls.fire_primary_state) {
			// Remember what we picked up, if it's better than what's waiting now
			if(PlayerCfg.SelectAfterFire) {

				// Nothing waiting -- remember this weapon
				if(delayed_primary_autoselect_weapon_index == -1) {
					delayed_primary_autoselect_weapon_index = weapon_index;

				// Something waiting -- is this better? 
				} else {
					if(POrderList(weapon_index) < POrderList(delayed_primary_autoselect_weapon_index)) {					
						delayed_primary_autoselect_weapon_index = weapon_index;
					}
				}
			} else if (! PlayerCfg.NoFireAutoselect) {		
				select_weapon(weapon_index,0,0,1);
			}
		} else {	
			select_weapon(weapon_index,0,0,1);
		}

		//select_weapon(weapon_index,0,0,1);
	}

	return 1;	//return amount used
}

/*
 * reads n weapon_info structs from a PHYSFS_file
 */
int weapon_info_read_n(weapon_info *wi, int n, PHYSFS_file *fp, int file_version)
{
	(void)file_version;
	int i, j;

	for (i = 0; i < n; i++) {
		wi[i].render_type = PHYSFSX_readByte(fp);
		wi[i].model_num = PHYSFSX_readByte(fp);
		wi[i].model_num_inner = PHYSFSX_readByte(fp);
		wi[i].persistent = PHYSFSX_readByte(fp);
		wi[i].flash_vclip = PHYSFSX_readByte(fp);
		wi[i].flash_sound = PHYSFSX_readShort(fp);
		wi[i].robot_hit_vclip = PHYSFSX_readByte(fp);
		wi[i].robot_hit_sound = PHYSFSX_readShort(fp);
		wi[i].wall_hit_vclip = PHYSFSX_readByte(fp);
		wi[i].wall_hit_sound = PHYSFSX_readShort(fp);
		wi[i].fire_count = PHYSFSX_readByte(fp);
		wi[i].ammo_usage = PHYSFSX_readByte(fp);
		wi[i].weapon_vclip = PHYSFSX_readByte(fp);
		wi[i].destroyable = PHYSFSX_readByte(fp);
		wi[i].matter = PHYSFSX_readByte(fp);
		wi[i].bounce = PHYSFSX_readByte(fp);
		wi[i].homing_flag = PHYSFSX_readByte(fp);
		wi[i].dum1 = PHYSFSX_readByte(fp);
		wi[i].dum2 = PHYSFSX_readByte(fp);
		wi[i].dum3 = PHYSFSX_readByte(fp);
		wi[i].energy_usage = PHYSFSX_readFix(fp);
		wi[i].fire_wait = PHYSFSX_readFix(fp);
		bitmap_index_read(&wi[i].bitmap, fp);
		wi[i].blob_size = PHYSFSX_readFix(fp);
		wi[i].flash_size = PHYSFSX_readFix(fp);
		wi[i].impact_size = PHYSFSX_readFix(fp);
		for (j = 0; j < NDL; j++)
			wi[i].strength[j] = PHYSFSX_readFix(fp);
		for (j = 0; j < NDL; j++)
			wi[i].speed[j] = PHYSFSX_readFix(fp);
		wi[i].mass = PHYSFSX_readFix(fp);
		wi[i].drag = PHYSFSX_readFix(fp);
		wi[i].thrust = PHYSFSX_readFix(fp);

		wi[i].po_len_to_width_ratio = PHYSFSX_readFix(fp);
		wi[i].light = PHYSFSX_readFix(fp);
		wi[i].lifetime = PHYSFSX_readFix(fp);
		wi[i].damage_radius = PHYSFSX_readFix(fp);
		bitmap_index_read(&wi[i].picture, fp);

		/*
		RT_LOG(RT_LOGSERVERITY_MEDIUM, "\n\nWeapon %d\n", i);
		RT_LOGF(RT_LOGSERVERITY_MEDIUM, "    fire_wait: %f\n", (float)(wi[i].fire_wait)/(float)(F1_0));
		RT_LOGF(RT_LOGSERVERITY_MEDIUM, "    blob_size: %f\n", (float)(wi[i].blob_size)/(float)(F1_0));
		RT_LOGF(RT_LOGSERVERITY_MEDIUM, "    mass: %f\n", (float)(wi[i].mass)/(float)(F1_0));
		RT_LOGF(RT_LOGSERVERITY_MEDIUM, "    drag: %f\n", (float)(wi[i].drag)/(float)(F1_0));
		RT_LOGF(RT_LOGSERVERITY_MEDIUM, "    thrust: %f\n", (float)(wi[i].thrust)/(float)(F1_0));
		RT_LOGF(RT_LOGSERVERITY_MEDIUM, "    lifetime: %f\n", (float)(wi[i].lifetime)/(float)(F1_0));
		RT_LOGF(RT_LOGSERVERITY_MEDIUM, "    damage_radius: %f\n", (float)(wi[i].damage_radius)/(float)(F1_0));
		RT_LOG(RT_LOGSERVERITY_MEDIUM, "    strength: \n");
		for (j = 0; j < NDL; j++) {
			RT_LOGF(RT_LOGSERVERITY_MEDIUM, "        %f\n", (float)(wi[i].strength[j])/(float)(F1_0));
		}
		RT_LOG(RT_LOGSERVERITY_MEDIUM, "    speed: \n");
		for (j = 0; j < NDL; j++) {
			RT_LOGF(RT_LOGSERVERITY_MEDIUM, "        %f\n", (float)(wi[i].speed[j])/(float)(F1_0));
		}
		*/
	}
	return i;
}

