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

#include "azimuth/state/wall.h"

#include <assert.h>
#include <math.h> // for fmax
#include <stdbool.h>

#include "azimuth/util/misc.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static az_vector_t wall_vertices_0[] = {
  {50, 25}, {-50, 25}, {-50, -25}, {50, -25}
};
static az_vector_t wall_vertices_1[] = {
  {50, 50}, {-50, 50}, {-50, -50}, {50, -50}
};
static az_vector_t wall_vertices_2[] = {
  {10, -25}, {60, -25}, {60, 25}, {-10.710678, 25},
  {-60.710678, -25}, {-25.355339, -60.355339}
};
static az_vector_t wall_vertices_3[] = {
  {-20, -8}, {15, -10}, {25, 0}, {15, 10}, {-20, 8}
};
static az_vector_t wall_vertices_4[] = {
  {-30, -10}, {15, -15}, {35, 0}, {15, 15}, {-30, 10}
};
static az_vector_t wall_vertices_5[] = {
  {31, 10}, {33, 30}, {20, 43}, {-2, 44}, {-7, 42}, {-30, 30}, {-48, 30},
  {-59, 0}, {-41, -34}, {-28, -35}, {-20, -29}, {-5, -42}, {4, -42}, {19, -27},
  {35, -28}, {40, 0}
};
static az_vector_t wall_vertices_girder_long[] = {
  {25, 0}, {50, 25}, {-50, 25}, {-25, 0}, {-50, -25}, {50, -25}
};
static az_vector_t wall_vertices_girder_short[] = {
  {2, 0}, {27, 25}, {-27, 25}, {-2, 0}, {-27, -25}, {27, -25}
};
static az_vector_t wall_vertices_girder_skinny[] = {
  {39, 0}, {59, 20}, {-59, 20}, {-39, 0}, {-59, -20}, {59, -20}
};
static az_vector_t wall_vertices_girder_skinny_capped[] = {
  {39, 0}, {59, 20}, {-59, 20}, {-59, -20}, {59, -20}
};
static az_vector_t wall_vertices_11[] = {
  {17, 4}, {23, 10}, {20, 21}, {8, 28}, {-2, 22}, {-20, 18}, {-20, 0},
  {-27, -10}, {-19, -21}, {-3, -18}, {7, -26}, {16, -20}
};

static az_wall_data_t wall_datas[] = {
  // Yellow rectangle block:
  [0] = {
    .style = AZ_WSTY_BEZEL_12, .bezel = 18.0,
    .color1 = {255, 255, 0, 255}, .color2 = {64, 64, 0, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_0)
  },
  // Large blue square block:
  [1] = {
    .style = AZ_WSTY_BEZEL_12, .bezel = 18.0,
    .color1 = {0, 255, 255, 255}, .color2 = {0, 32, 64, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_1)
  },
  // Yellow angle block:
  [2] = {
    .style = AZ_WSTY_BEZEL_12, .bezel = 18.0,
    .color1 = {255, 255, 0, 255}, .color2 = {64, 64, 0, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_2)
  },
  // Small cyan crystal:
  [3] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 1000.0,
    .color2 = {0, 32, 32, 255}, .color1 = {0, 255, 255, 255},
    .elasticity = 0.3, .impact_damage_coeff = 10.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_3)
  },
  // Big cyan crystal:
  [4] = {
    .style = AZ_WSTY_BEZEL_21, .bezel = 1000.0,
    .color2 = {0, 32, 32, 255}, .color1 = {0, 255, 255, 255},
    .elasticity = 0.3, .impact_damage_coeff = 10.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_4)
  },
  // Gray/brown boulder:
  [5] = {
    .style = AZ_WSTY_ALT_BEZEL_21, .bezel = 25.0,
    .color1 = {96, 80, 64, 255}, .color2 = {48, 32, 16, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_5)
  },
  // Silver long girder:
  [6] = {
    .style = AZ_WSTY_GIRDER, .bezel = 7.0,
    .color1 = {192, 192, 192, 255}, .color2 = {64, 64, 64, 255},
    .elasticity = 0.5, .impact_damage_coeff = 1.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_girder_long)
  },
  // Red long girder:
  [7] = {
    .style = AZ_WSTY_GIRDER, .bezel = 6.0,
    .color1 = {128, 32, 32, 255}, .color2 = {64, 0, 0, 255},
    .elasticity = 0.5, .impact_damage_coeff = 1.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_girder_long)
  },
  // Red short girder:
  [8] = {
    .style = AZ_WSTY_GIRDER, .bezel = 6.0,
    .color1 = {128, 32, 32, 255}, .color2 = {64, 0, 0, 255},
    .elasticity = 0.5, .impact_damage_coeff = 1.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_girder_short)
  },
  // Brown long girder:
  [9] = {
    .style = AZ_WSTY_GIRDER, .bezel = 4.7,
    .color1 = {128, 96, 32, 255}, .color2 = {64, 64, 64, 255},
    .elasticity = 0.5, .impact_damage_coeff = 1.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_girder_skinny)
  },
  // Brown long capped girder:
  [10] = {
    .style = AZ_WSTY_GIRDER_CAP, .bezel = 4.7,
    .color1 = {128, 96, 32, 255}, .color2 = {64, 64, 64, 255},
    .elasticity = 0.5, .impact_damage_coeff = 1.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_girder_skinny_capped)
  },
  // Small gray/brown boulder:
  [11] = {
    .style = AZ_WSTY_TRIFAN, .bezel = 900.0,
    .color1 = {96, 80, 64, 255}, .color2 = {48, 32, 16, 255},
    .elasticity = 0.25, .impact_damage_coeff = 4.0,
    .polygon = AZ_INIT_POLYGON(wall_vertices_11)
  }
};

