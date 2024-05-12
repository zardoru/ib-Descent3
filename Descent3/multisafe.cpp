/*
* Descent 3
* Copyright (C) 2024 Parallax Software
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "multisafe.h"
#include "weather.h"
#include "room.h"
#include "game.h"
#include "multi.h"
#include "damage.h"
#include "hud.h"
#include "doorway.h"
#include "trigger.h"
#include "gamepath.h"
#include "AIGoal.h"
#include "weapon.h"
#include "spew.h"
#include "hlsoundlib.h"
#include "sounds.h"
#include "ship.h"
#include "player.h"
#include "object_lighting.h"
#include "soundload.h"
#include "streamaudio.h"
#include "gamesequence.h"
#include "gameevent.h"
#include "SmallViews.h"
#include "difficulty.h"
#include "door.h"
#include "demofile.h"
#include "stringtable.h"
#include "d3music.h"
#include "multi_world_state.h"
#include "osiris_predefs.h"
#include "viseffect.h"
#include "levelgoal.h"
/*
	The following functions have been added or modified by Matt and/or someone else other than Jason,
	and thus Jason should check them out to make sure they're ok for multiplayer.

*/
// Returns true if the player has this weapon (this fuction should be moved)
bool PlayerHasWeapon(int slot, int weapon_index);
int AddWeaponAmmo(int slot, int weap_index, int ammo);
void CheckForWeaponSelect(int id, int weapon_index);
#define MOBJ	(ObjGet(mstruct->objhandle))
#define KOBJ	(ObjGet(mstruct->killer_handle))
#define IOBJ	(ObjGet(mstruct->ithandle))
//Returns true if the specified room is valid
bool VALIDATE_ROOM(int roomnum)
{
	bool valid = ((roomnum >= 0) && (roomnum <= Highest_room_index) && Rooms[roomnum].used);
	if (!valid)
		Int3();
	return valid;
}
//Returns true if the specified room & face are valid
bool VALIDATE_ROOM_FACE(int roomnum, int facenum)
{
	bool valid = ((roomnum >= 0) && (roomnum <= Highest_room_index) && (Rooms[roomnum].used) &&
		(facenum >= 0) && (facenum < Rooms[roomnum].num_faces));
	if (!valid)
		Int3();
	return valid;
}
//Returns true if the specified room & portal are valid
bool VALIDATE_ROOM_PORTAL(int roomnum, int portalnum)
{
	bool valid = ((roomnum >= 0) && (roomnum <= Highest_room_index) && (Rooms[roomnum].used) &&
		(portalnum >= 0) && (portalnum < Rooms[roomnum].num_portals));
	if (!valid)
		Int3();
	return valid;
}
//Returns an object pointer from a handle.  Really just ObjGet() with an mprintf()
object* VALIDATE_OBJECT(int handle)
{
	object* objp = ObjGet(handle);
	if (!objp)
	{
		mprintf((0, "Invalid object passed to multisafe.\n"));
	}
	return objp;
}


