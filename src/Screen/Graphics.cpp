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
#include "Screen/Graphics.hpp"
#include "Screen/Point.hpp"
#include "Screen/UnitSymbol.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Ramp.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Icon.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Color.hpp"
#include "Screen/Pen.hpp"
#include "Screen/Canvas.hpp"
#include "Math/Screen.hpp"
#include "Appearance.hpp"
#include "SettingsMap.hpp"
#include "resource.h"
#include "Asset.hpp"
#include "LogFile.hpp"

Pen Graphics::hAirspacePens[AIRSPACECLASSCOUNT];

#ifndef ENABLE_SDL
Brush Graphics::hAirspaceBrushes[NUMAIRSPACEBRUSHES];
Bitmap Graphics::hAirspaceBitmap[NUMAIRSPACEBRUSHES];
#endif

#ifdef HAVE_ALPHA_BLEND
Brush Graphics::solid_airspace_brushes[NUMAIRSPACECOLORS];
#endif

Pen Graphics::hpSnail[NUMSNAILCOLORS];
Pen Graphics::hpSnailVario[NUMSNAILCOLORS];

#ifndef ENABLE_SDL
Bitmap Graphics::hAboveTerrainBitmap;
Brush Graphics::hAboveTerrainBrush;
#endif

MaskedIcon Graphics::hAirspaceInterceptBitmap;
MaskedIcon Graphics::hTerrainWarning;
MaskedIcon Graphics::hFLARMTraffic;
MaskedIcon Graphics::hLogger, Graphics::hLoggerOff;
MaskedIcon Graphics::hCruise, Graphics::hClimb,
           Graphics::hFinalGlide, Graphics::hAbort;
MaskedIcon Graphics::hGPSStatus1, Graphics::hGPSStatus2;

Pen Graphics::hpAircraft;
Pen Graphics::hpAircraftSimple1;
Pen Graphics::hpAircraftSimple2;
Pen Graphics::hpCanopy;
Pen Graphics::hpTaskActive;
Pen Graphics::hpTaskInactive;
Pen Graphics::hpTaskArrow;
Pen Graphics::hpWind;
Pen Graphics::hpIsoline;
Pen Graphics::hpBearing;
Pen Graphics::hpBestCruiseTrack;
Pen Graphics::hpCompass;
Pen Graphics::hpFinalGlideAbove;
Pen Graphics::hpFinalGlideBelow;
Pen Graphics::hpFinalGlideBelowLandable;
Pen Graphics::hpMapScale;
Pen Graphics::hpTerrainLine;
Pen Graphics::hpTerrainLineThick;
Pen Graphics::hpTrackBearingLine;
Pen Graphics::TracePen;
Pen Graphics::ContestPen[3];

Brush Graphics::hbCanopy;
Brush Graphics::hbCompass;
Brush Graphics::hbThermalBand;
Brush Graphics::hbBestCruiseTrack;
Brush Graphics::hbFinalGlideBelow;
Brush Graphics::hbFinalGlideBelowLandable;
Brush Graphics::hbFinalGlideAbove;
Brush Graphics::hbWind;

MaskedIcon Graphics::SmallIcon, Graphics::TurnPointIcon;
MaskedIcon Graphics::MountainTopIcon, Graphics::BridgeIcon, Graphics::TunnelIcon;
MaskedIcon Graphics::TowerIcon, Graphics::PowerPlantIcon;
MaskedIcon Graphics::AirportReachableIcon, Graphics::AirportUnreachableIcon;
MaskedIcon Graphics::AirportMarginalIcon, Graphics::FieldMarginalIcon;
MaskedIcon Graphics::FieldReachableIcon, Graphics::FieldUnreachableIcon;
MaskedIcon Graphics::hBmpThermalSource;
MaskedIcon Graphics::hBmpTarget;
MaskedIcon Graphics::hBmpTeammatePosition;

MaskedIcon Graphics::hBmpMapScaleLeft;
MaskedIcon Graphics::hBmpMapScaleRight;

