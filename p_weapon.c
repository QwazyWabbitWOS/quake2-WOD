// g_weapon.c

#include "g_local.h"
#include "m_player.h"
#include "x_fbomb.h"
#include "x_fire.h"



static qboolean	is_quad;
static byte		is_silenced;


void weapon_grenade_fire (edict_t *ent, qboolean held);


void P_ProjectSource (gclient_t *client, vec3_t point, vec3_t distance,
							 vec3_t forward, vec3_t right, vec3_t result)
{
	vec3_t	_distance;

	VectorCopy (distance, _distance);
	if (client->pers.hand == LEFT_HANDED)
		_distance[1] *= -1;
	else if (client->pers.hand == CENTER_HANDED)
		_distance[1] = 0;
	G_ProjectSource (point, _distance, forward, right, result);
}


/*
===============
PlayerNoise

Each player can have two noise objects associated with it:
a personal noise (jumping, pain, weapon firing), and a weapon
target noise (bullet wall impacts)

Monsters that don't directly see the player can move
to a noise in hopes of seeing the player from there.
===============
*/
void PlayerNoise(edict_t *who, vec3_t where, int type)
{
	edict_t		*noise;

	if (type == PNOISE_WEAPON)
	{
		if (who->client->silencer_shots)
		{
			who->client->silencer_shots--;
			return;
		}
	}

	if (deathmatch->value)
		return;

	if (who->flags & FL_NOTARGET)
		return;


	if (!who->mynoise)
	{
		noise = G_Spawn();
		noise->classname = "player_noise";
		VectorSet (noise->mins, -8, -8, -8);
		VectorSet (noise->maxs, 8, 8, 8);
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		who->mynoise = noise;

		noise = G_Spawn();
		noise->classname = "player_noise";
		VectorSet (noise->mins, -8, -8, -8);
		VectorSet (noise->maxs, 8, 8, 8);
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		who->mynoise2 = noise;
	}

	if (type == PNOISE_SELF || type == PNOISE_WEAPON)
	{
		noise = who->mynoise;
		level.sound_entity = noise;
		level.sound_entity_framenum = level.framenum;
	}
	else // type == PNOISE_IMPACT
	{
		noise = who->mynoise2;
		level.sound2_entity = noise;
		level.sound2_entity_framenum = level.framenum;
	}

	VectorCopy (where, noise->s.origin);
	VectorSubtract (where, noise->maxs, noise->absmin);
	VectorAdd (where, noise->maxs, noise->absmax);
	noise->teleport_time = level.time;
	gi.linkentity (noise);
}


qboolean Pickup_Weapon (edict_t *ent, edict_t *other)
{
	int			index;
	gitem_t		*item, *item1, *item2;
	gitem_t		*ammo;

	item = ent->item;

#ifdef EXT_DEVT
	// HACK to use when I'm editing maps for Extinction.
	if (sv_cheats->value)
		gi.cprintf (other, PRINT_HIGH, "weapon at [%i,%i,%i]\n",
			(int)ent->s.origin[0], (int)ent->s.origin[1],
			(int)ent->s.origin[2]);
#endif // EXT_DEVT

	// Determine what weapon was picked up and what weapons will be added to the
	// inventory, depending on weapon banning.
	item1 = item2 = NULL;
	if (item == &gI_weapon_shotgun || item == &gI_weapon_sniper)
	{
		if (!((int)weaponban->value & WB_SHOTGUN))
			item1 = &gI_weapon_shotgun;
		else
			item = &gI_weapon_sniper;
		if (!((int)weaponban->value & WB_SNIPERGUN))
			item2 = &gI_weapon_sniper;
	}
	else if (item == &gI_weapon_supershotgun || item == &gI_weapon_freezer)
	{
		if (!((int)weaponban->value & WB_SUPERSHOTGUN))
			item1 = &gI_weapon_supershotgun;
		else
			item = &gI_weapon_freezer;
		if (!((int)weaponban->value & WB_FREEZEGUN))
			item2 = &gI_weapon_freezer;
	}
	else if (item == &gI_weapon_machinegun || item == &gI_weapon_machine)
	{
		if (!((int)weaponban->value & WB_MACHINEROCKETGUN))
			item1 = &gI_weapon_machinegun;
		else
			item = &gI_weapon_machine;
		if (!((int)weaponban->value & WB_MACHINEGUN))
			item2 = &gI_weapon_machine;
	}
	else if (item == &gI_weapon_chaingun || item == &gI_weapon_streetsweeper)
	{
		if (!((int)weaponban->value & WB_CHAINGUN))
			item1 = &gI_weapon_chaingun;
		else
			item = &gI_weapon_streetsweeper;
		if (!((int)weaponban->value & WB_STREETSWEEPER))
			item2 = &gI_weapon_streetsweeper;
	}
	else if (item == &gI_weapon_grenadelauncher || item == &gI_weapon_bazooka)
	{
		if (!((int)weaponban->value & WB_GRENADELAUNCHER))
			item1 = &gI_weapon_grenadelauncher;
		else
			item = &gI_weapon_bazooka;
		if (!((int)weaponban->value & WB_BAZOOKA))
			item2 = &gI_weapon_bazooka;
	}
	else if (item == &gI_weapon_rocketlauncher
		|| item == &gI_weapon_guidedrocketlauncher)
	{
		if (!((int)weaponban->value & WB_ROCKETLAUNCHER))
			item1 = &gI_weapon_rocketlauncher;
		else
			item = &gI_weapon_guidedrocketlauncher;
		if (!((int)weaponban->value & WB_GUIDEDROCKETLAUNCHER))
			item2 = &gI_weapon_guidedrocketlauncher;
	}
	else if (item == &gI_weapon_hyperblaster || item == &gI_weapon_plasma)
	{
		if (!((int)weaponban->value & WB_HYPERBLASTER))
			item1 = &gI_weapon_hyperblaster;
		else
			item = &gI_weapon_plasma;
		if (!((int)weaponban->value & WB_PLASMARIFLE))
			item2 = &gI_weapon_plasma;
	}
	else if (item == &gI_weapon_railgun || item == &gI_weapon_railgun2)
	{
		if (!((int)weaponban->value & WB_FLAMETHROWER))
			item1 = &gI_weapon_railgun;
		else
			item = &gI_weapon_railgun2;
		if (!((int)weaponban->value & WB_RAILGUN))
			item2 = &gI_weapon_railgun2;
	}
	else if (item == &gI_weapon_bfg)
	{
		if (!((int)weaponban->value & WB_BFG10K))
			item1 = &gI_weapon_bfg;
	}

	index = ITEM_INDEX (item);
	ammo = item->ammo;

	if ((((int)(dmflags->value) & DF_WEAPONS_STAY) || coop->value) 
	&& other->client->pers.inventory[index])
	{
		if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
			return false;	// leave the weapon for others to pickup
		if (ammo && !Add_Ammo (other, ammo, 0))
			return false;	// gun *and* full ammo already
	}

	// Now add the weapons to their inventory.
	if (item1)
		other->client->pers.inventory[ITEM_INDEX(item1)]++;
	if (item2)
		other->client->pers.inventory[ITEM_INDEX(item2)]++;

	if (!(ent->spawnflags & DROPPED_ITEM))
	{
		// give them some ammo with it
		if ((int)dmflags->value & DF_INFINITE_AMMO)
			Add_Ammo (other, ammo, 1000);
		else
			Add_Ammo (other, ammo, ammo->quantity * 2);

		if (! (ent->spawnflags & DROPPED_PLAYER_ITEM))
		{
			if (deathmatch->value)
			{
				if ((int)(dmflags->value) & DF_WEAPONS_STAY)
				{
					// If the weapon hasn't been moved, do a quick in-place respawn,
					// otherwise do a more visual respawn.
					if (VectorCompare (ent->pos1, ent->s.origin)
					&& VectorCompare (ent->pos2, ent->s.angles))
						ent->flags |= FL_RESPAWN;
					else
						SetRespawn (ent, 2 * FRAMETIME);
				}
				else
					SetRespawn (ent, 30);
			}
			if (coop->value)
				ent->flags |= FL_RESPAWN;
		}
	}

	if (other->client->pers.weapon != item && 
		(other->client->pers.inventory[index] == 1) &&
		(!deathmatch->value || other->client->pers.weapon == &gI_weapon_blaster))
		other->client->newweapon = item;

	return true;
}