extern bool Hud_show_controls;
// Gets a value for the calling party
void msafe_GetValue(int type, msafe_struct* mstruct)
{
	object* objp;
	switch (type)
	{
	case MSAFE_SHOW_ENABLED_CONTROLS:
		objp = ObjGet(mstruct->objhandle);
		if (objp && (objp->type == OBJ_PLAYER))
			mstruct->state = Hud_show_controls;
		else
			mstruct->state = false;
		break;
		break;
	case MSAFE_OBJECT_PLAYER_CMASK:
		objp = ObjGet(mstruct->objhandle);
		if (objp && (objp->type == OBJ_PLAYER))
			mstruct->control_mask = Players[objp->id].controller_bitflags;
		else
			mstruct->control_mask = 0;
		break;
	case MSAFE_OBJECT_POS:
		objp = ObjGet(mstruct->objhandle);
		if (!objp)
			mstruct->pos = Zero_vector;
		else
			mstruct->pos = objp->pos;
		break;
	case MSAFE_OBJECT_ORIENT:
		objp = ObjGet(mstruct->objhandle);
		if (!objp)
			mstruct->orient = Identity_matrix;
		else
			mstruct->orient = objp->orient;
		break;
	case MSAFE_OBJECT_ROOMNUM:
		objp = ObjGet(mstruct->objhandle);
		if (!objp)
			mstruct->roomnum = -1;
		else
			mstruct->roomnum = objp->roomnum;
		break;
	case MSAFE_OBJECT_WORLD_POSITION:
		objp = ObjGet(mstruct->objhandle);
		if (!objp)
		{
			mstruct->pos = Zero_vector;
			mstruct->orient = Identity_matrix;
			mstruct->roomnum = -1;
		}
		else
		{
			mstruct->orient = objp->orient;
			mstruct->roomnum = objp->roomnum;
			mstruct->pos = objp->pos;
		}
		break;
	case MSAFE_OBJECT_VELOCITY:
		objp = ObjGet(mstruct->objhandle);
		if (!objp || (objp->movement_type != MT_WALKING && objp->movement_type != MT_PHYSICS))
			mstruct->velocity = Zero_vector;
		else
			mstruct->velocity = objp->mtype.phys_info.velocity;
		break;
	case MSAFE_OBJECT_ROTVELOCITY:
		objp = ObjGet(mstruct->objhandle);
		if (!objp || (objp->movement_type != MT_WALKING && objp->movement_type != MT_PHYSICS))
			mstruct->rot_velocity = Zero_vector;
		else
			mstruct->rot_velocity = objp->mtype.phys_info.rotvel;
		break;
	case MSAFE_OBJECT_THRUST:
		objp = ObjGet(mstruct->objhandle);
		if (!objp || (objp->movement_type != MT_WALKING && objp->movement_type != MT_PHYSICS))
			mstruct->thrust = Zero_vector;
		else
			mstruct->thrust = objp->mtype.phys_info.thrust;
		break;
	case MSAFE_OBJECT_ROTTHRUST:
		objp = ObjGet(mstruct->objhandle);
		if (!objp || (objp->movement_type != MT_WALKING && objp->movement_type != MT_PHYSICS))
			mstruct->rot_thrust = Zero_vector;
		else
			mstruct->rot_thrust = objp->mtype.phys_info.rotthrust;
		break;
	case MSAFE_OBJECT_FLAGS:  // Needs to be send
		objp = ObjGet(mstruct->objhandle);
		if (!objp)
			mstruct->flags = 0;
		else
			mstruct->flags = objp->flags;
		break;
	case MSAFE_OBJECT_SIZE:
		objp = ObjGet(mstruct->objhandle);
		if (!objp)
			mstruct->size = 0.0f;
		else
			mstruct->size = objp->size;
		break;
	case MSAFE_OBJECT_CONTROL_TYPE:  // Needs to be send
		objp = ObjGet(mstruct->objhandle);
		if (!objp)
			mstruct->control_type = CT_NONE;
		else
			mstruct->control_type = objp->control_type;
		break;
	case MSAFE_OBJECT_MOVEMENT_TYPE:  // Needs to be send
		objp = ObjGet(mstruct->objhandle);
		if (!objp)
			mstruct->movement_type = MT_NONE;
		else
			mstruct->movement_type = objp->movement_type;
		break;
	case MSAFE_OBJECT_CREATION_TIME:
		objp = ObjGet(mstruct->objhandle);
		if (!objp)
			mstruct->creation_time = 0.0f;
		else
			mstruct->creation_time = objp->creation_time;
		break;
	case MSAFE_OBJECT_PHYSICS_FLAGS:  // Needs to be send
		objp = ObjGet(mstruct->objhandle);
		if (!objp || (objp->movement_type != MT_WALKING && objp->movement_type != MT_PHYSICS))
			mstruct->physics_flags = 0;
		else
			mstruct->physics_flags = objp->mtype.phys_info.flags;
		break;
	case MSAFE_OBJECT_ROTDRAG:
		objp = ObjGet(mstruct->objhandle);
		if (!objp || (objp->movement_type != MT_WALKING && objp->movement_type != MT_PHYSICS))
			mstruct->rot_drag = 1.0f;
		else
			mstruct->rot_drag = objp->mtype.phys_info.rotdrag;
		break;
	case MSAFE_OBJECT_SHIELDS:
		objp = ObjGet(mstruct->objhandle);
		if (!objp)
			mstruct->shields = -1;
		else
			mstruct->shields = objp->shields;
		break;
	case MSAFE_OBJECT_SHIELDS_ORIGINAL:
		objp = ObjGet(mstruct->objhandle);
		if (!objp || !IS_GENERIC(objp->type))
			mstruct->shields = -1;
		else
			mstruct->shields = Object_info[objp->id].hit_points;
		break;
	case MSAFE_OBJECT_ENERGY:
		ASSERT(MOBJ->type == OBJ_PLAYER);
		if (!VALIDATE_OBJECT(mstruct->objhandle) || (MOBJ->type != OBJ_PLAYER))
			mstruct->energy = 0;
		else
			mstruct->energy = Players[MOBJ->id].energy;
		break;
	case MSAFE_OBJECT_MOVEMENT_SCALAR:
		if (!VALIDATE_OBJECT(mstruct->objhandle) || (MOBJ->type != OBJ_PLAYER))
			mstruct->scalar = 0.0;
		else
			mstruct->scalar = Players[MOBJ->id].movement_scalar;
		break;
	case MSAFE_OBJECT_RECHARGE_SCALAR:
		if (!VALIDATE_OBJECT(mstruct->objhandle) || (MOBJ->type != OBJ_PLAYER))
			mstruct->scalar = 0.0;
		else
			mstruct->scalar = Players[MOBJ->id].weapon_recharge_scalar;
		break;
	case MSAFE_OBJECT_WSPEED_SCALAR:
		if (!VALIDATE_OBJECT(mstruct->objhandle) || (MOBJ->type != OBJ_PLAYER))
			mstruct->scalar = 0.0;
		else
			mstruct->scalar = Players[MOBJ->id].weapon_speed_scalar;
		break;
	case MSAFE_OBJECT_ARMOR_SCALAR:
		if (!VALIDATE_OBJECT(mstruct->objhandle) || (MOBJ->type != OBJ_PLAYER))
			mstruct->scalar = 0.0;
		else
			mstruct->scalar = Players[MOBJ->id].armor_scalar;
		break;
	case MSAFE_OBJECT_DAMAGE_SCALAR:
		if (!VALIDATE_OBJECT(mstruct->objhandle) || (MOBJ->type != OBJ_PLAYER))
			mstruct->scalar = 0.0;
		else
			mstruct->scalar = Players[MOBJ->id].damage_scalar;
		break;
	case MSAFE_OBJECT_TYPE:
		objp = ObjGet(mstruct->objhandle);
		if (!objp)
			mstruct->type = -1;
		else
			mstruct->type = objp->type;
		break;
	case MSAFE_OBJECT_ID:
		objp = ObjGet(mstruct->objhandle);
		if (!objp)
			mstruct->id = -1;
		else
			mstruct->id = objp->id;
		break;
	case MSAFE_OBJECT_PARENT:
	{
		object* obj = ObjGet(mstruct->objhandle);
		if (!obj)
			return;
		obj = ObjGetUltimateParent(obj);
		mstruct->objhandle = obj->handle;
		break;
	}
	case MSAFE_OBJECT_ENERGY_WEAPON:
	{
		object* objp = ObjGet(mstruct->objhandle);
		mstruct->state = (objp && (objp->type == OBJ_WEAPON) && !(Weapons[objp->id].flags & WF_MATTER_WEAPON));
	}
	case MSAFE_OBJECT_DAMAGE_AMOUNT:
	{
		mstruct->amount = 0;		//default
		object* objp = ObjGet(mstruct->objhandle);
		if (!objp || (objp->type != OBJ_WEAPON))
			return;
		mstruct->amount = Weapons[objp->id].generic_damage * objp->ctype.laser_info.multiplier;
		break;
	}
	case MSAFE_OBJECT_COUNT_TYPE:
	{
		int i, count = 0;
		for (i = 0, objp = Objects; i <= Highest_object_index; i++, objp++)
		{
			if ((objp->type == mstruct->type) && (objp->id == mstruct->id))
				count++;
		}
		mstruct->count = count;
		break;
	}
	//		case MSAFE_OBJECT_ANIM_FRAME:
	//		{
	//			objp = ObjGet(mstruct->objhandle);
	//			if(!objp || !(objp->flags & OF_POLYGON_OBJECT))
	//				mstruct->anim_frame = 0.0f;
	//			else
	//				mstruct->anim_frame = objp->rtype.pobj_info.anim_frame;
	//			break;
	//		}
	case MSAFE_OBJECT_INVULNERABLE:
	{
		if (!VALIDATE_OBJECT(mstruct->objhandle))
		{
			mstruct->state = 0;
			mstruct->lifetime = 0;
			return;
		}

		if (MOBJ->type == OBJ_PLAYER)
		{
			mstruct->state = (bool)((Players[MOBJ->id].flags & PLAYER_FLAGS_INVULNERABLE) != 0);
			if (mstruct->state)
				mstruct->lifetime = Players[MOBJ->id].invulnerable_time;
			else
				mstruct->lifetime = 0;
		}
		else
		{
			mstruct->state = (bool)((MOBJ->flags & OF_DESTROYABLE) == 0);
			mstruct->lifetime = 0;
		}

		break;
	}
	case MSAFE_OBJECT_CLOAK:
	{
		if (!VALIDATE_OBJECT(mstruct->objhandle))
		{
			mstruct->state = 0;
			mstruct->lifetime = 0;
			return;
		}
		if (!MOBJ->effect_info) {
			mstruct->state = 0;
			mstruct->lifetime = 0;
			return;
		}
		mstruct->state = (bool)((MOBJ->effect_info->type_flags & EF_CLOAKED) != 0);
		if (mstruct->state)
			mstruct->lifetime = MOBJ->effect_info->cloak_time;
		else
			mstruct->lifetime = 0;
		break;
	}
	case MSAFE_OBJECT_LIGHT_DIST:
	{
		mstruct->light_distance = 0.0;	//default
		object* objp = ObjGet(mstruct->objhandle);
		if (objp) {
			light_info* li = ObjGetLightInfo(objp);
			if (li)
				mstruct->light_distance = li->light_distance;
		}
		break;
	}
	case MSAFE_OBJECT_PLAYER_HANDLE:
		mstruct->objhandle = OBJECT_HANDLE_NONE;		//default to none
		if ((mstruct->slot >= 0) && (mstruct->slot < MAX_PLAYERS) && (Players[mstruct->slot].objnum != -1))
		{
			if (Game_mode & GM_MULTI)
			{
				//Don't return the handle for observers
				if (Objects[Players[mstruct->slot].objnum].type == OBJ_OBSERVER)
					break;

				if (NetPlayers[mstruct->slot].flags & NPF_CONNECTED && NetPlayers[mstruct->slot].sequence == NETSEQ_PLAYING)
					mstruct->objhandle = Objects[Players[mstruct->slot].objnum].handle;
			}
			else
			{
				if (mstruct->slot == 0)
					mstruct->objhandle = Objects[Players[mstruct->slot].objnum].handle;
			}
		}
		break;
	case MSAFE_OBJECT_PLAYER_CONTROLAI:
	{
		bool ok = false;
		mstruct->state = false;
		if ((mstruct->slot >= 0) && (mstruct->slot < MAX_PLAYERS) && (Players[mstruct->slot].objnum != -1))
		{
			if (Game_mode & GM_MULTI)
			{
				if (NetPlayers[mstruct->slot].flags & NPF_CONNECTED && NetPlayers[mstruct->slot].sequence == NETSEQ_PLAYING)
					ok = true;
			}
			else
			{
				if (mstruct->slot == 0)
					ok = true;
			}
		}
		if (ok)
		{
			if (Objects[Players[mstruct->slot].objnum].control_type == CT_AI)
				mstruct->state = true;
			else
				mstruct->state = false;
		}
	}
	break;
	case MSAFE_DOOR_LOCK_STATE:
		mstruct->state = DoorwayLocked(mstruct->objhandle);
		break;
	case MSAFE_DOOR_OPENABLE:
		mstruct->state = DoorwayOpenable(mstruct->objhandle, mstruct->ithandle);
		break;
	case MSAFE_DOOR_POSITION:
		mstruct->scalar = DoorwayGetPosition(mstruct->objhandle);
		break;
	case MSAFE_TRIGGER_SET:
		// Returns 1 if the trigger is enabled
		mstruct->state = TriggerGetState(mstruct->trigger_num);
		break;
	case MSAFE_ROOM_HAS_PLAYER:
	{
		int objnum;

		if (!VALIDATE_ROOM(mstruct->roomnum))
		{
			mstruct->state = 0;
			break;
		}
		for (objnum = Rooms[mstruct->roomnum].objects; objnum != -1; objnum = Objects[objnum].next)
		{
			if (Objects[objnum].type == OBJ_PLAYER)
				break;
		}
		mstruct->state = (objnum != -1);
		break;
	}
	case MSAFE_ROOM_PORTAL_RENDER:
	{
		if (!VALIDATE_ROOM_PORTAL(mstruct->roomnum, mstruct->portalnum))
		{
			mstruct->state = 0;
			break;
		}

		portal* pp = &Rooms[mstruct->roomnum].portals[mstruct->portalnum];

		if (pp->flags & PF_RENDER_FACES)
			mstruct->state = 1;
		else
			mstruct->state = 0;
		break;
	}
	case MSAFE_ROOM_PORTAL_BLOCK:
	{
		if (!VALIDATE_ROOM_PORTAL(mstruct->roomnum, mstruct->portalnum))
		{
			mstruct->state = 0;
			break;
		}

		portal* pp = &Rooms[mstruct->roomnum].portals[mstruct->portalnum];
		if (pp->flags & PF_BLOCK)
			mstruct->state = 1;
		else
			mstruct->state = 0;
		break;
	}
	case MSAFE_ROOM_DAMAGE:
		if (!VALIDATE_ROOM(mstruct->roomnum))
		{
			mstruct->amount = 0.0;
			mstruct->index = 0;
			break;
		}
		mstruct->amount = Rooms[mstruct->roomnum].damage;
		mstruct->index = Rooms[mstruct->roomnum].damage_type;
		break;
	case MSAFE_MISC_ENABLE_SHIP:
	{
		mstruct->state = PlayerIsShipAllowed(-1, mstruct->name);
	}break;
	case MSAFE_MISC_WAYPOINT:
		mstruct->index = Current_waypoint;
		break;
	case MSAFE_MISC_GUIDEBOT_NAME:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp && objp->type == OBJ_PLAYER && objp->id == Player_num)
		{
			Current_pilot.get_guidebot_name(mstruct->name);
		}
		else
		{
			strcpy(mstruct->name, "GB");
		}
	}break;
	case MSAFE_INVEN_CHECK:
	case MSAFE_COUNTERMEASURE_CHECK:
	{
		object* objp = ObjGet(mstruct->objhandle);
		int ret;
		if (objp && (objp->type == OBJ_PLAYER)) 
		{
			int slot = objp->id;
			if (type == MSAFE_COUNTERMEASURE_CHECK)
				mstruct->type = OBJ_WEAPON;
			if (mstruct->type == OBJ_WEAPON)
				ret = Players[slot].counter_measures.CheckItem(mstruct->type, mstruct->id);
			else
				ret = Players[slot].inventory.CheckItem(mstruct->type, mstruct->id);
			if (ret)
				mstruct->state = 1;
			else
				mstruct->state = 0;
		}
		break;
	}
	case MSAFE_INVEN_COUNT: 
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp && (objp->type == OBJ_PLAYER)) 
		{
			int slot = objp->id;
			mstruct->count = Players[slot].inventory.GetTypeIDCount(mstruct->type, mstruct->id);
		}
		break;
	}
	case MSAFE_INVEN_SIZE:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp && (objp->type == OBJ_PLAYER)) 
		{
			int slot = objp->id;
			mstruct->size = Players[slot].inventory.Size();
		}
	}break;
	case MSAFE_INVEN_GET_TYPE_ID:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp && (objp->type == OBJ_PLAYER)) 
		{
			int slot = objp->id;
			int size = Players[slot].inventory.Size();
			if (mstruct->index<0 || mstruct->index>size) 
			{
				mstruct->count = 0;
				mstruct->type = OBJ_NONE;
				mstruct->id = 0;
				return;
			}
			int savepos = Players[slot].inventory.GetPos();
			Players[slot].inventory.GotoPos(mstruct->index);
			mstruct->count = Players[slot].inventory.GetPosCount();
			Players[slot].inventory.GetPosTypeID(mstruct->type, mstruct->id);

			Players[slot].inventory.GotoPos(savepos);
		}
	}break;
	case MSAFE_INVEN_CHECK_OBJECT:
	{
		object* player = ObjGet(mstruct->objhandle);
		object* object = ObjGet(mstruct->ithandle);
		mstruct->state = false;
		if (player && object && (player->type == OBJ_PLAYER))
		{
			int slot = player->id;
			mstruct->state = Players[slot].inventory.CheckItem(mstruct->ithandle, -1);
		}
	}break;
	case MSAFE_COUNTERMEASURE_COUNT:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp && (objp->type == OBJ_PLAYER)) 
		{
			int slot = objp->id;
			int id = FindWeaponName(IGNORE_TABLE(mstruct->name));
			if (id == -1)
				mstruct->count = 0;
			else 
				mstruct->count = Players[slot].counter_measures.GetTypeIDCount(OBJ_WEAPON, id);
		}
	}break;
	case MSAFE_COUNTERMEASURE_SIZE:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp && (objp->type == OBJ_PLAYER)) 
		{
			int slot = objp->id;
			mstruct->size = Players[slot].counter_measures.Size();
		}
	}break;
	case MSAFE_COUNTERMEASURE_GET:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp && (objp->type == OBJ_PLAYER)) 
		{
			int slot = objp->id;
			int size = Players[slot].counter_measures.Size();
			if (mstruct->index<0 || mstruct->index>size) 
			{
				mstruct->count = 0;
				mstruct->type = OBJ_NONE;
				mstruct->id = 0;
				return;
			}
			int savepos = Players[slot].counter_measures.GetPos();
			Players[slot].counter_measures.GotoPos(mstruct->index);
			mstruct->count = Players[slot].counter_measures.GetPosCount();
			Players[slot].counter_measures.GetPosTypeID(mstruct->type, mstruct->id);
			Players[slot].counter_measures.GotoPos(savepos);
		}
	}break;
	case MSAFE_ROOM_FOG_STATE:
		if (!VALIDATE_ROOM(mstruct->roomnum))
			mstruct->state = 0;
		else
			mstruct->state = (Rooms[mstruct->roomnum].flags & RF_FOG) != 0;
		break;
	case MSAFE_WEAPON_CHECK:
	{
		//we want to check a player for a particular weapon
		//objhandle: the player
		//index: the weapon
		//state: whether the player has the weapon
		//count: ammo (if it's an ammo/secondary) of the weapon, else -1
		object* objp = ObjGet(mstruct->objhandle);
		mstruct->state = 0;
		ASSERT(mstruct->index >= 0 && mstruct->index < MAX_PRIMARY_WEAPONS + MAX_SECONDARY_WEAPONS);
		if (objp && (objp->type == OBJ_PLAYER))
		{
			int pnum = objp->id;
			if (Players[pnum].weapon_flags & HAS_FLAG(mstruct->index))
			{
				mstruct->state = 1;
				if (mstruct->index >= SECONDARY_INDEX)
				{
					mstruct->count = Players[pnum].weapon_ammo[mstruct->index];
				}
				else
				{
					ship* ship = &Ships[Players[pnum].ship_index];
					otype_wb_info* wb = &ship->static_wb[mstruct->index];
					ASSERT(wb != NULL);
					if (wb->ammo_usage)
					{
						//this guy is an ammo weapon!
						mstruct->count = Players[pnum].weapon_ammo[mstruct->index];
					}
					else
					{
						//no ammo
						mstruct->count = -1;
					}
				}
			}
		}
		else
		{
			Int3(); //uhhhh
		}
	}break;
	case MSAFE_WEAPON_ADD:
		Int3();	//huh?
		break;
	default:
		Int3();	// Invalid type passed to function
		break;
	}
}


//Gets the player slot from the specified object, if the object is a player or player weapon
//Returns -1 if can't find player
int GetPlayerSlot(int objhandle)
{
	object* objp = ObjGet(objhandle);
	if (!objp)
		return -1;
	if (objp->type == OBJ_WEAPON)
		objp = ObjGetUltimateParent(objp);
	if (objp->type == OBJ_PLAYER)
		return objp->id;
	else
		return -1;
}