Bitmap Graphics::hBmpTabTask;
Bitmap Graphics::hBmpTabWrench;
Bitmap Graphics::hBmpTabSettings;
Bitmap Graphics::hBmpTabCalculator;

// used for flarm
Brush Graphics::AlarmBrush;
Brush Graphics::WarningBrush;
Brush Graphics::TrafficBrush;

// used for landable rendering
Brush Graphics::hbGreen;
Brush Graphics::hbWhite;
Brush Graphics::hbMagenta;
Brush Graphics::hbOrange;
Brush Graphics::hbRed;
Brush Graphics::hbLightGray;
Brush Graphics::hbNotReachableTerrain;
Brush Graphics::hbGround;

// airspace brushes/colours
const Color
Graphics::GetAirspaceColour(const int i)
{
  return Colours[i];
}

#ifndef ENABLE_SDL
const Brush &
Graphics::GetAirspaceBrush(const int i)
{
  return hAirspaceBrushes[i];
}
#endif

const Color
Graphics::GetAirspaceColourByClass(const int i,
                                   const AirspaceRendererSettings &settings)
{
  return GetAirspaceColour(settings.colours[i]);
}

#ifndef ENABLE_SDL
const Brush &
Graphics::GetAirspaceBrushByClass(const int i,
                                  const AirspaceRendererSettings &settings)
{
  return GetAirspaceBrush(settings.brushes[i]);
}
#endif

const Color Graphics::inv_redColor = Color(0xff, 0x70, 0x70);
const Color Graphics::inv_blueColor = Color(0x90, 0x90, 0xff);
const Color Graphics::inv_yellowColor = Color::YELLOW;
const Color Graphics::inv_greenColor = Color::GREEN;
const Color Graphics::inv_magentaColor = Color::MAGENTA;

const Color Graphics::cAlertSafe = Color(0x29,0xb2,0x14);
const Color Graphics::cAlertWarning = Color(0xfe,0x84,0x38);
const Color Graphics::cAlertAlarm = Color(0xfb,0x35,0x2f);

static Color clrSepia(0x78,0x31,0x18);
const Color Graphics::GroundColor = Color::BROWN;
const Color Graphics::skyColor = Color(0x0a,0xb9,0xf3);
const Color Graphics::seaColor = Color(0xbd,0xc5,0xd5); // ICAO open water area

// Magenta ICAO color is 0x65,0x23,0x1c
const Color Graphics::TaskColor = Color(0x62,0x4e,0x90);
const Color Graphics::IsolineColor = BearingColor;
const Color Graphics::BearingColor = Color(0x3e,0x30,0x5f);

const Color Graphics::inv_sinkColor = Color(0xc4, 0x80, 0x1e);
const Color Graphics::inv_liftColor = Color(0x1e, 0xf1, 0x73);
const Color Graphics::sinkColor = Color(0xa3,0x69,0x0d);
const Color Graphics::liftColor = Color(0x19,0x94,0x03);

const Color Graphics::Colours[] = {
  Color::RED,
  Color::GREEN,
  Color::BLUE,
  Color::YELLOW,
  Color::MAGENTA,
  Color::CYAN,
  dark_color(Color::RED),
  dark_color(Color::GREEN),
  dark_color(Color::BLUE),
  dark_color(Color::YELLOW),
  dark_color(Color::MAGENTA),
  dark_color(Color::CYAN),
  Color::WHITE,
  Color::LIGHT_GRAY,
  Color::GRAY,
  Color::BLACK,
};

