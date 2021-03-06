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

#include "azimuth/view/cutscene.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <GL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/space.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/particle.h"
#include "azimuth/view/ship.h"
#include "azimuth/view/string.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static az_color_t scale_color(float r, float g, float b, az_color_t color) {
  color.r *= r;
  color.g *= g;
  color.b *= b;
  return color;
}

/*===========================================================================*/

static void draw_moving_stars_layer(
    double spacing, double scale, double speed, double total_time,
    GLfloat alpha) {
  const double modulus = AZ_SCREEN_WIDTH + spacing;
  const double scroll = fmod(total_time * speed, modulus);
  az_random_seed_t seed = {1, 1};
  glBegin(GL_LINES); {
    for (double xoff = 0.0; xoff < modulus; xoff += spacing) {
      for (double yoff = 0.0; yoff < modulus; yoff += spacing) {
        const double x =
          fmod(xoff + 3.0 * spacing * az_rand_udouble(&seed) + scroll,
               modulus);
        const double y = yoff + 3.0 * spacing * az_rand_udouble(&seed);
        glColor4f(1, 1, 1, alpha);
        glVertex2d(x, y);
        glColor4f(1, 1, 1, 0);
        glVertex2d(x - spacing * scale, y);
      }
    }
  } glEnd();
}

void az_draw_moving_starfield(double time, double speed, double scale) {
  glPushMatrix(); {
    if (speed < 0) {
      glScalef(-1, 1, 1);
      glTranslatef(-AZ_SCREEN_WIDTH, 0, 0);
      speed = -speed;
    }
    time *= speed;
    draw_moving_stars_layer(30, scale,  450, time, 0.15f);
    draw_moving_stars_layer(45, scale,  600, time, 0.25f);
    draw_moving_stars_layer(80, scale,  900, time, 0.40f);
    draw_moving_stars_layer(95, scale, 1200, time, 0.50f);
  } glPopMatrix();
}

static void draw_planet_starfield_internal(int width, az_clock_t clock) {
  const int star_spacing = 12;
  az_random_seed_t seed = {1, 1};
  glBegin(GL_POINTS); {
    int i = 0;
    for (int xoff = 0; xoff < width; xoff += star_spacing) {
      for (int yoff = 0; yoff < AZ_SCREEN_HEIGHT; yoff += star_spacing) {
        const int twinkle = az_clock_zigzag(10, 4, clock + i);
        glColor4f(1, 1, 1, (twinkle * 0.02) + 0.3 * az_rand_udouble(&seed));
        const double x = xoff + 3 * star_spacing * az_rand_udouble(&seed);
        const double y = yoff + 3 * star_spacing * az_rand_udouble(&seed);
        glVertex2d(x, y);
        ++i;
      }
    }
  } glEnd();
}

void az_draw_planet_starfield(az_clock_t clock) {
  draw_planet_starfield_internal(AZ_SCREEN_WIDTH, clock);
}

#define BASE_PLANET_RADIUS 130.0

static double planet_radius(double theta, double deform) {
  assert(0.0 <= deform && deform <= 1.0);
  return (deform <= 0.0 ? BASE_PLANET_RADIUS :
          BASE_PLANET_RADIUS * (1.0 - pow(deform, 5)) *
          (1.0 + 0.5 * pow(deform, 3) *
           (cos(17 * theta + AZ_TWO_PI * deform) +
            sin(7 * theta))));
}