/*
===============
ChangeWeapon

The old weapon has been dropped all the way, so make the new one
current
===============
*/

// ### Hentai ### BEGIN
void ShowGun(edict_t *ent)
{	
	char heldmodel[128];
	int n;

	if(!ent->client->pers.weapon)
	{
		ent->client->ps.gunindex = 0;		// WI: seems to be missing...?
		ent->s.modelindex2 = 0;
		return;
	}
	ent->s.modelindex2 = 255;
	strcpy(heldmodel, "#");
	strcat(heldmodel, ent->client->pers.weapon->icon);	
	strcat(heldmodel, ".md2");
	//gi.dprintf ("%s\n", heldmodel);
	n = (gi.modelindex(heldmodel) - vwep_index) << 8;

	ent->s.skinnum &= 0xFF;
	ent->s.skinnum |= n;
}
// ### Hentai ### END

void ChangeWeapon (edict_t *ent)
{
	if (ent->client->grenade_time)
	{
		ent->client->grenade_time = level.time;
		ent->client->weapon_sound = 0;
		weapon_grenade_fire (ent, false);
		ent->client->grenade_time = 0;
	}

	ent->client->pers.lastweapon = ent->client->pers.weapon;
	ent->client->pers.weapon = ent->client->newweapon;
	ent->client->newweapon = NULL;
	ent->client->machinegun_shots = 0;

	if (ent->client->pers.weapon && ent->client->pers.weapon->ammo)
		ent->client->ammo_index = ITEM_INDEX(ent->client->pers.weapon->ammo);
	else
		ent->client->ammo_index = 0;

	if (!ent->client->pers.weapon || ent->s.modelindex != 255 || ent->deadflag) // ### Hentai ### 
	{	// dead, or not on client, so VWep animations could do wacky things
		ent->client->ps.gunindex = 0;
		ent->s.modelindex2 = 0;
		return;
	}

	ent->client->weaponstate = WEAPON_ACTIVATING;
	ent->client->ps.gunframe = 0;
	ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);

	// ### Hentai ### BEGIN
	ent->client->anim_priority = ANIM_PAIN;
	if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
			ent->s.frame = FRAME_crpain1;
			ent->client->anim_end = FRAME_crpain4;
	}
	else
	{
			ent->s.frame = FRAME_pain301;
			ent->client->anim_end = FRAME_pain304;
			
	}
	
	ShowGun(ent);	

	// ### Hentai ### END

	// If they switched to/from a laser-guided weapon, adjust the lasersight.
	LaserSight_Check (ent);
}

/*
=================
NoAmmoWeaponChange
=================
*/
void NoAmmoWeaponChange (edict_t *ent)
{
	// Plasma rifle
	if (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_plasma.ammo)]
		>= gI_weapon_plasma.quantity
	&&  ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_plasma)])
	{
		ent->client->newweapon = &gI_weapon_plasma;
	}

	// Streetsweeper
	else if (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_streetsweeper.ammo)]
		>= gI_weapon_streetsweeper.quantity
	&&  ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_streetsweeper)])
	{
		ent->client->newweapon = &gI_weapon_streetsweeper;
	}

	// Machine rocket gun
	else if (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_machinegun.ammo)]
		>= gI_weapon_machinegun.quantity
	&&  ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_machinegun)])
	{
		ent->client->newweapon = &gI_weapon_machinegun;
	}

	// Hyperblaster
	else if (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_hyperblaster.ammo)]
		>= gI_weapon_hyperblaster.quantity
	&&  ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_hyperblaster)])
	{
		ent->client->newweapon = &gI_weapon_hyperblaster;
	}

	// Chaingun
	else if (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_chaingun.ammo)]
		>= gI_weapon_chaingun.quantity
	&&  ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_chaingun)])
	{
		ent->client->newweapon = &gI_weapon_chaingun;
	}

	// Flame rocketlauncher
	else if (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_rocketlauncher.ammo)]
		>= gI_weapon_rocketlauncher.quantity
	&&  ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_rocketlauncher)])
	{
		ent->client->newweapon = &gI_weapon_rocketlauncher;
	}

	// Guided rocketlauncher
	else if (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_guidedrocketlauncher.ammo)]
		>= gI_weapon_guidedrocketlauncher.quantity
	&&  ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_guidedrocketlauncher)])
	{
		ent->client->newweapon = &gI_weapon_guidedrocketlauncher;
	}

	// Standard machinegun
	else if (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_machine.ammo)]
		>= gI_weapon_machine.quantity
	&&  ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_machine)])
	{
		ent->client->newweapon = &gI_weapon_machine;
	}

	// Super shotgun
	else if (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_supershotgun.ammo)]
		>= gI_weapon_supershotgun.quantity
	&&  ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_supershotgun)])
	{
		ent->client->newweapon = &gI_weapon_supershotgun;
	}

	// Shotgun
	else if (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_shotgun.ammo)]
		>= gI_weapon_shotgun.quantity
	&&  ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_shotgun)])
	{
		ent->client->newweapon = &gI_weapon_shotgun;
	}

	// Super blaster
	else if (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_superblaster.ammo)]
		>= gI_weapon_superblaster.quantity
	&&  ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_superblaster)])
	{
		ent->client->newweapon = &gI_weapon_superblaster;
	}

	// Bazooka
	else if (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_bazooka.ammo)]
		>= ent->client->dM_ammoCost
	&&  ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_bazooka)])
	{
		ent->client->newweapon = &gI_weapon_bazooka;
	}

	// Sniper gun
	else if (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_sniper.ammo)]
		>= gI_weapon_sniper.quantity
	&&  ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_sniper)])
	{
		ent->client->newweapon = &gI_weapon_sniper;
	}

	// Grenade launcher
	else if (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_grenadelauncher.ammo)]
		>= ent->client->dM_ammoCost
	&&  ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_grenadelauncher)])
	{
		ent->client->newweapon = &gI_weapon_grenadelauncher;
	}

	// Flamethrower
	else if (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_railgun.ammo)]
		>= gI_weapon_railgun.quantity
	&&  ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_railgun)])
	{
		ent->client->newweapon = &gI_weapon_railgun;
	}

	// Railgun
	else if (ent->client->pers.inventory[ITEM_INDEX(gI_weapon_railgun2.ammo)]
		>= gI_weapon_railgun2.quantity
	&&  ent->client->pers.inventory[ITEM_INDEX(&gI_weapon_railgun2)])
	{
		ent->client->newweapon = &gI_weapon_railgun2;
	}

	// No other choice...give them the blaster.
	else
		ent->client->newweapon = &gI_weapon_blaster;
}

/*
=================
Think_Weapon

Called by ClientBeginServerFrame and ClientThink
=================
*/
void Think_Weapon (edict_t *ent)
{
	// if just died, put the weapon away
	if (ent->health <= 0)
	{
		ent->client->newweapon = NULL;
		ChangeWeapon (ent);
	}

	// call active weapon think routine
	if (ent->client->pers.weapon && ent->client->pers.weapon->weaponthink)
	{
		is_quad = (ent->client->quad_framenum > level.framenum);
		if (ent->client->silencer_shots)
			is_silenced = MZ_SILENCED;
		else
			is_silenced = 0;
		ent->client->pers.weapon->weaponthink (ent);
	}
}


