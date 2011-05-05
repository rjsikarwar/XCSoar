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

#include "HeightMatrix.hpp"
#include "RasterMap.hpp"
#include "WindowProjection.hpp"

#include <algorithm>
#include <assert.h>

void
HeightMatrix::SetSize(size_t _size)
{
  assert(_size > 0);

  data.grow_discard(_size);
}

void
HeightMatrix::SetSize(unsigned _width, unsigned _height)
{
  width = _width;
  height = _height;

  SetSize(width * height);
}

void
HeightMatrix::SetSize(unsigned width, unsigned height,
                      unsigned quantisation_pixels)
{
  SetSize((width + quantisation_pixels - 1) / quantisation_pixels,
          (height + quantisation_pixels - 1) / quantisation_pixels);
}

void
HeightMatrix::Fill(const RasterMap &map, const WindowProjection &projection,
                   unsigned quantisation_pixels, bool interpolate)
{
  const unsigned screen_width = projection.GetScreenWidth();
  const unsigned screen_height = projection.GetScreenHeight();

  SetSize((screen_width + quantisation_pixels - 1) / quantisation_pixels,
          (screen_height + quantisation_pixels - 1) / quantisation_pixels);

  for (unsigned y = 0; y < screen_height; y += quantisation_pixels) {
    short *p = data.begin() + y * width / quantisation_pixels;
    map.ScanLine(projection.ScreenToGeo(0, y),
                 projection.ScreenToGeo(screen_width, y),
                 p, width, interpolate);
  }
}

void
HeightMatrix::Offset(const short h_offset)
{
  const short* end = data.begin() + height * width;
  for (short *p = data.begin(); p!= end; ++p) {
    if (!RasterBuffer::is_special(*p)) {
      *p += h_offset;
      if (RasterBuffer::is_special(*p)) {
        *p = 0;
      }
    }
  }
}
