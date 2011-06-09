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

#include "GlueMapWindow.hpp"

#include "Screen/Icon.hpp"
#include "Language/Language.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "Logger/Logger.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Appearance.hpp"
#include "Screen/TextInBox.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/UnitSymbol.hpp"
#include "Terrain/RasterWeather.hpp"
#include "Units/UnitsFormatter.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "ThermalBandRenderer.hpp"
#include <stdio.h>

void
GlueMapWindow::DrawCrossHairs(Canvas &canvas) const
{
  Pen dash_pen(Pen::DASH, 1, COLOR_DARK_GRAY);
  canvas.select(dash_pen);

  const RasterPoint Orig_Screen = render_projection.GetScreenOrigin();

  canvas.line(Orig_Screen.x + 20, Orig_Screen.y,
              Orig_Screen.x - 20, Orig_Screen.y);
  canvas.line(Orig_Screen.x, Orig_Screen.y + 20,
              Orig_Screen.x, Orig_Screen.y - 20);
}

void
GlueMapWindow::DrawGPSStatus(Canvas &canvas, const PixelRect &rc,
                             const NMEA_INFO &info) const
{
  const TCHAR *txt;
  MaskedIcon *icon = NULL;

  if (!info.Connected) {
    icon = &Graphics::hGPSStatus2;
    txt = _("GPS not connected");
  } else if (!info.LocationAvailable) {
    icon = &Graphics::hGPSStatus1;
    txt = _("GPS waiting for fix");
  } else {
    return; // early exit
  }

  int x = rc.left + Layout::FastScale(2);
  int y = rc.bottom - Layout::FastScale(35);
  icon->draw(canvas, x, y);

  x += icon->get_size().cx + Layout::FastScale(4);
  y = rc.bottom - Layout::FastScale(34);

  TextInBoxMode_t mode;
  mode.Mode = RoundedBlack;
  mode.Bold = true;

  TextInBox(canvas, txt, x, y, mode, rc, NULL);
}

void
GlueMapWindow::DrawFlightMode(Canvas &canvas, const PixelRect &rc) const
{
  int offset = 0;
  int Height;
  TCHAR WarningText[80];
  WarningText[0] = '\0';
  PixelSize TextSize = canvas.text_size(WarningText);
  // draw logger status
  if (logger != NULL && logger->isLoggerActive()) {
    bool flip = (Basic().DateTime.second % 2) == 0;
    MaskedIcon &icon = flip ? Graphics::hLogger : Graphics::hLoggerOff;
    offset = icon.get_size().cx;
    icon.draw(canvas, rc.right - offset, rc.bottom - icon.get_size().cy);
  }

  // draw flight mode
  MaskedIcon *bmp;

  if (task != NULL && (task->get_mode() == TaskManager::MODE_ABORT))
    bmp = &Graphics::hAbort;
  else if (GetDisplayMode() == dmCircling)
    bmp = &Graphics::hClimb;
  else if (GetDisplayMode() == dmFinalGlide)
    bmp = &Graphics::hFinalGlide;
  else
    bmp = &Graphics::hCruise;

  offset += bmp->get_size().cx + Layout::Scale(6);

  bmp->draw(canvas, rc.right - offset,
            rc.bottom - bmp->get_size().cy - Layout::Scale(4));

  // draw flarm status
  if (SettingsMap().EnableFLARMGauge)
    // Don't show indicator when the gauge is indicating the traffic anyway
    return;

  const FLARM_STATE &flarm = Basic().flarm;
  if (!flarm.available || (flarm.GetActiveTrafficCount()==0))
    return;
  _tcscat(WarningText, _T(" FLARM "));

  canvas.select(Fonts::Title);
  canvas.background_opaque();

  Height = Fonts::Title.get_capital_height() + IBLSCALE(2);
  int y = rc.bottom - Height;

  TextSize = canvas.text_size(WarningText);
  y-= Layout::Scale(5);

  offset += TextSize.cx + Layout::Scale(5);
  canvas.fill_rectangle(rc.right - offset - Layout::Scale(1), y - Layout::Scale(1), rc.right - offset + TextSize.cx + Layout::Scale(1), y + TextSize.cy + Layout::Scale(1), COLOR_BLACK);

  switch (flarm.alarm_level) {
  case 0:
    canvas.set_background_color(COLOR_WHITE);
    canvas.set_text_color(COLOR_BLACK);
    break;
  case 1:
    canvas.set_background_color(COLOR_YELLOW);
    canvas.set_text_color(COLOR_BLACK);
    break;
  case 2:
  case 3:
  default:
    canvas.set_background_color(COLOR_RED);
    canvas.set_text_color(COLOR_WHITE);
    break;
  };

  canvas.text(rc.right - offset, y, WarningText);
}