static void draw_zenith_planet_internal(double blacken, double create,
                                        double deform, az_clock_t clock) {
  assert(0.0 <= blacken && blacken <= 1.0);
  assert(0.0 <= create && create <= 1.0);
  assert(0.0 <= deform && deform <= 1.0);
  glPushMatrix(); {
    glTranslated(AZ_SCREEN_WIDTH/2, AZ_SCREEN_HEIGHT/2, 0);

    if (create < 1.0) {
      // Blacken circle:
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(0, 0, 0, blacken);
        glVertex2f(0, 0);
        for (int i = 0; i <= 360; i += 3) {
          const double theta = AZ_DEG2RAD(i);
          const double radius = planet_radius(theta, deform);
          glVertex2d(radius * cos(theta), radius * sin(theta));
        }
      } glEnd();

      // Draw fragments:
      if (blacken > 0.0 && blacken < 1.0) {
        for (int i = 0; i < 360; i += 20) {
          glPushMatrix(); {
            az_gl_translated(az_vpolar(sqrt(blacken) * BASE_PLANET_RADIUS,
                                       AZ_DEG2RAD(i)));
            az_gl_rotated(2 * AZ_TWO_PI * blacken);
            glBegin(GL_TRIANGLES); {
              const double radius = 10.0 * (1.0 - blacken);
              for (int j = 0; j < 3; ++j) {
                const az_clock_t clk = clock + 2 * j;
                glColor3f((az_clock_mod(6, 1, clk)     < 3 ? 1.0f : 0.25f),
                          (az_clock_mod(6, 1, clk + 2) < 3 ? 1.0f : 0.25f),
                          (az_clock_mod(6, 1, clk + 4) < 3 ? 1.0f : 0.25f));
                glVertex2d(radius * cos(AZ_DEG2RAD(j * 120)),
                           radius * sin(AZ_DEG2RAD(j * 120)));
              }
            } glEnd();
          } glPopMatrix();
        }
      }

      // Draw tendrils:
      const double overall_progress = 0.25 * blacken + 0.75 * create;
      const int num_tendrils = 21;
      for (int i = 1; i <= num_tendrils; ++i) {
        const double progress =
          cbrt((overall_progress - (double)i / num_tendrils) /
               (1.0 - (double)i / num_tendrils));
        if (progress > 0.0) {
          const az_clock_t clk = clock + 7 * i;
          const GLfloat r = (az_clock_mod(6, 1, clk)     < 3 ? 1.0f : 0.25f);
          const GLfloat g = (az_clock_mod(6, 1, clk + 2) < 3 ? 1.0f : 0.25f);
          const GLfloat b = (az_clock_mod(6, 1, clk + 4) < 3 ? 1.0f : 0.25f);
          glColor4f(r, g, b, 0.5f);
          glBegin(GL_TRIANGLE_STRIP); {
            const double angle = AZ_DEG2RAD(108) * i;
            az_random_seed_t seed = {i, i};
            az_vector_t vec = AZ_VZERO;
            const az_vector_t step = az_vpolar(1.2, angle);
            const az_vector_t side = az_vrot90ccw(step);
            for (double j = 0.0; j <= progress; j += 0.01) {
              const az_vector_t edge =
                az_vmul(side, 3.0 * (progress - j) / progress);
              const az_vector_t wobble = {
                0, atan(0.02 * vec.x) * 10.0 *
                sin(overall_progress * 30.0 + vec.x / 10.0)};
              az_gl_vertex(az_vadd(az_vadd(vec, edge), wobble));
              az_gl_vertex(az_vadd(az_vsub(vec, edge), wobble));
              az_vpluseq(&vec, step);
              az_vpluseq(&vec, az_vmul(side, 2 * az_rand_sdouble(&seed)));
            }
          } glEnd();
        }
      }

      // Draw portal:
      glBegin(GL_TRIANGLE_FAN); {
        const GLfloat r = (az_clock_mod(6, 1, clock)     < 3 ? 1.0f : 0.25f);
        const GLfloat g = (az_clock_mod(6, 1, clock + 2) < 3 ? 1.0f : 0.25f);
        const GLfloat b = (az_clock_mod(6, 1, clock + 4) < 3 ? 1.0f : 0.25f);
        glColor4f(r, g, b, 1);
        glVertex2f(0, 0);
        glColor4f(r, g, b, 0);
        const double radius =
          blacken * (30 + 0.15 * az_clock_zigzag(90, 1, clock));
        for (int i = 0; i <= 360; i += 10) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)),
                     radius * sin(AZ_DEG2RAD(i)) * 0.8);
        }
      } glEnd();
    }

    // Draw the planet itself:
    glBegin(GL_TRIANGLE_FAN); {
      glColor4f(0.5, 0.3, 0.5, create);
      const double spot_theta = AZ_DEG2RAD(-125);
      const double spot_radius = 0.15 * planet_radius(spot_theta, deform);
      glVertex2d(spot_radius * cos(spot_theta), spot_radius * sin(spot_theta));
      glColor4f(0.25, 0.15, 0.15, create);
      for (int i = 0; i <= 360; i += 3) {
        const double theta = AZ_DEG2RAD(i);
        const double radius = planet_radius(theta, deform);
        glVertex2d(radius * cos(theta), radius * sin(theta));
      }
    } glEnd();

    // Draw planet "atmosphere":
    const double atmosphere_thickness =
      create * (18.0 + (1.0 - deform) * az_clock_zigzag(10, 8, clock));
    glBegin(GL_TRIANGLE_STRIP); {
      for (int i = 0; i <= 360; i += 3) {
        const double theta = AZ_DEG2RAD(i);
        const double radius = planet_radius(theta, deform);
        glColor4f(0, 1, 1, 0.5);
        glVertex2d(radius * cos(theta), radius * sin(theta));
        glColor4f(0, 1, 1, 0);
        glVertex2d((radius + atmosphere_thickness) * cos(theta),
                   (radius + atmosphere_thickness) * sin(theta));
      }
    } glEnd();

    if (deform > 0.0) {
      // Draw tendrils:
      const int num_tendrils = 12;
      for (int i = 1; i <= num_tendrils; ++i) {
        const double progress = 0.75 * (deform - pow(deform, 8));
        if (progress > 0.0) {
          const az_clock_t clk = clock + 7 * i;
          const GLfloat r = (az_clock_mod(6, 1, clk)     < 3 ? 1.0f : 0.25f);
          const GLfloat g = (az_clock_mod(6, 1, clk + 2) < 3 ? 1.0f : 0.25f);
          const GLfloat b = (az_clock_mod(6, 1, clk + 4) < 3 ? 1.0f : 0.25f);
          glColor4f(r, g, b, 0.5f);
          glBegin(GL_TRIANGLE_STRIP); {
            const double angle = AZ_DEG2RAD(108) * i;
            az_random_seed_t seed = {i, i};
            az_vector_t vec = AZ_VZERO;
            const az_vector_t step = az_vpolar(2.5, angle);
            const az_vector_t side = az_vrot90ccw(step);
            for (double j = 0.0; j <= progress; j += 0.01) {
              const az_vector_t edge =
                az_vmul(side, 3.0 * (progress - j) / progress);
              const az_vector_t wobble = {
                0, atan(0.02 * vec.x) * 10.0 *
                sin(deform * 20.0 + vec.x / 20.0)};
              az_gl_vertex(az_vadd(az_vadd(vec, edge), wobble));
              az_gl_vertex(az_vadd(az_vsub(vec, edge), wobble));
              az_vpluseq(&vec, step);
              az_vpluseq(&vec, az_vmul(side, 2 * az_rand_sdouble(&seed)));
            }
          } glEnd();
        }
      }

      // Draw portal:
      glBegin(GL_TRIANGLE_FAN); {
        const GLfloat r = (az_clock_mod(6, 1, clock)     < 3 ? 1.0f : 0.25f);
        const GLfloat g = (az_clock_mod(6, 1, clock + 2) < 3 ? 1.0f : 0.25f);
        const GLfloat b = (az_clock_mod(6, 1, clock + 4) < 3 ? 1.0f : 0.25f);
        glColor4f(r, g, b, 1);
        glVertex2f(0, 0);
        glColor4f(r, g, b, 0);
        const double radius = 2 * (1 - deform - pow(1 - deform, 4)) *
          (60 + 0.15 * az_clock_zigzag(30, 1, clock));
        for (int i = 0; i <= 360; i += 10) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)),
                     radius * sin(AZ_DEG2RAD(i)) * 0.8);
        }
      } glEnd();
    }
  } glPopMatrix();
}

void az_draw_zenith_planet(az_clock_t clock) {
  draw_zenith_planet_internal(0, 1, 0, clock);
}

void az_draw_zenith_planet_formation(double blacken, double create,
                                     az_clock_t clock) {
  draw_zenith_planet_internal(blacken, create, 0.0, clock);
}

void az_draw_planet_debris(az_clock_t clock) {
  const az_color_t color = az_color3f(0.5, 0.35, 0.45);
  az_random_seed_t seed = {1, 1};
  for (int i = 0; i < 25; ++i) {
    const double cx = 320 + 300 * az_rand_sdouble(&seed);
    const double cy = 240 + 200 * az_rand_sdouble(&seed);
    const double size = 2 + 2 * az_rand_udouble(&seed);
    const double angle = AZ_PI * az_rand_sdouble(&seed);
    const double spin = AZ_DEG2RAD(30) * az_rand_sdouble(&seed);
    const az_particle_t particle = {
      .kind = AZ_PAR_ROCK,
      .color = color,
      .age = 0.5,
      .lifetime = 1,
      .param1 = size
    };
    glPushMatrix(); {
      glTranslated(cx, cy, 0);
      az_gl_rotated(angle + spin * (clock / 60.0));
      az_draw_particle(&particle, clock);
    } glPopMatrix();
  }
}

static void tint_screen(GLfloat gray, GLfloat alpha) {
  glColor4f(gray, gray, gray, alpha);
  glBegin(GL_QUADS); {
    glVertex2i(0, 0);
    glVertex2i(0, AZ_SCREEN_HEIGHT);
    glVertex2i(AZ_SCREEN_WIDTH, AZ_SCREEN_HEIGHT);
    glVertex2i(AZ_SCREEN_WIDTH, 0);
  } glEnd();
}