#define GB_TOKEN	"GUIDEBOT:"
//If the src message is from the guidebot, xlates it into the proper format.  If not a GB
//message, just copies the string over.
//Returns true if is a GB message
bool XlateGBMessage(char* dest, char* src)
{
	if (strnicmp(src, GB_TOKEN, sizeof(GB_TOKEN) - 1) != 0) {
		strcpy(dest, src);
		return 0;
	}
	char* t = src + strlen(GB_TOKEN);		//skip the first part of the string
	while (*t == ' ')		//skip spaces
		t++;
	if (*t == '"')			//skip the leading quote
		t++;
	char gbname[100];
	Current_pilot.get_guidebot_name(gbname);
	sprintf(dest, "\1\255\255\1%s:\1\1\255\1 %s", gbname, t);
	while (dest[strlen(dest) - 1] == ' ')		//strip spaces at end
		dest[strlen(dest) - 1] = 0;
	if (dest[strlen(dest) - 1] == '"')		//Remove the trailing quote
		dest[strlen(dest) - 1] = 0;
	return 1;
}


char Custom_HUD_text[MSAFE_MESSAGE_LENGTH] = "";
extern bool Demo_call_ok;
// The main entry point for all the multisafe functions
// Pass the type of function you want, and then fill in the relevant fields
// of the mstruct 
void msafe_CallFunction(ubyte type, msafe_struct* mstruct)
{
	if ((Demo_flags == DF_PLAYBACK) && (!Demo_call_ok))
		return;
	ubyte send_it = 1;
	switch (type)
	{
	case MSAFE_WEATHER_RAIN:
		SetRainState(mstruct->state, mstruct->scalar);
		break;
	case MSAFE_WEATHER_SNOW:
		SetSnowState(mstruct->state, mstruct->scalar);
		break;
	case MSAFE_WEATHER_LIGHTNING:
		SetLightningState(mstruct->state, mstruct->scalar, mstruct->randval);
		break;
	case MSAFE_WEATHER_LIGHTNING_BOLT:
	{
		if (mstruct->texnum == -1)
		{
			mprintf((0, "Failing bolt because texnum is -1\n"));
			return;
		}

		if (mstruct->flags)
		{
			if (!VALIDATE_OBJECT(mstruct->objhandle))
				return;
		}
		else
		{
			if (!VALIDATE_OBJECT(mstruct->objhandle))
				return;
			if (!VALIDATE_OBJECT(mstruct->ithandle))
				return;
		}
		int visnum = VisEffectCreate(VIS_FIREBALL, THICK_LIGHTNING_INDEX, mstruct->roomnum, &mstruct->pos);
		if (visnum >= 0)
		{
			vis_effect* vis = &VisEffects[visnum];
			vis->lifeleft = mstruct->lifetime;
			vis->lifetime = mstruct->lifetime;
			vis->end_pos = mstruct->pos2;

			vis->custom_handle = mstruct->texnum;

			vis->lighting_color = mstruct->color;
			vis->billboard_info.width = mstruct->size;
			vis->billboard_info.texture = mstruct->state;
			vis->velocity.x = mstruct->count;
			vis->velocity.y = mstruct->interval;
			vis->velocity.z = mstruct->index;

			vis->flags = VF_USES_LIFELEFT | VF_WINDSHIELD_EFFECT | VF_LINK_TO_VIEWER;
			vis->size = vm_VectorDistanceQuick(&vis->pos, &vis->end_pos);
			if (mstruct->flags) // Do attached lightning effect
			{
				vis->flags |= VF_ATTACHED;
				vis->attach_info.obj_handle = MOBJ->handle;
				vis->attach_info.modelnum = MOBJ->rtype.pobj_info.model_num;
				vis->attach_info.vertnum = mstruct->g1;
				vis->attach_info.end_vertnum = mstruct->g2;
				WeaponCalcGun(&vis->pos, NULL, MOBJ, vis->attach_info.vertnum);
				WeaponCalcGun(&vis->end_pos, NULL, MOBJ, vis->attach_info.end_vertnum);
			}
			else
			{
				vis->flags |= VF_ATTACHED | VF_PLANAR;
				vis->attach_info.obj_handle = MOBJ->handle;
				vis->attach_info.dest_objhandle = IOBJ->handle;
				vis->end_pos = IOBJ->pos;
			}
		}
		break;
	}
	case MSAFE_ROOM_TEXTURE:
	{
		if (!VALIDATE_ROOM_FACE(mstruct->roomnum, mstruct->facenum))
			return;
		ASSERT(Rooms[mstruct->roomnum].num_faces > mstruct->facenum);
		ChangeRoomFaceTexture(mstruct->roomnum, mstruct->facenum, mstruct->index);
		break;
	}
	case MSAFE_ROOM_WIND:
		if (!VALIDATE_ROOM(mstruct->roomnum))
			return;
		Rooms[mstruct->roomnum].wind = mstruct->wind;
		Rooms[mstruct->roomnum].room_change_flags |= RCF_WIND;
		break;
	case MSAFE_ROOM_FOG:
		if (!VALIDATE_ROOM(mstruct->roomnum))
			return;
		Rooms[mstruct->roomnum].fog_r = mstruct->fog_r;
		Rooms[mstruct->roomnum].fog_g = mstruct->fog_g;
		Rooms[mstruct->roomnum].fog_b = mstruct->fog_b;
		Rooms[mstruct->roomnum].fog_depth = mstruct->fog_depth;
		Rooms[mstruct->roomnum].room_change_flags |= RCF_FOG;
		Rooms[mstruct->roomnum].flags |= RF_FOG;
		break;
	case MSAFE_ROOM_FOG_STATE:
		if (!VALIDATE_ROOM(mstruct->roomnum))
			return;
		if (mstruct->state)
			Rooms[mstruct->roomnum].flags |= RF_FOG;
		else
			Rooms[mstruct->roomnum].flags &= ~RF_FOG;
		Rooms[mstruct->roomnum].room_change_flags |= RCF_FOG;
		break;
	case MSAFE_ROOM_LIGHT_PULSE:
		if (!VALIDATE_ROOM(mstruct->roomnum))
			return;
		Rooms[mstruct->roomnum].pulse_time = mstruct->pulse_time;
		Rooms[mstruct->roomnum].pulse_offset = mstruct->pulse_offset;
		Rooms[mstruct->roomnum].room_change_flags |= RCF_LIGHTING;
		break;
	case MSAFE_ROOM_LIGHT_FLICKER:
		if (!VALIDATE_ROOM(mstruct->roomnum))
			return;
		if (mstruct->state)
			Rooms[mstruct->roomnum].flags |= RF_FLICKER;
		else
			Rooms[mstruct->roomnum].flags &= ~RF_FLICKER;
		Rooms[mstruct->roomnum].room_change_flags |= RCF_LIGHTING;
		break;
	case MSAFE_ROOM_LIGHT_STROBE:
		if (!VALIDATE_ROOM(mstruct->roomnum))
			return;
		if (mstruct->state)
			Rooms[mstruct->roomnum].flags |= RF_STROBE;
		else
			Rooms[mstruct->roomnum].flags &= ~RF_STROBE;
		Rooms[mstruct->roomnum].room_change_flags |= RCF_LIGHTING;
		break;
	case MSAFE_ROOM_REFUEL:
		if (!VALIDATE_ROOM(mstruct->roomnum))
			return;
		if (mstruct->state)
			Rooms[mstruct->roomnum].flags |= RF_FUELCEN;
		else
			Rooms[mstruct->roomnum].flags &= ~RF_FUELCEN;
		Rooms[mstruct->roomnum].room_change_flags |= RCF_REFUEL;
		break;
	case MSAFE_ROOM_CHANGING_FOG: 
	{
		if (!VALIDATE_ROOM(mstruct->roomnum))
			return;
		vector fog_vec;
		fog_vec.x = mstruct->fog_r;
		fog_vec.y = mstruct->fog_g;
		fog_vec.z = mstruct->fog_b;
		SetRoomChangeOverTime(mstruct->roomnum, 1, &fog_vec, mstruct->fog_depth, mstruct->interval);
		break;
	}
	case MSAFE_ROOM_CHANGING_WIND: 
	{
		if (!VALIDATE_ROOM(mstruct->roomnum))
			return;
		SetRoomChangeOverTime(mstruct->roomnum, 0, &mstruct->wind, 0, mstruct->interval);
		break;
	}
	case MSAFE_ROOM_PORTAL_RENDER:
	{
		if (!VALIDATE_ROOM_PORTAL(mstruct->roomnum, mstruct->portalnum))
			return;

		portal* pp = &Rooms[mstruct->roomnum].portals[mstruct->portalnum];

		if (mstruct->state)
			pp->flags |= PF_RENDER_FACES;
		else
			pp->flags &= ~PF_RENDER_FACES;
		Rooms[mstruct->roomnum].room_change_flags |= RCF_PORTAL_RENDER;
		pp->flags |= PF_CHANGED;

		//check other side of portal
		if (mstruct->flags) {
			portal* pp2 = &Rooms[pp->croom].portals[pp->cportal];
			if (mstruct->state)
				pp2->flags |= PF_RENDER_FACES;
			else
				pp2->flags &= ~PF_RENDER_FACES;
			Rooms[pp->croom].room_change_flags |= RCF_PORTAL_RENDER;
			pp2->flags |= PF_CHANGED;
		}
		break;
	}
	case MSAFE_ROOM_PORTAL_BLOCK:
	{
		if (!VALIDATE_ROOM_PORTAL(mstruct->roomnum, mstruct->portalnum))
			return;

		portal* pp = &Rooms[mstruct->roomnum].portals[mstruct->portalnum];
		if (!(pp->flags & (PF_BLOCK | PF_BLOCK_REMOVABLE)))
			return;
		if (mstruct->state)
			pp->flags |= PF_BLOCK;
		else
			pp->flags &= ~PF_BLOCK;
		Rooms[mstruct->roomnum].room_change_flags |= RCF_PORTAL_BLOCK;
		pp->flags |= PF_CHANGED;

		//check other side of portal
		portal* pp2 = &Rooms[pp->croom].portals[pp->cportal];
		if (mstruct->state)
			pp2->flags |= PF_BLOCK;
		else
			pp2->flags &= ~PF_BLOCK;
		Rooms[pp->croom].room_change_flags |= RCF_PORTAL_BLOCK;
		pp2->flags |= PF_CHANGED;
		break;
	}
	case MSAFE_ROOM_BREAK_GLASS:
	{
		void ComputeCenterPointOnFace(vector * vp, room * rp, int facenum);
		if (!VALIDATE_ROOM_PORTAL(mstruct->roomnum, mstruct->portalnum))
			return;
		send_it = 0;	// Handled in function below

		BreakGlassFace(&Rooms[mstruct->roomnum], Rooms[mstruct->roomnum].portals[mstruct->portalnum].portal_face);
		break;
	}
	case MSAFE_ROOM_DAMAGE:
		if (!VALIDATE_ROOM(mstruct->roomnum))
			return;
		Rooms[mstruct->roomnum].damage = mstruct->amount;
		Rooms[mstruct->roomnum].damage_type = mstruct->index;
		Rooms[mstruct->roomnum].room_change_flags |= RCF_DAMAGE;
		break;
	case MSAFE_ROOM_REMOVE_ALL_POWERUPS:
	{
		if (VALIDATE_ROOM(mstruct->roomnum))
		{
			int i, high_index;
			int invis_id;
			invis_id = FindObjectIDName("InvisiblePowerup");
			high_index = Highest_object_index + 1;
			for (i = 0; i < high_index; i++)
			{
				if (Objects[i].type != OBJ_POWERUP)
					continue;
				if (Objects[i].roomnum != mstruct->roomnum)
					continue;
				if (Objects[i].id == invis_id)
					continue;
				SetObjectDeadFlag(&Objects[i], true, false);
			}
		}
	}break;
	case MSAFE_OBJECT_FIRE_WEAPON:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp && (mstruct->index >= 0) && (mstruct->index < MAX_WEAPONS) && Weapons[mstruct->index].used)
			FireWeaponFromObject(objp, mstruct->index, mstruct->gunpoint);
		send_it = 0;
		break;
	}
	case MSAFE_OBJECT_SETONFIRE:
	{
		if (!VALIDATE_OBJECT(mstruct->objhandle))
			return;

		object* obj = MOBJ;
		float damage_time = mstruct->longevity;
		float d_per_sec = mstruct->interval;
		if (obj->effect_info)
		{
			if (damage_time > 0)
			{
				obj->effect_info->type_flags |= EF_NAPALMED;
				obj->effect_info->damage_time += damage_time;
				//NOTE: commented out because lava rocks need to go burning for awhile...
				//so we trust that someone doesn't do something stupid
				//obj->effect_info->damage_time=min(10.0f,obj->effect_info->damage_time);
				obj->effect_info->damage_per_second = d_per_sec;
				if (Gametime - obj->effect_info->last_damage_time > 1.0f)
					obj->effect_info->last_damage_time = 0;

				obj->effect_info->damage_handle = mstruct->ithandle;
				if (obj->effect_info->sound_handle == SOUND_NONE_INDEX)
					obj->effect_info->sound_handle = Sound_system.Play3dSound(SOUND_PLAYER_BURNING, SND_PRIORITY_HIGHEST, obj);
			}
			else
			{
				obj->effect_info->type_flags &= ~EF_NAPALMED;
				obj->effect_info->damage_time = 0;
			}
		}
	}
	break;
	case MSAFE_OBJECT_SHIELDS:
		if (!VALIDATE_OBJECT(mstruct->objhandle))
			return;

		MOBJ->shields = mstruct->shields;
		MOBJ->flags &= ~(OF_AI_DEATH);

		break;
	case MSAFE_OBJECT_ENERGY:
		if (!VALIDATE_OBJECT(mstruct->objhandle))
			return;
		ASSERT(MOBJ->type == OBJ_PLAYER);
		Players[MOBJ->id].energy = mstruct->energy;
		break;
	case MSAFE_SHOW_ENABLED_CONTROLS:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp && (objp->type == OBJ_PLAYER))
			Hud_show_controls = mstruct->state ? true : false;
	}
	break;
	case MSAFE_OBJECT_PLAYER_CMASK: 
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp && (objp->type == OBJ_PLAYER)) 
		{
			if (mstruct->control_val)
				Players[objp->id].controller_bitflags |= mstruct->control_mask;
			else
				Players[objp->id].controller_bitflags &= ~mstruct->control_mask;
		}
		break;
	}
	case MSAFE_OBJECT_PLAYER_KEY: 
	{
		int slot = GetPlayerSlot(mstruct->ithandle);
		if (slot == -1)
			break;	//couldn't find player object
		if (Players[slot].keys & KEY_FLAG(mstruct->index))
			return;		//if player already has key, do nothing
		//We should really have a function to set the key flags
		Players[slot].keys |= KEY_FLAG(mstruct->index);
		Global_keys |= KEY_FLAG(mstruct->index);
		if (mstruct->objhandle != OBJECT_HANDLE_NONE) 
		{
			object* keyobj = ObjGet(mstruct->objhandle);
			if (keyobj)
			{
				mprintf((0, "Adding key from multisafe to player %d\n", slot));
				Sound_system.Play3dSound(SOUND_POWERUP_PICKUP, SND_PRIORITY_HIGH, keyobj);
				Players[slot].inventory.Add(keyobj->type, keyobj->id, NULL, -1, -1, INVAF_NOTSPEWABLE, mstruct->message);

				if (!(Game_mode & GM_MULTI))	//If not multiplayer, delete the key
				{
					SetObjectDeadFlag(keyobj);
				}
				else if ((keyobj->flags & OF_INFORM_DESTROY_TO_LG))
				{
					Level_goals.Inform(LIT_OBJECT, LGF_COMP_DESTROY, keyobj->handle);
				}
			}
		}
		send_it = 0; // Handled by server
		break;
	}
	case MSAFE_OBJECT_MOVEMENT_SCALAR:
		if (!VALIDATE_OBJECT(mstruct->objhandle))
			return;
		ASSERT(MOBJ->type == OBJ_PLAYER);
		Players[MOBJ->id].movement_scalar = mstruct->scalar;
		break;
	case MSAFE_OBJECT_RECHARGE_SCALAR:
		if (!VALIDATE_OBJECT(mstruct->objhandle))
			return;
		ASSERT(MOBJ->type == OBJ_PLAYER);
		Players[MOBJ->id].weapon_recharge_scalar = mstruct->scalar;
		break;
	case MSAFE_OBJECT_WSPEED_SCALAR:
		if (!VALIDATE_OBJECT(mstruct->objhandle))
			return;
		ASSERT(MOBJ->type == OBJ_PLAYER);
		Players[MOBJ->id].weapon_speed_scalar = mstruct->scalar;
		break;
	case MSAFE_OBJECT_ARMOR_SCALAR:
		if (!VALIDATE_OBJECT(mstruct->objhandle))
			return;
		ASSERT(MOBJ->type == OBJ_PLAYER);
		Players[MOBJ->id].armor_scalar = mstruct->scalar;
		break;
	case MSAFE_OBJECT_DAMAGE_SCALAR:
		if (!VALIDATE_OBJECT(mstruct->objhandle))
			return;
		ASSERT(MOBJ->type == OBJ_PLAYER);
		Players[MOBJ->id].damage_scalar = mstruct->scalar;
		break;
	case MSAFE_OBJECT_ADD_WEAPON:
		if (!VALIDATE_OBJECT(mstruct->objhandle))
			return;
		ASSERT(MOBJ->type == OBJ_PLAYER);
		AddWeaponToPlayer(MOBJ->id, mstruct->index, mstruct->ammo);
		if (MOBJ->id == Player_num)
			send_it = 0;	// Don't send because the server is picking it up
		break;
	case MSAFE_OBJECT_DAMAGE_OBJECT:
		if (!VALIDATE_OBJECT(mstruct->objhandle))
			return;
		send_it = 0;	// Don't need to send because ApplyDamage does the work
		if (MOBJ->type == OBJ_PLAYER)
		{
			if (mstruct->killer_handle == OBJECT_HANDLE_NONE)
				ApplyDamageToPlayer(MOBJ, NULL, mstruct->damage_type, mstruct->amount);
			else
				ApplyDamageToPlayer(MOBJ, KOBJ, mstruct->damage_type, mstruct->amount);
		}
		else
		{
			if (mstruct->killer_handle == OBJECT_HANDLE_NONE)
				ApplyDamageToGeneric(MOBJ, NULL, mstruct->damage_type, mstruct->amount);
			else
				ApplyDamageToGeneric(MOBJ, KOBJ, mstruct->damage_type, mstruct->amount);
		}
		break;
	case MSAFE_OBJECT_START_SPEW:
	{
		if (!VALIDATE_OBJECT(mstruct->objhandle))
			return;
		spewinfo spew;
		if (mstruct->gunpoint == -2)
		{		//no gun
			object* objp = ObjGet(mstruct->objhandle);
			if (!objp)
				break;
			spew.use_gunpoint = false;
			spew.pt.origin = objp->pos;
			spew.pt.normal = objp->orient.uvec;
			spew.pt.room_num = objp->roomnum;
		}
		else	if (mstruct->gunpoint == -1)
		{		//no gun
			object* objp = ObjGet(mstruct->objhandle);
			if (!objp)
				break;
			spew.use_gunpoint = false;
			spew.pt.origin = objp->pos;
			spew.pt.normal = objp->orient.fvec;
			spew.pt.room_num = objp->roomnum;
		}
		else
		{
			spew.use_gunpoint = true;
			spew.gp.obj_handle = mstruct->objhandle;
			spew.gp.gunpoint = mstruct->gunpoint;
		}
		spew.random = mstruct->random;
		spew.real_obj = (bool)(mstruct->is_real != 0);
		spew.effect_type = mstruct->effect_type;
		spew.phys_info = mstruct->phys_info;
		spew.drag = mstruct->drag;
		spew.mass = mstruct->mass;
		spew.time_int = mstruct->interval;
		spew.longevity = mstruct->longevity;
		spew.lifetime = mstruct->lifetime;
		spew.size = mstruct->size;
		spew.speed = mstruct->speed;
		if ((Game_mode & GM_MULTI) && Netgame.local_role == LR_CLIENT)
		{
			int spewnum = SpewCreate(&spew);
			ASSERT(spewnum != -1);	//DAJ -1FIX
			spewnum &= 0xFF;

			Server_spew_list[mstruct->id] = spewnum;
		}
		else 
		{
			mstruct->id = SpewCreate(&spew);
			ASSERT(mstruct->id != -1);	//DAJ -1FIX
		}
		break;
	}
	case MSAFE_OBJECT_STOP_SPEW:
	{
		if ((Game_mode & GM_MULTI) && Netgame.local_role == LR_CLIENT)
		{
			SpewClearEvent(SpewEffects[mstruct->id].handle, true);
			Server_spew_list[mstruct->id] = 65535;
		}
		else
		{
			SpewClearEvent(mstruct->id, true);
		}
		break;
	}
	case MSAFE_OBJECT_NO_RENDER:
		if (!VALIDATE_OBJECT(mstruct->objhandle))
			return;
		MOBJ->render_type = RT_NONE;
		MOBJ->change_flags |= OCF_NO_RENDER;
		break;
	case MSAFE_OBJECT_GHOST:
		if (!VALIDATE_OBJECT(mstruct->objhandle))
			return;
		if (ObjGet(mstruct->objhandle)->type == OBJ_NONE) return;
		if (ObjGet(mstruct->objhandle)->type == OBJ_DUMMY) return;	//Don't ghost a ghosted object!!
		if (ObjGet(mstruct->objhandle)->type == OBJ_PLAYER) return;
		if (ObjGet(mstruct->objhandle)->type == OBJ_GHOST) return;
		ObjGhostObject(mstruct->objhandle & HANDLE_OBJNUM_MASK);
		break;
	case MSAFE_OBJECT_UNGHOST:
		if (!VALIDATE_OBJECT(mstruct->objhandle))
			return;
		if (ObjGet(mstruct->objhandle)->type != OBJ_DUMMY) return;	//Don't unghost an unghosted object!!
		ObjUnGhostObject(mstruct->objhandle & HANDLE_OBJNUM_MASK);
		break;
	case MSAFE_OBJECT_REMOVE:
		send_it = 0;
		if (!VALIDATE_OBJECT(mstruct->objhandle))
			return;

		SetObjectDeadFlag(MOBJ, true, (bool)(mstruct->playsound != 0));
		if (mstruct->playsound)
			Sound_system.Play3dSound(SOUND_POWERUP_PICKUP, SND_PRIORITY_HIGH, MOBJ);
		break;
	case MSAFE_OBJECT_INVULNERABLE:
	{
		if (!VALIDATE_OBJECT(mstruct->objhandle))
			return;

		if (MOBJ->type == OBJ_PLAYER)
		{
			if (mstruct->state)
				MakePlayerInvulnerable(MOBJ->id, mstruct->lifetime);
			else
				MakePlayerVulnerable(MOBJ->id);
		}
		else
		{
			if (mstruct->state)
				MOBJ->flags &= ~OF_DESTROYABLE;
			else
				MOBJ->flags |= OF_DESTROYABLE;
		}
		send_it = 1;
		break;
	}
	case MSAFE_OBJECT_CLOAKALLPLAYERS:
	{
		if (mstruct->state)
		{
			if (Game_mode & GM_MULTI)
			{
				for (int i = 0; i < MAX_PLAYERS; i++)
				{
					if (NetPlayers[i].flags & NPF_CONNECTED && NetPlayers[i].sequence == NETSEQ_PLAYING)
						MakeObjectInvisible(&Objects[Players[i].objnum], mstruct->lifetime, 1.0f, true);
				}
			}
			else
			{
				MakeObjectInvisible(Player_object, mstruct->lifetime, 1.0f, true);
			}
		}
		else
		{
			if (Game_mode & GM_MULTI)
			{
				for (int i = 0; i < MAX_PLAYERS; i++)
				{
					if (NetPlayers[i].flags & NPF_CONNECTED && NetPlayers[i].sequence == NETSEQ_PLAYING)
					{
						MakeObjectVisible(&Objects[Players[i].objnum]);
					}
				}
			}
			else
			{
				MakeObjectVisible(Player_object);
			}
		}
		send_it = 1;
	}break;
	case MSAFE_OBJECT_CLOAK:
	{
		if (!VALIDATE_OBJECT(mstruct->objhandle))
			return;
		if (mstruct->state)
			MakeObjectInvisible(MOBJ, mstruct->lifetime);
		else
			MakeObjectVisible(MOBJ);
		
		send_it = 1;
		break;
	}
	case MSAFE_OBJECT_LIGHT_DIST: 
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp) 
		{
			ObjSetLocalLighting(objp);
			if (mstruct->light_distance < 0)
				mstruct->light_distance = 0;
			objp->lighting_info->light_distance = mstruct->light_distance;
			objp->change_flags |= OCF_LIGHT_DISTANCE;
		}
		break;
	}
	case MSAFE_OBJECT_LIGHT_COLOR: 
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp) 
		{
			ObjSetLocalLighting(objp);
			objp->lighting_info->red_light1 = mstruct->r1;
			objp->lighting_info->green_light1 = mstruct->g1;
			objp->lighting_info->blue_light1 = mstruct->b1;
			objp->change_flags |= OCF_LIGHT_COLOR;
		}
		break;
	}
	case MSAFE_OBJECT_DEFORM: 
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp && objp->effect_info) 
		{
			objp->effect_info->type_flags |= EF_DEFORM;
			objp->effect_info->deform_range = mstruct->amount;
			objp->effect_info->deform_time = mstruct->lifetime;
		}
		break;
	}
	case MSAFE_OBJECT_SPARKS:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp && objp->effect_info)
		{
			objp->effect_info->type_flags |= EF_SPARKING;
			objp->effect_info->spark_delay = 1.0 / mstruct->amount;
			objp->effect_info->spark_timer = 0;
			objp->effect_info->spark_time_left = mstruct->lifetime;
		}
		break;
	}
	case MSAFE_OBJECT_VIEWER_SHAKE:
	{
		AddToShakeMagnitude(mstruct->amount);
		break;
	}
	case MSAFE_OBJECT_SHAKE_AREA:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp)
		{
			float dist = vm_VectorDistanceQuick(&Viewer_object->pos, &objp->pos);
			if (dist < mstruct->scalar)
				AddToShakeMagnitude(mstruct->amount * (1.0 - (dist / mstruct->scalar)));
		}
		break;
	}
	case MSAFE_OBJECT_WORLD_POSITION:
	{
		object* objp = ObjGet(mstruct->objhandle);
		send_it = 0;
		if (objp)
		{
			if (mstruct->roomnum != -1)
			{
				ObjSetPos(objp, &mstruct->pos, mstruct->roomnum, &mstruct->orient, true);
				// When DON'T we want to send this to the clients??
				send_it = 1;
			}
		}
	}break;
	case MSAFE_OBJECT_PLAYER_CONTROLAI:
	{
		bool ok = false;
		send_it = 0;
		if ((mstruct->slot >= 0) && (mstruct->slot < MAX_PLAYERS) && (Players[mstruct->slot].objnum != -1))
		{
			if (Game_mode & GM_MULTI)
			{
				if (NetPlayers[mstruct->slot].flags & NPF_CONNECTED && NetPlayers[mstruct->slot].sequence == NETSEQ_PLAYING)
					ok = true;
			}
			else
			{
				if (mstruct->slot == 0)
					ok = true;
			}
		}
		if (ok)
		{
			//if mstruct->state is true, then set to AI
			//else set back to control mode
			osipf_SetPlayerControlMode(mstruct->slot, (bool)(mstruct->state != 0));
		}
	}
	break;
	case MSAFE_OBJECT_DESTROY_ROBOTS_EXCEPT:
	{
		//destroy all object (generics) except those in the list specified
		tKillObjectItem* koilist = (tKillObjectItem*)mstruct->list;
		int koisize = mstruct->count;
		bool kill_it;
		send_it = 0;
		//go through all generic objects, kill those that don't match an exception
		for (int i = 0; i <= Highest_object_index; i++)
		{
			if (IS_ROBOT((&Objects[i])) && !(IS_GUIDEBOT(&Objects[i])))
			{
				kill_it = true;
				//see if it's on the exception list
				for (int x = 0; x < koisize; x++)
				{
					switch (koilist[x].info_type)
					{
					case KOI_ID:
						if (Objects[i].id == koilist[x].id)
						{
							//spare it!
							kill_it = false;
						}break;
					case KOI_HANDLE:
						if (Objects[i].handle == koilist[x].objhandle)
						{
							//spare it!
							kill_it = false;
						}break;
					}
					if (kill_it == false)
						break;
				}
				if (kill_it)
				{
					SetObjectDeadFlag(&Objects[i], true, false);
				}
			}
		}
	}break;
	case MSAFE_SOUND_STREAMING:
	{
		int slot;
		if (mstruct->state)
		{
			//player specified
			slot = GetPlayerSlot(mstruct->objhandle);
			if (slot == -1)
				break;	//couldn't find player object
			mstruct->objhandle = Objects[Players[slot].objnum].handle;
		}
		else
			slot = -1;	//all players

		if ((slot == -1) || (slot == Player_num))
			mstruct->sound_handle = StreamPlay(mstruct->name, mstruct->volume, mstruct->flags);
		if ((Game_mode & GM_MULTI) && Netgame.local_role == LR_SERVER && slot == Player_num)
			send_it = 0;
		mstruct->slot = slot;	//for send_to
		break;
	}
	case MSAFE_SOUND_2D:
	{
		int slot;
		if (mstruct->state)
		{
			//player specified
			slot = GetPlayerSlot(mstruct->objhandle);
			if (slot == -1)
				break;	//couldn't find player object
			mstruct->objhandle = Objects[Players[slot].objnum].handle;
		}
		else
			slot = -1;	//all players

		if ((slot == -1) || (slot == Player_num))
			mstruct->sound_handle = Sound_system.Play2dSound(mstruct->index, SND_PRIORITY_HIGHEST, mstruct->volume);
		else
			mstruct->sound_handle = -1;
		if ((Game_mode & GM_MULTI) && Netgame.local_role == LR_SERVER && slot == Player_num)
			send_it = 0;
		mstruct->slot = slot;	//for send_to
		break;
	}
	case MSAFE_SOUND_OBJECT:
	{
		if (!VALIDATE_OBJECT(mstruct->objhandle))
			return;
		mstruct->sound_handle = Sound_system.Play3dSound(mstruct->index, SND_PRIORITY_HIGHEST, MOBJ);
		break;
	}
	case MSAFE_SOUND_STOP:
		Int3();
		send_it = 0;
		break;
	case MSAFE_SOUND_STOP_OBJ:
		Sound_system.StopObjectSound(mstruct->objhandle);
		break;
	case MSAFE_SOUND_VOLUME_OBJ:
		Sound_system.SetVolumeObject(mstruct->objhandle, mstruct->volume);
		break;
	case MSAFE_MUSIC_REGION:
	{
		int slot;
		if (mstruct->state)
		{
			//player specified
			slot = GetPlayerSlot(mstruct->objhandle);
			if (slot == -1)
				break;		//couldn't find player object
			mstruct->objhandle = Objects[Players[slot].objnum].handle;
		}
		else
			slot = -1;	//all players
		if ((slot == -1) || (slot == Player_num))
			D3MusicSetRegion(mstruct->index);
		if ((Game_mode & GM_MULTI) && Netgame.local_role == LR_SERVER && slot == Player_num)
			send_it = 0;

		mstruct->slot = slot;	//for send_to
		break;
	}
	case MSAFE_MISC_LEVELGOAL:
		if (Game_mode & GM_MULTI && Netgame.local_role == LR_CLIENT)
		{
			int v = 0xFFFFFFFF;
			Level_goals.GoalSetName(mstruct->index, mstruct->message);
			Level_goals.GoalStatus(mstruct->index, LO_CLEAR_SPECIFIED, &v);
			Level_goals.GoalStatus(mstruct->index, LO_SET_SPECIFIED, &mstruct->type);
			Level_goals.GoalPriority(mstruct->index, LO_SET_SPECIFIED, &mstruct->count);
		}
		break;
	case MSAFE_MISC_WAYPOINT:
		PlayerAddWaypoint(mstruct->index);
		break;
	case MSAFE_MISC_ENABLE_SHIP:
	{
		PlayerSetShipPermission(-1, mstruct->name, (bool)(mstruct->state != 0));
	}break;
	case MSAFE_MISC_FILTERED_HUD_MESSAGE:
	case MSAFE_MISC_HUD_MESSAGE:
	{
		int slot;
		if (mstruct->state)
		{
			//player specified
			slot = GetPlayerSlot(mstruct->objhandle);
			if (slot == -1)
				break;		//couldn't find player object

			mstruct->objhandle = Objects[Players[slot].objnum].handle;
		}
		else
			slot = -1;	//all players

		if ((slot == -1) || (slot == Player_num))
		{
			char message[256];
			bool gb = XlateGBMessage(message, mstruct->message);
			bool added;
			if (type == MSAFE_MISC_HUD_MESSAGE)
				added = AddColoredHUDMessage(mstruct->color, "%s", message);	// "%s" because message could contain % signs!
			else
				added = AddFilteredColoredHUDMessage(mstruct->color, "%s", message);	// "%s" because message could contain % signs!
			if (gb && added)	//If was GB message & was added, play the GB sound
				Sound_system.Play2dSound(FindSoundName("GBotGreetB1"), SND_PRIORITY_HIGHEST, 1.0);
		}
		if ((Game_mode & GM_MULTI) && Netgame.local_role == LR_SERVER && slot == Player_num)
			send_it = 0;
		mstruct->slot = slot;	//for send_to
		break;
	}
	case MSAFE_MISC_GAME_MESSAGE:
	{
		int slot;
		if (mstruct->state)
		{
			//player specified
			slot = GetPlayerSlot(mstruct->objhandle);
			if (slot == -1)
				break;		//couldn't find player object
			mstruct->objhandle = Objects[Players[slot].objnum].handle;
		}
		else
			slot = -1;	//all players

		if ((slot == -1) || (slot == Player_num))
		{
			AddGameMessage(mstruct->message);
			char message2[256];
			XlateGBMessage(message2, mstruct->message2);
			int y = Game_window_h / 4 - 15;
			if (Demo_flags == DF_RECORDING)
				DemoWritePersistantHUDMessage(mstruct->color, HUD_MSG_PERSISTENT_CENTER, y, 10.0, HPF_FADEOUT + HPF_FREESPACE_DRAW, SOUND_GAME_MESSAGE, message2);
			if (Demo_flags != DF_PLAYBACK)
				AddPersistentHUDMessage(mstruct->color, HUD_MSG_PERSISTENT_CENTER, y, 10.0, HPF_FADEOUT + HPF_FREESPACE_DRAW, SOUND_GAME_MESSAGE, message2);
		}
		if ((Game_mode & GM_MULTI) && Netgame.local_role == LR_SERVER && slot == Player_num)
			send_it = 0;

		mstruct->slot = slot;	//for send_to
		break;
	}
	case MSAFE_MISC_END_LEVEL:
		if (Game_mode & GM_MULTI)
			MultiEndLevel();
		else
			SetGameState(mstruct->state ? GAMESTATE_LVLEND : GAMESTATE_LVLFAILED);
		send_it = 0;
		break;
	case MSAFE_MISC_POPUP_CAMERA:
	{
		CreateSmallView(SVW_LEFT, mstruct->objhandle, SVF_POPUP + SVF_BIGGER, mstruct->interval, D3_DEFAULT_ZOOM / mstruct->scalar, mstruct->gunpoint);
		break;
	}
	case MSAFE_MISC_CLOSE_POPUP:
		ClosePopupView(SVW_LEFT);
		break;
	case MSAFE_MISC_GUIDEBOT_NAME:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp && objp->type == OBJ_PLAYER && objp->id == Player_num)
			Current_pilot.set_guidebot_name(mstruct->name);
	}break;
	case MSAFE_MISC_START_TIMER:
	{
		extern void RenderHUDTimer(tHUDItem * item);
		int Osiris_GetTimerHandle(int id);
		int handle = Osiris_GetTimerHandle(mstruct->index);
		if (handle == 0)
			return;
		tHUDItem huditem;
		memset(&huditem, 0, sizeof(huditem));
		huditem.alpha = HUD_ALPHA;
		huditem.color = mstruct->color;
		huditem.type = HUD_ITEM_TIMER;
		huditem.x = 0;
		huditem.y = 0;
		huditem.data.timer_handle = handle;
		huditem.stat = STAT_CUSTOM;
		huditem.flags = HUD_FLAG_LEVEL;
		huditem.render_fn = RenderHUDTimer;
		AddHUDItem(&huditem);
		send_it = 0;
		break;
	}
	case MSAFE_MISC_UPDATE_HUD_ITEM:
		int FindCustomtext2HUDItem();
		//If item not added yet, add it now
		if (FindCustomtext2HUDItem() == -1)
		{
			tHUDItem huditem;
			memset(&huditem, 0, sizeof(huditem));
			huditem.alpha = HUD_ALPHA;
			huditem.color = mstruct->color;
			huditem.type = HUD_ITEM_CUSTOMTEXT2;
			huditem.x = 0;
			huditem.y = 0;
			huditem.buffer_size = MSAFE_MESSAGE_LENGTH;
			huditem.stat = STAT_CUSTOM;
			huditem.flags = HUD_FLAG_LEVEL;
			huditem.render_fn = NULL;
			AddHUDItem(&huditem);
			ASSERT(FindCustomtext2HUDItem() != -1);
		}
		UpdateCustomtext2HUDItem(mstruct->message);
		break;
	case MSAFE_DOOR_LOCK_STATE:
		DoorwayLockUnlock(mstruct->objhandle, (mstruct->state != 0));
		break;
	case MSAFE_DOOR_ACTIVATE:
		ASSERT(Objects[mstruct->objhandle & HANDLE_OBJNUM_MASK].type == OBJ_DOOR);
		DoorwayActivate(mstruct->objhandle);
		break;
	case MSAFE_DOOR_STOP:
		DoorwayStop(mstruct->objhandle);
		break;
	case MSAFE_DOOR_POSITION:
		DoorwaySetPosition(mstruct->objhandle, mstruct->scalar);
		break;
	case MSAFE_TRIGGER_SET:
		TriggerSetState(mstruct->trigger_num, (mstruct->state != 0));
		send_it = 0;
		break;
	case MSAFE_INVEN_ADD_OBJECT:
	{
		object* player = ObjGet(mstruct->objhandle);
		object* item = ObjGet(mstruct->ithandle);
		bool ret = false;

		send_it = 0;
		if (player && item && (player->type == OBJ_PLAYER))
		{
			int slot = player->id;
			ret = Players[slot].inventory.AddObject(item->handle, mstruct->flags, (mstruct->message[0] != '\0') ? mstruct->message : NULL);
			if (ret)
				send_it = 1;
		}
	}break;
	case MSAFE_INVEN_REMOVE_OBJECT:
	{
		object* player = ObjGet(mstruct->objhandle);
		object* item = ObjGet(mstruct->ithandle);
		mstruct->state = false;

		send_it = 0;
		if (player && item && (player->type == OBJ_PLAYER))
		{
			int slot = player->id;
			mstruct->state = Players[slot].inventory.Remove(item->handle, -1);
			if (mstruct->state)
				send_it = 1;
		}
	}break;
	case MSAFE_INVEN_ADD_TYPE_ID:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp && (objp->type == OBJ_PLAYER))
		{
			int slot = objp->id;

			if (mstruct->type == OBJ_WEAPON)
				Players[slot].counter_measures.Add(mstruct->type, mstruct->id, NULL, -1, -1, mstruct->flags);
			else
				Players[slot].inventory.Add(mstruct->type, mstruct->id, NULL, -1, -1, mstruct->flags);
		}
		break;
	}
	case MSAFE_INVEN_REMOVE:
	case MSAFE_COUNTERMEASURE_REMOVE:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp && (objp->type == OBJ_PLAYER))
		{
			int slot = objp->id;
			if (type == MSAFE_COUNTERMEASURE_REMOVE)
				mstruct->type = OBJ_WEAPON;

			if (mstruct->type == OBJ_WEAPON)
				Players[slot].counter_measures.Remove(mstruct->type, mstruct->id);
			else
				Players[slot].inventory.Remove(mstruct->type, mstruct->id);
		}
		break;
	}
	case MSAFE_COUNTERMEASURE_ADD:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp && (objp->type == OBJ_PLAYER))
		{
			int slot = objp->id;
			int total = 0;
			if (mstruct->count > 0)
			{
				int id = FindWeaponName(IGNORE_TABLE(mstruct->name));
				if (id != -1) {
					for (int i = 0; i < mstruct->count; i++)
					{
						if (Players[slot].counter_measures.Add(OBJ_WEAPON, id, objp, mstruct->aux_type, mstruct->aux_id))
							total++;
					}
				}
			}
			mstruct->count = total;
			if (total)
				send_it = 1;
			else
				send_it = 0;
		}
	}break;
	case MSAFE_OBJECT_ROTDRAG:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp)
		{
			objp->mtype.phys_info.rotdrag = mstruct->rot_drag;
			send_it = false;
		}
		else
		{
			return;
		}
	}
	break;
	case MSAFE_OBJECT_TYPE:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp)
			objp->type = mstruct->type;
		else
			return;
	}
	break;
	case MSAFE_OBJECT_ID:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp)
			objp->id = mstruct->id;
		else
			return;
	}
	break;
	case MSAFE_OBJECT_CONTROL_TYPE:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp && !(objp->flags & OF_DYING))
			objp->control_type = mstruct->control_type;
		else
			return;
	}
	break;
	case MSAFE_OBJECT_FLAGS:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp)
		{
			if (Game_mode & GM_MULTI && Netgame.local_role == LR_CLIENT)
			{
				ASSERT(objp->flags & OF_SERVER_OBJECT);
			}
			if (objp->flags != (unsigned int)mstruct->flags)
				objp->flags = mstruct->flags;
			else
				send_it = false;
			if (Game_mode & GM_MULTI && Netgame.local_role == LR_CLIENT)
				objp->flags |= OF_SERVER_OBJECT;
		}
		else
		{
			return;
		}
	}
	break;
	case MSAFE_OBJECT_MOVEMENT_TYPE:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp)
			objp->movement_type = mstruct->movement_type;
		else
			return;
	}
	break;
	case MSAFE_OBJECT_CREATION_TIME:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp)
			objp->creation_time = mstruct->creation_time;
		else
			return;
	}
	break;
	case MSAFE_OBJECT_PHYSICS_FLAGS:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp)
		{
			if (objp->mtype.phys_info.flags != (unsigned int)mstruct->physics_flags)
			{
				objp->mtype.phys_info.flags = mstruct->physics_flags;
				objp->change_flags |= OCF_PHYS_FLAGS;
			}
			else
			{
				send_it = false;
			}
		}
		else
		{
			return;
		}
	}
	break;
	case MSAFE_OBJECT_PARENT:
	{
		object* objp = ObjGet(mstruct->objhandle);
		if (objp)
			objp->parent_handle = mstruct->ithandle;
		else
			return;
	}
	break;

	case MSAFE_WEAPON_CHECK:
		Int3();	//hmmmm
		break;
	case MSAFE_WEAPON_ADD:
	{
		//add/subtract ammo/weapons from a player
		//if count>0 and the player doesn't have the weapon it will be added
		//objhandle: the player
		//index: the weapon
		//state: if the weapon is a secondary:
		//			1: remove weapon if ammo<=0
		//			0: do nothing to the weapon
		//		 if the weapon is a primary
		//			1: remove weapon if count==0
		//			0: do nothing to the weapon
		//count: ammo (if it's an ammo/secondary) of the weapon
		object* player = ObjGet(mstruct->objhandle);
		ASSERT(mstruct->index >= 0 && mstruct->index < MAX_PRIMARY_WEAPONS + MAX_SECONDARY_WEAPONS);
		if (player && player->type == OBJ_PLAYER)
		{
			ship* ship = &Ships[Players[player->id].ship_index];
			otype_wb_info* wb = &ship->static_wb[mstruct->index];
			ASSERT(wb != NULL);
			// check if the player has the weapon
			bool have_weapon = PlayerHasWeapon(player->id, mstruct->index);
			if (!have_weapon && mstruct->count <= 0)
			{
				//nothing to do
				return;
			}
			if (mstruct->count > 0)
			{
				bool is_ammo = false;
				if ((mstruct->index >= SECONDARY_INDEX) || wb->ammo_usage)
					is_ammo = true;

				//we're going to give the player the weapon and ammo (if needed)
				if (!have_weapon)
				{
					AddWeaponToPlayer(player->id, mstruct->index, (is_ammo) ? mstruct->count : 0);
					//thats it!
					return;
				}

				//ok, the player already has this weapon, add some ammo if ammo weapon
				int old_ammo = Players[player->id].weapon_ammo[mstruct->index];
				int added = 1;
				//now add ammo if necessary
				if (is_ammo)
					added = AddWeaponAmmo(player->id, mstruct->index, mstruct->count);
				if (added > 0 && player->id == Player_num && old_ammo == 0)
					CheckForWeaponSelect(player->id, mstruct->index);
			}
			else
			{
				bool weap_removed = false;
				int type;
				//we're going to remove the weapon/ammo
				if (mstruct->index >= SECONDARY_INDEX)
				{
					type = PW_SECONDARY;
					//remove ammo
					Players[player->id].weapon_ammo[mstruct->index] += mstruct->count;
					if (Players[player->id].weapon_ammo[mstruct->index] < 0)
						Players[player->id].weapon_ammo[mstruct->index] = 0;
					if (Players[player->id].weapon_ammo[mstruct->index] == 0)
					{
						weap_removed = true;
						if (mstruct->state == 1)
						{
							//remove weapon
							Players[player->id].weapon_flags &= ~HAS_FLAG(mstruct->index);
						}
					}
				}
				else
				{
					type = PW_PRIMARY;
					//remove the weapon
					if (mstruct->state == 1)
					{
						Players[player->id].weapon_flags &= ~HAS_FLAG(mstruct->index);
						weap_removed = true;
					}
				}
				if (weap_removed)
				{
					int curr_weapon_index = Players[player->id].weapon[type].index;
					if (curr_weapon_index == mstruct->index)
					{
						//check autoselect
						if (player->id == Player_num) 
						{
							bool select_new = AutoSelectWeapon(type, -1);
							if (!select_new) {
								//AddHUDMessage ("%s",TXT(Static_weapon_names_msg[Players[player->id].weapon[type].index]));
							}
						}
					}
				}
			}
		}
		else
		{
			Int3();	//come on it isn't even a player!
		}
	}break;
	default:
		Int3(); // Illegal type passed to multisafe function
		break;
	}
	if (((Demo_flags == DF_RECORDING) && send_it) || (send_it && (Game_mode & GM_MULTI) && Netgame.local_role == LR_SERVER))
		MultiSendMSafeFunction(type, mstruct);
}