void
Graphics::Initialise()
{
  /// @todo enhancement: support red/green color blind pilots with adjusted colour scheme

  LogStartUp(_T("Initialise graphics"));

  LoadUnitSymbols();

  AlarmBrush.set(cAlertAlarm);
  WarningBrush.set(cAlertWarning);
  TrafficBrush.set(cAlertSafe);

  hFLARMTraffic.load_big(IDB_FLARMTRAFFIC, IDB_FLARMTRAFFIC_HD);
  hTerrainWarning.load_big(IDB_TERRAINWARNING, IDB_TERRAINWARNING_HD);
  hGPSStatus1.load_big(IDB_GPSSTATUS1, IDB_GPSSTATUS1_HD, false);
  hGPSStatus2.load_big(IDB_GPSSTATUS2, IDB_GPSSTATUS2_HD, false);
  hLogger.load_big(IDB_LOGGER, IDB_LOGGER_HD);
  hLoggerOff.load_big(IDB_LOGGEROFF, IDB_LOGGEROFF_HD);
  hBmpTeammatePosition.load_big(IDB_TEAMMATE_POS, IDB_TEAMMATE_POS_HD);

  hCruise.load_big(IDB_CRUISE, IDB_CRUISE_HD, false);
  hClimb.load_big(IDB_CLIMB, IDB_CLIMB_HD, false);
  hFinalGlide.load_big(IDB_FINALGLIDE, IDB_FINALGLIDE_HD, false);
  hAbort.load_big(IDB_ABORT, IDB_ABORT_HD, false);

  // airspace brushes and colors
#ifndef ENABLE_SDL
  hAirspaceBitmap[0].load(IDB_AIRSPACE0);
  hAirspaceBitmap[1].load(IDB_AIRSPACE1);
  hAirspaceBitmap[2].load(IDB_AIRSPACE2);
  hAirspaceBitmap[3].load(IDB_AIRSPACE3);
  hAirspaceBitmap[4].load(IDB_AIRSPACE4);
  hAirspaceBitmap[5].load(IDB_AIRSPACE5);
  hAirspaceBitmap[6].load(IDB_AIRSPACE6);
  hAirspaceBitmap[7].load(IDB_AIRSPACE7);
#endif

  hAirspaceInterceptBitmap.load_big(IDB_AIRSPACEI, IDB_AIRSPACEI_HD);

#ifndef ENABLE_SDL
  hAboveTerrainBitmap.load(IDB_ABOVETERRAIN);

  for (int i = 0; i < NUMAIRSPACEBRUSHES; i++)
    hAirspaceBrushes[i].set(hAirspaceBitmap[i]);

  hAboveTerrainBrush.set(hAboveTerrainBitmap);
#endif

#ifdef HAVE_ALPHA_BLEND
  if (AlphaBlendAvailable())
    for (unsigned i = 0; i < NUMAIRSPACECOLORS; ++i)
      solid_airspace_brushes[i].set(Colours[i]);
#endif

  hpIsoline.set(Pen::DASH, Layout::Scale(1), IsolineColor);

  hpWind.set(Layout::Scale(1), dark_color(Color::GRAY));
  hbWind.set(Color::GRAY);

  hpTaskActive.set(Pen::DASH, Layout::Scale(2), Graphics::TaskColor);
  hpTaskInactive.set(Pen::DASH, Layout::Scale(1), Graphics::TaskColor);
  hpTaskArrow.set(Layout::Scale(1), Graphics::TaskColor);

  hBmpMapScaleLeft.load_big(IDB_MAPSCALE_LEFT, IDB_MAPSCALE_LEFT_HD, false);
  hBmpMapScaleRight.load_big(IDB_MAPSCALE_RIGHT, IDB_MAPSCALE_RIGHT_HD, false);

  hBmpTabTask.load((Layout::scale > 1) ? IDB_TASK_HD : IDB_TASK);
  hBmpTabWrench.load((Layout::scale > 1) ? IDB_WRENCH_HD : IDB_WRENCH);
  hBmpTabSettings.load((Layout::scale > 1) ? IDB_SETTINGS_HD : IDB_SETTINGS);
  hBmpTabCalculator.load((Layout::scale > 1) ? IDB_CALCULATOR_HD : IDB_CALCULATOR);

  hBmpThermalSource.load_big(IDB_THERMALSOURCE, IDB_THERMALSOURCE_HD);
  hBmpTarget.load_big(IDB_TARGET, IDB_TARGET_HD);

  hbCompass.set(Color(207, 207, 207));

  hbThermalBand.set(skyColor);

  hbFinalGlideBelow.set(Color::RED);
  hpFinalGlideBelow.set(Layout::Scale(1), dark_color(Color::RED));

  hbFinalGlideBelowLandable.set(Color::ORANGE);
  hpFinalGlideBelowLandable.set(Layout::Scale(1), dark_color(Color::ORANGE));

  hbFinalGlideAbove.set(Color::GREEN);
  hpFinalGlideAbove.set(Layout::Scale(1), dark_color(Color::GREEN));

  hpBearing.set(Layout::Scale(2), BearingColor);
  hpBestCruiseTrack.set(Layout::Scale(1), dark_color(BearingColor));
  hbBestCruiseTrack.set(BearingColor);

  hpCompass.set(Layout::Scale(1), Color::GRAY);

  hpMapScale.set(Layout::Scale(1), Color::BLACK);

  hpTerrainLine.set(Pen::DASH, Layout::Scale(1), clrSepia);
  hpTerrainLineThick.set(Pen::DASH, Layout::Scale(2), clrSepia);

  TracePen.set(2, Color(50, 243, 45));
  ContestPen[0].set(Layout::Scale(1)+2, Color::RED);
  ContestPen[1].set(Layout::Scale(1)+1, Color::ORANGE);
  ContestPen[2].set(Layout::Scale(1), Color::BLUE);

  SmallIcon.load_big(IDB_SMALL, IDB_SMALL_HD);
  TurnPointIcon.load_big(IDB_TURNPOINT, IDB_TURNPOINT_HD);
  MountainTopIcon.load_big(IDB_MOUNTAIN_TOP, IDB_MOUNTAIN_TOP_HD);
  BridgeIcon.load_big(IDB_BRIDGE, IDB_BRIDGE_HD);
  TunnelIcon.load_big(IDB_TUNNEL, IDB_TUNNEL_HD);
  TowerIcon.load_big(IDB_TOWER, IDB_TOWER_HD);
  PowerPlantIcon.load_big(IDB_POWER_PLANT, IDB_POWER_PLANT_HD);

  hpAircraft.set(1, Color::DARK_GRAY);
  hpAircraftSimple1.set(Layout::Scale(1), Color::BLACK);
  hpAircraftSimple2.set(Layout::Scale(3), Color::WHITE);
  hpCanopy.set(1, dark_color(Color::CYAN));
  hbCanopy.set(Color::CYAN);

    // used for landable rendering
  hbGreen.set(Color::GREEN);
  hbWhite.set(Color::WHITE);
  hbMagenta.set(Color::MAGENTA);
  hbOrange.set(Color::ORANGE);
  hbRed.set(Color::RED);
  hbLightGray.set(Color::LIGHT_GRAY);
  hbNotReachableTerrain.set(light_color(Color::RED));

  hbGround.set(GroundColor);

  hpTrackBearingLine.set(3, Color::GRAY);
}

