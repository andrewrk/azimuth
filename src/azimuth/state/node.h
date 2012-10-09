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
#ifndef AZIMUTH_STATE_NODE_H_
#define AZIMUTH_STATE_NODE_H_

#include "azimuth/state/uid.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

typedef enum {
  AZ_NODE_NOTHING = 0,
  AZ_NODE_TRACTOR,
  AZ_NODE_REFILL_SHIELD,
  AZ_NODE_REFILL_AMMO,
  AZ_NODE_REFILL_ALL,
  AZ_NODE_COMM
} az_node_kind_t;

typedef struct {
  az_node_kind_t kind; // if AZ_NODE_NOTHING, this node is not present
  az_uid_t uid;
  az_vector_t position;
  double angle;
} az_node_t;

/*===========================================================================*/

#endif // AZIMUTH_STATE_NODE_H_
