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

#ifndef XCSOAR_SCREEN_COLOR_HPP
#define XCSOAR_SCREEN_COLOR_HPP

#ifdef ENABLE_SDL
#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Color.hpp"
#else /* !OPENGL */
#include "Screen/SDL/Color.hpp"
#endif /* !OPENGL */
#else /* !SDL */
#include "Screen/GDI/Color.hpp"
#endif /* !SDL */

#define COLOR_WHITE Color(0xff, 0xff, 0xff)
#define COLOR_BLACK Color(0x00, 0x00, 0x00)
#define COLOR_GRAY Color(0x80, 0x80, 0x80)
#define COLOR_LIGHT_GRAY Color(0xc0, 0xc0, 0xc0)
#define COLOR_DARK_GRAY Color(0x40, 0x40, 0x40)
#define COLOR_RED Color(0xff, 0x00, 0x00)
#define COLOR_GREEN Color(0x00, 0xff, 0x00)
#define COLOR_BLUE Color(0x00, 0x00, 0xff)
#define COLOR_YELLOW Color(0xff, 0xff, 0x00)
#define COLOR_CYAN Color(0x00, 0xff, 0xff)
#define COLOR_MAGENTA Color(0xff, 0x00, 0xff)
#define COLOR_ORANGE Color(0xff, 0xa2, 0x00)
#define COLOR_BROWN Color(0xb7,0x64,0x1e)

/**
 * Returns a lighter version of the specified color, adequate for
 * SRCAND filtering.
 */
Color light_color(Color c);

/**
 * Returns a darker version of the specified color.
 */
Color dark_color(Color c);

Color desaturate(Color c);

struct DialogPreferences {
  DialogPreferences();
  Color form_background;
  Color focus_background;
  Color select_background;
  Color widget_background;
  Color widget_text;
  Color widget_disabled;
  Color control_background;
  Color choice_background;
  Color shade_background;
  Color titlebar_normal;
  Color titlebar_alert;
};

extern DialogPreferences dialog_prefs;

#endif
