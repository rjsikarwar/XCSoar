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

#include "Look/AirspaceLook.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "resource.h"
#include "Util/Macros.hpp"

#ifdef USE_GDI
#include "Screen/GDI/AlphaBlend.hpp"
#endif

const Color AirspaceLook::preset_colors[] = {
  COLOR_WHITE,
  Color(255,255,214),
  Color(255,255,205),
  Color(255,255,204),
  Color(255,255,203),
  Color(255,255,202),
  Color(255,255,201),
  Color(255,255,200),
  COLOR_BLACK,
};

void
AirspaceLook::Initialise(const AirspaceRendererSettings &settings)
{
  for (unsigned i = 0; i < AIRSPACECLASSCOUNT; i++)
    pens[i].Set(Layout::ScalePenWidth(2), settings.classes[i].color);

  // airspace brushes and colors
#ifdef HAVE_HATCHED_BRUSH
  bitmaps[0].Load(IDB_AIRSPACE0);
  bitmaps[1].Load(IDB_AIRSPACE1);
  bitmaps[2].Load(IDB_AIRSPACE2);
  bitmaps[3].Load(IDB_AIRSPACE3);
  bitmaps[4].Load(IDB_AIRSPACE4);
  bitmaps[5].Load(IDB_AIRSPACE5);
  bitmaps[6].Load(IDB_AIRSPACE6);
  bitmaps[7].Load(IDB_AIRSPACE7);

  for (unsigned i = 0; i < ARRAY_SIZE(AirspaceLook::brushes); i++)
    brushes[i].Set(bitmaps[i]);
#endif

#ifdef HAVE_ALPHA_BLEND
  if (AlphaBlendAvailable())
#endif
#if defined(HAVE_ALPHA_BLEND) || !defined(HAVE_HATCHED_BRUSH)
    for (unsigned i = 0; i < AIRSPACECLASSCOUNT; ++i)
      solid_brushes[i].Set(settings.classes[i].color);
#endif

  thick_pen.Set(Layout::ScalePenWidth(10), COLOR_BLACK);
#ifndef ENABLE_OPENGL
  medium_pen.Set(Pen::SOLID, Layout::ScalePenWidth(3), COLOR_BLACK);
#endif

  intercept_icon.Load(IDB_AIRSPACEI, IDB_AIRSPACEI_HD);
}
