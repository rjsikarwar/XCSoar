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


DialogPreferences::DialogPreferences():
  form_background(Color(0xe2,0xdc,0xbe)),
  focus_background(Color(0x37,0xb7,0xef)),
  select_background(COLOR_YELLOW),
  widget_background(Color(0xd4,0xd0,0xc8)),
  widget_text(COLOR_BLACK),
  widget_disabled(COLOR_GRAY),
  control_background(COLOR_WHITE),
  choice_background(COLOR_LIGHT_GRAY),
  shade_background(dark_color(form_background)),
  infobox_light_shade(Color(0x97,0xb5,0xc3)), // Color(0xe2,0xdc,0xbe)
  infobox_neutral_shade(Color(0xb0,0xa0,0x4e)),
  infobox_dark_shade(Color(0x41,0x3b,0x1d)),
  infobox_light_text(Color(0x16,0x36,0x8b)),
  infobox_dark_text(Color(0x80,0xff,0xff)),
  titlebar_normal(Color(0, 77, 124)),
  titlebar_alert(Color(0xb9,0x3c,0x00))
{
}

DialogPreferences dialog_prefs;