/*
================
Use_Weapon

Make the weapon ready if there is ammo
================
*/
void Use_Weapon (edict_t *ent, gitem_t *item)
{
	int			ammo_index;
	gitem_t		*ammo_item;

	// see if we're already using it
	if (item == ent->client->pers.weapon)
		return;

	if (item->ammo && !g_select_empty->value && !(item->flags & IT_AMMO))
	{
		ammo_item = item->ammo;
		ammo_index = ITEM_INDEX(ammo_item);

		if (!ent->client->pers.inventory[ammo_index])
		{
			gi.cprintf (ent, PRINT_HIGH, "No %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
			return;
		}

		if (ent->client->pers.inventory[ammo_index] < item->quantity)
		{
			gi.cprintf (ent, PRINT_HIGH, "Not enough %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
			return;
		}
	}

	// change to this weapon when down
	ent->client->newweapon = item;
}



/*
================
Drop_Weapon
================
*/
void Drop_Weapon (edict_t *ent, gitem_t *item)
{
	gitem_t *altitem = NULL;
	int index, altindex;

	if ((int)(dmflags->value) & DF_WEAPONS_STAY)
		return;

	// Find the counterpart to this weapon.
	if (item == &gI_weapon_shotgun)
		altitem = &gI_weapon_sniper;
	else if (item == &gI_weapon_sniper)
		altitem = &gI_weapon_shotgun;
	else if (item == &gI_weapon_supershotgun)
		altitem = &gI_weapon_freezer;
	else if (item == &gI_weapon_freezer)
		altitem = &gI_weapon_supershotgun;
	else if (item == &gI_weapon_machinegun)
		altitem = &gI_weapon_machine;
	else if (item == &gI_weapon_machine)
		altitem = &gI_weapon_machinegun;
	else if (item == &gI_weapon_chaingun)
		altitem = &gI_weapon_streetsweeper;
	else if (item == &gI_weapon_streetsweeper)
		altitem = &gI_weapon_chaingun;
	else if (item == &gI_weapon_grenadelauncher)
		altitem = &gI_weapon_bazooka;
	else if (item == &gI_weapon_bazooka)
		altitem = &gI_weapon_grenadelauncher;
	else if (item == &gI_weapon_rocketlauncher)
		altitem = &gI_weapon_guidedrocketlauncher;
	else if (item == &gI_weapon_guidedrocketlauncher)
		altitem = &gI_weapon_rocketlauncher;
	else if (item == &gI_weapon_hyperblaster)
		altitem = &gI_weapon_plasma;
	else if (item == &gI_weapon_plasma)
		altitem = &gI_weapon_hyperblaster;
	else if (item == &gI_weapon_railgun)
		altitem = &gI_weapon_railgun2;
	else if (item == &gI_weapon_railgun2)
		altitem = &gI_weapon_railgun;

	// Get their indexes.
	index = ITEM_INDEX (item);
	altindex = (altitem != NULL) ? ITEM_INDEX (altitem) : 0;

	// Decrease their inventory of the dropped weapon.  If they have an equal
	// number of the counterpart weapon, decrease that too.
	if (altitem != NULL
		&& ent->client->pers.inventory[index]
			== ent->client->pers.inventory[altindex])
		ent->client->pers.inventory[altindex]--;
	ent->client->pers.inventory[index]--;

	// Finally, drop the weapon.  Don't drop an alternate weapon -- make it a
	// normal weapon.
	/* if (item->flags & IT_ALTWEAPON)
		Drop_Item (ent, altitem);
	else */
		Drop_Item (ent, item);

	// If they dropped the current weapon, switch.
	if ((item == ent->client->pers.weapon && !ent->client->pers.inventory[index])
	|| (altitem == ent->client->pers.weapon && !ent->client->pers.inventory[altindex]))
	{
		NoAmmoWeaponChange (ent);
	}
}


/*
================
Weapon_Generic

A generic function to handle the basics of weapon thinking
================
*/
#define FRAME_FIRE_FIRST		(FRAME_ACTIVATE_LAST + 1)
#define FRAME_IDLE_FIRST		(FRAME_FIRE_LAST + 1)
#define FRAME_DEACTIVATE_FIRST	(FRAME_IDLE_LAST + 1)

void Weapon_Generic (edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST,
							int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST,
							int *pause_frames, int *fire_frames,
							void (*fire)(edict_t *ent))
{
	int		n;

	if (ent->deadflag || ent->s.modelindex != 255) // ### Hentai ### 
		return; // not on client, so VWep animations could do wacky things

	if (ent->client->weaponstate == WEAPON_DROPPING)
	{
		if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST)
		{
			ChangeWeapon (ent);
			return;
		}
			// ### Hentai ### BEGIN
		else if((FRAME_DEACTIVATE_LAST - ent->client->ps.gunframe) == 4)
		{
			ent->client->anim_priority = ANIM_REVERSE;
			if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crpain4+1;
				ent->client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent->s.frame = FRAME_pain304+1;
				ent->client->anim_end = FRAME_pain301;
				
			}
		}
		// ### Hentai ### END

		ent->client->ps.gunframe++;
		return;
	}

	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{
		if (ent->client->ps.gunframe == FRAME_ACTIVATE_LAST)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}

		ent->client->ps.gunframe++;
		return;
	}

	if ((ent->client->newweapon) && (ent->client->weaponstate != WEAPON_FIRING))
	{
		ent->client->weaponstate = WEAPON_DROPPING;
		ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;
				// ### Hentai ### BEGIN
		if((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4)
		{
			ent->client->anim_priority = ANIM_REVERSE;
			if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crpain4+1;
				ent->client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent->s.frame = FRAME_pain304+1;
				ent->client->anim_end = FRAME_pain301;
				
			}
		}
		// ### Hentai ### END

		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if (((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK))
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;

			if ((!ent->client->ammo_index) || 
				(ent->client->pers.inventory[ent->client->ammo_index]
				>= ent->client->pers.weapon->quantity))
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;

				// ### Hentai ### BEGIN
				// start the animation
				ent->client->anim_priority = ANIM_ATTACK;
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				{
					ent->s.frame = FRAME_crattak1-1;
					ent->client->anim_end = FRAME_crattak9;
				}
				else
				{
					ent->s.frame = FRAME_attack1-1;
					ent->client->anim_end = FRAME_attack8;
				}
				// ### Hentai ### END
			}
			else
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange (ent);
			}
		}
		else
		{
			if (ent->client->ps.gunframe == FRAME_IDLE_LAST)
			{
				ent->client->ps.gunframe = FRAME_IDLE_FIRST;
				return;
			}

			if (pause_frames)
			{
				for (n = 0; pause_frames[n]; n++)
				{
					if (ent->client->ps.gunframe == pause_frames[n])
					{
						if (rand()&15)
							return;
					}
				}
			}

			ent->client->ps.gunframe++;
			return;
		}
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		for (n = 0; fire_frames[n]; n++)
		{
			if (ent->client->ps.gunframe == fire_frames[n])
			{
				if (ent->client->quad_framenum > level.framenum)
					gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);

				fire (ent);
				break;
			}
		}

		if (!fire_frames[n])
			ent->client->ps.gunframe++;

		if (ent->client->ps.gunframe == FRAME_IDLE_FIRST+1)
			ent->client->weaponstate = WEAPON_READY;
	}
}


/*
======================================================================

GRENADE

======================================================================
*/

#define GRENADE_TIMER		3.0
#define GRENADE_MINSPEED	400
#define GRENADE_MAXSPEED	800

