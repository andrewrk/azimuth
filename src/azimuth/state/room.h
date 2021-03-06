/*=============================================================================
| Copyright 2012 Matthew D. Steele <mdsteele@alum.mit.edu>                    |
|                                                                             |
| This file is part of Azimuth.                                               |
|                                                                             |
| Azimuth is free software: you can redistribute it and/or modify it under    |
| the terms of the GNU General Public License as published by the Free        |
| Software Foundation, either version 3 of the License, or (at your option)   |
| any later version.                                                          |
|                                                                             |
| Azimuth is distributed in the hope that it will be useful, but WITHOUT      |
| ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       |
| FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   |
| more details.                                                               |
|                                                                             |
| You should have received a copy of the GNU General Public License along     |
| with Azimuth.  If not, see <http://www.gnu.org/licenses/>.                  |
=============================================================================*/

#pragma once
#ifndef AZIMUTH_STATE_ROOM_H_
#define AZIMUTH_STATE_ROOM_H_

#include <stdint.h>

#include "azimuth/state/background.h"
#include "azimuth/state/baddie.h" // for az_baddie_kind_t
#include "azimuth/state/camera.h" // for az_camera_bounds_t
#include "azimuth/state/door.h" // for az_door_kind_t
#include "azimuth/state/gravfield.h"
#include "azimuth/state/node.h" // for az_node_kind_t
#include "azimuth/state/player.h"
#include "azimuth/state/script.h"
#include "azimuth/state/wall.h"

/*===========================================================================*/

// Maximum number of each kind of object that can be in one room:
#define AZ_MAX_NUM_BADDIES 60
#define AZ_MAX_NUM_DOORS 20
#define AZ_MAX_NUM_GRAVFIELDS 50
#define AZ_MAX_NUM_NODES 150
#define AZ_MAX_NUM_WALLS 300

// The size of the UUID table:
#define AZ_NUM_UUID_SLOTS 50

typedef struct {
  az_baddie_kind_t kind;
  az_script_t *on_kill; // owned; NULL if no script
  az_vector_t position;
  double angle;
  int uuid_slot; // 0 if none, otherwise from 1 to AZ_NUM_UUID_SLOTS inclusive
  int cargo_slots[AZ_MAX_BADDIE_CARGO_UUIDS]; // UUID slots of objects to carry
} az_baddie_spec_t;

typedef struct {
  az_door_kind_t kind;
  az_script_t *on_open; // owned; NULL if no script
  az_vector_t position;
  double angle;
  az_room_key_t destination;
  int uuid_slot; // 0 if none, otherwise from 1 to AZ_NUM_UUID_SLOTS inclusive
} az_door_spec_t;

typedef struct {
  az_gravfield_kind_t kind;
  az_script_t *on_enter; // owned; NULL if no script
  az_vector_t position;
  double angle;
  double strength;
  az_gravfield_size_t size;
  int uuid_slot; // 0 if none, otherwise from 1 to AZ_NUM_UUID_SLOTS inclusive
} az_gravfield_spec_t;

typedef struct {
  az_node_kind_t kind;
  az_node_subkind_t subkind;
  az_script_t *on_use; // owned; NULL if no script
  az_vector_t position;
  double angle;
  int uuid_slot; // 0 if none, otherwise from 1 to AZ_NUM_UUID_SLOTS inclusive
} az_node_spec_t;

typedef struct {
  az_wall_kind_t kind;
  const az_wall_data_t *data;
  az_vector_t position;
  double angle;
  int uuid_slot; // 0 if none, otherwise from 1 to AZ_NUM_UUID_SLOTS inclusive
} az_wall_spec_t;

// Bitset of flags dictating special room behavior:
typedef uint_fast8_t az_room_flags_t;
// HEATED: The room is hot, and deals damage unless you have Thermal Armor.
#define AZ_ROOMF_HEATED      ((az_room_flags_t)(1u << 1))
// UNMAPPED: The room is not included in the map data for the room's zone.
#define AZ_ROOMF_UNMAPPED    ((az_room_flags_t)(1u << 2))
// MARK_IF_CLR: The room should have a map marker if the marker_flag flag is
// not set, and the room is visible on the minimap.
#define AZ_ROOMF_MARK_IF_CLR ((az_room_flags_t)(1u << 3))
// MARK_IF_SET: The room should have a map marker if the marker_flag flag is
// set, even if the room isn't visible on the minimap.
#define AZ_ROOMF_MARK_IF_SET ((az_room_flags_t)(1u << 0))
// WITH_*: The room is labelled on the map as containing the given console.
#define AZ_ROOMF_WITH_COMM   ((az_room_flags_t)(1u << 4))
#define AZ_ROOMF_WITH_REFILL ((az_room_flags_t)(1u << 5))
#define AZ_ROOMF_WITH_SAVE   ((az_room_flags_t)(1u << 6))

// Represents one room of the planetoid.  This sturct owns all of its pointers.
typedef struct {
  az_zone_key_t zone_key;
  az_room_flags_t properties;
  az_flag_t marker_flag; // if MARKER, show marker until this flag is set
  az_camera_bounds_t camera_bounds;
  az_script_t *on_start; // NULL if no script
  az_background_pattern_t background_pattern;
  // Initial room objects:
  int num_baddies;
  az_baddie_spec_t *baddies;
  int num_doors;
  az_door_spec_t *doors;
  int num_gravfields;
  az_gravfield_spec_t *gravfields;
  int num_nodes;
  az_node_spec_t *nodes;
  int num_walls;
  az_wall_spec_t *walls;
} az_room_t;

// Attempt to open the file located at the given path and load room data from
// it.  Returns true on success, false on failure.
bool az_load_room_from_path(const char *filepath, az_room_t *room_out);

// Attempt to save a room to the file located at the given path.  Return true
// on success, or false on failure.
bool az_save_room_to_path(const az_room_t *room, const char *filepath);

// Delete the data arrays owned by a room (but not the room object itself).
void az_destroy_room(az_room_t *room);

// Get the position that is (roughly) the center of the room.
az_vector_t az_room_center(const az_room_t *room);

// Return true if the room should be displayed on the minimap.
bool az_test_room_mapped(const az_player_t *player, az_room_key_t room_key,
                         const az_room_t *room);

// Return true if the room should have a map marker on the minimap.
bool az_should_mark_room(const az_player_t *player, az_room_key_t room_key,
                         const az_room_t *room);

/*===========================================================================*/

#endif // AZIMUTH_STATE_ROOM_H_