void
Graphics::InitialiseConfigured(const SETTINGS_MAP &settings_map)
{
  InitSnailTrail(settings_map);
  InitLandableIcons();
  InitAirspacePens(settings_map.airspace);
}

void
Graphics::InitSnailTrail(const SETTINGS_MAP &settings_map)
{
  const ColorRamp snail_colors_vario[] = {
    {0,   0xc4, 0x80, 0x1e}, // sinkColor
    {100, 0xa0, 0xa0, 0xa0},
    {200, 0x1e, 0xf1, 0x73} // liftColor
  };

  const ColorRamp snail_colors_vario2[] = {
    {0,   0x00, 0x00, 0xff},
    {99,  0x00, 0xff, 0xff},
    {100, 0xff, 0xff, 0x00},
    {200, 0xff, 0x00, 0x00}
  };

  const ColorRamp snail_colors_alt[] = {
    {0,   0xff, 0x00, 0x00},
    {50,  0xff, 0xff, 0x00},
    {100, 0x00, 0xff, 0x00},
    {150, 0x00, 0xff, 0xff},
    {200, 0x00, 0x00, 0xff},
  };

  int iwidth;
  int minwidth = Layout::Scale(2);

  for (int i = 0; i < NUMSNAILCOLORS; i++) {
    short ih = i * 200 / (NUMSNAILCOLORS - 1);
    Color color = (settings_map.SnailType == stAltitude) ?
                  ColorRampLookup(ih, snail_colors_alt, 5) :
                  (settings_map.SnailType == stSeeYouVario) ?
                  ColorRampLookup(ih, snail_colors_vario2, 4) :
                  ColorRampLookup(ih, snail_colors_vario, 3);

    if (i < NUMSNAILCOLORS / 2 ||
        !settings_map.SnailScaling)
      iwidth = minwidth;
    else
      iwidth = max(minwidth, (i - NUMSNAILCOLORS / 2) *
                             Layout::Scale(16) / NUMSNAILCOLORS);

    hpSnail[i].set(minwidth, color);
    hpSnailVario[i].set(iwidth, color);
  }
}