void weapon_grenade_fire (edict_t *ent, qboolean held)
{
	vec3_t	offset;
	vec3_t	forward, right;
	vec3_t	start;
	int		damage = 125;
	float	timer;
	int		speed;
	float	radius;
        float             typ;

	radius = damage+40;
	if (is_quad)
		damage *= 4;

	VectorSet(offset, 8, 8, ent->viewheight-8);
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	timer = ent->client->grenade_time - level.time;
	speed = GRENADE_MINSPEED + (GRENADE_TIMER - timer) * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER);

	// darKMajick:
	typ = ent->client->dM_grenade;
	// fire_grenade2 (ent, start, forward, damage, speed, timer, radius);
	fire_grenade_dM (ent, start, forward, damage, speed, timer, radius, typ,
		/* held */ true, /* bazookad */ false);

	if (!ent->deadflag && ent->s.modelindex == 255)
	{
		// ### Hentai ### BEGIN
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			ent->client->anim_priority = ANIM_ATTACK;
			ent->s.frame = FRAME_crattak1-1;
			ent->client->anim_end = FRAME_crattak3;
		}
		else if(ent->s.modelindex != 255)
		{
			ent->client->anim_priority = ANIM_REVERSE;
			ent->s.frame = FRAME_wave08;
			ent->client->anim_end = FRAME_wave01;
		}
		// ### Hentai ### END
	}

	// Altered to use dM_ammoCost
	if (! ((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index]
			-= ent->client->dM_ammoCost;

	ent->client->grenade_time = level.time + 1.0;
}

void Weapon_Grenade (edict_t *ent)
{
	if ((ent->client->newweapon) && (ent->client->weaponstate == WEAPON_READY))
	{
		ChangeWeapon (ent);
		return;
	}

	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{
		ent->client->weaponstate = WEAPON_READY;
		ent->client->ps.gunframe = 16;
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if (((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK))
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;
			if (ent->client->pers.inventory[ent->client->ammo_index]
				>= ent->client->dM_ammoCost)
			{
				// Start throwing the grenade.
				ent->client->ps.gunframe = 1;
				ent->client->weaponstate = WEAPON_FIRING;
				ent->client->grenade_time = 0;
			}
			else
			{
				// No grenades, switch to something else.
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange (ent);
			}
			return;
		}

		if ((ent->client->ps.gunframe == 29) || (ent->client->ps.gunframe == 34) || (ent->client->ps.gunframe == 39) || (ent->client->ps.gunframe == 48))
		{
			if (rand()&15)
				return;
		}

		if (++ent->client->ps.gunframe > 48)
			ent->client->ps.gunframe = 16;
		return;
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		if (ent->client->ps.gunframe == 5)
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/hgrena1b.wav"), 1, ATTN_NORM, 0);

		if (ent->client->ps.gunframe == 11)
		{
			if (!ent->client->grenade_time)
			{
				ent->client->grenade_time = level.time + GRENADE_TIMER + 0.2;
				ent->client->weapon_sound = gi.soundindex("weapons/hgrenc1b.wav");
			}

			// they waited too long, detonate it in their hand
			if (!ent->client->grenade_blew_up && level.time >= ent->client->grenade_time)
			{
				ent->client->weapon_sound = 0;
				weapon_grenade_fire (ent, true);
				ent->client->grenade_blew_up = true;
			}

			if (ent->client->buttons & BUTTON_ATTACK)
				return;

			if (ent->client->grenade_blew_up)
			{
				if (level.time >= ent->client->grenade_time)
				{
					ent->client->ps.gunframe = 15;
					ent->client->grenade_blew_up = false;
				}
				else
				{
					return;
				}
			}
		}

		if (ent->client->ps.gunframe == 12)
		{
			ent->client->weapon_sound = 0;
			weapon_grenade_fire (ent, false);
		}

		if ((ent->client->ps.gunframe == 15) && (level.time < ent->client->grenade_time))
			return;

		ent->client->ps.gunframe++;

		if (ent->client->ps.gunframe == 16)
		{
			ent->client->grenade_time = 0;
			ent->client->weaponstate = WEAPON_READY;
		}
	}
}

void Use_GrenadeWeapon (edict_t *ent, gitem_t *item)
{
	// First, see if we can use this special grenade type.
	Use_Weapon (ent, item);

	// Set it up for use.  (Let them select grenades even if they can't use
	// them.  They'll NoAmmoWeaponChange() immediately.)

	// Make them use the grenades.
	if (item != &gI_ammo_grenades)
		ent->client->newweapon = &gI_ammo_grenades;

	// Set the type of grenade to use.
	if (item == &gI_weapon_clustergrenade)
		ent->client->dM_grenade = 1;
	else if (item == &gI_weapon_railbomb)
		ent->client->dM_grenade = 2;
	else if (item == &gI_weapon_plasmagrenade)
		ent->client->dM_grenade = 3;
	else if (item == &gI_weapon_napalmgrenade)
		ent->client->dM_grenade = 4;
	else if (item == &gI_weapon_shrapnelgrenade)
		ent->client->dM_grenade = 5;
	else if (item == &gI_weapon_cataclysm)
		ent->client->dM_grenade = 6;

	// If we get lost, recover.
	else
	{
		ent->client->dM_grenade = 0;
		ent->client->dM_ammoCost = 1;
	}

	// Set the ammo usage.
	if (ent->client->dM_grenade != 0)
		ent->client->dM_ammoCost = item->quantity;
}

/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

void weapon_grenadelauncher_fire (edict_t *ent)
{
	vec3_t	offset;
	vec3_t	forward, right;
	vec3_t	start;
	int		damage = 120;
	float	radius;

	if (ent->client->pers.inventory[ent->client->ammo_index]
		< ent->client->dM_ammoCost)
	{
		// No grenades, switch to something else.
		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1;
		}
		NoAmmoWeaponChange (ent);
		ent->client->ps.gunframe = /* FRAME_IDLE_FIRST */ 17;
		return;
	}

	radius = damage+40;
	if (is_quad)
		damage *= 4;

	VectorSet(offset, 8, 8, ent->viewheight-8);
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	if (ent->client->dM_grenade == 0)
		fire_flamegrenade (ent, start, forward, damage, 600, 2.5, radius);
	else
		fire_grenade_dM (ent, start, forward, damage, 600, 2.5, radius,
			ent->client->dM_grenade, /* held */ false, /* bazookad */ false);

	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_GRENADE | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index]
			-= ent->client->dM_ammoCost;
}

void Weapon_GrenadeLauncher (edict_t *ent)
{
	static int	pause_frames[]	= {34, 51, 59, 0};
	static int	fire_frames[]	= {6, 0};

	Weapon_Generic (ent, 5, 16, 59, 64, pause_frames, fire_frames, weapon_grenadelauncher_fire);
}

void weapon_bazooka_fire (edict_t *ent)
{
	vec3_t	offset;
	vec3_t	forward, right;
	vec3_t	start;
	int		damage = 120;
	float	radius;

	if (ent->client->pers.inventory[ent->client->ammo_index]
		< ent->client->dM_ammoCost)
	{
		// No grenades, switch to something else.
		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1;
		}
		NoAmmoWeaponChange (ent);
		ent->client->ps.gunframe = /* FRAME_IDLE_FIRST */ 17;
		return;
	}

	radius = damage+40;
	if (is_quad)
		damage *= 4;

	VectorSet(offset, 8, 8, ent->viewheight-8);
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	fire_grenade_dM (ent, start, forward, damage, 500, 2.5, radius,
		ent->client->dM_grenade, /* held */ false, /* bazookad */ true);

	if (!is_silenced)
	{
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_ROCKET | is_silenced);
		gi.multicast (ent->s.origin, MULTICAST_PVS);
	}
	else
	{
		gi.WriteByte (svc_muzzleflash2);
		gi.WriteShort (ent - g_edicts);
		gi.WriteByte (MZ2_CHICK_ROCKET_1);
		gi.multicast (start, MULTICAST_PVS);
	}

	ent->client->ps.gunframe++;

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index]
			-= ent->client->dM_ammoCost;
}

void Weapon_Bazooka (edict_t *ent)
{
	static int	pause_frames[]	= {34, 51, 59, 0};
	static int	fire_frames[]	= {6, 0};

	Weapon_Generic (ent, 5, 16, 59, 64, pause_frames, fire_frames, weapon_bazooka_fire);
}

/*
======================================================================

ROCKET

======================================================================
*/

void Weapon_RocketLauncher_Fire (edict_t *ent)
{
	vec3_t	offset, start;
	vec3_t	forward, right;
	int		damage;
	float	damage_radius;
	int		radius_damage;

	damage = 100 + (int)(random() * 20.0);
	radius_damage = 120;
	damage_radius = 120;
	if (is_quad)
	{
		damage *= 4;
		radius_damage *= 4;
	}

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	VectorSet(offset, 8, 8, ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	fire_flamerocket (ent, start, forward, damage, 650, damage_radius, radius_damage);

	// send muzzle flash
	if (is_silenced)
	{
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_ROCKET | is_silenced);
		gi.multicast (ent->s.origin, MULTICAST_PVS);
	}
	else
	{
		gi.WriteByte (svc_muzzleflash2);
		gi.WriteShort (ent - g_edicts);
		gi.WriteByte (MZ2_CHICK_ROCKET_1);
		gi.multicast (start, MULTICAST_PVS);
	}

	ent->client->ps.gunframe++;

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index]
			-= ent->client->pers.weapon->quantity;
}

void Weapon_RocketLauncher (edict_t *ent)
{
	static int	pause_frames[]	= {25, 33, 42, 50, 0};
	static int	fire_frames[]	= {5, 0};

	Weapon_Generic (ent, 4, 12, 50, 54, pause_frames, fire_frames, Weapon_RocketLauncher_Fire);
}


void Weapon_GuidedRocket_Fire (edict_t *ent)
{
	vec3_t	offset, start;
	vec3_t	forward, right;
	int		damage;
	float	damage_radius;
	int		radius_damage;

	damage = 100 + (int)(random() * 20.0);
	radius_damage = 120;
	damage_radius = 120;
	if (is_quad)
	{
		damage *= 4;
		radius_damage *= 4;
	}

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	VectorSet(offset, 8, 8, ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	fire_guidedrocket (ent, start, forward, damage, 650, damage_radius, radius_damage);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_ROCKET | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index]
			-= ent->client->pers.weapon->quantity;
}

