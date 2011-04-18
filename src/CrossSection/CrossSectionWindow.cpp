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

#include "CrossSectionWindow.hpp"
#include "Components.hpp"
#include "GlideComputer.hpp"
#include "Interface.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Chart.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Graphics.hpp"
#include "Airspace/AirspaceIntersectionVisitor.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Units/Units.hpp"
#include "NMEA/Aircraft.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

#define AIRSPACE_SCANSIZE_X 16

/**
 * Local visitor class used for rendering airspaces in the CrossSectionWindow
 */
class AirspaceIntersectionVisitorSlice: public AirspaceIntersectionVisitor
{
public:
  /**
   * Constructor of the AirspaceIntersectionVisitorSlice class
   * @param _canvas The canvas to draw to
   * @param _chart Chart instance for scaling coordinates
   * @param _settings_map SettingsMap for colors, pens and brushes
   * @param _start GeoPoint at the left of the CrossSection
   * @param _state AltitudeState instance used for AGL-based airspaces
   */
  AirspaceIntersectionVisitorSlice(Canvas &_canvas, Chart &_chart,
                                   const SETTINGS_MAP &_settings_map,
                                   const GeoPoint _start,
                                   const ALTITUDE_STATE& _state) :
    canvas(_canvas), chart(_chart), settings_map(_settings_map),
    start(_start), state(_state) {}

  /**
   * Render an airspace box to the canvas
   * @param rc On-screen coordinates of the box
   * @param brush Brush to use
   * @param black Use black pen?
   * @param type Airspace class
   */
  void
  RenderBox(const PixelRect rc, const Brush &brush, bool black, int type)
  {
    // Enable "transparency" effect
#ifdef ENABLE_OPENGL
    GLBlend blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#elif !defined(ENABLE_SDL)
    canvas.mix_mask();
#endif /* GDI */

    // Use filling brush without outline
    canvas.select(brush);
    canvas.null_pen();

    // Draw thick brushed outlines
    int border_width = Layout::Scale(10);
    if ((rc.right - rc.left) > border_width * 2 &&
        (rc.bottom - rc.top) > border_width * 2) {
      PixelRect border = rc;
      border.left += border_width;
      border.right -= border_width;
      border.top += border_width;
      border.bottom -= border_width;

      // Left border
      canvas.rectangle(rc.left, rc.top, border.left, rc.bottom);

      // Right border
      canvas.rectangle(border.right, rc.top, rc.right, rc.bottom);

      // Bottom border
      canvas.rectangle(border.left, border.bottom, border.right, rc.bottom);

      // Top border
      canvas.rectangle(border.left, rc.top, border.right, border.top);
    } else {
      // .. or fill the entire rect if the outlines would overlap
      canvas.rectangle(rc.left, rc.top, rc.right, rc.bottom);
    }

    // Disable "transparency" effect
#ifdef ENABLE_OPENGL
    glDisable(GL_BLEND);
#elif !defined(ENABLE_SDL)
    canvas.mix_copy();
#endif /* GDI */

    // Use transparent brush and type-dependent pen for the outlines
    canvas.hollow_brush();
    if (black)
      canvas.black_pen();
    else
      canvas.select(Graphics::hAirspacePens[type]);

    // Draw thin outlines
    canvas.rectangle(rc.left, rc.top, rc.right, rc.bottom);
  }

