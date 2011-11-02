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
#include "SettingsMap.hpp"
#include "resource.h"
#include "Asset.hpp"
#include "LogFile.hpp"

Pen Graphics::hpSnail[NUMSNAILCOLORS];
Pen Graphics::hpSnailVario[NUMSNAILCOLORS];
Brush Graphics::hpSnailVarioNegative[NUMSNAILCOLORS];
int Graphics::hSnailVarioNegative[NUMSNAILCOLORS];

#ifdef HAVE_HATCHED_BRUSH
Bitmap Graphics::hAboveTerrainBitmap;
Brush Graphics::hAboveTerrainBrush;
#endif

MaskedIcon Graphics::hTerrainWarning;
MaskedIcon Graphics::hLogger, Graphics::hLoggerOff;
MaskedIcon Graphics::hCruise, Graphics::hClimb,
           Graphics::hFinalGlide, Graphics::hAbort;
MaskedIcon Graphics::hGPSStatus1, Graphics::hGPSStatus2;
MaskedIcon Graphics::hBmpTrafficSafe, Graphics::hBmpTrafficWarning, Graphics::hBmpTrafficAlarm;

Pen Graphics::hpWind;
Pen Graphics::hpWindTail;
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

Brush Graphics::hbCompass;
Brush Graphics::hbFinalGlideBelow;
Brush Graphics::hbFinalGlideBelowLandable;
Brush Graphics::hbFinalGlideAbove;
Brush Graphics::hbWind;

MaskedIcon Graphics::hBmpThermalSource;

MaskedIcon Graphics::hBmpMapScaleLeft;
MaskedIcon Graphics::hBmpMapScaleRight;

Bitmap Graphics::hBmpTabTask;
Bitmap Graphics::hBmpTabWrench;
Bitmap Graphics::hBmpTabSettings;
Bitmap Graphics::hBmpTabCalculator;

Bitmap Graphics::hBmpTabFlight;
Bitmap Graphics::hBmpTabSystem;
Bitmap Graphics::hBmpTabRules;
Bitmap Graphics::hBmpTabTimes;

// used for landable rendering
Brush Graphics::hbGreen;
Brush Graphics::hbWhite;
Brush Graphics::hbOrange;
Brush Graphics::hbLightGray;
Brush Graphics::hbNotReachableTerrain;
Brush Graphics::hbGround;

static Color clrSepia(COLOR_WHITE);
const Color Graphics::GroundColor = COLOR_WHITE;
const Color Graphics::skyColor = COLOR_WHITE;
const Color Graphics::seaColor = COLOR_WHITE; // ICAO open water area