void Weapon_GuidedRocketLauncher (edict_t *ent)
{
	static int	pause_frames[]	= {25, 33, 42, 50, 0};
	static int	fire_frames[]	= {5, 0};

	Weapon_Generic (ent, 4, 12, 50, 54, pause_frames, fire_frames, Weapon_GuidedRocket_Fire);
}

/*
======================================================================

BLASTER / HYPERBLASTER

======================================================================
*/

void Blaster_Fire (edict_t *ent, vec3_t g_offset, int damage, qboolean hyper, int effect)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;

	if (is_quad)
		damage *= 4;
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 24, 8, ent->viewheight-8);
	VectorAdd (offset, g_offset, offset);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	fire_blaster (ent, start, forward, damage, 1000, effect /* , hyper */);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	if (hyper)
		gi.WriteByte (MZ_HYPERBLASTER | is_silenced);
	else
		gi.WriteByte (MZ_BLASTER | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);
}


void Weapon_Blaster_Fire (edict_t *ent)
{
	int		damage;

	if (deathmatch->value)
		damage = 15;
	else
		damage = 10;
	Blaster_Fire (ent, vec3_origin, damage, false, EF_BLASTER);

	// ### Hentai ### BEGIN

	ent->client->anim_priority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crattak1 - 1;
		ent->client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent->s.frame = FRAME_attack1 - 1;
		ent->client->anim_end = FRAME_attack8;
	}

	// ### Hentai ### END

	ent->client->ps.gunframe++;
}

void Weapon_Blaster (edict_t *ent)
{
	static int	pause_frames[]	= {25, 33, 42, 50, 0};
	static int	fire_frames[]	= {5, 0};

	Weapon_Generic (ent, 4, 6, 52, 55, pause_frames, fire_frames, Weapon_Blaster_Fire);
}
//sb
void Super_Fire (edict_t *ent, vec3_t g_offset, int damage, qboolean hyper, int effect)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;

	if (is_quad)
		damage *= 4;
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 24, 8, ent->viewheight-8);
	VectorAdd (offset, g_offset, offset);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	fire_super (ent, start, forward, damage, 1000, effect /* , hyper */);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	if (hyper)
		gi.WriteByte (MZ_HYPERBLASTER | is_silenced);
	else
		gi.WriteByte (MZ_BLASTER | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		ent->client->pers.inventory[ent->client->ammo_index]
			-= ent->client->pers.weapon->quantity;
}


void Weapon_Super_Fire (edict_t *ent)
{
	int		damage;

	if (deathmatch->value)
		damage = 50;
	else
		damage = 150;
	Super_Fire (ent, vec3_origin, damage, false, EF_BLASTER);
	ent->client->ps.gunframe++;
}

void Weapon_SuperBlaster (edict_t *ent)
{
	static int	pause_frames[]	= {25, 33, 42, 50, 0};
	static int	fire_frames[]	= {5, 0};

	Weapon_Generic (ent, 4, 8, 52, 55, pause_frames, fire_frames, Weapon_Super_Fire);
}
// sb end


void Weapon_HyperBlaster_Fire (edict_t *ent)
{
	float	rotation;
	vec3_t	offset;
	int		effect;
	int		damage;

	ent->client->weapon_sound = gi.soundindex("weapons/hyprbl1a.wav");

	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->ps.gunframe++;
	}
	else
	{
		if (! ent->client->pers.inventory[ent->client->ammo_index] )
		{
			if (level.time >= ent->pain_debounce_time)
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
				ent->pain_debounce_time = level.time + 1;
			}
			NoAmmoWeaponChange (ent);
		}
		else
		{
			rotation = (ent->client->ps.gunframe - 5) * 2*M_PI/6;
			offset[0] = -4 * sin(rotation);
			offset[1] = 0;
			offset[2] = 4 * cos(rotation);

			// Don't give every shot a yellow glow, otherwise the client will
			// get too much rendering lag.
			if ((ent->client->ps.gunframe == 6) || (ent->client->ps.gunframe == 9))
				effect = EF_HYPERBLASTER;
			else
				effect = 0;

			if (deathmatch->value)
				damage = 15;
			else
				damage = 20;

			Blaster_Fire (ent, offset, damage, true, effect);

			if (!ent->deadflag && ent->s.modelindex == 255)
			{
				// ### Hentai ### BEGIN

				ent->client->anim_priority = ANIM_ATTACK;
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				{
					ent->s.frame = FRAME_crattak1 - 1;
					ent->client->anim_end = FRAME_crattak9;
				}
				else
				{
					ent->s.frame = FRAME_attack1 - 1;
					ent->client->anim_end = FRAME_attack8;
				}
		
				// ### Hentai ### END
			}

			if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
				ent->client->pers.inventory[ent->client->ammo_index]
					-= ent->client->pers.weapon->quantity;
		}

		ent->client->ps.gunframe++;
		if (ent->client->ps.gunframe == 12 && ent->client->pers.inventory[ent->client->ammo_index])
			ent->client->ps.gunframe = 6;
	}

	if (ent->client->ps.gunframe == 12)
	{
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/hyprbd1a.wav"), 1, ATTN_NORM, 0);
		ent->client->weapon_sound = 0;
	}
}

void Weapon_HyperBlaster (edict_t *ent)
{
	static int	pause_frames[]	= {0};
	static int	fire_frames[]	= {6, 7, 8, 9, 10, 11, 0};

	Weapon_Generic (ent, 5, 20, 49, 53, pause_frames, fire_frames, Weapon_HyperBlaster_Fire);
}
/*
======================================================================

MACHINEGUN / CHAINGUN

======================================================================
*/
void Machine_Fire (edict_t *ent)
{
	int	i;
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		angles;
	int			damage = 8 + (int)(random() * 5.0);
	int			kick = 2 + (int)(random() * 3.0);
	vec3_t		offset;
	int			mod;

	if (!(ent->client->buttons & BUTTON_ATTACK) &&
	((ent->client->burstfire_count > 2) ||
	(!ent->client->burstfire_count)))
	{ 
		ent->client->machinegun_shots=0;
		ent->client->burstfire_count=0;
		ent->client->ps.gunframe++;
		return;
	}

	if (ent->client->burstfire_count < 3)
	{
		if (ent->client->ps.gunframe == 5)
			ent->client->ps.gunframe = 4;
		else
			ent->client->ps.gunframe = 5;
	}
	if (ent->client->pers.inventory[ent->client->ammo_index] < 1)
	{
		ent->client->ps.gunframe = 6;
		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1;
		}
		ent->client->burstfire_count=0;

		NoAmmoWeaponChange (ent);
		return;
	}

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	for (i=1 ; i<3 ; i++)
	{
		ent->client->kick_origin[i] = crandom() * 0.35;
		ent->client->kick_angles[i] = crandom() * 0.7;
	}
	/* ent->client->kick_origin[0] = crandom() * 0.35;
	ent->client->kick_angles[0] = ent->client->machinegun_shots * -1.5; */

	// raise the gun as it is firing
	if (!deathmatch->value && !ent->client->pers.fire_mode)
	{
		ent->client->machinegun_shots++;
		if (ent->client->machinegun_shots > 9)
			ent->client->machinegun_shots = 9;
	}

	// get start / end positions
	VectorAdd (ent->client->v_angle, ent->client->kick_angles, angles);
	AngleVectors (angles, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	switch (ent->client->pers.fire_mode) 
	{ 
		// Fire burst
		case 1:
			// Set up the means of death.
			mod = MOD_MACHINE;
			if ((int)fragban->value & WB_BURSTMACHINEGUN)
				mod |= MOD_NOFRAG;

			ent->client->burstfire_count++;
			if (ent->client->burstfire_count < 4)
			{
				fire_bullet (ent, start, forward, damage*4, kick/2,
					DEFAULT_BULLET_HSPREAD/2, DEFAULT_BULLET_VSPREAD/2, mod);
				gi.WriteByte (svc_muzzleflash);
				gi.WriteShort (ent-g_edicts);
				gi.WriteByte (MZ_MACHINEGUN | is_silenced);
				gi.multicast (ent->s.origin, MULTICAST_PVS);
				PlayerNoise(ent, start, PNOISE_WEAPON);
				if (! ((int)dmflags->value & DF_INFINITE_AMMO))
					ent->client->pers.inventory[ent->client->ammo_index] -= 2;
			} 
			else if (ent->client->burstfire_count > 6) 
				ent->client->burstfire_count=0;
			break;

		// Fire Fully Automatic
		case 0:
		default:
			// Set up the means of death.
			mod = MOD_MACHINE;
			if ((int)fragban->value & WB_MACHINEGUN)
				mod |= MOD_NOFRAG;

			fire_bullet (ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD,
				DEFAULT_BULLET_VSPREAD, mod);
			gi.WriteByte (svc_muzzleflash);
			gi.WriteShort (ent-g_edicts);
			gi.WriteByte (MZ_MACHINEGUN | is_silenced);
			gi.multicast (ent->s.origin, MULTICAST_PVS);
			PlayerNoise(ent, start, PNOISE_WEAPON);
			if (! ((int)dmflags->value & DF_INFINITE_AMMO))
				ent->client->pers.inventory[ent->client->ammo_index]
					-= ent->client->pers.weapon->quantity;
			break;
	}

	if (!ent->deadflag && ent->s.modelindex == 255)
	{
		// ### Hentai ### BEGIN

		ent->client->anim_priority = ANIM_ATTACK;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			ent->s.frame = FRAME_crattak1 - (int) (random()+0.25);
			ent->client->anim_end = FRAME_crattak9;
		}
		else
		{
			ent->s.frame = FRAME_attack1 - (int) (random()+0.25);
			ent->client->anim_end = FRAME_attack8;
		}

		// ### Hentai ### END
	}
}