static void draw_sapiai_planet_atmosphere(
    double surface_radius, double atmosphere_thickness,
    az_color_t atmosphere_color) {
  az_color_t outer_color = atmosphere_color;
  outer_color.a = 0;
  const double outer_radius = surface_radius + atmosphere_thickness;
  glBegin(GL_TRIANGLE_STRIP); {
    for (int i = 0; i <= 360; i += 3) {
      az_gl_color(atmosphere_color);
      glVertex2d(surface_radius * cos(AZ_DEG2RAD(i)),
                 surface_radius * sin(AZ_DEG2RAD(i)));
      az_gl_color(outer_color);
      glVertex2d(outer_radius * cos(AZ_DEG2RAD(i)),
                 outer_radius * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
}

static void draw_gas_planet_stripe(
    double radius, int min_angle, int max_angle,
    az_color_t min_color, az_color_t max_color) {
  assert(max_angle > min_angle);
  for (int sign = -1; sign <= 1; sign += 2) {
    glBegin(GL_TRIANGLE_STRIP); {
      for (int i = min_angle; i <= max_angle; ++i) {
        const double transition =
          (double)(i - min_angle) / (double)(max_angle - min_angle);
        const az_color_t base_color =
          az_transition_color(min_color, max_color, transition);
        const float factor = (float)i / 90.0f;
        az_gl_color(scale_color(0.5f, 0.35f, 0.5f, base_color));
        glVertex2d(sign * radius * cos(AZ_DEG2RAD(i)),
                   -radius * sin(AZ_DEG2RAD(i)));
        az_gl_color(scale_color(1.0f - factor * 0.5f, 1.0f - factor * 0.65f,
                                1.0f - factor * 0.5f, base_color));
        glVertex2d(0, -radius * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
  }
}

/*===========================================================================*/

static void draw_particle(const az_particle_t *particle, az_clock_t clock) {
  if (particle->kind == AZ_PAR_NOTHING) return;
  glPushMatrix(); {
    az_gl_translated(particle->position);
    az_gl_rotated(particle->angle);
    az_draw_particle(particle, clock);
  } glPopMatrix();
}

static void draw_bg_particles(
    const az_cutscene_state_t *cutscene, az_clock_t clock) {
  AZ_ARRAY_LOOP(particle, cutscene->bg_particles) {
    draw_particle(particle, clock);
  }
}

static void draw_fg_particles(
    const az_cutscene_state_t *cutscene, az_clock_t clock) {
  AZ_ARRAY_LOOP(particle, cutscene->fg_particles) {
    draw_particle(particle, clock);
  }
}

/*===========================================================================*/

static void draw_cruising_scene(
    const az_cutscene_state_t *cutscene, az_clock_t clock) {
  assert(cutscene->scene == AZ_SCENE_CRUISING);
  az_draw_moving_starfield(cutscene->time, 1.0, 1.0);
  az_ship_t ship = {
    .position = {320, 240}, .angle = AZ_PI, .thrusters = AZ_THRUST_FORWARD
  };
  az_draw_ship_body(&ship, clock);
}

static void draw_move_out_scene(
    const az_cutscene_state_t *cutscene, az_clock_t clock) {
  assert(cutscene->scene == AZ_SCENE_MOVE_OUT);
  const double accel = cutscene->param1;
  az_draw_moving_starfield(cutscene->time, 1.0, 1.0 + 5.0 * accel * accel);
  az_ship_t ship = {
    .position = {320 - 350 * (2 * accel * accel - accel), 240},
    .angle = AZ_PI, .thrusters = AZ_THRUST_FORWARD
  };
  az_draw_ship_body(&ship, clock);
  if (cutscene->param2 > 0.0) tint_screen(1, cutscene->param2);
}

static void draw_arrival_scene(
    const az_cutscene_state_t *cutscene, az_clock_t clock) {
  assert(cutscene->scene == AZ_SCENE_ARRIVAL);
  az_draw_planet_starfield(clock);
  az_draw_zenith_planet(clock);
  glPushMatrix(); {
    const az_vector_t ctrl0 = {700, 20}, ctrl1 = {500, 600};
    const az_vector_t ctrl2 = {-100, 420}, ctrl3 = {220, 290};
    const double t = 1.0 - exp(-7.0 * cutscene->param1);
    const double s = 1.0 - t;
    const az_vector_t pos =
      az_vadd(az_vadd(az_vmul(ctrl0, s*s*s),
                      az_vmul(ctrl1, 3*s*s*t)),
              az_vadd(az_vmul(ctrl2, 3*s*t*t),
                      az_vmul(ctrl3, t*t*t)));
    const double angle = az_vtheta(
      az_vadd(az_vadd(az_vmul(ctrl0, -3*s*s),
                      az_vmul(ctrl1, 3*s*s - 6*s*t)),
              az_vadd(az_vmul(ctrl2, 6*s*t - 3*t*t),
                      az_vmul(ctrl3, 3*t*t))));
    glTranslatef(pos.x, pos.y, 0);
    const double scale = fmax(0.05, exp(1.5 - 12.0 * cutscene->param1));
    glScaled(scale, scale, 1);
    az_ship_t ship = {
      .position = {0, 0}, .angle = angle, .thrusters = AZ_THRUST_FORWARD
    };
    az_draw_ship_body(&ship, clock);
  } glPopMatrix();
}

static void draw_zenith_scene(
    const az_cutscene_state_t *cutscene, az_clock_t clock) {
  assert(cutscene->scene == AZ_SCENE_ZENITH);
  az_draw_planet_starfield(clock);
  az_draw_zenith_planet(clock);
}

static void draw_escape_scene(
    const az_cutscene_state_t *cutscene, az_clock_t clock) {
  assert(cutscene->scene == AZ_SCENE_ESCAPE);
  az_draw_planet_starfield(clock);
  draw_bg_particles(cutscene, clock);
  if (cutscene->step == 0) {
    // Step 0: Deform planet
    draw_zenith_planet_internal(0, 1, cutscene->step_progress, clock);
  } else if (cutscene->step == 1) {
    // Step 1: Asplode planet, with ship escaping
    glPushMatrix(); {
      // Explosion:
      glTranslated(AZ_SCREEN_WIDTH/2, AZ_SCREEN_HEIGHT/2, 0);
      glBegin(GL_TRIANGLE_FAN); {
        const double blast = pow(fmin(1.0, 3 * cutscene->step_progress), 3);
        glColor4f(blast, 1, 1, 0.5 + 0.5 * blast);
        glVertex2f(0, 0);
        glColor4f(blast, 1, 1, blast);
        for (int i = 0; i <= 360; i += 3) {
          const double theta = AZ_DEG2RAD(i);
          const double radius = 18.0 + blast * AZ_SCREEN_RADIUS;
          glVertex2d(1.5 * radius * cos(theta), radius * sin(theta));
        }
      } glEnd();
      // Escaping ship shadow:
      glPushMatrix(); {
        const double translation = 1.2 * pow(cutscene->step_progress, 4);
        glTranslated(-320 * translation, 270 * translation, 0);
        const GLfloat scale_factor = 3.0 * translation;
        glScalef(scale_factor, 0.66f * scale_factor, 1);
        az_gl_rotated(AZ_DEG2RAD(125));
        glColor3f(0, 0, 0);
        // Engines:
        glBegin(GL_QUADS); {
          // Struts:
          glVertex2f( 1,  9); glVertex2f(-7,  9);
          glVertex2f(-7, -9); glVertex2f( 1, -9);
          // Port engine:
          glVertex2f(-10,  12); glVertex2f(  6,  12);
          glVertex2f(  8,   7); glVertex2f(-11,   7);
          // Starboard engine:
          glVertex2f(  8,  -7); glVertex2f(-11,  -7);
          glVertex2f(-10, -12); glVertex2f(  6, -12);
        } glEnd();
        // Main body:
        glBegin(GL_TRIANGLE_FAN); {
          glVertex2f(20, 0); glVertex2f( 15,  4); glVertex2f(-14,  4);
          glVertex2f(-15,  3); glVertex2f(-16,  0); glVertex2f(-15, -3);
          glVertex2f(-14, -4); glVertex2f( 15, -4);
        } glEnd();
      } glPopMatrix();
    } glPopMatrix();
  } else if (cutscene->step == 2) {
    // Step 2: Explosion fades away
    glBegin(GL_TRIANGLE_FAN); {
      glColor4f(1, 1, 1, 1.0 - cutscene->step_progress);
      glVertex2i(0, 0); glVertex2i(AZ_SCREEN_WIDTH, 0);
      glVertex2i(AZ_SCREEN_WIDTH, AZ_SCREEN_HEIGHT);
      glVertex2i(0, AZ_SCREEN_HEIGHT);
    } glEnd();
  }
  draw_fg_particles(cutscene, clock);
}

static void draw_oth_scene(const az_cutscene_state_t *cutscene,
                           az_clock_t clock) {
  assert(cutscene->scene == AZ_SCENE_OTH);
  az_draw_planet_starfield(clock);
  draw_bg_particles(cutscene, clock);
  glPushMatrix(); {
    glTranslated(AZ_SCREEN_WIDTH/2, AZ_SCREEN_HEIGHT/2, 0);
    const double progress =
      0.7 + 0.3 * tanh(0.25 * cutscene->step_timer - 3.0);
    // Draw tendrils:
    const int num_tendrils = 12;
    for (int i = 1; i <= num_tendrils; ++i) {
      if (progress > 0.0) {
        const az_clock_t clk = clock + 7 * i;
        const GLfloat r = (az_clock_mod(6, 1, clk)     < 3 ? 0.75f : 0.25f);
        const GLfloat g = (az_clock_mod(6, 1, clk + 2) < 3 ? 0.75f : 0.25f);
        const GLfloat b = (az_clock_mod(6, 1, clk + 4) < 3 ? 0.75f : 0.25f);
        glColor4f(r, g, b, 0.5f);
        glBegin(GL_TRIANGLE_STRIP); {
          const double angle = AZ_DEG2RAD(108) * i;
          az_random_seed_t seed = {i, i};
          az_vector_t vec = AZ_VZERO;
          const az_vector_t step = az_vpolar(2 + 3 * progress, angle);
          const az_vector_t side = az_vrot90ccw(step);
          for (double j = 0.0; j <= progress; j += 0.01) {
            const az_vector_t edge =
              az_vmul(side, 2.0 * (progress - j) / progress);
            const az_vector_t wobble = {
              0, atan(0.02 * vec.x) * 10.0 *
              sin(cutscene->step_timer * 2.0 + vec.x / 20.0)};
            az_gl_vertex(az_vadd(az_vadd(vec, edge), wobble));
            az_gl_vertex(az_vadd(az_vsub(vec, edge), wobble));
            az_vpluseq(&vec, step);
            az_vpluseq(&vec, az_vmul(side, 2 * az_rand_sdouble(&seed)));
          }
        } glEnd();
      }
    }
    // Draw portal:
    glBegin(GL_TRIANGLE_FAN); {
      const GLfloat r = (az_clock_mod(6, 1, clock)     < 3 ? 1.0f : 0.25f);
      const GLfloat g = (az_clock_mod(6, 1, clock + 2) < 3 ? 1.0f : 0.25f);
      const GLfloat b = (az_clock_mod(6, 1, clock + 4) < 3 ? 1.0f : 0.25f);
      glColor4f(r, g, b, 1);
      glVertex2f(0, 0);
      glColor4f(r, g, b, 0);
      const double radius =
        progress * (80 + 0.25 * az_clock_zigzag(90, 1, clock));
      for (int i = 0; i <= 360; i += 10) {
        glVertex2d(radius * cos(AZ_DEG2RAD(i)),
                   radius * sin(AZ_DEG2RAD(i)) * 0.8);
      }
    } glEnd();
  } glPopMatrix();
  draw_fg_particles(cutscene, clock);
}

static void draw_sapiais_scene(
    const az_cutscene_state_t *cutscene, az_clock_t clock) {
  assert(cutscene->scene == AZ_SCENE_SAPIAIS);
  az_draw_planet_starfield(clock);
  const double glow = 0.02 * az_clock_zigzag(50, 1, clock);
  // Sun:
  const az_vector_t sun_position = {500, 50};
  glPushMatrix(); {
    az_gl_translated(sun_position);
    const double radius1 = 10;
    const double radius2 = 40;
    const double radius3 = 300 + 10 * glow;
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(1, 1, 1); glVertex2d(0, 0);
      for (int i = 0; i <= 360; i += 3) {
        glVertex2d(radius1 * cos(AZ_DEG2RAD(i)), radius1 * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
    glBegin(GL_TRIANGLE_STRIP); {
      for (int i = 0; i <= 360; i += 3) {
        glColor3f(1, 1, 1);
        glVertex2d(radius1 * cos(AZ_DEG2RAD(i)), radius1 * sin(AZ_DEG2RAD(i)));
        glColor4f(1, 1, 1, 0.7);
        glVertex2d(radius2 * cos(AZ_DEG2RAD(i)), radius2 * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
    glBegin(GL_TRIANGLE_STRIP); {
      for (int i = 0; i <= 360; i += 3) {
        glColor4f(1, 1, 1, 0.7);
        glVertex2d(radius2 * cos(AZ_DEG2RAD(i)), radius2 * sin(AZ_DEG2RAD(i)));
        glColor4f(1, 1, 1, 0);
        glVertex2d(radius3 * cos(AZ_DEG2RAD(i)), radius3 * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
  } glPopMatrix();
  // Small planet:
  const az_vector_t small_planet_position = {200, 150};
  glPushMatrix(); {
    az_gl_translated(small_planet_position);
    const double radius = 80;
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0.5, 0.5, 0.55); glVertex2d(40, -40);
      glColor3f(0.3, 0.3, 0.35);
      for (int i = 0; i <= 360; i += 3) {
        glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
    draw_sapiai_planet_atmosphere(radius, 10 + 5 * glow,
                                  (az_color_t){192, 192, 255, 128});
  } glPopMatrix();
  // Large planet:
  const az_vector_t large_planet_position = {570, 600};
  glPushMatrix(); {
    az_gl_translated(large_planet_position);
    const double radius = 300;
    draw_sapiai_planet_atmosphere(radius, 30 + 10 * glow,
                                  (az_color_t){255, 192, 192, 128});
    az_gl_rotated(az_vtheta(az_vsub(sun_position, large_planet_position)) +
                  AZ_HALF_PI);
    az_color_t stripe1 = {212, 212, 148, 255};
    az_color_t stripe2 = {212, 148, 148, 255};
    az_color_t stripe3 = {148, 148,  84, 255};
    az_color_t stripe4 = {148, 148, 148, 255};
    draw_gas_planet_stripe(radius, 20, 30, stripe1, stripe2);
    draw_gas_planet_stripe(radius, 30, 40, stripe2, stripe3);
    draw_gas_planet_stripe(radius, 40, 50, stripe3, stripe2);
    draw_gas_planet_stripe(radius, 50, 60, stripe2, stripe4);
    draw_gas_planet_stripe(radius, 60, 90, stripe4, stripe1);
  } glPopMatrix();
}

/*===========================================================================*/

static void draw_uhp_cruiser_front_half(bool lit, double door_openness) {
  // Dorsal fin:
  glBegin(GL_TRIANGLE_STRIP); {
    glColor3f(0.15, 0.15, 0.15); glVertex2f(-135,  90); glVertex2f( 10,  90);
    glColor3f(0.23, 0.23, 0.23); glVertex2f(-130,  60); glVertex2f( 40,  60);
  } glEnd();
  // Fighter bay:
  glBegin(GL_TRIANGLE_STRIP); {
    glColor4f(0.3, 1, 1, 0.3); glVertex2f(80, -65); glVertex2f(160, -63);
    glColor4f(0.3, 1, 1, 0.0); glVertex2f(80, -70); glVertex2f(160, -68);
  } glEnd();
  glPushMatrix(); {
    glTranslatef(-40 * door_openness, 0, 0);
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0.23, 0.23, 0.23); glVertex2f(65, -60); glVertex2f(120, -59);
      glColor3f(0.20, 0.20, 0.20); glVertex2f(70, -70); glVertex2f(120, -69);
    } glEnd();
  } glPopMatrix();
  glPushMatrix(); {
    glTranslatef(40 * door_openness, 0, 0);
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0.23, 0.23, 0.23); glVertex2f(120, -59); glVertex2f(175, -58);
      glColor3f(0.20, 0.20, 0.20); glVertex2f(120, -69); glVertex2f(170, -68);
    } glEnd();
  } glPopMatrix();
  // Body:
  glBegin(GL_TRIANGLE_STRIP); {
    glColor3f(0.25, 0.25, 0.25); glVertex2f(-25,  60); glVertex2f( 80,  60);
    glColor3f(0.50, 0.50, 0.50); glVertex2f(  0,  30); glVertex2f(210,  30);
    glColor3f(0.70, 0.70, 0.70); glVertex2f(-60, -30); glVertex2f(300, -30);
    glColor3f(0.40, 0.40, 0.40); glVertex2f(-50, -60); glVertex2f(295, -60);
    glColor3f(0.25, 0.25, 0.25); glVertex2f(-75, -68); glVertex2f(-50, -70);
  } glEnd();
  // Windows:
  glBegin(GL_TRIANGLES); {
    if (lit) glColor3f(0, 0.65, 0.65);
    else glColor3f(0, 0.325, 0.325);
    glVertex2f(213, 22); glVertex2f(209, 14); glVertex2f(246, 0);
    if (lit) glColor3f(0, 0.6, 0.6);
    else glColor3f(0, 0.3, 0.3);
    glVertex2f(155, 37); glVertex2f(208, 25); glVertex2f(203, 17);
    if (lit) glColor3f(0, 0.5, 0.5);
    else glColor3f(0, 0.25, 0.25);
    glVertex2f(80, 55); glVertex2f(90, 40); glVertex2f(145, 40);
    glVertex2f(70, 55); glVertex2f(80, 40); glVertex2f(50, 40);
    glVertex2f(60, 55); glVertex2f(40, 40); glVertex2f(35, 55);
    glVertex2f(27, 55); glVertex2f(32, 40); glVertex2f(15, 40);
  } glEnd();
  // Text:
  glPushMatrix(); {
    glScalef(1.5, -1, 1);
    glColor4f(0, 0.25, 0.25, 0.3);
    az_draw_string(5, AZ_ALIGN_LEFT, -110, -10, "                   S");
    az_draw_string(7, AZ_ALIGN_LEFT, -110, 10, "           NE");
  } glPopMatrix();
}

static void draw_uhp_cruiser_rear_half(bool exhaust, az_clock_t clock) {
  // Rear exhaust:
  if (exhaust) {
    glBegin(GL_TRIANGLE_STRIP); {
      const float zig = az_clock_zigzag(10, 2, clock);
      glColor4f(1, 0.25, 0, 0.0); glVertex2f(-275, 20);
      glColor4f(1, 0.75, 0, 0.9); glVertex2f(-275, 0);
      glColor4f(1, 0.25, 0, 0.0); glVertex2f(-330 - zig, 0);
                                  glVertex2f(-275, -20);
    } glEnd();
    glBegin(GL_TRIANGLE_STRIP); {
      const float zig = az_clock_zigzag(10, 2, clock);
      glColor4f(1, 0.25, 0, 0.0); glVertex2f(-280, -10);
      glColor4f(1, 0.75, 0, 0.9); glVertex2f(-280, -30);
      glColor4f(1, 0.25, 0, 0.0); glVertex2f(-345 - zig, -30);
                                  glVertex2f(-280, -50);
    } glEnd();
  }
  // Rear engines:
  glBegin(GL_TRIANGLE_STRIP); {
    glColor3f(0.20, 0.20, 0.20); glVertex2f(-275,  20); glVertex2f(-250,  20);
    glColor3f(0.35, 0.35, 0.35); glVertex2f(-275,   0); glVertex2f(-250,   0);
    glColor3f(0.15, 0.15, 0.15); glVertex2f(-275, -20); glVertex2f(-250, -20);
  } glEnd();
  glBegin(GL_TRIANGLE_STRIP); {
    glColor3f(0.20, 0.20, 0.20); glVertex2f(-280, -10); glVertex2f(-250, -10);
    glColor3f(0.35, 0.35, 0.35); glVertex2f(-280, -30); glVertex2f(-250, -30);
    glColor3f(0.15, 0.15, 0.15); glVertex2f(-280, -50); glVertex2f(-250, -50);
  } glEnd();
  // Body:
  glBegin(GL_TRIANGLE_STRIP); {
    glColor3f(0.25, 0.25, 0.25); glVertex2f(-150,  60); glVertex2f(-25,  60);
    glColor3f(0.50, 0.50, 0.50); glVertex2f(-250,  30); glVertex2f(  0,  30);
    glColor3f(0.70, 0.70, 0.70); glVertex2f(-250, -30); glVertex2f(-60, -30);
    glColor3f(0.40, 0.40, 0.40); glVertex2f(-250, -60); glVertex2f(-50, -60);
    glColor3f(0.25, 0.25, 0.25); glVertex2f(-100, -68); glVertex2f(-75, -70);
  } glEnd();
  // Text:
  glPushMatrix(); {
    glScalef(1.5, -1, 1);
    glColor4f(0, 0.25, 0.25, 0.3);
    az_draw_string(5, AZ_ALIGN_LEFT, -110, -10, "UNITED HUMAN PLANET ");
    az_draw_string(7, AZ_ALIGN_LEFT, -110, 10, "HLS MELPOMEN ");
    az_draw_string(7, AZ_ALIGN_LEFT, -105.5, 10.5, "...");
  } glPopMatrix();
  // Side exhaust:
  if (exhaust) {
    glBegin(GL_TRIANGLE_STRIP); {
      const float zig = az_clock_zigzag(10, 2, clock);
      glColor4f(1, 0.25, 0, 0.0); glVertex2f(-150, -52);
      glColor4f(1, 0.75, 0, 0.9); glVertex2f(-150, -65);
      glColor4f(1, 0.25, 0, 0.0); glVertex2f(-220 - zig, -65);
                                  glVertex2f(-150, -78);
    } glEnd();
  }
  // Side engine:
  glBegin(GL_TRIANGLE_STRIP); {
    glColor3f(0.35, 0.35, 0.35); glVertex2f(-110, -40); glVertex2f(-30, -40);
    glColor3f(0.50, 0.50, 0.50); glVertex2f(-150, -50); glVertex2f(  0, -50);
    glColor3f(0.75, 0.75, 0.75); glVertex2f(-150, -65); glVertex2f(  0, -65);
    glColor3f(0.40, 0.40, 0.40); glVertex2f(-150, -80); glVertex2f(  0, -80);
    glColor3f(0.20, 0.20, 0.20); glVertex2f(-110, -90); glVertex2f(-30, -90);
  } glEnd();
}

static void draw_uhp_cruiser(double door_openness, az_clock_t clock) {
  draw_uhp_cruiser_front_half(true, door_openness);
  draw_uhp_cruiser_rear_half(true, clock);
}

static void draw_uhp_fighter(az_clock_t clock) {
  // Body:
  glBegin(GL_TRIANGLE_STRIP); {
    glColor3f(0.25, 0.25, 0.25); glVertex2f(-15,  4); glVertex2f( 8,  4);
    glColor3f(0.70, 0.70, 0.70); glVertex2f(-15, -1); glVertex2f(20, -1);
    glColor3f(0.25, 0.25, 0.25); glVertex2f(-15, -5); glVertex2f(11, -5);
  } glEnd();
  // Tail:
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(0.70, 0.70, 0.70); glVertex2f(-15, -1);
    glColor3f(0.25, 0.25, 0.25); glVertex2f(-15,  4); glVertex2f(-20,  2);
    glColor3f(0.70, 0.70, 0.70); glVertex2f(-20, -1);
    glColor3f(0.25, 0.25, 0.25); glVertex2f(-20, -4); glVertex2f(-15, -5);
  } glEnd();
  // Windshield:
  glBegin(GL_TRIANGLE_STRIP); {
    glColor3f(0, 1, 1); glVertex2f(17, 0);
    glColor3f(0, 0.5, 0.5); glVertex2f(8, 4); glVertex2f(12, -1);
  } glEnd();
  // Side exhaust:
  glBegin(GL_TRIANGLE_STRIP); {
    const float zig = az_clock_zigzag(10, 1, clock);
    glColor4f(1, 0.50, 0, 0.0); glVertex2f(-16, 0);
    glColor4f(1, 0.75, 0, 0.9); glVertex2f(-16, -4);
    glColor4f(1, 0.50, 0, 0.0); glVertex2f(-30 - zig, -4); glVertex2f(-16, -8);
  } glEnd();
  // Side engine:
  glBegin(GL_TRIANGLE_STRIP); {
    glColor3f(0.30, 0.30, 0.30); glVertex2f(-12,  0); glVertex2f(-2,  0);
    glColor3f(0.50, 0.50, 0.50); glVertex2f(-16, -1); glVertex2f( 2, -1);
    glColor3f(0.75, 0.75, 0.75); glVertex2f(-16, -4); glVertex2f( 2, -4);
    glColor3f(0.40, 0.40, 0.40); glVertex2f(-16, -7); glVertex2f( 2, -7);
    glColor3f(0.20, 0.20, 0.20); glVertex2f(-12, -8); glVertex2f(-2, -8);
  } glEnd();
}

static void draw_uhp_ships_scene(
    const az_cutscene_state_t *cutscene, az_clock_t clock) {
  assert(cutscene->scene == AZ_SCENE_UHP_SHIPS);
  az_draw_moving_starfield(cutscene->time, -1.0,
                           (cutscene->step < 6 ? 0.6 : 0.2));
  glPushMatrix(); {
    glScalef(1, -1, 1);
    glTranslatef(AZ_SCREEN_WIDTH/2, -AZ_SCREEN_HEIGHT/2, 0);
    const float zoom_factor =
      (cutscene->step < 3 ? 1 : cutscene->step > 3 ? 0.5 :
       1 - 0.5 * cutscene->step_progress * cutscene->step_progress *
       (3 - 2 * cutscene->step_progress));
    glTranslatef(200 * (zoom_factor - 1), 0, 0);
    glScalef(zoom_factor, zoom_factor, 1);
    draw_bg_particles(cutscene, clock);
    // Fighters:
    AZ_ARRAY_LOOP(object, cutscene->objects) {
      if (object->kind == 0) continue;
      glPushMatrix(); {
        az_gl_translated(object->position);
        draw_uhp_fighter(clock);
      } glPopMatrix();
    }
    // Crusier:
    if (cutscene->step > 0) {
      glPushMatrix(); {
        glTranslatef((cutscene->step > 1 ? 0 :
                      -650 * pow(1 - cutscene->step_progress, 1.5)),
                     8 * sin(cutscene->time), 0);
        if (cutscene->step < 4) {
          draw_uhp_cruiser(cutscene->param1, clock);
        } else {
          const double param = 1 - pow(0.1, cutscene->param2);
          const float horz_drift = 300 * sqrt(param);
          const float vert_drift = 200 * sin(AZ_HALF_PI * param);
          glPushMatrix(); {
            glTranslatef(horz_drift, vert_drift, 0);
            az_gl_rotated(-AZ_DEG2RAD(70) * param);
            draw_uhp_cruiser_front_half(false, 0);
          } glPopMatrix();
          glPushMatrix(); {
            glTranslatef(-horz_drift, vert_drift, 0);
            az_gl_rotated(AZ_DEG2RAD(110) * param);
            draw_uhp_cruiser_rear_half(false, clock);
          } glPopMatrix();
        }
      } glPopMatrix();
    }
    draw_fg_particles(cutscene, clock);
  } glPopMatrix();
}

/*===========================================================================*/

static void draw_distant_ship(az_vector_t center, double size) {
  glPushMatrix(); {
    az_gl_translated(center);
    glScaled(size, size, 1);
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0.5, 0.5, 0.5); glVertex2f(0, 0); glColor3f(0.15, 0.15, 0.15);
      glVertex2f(4, 0); glVertex2f(0, 2); glVertex2f(-2, 0);
      glVertex2f(0, -2); glVertex2f(4, 0);
    } glEnd();
  } glPopMatrix();
}

static void draw_blaster_shot(az_vector_t start, az_vector_t end,
                              bool federation, int phase) {
  if (phase >= 3) return;
  glPushMatrix(); {
    az_gl_translated(start);
    az_gl_rotated(az_vtheta(az_vsub(end, start)));
    glScaled(az_vdist(start, end) / 100.0, 1, 1);
    glBegin(GL_TRIANGLE_STRIP); {
      if (federation) glColor4f(1.0f, 1.0f, 0.25f, 0.25f);
      else glColor4f(1.0f, 0.25f, 0.25f, 0.25f);
      if (phase < 2) {
        glVertex2f(0, -0.5); glVertex2f(0, 0.5);
        glVertex2f(20, -2); glVertex2f(20, 2);
      }
      glVertex2f(60, -1); glVertex2f(60, 1);
      if (phase > 0) {
        glVertex2f(100, 0); glVertex2f(100, 0);
      }
    } glEnd();
  } glPopMatrix();
}

static void draw_gunship_fragments(unsigned int bits) {
  // Struts:
  if (bits & 0x01) {
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0.25, 0.25, 0.25);
      glVertex2f(-7, -3); glVertex2f( 1, -3);
      glVertex2f(-7, -8); glVertex2f( 1, -8);
    } glEnd();
  }
  if (bits & 0x02) {
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0.25, 0.25, 0.25);
      glVertex2f(-7,  8); glVertex2f( 1,  8);
      glVertex2f(-7,  3); glVertex2f( 1,  3);
    } glEnd();
  }
  // Engines:
  if (bits & 0x04) {
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0.25, 0.25, 0.25);
      glVertex2f(-10, -12); glVertex2f(  6, -12);
      glColor3f(0.75, 0.75, 0.75);
      glVertex2f(-11,  -7); glVertex2f(  8,  -7);
    } glEnd();
  }
  if (bits & 0x08) {
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0.25, 0.25, 0.25);
      glVertex2f(-10,  12); glVertex2f(  6,  12);
      glColor3f(0.75, 0.75, 0.75);
      glVertex2f(-11,   7); glVertex2f(  8,   7);
    } glEnd();
  }
  // Main body:
  if (bits & 0x10) {
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0.25, 0.25, 0.25);
      glVertex2f( 15,  4); glVertex2f( 2,  4);
      glColor3f(0.75, 0.75, 0.75);
      glVertex2f( 20,  0); glVertex2f(-6,  0);
      glColor3f(0.25, 0.25, 0.25);
      glVertex2f( 15, -4); glVertex2f( 6, -4);
    } glEnd();
    // Windshield:
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0, 0.25, 0.25); glVertex2f(15,  2);
      glColor3f(0, 0.50, 0.50); glVertex2f(18,  0); glVertex2f(12, 0);
      glColor3f(0, 0.25, 0.25); glVertex2f(15, -2);
    } glEnd();
  }
  if (bits & 0x20) {
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0.25, 0.25, 0.25);
      glVertex2f(-10,  4); glVertex2f(-14,  4);
      glColor3f(0.75, 0.75, 0.75);
      glVertex2f( -6,  0); glVertex2f(-14,  0);
      glColor3f(0.25, 0.25, 0.25);
      glVertex2f(-11, -4); glVertex2f(-14, -4);
    } glEnd();
  }
  if (bits & 0x40) {
    glBegin(GL_TRIANGLES); {
      glColor3f(0.25, 0.25, 0.25);
      glVertex2f(-10,  4); glVertex2f(2,  4);
      glColor3f(0.75, 0.75, 0.75);
      glVertex2f( -6,  0);
    } glEnd();
  }
  if (bits & 0x80) {
    glBegin(GL_TRIANGLES); {
      glColor3f(0.25, 0.25, 0.25);
      glVertex2f(-11, -4); glVertex2f(6, -4);
      glColor3f(0.75, 0.75, 0.75);
      glVertex2f( -6,  0);
    } glEnd();
  }
}