void
Graphics::Initialise()
{
  /// @todo enhancement: support red/green color blind pilots with adjusted colour scheme

  LogStartUp(_T("Initialise graphics"));

  LoadUnitSymbols();

  hTerrainWarning.Load(IDB_TERRAINWARNING, IDB_TERRAINWARNING_HD);
  hGPSStatus1.Load(IDB_GPSSTATUS1, IDB_GPSSTATUS1_HD, false);
  hGPSStatus2.Load(IDB_GPSSTATUS2, IDB_GPSSTATUS2_HD, false);
  hLogger.Load(IDB_LOGGER, IDB_LOGGER_HD);
  hLoggerOff.Load(IDB_LOGGEROFF, IDB_LOGGEROFF_HD);

  hCruise.Load(IDB_CRUISE, IDB_CRUISE_HD, false);
  hClimb.Load(IDB_CLIMB, IDB_CLIMB_HD, false);
  hFinalGlide.Load(IDB_FINALGLIDE, IDB_FINALGLIDE_HD, false);
  hAbort.Load(IDB_ABORT, IDB_ABORT_HD, false);

#ifdef HAVE_HATCHED_BRUSH
  hAboveTerrainBitmap.load(IDB_ABOVETERRAIN);

  hAboveTerrainBrush.Set(hAboveTerrainBitmap);
#endif

  hpWind.Set(Layout::Scale(1), DarkColor(COLOR_GRAY));
  hpWindTail.Set(Pen::DASH, 1, COLOR_BLACK);
  hbWind.Set(COLOR_GRAY);

  hBmpMapScaleLeft.Load(IDB_MAPSCALE_LEFT, IDB_MAPSCALE_LEFT_HD, false);
  hBmpMapScaleRight.Load(IDB_MAPSCALE_RIGHT, IDB_MAPSCALE_RIGHT_HD, false);

  hBmpTabTask.load((Layout::scale > 1) ? IDB_TASK_HD : IDB_TASK);
  hBmpTabWrench.load((Layout::scale > 1) ? IDB_WRENCH_HD : IDB_WRENCH);
  hBmpTabSettings.load((Layout::scale > 1) ? IDB_SETTINGS_HD : IDB_SETTINGS);
  hBmpTabCalculator.load((Layout::scale > 1) ? IDB_CALCULATOR_HD : IDB_CALCULATOR);

  hBmpTabFlight.load((Layout::scale > 1) ? IDB_GLOBE_HD : IDB_GLOBE);
  hBmpTabSystem.load((Layout::scale > 1) ? IDB_DEVICE_HD : IDB_DEVICE);
  hBmpTabRules.load((Layout::scale > 1) ? IDB_RULES_HD : IDB_RULES);
  hBmpTabTimes.load((Layout::scale > 1) ? IDB_CLOCK_HD : IDB_CLOCK);

  hBmpThermalSource.Load(IDB_THERMALSOURCE, IDB_THERMALSOURCE_HD);

  hBmpTrafficSafe.Load(IDB_TRAFFIC_SAFE, IDB_TRAFFIC_SAFE_HD, false);
  hBmpTrafficWarning.Load(IDB_TRAFFIC_WARNING, IDB_TRAFFIC_WARNING_HD, false);
  hBmpTrafficAlarm.Load(IDB_TRAFFIC_ALARM, IDB_TRAFFIC_ALARM_HD, false);

  hbCompass.Set(Color(207, 207, 207));

  hbFinalGlideBelow.Set(COLOR_RED);
  hpFinalGlideBelow.Set(Layout::Scale(2), COLOR_WHITE);

  hbFinalGlideBelowLandable.Set(COLOR_ORANGE);
  hpFinalGlideBelowLandable.Set(Layout::Scale(2), COLOR_WHITE);

  hbFinalGlideAbove.Set(COLOR_GREEN);
  hpFinalGlideAbove.Set(Layout::Scale(2), COLOR_WHITE);

  hpCompass.Set(Layout::Scale(2), COLOR_WHITE);

  hpMapScale.Set(Layout::Scale(1), COLOR_BLACK);

  hpTerrainLine.Set(Pen::DASH, Layout::Scale(2), clrSepia);
  hpTerrainLineThick.Set(Pen::DASH, Layout::Scale(4), clrSepia);

  TracePen.Set(2, Color(50, 243, 45));
  ContestPen[0].Set(Layout::Scale(1)+2, COLOR_RED);
  ContestPen[1].Set(Layout::Scale(1)+1, COLOR_ORANGE);
  ContestPen[2].Set(Layout::Scale(1), COLOR_BLUE);

    // used for landable rendering
  hbGreen.Set(COLOR_GREEN);
  hbWhite.Set(COLOR_WHITE);
  hbOrange.Set(COLOR_ORANGE);
  hbLightGray.Set(COLOR_LIGHT_GRAY);
  hbNotReachableTerrain.Set(LightColor(COLOR_RED));

  hbGround.Set(GroundColor);

  hpTrackBearingLine.Set(3, COLOR_GRAY);
}

void
Graphics::InitialiseConfigured(const SETTINGS_MAP &settings_map)
{
  InitSnailTrail(settings_map);
}