void Weapon_Machine (edict_t *ent)
{
	static int	pause_frames[]	= {23, 45, 0};
	static int	fire_frames[]	= {4, 5, 0};

	Weapon_Generic (ent, 3, 5, 45, 49, pause_frames, fire_frames, Machine_Fire);
}


void Machinegun_Fire (edict_t *ent)
{
	vec3_t	offset, start;
	vec3_t	forward, right;
	int		damage;
	float	damage_radius;
	int		radius_damage;

	damage = 15 + (int)(random() * 15.0);
	radius_damage = 15 + (int)(random() * 10.0);
	damage_radius = 20;
	if (is_quad)
	{
		damage *= 4;
		radius_damage *= 4;
	}

	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->machinegun_shots = 0;
		ent->client->ps.gunframe++;
		return;
	}

	if (ent->client->ps.gunframe == 5)
		ent->client->ps.gunframe = 4;
	else
		ent->client->ps.gunframe = 5;

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	VectorSet(offset, 8, 8, ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	fire_mr (ent, start, forward, damage, 1000, damage_radius, radius_damage);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_CHAINGUN1 | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!ent->deadflag && ent->s.modelindex == 255)
	{
		// ### Hentai ### BEGIN

		ent->client->anim_priority = ANIM_ATTACK;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			ent->s.frame = FRAME_crattak1 - (int) (random()+0.25);
			ent->client->anim_end = FRAME_crattak9;
		}
		else
		{
			ent->s.frame = FRAME_attack1 - (int) (random()+0.25);
			ent->client->anim_end = FRAME_attack8;
		}

		// ### Hentai ### END
	}

	if (! ((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index]
			-= ent->client->pers.weapon->quantity;
}

void Weapon_Machinegun (edict_t *ent)
{
	static int	pause_frames[]	= {23, 45, 0};
	static int	fire_frames[]	= {4, 5, 0};

	Weapon_Generic (ent, 3, 4, 45, 49, pause_frames, fire_frames, Machinegun_Fire);
}

void Chaingun_Fire (edict_t *ent)
{
	int			i;
	int			shots;
	vec3_t		start;
	vec3_t		forward, right, up;
	float		r, u;
	vec3_t		offset;
	int			damage;
	int			kick = 2;
	int			mod;

	if (deathmatch->value)
		damage = 6;
	else
		damage = 8;

	if (ent->client->ps.gunframe == 5)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);

	if ((ent->client->ps.gunframe == 14) && !(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->ps.gunframe = 32;
		ent->client->weapon_sound = 0;
		return;
	}
	else if ((ent->client->ps.gunframe == 21) && (ent->client->buttons & BUTTON_ATTACK)
		&& ent->client->pers.inventory[ent->client->ammo_index])
	{
		ent->client->ps.gunframe = 15;
	}
	else
	{
		ent->client->ps.gunframe++;
	}

	if (ent->client->ps.gunframe == 22)
	{
		ent->client->weapon_sound = 0;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnd1a.wav"), 1, ATTN_IDLE, 0);
	}
	else
	{
		ent->client->weapon_sound = gi.soundindex("weapons/chngnl1a.wav");
	}

	if (!ent->deadflag && ent->s.modelindex == 255)
	{
		// ### Hentai ### BEGIN

		ent->client->anim_priority = ANIM_ATTACK;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			ent->s.frame = FRAME_crattak1 - (ent->client->ps.gunframe & 1);
			ent->client->anim_end = FRAME_crattak9;
		}
		else
		{
			ent->s.frame = FRAME_attack1 - (ent->client->ps.gunframe & 1);
			ent->client->anim_end = FRAME_attack8;
		}

		// ### Hentai ### END
	}

	if (ent->client->ps.gunframe <= 9)
		shots = 1;
	else if (ent->client->ps.gunframe <= 14)
	{
		if (ent->client->buttons & BUTTON_ATTACK)
			shots = 2;
		else
			shots = 1;
	}
	else
		shots = 3;

	if (ent->client->pers.inventory[ent->client->ammo_index] < shots)
		shots = ent->client->pers.inventory[ent->client->ammo_index];

	if (!shots)
	{
		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1;
		}
		NoAmmoWeaponChange (ent);
		return;
	}

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	for (i=0 ; i<3 ; i++)
	{
		ent->client->kick_origin[i] = crandom() * 0.35;
		ent->client->kick_angles[i] = crandom() * 0.7;
	}

	// Set up the means of death.
	mod = MOD_CHAINGUN;
	if ((int)fragban->value & WB_CHAINGUN)
		mod |= MOD_NOFRAG;

	for (i=0 ; i<shots ; i++)
	{
		// get start / end positions
		AngleVectors (ent->client->v_angle, forward, right, up);
		r = 7 + crandom()*4;
		u = crandom()*4;
		VectorSet(offset, 0, r, u + ent->viewheight-8);
		P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

		fire_bullet (ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD,
			DEFAULT_BULLET_VSPREAD, mod);
	}

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte ((MZ_CHAINGUN1 + shots - 1) | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!ent->deadflag && ent->s.modelindex == 255)
	{
		// ### Hentai ### BEGIN

		ent->client->anim_priority = ANIM_ATTACK;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			ent->s.frame = FRAME_crattak1 - (int) (random()+0.25);
			ent->client->anim_end = FRAME_crattak9;
		}
		else
		{
			ent->s.frame = FRAME_attack1 - (int) (random()+0.25);
			ent->client->anim_end = FRAME_attack8;
		}

		// ### Hentai ### END
	}

	if (! ((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index] -= shots;
}


void Weapon_Chaingun (edict_t *ent)
{
	static int	pause_frames[]	= {38, 43, 51, 61, 0};
	static int	fire_frames[]	= {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 0};

	Weapon_Generic (ent, 4, 31, 61, 64, pause_frames, fire_frames, Chaingun_Fire);
}


void weapon_streetsweeper_fire (edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	int			damage = 4;
	int			kick = 8;
	int			mod;

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -2;

	VectorSet(offset, 0, 8,  ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	if (!(ent->client->pers.inventory[ent->client->ammo_index]))
	{
		// Stop firing.
		ent->client->ps.gunframe = 22;
		ent->client->weapon_sound = 0;

		// Make the streetsweeper-noammo sound.
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/noammo.wav"), 1, ATTN_IDLE, 0);

		// Switch to some other weapon.
		NoAmmoWeaponChange (ent);

		return;
	}
	else if (ent->client->weaponstate == WEAPON_FIRING
	&& ent->client->ps.gunframe == 21
	&& (ent->client->buttons & BUTTON_ATTACK)
	&& ent->client->pers.inventory[ent->client->ammo_index])
	{
		// Keep firing.
		ent->client->ps.gunframe = 15;
	}
	else if (ent->client->weaponstate == WEAPON_FIRING
	&& ent->client->ps.gunframe >= 5
	&& ent->client->ps.gunframe <= 21
	&& !(ent->client->buttons & BUTTON_ATTACK))
	{
		// Stop firing.
		ent->client->ps.gunframe = 22;
		ent->client->weapon_sound = 0;

		// Make the streetsweeper-winddown sound.
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnd1a.wav"), 1, ATTN_IDLE, 0);
	}
	else
	{
		if (ent->client->ps.gunframe == 5)
		{
			// Start firing.

			// Make the streetsweeper-windup sound.
			gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);
		}
		else if (ent->client->ps.gunframe >= 15
		&& ent->client->ps.gunframe <= 21)
		{
			// Now that the wind-up sound has finished, play the "during" sound
			// continuously until they stop firing.
			ent->client->weapon_sound = gi.soundindex("weapons/chngnl1a.wav");
		}

		ent->client->ps.gunframe++;
	}

	if (!ent->deadflag && ent->s.modelindex == 255)
	{
		// ### Hentai ### BEGIN

		ent->client->anim_priority = ANIM_ATTACK;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			ent->s.frame = FRAME_crattak1 - (ent->client->ps.gunframe & 1);
			ent->client->anim_end = FRAME_crattak9;
		}
		else
		{
			ent->s.frame = FRAME_attack1 - (ent->client->ps.gunframe & 1);
			ent->client->anim_end = FRAME_attack8;
		}

		// ### Hentai ### END
	}

	// Set up the means of death.
	mod = MOD_STREETSWEEP;
	if ((int)fragban->value & WB_STREETSWEEPER)
		mod |= MOD_NOFRAG;

	if (deathmatch->value)
		fire_shotgun (ent, start, forward, damage, kick, 500, 500,
			DEFAULT_STREETSWEEPER_COUNT, mod);
	else
		fire_shotgun (ent, start, forward, damage, kick, 500, 500,
			DEFAULT_STREETSWEEPER_COUNT, mod);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_SHOTGUN | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!ent->deadflag && ent->s.modelindex == 255)
	{
		// ### Hentai ### BEGIN

		ent->client->anim_priority = ANIM_ATTACK;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			ent->s.frame = FRAME_crattak1 - (int) (random()+0.25);
			ent->client->anim_end = FRAME_crattak9;
		}
		else
		{
			ent->s.frame = FRAME_attack1 - (int) (random()+0.25);
			ent->client->anim_end = FRAME_attack8;
		}

		// ### Hentai ### END
	}

	if (! ((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index]
			-= ent->client->pers.weapon->quantity;
}

void Weapon_Streetsweeper (edict_t *ent)
{
	static int	pause_frames[]	= {38, 43, 51, 61, 0};
	static int	fire_frames[]	= {5, 7, 9, 11, 13, 15, 17, 19, 21, 0};

	// Hacked-up chaingun spec
	Weapon_Generic (ent, 4, 31, 61, 64, pause_frames, fire_frames,
		weapon_streetsweeper_fire);
}


/*
======================================================================

SHOTGUN / SUPERSHOTGUN

======================================================================
*/

void weapon_shotgun_fire (edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	int			damage = 4;
	int			kick = 8;
	int			mod;

	if (ent->client->ps.gunframe == 9)
	{
		ent->client->ps.gunframe++;
		return;
	}

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -2;

	VectorSet(offset, 0, 8,  ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	// Set up the means of death.
	mod = MOD_SHOTGUN;
	if ((int)fragban->value & WB_SHOTGUN)
		mod |= MOD_NOFRAG;

	if (deathmatch->value)
		fire_shotgun (ent, start, forward, damage, kick, 200, 200,
			DEFAULT_DEATHMATCH_SHOTGUN_COUNT, mod);
	else
		fire_shotgun (ent, start, forward, damage, kick, 200, 200,
			DEFAULT_SHOTGUN_COUNT, mod);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_SHOTGUN | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index]
			-= ent->client->pers.weapon->quantity;
}

void Weapon_Shotgun (edict_t *ent)
{
	static int	pause_frames[]	= {22, 28, 34, 0};
	static int	fire_frames[]	= {8, 9, 0};

	Weapon_Generic (ent, 7, 18, 36, 39, pause_frames, fire_frames, weapon_shotgun_fire);
}


void weapon_sawedoffshotgun_fire (edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	int			damage = 4;
	int			kick = 8;
	int			mod;

	if (ent->client->pers.inventory[ent->client->ammo_index]
		< ent->client->pers.weapon->quantity)
	{
		ent->client->ps.gunframe++;
		return;
	}

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -2;

	VectorSet(offset, 0, 8,  ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	// Set up the means of death.
	mod = MOD_SHOTGUN;
	if ((int)fragban->value & WB_SHOTGUN)
		mod |= MOD_NOFRAG;

	if (deathmatch->value)
		fire_shotgun (ent, start, forward, damage, kick, 250, 250,
			DEFAULT_DEATHMATCH_SHOTGUN_COUNT, mod);
	else
		fire_shotgun (ent, start, forward, damage, kick, 250, 250,
			DEFAULT_SHOTGUN_COUNT, mod);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_SHOTGUN | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index]
			-= ent->client->pers.weapon->quantity;
}

void Weapon_SawedOffShotgun (edict_t *ent)
{
	static int	pause_frames[]	= {22, 28, 34, 0};
	static int	fire_frames[]	= {8, 9, 0};

	Weapon_Generic (ent, 7, 18, 36, 39, pause_frames, fire_frames, weapon_sawedoffshotgun_fire);
}


void weapon_supershotgun_fire (edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	vec3_t		v;
	int			damage = 6;
	int			kick = 100;
	int			mod;

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -2;

	VectorSet(offset, 0, 8,  ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	// Set up the means of death.
	mod = MOD_SSHOTGUN;
	if ((int)fragban->value & WB_SUPERSHOTGUN)
		mod |= MOD_NOFRAG;

	v[PITCH] = ent->client->v_angle[PITCH];
	v[YAW]   = ent->client->v_angle[YAW] - 5;
	v[ROLL]  = ent->client->v_angle[ROLL];
	AngleVectors (v, forward, NULL, NULL);
	fire_shotgun (ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD,
		DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT/2, mod);
	v[YAW]   = ent->client->v_angle[YAW] + 5;
	AngleVectors (v, forward, NULL, NULL);
	fire_shotgun (ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD,
		DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT/2, mod);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_SSHOTGUN | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index]
			-= ent->client->pers.weapon->quantity;
}

void Weapon_SuperShotgun (edict_t *ent)
{
	static int	pause_frames[]	= {29, 42, 57, 0};
	static int	fire_frames[]	= {7, 0};

	Weapon_Generic (ent, 6, 17, 57, 61, pause_frames, fire_frames, weapon_supershotgun_fire);
}


/*
======================================================================

SNIPER GUN

======================================================================
*/

void Sniper_Fire (edict_t *ent, vec3_t g_offset, int damage, qboolean hyper, int effect)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;

	if (is_quad)
		damage *= 4;
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 24, 8, ent->viewheight-8);
	VectorAdd (offset, g_offset, offset);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	fire_sniper (ent, start, forward, damage, 2500, effect);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	if (hyper)
		gi.WriteByte (MZ_HYPERBLASTER | is_silenced);
	else
		gi.WriteByte (MZ_RAILGUN | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);
}