void
Graphics::InitLandableIcons()
{
  if (Appearance.IndLandable == wpLandableWinPilot) {
    AirportReachableIcon.load_big(IDB_REACHABLE, IDB_REACHABLE_HD);
    AirportMarginalIcon.load_big(IDB_MARGINAL, IDB_MARGINAL_HD);
    AirportUnreachableIcon.load_big(IDB_LANDABLE, IDB_LANDABLE_HD);
    FieldReachableIcon.load_big(IDB_REACHABLE, IDB_REACHABLE_HD);
    FieldMarginalIcon.load_big(IDB_MARGINAL, IDB_MARGINAL_HD);
    FieldUnreachableIcon.load_big(IDB_LANDABLE, IDB_LANDABLE_HD);
  } else if (Appearance.IndLandable == wpLandableAltA) {
    AirportReachableIcon.load_big(IDB_AIRPORT_REACHABLE,
                                  IDB_AIRPORT_REACHABLE_HD);
    AirportMarginalIcon.load_big(IDB_AIRPORT_MARGINAL,
                                 IDB_AIRPORT_MARGINAL_HD);
    AirportUnreachableIcon.load_big(IDB_AIRPORT_UNREACHABLE,
                                    IDB_AIRPORT_UNREACHABLE_HD);
    FieldReachableIcon.load_big(IDB_OUTFIELD_REACHABLE,
                                IDB_OUTFIELD_REACHABLE_HD);
    FieldMarginalIcon.load_big(IDB_OUTFIELD_MARGINAL,
                               IDB_OUTFIELD_MARGINAL_HD);
    FieldUnreachableIcon.load_big(IDB_OUTFIELD_UNREACHABLE,
                                  IDB_OUTFIELD_UNREACHABLE_HD);
  } else if (Appearance.IndLandable == wpLandableAltB) {
    AirportReachableIcon.load_big(IDB_AIRPORT_REACHABLE,
                                  IDB_AIRPORT_REACHABLE_HD);
    AirportMarginalIcon.load_big(IDB_AIRPORT_MARGINAL2,
                                 IDB_AIRPORT_MARGINAL2_HD);
    AirportUnreachableIcon.load_big(IDB_AIRPORT_UNREACHABLE2,
                                    IDB_AIRPORT_UNREACHABLE2_HD);
    FieldReachableIcon.load_big(IDB_OUTFIELD_REACHABLE,
                                IDB_OUTFIELD_REACHABLE_HD);
    FieldMarginalIcon.load_big(IDB_OUTFIELD_MARGINAL2,
                               IDB_OUTFIELD_MARGINAL2_HD);
    FieldUnreachableIcon.load_big(IDB_OUTFIELD_UNREACHABLE2,
                                  IDB_OUTFIELD_UNREACHABLE2_HD);
  }
}

