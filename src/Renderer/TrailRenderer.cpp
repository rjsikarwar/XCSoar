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

#include "TrailRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Graphics.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "SettingsMap.hpp"
#include "Computer/GlideComputer.hpp"
#include "Projection/WindowProjection.hpp"
#include "Engine/Math/Earth.hpp"

#include <algorithm>

using std::min;
using std::max;

/**
 * This function returns the corresponding SnailTrail
 * color array index to the input
 * @param cv Input value between -1.0 and 1.0
 * @return SnailTrail color array index
 */
gcc_const
static int
GetSnailColorIndex(fixed cv)
{
  return max((short)0, min((short)(NUMSNAILCOLORS - 1),
                           (short)((cv + fixed_one) / 2 * NUMSNAILCOLORS)));
}

void
TrailRenderer::Draw(Canvas &canvas, const GlideComputer &glide_computer,
                    const WindowProjection &projection, unsigned min_time,
                    bool enable_traildrift, const RasterPoint pos,
                    const NMEAInfo &basic, const DerivedInfo &calculated,
                    const SETTINGS_MAP &settings) const
{
  if (settings.trail_length == TRAIL_OFF)
    return;

  trace.clear();
  glide_computer.LockedCopyTraceTo(trace, min_time,
                                   projection.GetGeoScreenCenter(),
                                   projection.DistancePixelsToMeters(3));
  if (trace.empty())
    return;

  if (!calculated.wind_available)
    enable_traildrift = false;

  GeoPoint traildrift;
  if (enable_traildrift) {
    GeoPoint tp1 = FindLatitudeLongitude(basic.location,
                                         calculated.wind.bearing,
                                         calculated.wind.norm);
    traildrift = basic.location - tp1;
  }

  fixed value_max, value_min;

  if (settings.snail_type == stAltitude) {
    value_max = fixed(1000);
    value_min = fixed(500);
    for (TracePointVector::const_iterator it = trace.begin();
         it != trace.end(); ++it) {
      value_max = max(it->GetAltitude(), value_max);
      value_min = min(it->GetAltitude(), value_min);
    }
  } else {
    value_max = fixed(0.75);
    value_min = fixed(-2.0);
    for (TracePointVector::const_iterator it = trace.begin();
         it != trace.end(); ++it) {
      value_max = max(it->GetVario(), value_max);
      value_min = min(it->GetVario(), value_min);
    }
    value_max = min(fixed(7.5), value_max);
    value_min = max(fixed(-5.0), value_min);
  }

  bool scaled_trail = settings.snail_scaling_enabled &&
                      projection.GetMapScale() <= fixed_int_constant(6000);

  const GeoBounds bounds = projection.GetScreenBounds().scale(fixed_four);

  RasterPoint last_point;
  bool last_valid = false;
  for (TracePointVector::const_iterator it = trace.begin(), end = trace.end();
       it != end; ++it) {
    const fixed dt = basic.time - fixed(it->time);
    const GeoPoint gp = enable_traildrift ?
      it->get_location().Parametric(traildrift, dt * it->drift_factor / 256) :
      it->get_location();
    if (!bounds.inside(gp)) {
      /* the point is outside of the MapWindow; don't paint it */
      last_valid = false;
      continue;
    }

    RasterPoint pt = projection.GeoToScreen(gp);

    if (last_valid) {
      if (settings.snail_type == stAltitude) {
        int index((it->GetAltitude() - value_min) / (value_max - value_min)
                  * (NUMSNAILCOLORS - 1));
        index = max(0, min(NUMSNAILCOLORS - 1, index));
        canvas.select(Graphics::hpSnail[index]);
	canvas.line_piece(last_point, pt);
      } else {
        const fixed colour_vario = negative(it->GetVario())
          ? - it->GetVario() / value_min
          : it->GetVario() / value_max ;

        if (!scaled_trail)||(dt > 60)
          canvas.select(Graphics::hpSnail[GetSnailColorIndex(colour_vario)]);
        else
          canvas.select(Graphics::hpSnailVario[GetSnailColorIndex(colour_vario)]);
        if (negative(it->GetVario())){
          canvas.null_pen();
          //canvas.black_brush();
          canvas.select(Graphics::hpSnailVarioNegative[GetSnailColorIndex(colour_vario)]);
          if (dt > 60)
	    canvas.circle(last_point.x + (pt.x - last_point.x)/2, last_point.y + (pt.y - last_point.y)/2, 3);
          else
	    canvas.circle(last_point.x + (pt.x - last_point.x)/2, last_point.y + (pt.y - last_point.y)/2, 3-(5*colour_vario));
        }
        else
          canvas.line_piece(last_point, pt);
      }
    }
    last_point = pt;
    last_valid = true;
  }

  canvas.line(last_point, pos);
}

void
TrailRenderer::DrawTraceVector(Canvas &canvas, const Projection &projection,
                               const TracePointVector &trace) const
{
  points.GrowDiscard(trace.size());

  unsigned n = 0;
  for (TracePointVector::const_iterator i = trace.begin(), end = trace.end();
       i != end; ++i)
    points[n++] = projection.GeoToScreen(i->get_location());

  canvas.polyline(points.begin(), n);
}

void
TrailRenderer::Draw(Canvas &canvas, const GlideComputer &glide_computer,
                    const WindowProjection &projection,
                    unsigned min_time) const
{
  trace.clear();
  glide_computer.LockedCopyTraceTo(trace, min_time,
                                   projection.GetGeoScreenCenter(),
                                   projection.DistancePixelsToMeters(4));
  if (trace.empty())
    return;

  canvas.select(Graphics::TracePen);
  DrawTraceVector(canvas, projection, trace);
}