static void spin_gunship_fragments(double cx, double cy, double spin,
                                   unsigned int bits, double gx, double gy) {
  glPushMatrix(); {
    glTranslated(cx, cy, 0);
    glScalef(1, -1, 1);
    az_gl_rotated(spin);
    glTranslated(-gx, -gy, 0);
    draw_gunship_fragments(bits);
  } glPopMatrix();
}

static void draw_civil_war_scene(
    const az_cutscene_state_t *cutscene, az_clock_t clock) {
  assert(cutscene->scene == AZ_SCENE_CIVIL_WAR);
  const int stars_parallax = 75;
  const int bg_ships_parallax = 100;
  const int debris_parallax = 500;
  const double pan = 0.5 * (1.0 + tanh(0.3 * cutscene->step_timer - 3.0));

  // Stars:
  glPushMatrix(); {
    glTranslated(-stars_parallax * pan, 0, 0);
    draw_planet_starfield_internal(AZ_SCREEN_WIDTH + stars_parallax, clock);
  } glPopMatrix();

  // Ships fighting in background:
  glPushMatrix(); {
    glTranslated(bg_ships_parallax * (1 - pan), 0, 0);
    const double drift = 1 - exp(-0.05 * cutscene->step_timer);
    const az_vector_t fed1 = {100 + 20 * drift, 300};
    const az_vector_t fed2 = {170 + 20 * drift, 350};
    const az_vector_t fed3 = {230 + 30 * drift, 230};
    const az_vector_t fed4 = {350 + 40 * drift, 250};
    const az_vector_t tri1 = {300 - 50 * drift, 150};
    const az_vector_t tri2 = {600 - 80 * drift, 180};
    const az_vector_t tri3 = {500 - 60 * drift, 320};
    const az_vector_t tri4 = {655 - 60 * drift, 210};
    draw_distant_ship(fed1, 1.0);
    draw_distant_ship(fed2, 1.5);
    draw_distant_ship(fed3, 1.0);
    draw_distant_ship(fed4, 1.5);
    draw_distant_ship(tri1, 2.0);
    draw_distant_ship(tri2, 1.8);
    draw_distant_ship(tri3, 2.0);
    draw_distant_ship(tri4, 2.0);
    if (cutscene->step_timer >= 2.0) {
      draw_blaster_shot(fed1, tri1, true, az_clock_mod(61, 2, clock));
      draw_blaster_shot(fed2, tri1, true, az_clock_mod(61, 2, clock + 20));
      draw_blaster_shot(fed3, tri1, true, az_clock_mod(81, 2, clock));
      draw_blaster_shot(fed4, tri1, true, az_clock_mod(63, 2, clock + 40));
      draw_blaster_shot(tri1, fed1, false, az_clock_mod(73, 2, clock + 60));
      draw_blaster_shot(tri1, fed3, false, az_clock_mod(47, 2, clock));
      draw_blaster_shot(tri2, fed4, false, az_clock_mod(67, 2, clock + 60));
      draw_blaster_shot(tri3, fed4, false, az_clock_mod(67, 2, clock));
      draw_blaster_shot(tri4, fed4, false, az_clock_mod(67, 2, clock + 10));
    }
  } glPopMatrix();

  // Debris field:
  const double drift = 1 - exp(-0.05 * cutscene->step_timer);
  const double spin = cutscene->step_timer;
  glPushMatrix(); {
    glTranslated(-1.0 * debris_parallax * pan, 0, 0);
    glPushMatrix(); {
      glTranslatef(700, 300, 0);
      glScalef(1, -1, 1);
      az_gl_rotated(AZ_DEG2RAD(-50));
      draw_uhp_cruiser_front_half(false, 0);
    } glPopMatrix();
    glPushMatrix(); {
      glTranslatef(820, 200, 0);
      glScalef(1, -1, 1);
      az_gl_rotated(AZ_DEG2RAD(-160));
      draw_uhp_cruiser_rear_half(false, clock);
    } glPopMatrix();

    spin_gunship_fragments(100 + 100 * drift, 100 + 50 * drift,
                           AZ_DEG2RAD(-5) * spin, 0xe7, -3, -2);

    spin_gunship_fragments(150 - 120 * drift, 400 - 50 * drift,
                           AZ_DEG2RAD(-15) * spin, 0x85, -5, -5);

    spin_gunship_fragments(380 + 5 * drift, 120 + 10 * drift,
                           AZ_DEG2RAD(70) + AZ_DEG2RAD(5) * spin, 0x7a, 0, 2);
    spin_gunship_fragments(400 + 100 * drift, 120 - 5 * drift,
                           AZ_DEG2RAD(-15) * spin, 0x05, -3, -6);

    spin_gunship_fragments(320 + 5 * drift, 330 + 10 * drift,
                           AZ_DEG2RAD(-110) - AZ_DEG2RAD(8) * spin,
                           0xd7, 2, -2);
    spin_gunship_fragments(250 - 100 * drift, 330 - 150 * drift,
                           AZ_DEG2RAD(80) * spin, 0xa0, -12, -1);
  } glPopMatrix();
  glPushMatrix(); {
    glTranslated(-1.1 * debris_parallax * pan, 0, 0);

    spin_gunship_fragments(650 - 50 * drift, 100 - 50 * drift,
                           AZ_DEG2RAD(-30), 0x80, 0, 0);
    spin_gunship_fragments(700 + 50 * drift, 80 - 30 * drift,
                           AZ_DEG2RAD(-60), 0x40, 0, 0);

    spin_gunship_fragments(580 - 80 * drift, 320 - 80 * drift,
                           AZ_DEG2RAD(-10) * spin, 0x0a, -5, 5);
    spin_gunship_fragments(610 + 70 * drift, 350 + 20 * drift,
                           AZ_DEG2RAD(-15) * spin, 0xa5, -5, -5);
    spin_gunship_fragments(570 - 50 * drift, 380 + 100 * drift,
                           AZ_DEG2RAD(5) * spin, 0x50, 12, 0);

    spin_gunship_fragments(980 - 120 * drift, 350 - 50 * drift,
                           AZ_DEG2RAD(15) * spin, 0x4a, -5, 5);
    spin_gunship_fragments(980 - 100 * drift, 350 + 100 * drift,
                           AZ_DEG2RAD(-20) * spin, 0xa5, -5, -5);
    spin_gunship_fragments(980 + 100 * drift, 350 + 20 * drift,
                           AZ_DEG2RAD(10) * spin, 0x10, 12, 0);
  } glPopMatrix();
}

