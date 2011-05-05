/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Terrain/TerrainRenderer.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Screen/Ramp.hpp"
#include "WindowProjection.hpp"

#include <assert.h>

const short terrain_zero[10] = {
  600, // flat
  700, // mount
  700, // imhof 7
  450, // imhof 4
  650, // imhof 12
  600, // imhof atlas
  750, // icao
  500, // gray
  520, // icao2
  500  // icao3
};

const ColorRamp terrain_colors[10][NUM_COLOR_RAMP_LEVELS] = {
  { // flatlands
    {0,           0x70, 0xc0, 0xa7},
    {250,         0xca, 0xe7, 0xb9},
    {500,         0xf4, 0xea, 0xaf},
    {750,         0xdc, 0xb2, 0x82},
    {1000,        0xca, 0x8e, 0x72},
    {1250,        0xde, 0xc8, 0xbd},
    {1500,        0xe3, 0xe4, 0xe9},
    {1750,        0xdb, 0xd9, 0xef},
    {2000,        0xce, 0xcd, 0xf5},
    {2250,        0xc2, 0xc1, 0xfa},
    {2500,        0xb7, 0xb9, 0xff},
    {3001,        0xb7, 0xb9, 0xff},
    {3002,        0xb7, 0xb9, 0xff}
  },
  { // mountainous
    {0,           0x70, 0xc0, 0xa7},
    {500,         0xca, 0xe7, 0xb9},
    {1000,        0xf4, 0xea, 0xaf},
    {1500,        0xdc, 0xb2, 0x82},
    {2000,        0xca, 0x8e, 0x72},
    {2500,        0xde, 0xc8, 0xbd},
    {3000,        0xe3, 0xe4, 0xe9},
    {3500,        0xdb, 0xd9, 0xef},
    {4000,        0xce, 0xcd, 0xf5},
    {4500,        0xc2, 0xc1, 0xfa},
    {5000,        0xb7, 0xb9, 0xff},
    {5501,        0xb7, 0xb9, 0xff},
    {5502,        0xb7, 0xb9, 0xff}
  },
  { // Imhof Type 7, geomteric 1.35 9
    {0,    153, 178, 169},
    {268,  180, 205, 181},
    {496,  225, 233, 192},
    {670,  255, 249, 196},
    {905, 255, 219, 173},
    {1222, 254, 170, 136},
    {1650, 253, 107, 100},
    {2227, 255, 255, 255},
    {3008, 255, 255, 255},
    {3009, 255, 255, 255},
    {3010, 255, 255, 255},
    {3011, 255, 255, 255},
    {3012, 255, 255, 255}
  },
  { // Imhof Type 4, geomteric 1.5 8
    {0,    175, 224, 203},
    {264,  211, 237, 211},
    {396,  254, 254, 234},
    {594,  252, 243, 210},
    {891,  237, 221, 195},
    {1336, 221, 199, 175},
    {2004, 215, 170, 148},
    {3007, 255, 255, 255},
    {4000, 255, 255, 255},
    {4001, 255, 255, 255},
    {4002, 255, 255, 255},
    {4003, 255, 255, 255},
    {4004, 255, 255, 255}
  },
  { // Imhof Type 12, geomteric 1.5 8
    {0,    165, 220, 201},
    {359,  219, 239, 212},
    {558,  254, 253, 230},
    {782,  254, 247, 211},
    {1094, 254, 237, 202},
    {1532, 254, 226, 207},
    {2145, 254, 209, 204},
    {3004, 255, 255, 255},
    {4000, 255, 255, 255},
    {4001, 255, 255, 255},
    {4002, 255, 255, 255},
    {4003, 255, 255, 255},
    {4004, 255, 255, 255}
  },
  { // Imhof Atlas der Schweiz
    {0,     47, 101, 147},
    {368,   58, 129, 152},
    {496,  117, 148, 153},
    {670,  155, 178, 140},
    {905,  192, 190, 139},
    {1222, 215, 199, 137},
    {1650, 229, 203, 171},
    {2227, 246, 206, 171},
    {3007, 252, 246, 244},
    {4001, 252, 246, 244},
    {4002, 252, 246, 244},
    {4003, 252, 246, 244},
    {4004, 252, 246, 244}
  },
  { // ICAO
    {0,           180, 205, 181},
    {200,         225, 233, 192},
    {500,         255, 249, 196},
    {1000,        255, 219, 173},
    {1500,        254, 170, 136},
    {2000,        253, 107, 100},
    {2500,        255, 255, 255},
    {3000,        255, 255, 255},
    {3001,        255, 255, 255},
    {3002,        255, 255, 255},
    {3003,        255, 255, 255},
    {3004,        255, 255, 255},
    {3005,        255, 255, 255},
  },
  { // Grey
    {0,           220, 220, 220},
    {100,         220, 220, 220},
    {200,         220, 220, 220},
    {300,         220, 220, 220},
    {500,         220, 220, 220},
    {700,         220, 220, 220},
    {1000,        220, 220, 220},
    {1250,        220, 220, 220},
    {1500,        220, 220, 220},
    {1750,        220, 220, 220},
    {2000,        220, 220, 220},
    {2250,        220, 220, 220},
    {2500,        220, 220, 220}
  },
  { // ICAO2
    {0,           0xaa, 0xce, 0x66},
    {200,         0xbb, 0xd2, 0x21},
    {400,         0xf8, 0xfd, 0xbd},
    {750,         0xfe, 0xfb, 0x87},
    {1000,         0xfd, 0xd6, 0x0f},
    {1500,        0xfe, 0xc5, 0x08},
    {2000,        0xfc, 0xdb, 0x7b},
    {2500,        0xfa, 0xd6, 0x98},
    {3000,        0xf9, 0xe0, 0xc2},
    {3500,        0xff, 0xf4, 0xe2},
    {4000,        0xff, 0xff, 0xff},
    {4501,        0xff, 0xff, 0xff},
    {4502,        0xff, 0xff, 0xff}
  },
  { // ICAO3
    {0,           0x5e, 0x69, 0x51},
    {200,         0x80, 0x7a, 0x18},
    {400,         0xf0, 0xf0, 0xf0},
    {600,         0x9c, 0x90, 0x15},
    {1000,         0xfa, 0xd0, 0x6e},
    {1500,        0xb9, 0x66, 0x1e},
    {2000,        0x79, 0x32, 0x19},
    {2501,        0x79, 0x32, 0x19},
    {2502,        0x79, 0x32, 0x19},
    {2503,        0x79, 0x32, 0x19},
    {2504,        0x79, 0x32, 0x19},
    {2505,        0x79, 0x32, 0x19},
    {2506,        0x79, 0x32, 0x19},
  }
};