void
Graphics::InitAirspacePens(const AirspaceRendererSettings &settings)
{
  for (int i = 0; i < AIRSPACECLASSCOUNT; i++)
    hAirspacePens[i].set(Layout::Scale(2),
                         GetAirspaceColourByClass(i, settings));
}

void
Graphics::Deinitialise()
{
  DeinitialiseUnitSymbols();

  AlarmBrush.reset();
  WarningBrush.reset();
  TrafficBrush.reset();

  hFLARMTraffic.reset();
  hTerrainWarning.reset();
  hGPSStatus1.reset();
  hGPSStatus2.reset();
  hLogger.reset();
  hLoggerOff.reset();
  hBmpTeammatePosition.reset();

  hCruise.reset();
  hClimb.reset();
  hFinalGlide.reset();
  hAbort.reset();

  hAirspaceInterceptBitmap.reset();

#ifndef ENABLE_SDL
  for (unsigned i = 0; i < NUMAIRSPACEBRUSHES; i++) {
    hAirspaceBrushes[i].reset();
    hAirspaceBitmap[i].reset();
  }

  hAboveTerrainBrush.reset();
  hAboveTerrainBitmap.reset();
#endif

#ifdef HAVE_ALPHA_BLEND
  if (AlphaBlendAvailable())
    for (unsigned i = 0; i < NUMAIRSPACECOLORS; ++i)
      solid_airspace_brushes[i].reset();
#endif

  for (unsigned i = 0; i < AIRSPACECLASSCOUNT; i++)
    hAirspacePens[i].reset();

  hbWind.reset();

  hBmpMapScaleLeft.reset();
  hBmpMapScaleRight.reset();

  hBmpTabTask.reset();
  hBmpTabWrench.reset();
  hBmpTabSettings.reset();
  hBmpTabCalculator.reset();

  hBmpThermalSource.reset();
  hBmpTarget.reset();

  hbCompass.reset();

  hbThermalBand.reset();
  hbBestCruiseTrack.reset();
  hbFinalGlideBelow.reset();
  hbFinalGlideBelowLandable.reset();
  hbFinalGlideAbove.reset();

  hpTaskActive.reset();
  hpTaskInactive.reset();
  hpTaskArrow.reset();

  hpWind.reset();
  hpIsoline.reset();

  hpBearing.reset();
  hpBestCruiseTrack.reset();
  hpCompass.reset();

  hpFinalGlideBelow.reset();
  hpFinalGlideBelowLandable.reset();
  hpFinalGlideAbove.reset();

  hpMapScale.reset();
  hpTerrainLine.reset();
  hpTerrainLineThick.reset();

  TracePen.reset();
  ContestPen[0].reset();
  ContestPen[1].reset();
  ContestPen[2].reset();

  SmallIcon.reset();
  TurnPointIcon.reset();
  MountainTopIcon.reset();
  BridgeIcon.reset();
  TunnelIcon.reset();
  TowerIcon.reset();
  PowerPlantIcon.reset();

  hpAircraft.reset();
  hpAircraftSimple1.reset();
  hpAircraftSimple2.reset();
  hpCanopy.reset();
  hbCanopy.reset();

  hbGreen.reset();
  hbWhite.reset();
  hbMagenta.reset();
  hbOrange.reset();
  hbRed.reset();
  hbLightGray.reset();
  hbNotReachableTerrain.reset();

  hbGround.reset();

  hpTrackBearingLine.reset();

  for (unsigned i = 0; i < NUMSNAILCOLORS; i++) {
    hpSnail[i].reset();
    hpSnailVario[i].reset();
  }

  AirportReachableIcon.reset();
  AirportUnreachableIcon.reset();
  AirportMarginalIcon.reset();
  FieldMarginalIcon.reset();
  FieldReachableIcon.reset();
  FieldUnreachableIcon.reset();
}