  /**
   * Renders the AbstractAirspace on the canvas
   * @param as AbstractAirspace to render
   */
  void
  Render(const AbstractAirspace& as)
  {
    int type = as.get_type();
    if (type <= 0)
      return;

    // No intersections for this airspace
    if (m_intersections.empty())
      return;

    // Select pens and brushes
#ifdef ENABLE_SDL
    Color color =
      Graphics::GetAirspaceColour(settings_map.iAirspaceColour[type]);
#ifdef ENABLE_OPENGL
    color = color.with_alpha(48);
#endif
    Brush brush(color);
#else
    const Brush &brush = Graphics::GetAirspaceBrushByClass(type, settings_map);
    canvas.set_text_color(light_color(
        Graphics::GetAirspaceColourByClass(type, settings_map)));
#endif

    PixelRect rcd;
    // Calculate top and bottom coordinate
    rcd.top = chart.screenY(as.get_top_altitude(state));
    if (as.is_base_terrain())
      rcd.bottom = chart.screenY(fixed_zero);
    else
      rcd.bottom = chart.screenY(as.get_base_altitude(state));

    // Iterate through the intersections
    for (AirspaceIntersectionVector::const_iterator it = m_intersections.begin();
         it != m_intersections.end(); ++it) {
      const GeoPoint p_start = it->first;
      const GeoPoint p_end = it->second;
      const fixed distance_start = start.distance(p_start);
      const fixed distance_end = start.distance(p_end);

      // Determine left and right coordinate
      rcd.left = chart.screenX(distance_start);
      rcd.right = chart.screenX(distance_end);

      // only one edge found, next edge must be beyond screen
      if ((rcd.left == rcd.right) && (p_start == p_end)) {
        rcd.right = chart.screenX(chart.getXmax());
      }

      // Draw the airspace
      RenderBox(rcd, brush, settings_map.bAirspaceBlackOutline, type);
    }
  }

  /**
   * Visitor function for intersectingAirspaceCircle objects
   * @param as Intersecting AirspaceCircle instance
   */
  void
  Visit(const AirspaceCircle& as)
  {
    Render(as);
  }

  /**
   * Visitor function for intersecting AirspacePolygon objects
   * @param as Intersecting AirspacePolygon instance
   */
  void
  Visit(const AirspacePolygon& as)
  {
    Render(as);
  }

private:
  /** Canvas to draw on */
  Canvas& canvas;
  /** Chart for scaling the airspace CrossSection */
  Chart& chart;
  /** SettingsMap for reading airspace colors, pen and brushes */
  const SETTINGS_MAP& settings_map;
  /** GeoPoint at the left of the CrossSection */
  const GeoPoint start;
  /** AltitudeState instance used for AGL-based airspaces */
  const ALTITUDE_STATE& state;
};

CrossSectionWindow::CrossSectionWindow() :
  terrain_brush(Graphics::GroundColor),
  grid_pen(Pen::DASH, 1, Color(0x60, 0x60, 0x60)),
  terrain(NULL), airspace_database(NULL),
  start(Angle::native(fixed_zero), Angle::native(fixed_zero)),
  vec(fixed(50000), Angle::native(fixed_zero)),
  background_color(Color::WHITE), text_color(Color::BLACK) {}

void
CrossSectionWindow::ReadBlackboard(const NMEA_INFO &_gps_info,
                                   const DERIVED_INFO &_calculated_info,
                                   const SETTINGS_MAP &_settings_map)
{
  gps_info = _gps_info;
  calculated_info = _calculated_info;
  settings_map = _settings_map;
}

void
CrossSectionWindow::Paint(Canvas &canvas, const PixelRect rc)
{
  fixed hmin = max(fixed_zero, gps_info.NavAltitude - fixed(3300));
  fixed hmax = max(fixed(3300), gps_info.NavAltitude + fixed(1000));

  Chart chart(canvas, rc);
  chart.ResetScale();
  chart.ScaleXFromValue(fixed_zero);
  chart.ScaleXFromValue(vec.Distance);
  chart.ScaleYFromValue(hmin);
  chart.ScaleYFromValue(hmax);

  PaintAirspaces(canvas, chart);
  PaintTerrain(canvas, chart);
  PaintGlide(chart);
  PaintAircraft(canvas, chart, rc);
  PaintGrid(canvas, chart);
}

void
CrossSectionWindow::PaintAirspaces(Canvas &canvas, Chart &chart)
{
  // Quit early if no airspace database available
  if (airspace_database == NULL)
    return;

  // Create IntersectionVisitor to render to the canvas
  AirspaceIntersectionVisitorSlice ivisitor(canvas, chart, settings_map,
                                            start,
                                            ToAircraftState(Basic(),
                                                            Calculated()));

  // Call visitor with intersecting airspaces
  airspace_database->visit_intersecting(start, vec, ivisitor);
}

