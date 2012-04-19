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

#include "InfoBoxes/Content/Other.hpp"
#include "InfoBoxes/InfoBoxWindow.hpp"
#include "Interface.hpp"
#include "Renderer/HorizonRenderer.hpp"
#include "Hardware/Battery.hpp"
#include "OS/SystemLoad.hpp"
#include "OS/MemInfo.hpp"
#include "Asset.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"
#include "Units/Units.hpp"
#include "Atmosphere/Temperature.hpp"

#ifdef HAVE_MEM_INFO
#include "Formatter/ByteSizeFormatter.hpp"
#endif

#include <tchar.h>
#include <stdio.h>

void
InfoBoxContentGLoad::Update(InfoBoxData &data)
{
  if (!XCSoarInterface::Basic().acceleration.available) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValue(_T("%2.2f"), XCSoarInterface::Basic().acceleration.g_load);
}

void
InfoBoxContentBattery::Update(InfoBoxData &data)
{
#ifdef HAVE_BATTERY
  bool DisplaySupplyVoltageAsValue=false;
  switch (Power::External::Status) {
    case Power::External::OFF:
      data.SetComment(_("AC Off"));
      break;
    case Power::External::ON:
      if (!XCSoarInterface::Basic().voltage_available)
        data.SetComment(_("AC ON"));
      else{
        DisplaySupplyVoltageAsValue = true;
        data.SetValue(_T("%2.1fV"),
                          XCSoarInterface::Basic().voltage);
      }
      break;
    case Power::External::UNKNOWN:
    default:
      data.SetCommentInvalid();
  }
#ifndef ANDROID
  switch (Power::Battery::Status){
    case Power::Battery::HIGH:
    case Power::Battery::LOW:
    case Power::Battery::CRITICAL:
    case Power::Battery::CHARGING:
      if (Power::Battery::RemainingPercentValid){
#endif
        if (!DisplaySupplyVoltageAsValue)
          data.UnsafeFormatValue(_T("%d%%"), Power::Battery::RemainingPercent);
        else
          data.UnsafeFormatComment(_T("%d%%"), Power::Battery::RemainingPercent);
#ifndef ANDROID
      }
      else
        if (!DisplaySupplyVoltageAsValue)
          data.SetValueInvalid();
        else
          data.SetCommentInvalid();
      break;
    case Power::Battery::NOBATTERY:
    case Power::Battery::UNKNOWN:
      if (!DisplaySupplyVoltageAsValue)
        data.SetValueInvalid();
      else
        data.SetCommentInvalid();
  }
#endif
  return;

#endif

  if (XCSoarInterface::Basic().voltage_available) {
    data.SetValue(_T("%2.1fV"), XCSoarInterface::Basic().voltage);
    return;
  } else if (XCSoarInterface::Basic().battery_level_available) {
    data.SetValue(_T("%.0f%%"), XCSoarInterface::Basic().battery_level);
    return;
  }

  data.SetInvalid();
}

void
InfoBoxContentBatteryTemperature::Update(InfoBoxData &data)
{
#ifndef HAVE_BATTERY
    data.SetInvalid();
    return;
#endif

  // Set Value
  data.SetValue(_T("%2.1f"),
                    Units::ToUserTemperature(CelsiusToKelvin(fixed(Power::Battery::Temperature))));

  data.SetValueUnit(Units::current.temperature_unit);
}




void
InfoBoxContentExperimental1::Update(InfoBoxData &data)
{
  // Set Value
  data.SetInvalid();
}

void
InfoBoxContentExperimental2::Update(InfoBoxData &data)
{
  // Set Value
  data.SetInvalid();
}

void
InfoBoxContentCPULoad::Update(InfoBoxData &data)
{
  unsigned percent_load = SystemLoadCPU();
  if (percent_load <= 100) {
    data.UnsafeFormatValue(_T("%d%%"), percent_load);
  } else {
    data.SetInvalid();
  }
}

void
InfoBoxContentFreeRAM::Update(InfoBoxData &data)
{
#ifdef HAVE_MEM_INFO
  FormatByteSize(data.value.buffer(), data.value.MAX_SIZE, SystemFreeRAM(), true);
#else
  data.SetInvalid();
#endif
}

void
InfoBoxContentHorizon::OnCustomPaint(InfoBoxWindow &infobox, Canvas &canvas)
{
#ifndef NO_HORIZON
  if (CommonInterface::Basic().acceleration.available) {
    const Look &look = UIGlobals::GetLook();
    HorizonRenderer::Draw(canvas, infobox.GetValueAndCommentRect(),
                          look.horizon, CommonInterface::Basic().attitude);
  }
#endif
}

void
InfoBoxContentHorizon::Update(InfoBoxData &data)
{
#ifdef NO_HORIZON
  data.SetInvalid();
  data.SetComment(_("Disabled"));
#else
  if (!CommonInterface::Basic().attitude.bank_angle_available &&
      !CommonInterface::Basic().attitude.pitch_angle_available) {
    data.SetInvalid();
    return;
  }

  data.SetCustom();
#endif
}
