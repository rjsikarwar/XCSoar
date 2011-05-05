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

#include "Screen/Color.hpp"

const Color
  Color::WHITE(0xff, 0xff, 0xff),
  Color::BLACK(0, 0, 0),
  Color::GRAY(0x80, 0x80, 0x80),
  Color::LIGHT_GRAY(0xc0, 0xc0, 0xc0),
  Color::DARK_GRAY(0x40, 0x40, 0x40),
  Color::RED(0xff, 0, 0),
  Color::GREEN(0, 0xff, 0),
  Color::BLUE(0, 0, 0xff),
  Color::YELLOW(0xff, 0xff, 0),
  Color::CYAN(0, 0xff, 0xff),
  Color::MAGENTA(0xff, 0, 0xff),
  Color::ORANGE(0xff, 0xa2, 0),
  Color::BROWN(0xb7,0x64,0x1e); // ICAO

static unsigned char light_color(unsigned char c) {
  return ((c ^ 0xff) >> 1) ^ 0xff;
}

Color light_color(Color c) {
  return Color(light_color(c.red()), light_color(c.green()),
               light_color(c.blue()));
}

static unsigned char dark_color(unsigned char c) {
  return (c >> 1);
}

Color dark_color(Color c) {
  return Color(dark_color(c.red()), dark_color(c.green()),
               dark_color(c.blue()));
}

Color desaturate(Color c) {
  int a = (c.red()+c.green()+c.blue())/3;
  return Color((c.red()+a)/2,
               (c.green()+a)/2,
               (c.blue()+a)/2);
}