// map scale is approximately 2 points on the grid
// therefore, want one to one mapping if mapscale is 0.5
// there are approx 30 pixels in mapscale
// 240/QUANTISATION_PIXELS resolution = 6 pixels per terrain
// (mapscale/30)  km/pixels
//        0.250   km/terrain
// (0.25*30/mapscale) pixels/terrain
//  mapscale/(0.25*30)
//  mapscale/7.5 terrain units/pixel
//
// this is for TerrainInfo.StepSize = 0.0025;
TerrainRenderer::TerrainRenderer(const RasterTerrain *_terrain) :
  last_color_ramp(NULL),
  terrain(_terrain)
{
  assert(terrain != NULL);
}

void
TerrainRenderer::ScanSpotHeights()
{
  spot_max_pt.x = -1;
  spot_max_pt.y = -1;
  spot_min_pt.x = -1;
  spot_min_pt.y = -1;
  spot_max_val = -1;
  spot_min_val = 32767;

  const HeightMatrix &height_matrix = raster_renderer.GetHeightMatrix();
  const short *h_buf = height_matrix.GetData();
  const unsigned quantisation_pixels = raster_renderer.GetQuantisation();

  for (unsigned y = 0; y < height_matrix.get_height(); ++y) {
    for (unsigned x = 0; x < height_matrix.get_width(); ++x) {
      short val = *h_buf++;
      if (RasterBuffer::is_special(val))
        continue;

      if (val > spot_max_val) {
        spot_max_val = val;
        spot_max_pt.x = x;
        spot_max_pt.y = y;
      }
      if (val < spot_min_val) {
        spot_min_val = val;
        spot_min_pt.x = x;
        spot_min_pt.y = y;
      }
    }
  }

  spot_max_pt.x *= quantisation_pixels;
  spot_max_pt.y *= quantisation_pixels;
  spot_min_pt.x *= quantisation_pixels;
  spot_min_pt.y *= quantisation_pixels;
}

void
TerrainRenderer::CopyTo(Canvas &canvas, unsigned width, unsigned height)
{
  raster_renderer.GetImage().stretch_to(raster_renderer.get_width(),
                                        raster_renderer.get_height(), canvas,
                                        width, height);
}

/**
 * Draws the terrain to the given canvas
 * @param canvas The drawing canvas
 * @param map_projection The Projection
 * @param sunazimuth Azimuth of the sun (for terrain shading)
 */
void
TerrainRenderer::Draw(Canvas &canvas,
                      const WindowProjection &map_projection,
                      const Angle sunazimuth,
                      const short h_offset)
{
  const bool do_water = true;
  const unsigned height_scale = 4;
  const bool is_terrain = true;
  const bool do_shading = is_terrain && settings.slope_shading != sstOff;
  const int interp_levels = std::min(2, (int)settings.interpolate_bits);

  const ColorRamp *const color_ramp = &terrain_colors[settings.ramp][0];
  if (color_ramp != last_color_ramp) {
    raster_renderer.ColorTable(color_ramp, do_water,
                               height_scale, interp_levels);
    last_color_ramp = color_ramp;
  }

  {
    RasterTerrain::Lease map(*terrain);
    const short zero_point = h_offset>0? terrain_zero[settings.ramp]-h_offset : 0;
    raster_renderer.ScanMap(map, map_projection, zero_point);
  }

  raster_renderer.GenerateImage(do_shading, height_scale,
                                settings.contrast, settings.brightness,
                                sunazimuth);

  CopyTo(canvas, map_projection.GetScreenWidth(),
         map_projection.GetScreenHeight());
}

bool 
TerrainRenderer::do_scan_spot()
{
  return false;
}