void
GlueMapWindow::DrawFinalGlide(Canvas &canvas, const PixelRect &rc) const
{
  RasterPoint GlideBar[6] = {
      { 0, 0 }, { 9, -9 }, { 18, 0 }, { 18, 0 }, { 9, 0 }, { 0, 0 }
  };
  RasterPoint GlideBar0[6] = {
      { 0, 0 }, { 9, -9 }, { 18, 0 }, { 18, 0 }, { 9, 0 }, { 0, 0 }
  };

  TCHAR Value[10];

  int Offset;
  int Offset0;
  int i;

  if (Calculated().task_stats.task_valid &&
      Calculated().task_stats.total.solution_remaining.defined() &&
      Calculated().task_stats.total.solution_mc0.defined()) {
    const int y0 = ((rc.bottom - rc.top) / 2) + rc.top;

    // 60 units is size, div by 8 means 60*8 = 480 meters.
    Offset = ((int)Calculated().task_stats.total.solution_remaining.AltitudeDifference) / 8;

    // JMW OLD_TASK this is broken now
    Offset0 = ((int)Calculated().task_stats.total.solution_mc0.AltitudeDifference) / 8;
    // TODO feature: should be an angle if in final glide mode

    if (Offset > 60)
      Offset = 60;
    if (Offset < -60)
      Offset = -60;

    Offset = IBLSCALE(Offset);
    if (Offset < 0)
      GlideBar[1].y = IBLSCALE(9);

    if (Offset0 > 60)
      Offset0 = 60;
    if (Offset0 < -60)
      Offset0 = -60;

    Offset0 = IBLSCALE(Offset0);
    if (Offset0 < 0)
      GlideBar0[1].y = IBLSCALE(9);

    for (i = 0; i < 6; i++) {
      GlideBar[i].y += y0;
      GlideBar[i].x = IBLSCALE(GlideBar[i].x) + rc.left;
    }

    GlideBar[0].y -= Offset;
    GlideBar[1].y -= Offset;
    GlideBar[2].y -= Offset;

    for (i = 0; i < 6; i++) {
      GlideBar0[i].y += y0;
      GlideBar0[i].x = IBLSCALE(GlideBar0[i].x) + rc.left;
    }

    GlideBar0[0].y -= Offset0;
    GlideBar0[1].y -= Offset0;
    GlideBar0[2].y -= Offset0;

    if ((Offset < 0) && (Offset0 < 0)) {
      // both below
      if (Offset0 != Offset) {
        int dy = (GlideBar0[0].y - GlideBar[0].y) +
            (GlideBar0[0].y - GlideBar0[3].y);
        dy = max(IBLSCALE(3), dy);
        GlideBar[3].y = GlideBar0[0].y - dy;
        GlideBar[4].y = GlideBar0[1].y - dy;
        GlideBar[5].y = GlideBar0[2].y - dy;

        GlideBar0[0].y = GlideBar[3].y;
        GlideBar0[1].y = GlideBar[4].y;
        GlideBar0[2].y = GlideBar[5].y;
      } else {
        Offset0 = 0;
      }
    } else if ((Offset > 0) && (Offset0 > 0)) {
      // both above
      GlideBar0[3].y = GlideBar[0].y;
      GlideBar0[4].y = GlideBar[1].y;
      GlideBar0[5].y = GlideBar[2].y;

      if (abs(Offset0 - Offset) < IBLSCALE(4))
        Offset = Offset0;
    }

    // draw actual glide bar
    if (Offset <= 0) {
      if (Calculated().common_stats.landable_reachable) {
        canvas.select(Graphics::hpFinalGlideBelowLandable);
        canvas.select(Graphics::hbFinalGlideBelowLandable);
      } else {
        canvas.select(Graphics::hpFinalGlideBelow);
        canvas.select(Graphics::hbFinalGlideBelow);
      }
    } else {
      canvas.select(Graphics::hpFinalGlideAbove);
      canvas.select(Graphics::hbFinalGlideAbove);
    }
    canvas.polygon(GlideBar, 6);

    // draw glide bar at mc 0
    if (Offset0 <= 0) {
      if (Calculated().common_stats.landable_reachable) {
        canvas.select(Graphics::hpFinalGlideBelowLandable);
        canvas.hollow_brush();
      } else {
        canvas.select(Graphics::hpFinalGlideBelow);
        canvas.hollow_brush();
      }
    } else {
      canvas.select(Graphics::hpFinalGlideAbove);
      canvas.hollow_brush();
    }

    if (Offset != Offset0)
      canvas.polygon(GlideBar0, 6);

    // draw cross (x) on final glide bar if unreachable at current Mc
    // or above final glide but impeded by obstacle
    int cross_sign = 0;

    if (!Calculated().task_stats.total.achievable())
      cross_sign = 1;
    if (Calculated().TerrainWarning && (Offset>0))
      cross_sign = -1;

    if (cross_sign != 0) {
      canvas.select(Graphics::hpBearing);
      canvas.line(Layout::Scale(9 - 5), y0 + cross_sign * Layout::Scale(9 - 5),
                  Layout::Scale(9 + 5), y0 + cross_sign * Layout::Scale(9 + 5));
      canvas.line(Layout::Scale(9 - 5), y0 + cross_sign * Layout::Scale(9 + 5),
                  Layout::Scale(9 + 5), y0 + cross_sign * Layout::Scale(9 - 5));
    }

    Units::FormatUserAltitude(Calculated().task_stats.total.solution_remaining.AltitudeDifference,
                              Value, sizeof(Value) / sizeof(Value[0]),
                              false);

    if (Offset >= 0)
      Offset = GlideBar[2].y + Offset + IBLSCALE(5);
    else if (Offset0 > 0)
      Offset = GlideBar0[1].y - IBLSCALE(15);
    else
      Offset = GlideBar[2].y + Offset - IBLSCALE(15);

    canvas.set_text_color(COLOR_BLACK);
    canvas.set_background_color(COLOR_WHITE);

    TextInBoxMode_t TextInBoxMode;
    TextInBoxMode.Mode = RoundedBlack;
    TextInBoxMode.Bold = true;
    TextInBoxMode.MoveInView = true;
    TextInBox(canvas, Value, 0, (int)Offset, TextInBoxMode, rc);
  }
}

