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

#include "azimuth/view/space.h"

#include <assert.h>

#include <GL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/state/space.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/baddie.h"
#include "azimuth/view/door.h"
#include "azimuth/view/hud.h"
#include "azimuth/view/node.h"
#include "azimuth/view/particle.h"
#include "azimuth/view/pickup.h"
#include "azimuth/view/projectile.h"
#include "azimuth/view/wall.h"
#include "azimuth/view/ship.h"

/*===========================================================================*/

static void draw_camera_view(az_space_state_t *state) {
  az_draw_walls(state);
  az_draw_nodes(state);
  az_draw_pickups(state);
  az_draw_projectiles(state);
  az_draw_baddies(state);
  az_draw_ship(state);
  az_draw_particles(state);
  az_draw_doors(state);
}

void az_space_draw_screen(az_space_state_t *state) {
  // Check if we're in a mode where we should be tinting the camera view black.
  double fade_alpha = 0.0;
  if (state->mode == AZ_MODE_DOORWAY) {
    switch (state->mode_data.doorway.step) {
      case AZ_DWS_FADE_OUT:
        fade_alpha = state->mode_data.doorway.progress;
        break;
      case AZ_DWS_SHIFT:
        fade_alpha = 1.0;
        break;
      case AZ_DWS_FADE_IN:
        fade_alpha = 1.0 - state->mode_data.doorway.progress;
        break;
    }
  } else if (state->mode == AZ_MODE_GAME_OVER &&
             state->mode_data.game_over.step == AZ_GOS_FADE_OUT) {
    fade_alpha = state->mode_data.game_over.progress;
  }
  assert(fade_alpha >= 0.0);
  assert(fade_alpha <= 1.0);

  // Draw the camera view.
  glPushMatrix(); {
    // Make positive Y be up instead of down.
    glScaled(1, -1, 1);
    // Center the screen on position (0, 0).
    glTranslated(AZ_SCREEN_WIDTH/2, -AZ_SCREEN_HEIGHT/2, 0);
    // Move the screen to the camera pose.
    glTranslated(0, -az_vnorm(state->camera), 0);
    glRotated(90.0 - AZ_RAD2DEG(az_vtheta(state->camera)), 0, 0, 1);
    // Draw what the camera sees.
    if (fade_alpha < 1.0) draw_camera_view(state);
    if (state->mode == AZ_MODE_DOORWAY) {
      // TODO: draw door transition
    }
  } glPopMatrix();

  // Tint the camera view black (based on fade_alpha).
  if (fade_alpha > 0.0) {
    glColor4f(0, 0, 0, fade_alpha);
    glBegin(GL_QUADS); {
      glVertex2i(0, 0);
      glVertex2i(0, AZ_SCREEN_HEIGHT);
      glVertex2i(AZ_SCREEN_WIDTH, AZ_SCREEN_HEIGHT);
      glVertex2i(AZ_SCREEN_WIDTH, 0);
    } glEnd();
  }

  az_draw_hud(state);
}

/*===========================================================================*/
