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

#include "azimuth/tick/projectile.h"

#include <assert.h>
#include <math.h> // for INFINITY

#include "azimuth/state/projectile.h"
#include "azimuth/state/space.h"
#include "azimuth/state/uid.h"
#include "azimuth/tick/pickup.h" // for az_add_random_pickup
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"

/*===========================================================================*/

static void try_open_door(az_space_state_t *state, az_door_t *door,
                          az_damage_flags_t damage_kind) {
  assert(door->kind != AZ_DOOR_NOTHING);
  if (az_can_open_door(door->kind, damage_kind) && !door->is_open) {
    door->is_open = true;
    az_play_sound(&state->soundboard, AZ_SND_DOOR_OPEN);
  }
}

// Common projectile impact code, called by both on_projectile_hit_wall and
// on_projectile_hit_target.
static void on_projectile_impact(az_space_state_t *state,
                                 az_projectile_t *proj, az_vector_t normal) {
  assert(proj->kind != AZ_PROJ_NOTHING);
  const double radius = proj->data->splash_radius;
  // If applicable, deal splash damage.
  if (radius > 0.0 && proj->data->splash_damage > 0.0) {
    // Damage the ship if it's within the blast.
    if (az_ship_is_present(&state->ship) &&
        az_vwithin(state->ship.position, proj->position, radius)) {
      az_damage_ship(state, proj->data->splash_damage);
    }
    // Damage baddies that are within the blast.
    AZ_ARRAY_LOOP(baddie, state->baddies) {
      if (baddie->kind == AZ_BAD_NOTHING) continue;
      // TODO: This isn't a good condition on which to do splash damage.  We
      //       need an az_circle_touches_baddie function, or whatever.
      if (az_vwithin(baddie->position, proj->position,
                     radius + baddie->data->overall_bounding_radius)) {
        az_try_damage_baddie(state, baddie, &baddie->data->main_body,
                             proj->data->damage_kind,
                             proj->data->splash_damage);
      }
    }
    // Open doors that are within the blast.
    if (!proj->fired_by_enemy) {
      AZ_ARRAY_LOOP(door, state->doors) {
        if (door->kind == AZ_DOOR_NOTHING) continue;
        if (az_circle_touches_door_outside(door, radius, proj->position)) {
          try_open_door(state, door, proj->data->damage_kind);
        }
      }
    }
  }

  // If applicable, burst into shrapnel.
  if (proj->data->shrapnel_kind != AZ_PROJ_NOTHING) {
    const double mid_theta = az_vtheta(normal);
    for (int i = -2; i <= 2; ++i) {
      const double theta = mid_theta + 0.2 * AZ_PI * (i + az_random(-.5, .5));
      az_projectile_t *shrapnel;
      if (az_insert_projectile(state, &shrapnel)) {
        az_init_projectile(shrapnel, proj->data->shrapnel_kind, false,
                           az_vadd(proj->position, az_vpolar(0.1, theta)),
                           theta);
        if (!shrapnel->data->homing) {
          shrapnel->velocity =
            az_vmul(shrapnel->velocity, az_random(0.5, 1.0));
        }
      }
    }
  }

  // Add particles.
  az_particle_t *particle;
  switch (proj->kind) {
    case AZ_PROJ_GUN_PHASE:
    case AZ_PROJ_GUN_FREEZE_PHASE:
    case AZ_PROJ_GUN_TRIPLE_PHASE:
    case AZ_PROJ_GUN_HOMING_PHASE:
    case AZ_PROJ_GUN_PHASE_BURST:
    case AZ_PROJ_GUN_PHASE_PIERCE:
      az_add_speck(state, AZ_WHITE, 1.0, proj->position,
                   az_vpolar(az_random(20, 70), az_random(0, AZ_TWO_PI)));
      break;
    default:
      if (az_insert_particle(state, &particle)) {
        particle->kind = AZ_PAR_BOOM;
        particle->color = AZ_WHITE;
        particle->position = proj->position;
        particle->velocity = AZ_VZERO;
        particle->lifetime = 0.3;
        particle->param1 = (radius <= 0.0 ? 10.0 : radius);
      }
      for (int i = 0; i < 5; ++i) {
        az_add_speck(state, AZ_WHITE, 1.0, proj->position,
                     az_vpolar(az_random(20, 70), az_random(0, AZ_TWO_PI)));
      }
      break;
  }

  // Play sound.
  switch (proj->kind) {
    case AZ_PROJ_ROCKET:
      az_play_sound(&state->soundboard, AZ_SND_EXPLODE_ROCKET);
      break;
    case AZ_PROJ_HYPER_ROCKET:
      az_play_sound(&state->soundboard, AZ_SND_EXPLODE_HYPER_ROCKET);
      break;
    case AZ_PROJ_BOMB:
      az_play_sound(&state->soundboard, AZ_SND_EXPLODE_BOMB);
      break;
    case AZ_PROJ_MEGA_BOMB:
      az_play_sound(&state->soundboard, AZ_SND_EXPLODE_MEGA_BOMB);
      break;
    default: break;
  }
}