#define SHIELD_BONUS	12.0f
#define ENERGY_BONUS	12.0f
// Returns true if the player has this weapon (this fuction should be moved)
bool PlayerHasWeapon(int slot, int weapon_index)
{
	if (Players[slot].weapon_flags & HAS_FLAG(weapon_index))
		if (weapon_index >= SECONDARY_INDEX)
			return (Players[slot].weapon_ammo[weapon_index] > 0);	//have secondary only if have ammo
		else
			return true;	//primaries don't check for ammo
	else
		return false;
}


// Adds weapon ammo to a player, returns 0 if it couldn't
// This function should be moved!
int AddWeaponAmmo(int slot, int weap_index, int ammo)
{
	//Get pointer to the battery for this weapon
	player* player = &Players[slot];
	ship* ship = &Ships[player->ship_index];
	otype_wb_info* wb = &ship->static_wb[weap_index];
	ASSERT(wb != NULL);
	//if secondary or primary that uses ammo, then use the ammo
	if ((weap_index >= SECONDARY_INDEX) || wb->ammo_usage)
	{
		//figure out much ammo to add
		int added = min(ship->max_ammo[weap_index] - player->weapon_ammo[weap_index], ammo);
		//now add it
		player->weapon_ammo[weap_index] += added;
		return added;
	}
	else	//not an ammo-using weapon
	{
		Int3();		//trying to add ammo to something that doesn't take it
		return 0;
	}
}