void Weapon_Sniper_Fire (edict_t *ent)
{
	int		damage;

	if (deathmatch->value)
		damage = 300;
	else
		damage = 600;
	Sniper_Fire (ent, vec3_origin, damage, false, EF_BLASTER);
	ent->client->ps.gunframe++;
	if (! ((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index]
			-= ent->client->pers.weapon->quantity;
}

void Weapon_Sniper (edict_t *ent)
{
	static int	pause_frames[]	= {25, 33, 42, 50, 0};
	static int	fire_frames[]	= {5, 0};

	Weapon_Generic (ent, 4, 29, 52, 55, pause_frames, fire_frames, Weapon_Sniper_Fire);
}


/*
======================================================================

PLASMA RIFLE

======================================================================
*/

void Plasma_Fire (edict_t *ent, vec3_t g_offset, int damage,
						qboolean hyper, int effect)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;

	if (is_quad)
		damage *= 4;
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 24, 8, ent->viewheight-8);
	VectorAdd (offset, g_offset, offset);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

#if 0
	fire_plasma (ent, start, forward, damage, 1200, effect);
#else
	{
		void fire_bolt (edict_t *ent, vec3_t start, vec3_t forward, int damage);
		fire_bolt (ent, start, forward, damage);
	}
#endif

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	if (hyper)
		gi.WriteByte (MZ_HYPERBLASTER | is_silenced);
	else
		gi.WriteByte (MZ_BLASTER | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);
}


