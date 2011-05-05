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

#include "RenderTaskPoint.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "WindowProjection.hpp"
#include "Task/Tasks/BaseTask/UnorderedTaskPoint.hpp"
#include "Task/TaskPoints/AATIsolineSegment.hpp"
#include "Task/TaskPoints/StartPoint.hpp"
#include "Task/TaskPoints/ASTPoint.hpp"
#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/FinishPoint.hpp"
#include "Math/Screen.hpp"
#include "RenderObservationZone.hpp"
#include "NMEA/Info.hpp"
#include "SettingsMap.hpp"
#include "Asset.hpp"

RenderTaskPoint::RenderTaskPoint(Canvas &_canvas, Canvas *_buffer,
                                 const WindowProjection &_projection,
                                 const SETTINGS_MAP &_settings_map,
                                 const TaskProjection &_task_projection,
                                 RenderObservationZone &_ozv,
                                 const bool draw_bearing,
                                 const GeoPoint &location)
  :canvas(_canvas), buffer(_buffer), m_proj(_projection),
   map_canvas(_canvas, _projection,
              _projection.GetScreenBounds().scale(fixed(1.1))),
   m_settings_map(_settings_map),
   task_projection(_task_projection),
   m_draw_bearing(draw_bearing),
   pen_leg_active(Pen::DASH, IBLSCALE(2), Graphics::TaskColor),
   pen_leg_inactive(Pen::DASH, IBLSCALE(1), Graphics::TaskColor),
   pen_leg_arrow(Pen::SOLID, IBLSCALE(1), Graphics::TaskColor),
   pen_isoline(Pen::SOLID, IBLSCALE(2), Color(0,0,255)), 
   m_index(0),
   ozv(_ozv),
   m_active_index(0),
   m_layer(RENDER_TASK_OZ_SHADE),
   m_location(location),
   bb_screen(FlatGeoPoint(0,0),0), // uninitialised!
   mode_optional_start(false)
{
}

void 
RenderTaskPoint::Visit(const UnorderedTaskPoint& tp) 
{
  if (m_layer == RENDER_TASK_LEG) {
    draw_task_line(m_location, tp.get_location_remaining());
  }
  if (m_layer == RENDER_TASK_SYMBOLS) {
    draw_bearing(tp);
  }
  m_index++;
}

void 
RenderTaskPoint::draw_ordered(const OrderedTaskPoint& tp) 
{
  const bool visible = tp.boundingbox_overlaps(bb_screen);

  if (visible && (m_layer == RENDER_TASK_OZ_SHADE)) {
    // draw shaded part of observation zone
    draw_oz_background(canvas, tp);
  }
  
  if (m_layer == RENDER_TASK_LEG) {
    if (m_index>0) {
      draw_task_line(m_last_point, tp.get_location_remaining());
    }
    m_last_point = tp.get_location_remaining();
  }
  
  if (visible && (m_layer == RENDER_TASK_OZ_OUTLINE)) {
    draw_oz_foreground(tp);
  }
}

void 
RenderTaskPoint::Visit(const StartPoint& tp) 
{
  m_index = 0;
  draw_ordered(tp);
  if (m_layer == RENDER_TASK_SYMBOLS) {
    draw_bearing(tp);
    draw_target(tp);
  }
}

void 
RenderTaskPoint::Visit(const FinishPoint& tp) 
{
  m_index++;
  draw_ordered(tp);
  if (m_layer == RENDER_TASK_SYMBOLS) {
    draw_bearing(tp);
    draw_target(tp);
  }
}

void 
RenderTaskPoint::Visit(const AATPoint& tp) 
{
  m_index++;
  
#ifndef ENABLE_OPENGL
  if (m_layer == RENDER_TASK_OZ_SHADE && buffer != NULL &&
      do_draw_deadzone(tp) && !is_ancient_hardware()) {
    // Draw clear area on top indicating part of OZ already travelled in
    // This provides a simple and intuitive visual representation of
    // where in the OZ to go to increase scoring distance.

    if (!tp.boundingbox_overlaps(bb_screen))
      return;

    const SearchPointVector &dead_zone = point_current()
      // scoring deadzone should include the area to the next destination
      ? tp.get_deadzone()
      // scoring deadzone is just the samples convex hull
      : tp.get_sample_points();

    /* need at least 3 points to draw the polygon */
    if (dead_zone.size() >= 3) {
      buffer->clear_white();

      /* draw the background shade into the buffer */
      draw_oz_background(*buffer, tp);

      /* now erase the dead zone by drawing a white polygon over it */
      buffer->null_pen();
      buffer->white_brush();
      MapCanvas map_canvas(*buffer, m_proj,
                           m_proj.GetScreenBounds().scale(fixed(1.1)));
      map_canvas.draw(dead_zone);

      /* copy the result into the canvas */
      /* we use copy_and() here to simulate Canvas::mix_mask() */
      canvas.copy_and(*buffer);
      return;
    }
  }
#endif

  draw_ordered(tp);

  if (m_layer == RENDER_TASK_SYMBOLS) {
    draw_isoline(tp);
    draw_bearing(tp);
    draw_target(tp);
  }
}