bool AddPowerupEnergyToPlayer(int id)
{
	if (Game_mode & GM_MULTI)
		return false;
	float curr_energy = Players[Player_num].energy;
	if (Players[id].energy >= MAX_ENERGY)
		return false;
	float amount = 10.0f * Diff_general_scalar[DIFF_LEVEL];
	curr_energy = min(MAX_ENERGY, curr_energy + amount);
	Players[id].energy = curr_energy;
	return true;
}


// returns true if it was handled
bool HandleCommonPowerups(char* pname, msafe_struct* mstruct, ubyte* pickup);
bool HandleWeaponPowerups(char* pname, msafe_struct* mstruct, ubyte* pickup);
bool HandleCounterMeasurePowerups(char* pname, msafe_struct* mstruct, ubyte* pickup);
bool HandleInventoryPowerups(char* pname, msafe_struct* mstruct, ubyte* pickup);
// Does whatever magic needs to be done to get the default powerups to work
void msafe_DoPowerup(msafe_struct* mstruct)
{
	char pname[255];
	strcpy(pname, Object_info[MOBJ->id].name);
	ASSERT(MOBJ->type == OBJ_POWERUP);
	ASSERT(IOBJ->type == OBJ_PLAYER);
	ubyte pickup = 0;
	// Now go through and do the magic for each powerup
	bool handled = false;
	handled = HandleCommonPowerups(pname, mstruct, &pickup);
	if (!handled)
		handled = HandleWeaponPowerups(pname, mstruct, &pickup);
	if (!handled)
		handled = HandleCounterMeasurePowerups(pname, mstruct, &pickup);
	if (!handled)
		handled = HandleInventoryPowerups(pname, mstruct, &pickup);
	if (!handled)
		return;
	//if (Game_mode & GM_MULTI && Netgame.local_role==LR_SERVER)
	//{
	MultiSendMSafePowerup(mstruct);
	//}

	// Now remove the object from the world
	if (pickup)
	{
		if (((Game_mode & GM_MULTI) && Netgame.local_role == LR_SERVER) || !(Game_mode & GM_MULTI))
		{
			if (Demo_flags == DF_RECORDING)
			{
				//DemoWriteSetObjDead(MOBJ);
			}
			SetObjectDeadFlag(MOBJ, true, (bool)(mstruct->playsound != 0));
			Sound_system.Play3dSound(SOUND_POWERUP_PICKUP, SND_PRIORITY_HIGH, MOBJ);
		}
	}
}


