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

#include "azimuth/state/gravfield.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include "azimuth/util/misc.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

bool az_is_trapezoidal(az_gravfield_kind_t kind) {
  assert(kind != AZ_GRAV_NOTHING);
  switch (kind) {
    case AZ_GRAV_NOTHING:
      AZ_ASSERT_UNREACHABLE();
    case AZ_GRAV_TRAPEZOID:
    case AZ_GRAV_WATER:
    case AZ_GRAV_LAVA:
      return true;
    case AZ_GRAV_SECTOR_PULL:
    case AZ_GRAV_SECTOR_SPIN:
      return false;
  }
  AZ_ASSERT_UNREACHABLE();
}

bool az_is_liquid(az_gravfield_kind_t kind) {
  return (kind == AZ_GRAV_WATER || kind == AZ_GRAV_LAVA);
}

double az_sector_interior_angle(const az_gravfield_size_t *size) {
  const double radians =
    az_mod2pi_nonneg(AZ_DEG2RAD(size->sector.sweep_degrees));
  return (radians == 0.0 ? AZ_TWO_PI : radians);
}

bool az_point_within_gravfield(const az_gravfield_t *gravfield,
                               az_vector_t point) {
  assert(gravfield->kind != AZ_GRAV_NOTHING);
  const az_gravfield_size_t *size = &gravfield->size;
  if (az_is_trapezoidal(gravfield->kind)) {
    const double semilength = size->trapezoid.semilength;
    const double front_offset = size->trapezoid.front_offset;
    const double front_semiwidth = size->trapezoid.front_semiwidth;
    const double rear_semiwidth = size->trapezoid.rear_semiwidth;
    const az_vector_t relpoint =
      az_vrotate(az_vsub(point, gravfield->position), -gravfield->angle);
    if (az_is_liquid(gravfield->kind)) {
      const double position_norm = az_vnorm(gravfield->position);
      const az_vector_t abspoint = { relpoint.x + position_norm, relpoint.y };
      const double outer_radius =
        hypot(fmax(front_offset + front_semiwidth,
                   front_offset - front_semiwidth),
              position_norm + semilength);
      const double inner_radius =
        hypot(rear_semiwidth, position_norm - semilength);
      const double absnorm = az_vnorm(abspoint);
      if (absnorm < inner_radius || absnorm > outer_radius) return false;
      const az_vector_t inner_start =
        az_vwithlen((az_vector_t){position_norm - semilength, -rear_semiwidth},
                    inner_radius);
      const az_vector_t outer_start =
        az_vwithlen((az_vector_t){position_norm + semilength,
                                  front_offset - front_semiwidth},
                    outer_radius);
      az_vector_t mid_start;
      if (!az_ray_hits_circle(absnorm, AZ_VZERO, outer_start,
                              az_vsub(inner_start, outer_start),
                              &mid_start, NULL)) return false;
      const az_vector_t inner_end =
        az_vwithlen((az_vector_t){position_norm - semilength, rear_semiwidth},
                    inner_radius);
      const az_vector_t outer_end =
        az_vwithlen((az_vector_t){position_norm + semilength,
                                  front_offset + front_semiwidth},
                    outer_radius);
      az_vector_t mid_end;
      if (!az_ray_hits_circle(absnorm, AZ_VZERO, outer_end,
                              az_vsub(inner_end, outer_end),
                              &mid_end, NULL)) return false;
      const double start_theta = az_vtheta(mid_start);
      return (az_mod2pi_nonneg(az_vtheta(abspoint) - start_theta) <=
              az_mod2pi_nonneg(az_vtheta(mid_end) - start_theta));
    } else {
      const az_vector_t vertices[4] = {
        {semilength, front_offset - front_semiwidth},
        {semilength, front_offset + front_semiwidth},
        {-semilength, rear_semiwidth},
        {-semilength, -rear_semiwidth}
      };
      const az_polygon_t trapezoid = AZ_INIT_POLYGON(vertices);
      return az_polygon_contains(trapezoid, relpoint);
    }
  } else {
    assert(!az_is_liquid(gravfield->kind));
    const double thickness = size->sector.thickness;
    assert(thickness > 0.0);
    const double inner_radius = size->sector.inner_radius;
    assert(inner_radius >= 0.0);
    const double outer_radius = inner_radius + thickness;
    return (az_vwithin(point, gravfield->position, outer_radius) &&
            !az_vwithin(point, gravfield->position, inner_radius) &&
            az_mod2pi_nonneg(az_vtheta(az_vsub(point, gravfield->position)) -
                             gravfield->angle) <=
            az_sector_interior_angle(size));
  }
}

static void get_liquid_surface_arc(
    const az_gravfield_t *gravfield, double *arc_radius_out,
    az_vector_t *arc_center_out, double *min_theta_out,
    double *theta_span_out) {
  assert(az_is_liquid(gravfield->kind));
  assert(az_is_trapezoidal(gravfield->kind));
  const double semilength = gravfield->size.trapezoid.semilength;
  const double front_offset = gravfield->size.trapezoid.front_offset;
  const double front_semiwidth = gravfield->size.trapezoid.front_semiwidth;
  const double position_norm = az_vnorm(gravfield->position);
  *arc_radius_out =
    hypot(fmax(front_offset + front_semiwidth, front_offset - front_semiwidth),
          position_norm + semilength);
  *arc_center_out =
    az_vsub(gravfield->position, az_vpolar(position_norm, gravfield->angle));
  const double theta1 = atan2(front_offset - front_semiwidth,
                              position_norm + semilength);
  const double theta2 = atan2(front_offset + front_semiwidth,
                              position_norm + semilength);
  *min_theta_out = az_mod2pi(theta1 + gravfield->angle);
  *theta_span_out = az_mod2pi_nonneg(theta2 - theta1);
}

bool az_ray_hits_liquid_surface(
    const az_gravfield_t *gravfield, az_vector_t start, az_vector_t delta,
    az_vector_t *point_out, az_vector_t *normal_out) {
  double arc_radius, min_theta, theta_span;
  az_vector_t arc_center;
  get_liquid_surface_arc(gravfield, &arc_radius, &arc_center, &min_theta,
                         &theta_span);
  return az_ray_hits_arc(arc_radius, arc_center, min_theta, theta_span,
                         start, delta, point_out, normal_out);
}

bool az_circle_hits_liquid_surface(
    const az_gravfield_t *gravfield, double radius, az_vector_t start,
    az_vector_t delta, az_vector_t *point_out, az_vector_t *normal_out) {
  double arc_radius, min_theta, theta_span;
  az_vector_t arc_center;
  get_liquid_surface_arc(gravfield, &arc_radius, &arc_center, &min_theta,
                         &theta_span);
  return az_circle_hits_arc(arc_radius, arc_center, min_theta, theta_span,
                            radius, start, delta, point_out, normal_out);
}

/*===========================================================================*/