void 
RenderTaskPoint::Visit(const ASTPoint& tp) 
{
  m_index++;
  
  draw_ordered(tp);
  if (m_layer == RENDER_TASK_SYMBOLS) {
    draw_bearing(tp);
    draw_target(tp);
  }
}

bool 
RenderTaskPoint::do_draw_target(const TaskPoint &tp) const
{
  if (!tp.has_target()) {
    return false;
  }
  return (point_current()
          || m_settings_map.EnablePan 
          || m_settings_map.TargetPan);
}

void 
RenderTaskPoint::draw_bearing(const TaskPoint &tp) 
{
  if (!do_draw_bearing(tp)) 
    return;

  canvas.select(Graphics::hpBearing);
  map_canvas.offset_line(m_location, tp.get_location_remaining());
}

void 
RenderTaskPoint::draw_target(const TaskPoint &tp) 
{
  if (!do_draw_target(tp)) 
    return;
}

void 
RenderTaskPoint::draw_task_line(const GeoPoint& start, const GeoPoint& end) 
{
  canvas.select(leg_active() ? pen_leg_active : pen_leg_inactive);
  canvas.background_transparent();
  map_canvas.line(start, end);
  canvas.background_opaque();
  
  // draw small arrow along task direction
  RasterPoint p_p;
  RasterPoint Arrow[3] = { {6,6}, {-6,6}, {0,0} };
  
  const RasterPoint p_start = m_proj.GeoToScreen(start);
  const RasterPoint p_end = m_proj.GeoToScreen(end);
  
  const Angle ang = Angle::radians(atan2(fixed(p_end.x - p_start.x),
                                         fixed(p_start.y - p_end.y))).as_bearing();

  ScreenClosestPoint(p_start, p_end, m_proj.GetScreenOrigin(), &p_p, IBLSCALE(25));
  PolygonRotateShift(Arrow, 2, p_p.x, p_p.y, ang);
  Arrow[2] = Arrow[1];
  Arrow[1] = p_p;
  
  canvas.select(pen_leg_arrow);
  canvas.polyline(Arrow, 3);
}

void 
RenderTaskPoint::draw_isoline(const AATPoint& tp) 
{
  if (!tp.valid() || !do_draw_isoline(tp))
    return;

  AATIsolineSegment seg(tp, task_projection);
  if (!seg.valid()) {
    return;
  }

  #define fixed_twentieth fixed(1.0 / 20.0)
  
  if (m_proj.GeoToScreenDistance(seg.parametric(fixed_zero).
                                    distance(seg.parametric(fixed_one)))>2) {
    
    RasterPoint screen[20];
    for (unsigned i = 0; i < 20; ++i) {
      fixed t = i * fixed_twentieth;
      GeoPoint ga = seg.parametric(t);
      screen[i] = m_proj.GeoToScreen(ga);
    }

    canvas.select(pen_isoline);
    canvas.polyline(screen, 20);
  }
}

void 
RenderTaskPoint::draw_deadzone(const AATPoint& tp) 
{
  if (!do_draw_deadzone(tp) || is_ancient_hardware()) {
    return;
  }
  /*
    canvas.set_text_color(Graphics::Colours[m_settings_map.
    iAirspaceColour[1]]);
    // get brush, can be solid or a 1bpp bitmap
    canvas.select(Graphics::hAirspaceBrushes[m_settings_map.
    iAirspaceBrush[1]]);
    */

  // erase where aircraft has been
  canvas.white_brush();
  canvas.white_pen();
  
  if (point_current()) {
    // scoring deadzone should include the area to the next destination
    map_canvas.draw(tp.get_deadzone());
  } else {
    // scoring deadzone is just the samples convex hull
    map_canvas.draw(tp.get_sample_points());
  }
}

void 
RenderTaskPoint::draw_oz_background(Canvas &canvas, const OrderedTaskPoint& tp)
{
  ozv.set_layer(RenderObservationZone::LAYER_SHADE);

  if (ozv.draw_style(canvas, m_settings_map.airspace, m_index - m_active_index)) {
    ozv.Draw(canvas, m_proj, *tp.get_oz());
    ozv.un_draw_style(canvas);
  }
}

void 
RenderTaskPoint::draw_oz_foreground(const OrderedTaskPoint& tp) 
{
  int offset = m_index - m_active_index;
  if (mode_optional_start) {
    offset = -1; // render optional starts as deactivated
  }
  ozv.set_layer(RenderObservationZone::LAYER_INACTIVE);
  if (ozv.draw_style(canvas, m_settings_map.airspace, offset)) {
    ozv.Draw(canvas, m_proj, *tp.get_oz());
    ozv.un_draw_style(canvas);
  }

  ozv.set_layer(RenderObservationZone::LAYER_ACTIVE);
  if (ozv.draw_style(canvas, m_settings_map.airspace, offset)) {
    ozv.Draw(canvas, m_proj, *tp.get_oz());
    ozv.un_draw_style(canvas);
  }
}

