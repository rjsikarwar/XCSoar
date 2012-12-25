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

#include "Fonts.hpp"
#include "StandardFonts.hpp"
#include "Screen/Font.hpp"
#include "Screen/Layout.hpp"

/// values inside infoboxes  like numbers, etc.
Font Fonts::infobox;
Font Fonts::infobox_small;
#ifndef GNAV
Font Fonts::infobox_units;
#endif
/// Titles of infoboxes like Next, WP L/D etc.
Font Fonts::title;
/// vario display, runway informations
Font Fonts::cdi;
Font Fonts::monospace;
/// text names on the map
Font Fonts::map;
/// menu buttons, waypoint selection, messages, etc.
Font Fonts::map_bold;
/// Flarm Traffic draweing and stats, map labels in italic
Font Fonts::map_label;
/// font labels for important labels (e.g. big/medium cities)
Font Fonts::map_label_important;

// these are the non-custom parameters
LOGFONT log_infobox;
#ifndef GNAV
LOGFONT log_infobox_units;
#endif
LOGFONT log_title;
LOGFONT log_map;
LOGFONT log_infobox_small;
LOGFONT log_map_bold;
LOGFONT log_cdi;
LOGFONT log_map_label;
LOGFONT log_map_label_important;
static LOGFONT log_monospace;

static void
InitialiseLogfont(LOGFONT* font, const TCHAR* facename, UPixelScalar height,
                  bool bold = false, bool italic = false,
                  bool variable_pitch = true)
{
  memset((char *)font, 0, sizeof(LOGFONT));

  _tcscpy(font->lfFaceName, facename);

  font->lfPitchAndFamily = (variable_pitch ? VARIABLE_PITCH : FIXED_PITCH)
                          | FF_DONTCARE;

  font->lfHeight = (long)height;
  font->lfWeight = (long)(bold ? FW_BOLD : FW_MEDIUM);
  font->lfItalic = italic;

#ifdef WIN32
  if (IsAltair())
    font->lfQuality = NONANTIALIASED_QUALITY;
  else
    font->lfQuality = ANTIALIASED_QUALITY;
#endif
}

static void
LoadAltairLogFonts()
{
  InitialiseLogfont(&log_infobox, _T("RasterGothicTwentyFourCond"), 24, true);
  InitialiseLogfont(&log_title, _T("RasterGothicNineCond"), 10);
  InitialiseLogfont(&log_cdi, _T("RasterGothicEighteenCond"), 19, true);
  InitialiseLogfont(&log_map_label, _T("RasterGothicTwelveCond"), 13);
  InitialiseLogfont(&log_map_label_important,
                    _T("RasterGothicTwelveCond"), 13);
  InitialiseLogfont(&log_map, _T("RasterGothicFourteenCond"), 15);
  InitialiseLogfont(&log_map_bold, _T("RasterGothicFourteenCond"), 15, true);
  InitialiseLogfont(&log_infobox_small, _T("RasterGothicEighteenCond"), 19, true);
  InitialiseLogfont(&log_monospace, GetStandardMonospaceFontFace(),
                    10, false, false, false);
}

static void
SizeLogFont(LOGFONT &logfont, UPixelScalar width, const TCHAR* str)
{
  // JMW algorithm to auto-size info window font.
  // this is still required in case title font property doesn't exist.
  PixelSize tsize;
  do {
    --logfont.lfHeight;

    Font font;
    if (!font.Load(logfont))
      break;

    tsize = font.TextSize(str);
  } while ((unsigned)tsize.cx > width);

  ++logfont.lfHeight;
}

static void
InitialiseLogFonts()
{
  if (IsAltair()) {
    LoadAltairLogFonts();
    return;
  }

#ifndef USE_GDI
  UPixelScalar FontHeight = Layout::SmallScale(IsAndroid() ? 30 : 24);
#else
  UPixelScalar FontHeight = Layout::SmallScale(35);
#endif

  // oversize first so can then scale down
  InitialiseLogfont(&log_infobox, GetStandardFontFace(),
                    (int)(FontHeight * 1.4), true, false, true);

#ifdef WIN32
  log_infobox.lfCharSet = ANSI_CHARSET;
#endif

  InitialiseLogfont(&log_title, GetStandardFontFace(),
                    FontHeight / 3, true);

  // new font for CDI Scale
  InitialiseLogfont(&log_cdi, GetStandardFontFace(),
                    UPixelScalar(FontHeight * 0.6), false, false, false);

  // new font for map labels
  InitialiseLogfont(&log_map_label, GetStandardFontFace(),
                    UPixelScalar(FontHeight * 0.39), false, true);

  // new font for map labels big/medium cities
  InitialiseLogfont(&log_map_label_important, GetStandardFontFace(),
                    UPixelScalar(FontHeight * 0.39), false, true);

  // new font for map labels
  InitialiseLogfont(&log_map, GetStandardFontFace(),
                    UPixelScalar(FontHeight * 0.507));

  // Font for map bold text
  InitialiseLogfont(&log_map_bold, GetStandardFontFace(),
                    UPixelScalar(FontHeight * 0.507), true);

  InitialiseLogfont(&log_infobox_small, GetStandardFontFace(),
                    Layout::Scale(20));

  InitialiseLogfont(&log_infobox_small, GetStandardFontFace(),
                    (int)(FontHeight * 0.56), true);

  InitialiseLogfont(&log_monospace, GetStandardMonospaceFontFace(),
                    UPixelScalar(FontHeight * 0.39), false, false, false);
}

bool
Fonts::Initialize()
{
  InitialiseLogFonts();

  title.Load(log_title);
  cdi.Load(log_cdi);
  map_label.Load(log_map_label);
  map_label_important.Load(log_map_label_important);
  map.Load(log_map);
  map_bold.Load(log_map_bold);
  monospace.Load(log_monospace);

  return title.IsDefined() && cdi.IsDefined() &&
    map_label.IsDefined() && map_label_important.IsDefined() &&
    map.IsDefined() && map_bold.IsDefined() &&
    monospace.IsDefined();
}

void
Fonts::SizeInfoboxFont(UPixelScalar control_width)
{
  LOGFONT lf = log_infobox;

  if (!IsAltair())
    SizeLogFont(lf, control_width, _T("1234m"));
  infobox.Load(lf);

#ifndef GNAV
  unsigned height = lf.lfHeight;
  lf = log_infobox_units;
  lf.lfHeight = (height * 2) / 5;
  infobox_units.Load(lf);
#endif

  lf = log_infobox_small;
  if (!IsAltair())
    SizeLogFont(lf, control_width, _T("12345m"));
  infobox_small.Load(lf);
}

void
Fonts::Deinitialize()
{
  infobox.Destroy();
  infobox_small.Destroy();
#ifndef GNAV
  infobox_units.Destroy();
#endif
  title.Destroy();
  map.Destroy();
  map_bold.Destroy();
  cdi.Destroy();
  map_label.Destroy();
  map_label_important.Destroy();
  monospace.Destroy();
}