void
CrossSectionWindow::PaintTerrain(Canvas &canvas, Chart &chart)
{
  if (terrain == NULL)
    return;

  const GeoPoint p_diff = vec.end_point(start) - start;

  RasterTerrain::Lease map(*terrain);

  RasterPoint points[2 + AIRSPACE_SCANSIZE_X];

  points[0].x = chart.screenX(vec.Distance);
  points[0].y = chart.screenY(fixed_zero);
  points[1].x = chart.screenX(fixed_zero);
  points[1].y = chart.screenY(fixed_zero);

  unsigned i = 2;
  for (unsigned j = 0; j < AIRSPACE_SCANSIZE_X; ++j) {
    const fixed t_this = fixed(j) / (AIRSPACE_SCANSIZE_X - 1);
    const GeoPoint p_this = start + p_diff * t_this;

    short h = map->GetHeight(p_this);
    if (RasterBuffer::is_special(h)) {
      if (RasterBuffer::is_water(h))
        /* water is at 0m MSL */
        /* XXX paint in blue? */
        h = 0;
      else
        /* skip "unknown" values */
        continue;
    }

    points[i].x = chart.screenX(t_this * vec.Distance);
    points[i].y = chart.screenY(fixed(h));
    i++;
  }

  if (i >= 4) {
    canvas.null_pen();
    canvas.select(terrain_brush);
    canvas.polygon(&points[0], i);
  }
}

void
CrossSectionWindow::PaintGlide(Chart &chart)
{
  if (gps_info.GroundSpeed > fixed(10)) {
    fixed t = vec.Distance / gps_info.GroundSpeed;
    chart.DrawLine(fixed_zero, gps_info.NavAltitude, vec.Distance,
                   gps_info.NavAltitude + calculated_info.Average30s * t,
                   Chart::STYLE_BLUETHIN);
  }
}

void
CrossSectionWindow::PaintAircraft(Canvas &canvas, const Chart &chart,
                                  const PixelRect rc)
{
  Brush brush(text_color);
  canvas.select(brush);

  Pen pen(1, text_color);
  canvas.select(pen);

  RasterPoint line[4];
  line[0].x = chart.screenX(fixed_zero);
  line[0].y = chart.screenY(gps_info.NavAltitude);
  line[1].x = rc.left;
  line[1].y = line[0].y;
  line[2].x = line[1].x;
  line[2].y = line[0].y - (line[0].x - line[1].x) / 2;
  line[3].x = (line[1].x + line[0].x) / 2;
  line[3].y = line[0].y;
  canvas.polygon(line, 4);
}

void
CrossSectionWindow::PaintGrid(Canvas &canvas, Chart &chart)
{
  canvas.set_text_color(text_color);

  chart.DrawXGrid(Units::ToSysDistance(fixed(5)), fixed_zero,
                  grid_pen, fixed(5), true);
  chart.DrawYGrid(Units::ToSysAltitude(fixed(1000)), fixed_zero,
                  grid_pen, fixed(1000), true);

  chart.DrawXLabel(_T("D"));
  chart.DrawYLabel(_T("h"));
}

void
CrossSectionWindow::on_paint(Canvas &canvas)
{
  canvas.clear(background_color);
  canvas.set_text_color(text_color);
  canvas.select(Fonts::Map);

  const PixelRect rc = get_client_rect();

  Paint(canvas, rc);
}

bool
CrossSectionWindow::on_mouse_down(int x, int y)
{
  if (mOnMouseDownCallback)
    return mOnMouseDownCallback(this, x, y);

  return false;
}

bool
CrossSectionWindow::on_mouse_up(int x, int y)
{
  if (mOnMouseUpCallback)
    return mOnMouseUpCallback(this, x, y);

  return false;
}

bool
CrossSectionWindow::on_mouse_move(int x, int y, unsigned keys)
{
  if (mOnMouseMoveCallback)
    return mOnMouseMoveCallback(this, x, y, keys);

  return false;
}
