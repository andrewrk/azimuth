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
#ifndef AZIMUTH_STATE_PLANET_H_
#define AZIMUTH_STATE_PLANET_H_

#include <stdbool.h>

#include "azimuth/state/player.h" // for az_room_key_t
#include "azimuth/state/room.h"

/*===========================================================================*/

typedef struct {
  az_room_key_t start_room;
  az_vector_t start_position;
  double start_angle;
  int num_rooms;
  az_room_t *rooms;
} az_planet_t;

bool az_load_planet(const char *resource_dir, az_planet_t *planet_out);

bool az_save_planet(const az_planet_t *planet, const char *resource_dir);

// Delete the data arrays owned by a planet (but not the planet object itself).
void az_destroy_planet(az_planet_t *planet);

/*===========================================================================*/

#endif // AZIMUTH_STATE_PLANET_H_