//Data for primary powerups
struct 
{
	char* name;
	int	weapon_index;
	int	pickup_msg, already_have_msg, ammo_msg, full_msg;
} powerup_data_primary[] = 
{
	{"Vauss",VAUSS_INDEX,TXI_MSG_VAUSS,TXI_MSG_HAVEVAUSS,TXI_MSG_VAUSSAMMO,TXI_MSG_VAUSSFULL},
	{"Napalm",NAPALM_INDEX,TXI_MSG_NAPALM,TXI_MSG_NAPALMHAVE,TXI_MSG_NAPALMFUEL,TXI_MSG_NAPALMFULL},
	{"EMDlauncher",EMD_INDEX,TXI_MSG_EMD,TXI_MSG_EMDALREADYHAVE,-1,-1},
	{"Microwave",MICROWAVE_INDEX,TXI_MSG_MICROWAVE,TXI_MSG_MICROWAVEHAVE,-1,-1},
	{"MassDriver",MASSDRIVER_INDEX,TXI_MSG_MASSDRIVER,TXI_MSG_MASSALREADYHAVE,TXI_MSG_MASSAMMO,TXI_MSG_MASSFULL},
	{"SuperLaser",SUPER_LASER_INDEX,TXI_MSG_SUPERLASER,TXI_MSG_SUPERLHAVE,-1,-1},
	{"Plasmacannon",PLASMA_INDEX,TXI_MSG_PLASMA,TXI_MSG_PLASMAHAVE,-1,-1},
	{"Fusioncannon",FUSION_INDEX,TXI_MSG_FUSION,TXI_MSG_FUSIONHAVE,-1,-1},
	{"Omegacannon",OMEGA_INDEX,TXI_MSG_OMEGA,TXI_MSG_OMEGAHAVE,-1,-1},
};