void
GlueMapWindow::DrawMapScale(Canvas &canvas, const PixelRect &rc,
                            const MapWindowProjection &projection) const
{
  fixed MapWidth;
  TCHAR ScaleInfo[80];

  int Height;
  Units_t Unit;

  MapWidth = projection.GetScreenWidthMeters();

  canvas.select(Fonts::MapBold);
  Units::FormatUserMapScale(&Unit, MapWidth, ScaleInfo,
                            sizeof(ScaleInfo) / sizeof(TCHAR), false);
  PixelSize TextSize = canvas.text_size(ScaleInfo);

  Height = Fonts::MapBold.get_capital_height() + IBLSCALE(2);
  // 2: add 1pix border

  canvas.fill_rectangle(IBLSCALE(4), rc.bottom - Height,
                        TextSize.cx + IBLSCALE(16), rc.bottom,
                        COLOR_WHITE);

  canvas.background_transparent();
  canvas.set_text_color(COLOR_BLACK);

  canvas.text(IBLSCALE(7),
              rc.bottom - Fonts::MapBold.get_ascent_height() - IBLSCALE(1),
              ScaleInfo);

  Graphics::hBmpMapScaleLeft.draw(canvas, 0, rc.bottom - Height);
  Graphics::hBmpMapScaleRight.draw(canvas, IBLSCALE(14) + TextSize.cx,
                                   rc.bottom - Height);

  const UnitSymbol *symbol = GetUnitSymbol(Unit);
  if (symbol != NULL)
    symbol->draw(canvas, IBLSCALE(8) + TextSize.cx, rc.bottom - Height);

  ScaleInfo[0] = '\0';
  if (SettingsMap().AutoZoom)
    _tcscat(ScaleInfo, _T("AUTO "));

  if (SettingsMap().TargetPan)
    _tcscat(ScaleInfo, _T("TARGET "));
  else if (SettingsMap().EnablePan)
    _tcscat(ScaleInfo, _T("PAN "));

  if (SettingsMap().EnableAuxiliaryInfo) {
    _tcscat(ScaleInfo, InfoBoxManager::GetCurrentPanelName());
    _tcscat(ScaleInfo, _T(" "));
  }

  if (Basic().gps.Replay)
    _tcscat(ScaleInfo, _T("REPLAY "));
  else if (Basic().gps.Simulator) {
    _tcscat(ScaleInfo, _("Simulator"));
    _tcscat(ScaleInfo, _T(" "));
  }

  if (SettingsComputer().BallastTimerActive) {
    TCHAR TEMP[20];
    _stprintf(TEMP, _T("BALLAST %d LITERS "),
              (int)SettingsComputer().glide_polar_task.get_ballast_litres());
    _tcscat(ScaleInfo, TEMP);
  }

  if (weather != NULL && weather->GetParameter() > 0) {
    const TCHAR *label = weather->ItemLabel(weather->GetParameter());
    if (label != NULL)
      _tcscat(ScaleInfo, label);
  }

  if (ScaleInfo[0]) {
    int y = rc.bottom - Height;

    canvas.select(Fonts::Title);
    canvas.background_opaque();
    canvas.set_background_color(COLOR_WHITE);

    TextSize = canvas.text_size(ScaleInfo);
    y-= TextSize.cy;
    canvas.text(0, y, ScaleInfo);
  }
}