static void
DrawMirroredPolygon(const RasterPoint *src, RasterPoint *dst, unsigned points,
                    Canvas &canvas, const Angle angle,
                    const RasterPoint pos)
{
  std::copy(src, src + points, dst);
  for (unsigned i = 0; i < points; ++i) {
    dst[2 * points - i - 1].x = -dst[i].x;
    dst[2 * points - i - 1].y = dst[i].y;
  }
  PolygonRotateShift(dst, 2 * points, pos.x, pos.y, angle, false);
  canvas.polygon(dst, 2 * points);
}


static void
DrawDetailedAircraft(Canvas &canvas, const SETTINGS_MAP &settings_map,
                     const Angle angle,
                     const RasterPoint aircraft_pos)
{
  {
    static const RasterPoint Aircraft[] = {
      {0, -10},
      {-2, -7},
      {-2, -2},
      {-16, -2},
      {-32, -1},
      {-32, 2},
      {-1, 3},
      {-1, 15},
      {-3, 15},
      {-5, 17},
      {-5, 18},
      {0, 18},
    };
    const unsigned AIRCRAFT_POINTS = sizeof(Aircraft) / sizeof(Aircraft[0]);
    RasterPoint buffer[2 * AIRCRAFT_POINTS];

    if (settings_map.terrain.enable) {
      canvas.white_brush();
      canvas.select(Graphics::hpAircraft);
    } else {
      canvas.black_brush();
      canvas.white_pen();
    }

    DrawMirroredPolygon(Aircraft, buffer, AIRCRAFT_POINTS,
                        canvas, angle, aircraft_pos);
  }

  {
    static const RasterPoint Canopy[] = {
      {0, -7},
      {-1, -7},
      {-1, -2},
      {0, -1},
    };
    const unsigned CANOPY_POINTS = sizeof(Canopy) / sizeof(Canopy[0]);
    RasterPoint buffer[2 * CANOPY_POINTS];

    canvas.select(Graphics::hpCanopy);
    canvas.select(Graphics::hbCanopy);
    DrawMirroredPolygon(Canopy, buffer, CANOPY_POINTS,
                        canvas, angle, aircraft_pos);
  }
}


static void
DrawSimpleAircraft(Canvas &canvas, const Angle angle,
                   const RasterPoint aircraft_pos)
{
  static const RasterPoint Aircraft[] = {
    {1, -5},
    {1, 0},
    {14, 0},
    {14, 1},
    {1, 1},
    {1, 8},
    {4, 8},
    {4, 9},
    {-3, 9},
    {-3, 8},
    {0, 8},
    {0, 1},
    {-13, 1},
    {-13, 0},
    {0, 0},
    {0, -5},
  };
  const unsigned AIRCRAFT_POINTS = sizeof(Aircraft) / sizeof(Aircraft[0]);

  RasterPoint aircraft[AIRCRAFT_POINTS];
  std::copy(Aircraft, Aircraft + AIRCRAFT_POINTS, aircraft);
  PolygonRotateShift(aircraft, AIRCRAFT_POINTS,
                     aircraft_pos.x, aircraft_pos.y, angle, true);
  canvas.select(Graphics::hpAircraftSimple2);
  canvas.polygon(aircraft, AIRCRAFT_POINTS);
  canvas.black_brush();
  canvas.select(Graphics::hpAircraftSimple1);
  canvas.polygon(aircraft, AIRCRAFT_POINTS);
}


void
Graphics::DrawAircraft(Canvas &canvas, const SETTINGS_MAP &settings_map,
                       const Angle angle,
                       const RasterPoint aircraft_pos)
{
  if (Appearance.AircraftSymbol == acDetailed)
    DrawDetailedAircraft(canvas, settings_map, angle, aircraft_pos);
  else
    DrawSimpleAircraft(canvas, angle, aircraft_pos);
}