#define NUM_POWERUP_TYPES_PRIMARY (sizeof(powerup_data_primary) / sizeof(*powerup_data_primary))
struct 
{
	char* name;
	int	weapon_index;
	int	added_one_msg, added_multi_msg, full_msg;
} powerup_data_secondary[] = 
{
	{"Frag",FRAG_INDEX,TXI_MSG_FRAG,-1,TXI_MSG_FRAGFULL},
	{"ImpactMortar",IMPACTMORTAR_INDEX,TXI_MSG_IMPACTM,-1,TXI_MSG_IMPACTMFULL},
	{"NapalmRocket",NAPALMROCKET_INDEX,TXI_MSG_NAPALMR,-1,TXI_MSG_NAPALMRFULL},
	{"Cyclone",CYCLONE_INDEX,TXI_MSG_CYCLONE,-1,TXI_MSG_CYCLONEFULL},
	{"BlackShark",BLACKSHARK_INDEX,TXI_MSG_BSHARK,-1,TXI_MSG_BSHARKFULL},
	{"Concussion",CONCUSSION_INDEX,TXI_MSG_CONC,-1,TXI_MSG_CONCFULL},
	{"Homing",HOMING_INDEX,TXI_MSG_HOMING,-1,TXI_MSG_HOMINGFULL},
	{"Smart",SMART_INDEX,TXI_MSG_SMART,-1,TXI_MSG_SMARTFULL},
	{"Mega",MEGA_INDEX,TXI_MSG_MEGA,-1,TXI_MSG_MEGAFULL},
	{"Guided",GUIDED_INDEX,TXI_MSG_GUIDED,-1,TXI_MSG_GUIDEDFULL},
	{"4PackHoming",HOMING_INDEX,TXI_MSG_HOMING,TXI_MSG_MULTI_HOMING,TXI_MSG_HOMINGFULL},
	{"4PackConc",CONCUSSION_INDEX,TXI_MSG_CONC,TXI_MSG_MULTI_CONC,TXI_MSG_CONCFULL},
	{"4PackFrag",FRAG_INDEX,TXI_MSG_FRAG,TXI_MSG_MULTI_FRAG,TXI_MSG_FRAGFULL},
	{"4PackGuided",GUIDED_INDEX,TXI_MSG_GUIDED,TXI_MSG_MULTI_GUIDED,TXI_MSG_GUIDEDFULL},
};


#define NUM_POWERUP_TYPES_SECONDARY (sizeof(powerup_data_secondary) / sizeof(*powerup_data_secondary))
struct 
{
	char* name;
	int	weapon_index;
	int	ammo_msg, full_msg;
} powerup_data_ammo[] = 
{
	{"Vauss clip",VAUSS_INDEX,TXI_MSG_VAUSSAMMO,TXI_MSG_VAUSSFULL},
	{"MassDriverAmmo",MASSDRIVER_INDEX,TXI_MSG_MASSAMMO,TXI_MSG_MASSFULL},
	{"NapalmTank",NAPALM_INDEX,TXI_MSG_NAPALMFUEL,TXI_MSG_NAPALMFULL}
};


#define NUM_POWERUP_TYPES_AMMO (sizeof(powerup_data_ammo) / sizeof(*powerup_data_ammo))
void ShowAmmoAddedMessage(int weapon_index, int msg_index, int added)
{
	int added_frac = 0;
	if (Ships[Players[Player_num].ship_index].fire_flags[weapon_index] & SFF_TENTHS) 
	{
		added_frac = added;
		while (added_frac >= 100)
			added_frac -= 100;
		while (added_frac >= 10)
			added_frac -= 10;
		added /= 10;
	}
	AddFilteredHUDMessage(TXT(msg_index), added, added_frac);
}


//See if we should select the specified weapon
void CheckForWeaponSelect(int id, int weapon_index)
{
	if (id == Player_num)
	{
		bool select_new;
		if (weapon_index < SECONDARY_INDEX)
			select_new = AutoSelectWeapon(PW_PRIMARY, weapon_index);
		else
			select_new = AutoSelectWeapon(PW_SECONDARY, weapon_index);

		if (!select_new)
			AddFilteredHUDMessage("%s", TXT(Static_weapon_names_msg[weapon_index]));
	}
}


extern ubyte AutomapVisMap[MAX_ROOMS];
//New spiffy table-based code
bool HandleWeaponPowerups(char* pname, msafe_struct* mstruct, ubyte* pickup)
{
	object* player = ObjGet(mstruct->ithandle);
	object* powerup = ObjGet(mstruct->objhandle);
	int p;
	if (!player || !powerup)
		return 0;
	bool handled = false;
	ASSERT(player->type == OBJ_PLAYER);
	ASSERT(powerup->control_type == CT_POWERUP);
	//Check the primaries
	for (p = 0; p < NUM_POWERUP_TYPES_PRIMARY; p++)
	{
		if (!stricmp(powerup_data_primary[p].name, pname))
		{
			handled = true;
			int weapon_index = powerup_data_primary[p].weapon_index;
			if (!PlayerHasWeapon(player->id, weapon_index))
			{
				*pickup = 1;
				if (player->id == Player_num)
					AddFilteredHUDMessage(TXT(powerup_data_primary[p].pickup_msg));
				AddWeaponToPlayer(player->id, weapon_index, powerup->ctype.powerup_info.count);
			}
			else
			{
				//we already have this weapon
				//If multiplayer, don't do anything (except print the message), so only do stuff if not multiplayer
				if (!(Game_mode & GM_MULTI))
				{
					//If this is an ammo weapon, give the player the ammo
					if (powerup->ctype.powerup_info.count > 0)
					{
						int old_ammo = Players[player->id].weapon_ammo[weapon_index];
						int added = AddWeaponAmmo(player->id, weapon_index, powerup->ctype.powerup_info.count);
						if (added)
						{
							//got ammo, so remove powerup
							*pickup = 1;
							if (player->id == Player_num)
							{
								ShowAmmoAddedMessage(weapon_index, powerup_data_primary[p].ammo_msg, added);
								if (old_ammo == 0)
									CheckForWeaponSelect(player->id, weapon_index);
							}
						}
						else
						{
							//did not get ammo from powerup, so say full
							if (player->id == Player_num)
								AddFilteredHUDMessage(TXT(powerup_data_primary[p].full_msg));
						}
					}
					else
					{
						//not an ammo weapon (or has no ammo)
						//Give the player energy & remove powerup, if energy not full,
						if (AddPowerupEnergyToPlayer(player->id))
							*pickup = 1;
					}
				}
				else
				{
					if (player->id == Player_num)
						AddFilteredHUDMessage(TXT_ALREADY_HAVE_WEAPON);
				}
			}
		}
	}


	//Check the secondaries
	for (p = 0; p < NUM_POWERUP_TYPES_SECONDARY; p++)
	{
		if (!stricmp(powerup_data_secondary[p].name, pname))
		{
			handled = true;
			int weapon_index = powerup_data_secondary[p].weapon_index;
			//Does the player have any now?
			bool has_weapon = PlayerHasWeapon(player->id, weapon_index);
			//Determine how much to add
			int added = AddWeaponAmmo(player->id, weapon_index, powerup->ctype.powerup_info.count);
			//Pickup powerup if multiplayer or if got any ammo from it
			*pickup = (Game_mode & GM_MULTI) || (added != 0);

			//Show message
			if (player->id == Player_num)
			{
				if (added > 1)
				{
					ASSERT(powerup_data_secondary[p].added_multi_msg != -1);
					AddFilteredHUDMessage(TXT(powerup_data_secondary[p].added_multi_msg), added);
				}
				else if (added == 1)
					AddFilteredHUDMessage(TXT(powerup_data_secondary[p].added_one_msg));
				else
					AddFilteredHUDMessage(TXT(powerup_data_secondary[p].full_msg));
			}
			//Give the player the weapon if didn't already have it

			ship* ship = &Ships[Players[player->id].ship_index];
			otype_wb_info* wb = &ship->static_wb[weapon_index];

			if (!has_weapon || (player->id == Player_num && wb->ammo_usage > 1 && wb->ammo_usage == Players[player->id].weapon_ammo[weapon_index]))
				AddWeaponToPlayer(player->id, weapon_index, 0);
		}
	}

	//Check the ammo powerups
	for (p = 0; p < NUM_POWERUP_TYPES_AMMO; p++)
	{
		if (!stricmp(pname, powerup_data_ammo[p].name))
		{
			handled = true;
			int weapon_index = powerup_data_ammo[p].weapon_index;
			ASSERT(weapon_index < SECONDARY_INDEX);
			//Get the ammo from this weapon
			int old_ammo = Players[player->id].weapon_ammo[weapon_index];
			int added = AddWeaponAmmo(player->id, weapon_index, powerup->ctype.powerup_info.count);
			//Pickup powerup if multiplayer or if got any ammo from it
			*pickup = (Game_mode & GM_MULTI) || (added != 0);

			if (player->id == Player_num)
			{
				if (added > 0)
					ShowAmmoAddedMessage(weapon_index, powerup_data_ammo[p].ammo_msg, added);
				else
					AddFilteredHUDMessage(TXT(powerup_data_ammo[p].full_msg));
			}
			//Check to select the weapon
			if (added && PlayerHasWeapon(player->id, weapon_index) && (old_ammo == 0))
				CheckForWeaponSelect(player->id, weapon_index);
		}
	}
	return handled;
}
// returns true if it was handled
bool HandleCommonPowerups(char* pname, msafe_struct* mstruct, ubyte* pickup)
{
	int pnum = IOBJ->id;
	bool handled = false;
	// Shield bonus
	if (!stricmp("Shield", pname))
	{
		handled = true;
		// If this guy is already at max shields, then don't do anything
		if ((IOBJ->shields) >= MAX_SHIELDS)
		{
			if (pnum == Player_num)
				AddFilteredHUDMessage(TXT_MAXSHIELDS);
		}
		else
		{
			// Pick it up
			*pickup = 1;
			float addval = SHIELD_BONUS * Diff_shield_energy_scalar[DIFF_LEVEL];
			if (IOBJ->shields + addval > MAX_SHIELDS)
				addval = MAX_SHIELDS - IOBJ->shields;

			IOBJ->shields += addval;
			if (pnum == Player_num)
				AddFilteredHUDMessage(TXT_SHIELDBOOST, (int)IOBJ->shields);
		}
	}
	// ENERGY POWERUP
	else if (!stricmp("Energy", pname))
	{
		handled = true;
		float addval = ENERGY_BONUS * Diff_shield_energy_scalar[DIFF_LEVEL];
		addval = min(addval, MAX_ENERGY - Players[pnum].energy);
		Players[pnum].energy += addval;

		//Pick up if multiplayer or added
		if ((Game_mode & GM_MULTI) || (addval > 0))
			*pickup = 1;
		if (pnum == Player_num)
			AddFilteredHUDMessage((addval > 0) ? TXT_MSG_ENERGY : TXT_MAXENERGY);
	}
	//Quad Laser
	else if (!stricmp("QuadLaser", pname))
	{
		handled = true;
		object* pobj = &Objects[Players[pnum].objnum];
		if (!(pobj->dynamic_wb[LASER_INDEX].flags & DWBF_QUAD))
		{
			//player doesn't have Quad Lasers

			if (pnum == Player_num)
				AddFilteredHUDMessage(TXT_QUADLASER);
			*pickup = 1;
			pobj->dynamic_wb[LASER_INDEX].flags |= DWBF_QUAD;
			pobj->dynamic_wb[SUPER_LASER_INDEX].flags |= DWBF_QUAD;

			//add them to the inventory so we can spew it
			static int quad_id = -2;
			if (quad_id == -2)
				quad_id = FindObjectIDName(IGNORE_TABLE(pname));
			if (quad_id > -1)
			{
				Players[pnum].inventory.Add(OBJ_POWERUP, quad_id, NULL, -1, -1, INVAF_LEVELLAST | INVAF_TIMEOUTONSPEW);
			}
		}
		else if (pnum == Player_num)
			AddFilteredHUDMessage(TXT_MSG_QUADHAVE);
	}
	//FullMap Powerup
	else if (!stricmp("FullMap", pname))
	{
		handled = true;
		if (pnum == Player_num) {
			AddFilteredHUDMessage(TXT_FULLMAP);

			for (int i = 0; i < MAX_ROOMS; i++)
			{
				if (!(Rooms[i].flags & RF_SECRET))
					AutomapVisMap[i] = 1;
			}
		}

		*pickup = 1;
	}
	//Thief stolen automap
	else if (!stricmp("ThiefAutoMap", pname))
	{
		handled = true;
		if (pnum == Player_num)
			AddFilteredHUDMessage(TXT_RETURNEDAUTOMAP);
		
		ThiefReturnItem(IOBJ->handle, THIEFITEM_AUTOMAP);
		*pickup = 1;
	}
	//Thief stolen headlight
	else if (!stricmp("HeadlightPowerup", pname))
	{
		handled = true;
		if (!ThiefPlayerHasItem(IOBJ->handle, THIEFITEM_HEADLIGHT))
		{
			if (pnum == Player_num)
				AddFilteredHUDMessage(TXT_RETURNEDHEADLIGHT);
			
			ThiefReturnItem(IOBJ->handle, THIEFITEM_HEADLIGHT);
			*pickup = 1;
		}
		else
		{
			if (pnum == Player_num)
				AddFilteredHUDMessage(TXT_HAVEHEADLIGHT);
		}
	}
	//Invulnerability
	else if (!stricmp(pname, "Invulnerability"))
	{
		handled = true;
		if (!(Players[pnum].flags & PLAYER_FLAGS_INVULNERABLE))
		{
			float time_period = 30.0f;	//30 seconds
			MakePlayerInvulnerable(pnum, time_period, true);
			*pickup = 1;
		}
		else if (pnum == Player_num)
		{
			AddFilteredHUDMessage(TXT_INVULNALREADY);
		}
	}
	//Cloak
	else if (!stricmp(pname, "Cloak"))
	{
		handled = true;
		if (IOBJ->effect_info)
		{
			if (!((IOBJ->effect_info->type_flags & EF_FADING_OUT) || (IOBJ->effect_info->type_flags & EF_CLOAKED)))
			{
				float time_period = 30.0f;	//30 seconds
				MakeObjectInvisible(IOBJ, time_period);
				*pickup = 1;
			}
			else if (pnum == Player_num)
				AddFilteredHUDMessage(TXT_CLOAKALREADY);
		}
	}
	return handled;
}