// Called when a projectile hits a wall or a door.  Note that phased
// projectiles can't ever hit walls, but they _can_ hit doors.
static void on_projectile_hit_wall(az_space_state_t *state,
                                   az_projectile_t *proj, az_vector_t normal) {
  assert(proj->kind != AZ_PROJ_NOTHING);
  on_projectile_impact(state, proj, normal);
  // Shake the screen.
  const double shake = proj->data->impact_shake;
  if (shake > 0.0) {
    az_shake_camera(&state->camera, shake, shake * 0.75);
  }
  // Remove the projectile.  Note that we don't often call
  // on_projectile_hit_wall for phased projectiles, but when we do (e.g. for
  // hitting closed doors) we still want to remove the projetile.
  proj->kind = AZ_PROJ_NOTHING;
}

// Common projectile impact code, called by both on_projectile_hit_baddie and
// on_projectile_hit_ship.
static void on_projectile_hit_target(
    az_space_state_t *state, az_projectile_t *proj, az_vector_t normal) {
  assert(proj->kind != AZ_PROJ_NOTHING);
  on_projectile_impact(state, proj, normal);
  // Shake the screen (less than we do if the projectile hit a wall).
  const double shake = proj->data->impact_shake;
  if (shake > 0.0) {
    az_shake_camera(&state->camera, shake * 0.75, shake * 0.25);
  }
  // Remove the projectile (unless it's piercing, in which case it should
  // continue on beyond this target).
  if (!proj->data->piercing) {
    proj->kind = AZ_PROJ_NOTHING;
  }
}

// Called when a projectile hits a baddie.
static void on_projectile_hit_baddie(
    az_space_state_t *state, az_projectile_t *proj, az_baddie_t *baddie,
    const az_component_data_t *component, az_vector_t normal) {
  assert(proj->kind != AZ_PROJ_NOTHING);
  assert(!proj->fired_by_enemy);
  proj->last_hit_uid = baddie->uid;
  az_try_damage_baddie(state, baddie, component, proj->data->damage_kind,
                       proj->data->impact_damage);
  // Note that at this point, the baddie may now be dead and removed.  So we
  // can no longer use the `baddie` or `component` pointers.
  on_projectile_hit_target(state, proj, normal);
}

// Called when a projectile hits the ship.
static void on_projectile_hit_ship(
    az_space_state_t *state, az_projectile_t *proj, az_vector_t normal) {
  assert(proj->kind != AZ_PROJ_NOTHING);
  assert(proj->fired_by_enemy);
  assert(az_ship_is_present(&state->ship));
  proj->last_hit_uid = AZ_SHIP_UID;
  az_damage_ship(state, proj->data->impact_damage);
  on_projectile_hit_target(state, proj, normal);
}

static void projectile_home_in(az_space_state_t *state,
                               az_projectile_t *proj,
                               double time) {
  assert(proj->data->homing);
  const double proj_angle = az_vtheta(proj->velocity);
  // First, figure out what position we're homing in on.
  az_vector_t goal = state->ship.position;
  if (!proj->fired_by_enemy) {
    double best_dist = INFINITY;
    bool found_target = false;
    AZ_ARRAY_LOOP(baddie, state->baddies) {
      if (baddie->kind == AZ_BAD_NOTHING) continue;
      if (baddie->uid == proj->last_hit_uid) continue;
      const double dist = az_vdist(baddie->position, proj->position) +
        fabs(az_mod2pi(az_vtheta(az_vsub(baddie->position, proj->position)) -
                       proj_angle)) * 100.0;
      if (dist < best_dist) {
        best_dist = dist;
        found_target = true;
        goal = baddie->position;
      }
    }
    if (!found_target) return;
  }
  // Now, home in on the goal position.
  const double new_angle =
    az_angle_towards(proj_angle, time * 3.5,
                     az_vtheta(az_vsub(goal, proj->position)));
  proj->velocity = az_vpolar(proj->data->speed, new_angle);
  proj->angle = new_angle;
}