void Weapon_Plasma_Fire (edict_t *ent)
{
	int		damage;

	if (deathmatch->value)
		damage = 15;
	else
		damage = 20;
	Plasma_Fire (ent, vec3_origin, damage, false, EF_BFG);
	ent->client->ps.gunframe++;
	if (! ((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index]
			-= ent->client->pers.weapon->quantity;
}

#if 0
void Weapon_Plasma (edict_t *ent)
{
	static int	pause_frames[]	= {25, 33, 42, 50, 0};
	static int	fire_frames[]	= {6, 7, 0};

	// As long as they're holding down the fire button, fire.
	// (That's why we fire in frame 7; to keep it from going idle.)
	if (ent->client->weaponstate == WEAPON_FIRING
	&& ent->client->ps.gunframe == 7)
	{
		if (!ent->client->newweapon
		&& (ent->client->buttons & BUTTON_ATTACK)
		&& ent->client->pers.inventory[ITEM_INDEX(gI_weapon_plasma.ammo)]
			>= gI_weapon_plasma.quantity)
		{
			// Turn back the clock.
			ent->client->ps.gunframe = 6;
		}
		else
		{
			// Move the clock forward.
			ent->client->ps.gunframe = 8;
		}
	}

	// Then act like a normal weapon.
	Weapon_Generic (ent, 5, 7, 59, 64, pause_frames, fire_frames,
		Weapon_Plasma_Fire);
}
#else
void Weapon_Plasma (edict_t *ent)
{
	static int	pause_frames[]	= {0};
	static int	fire_frames[]	= {6, 7, 8, 9, 10, 11, 0};

	// As long as they're holding down the fire button, fire.
	// (That's why we fire in frame 7; to keep it from going idle.)
	if (ent->client->weaponstate == WEAPON_FIRING
	&& ent->client->ps.gunframe == 7)
	{
		if (!ent->client->newweapon
		&& (ent->client->buttons & BUTTON_ATTACK)
		&& ent->client->pers.inventory[ITEM_INDEX(gI_weapon_plasma.ammo)]
			>= gI_weapon_plasma.quantity)
		{
			// Turn back the clock.
			ent->client->ps.gunframe = 6;
		}
		else
		{
			// Move the clock forward.
			ent->client->ps.gunframe = 12;
		}
	}

	// Then act like a normal weapon.
	Weapon_Generic (ent, 5, 20, 49, 53, pause_frames, fire_frames,
		Weapon_Plasma_Fire);
}
#endif


/*
======================================================================

FLAMETHROWER (used to be the railgun)

======================================================================
*/

void fire_rg (edict_t *ent, vec3_t g_offset, int damage, qboolean hyper,
				  int effect)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;
	vec3_t	direct_damage = {6, 9, 25};
	vec3_t	radius_damage = {6, 4, 25};

	if (is_quad)
		damage *= 4;
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 24, 8, ent->viewheight-8);
	VectorAdd (offset, g_offset, offset);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	// ow, my flesh, its burning!
	PBM_FireFlamer (ent, start, forward, 1200, 70, direct_damage,
		radius_damage, 100, 50);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	if (hyper)
		gi.WriteByte (MZ_HYPERBLASTER | is_silenced);
	else
		gi.WriteByte (MZ_HYPERBLASTER | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);
}

void weapon_railgun_fire (edict_t *ent)
{
	int		damage;

	if (deathmatch->value)
		damage = 10;
	else
		damage = 20;
	fire_rg (ent, vec3_origin, damage, false, EF_ROCKET);
	ent->client->ps.gunframe++;
	if (! ((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index]
			-= ent->client->pers.weapon->quantity;
}

void Weapon_Railgun (edict_t *ent)
{
	static int	pause_frames[]	= {34, 51, 59, 0};
	static int	fire_frames[]	= {4, 5, 0};

	Weapon_Generic (ent, 3, 5, 56, 61, pause_frames, fire_frames, weapon_railgun_fire);
}


/*
======================================================================

RAILGUN

======================================================================
*/

void weapon_railgun2_fire (edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	int			damage;
	int			kick;

	// Same "extreme" damage in SP game *and* deathmatch :)
	damage = 150;
	kick = 250;

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -3, ent->client->kick_origin);
	ent->client->kick_angles[0] = -3;

	VectorSet(offset, 0, 7,  ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	fire_rail (ent, start, forward, damage, kick);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_RAILGUN | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index]
			-= ent->client->pers.weapon->quantity;
}


void Weapon_Railgun2 (edict_t *ent)
{
	static int	pause_frames[]	= {56, 0};
	static int	fire_frames[]	= {4, 0};

	Weapon_Generic (ent, 3, 18, 56, 61, pause_frames, fire_frames, weapon_railgun2_fire);
}


/*
======================================================================

BFG10K

======================================================================
*/

void weapon_bfg_fire (edict_t *ent)
{
	vec3_t	offset, start;
	vec3_t	forward, right;
	int		damage;
	float	damage_radius = 1000;

	if (deathmatch->value)
		damage = 200;
	else
		damage = 500;

	if (ent->client->ps.gunframe == 9)
	{
		// send muzzle flash
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_BFG | is_silenced);
		gi.multicast (ent->s.origin, MULTICAST_PVS);

		ent->client->ps.gunframe++;

		PlayerNoise(ent, start, PNOISE_WEAPON);
		return;
	}

	// cells can go down during windup (from power armor hits), so
	// check again and abort firing if we don't have enough now
	if (ent->client->pers.inventory[ent->client->ammo_index] < 50)
	{
		ent->client->ps.gunframe++;
		return;
	}

	if (is_quad)
		damage *= 4;

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -2, ent->client->kick_origin);

	// make a big pitch kick with an inverse fall
	ent->client->v_dmg_pitch = -40;
	ent->client->v_dmg_roll = crandom()*8;
	ent->client->v_dmg_time = level.time + DAMAGE_TIME;

	VectorSet(offset, 8, 8, ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	fire_bfg (ent, start, forward, damage, 400, damage_radius);

	ent->client->ps.gunframe++;

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index]
			-= ent->client->pers.weapon->quantity;
}

void Weapon_BFG (edict_t *ent)
{
	static int	pause_frames[]	= {39, 45, 50, 55, 0};
	static int	fire_frames[]	= {9, 17, 0};

	Weapon_Generic (ent, 8, 32, 55, 58, pause_frames, fire_frames, weapon_bfg_fire);
}


/*
======================================================================

FREEZE GUN

======================================================================
*/

void Freezer_Fire (edict_t *ent, vec3_t g_offset, int damage, qboolean hyper, int effect)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;

	if (is_quad)
		damage *= 4;
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 24, 8, ent->viewheight-8);
	VectorAdd (offset, g_offset, offset);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	// tell it to fire the freezer instead
	fire_freezer (ent, start, forward, damage, 2500, effect);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	if (hyper)
		gi.WriteByte (MZ_HYPERBLASTER | is_silenced);
	else
		gi.WriteByte (MZ_RAILGUN | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);
}

void Weapon_Freezer_Fire (edict_t *ent)
{
	int		damage;

	if (deathmatch->value)
		damage = 20;
	else
		damage = 15;
	Freezer_Fire (ent, vec3_origin, damage, false, EF_BLASTER);
	ent->client->ps.gunframe++;
	if (! ((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index]
			-= ent->client->pers.weapon->quantity;
}

/* void Weapon_Freezer (edict_t *ent)
{
	static int	pause_frames[]	= {19, 32, 0};
	static int	fire_frames[]	= {5, 0};

	Weapon_Generic (ent, 3, 18, 56, 61, pause_frames, fire_frames, Weapon_Freezer_Fire);
} */
void Weapon_Freezer (edict_t *ent)
{
	static int	pause_frames[]	= {29, 42, 57, 0};
	static int	fire_frames[]	= {7, 0};

	Weapon_Generic (ent, 6, 17, 57, 61, pause_frames, fire_frames, Weapon_Freezer_Fire);
}