bool HandleCounterMeasurePowerups(char* pname, msafe_struct* mstruct, ubyte* pickup)
{
	bool handled = false;
	// Chaff
	if (!stricmp("Chaff", pname))
	{
		handled = true;
		int pnum = IOBJ->id;
		int added_count = 0;
		int weapon_id = FindWeaponName("Chaff");

		if (weapon_id != -1)
		{
			int amount = 12 - Players[pnum].counter_measures.GetTypeIDCount(OBJ_WEAPON, weapon_id);
			if (amount > 4)
				amount = 4;
			if (amount > 0)
			{
				for (int c = 0; c < amount; c++)
				{
					if (Players[pnum].counter_measures.AddCounterMeasure(weapon_id, MOBJ->type, MOBJ->id))
						added_count++;
					else
						break;
				}
				if (added_count)
				{
					// Pick it up
					*pickup = 1;
				}
				if (pnum == Player_num)
				{
					if (added_count == 1)
						AddFilteredHUDMessage(TXT_CHAFF);
					else if (added_count > 1)
						AddFilteredHUDMessage(TXT_MSG_MULTI_CHAFFS, added_count);
					else
						AddFilteredHUDMessage(TXT_COUNTERMEASUREFULL);
				}
			}
			else if (pnum == Player_num)
				AddFilteredHUDMessage(TXT_CHAFFSFULL);
		}
	}

	// Betty4pack
	else if (!stricmp("Betty4Pack", pname))
	{
		handled = true;
		int pnum = IOBJ->id;
		int added_count = 0;
		int weapon_id = FindWeaponName("Betty");

		if (weapon_id != -1)
		{
			int amount = 12 - Players[pnum].counter_measures.GetTypeIDCount(OBJ_WEAPON, weapon_id);
			if (amount > 4)
				amount = 4;
			if (amount > 0)
			{
				for (int c = 0; c < amount; c++)
				{
					if (Players[pnum].counter_measures.AddCounterMeasure(weapon_id, MOBJ->type, MOBJ->id))
						added_count++;
					else
						break;
				}

				if (added_count)
				{
					// Pick it up
					*pickup = 1;
				}

				if (pnum == Player_num)
				{
					if (added_count == 1)
						AddFilteredHUDMessage(TXT_BETTY);
					else if (added_count > 1)
						AddFilteredHUDMessage(TXT_MSG_MULTI_BETTY, added_count);
					else
						AddFilteredHUDMessage(TXT_COUNTERMEASUREFULL);
				}
			}
			else if (pnum == Player_num) 
			{
				AddFilteredHUDMessage(TXT_BETTYFULL);
			}
		}
	}

	// Seeker3pack
	else if (!stricmp("Seeker3Pack", pname))
	{
		handled = true;
		int pnum = IOBJ->id;
		int added_count = 0;
		int weapon_id = FindWeaponName("SeekerMine");

		if (weapon_id != -1)
		{
			int amount = 12 - Players[pnum].counter_measures.GetTypeIDCount(OBJ_WEAPON, weapon_id);
			if (amount > 3)
				amount = 3;
			if (amount > 0)
			{
				for (int c = 0; c < amount; c++)
				{
					if (Players[pnum].counter_measures.AddCounterMeasure(weapon_id, MOBJ->type, MOBJ->id))
						added_count++;
					else
						break;
				}
				if (added_count)
				{
					// Pick it up
					*pickup = 1;
				}
				if (pnum == Player_num)
				{
					if (added_count == 1)
						AddFilteredHUDMessage(TXT_SEEKERMINE);
					else if (added_count > 1)
						AddFilteredHUDMessage(TXT_MSG_MULTI_SEEKERS, added_count);
					else
						AddFilteredHUDMessage(TXT_COUNTERMEASUREFULL);
				}
			}
			else if (pnum == Player_num)
			{
				AddFilteredHUDMessage(TXT_SEEKERFULL);
			}
		}
	}

	// Gunboy
	else if (!stricmp("GunboyPowerup", pname))
	{
		handled = true;
		int pnum = IOBJ->id;
		int added_count = 0;
		int weapon_id = FindWeaponName("GunBoy");

		if (weapon_id != -1)
		{
			int amount = 4 - Players[pnum].counter_measures.GetTypeIDCount(OBJ_WEAPON, weapon_id);
			if (amount > 1)
				amount = 1;
			if (amount > 0)
			{
				for (int c = 0; c < amount; c++)
				{
					if (Players[pnum].counter_measures.AddCounterMeasure(weapon_id, MOBJ->type, MOBJ->id))
						added_count++;
					else
						break;
				}
				if (added_count)
				{
					// Pick it up
					*pickup = 1;
				}
				if (pnum == Player_num)
				{
					if (added_count == 1)
						AddFilteredHUDMessage(TXT_GUNBOY);
					else if (added_count > 1)
						AddFilteredHUDMessage(TXT_MSG_MULTI_GUNBOY, added_count);
					else
						AddFilteredHUDMessage(TXT_COUNTERMEASUREFULL);
				}
			}
			else if (pnum == Player_num)
			{
				AddFilteredHUDMessage(TXT_GUNBOYFULL);
			}
		}
	}

	// ProxMine
	else if (!stricmp("ProxMinePowerup", pname))
	{
		handled = true;
		int pnum = IOBJ->id;
		int added_count = 0;
		int weapon_id = FindWeaponName("ProxMine");

		if (weapon_id != -1)
		{
			int amount = 12 - Players[pnum].counter_measures.GetTypeIDCount(OBJ_WEAPON, weapon_id);
			if (amount > 4)
				amount = 4;
			if (amount > 0)
			{
				for (int c = 0; c < amount; c++)
				{
					if (Players[pnum].counter_measures.AddCounterMeasure(weapon_id, MOBJ->type, MOBJ->id))
						added_count++;
					else
						break;
				}
				if (added_count)
				{
					// Pick it up
					*pickup = 1;
				}
				if (pnum == Player_num)
				{
					if (added_count == 1)
						AddFilteredHUDMessage(TXT_PROXMINE);
					else if (added_count > 1)
						AddFilteredHUDMessage(TXT_PROXMINECOUNT, added_count);
					else
						AddFilteredHUDMessage(TXT_COUNTERMEASUREFULL);
				}
			}
			else if (pnum == Player_num) 
			{
				AddFilteredHUDMessage(TXT_PROXMINEFULL);
			}
		}
	}
	return handled;
}

bool HandleInventoryPowerups(char* pname, msafe_struct* mstruct, ubyte* pickup)
{
	bool handled = false;
	int pnum = IOBJ->id;
	//Afterburner cooler
	if (!stricmp("Afterburner", pname))
	{
		handled = true;
		int ab_id = FindObjectIDName("Afterburner");
		if (ab_id != -1)
		{
			int inv_count = Players[pnum].inventory.GetTypeIDCount(OBJ_POWERUP, ab_id);
			if (inv_count == 0)
			{
				//pickit up
				*pickup = 1;
				Players[pnum].inventory.Add(OBJ_POWERUP, ab_id, NULL, -1, -1, INVAF_TIMEOUTONSPEW, TXT_ABCOOLER);
				if (pnum == Player_num)
					AddFilteredHUDMessage(TXT_AFTERBURNERCOOLER);
			}
			else if (pnum == Player_num)
			{
				AddFilteredHUDMessage(TXT_ABCOOLERHAVE);
			}
		}
	}

	//Energy->Shield Converter
	else if (!stricmp("Converter", pname))
	{
		handled = true;
		int es_id = FindObjectIDName("Converter");
		if (es_id != -1)
		{
			int inv_count = Players[pnum].inventory.GetTypeIDCount(OBJ_POWERUP, es_id);
			if (inv_count == 0)
			{
				//pick it up
				*pickup = 1;
				Players[pnum].inventory.Add(OBJ_POWERUP, es_id, NULL, -1, -1, INVAF_TIMEOUTONSPEW);
				if (pnum == Player_num)
					AddFilteredHUDMessage(TXT_ETOSCONVERTER);
			}
			else if (pnum == Player_num)
			{
				AddFilteredHUDMessage(TXT_ETOSCONVHAVE);
			}
		}
	}
	return handled;
}