/*===========================================================================*/

const int AZ_NUM_WALL_DATAS = AZ_ARRAY_SIZE(wall_datas);

static bool wall_data_initialized = false;

void az_init_wall_datas(void) {
  assert(!wall_data_initialized);
  AZ_ARRAY_LOOP(data, wall_datas) {
    assert(data->polygon.num_vertices >= 3);
    double radius = 0.0;
    for (int i = 0; i < data->polygon.num_vertices; ++i) {
      radius = fmax(radius, az_vnorm(data->polygon.vertices[i]));
    }
    data->bounding_radius = radius + 0.01; // small safety margin
  }
  wall_data_initialized = true;
}

const az_wall_data_t *az_get_wall_data(int index) {
  assert(wall_data_initialized);
  assert(index >= 0);
  assert(index < AZ_NUM_WALL_DATAS);
  return &wall_datas[index];
}

int az_wall_data_index(const az_wall_data_t *data) {
  assert(data >= wall_datas);
  assert(data < wall_datas + AZ_NUM_WALL_DATAS);
  return data - wall_datas;
}

/*===========================================================================*/

bool az_point_touches_wall(const az_wall_t *wall, az_vector_t point) {
  assert(wall->kind != AZ_WALL_NOTHING);
  return (az_vwithin(point, wall->position, wall->data->bounding_radius) &&
          az_polygon_contains(wall->data->polygon,
                              az_vrotate(az_vsub(point, wall->position),
                                         -wall->angle)));
}

bool az_circle_touches_wall(
    const az_wall_t *wall, double radius, az_vector_t center) {
  assert(wall->kind != AZ_WALL_NOTHING);
  return (az_vwithin(center, wall->position,
                     radius + wall->data->bounding_radius) &&
          az_circle_touches_polygon_trans(wall->data->polygon, wall->position,
                                          wall->angle, radius, center));
}

bool az_ray_hits_wall(const az_wall_t *wall, az_vector_t start,
                      az_vector_t delta, az_vector_t *point_out,
                      az_vector_t *normal_out) {
  assert(wall->kind != AZ_WALL_NOTHING);
  return (az_ray_hits_bounding_circle(start, delta, wall->position,
                                      wall->data->bounding_radius) &&
          az_ray_hits_polygon_trans(wall->data->polygon, wall->position,
                                    wall->angle, start, delta,
                                    point_out, normal_out));
}

bool az_circle_hits_wall(
    const az_wall_t *wall, double radius, az_vector_t start, az_vector_t delta,
    az_vector_t *pos_out, az_vector_t *impact_out) {
  assert(wall->kind != AZ_WALL_NOTHING);
  return (az_ray_hits_bounding_circle(start, delta, wall->position,
                                      wall->data->bounding_radius + radius) &&
          az_circle_hits_polygon_trans(wall->data->polygon, wall->position,
                                       wall->angle, radius, start, delta,
                                       pos_out, impact_out));
}

bool az_arc_circle_hits_wall(
    const az_wall_t *wall, double circle_radius,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out) {
  assert(wall->kind != AZ_WALL_NOTHING);
  return (az_arc_ray_might_hit_bounding_circle(
              start, spin_center, spin_angle, wall->position,
              wall->data->bounding_radius + circle_radius) &&
          az_arc_circle_hits_polygon_trans(
              wall->data->polygon, wall->position, wall->angle,
              circle_radius, start, spin_center, spin_angle,
              angle_out, pos_out, impact_out));
}

/*===========================================================================*/