void
GlueMapWindow::DrawThermalEstimate(Canvas &canvas) const
{
  if (GetDisplayMode() == dmCircling) {
    // in circling mode, draw thermal at actual estimated location
    const MapWindowProjection &projection = render_projection;
    const THERMAL_LOCATOR_INFO &thermal_locator = Calculated().thermal_locator;
    if (thermal_locator.estimate_valid) {
      RasterPoint sc;
      if (projection.GeoToScreenIfVisible(thermal_locator.estimate_location, sc)) {
        Graphics::hBmpThermalSource.draw(canvas, sc);
      }
    }
  } else {
    MapWindow::DrawThermalEstimate(canvas);
  }
}

void
GlueMapWindow::RenderTrail(Canvas &canvas, const RasterPoint aircraft_pos) const
{
  unsigned min_time = 0;
  if (GetDisplayMode() == dmCircling) {
    min_time = max(0, (int)Basic().Time - 600);
  } else {
    switch(SettingsMap().TrailActive) {
    case 1:
      min_time = max(0, (int)Basic().Time - 3600);
      break;
    case 2:
      min_time = max(0, (int)Basic().Time - 600);
      break;
    case 3:
      min_time = 0; // full
      break;
    }
  }

  DrawTrail(canvas, aircraft_pos, min_time,
            SettingsMap().EnableTrailDrift && GetDisplayMode() == dmCircling);
}

void
GlueMapWindow::DrawThermalBand(Canvas &canvas, const PixelRect &rc) const
{
  if (Calculated().task_stats.total.solution_remaining.defined() &&
      Calculated().task_stats.total.solution_remaining.AltitudeDifference > fixed(50)
      && GetDisplayMode() == dmFinalGlide)
    return;

  PixelRect tb_rect;
  tb_rect.left = rc.left;
  tb_rect.right = rc.left+IBLSCALE(20);
  tb_rect.top = IBLSCALE(4);
  tb_rect.bottom = tb_rect.top+(rc.bottom-rc.top)/2-IBLSCALE(30);

  ThermalBandRenderer renderer(Graphics::thermal_band, Graphics::chart);
  if (task != NULL) {
    ProtectedTaskManager::Lease task_manager(*task);
    renderer.DrawThermalBand(Basic(),
                             Calculated(),
                             SettingsComputer(),
                             canvas,
                             tb_rect,
                             SettingsComputer(),
                             true,
                             &task_manager->get_ordered_task_behaviour());
  } else {
    renderer.DrawThermalBand(Basic(),
                             Calculated(),
                             SettingsComputer(),
                             canvas,
                             tb_rect,
                             SettingsComputer(),
                             true);
  }
}

void
GlueMapWindow::DrawStallRatio(Canvas &canvas, const PixelRect &rc) const
{
  if (Basic().StallRatioAvailable) {
    // JMW experimental, display stall sensor
    fixed s = max(fixed_zero, min(fixed_one, Basic().StallRatio));
    long m = (long)((rc.bottom - rc.top) * s * s);

    canvas.black_pen();
    canvas.line(rc.right - 1, rc.bottom - m, rc.right - 11, rc.bottom - m);
  }
}
