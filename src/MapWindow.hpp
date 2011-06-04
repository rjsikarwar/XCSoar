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

#ifndef XCSOAR_MAP_WINDOW_HPP
#define XCSOAR_MAP_WINDOW_HPP

#include "Util/StaticArray.hpp"
#include "MapWindowProjection.hpp"
#include "MapWindowTimer.hpp"
#include "Screen/DoubleBufferWindow.hpp"
#ifndef ENABLE_OPENGL
#include "Screen/BufferCanvas.hpp"
#endif
#include "Screen/LabelBlock.hpp"
#include "MapWindowBlackboard.hpp"
#include "NMEA/Derived.hpp"
#include "BackgroundDrawHelper.hpp"
#include "WayPoint/WayPointRenderer.hpp"
#include "Compiler.h"
#include <vector>

class TopographyStore;
class TopographyRenderer;
class RasterTerrain;
class RasterWeather;
class Marks;
class Waypoints;
class Waypoint;
class Airspaces;
class ProtectedAirspaceWarningManager;
class ProtectedTaskManager;
class GlidePolar;
class ContainerWindow;
class WayPointLabelList;

class MapWindow :
  public DoubleBufferWindow,
  public MapWindowBlackboard,
  public MapWindowTimer
{
#ifndef ENABLE_OPENGL
  // graphics vars

  BufferCanvas buffer_canvas;
  BufferCanvas stencil_canvas;
#endif

  LabelBlock label_block;

protected:
  /**
   * The projection as currently visible on the screen.  This object
   * is being edited by the user.
   */
  MapWindowProjection visible_projection;

#ifndef ENABLE_OPENGL
  /**
   * The projection of the buffer.  This differs from
   * visible_projection only after the projection was modified, until
   * the DrawThread has finished drawing the new projection.
   */
  MapWindowProjection buffer_projection;
#endif

  /**
   * The projection of the DrawThread.  This is used to render the new
   * map.  After rendering has completed, this object is copied over
   * #buffer_projection.
   */
  MapWindowProjection render_projection;

  const Waypoints *way_points;
  TopographyStore *topography;
  TopographyRenderer *topography_renderer;

  RasterTerrain *terrain;
  GeoPoint terrain_center;
  fixed terrain_radius;

  RasterWeather *weather;

  BackgroundDrawHelper m_background;
  WayPointRenderer way_point_renderer;

  Airspaces *airspace_database;
  ProtectedAirspaceWarningManager *airspace_warnings;
  ProtectedTaskManager *task;

  Marks *marks;

#ifndef ENABLE_OPENGL
  /**
   * Tracks whether the buffer canvas contains valid data.  We use
   * those attributes to prevent showing invalid data on the map, when
   * the user switches quickly to or from full-screen mode.
   */
  unsigned ui_generation, buffer_generation;

  /**
   * If non-zero, then the buffer will be scaled to the new
   * projection, and this variable is decremented.  This is used while
   * zooming and panning, to give instant visual feedback.
   */
  unsigned scale_buffer;
#endif

  StaticArray<GeoPoint,32> m_airspace_intersections;

  friend class DrawThread;

public:
  MapWindow();
  virtual ~MapWindow();

  virtual void set(ContainerWindow &parent, const PixelRect &rc);

  void set_way_points(const Waypoints *_way_points) {
    way_points = _way_points;
    way_point_renderer.set_way_points(way_points);
  }

  void set_task(ProtectedTaskManager *_task) {
    task = _task;
  }

  void set_airspaces(Airspaces *_airspace_database,
                     ProtectedAirspaceWarningManager *_airspace_warnings) {
    airspace_database = _airspace_database;
    airspace_warnings = _airspace_warnings;
  }

  void set_topography(TopographyStore *_topography);
  void set_terrain(RasterTerrain *_terrain);
  void set_weather(RasterWeather *_weather);

  void set_marks(Marks *_marks) {
    marks = _marks;
  }

  void ReadBlackboard(const NMEA_INFO &nmea_info,
                      const DERIVED_INFO &derived_info);

  void ReadBlackboard(const NMEA_INFO &nmea_info,
                      const DERIVED_INFO &derived_info,
                      const SETTINGS_COMPUTER &settings_computer,
                      const SETTINGS_MAP &settings_map);

  const MapWindowProjection &VisibleProjection() const {
    return visible_projection;
  }

  void SetLocation(const GeoPoint location) {
    visible_projection.SetGeoLocation(location);
  }

public:
  void DrawBestCruiseTrack(Canvas &canvas,
                           const RasterPoint aircraft_pos) const;
  void DrawTrackBearing(Canvas &canvas,
                        const RasterPoint aircraft_pos) const;
  void DrawCompass(Canvas &canvas, const PixelRect &rc) const;
  void DrawWind(Canvas &canvas, const RasterPoint &Orig,
                           const PixelRect &rc) const;
  void DrawAirspace(Canvas &canvas);
  void DrawAirspaceIntersections(Canvas &canvas) const;
  void DrawWaypoints(Canvas &canvas);

  void DrawTrail(Canvas &canvas, const RasterPoint aircraft_pos,
                 unsigned min_time, bool enable_traildrift = false) const;
  virtual void RenderTrail(Canvas &canvas, const RasterPoint aircraft_pos) const;
  void DrawTeammate(Canvas &canvas) const;
  void DrawTask(Canvas &canvas);
  void DrawTaskOffTrackIndicator(Canvas &canvas);
  virtual void DrawThermalEstimate(Canvas &canvas) const;

  void DrawGlideThroughTerrain(Canvas &canvas) const;
  void DrawTerrainAbove(Canvas &canvas);
  void DrawFLARMTraffic(Canvas &canvas, const RasterPoint aircraft_pos) const;

  // thread, main functions
  /**
   * Renders all the components of the moving map
   * @param canvas The drawing canvas
   * @param rc The area to draw in
   */
  virtual void Render(Canvas &canvas, const PixelRect &rc);

protected:
  unsigned UpdateTopography(unsigned max_update=1024);

  /**
   * @return true if UpdateTerrain() should be called again
   */
  bool UpdateTerrain();

  /**
   * @return true if UpdateWeather() should be called again
   */
  bool UpdateWeather();

  void UpdateAll() {
    UpdateTopography();
    UpdateTerrain();
    UpdateWeather();
  }

protected:
  virtual bool on_create();
  virtual bool on_destroy();
  virtual bool on_resize(unsigned width, unsigned height);

  virtual void on_paint(Canvas& canvas);
  virtual void on_paint_buffer(Canvas& canvas);
  bool do_deemphasise() const;

private:
  /**
   * Renders the terrain background
   * @param canvas The drawing canvas
   */
  void RenderTerrain(Canvas &canvas);
  /**
   * Renders the topography
   * @param canvas The drawing canvas
   */
  void RenderTopography(Canvas &canvas);
  /**
   * Renders the topography labels
   * @param canvas The drawing canvas
   */
  void RenderTopographyLabels(Canvas &canvas);
  /**
   * Renders the final glide shading
   * @param canvas The drawing canvas
   */
  void RenderFinalGlideShading(Canvas &canvas);
  /**
   * Renders the airspace
   * @param canvas The drawing canvas
   */
  void RenderAirspace(Canvas &canvas);
  /**
   * Renders the markers
   * @param canvas The drawing canvas
   */
  void RenderMarks(Canvas &canvas);
  /**
   * Render final glide through terrain marker
   * @param canvas The drawing canvas
   */
  void RenderGlide(Canvas &canvas);

public:
  void SetMapScale(const fixed x);
};

#endif
