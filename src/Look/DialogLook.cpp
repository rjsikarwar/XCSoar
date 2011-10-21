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

#include "DialogLook.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"

void
DialogLook::Initialise(const Font &caption_font,
                       const Font &_text_font,
                       const Font &_button_font,
                       const Font &list_font)
{
  caption.background_color = Color(255, 255, 255);
  caption.text_color = COLOR_BLACK;
  caption.font = &caption_font;

  SetBackgroundColor(Color(0xff, 0xff, 0xff));
  text_color = COLOR_BLACK;

  text_font = &_text_font;
  button_font = &_button_font;

  focused.text_color = COLOR_BLACK;
  focused.border_pen.set(Layout::FastScale(1) + 2, COLOR_BLACK);

  list.background_color = COLOR_WHITE;
  list.text_color = COLOR_BLACK;
  list.selected.background_color = COLOR_WHITE;
  list.selected.text_color = COLOR_BLACK;
  list.font = &list_font;
}

void
DialogLook::SetBackgroundColor(Color color)
{
  background_color = color;
  background_brush.set(color);

  focused.background_color = color.highlight();
}
