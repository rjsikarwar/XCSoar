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

#include "Math/Angle.hpp"
#include "HorizonRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Graphics.hpp"

void
DrawHorizon(Canvas &canvas, const PixelRect &rc,
            const ACCELERATION_STATE &acceleration)
{
  RasterPoint center;
  center.y = (rc.top+rc.bottom)/2;
  center.x = (rc.left+rc.right)/2;
  const int radius = min(rc.right-rc.left,rc.bottom-rc.top)/2-IBLSCALE(1);

  /*
  FEATURE TEMPORARILY DISABLED DUE TO USE OF XCSOAR IN FAI COMPETITIONS

  This feature of having a backup artificial horizon based on inferred
  orientation from GPS and vario data is useful, and reasonably well
  tested, but has the issue of potentially invalidating use of XCSoar in
  FAI contests due to rule ref Annex A to Section 3 (2010 Edition) 4.1.2
  "No instruments permitting pilots to fly without visual reference to
  the ground may be carried on board, even if made unserviceable."  The
  quality of XCSoar's pseudo-AH is arguably good enough that this
  violates the rule.  We need to seek clarification as to whether this
  is the case or not.
  */

  Pen hpHorizonSky(IBLSCALE(1), dark_color(Graphics::skyColor));
  Brush hbHorizonSky(Graphics::skyColor);
  Pen hpHorizonGround(IBLSCALE(1), dark_color(Graphics::GroundColor));

#define fixed_div fixed(1.0 / 35.0)
#define fixed_89 fixed_int_constant(89)

  fixed phi = max(-fixed_89, min(fixed_89,
      acceleration.BankAngle.value_degrees()));
  fixed alpha = fixed_rad_to_deg * acos(max(-fixed_one, min(fixed_one,
      acceleration.PitchAngle.value_degrees() * fixed_div)));
  fixed sphi = fixed_180 - phi;
  Angle alpha1 = Angle::degrees(sphi - alpha);
  Angle alpha2 = Angle::degrees(sphi + alpha);

  // draw sky part
  canvas.select(hpHorizonSky);
  canvas.select(hbHorizonSky);
  canvas.segment(center.x, center.y, radius, alpha2, alpha1, true);

  // draw ground part
  canvas.select(hpHorizonGround);
  canvas.select(Graphics::hbGround);
  canvas.segment(center.x, center.y, radius, alpha1, alpha2, true);

  const int s = radius/10;

  // draw 10 degree lines
  canvas.white_pen();
  canvas.line(center.x-2*s, center.y+3*s, center.x+2*s, center.y+3*s);
  canvas.line(center.x-2*s, center.y-3*s, center.x+2*s, center.y-3*s);
  canvas.line(center.x-3*s, center.y+6*s, center.x+3*s, center.y+6*s);
  canvas.line(center.x-3*s, center.y-6*s, center.x+3*s, center.y-6*s);

  // draw aircraft symbol
  Pen aircraft_pen(IBLSCALE(2), COLOR_BLACK);
  canvas.select(aircraft_pen);

  RasterPoint aircraft_w[7] = {{6*s, 0},
                               {2*s, 0},
                               {s, s},
                               {0, 0},
                               {-s, s},
                               {-2*s, 0},
                               {-6*s, 0}};
  for (int i=0; i<7; ++i) {
    aircraft_w[i].x += center.x;
    aircraft_w[i].y += center.y;
  }
  canvas.polyline(aircraft_w, 7);

  // draw 45 degree dash marks
  const unsigned rr2p = uround(radius * fixed_sqrt_half) + IBLSCALE(1);
  const unsigned rr2n = rr2p - IBLSCALE(2);
  canvas.line(center.x + rr2p, center.y - rr2p, center.x + rr2n, center.y - rr2n);
  canvas.line(center.x - rr2p, center.y - rr2p, center.x - rr2n, center.y - rr2n);
}