void
Graphics::InitSnailTrail(const SETTINGS_MAP &settings_map)
{
  static gcc_constexpr_data ColorRamp snail_colors_vario[] = {
    {0,   0xc4, 0x80, 0x1e}, // sinkColor
    {100, 0xa0, 0xa0, 0xa0},
    {200, 0x1e, 0xf1, 0x73} // liftColor
  };

  static gcc_constexpr_data ColorRamp snail_colors_vario2[] = {
    {0,   0x00, 0x00, 0xff},
    {99,  0x00, 0xff, 0xff},
    {100, 0xff, 0xff, 0x00},
    {200, 0xff, 0x00, 0x00}
  };

  static gcc_constexpr_data ColorRamp snail_colors_alt[] = {
    {0,   0xff, 0x00, 0x00},
    {50,  0xff, 0xff, 0x00},
    {100, 0x00, 0xff, 0x00},
    {150, 0x00, 0xff, 0xff},
    {200, 0x00, 0x00, 0xff},
  };

  PixelScalar iwidth;
  PixelScalar minwidth = Layout::Scale(2);

  for (int i = 0; i < NUMSNAILCOLORS; i++) {
    short ih = i * 200 / (NUMSNAILCOLORS - 1);
    Color color = (settings_map.snail_type == stAltitude) ?
                  ColorRampLookup(ih, snail_colors_alt, 5) :
                  (settings_map.snail_type == stSeeYouVario) ?
                  ColorRampLookup(ih, snail_colors_vario2, 4) :
                  ColorRampLookup(ih, snail_colors_vario, 3);

    if (i < NUMSNAILCOLORS / 2 ||
        !settings_map.snail_scaling_enabled)
      iwidth = minwidth;
    else
      iwidth = max(minwidth,
                   PixelScalar((i - NUMSNAILCOLORS / 2) *
                               Layout::Scale(16) / NUMSNAILCOLORS));

    hpSnail[i].Set(minwidth, color);
    hpSnailVario[i].Set(iwidth, color);
    hpSnailVarioNegative[i].Set(color);
    hSnailVarioNegative[i]= NUMSNAILCOLORS - i; //max(minwidth, (i - NUMSNAILCOLORS / 2) *
                             // Layout::Scale(32) / NUMSNAILCOLORS);
  }
}

void
Graphics::Deinitialise()
{
  DeinitialiseUnitSymbols();

  hTerrainWarning.Reset();
  hGPSStatus1.Reset();
  hGPSStatus2.Reset();
  hLogger.Reset();
  hLoggerOff.Reset();

  hCruise.Reset();
  hClimb.Reset();
  hFinalGlide.Reset();
  hAbort.Reset();

#ifdef HAVE_HATCHED_BRUSH
  hAboveTerrainBrush.Reset();
  hAboveTerrainBitmap.reset();
#endif

  hbWind.Reset();

  hBmpMapScaleLeft.Reset();
  hBmpMapScaleRight.Reset();

  hBmpTabTask.reset();
  hBmpTabWrench.reset();
  hBmpTabSettings.reset();
  hBmpTabCalculator.reset();

  hBmpTabFlight.reset();
  hBmpTabSystem.reset();
  hBmpTabRules.reset();
  hBmpTabTimes.reset();

  hBmpThermalSource.Reset();

  hBmpTrafficSafe.Reset();
  hBmpTrafficWarning.Reset();
  hBmpTrafficAlarm.Reset();

  hbCompass.Reset();

  hbFinalGlideBelow.Reset();
  hbFinalGlideBelowLandable.Reset();
  hbFinalGlideAbove.Reset();

  hpWind.Reset();
  hpWindTail.Reset();

  hpCompass.Reset();

  hpFinalGlideBelow.Reset();
  hpFinalGlideBelowLandable.Reset();
  hpFinalGlideAbove.Reset();

  hpMapScale.Reset();
  hpTerrainLine.Reset();
  hpTerrainLineThick.Reset();

  TracePen.Reset();
  ContestPen[0].Reset();
  ContestPen[1].Reset();
  ContestPen[2].Reset();

  hbGreen.Reset();
  hbWhite.Reset();
  hbOrange.Reset();
  hbLightGray.Reset();
  hbNotReachableTerrain.Reset();

  hbGround.Reset();

  hpTrackBearingLine.Reset();

  for (unsigned i = 0; i < NUMSNAILCOLORS; i++) {
    hpSnail[i].Reset();
    hpSnailVario[i].Reset();
    hpSnailVarioNegative[i].Reset();
    hSnailVarioNegative[i] = 0;
  }
}
