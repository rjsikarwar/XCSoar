/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "CrossSectionLook.hpp"

void
CrossSectionLook::Initialise()
{
  background_color = COLOR_WHITE;
  text_color = COLOR_BLACK;

#ifdef NOOK
  sky_color = GRAYSCALE_1;
  terrain_color = GRAYSCALE_3;
  sea_color = GRAYSCALE_5; // ICAO open water area
  grid_pen.Set(Pen::DASH, 1, GRAYSCALE_5);
#else
  sky_color = Color(0x0a, 0xb9, 0xf3);
  terrain_color = Color(0x80, 0x45, 0x15);
  sea_color = Color(0xbd, 0xc5, 0xd5); // ICAO open water area
  grid_pen.Set(Pen::DASH, 1, Color(0x60, 0x60, 0x60));
#endif

  terrain_brush.Set(terrain_color);
  sea_brush.Set(sea_color);

  aircraft_brush.Set(text_color);
}