/*===========================================================================*/

void az_draw_cutscene(const az_space_state_t *state) {
  const az_cutscene_state_t *cutscene = &state->cutscene;
  assert(cutscene->scene != AZ_SCENE_NOTHING);
  switch (cutscene->scene) {
    case AZ_SCENE_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_SCENE_TEXT:
      assert(cutscene->scene_text != NULL);
      az_draw_paragraph(16, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2,
                        AZ_SCREEN_HEIGHT/2 - 8, 20, -1,
                        state->prefs, cutscene->scene_text);
      break;
    case AZ_SCENE_CRUISING:
      draw_cruising_scene(cutscene, state->clock);
      break;
    case AZ_SCENE_MOVE_OUT:
      draw_move_out_scene(cutscene, state->clock);
      break;
    case AZ_SCENE_ARRIVAL:
      draw_arrival_scene(cutscene, state->clock);
      break;
    case AZ_SCENE_ZENITH:
      draw_zenith_scene(cutscene, state->clock);
      break;
    case AZ_SCENE_ESCAPE:
      draw_escape_scene(cutscene, state->clock);
      break;
    case AZ_SCENE_OTH:
      draw_oth_scene(cutscene, state->clock);
      break;
    case AZ_SCENE_BLACK: break;
    case AZ_SCENE_SAPIAIS:
      draw_sapiais_scene(cutscene, state->clock);
      break;
    case AZ_SCENE_UHP_SHIPS:
      draw_uhp_ships_scene(cutscene, state->clock);
      break;
    case AZ_SCENE_CIVIL_WAR:
      draw_civil_war_scene(cutscene, state->clock);
      break;
  }
  if (cutscene->fade_alpha > 0.0) tint_screen(0, cutscene->fade_alpha);
}

/*===========================================================================*/