// Called after aging the projectile, but before doing anything else (including
// removing it if it is past its lifetime).
static void projectile_special_logic(az_space_state_t *state,
                                     az_projectile_t *proj,
                                     double time) {
  // The projectile still hasn't hit anything.  Apply kind-specific logic to
  // the projectile (e.g. homing projectiles will home in).
  switch (proj->kind) {
    case AZ_PROJ_GUN_FREEZE:
    case AZ_PROJ_GUN_CHARGED_FREEZE:
    case AZ_PROJ_GUN_FREEZE_TRIPLE:
    case AZ_PROJ_GUN_FREEZE_HOMING:
    case AZ_PROJ_GUN_FREEZE_BURST:
    case AZ_PROJ_GUN_FREEZE_SHRAPNEL:
    case AZ_PROJ_GUN_FREEZE_PIERCE:
      for (int i = (proj->kind == AZ_PROJ_GUN_CHARGED_FREEZE ? 2 : 1);
           i > 0; --i) {
        az_add_speck(state, (az_color_t){0, 255, 255, 255},
                     (proj->kind == AZ_PROJ_GUN_CHARGED_FREEZE ? 1.0 :
                      proj->kind == AZ_PROJ_GUN_FREEZE_SHRAPNEL ? 0.2 : 0.3),
                     proj->position, az_vpolar(30.0, az_random(0, AZ_TWO_PI)));
      }
      break;
    case AZ_PROJ_GUN_HOMING:
    case AZ_PROJ_GUN_CHARGED_HOMING:
    case AZ_PROJ_GUN_TRIPLE_HOMING:
      az_add_speck(state, (az_color_t){0, 0, 255, 255}, 0.2,
                   proj->position, AZ_VZERO);
      break;
    case AZ_PROJ_ROCKET:
      az_add_speck(state, (az_color_t){255, 255, 0, 255}, 1.0, proj->position,
                   az_vrotate(az_vmul(proj->velocity, -az_random(0, 0.3)),
                              (az_random(-AZ_DEG2RAD(30), AZ_DEG2RAD(30)))));
      break;
    case AZ_PROJ_HYPER_ROCKET:
      for (int i = 0; i < 6; ++i) {
        az_add_speck(state, (az_color_t){255, 255, 0, 255},
                     az_random(1.0, 2.0), proj->position,
                     az_vrotate(az_vmul(proj->velocity, -az_random(0, 0.3)),
                                (az_random(-AZ_DEG2RAD(5), AZ_DEG2RAD(5)))));
      }
      break;
    case AZ_PROJ_BOMB:
    case AZ_PROJ_MEGA_BOMB:
      if (proj->age >= proj->data->lifetime) {
        on_projectile_hit_wall(state, proj, AZ_VZERO);
      } else {
        proj->angle = az_mod2pi(proj->angle + 1.5 * time);
      }
      if (proj->kind == AZ_PROJ_MEGA_BOMB &&
          (proj->age < 2.0 ?
           (ceil(2.0 * proj->age) > ceil(2.0 * (proj->age - time))) :
           (ceil(6.0 * proj->age) > ceil(6.0 * (proj->age - time))))) {
        az_play_sound(&state->soundboard, AZ_SND_BLINK_MEGA_BOMB);
      }
      break;
    case AZ_PROJ_FIREBALL_FAST:
    case AZ_PROJ_FIREBALL_SLOW:
      {
        az_particle_t *particle;
        if (az_insert_particle(state, &particle)) {
          particle->kind = AZ_PAR_EMBER;
          particle->color = (az_color_t){255, 128, 0, 128};
          particle->position = proj->position;
          particle->velocity = AZ_VZERO;
          particle->angle = proj->angle;
          particle->lifetime = 0.1;
          particle->param1 = 5.0;
        }
      }
      break;
    default: break;
  }
}

static void tick_projectile(az_space_state_t *state, az_projectile_t *proj,
                            double time) {
  // Age the projectile, and remove it if it is expired.
  proj->age += time;
  projectile_special_logic(state, proj, time);
  if (proj->kind == AZ_PROJ_NOTHING) return;
  if (proj->age > proj->data->lifetime) {
    proj->kind = AZ_PROJ_NOTHING;
    return;
  }

  // Figure out what, if anything, the projectile hits:
  az_impact_flags_t skip_types =
    (proj->fired_by_enemy ? AZ_IMPF_BADDIE : AZ_IMPF_SHIP);
  if (proj->data->phased) skip_types |= AZ_IMPF_WALL | AZ_IMPF_DOOR_INSIDE;
  az_impact_t impact;
  az_ray_impact(state, proj->position, az_vmul(proj->velocity, time),
                skip_types, proj->last_hit_uid, &impact);

  // Move the projectile:
  proj->position = impact.position;

  // Resolve the impact (if any):
  switch (impact.type) {
    case AZ_IMP_NOTHING:
      if (proj->data->homing) {
        projectile_home_in(state, proj, time);
      }
      break;
    case AZ_IMP_BADDIE:
      on_projectile_hit_baddie(state, proj, impact.target.baddie.baddie,
                               impact.target.baddie.component, impact.normal);
      break;
    case AZ_IMP_SHIP:
      on_projectile_hit_ship(state, proj, impact.normal);
      break;
    case AZ_IMP_DOOR_OUTSIDE:
      if (!proj->fired_by_enemy) {
        try_open_door(state, impact.target.door, proj->data->damage_kind);
      }
      on_projectile_hit_wall(state, proj, impact.normal);
      break;
    case AZ_IMP_DOOR_INSIDE:
    case AZ_IMP_WALL:
      on_projectile_hit_wall(state, proj, impact.normal);
      break;
  }
}

void az_tick_projectiles(az_space_state_t *state, double time) {
  AZ_ARRAY_LOOP(proj, state->projectiles) {
    if (proj->kind == AZ_PROJ_NOTHING) continue;
    tick_projectile(state, proj, time);
  }
}

/*===========================================================================*/
