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

#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxWindow.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Protection.hpp"
#include "InfoBoxes/Content/Factory.hpp"
#include "InfoBoxes/Content/Base.hpp"
#include "Profile/InfoBoxConfig.hpp"
#include "InputEvents.hpp"
#include "Screen/Blank.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "Hardware/Battery.hpp"
#include "MainWindow.hpp"
#include "Appearance.hpp"
#include "Language/Language.hpp"
#include "DataField/Enum.hpp"
#include "DataField/ComboList.hpp"
#include "Dialogs/Dialogs.h"
#include "Dialogs/ComboPicker.hpp"
#include "Profile/InfoBoxConfig.hpp"
#include "Interface.hpp"
#include "resource.h"

#include <assert.h>
#include <stdio.h>

#include <algorithm>

namespace InfoBoxManager
{
  InfoBoxLayout::Layout layout;

  /** the window for displaying infoboxes full-screen */
  InfoBoxFullWindow full_window;

  /**
   * Is this the initial DisplayInfoBox() call?  If yes, then all
   * content objects need to be created.
   */
  static bool first;

  unsigned GetCurrentType(unsigned box);
  void SetCurrentType(unsigned box, char type);

  void DisplayInfoBox();
  void InfoBoxDrawIfDirty();
  int GetFocused();

  int GetInfoBoxBorder(unsigned i);
}

static bool InfoBoxesDirty = false;
static bool InfoBoxesHidden = false;

InfoBoxWindow *InfoBoxes[InfoBoxPanelConfig::MAX_INFOBOXES];

InfoBoxManagerConfig infoBoxManagerConfig;

static InfoBoxLook *info_box_look;

void
InfoBoxFullWindow::on_paint(Canvas &canvas)
{
  canvas.clear_white();

  for (unsigned i = 0; i < InfoBoxManager::layout.count; i++) {
    // JMW TODO: make these calculated once only.
    int x, y;
    int rx, ry;
    int rw;
    int rh;
    double fw, fh;

    if (Layout::landscape) {
      rw = 84;
      rh = 68;
    } else {
      rw = 120;
      rh = 80;
    }

    fw = rw / (double)InfoBoxManager::layout.control_width;
    fh = rh / (double)InfoBoxManager::layout.control_height;

    double f = std::min(fw, fh);
    rw = (int)(f * InfoBoxManager::layout.control_width);
    rh = (int)(f * InfoBoxManager::layout.control_height);

    if (Layout::landscape) {
      rx = i % 3;
      ry = i / 3;

      x = (rw + 4) * rx;
      y = (rh + 3) * ry;
    } else {
      rx = i % 2;
      ry = i / 4;

      x = (rw) * rx;
      y = (rh) * ry;
    }

    assert(InfoBoxes[i] != NULL);
    InfoBoxes[i]->PaintInto(canvas, IBLSCALE(x), IBLSCALE(y),
                            IBLSCALE(rw), IBLSCALE(rh));
  }
}

// TODO locking
void
InfoBoxManager::Hide()
{
  InfoBoxesHidden = true;

  for (unsigned i = 0; i < layout.count; i++)
    InfoBoxes[i]->fast_hide();

  full_window.hide();
}

void
InfoBoxManager::Show()
{
  InfoBoxesHidden = false;

  for (unsigned i = 0; i < layout.count; i++)
    InfoBoxes[i]->show();
}

int
InfoBoxManager::GetFocused()
{
  for (unsigned i = 0; i < layout.count; i++)
    if (InfoBoxes[i]->has_focus())
      return i;

  return -1;
}

void
InfoBoxManager::Event_Select(int i)
{
  int InfoFocus = GetFocused();

  if (InfoFocus < 0) {
    InfoFocus = (i >= 0 ? 0 : layout.count - 1);
  } else {
    InfoFocus += i;

    if (InfoFocus < 0 || (unsigned)InfoFocus >= layout.count)
      InfoFocus = -1;
  }

  if (InfoFocus >= 0)
    XCSoarInterface::main_window.map.set_focus();
  else
    InfoBoxes[i]->set_focus();
}

unsigned
InfoBoxManager::GetCurrentPanel()
{
  if (XCSoarInterface::SettingsMap().EnableAuxiliaryInfo) {
    unsigned panel = XCSoarInterface::SettingsMap().AuxiliaryInfoBoxPanel;
    if (panel >= InfoBoxManagerConfig::MAX_INFOBOX_PANELS)
      panel = PANEL_AUXILIARY;
    return panel;
  }
  else if (XCSoarInterface::main_window.map.GetDisplayMode() == dmCircling)
    return PANEL_CIRCLING;
  else if (XCSoarInterface::main_window.map.GetDisplayMode() == dmFinalGlide)
    return PANEL_FINAL_GLIDE;
  else
    return PANEL_CRUISE;
}

const TCHAR*
InfoBoxManager::GetPanelName(unsigned panelIdx)
{
  return gettext(infoBoxManagerConfig.panel[panelIdx].name);
}

const TCHAR*
InfoBoxManager::GetCurrentPanelName()
{
  return GetPanelName(GetCurrentPanel());
}

unsigned
InfoBoxManager::GetType(unsigned box, unsigned panelIdx)
{
  assert(box < InfoBoxPanelConfig::MAX_INFOBOXES);
  assert(panelIdx < InfoBoxManagerConfig::MAX_INFOBOX_PANELS);

  return infoBoxManagerConfig.panel[panelIdx].infoBoxID[box];
}

unsigned
InfoBoxManager::GetCurrentType(unsigned box)
{
  unsigned retval = GetType(box, GetCurrentPanel());
  return std::min(InfoBoxFactory::NUM_TYPES - 1, retval);
}

const TCHAR*
InfoBoxManager::GetTitle(unsigned box)
{
  if (InfoBoxes[box] != NULL)
    return InfoBoxes[box]->GetTitle();
  else
    return NULL;
}

bool
InfoBoxManager::IsEmpty(unsigned panelIdx)
{
  return infoBoxManagerConfig.panel[panelIdx].IsEmpty();
}

void
InfoBoxManager::SetType(unsigned i, unsigned type, unsigned panelIdx)
{
  assert(i < InfoBoxPanelConfig::MAX_INFOBOXES);
  assert(panelIdx < InfoBoxManagerConfig::MAX_INFOBOX_PANELS);

  if ((unsigned int) type != infoBoxManagerConfig.panel[panelIdx].infoBoxID[i]) {
    infoBoxManagerConfig.panel[panelIdx].infoBoxID[i] = type;
    infoBoxManagerConfig.panel[panelIdx].modified = true;
  }
}

void
InfoBoxManager::SetCurrentType(unsigned box, unsigned type)
{
  SetType(box, type, GetCurrentPanel());
}

void
InfoBoxManager::Event_Change(int i)
{
  int j = 0, k;

  int InfoFocus = GetFocused();
  if (InfoFocus < 0)
    return;

  k = GetCurrentType(InfoFocus);
  if (i > 0)
    j = InfoBoxFactory::GetNext(k);
  else if (i < 0)
    j = InfoBoxFactory::GetPrevious(k);

  // TODO code: if i==0, go to default or reset

  SetCurrentType(InfoFocus, (unsigned) j);

  InfoBoxes[InfoFocus]->UpdateContent();
  Paint();
}

void
InfoBoxManager::DisplayInfoBox()
{
  if (InfoBoxesHidden)
    return;

  static int DisplayTypeLast[InfoBoxPanelConfig::MAX_INFOBOXES];
  static bool last_invalid[InfoBoxPanelConfig::MAX_INFOBOXES];

  // JMW note: this is updated every GPS time step

  for (unsigned i = 0; i < layout.count; i++) {
    // All calculations are made in a separate thread. Slow calculations
    // should apply to the function DoCalculationsSlow()
    // Do not put calculations here!

    int DisplayType = GetCurrentType(i);

    bool needupdate = ((DisplayType != DisplayTypeLast[i]) || first);

    if (needupdate) {
      InfoBoxes[i]->SetTitle(gettext(InfoBoxFactory::GetCaption(DisplayType)));
      InfoBoxes[i]->SetContentProvider(InfoBoxFactory::Create(DisplayType));
      InfoBoxes[i]->SetID(i);
    }

    if (InfoBoxes[i]->UpdateContent()) {
      // check if need to repaint entire window due to validity
      bool invalid = InfoBoxes[i]->get_invalid();
      if (invalid != last_invalid[i]) {
        InfoBoxes[i]->invalidate();
        last_invalid[i] = invalid;
      }
    }

    DisplayTypeLast[i] = DisplayType;
  }

  Paint();

  first = false;
}

void
InfoBoxManager::ProcessKey(InfoBoxContent::InfoBoxKeyCodes keycode)
{
  int focus = GetFocused();
  if (focus < 0)
    return;

  if (InfoBoxes[focus] != NULL)
    InfoBoxes[focus]->HandleKey(keycode);

  InputEvents::HideMenu();

  SetDirty();
  // emulate update to trigger calculations
  if (!XCSoarInterface::Basic().LocationAvailable)
    TriggerGPSUpdate();

  ResetDisplayTimeOut();
}

InfoBoxContent::DialogContent*
InfoBoxManager::GetDialogContent(const int id)
{
  if (id < 0)
    return NULL;

  if (InfoBoxes[id] != NULL)
    return InfoBoxes[id]->GetDialogContent();

  return NULL;
}

void
InfoBoxManager::ProcessQuickAccess(const int id, const TCHAR *Value)
{
  if (id < 0)
    return;

  // do approciate action
  if (InfoBoxes[id] != NULL)
    InfoBoxes[id]->HandleQuickAccess(Value);

  SetDirty();

  // emulate update to trigger calculations
  if (!XCSoarInterface::Basic().LocationAvailable)
    TriggerGPSUpdate();

  ResetDisplayTimeOut();
}

bool
InfoBoxManager::HasFocus()
{
  return GetFocused() >= 0;
}

void
InfoBoxManager::InfoBoxDrawIfDirty()
{
  // No need to redraw map or infoboxes if screen is blanked.
  // This should save lots of battery power due to CPU usage
  // of drawing the screen

  if (InfoBoxesDirty && !XCSoarInterface::SettingsMap().ScreenBlanked) {
    DisplayInfoBox();
    InfoBoxesDirty = false;
  }
}

void
InfoBoxManager::SetDirty()
{
  InfoBoxesDirty = true;
}

void
InfoBoxManager::ProcessTimer()
{
  static Validity last;

  if (XCSoarInterface::Basic().Connected.Modified(last)) {
    SetDirty();
    last = XCSoarInterface::Basic().Connected;
  }

  InfoBoxDrawIfDirty();
}

void
InfoBoxManager::Paint()
{
  if (!InfoBoxLayout::fullscreen) {
    full_window.hide();
  } else {
    full_window.invalidate();
    full_window.show();
  }
}

int
InfoBoxManager::GetInfoBoxBorder(unsigned i)
{
  if ((Appearance.InfoBoxBorder == apIbTab) ||
      (Appearance.InfoBoxBorder == apIbShade))
    return 0;

  unsigned border = 0;

  switch (InfoBoxLayout::InfoBoxGeometry) {
  case InfoBoxLayout::ibTop4Bottom4:
    if (i < 4)
      border |= BORDERBOTTOM;
    else
      border |= BORDERTOP;

    if (i != 3 && i != 7)
      border |= BORDERRIGHT;
    break;

  case InfoBoxLayout::ibBottom8:
    border |= BORDERTOP;

    if (i != 3 && i != 7)
      border |= BORDERRIGHT;
    break;

  case InfoBoxLayout::ibBottom12:
    border |= BORDERTOP;

    if (i != 5 && i != 11)
      border |= BORDERRIGHT;
    break;

  case InfoBoxLayout::ibTop8:
    border |= BORDERBOTTOM;

    if (i != 3 && i != 7)
      border |= BORDERRIGHT;
    break;

  case InfoBoxLayout::ibTop12:
    border |= BORDERBOTTOM;

    if (i != 5 && i != 11)
      border |= BORDERRIGHT;
    break;

  case InfoBoxLayout::ibLeft4Right4:
    if (i != 3 && i != 7)
      border |= BORDERBOTTOM;

    if (i < 4)
      border |= BORDERRIGHT;
    else
      border |= BORDERLEFT;
    break;

  case InfoBoxLayout::ibGNav2:
    if ((i != 0) && (i != 6))
      border |= BORDERTOP;
    if (i < 6)
      border |= BORDERRIGHT;
    else
      border |= BORDERLEFT;
    break;

  case InfoBoxLayout::ibLeft8:
    if (i != 3 && i != 7)
      border |= BORDERBOTTOM;

    border |= BORDERRIGHT;
    break;

  case InfoBoxLayout::ibRight8:
    if (i != 3 && i != 7)
      border |= BORDERBOTTOM;

    border |= BORDERLEFT;
    break;

  case InfoBoxLayout::ibGNav:
    if (i != 0)
      border |= BORDERTOP;
    if (i < 6)
      border |= BORDERLEFT|BORDERRIGHT;
    break;

  case InfoBoxLayout::ibSquare:
    break;

  case InfoBoxLayout::ibRight12:
    if (i % 6 != 0)
      border |= BORDERTOP;
    border |= BORDERLEFT;
    break;

  case InfoBoxLayout::ibRight24:
    if (i % 8 != 0)
      border |= BORDERTOP;
    border |= BORDERLEFT;
    break;
  }

  return border;
}

static void
InfoBoxLookDefaults(InfoBoxLook &info_box_look)
{
  info_box_look.value.fg_color
    = info_box_look.comment.fg_color
    = Appearance.InverseInfoBox ? COLOR_WHITE : COLOR_BLACK;

  info_box_look.title.fg_color = Appearance.InverseInfoBox ?
    dialog_prefs.infobox_dark_text : dialog_prefs.infobox_light_text;

  info_box_look.background_brush.set(Appearance.InverseInfoBox
                                     ? COLOR_BLACK : COLOR_WHITE);
  info_box_look.shade_brush.set(Appearance.InverseInfoBox ?
                                dialog_prefs.infobox_dark_shade :
                                dialog_prefs.infobox_light_shade);

  if (Appearance.InverseInfoBox) {
    info_box_look.background_bitmap.load(Layout::ScaleEnabled()?
                                         IDB_INFOBOXI_HD : IDB_INFOBOXI);
  } else {
    info_box_look.background_bitmap.load(Layout::ScaleEnabled()?
                                         IDB_INFOBOX_HD : IDB_INFOBOX);
  }

  const Color border_color = dialog_prefs.infobox_neutral_shade;
  info_box_look.border_pen.set(InfoBoxWindow::BORDER_WIDTH, border_color);
  info_box_look.selector_pen.set(IBLSCALE(1) + 2,
                                 info_box_look.value.fg_color);

  info_box_look.value.font = &Fonts::InfoBox;
  info_box_look.title.font = &Fonts::Title;
  info_box_look.comment.font = &Fonts::Title;
  info_box_look.small_font = &Fonts::InfoBoxSmall;

  info_box_look.colors[0] = border_color;
  info_box_look.colors[1] = Appearance.InverseInfoBox
    ? Graphics::inv_redColor : COLOR_RED;
  info_box_look.colors[2] = Appearance.InverseInfoBox
    ? Graphics::inv_blueColor : COLOR_BLUE;
  info_box_look.colors[3] = Appearance.InverseInfoBox
    ? Graphics::inv_greenColor : COLOR_GREEN;
  info_box_look.colors[4] = Appearance.InverseInfoBox
    ? Graphics::inv_yellowColor : COLOR_YELLOW;
  info_box_look.colors[5] = Appearance.InverseInfoBox
    ? Graphics::inv_magentaColor : COLOR_MAGENTA;
}

void
InfoBoxManager::Create(PixelRect rc, const InfoBoxLayout::Layout &_layout)
{
  first = true;
  layout = _layout;

  info_box_look = new InfoBoxLook;
  InfoBoxLookDefaults(*info_box_look);

  WindowStyle style;
  style.hide();
  full_window.set(XCSoarInterface::main_window, rc.left, rc.top,
                  rc.right - rc.left, rc.bottom - rc.top, style);

  // create infobox windows
  for (unsigned i = 0; i < layout.count; i++) {
    const PixelRect &rc = layout.positions[i];
    int Border = GetInfoBoxBorder(i);

    InfoBoxes[i] = new InfoBoxWindow(XCSoarInterface::main_window,
                                     rc.left, rc.top,
                                     rc.right - rc.left, rc.bottom - rc.top,
                                     Border, *info_box_look);
  }

  SetDirty();
}

void
InfoBoxManager::Destroy()
{
  for (unsigned i = 0; i < layout.count; i++)
    delete (InfoBoxes[i]);

  full_window.reset();

  delete info_box_look;
}

static const ComboList *info_box_combo_list;

static void
OnInfoBoxHelp(unsigned item)
{
  int type = (*info_box_combo_list)[item].DataFieldIndex;

  TCHAR caption[100];
  _stprintf(caption, _T("%s: %s"), _("InfoBox"), gettext(InfoBoxFactory::GetName(type)));

  const TCHAR* text = InfoBoxFactory::GetDescription(type);
  if (text)
    dlgHelpShowModal(XCSoarInterface::main_window, caption, gettext(text));
  else
    dlgHelpShowModal(XCSoarInterface::main_window, caption,
                     _("No help available on this item"));
}

void
InfoBoxManager::ShowDlgInfoBox(const int id)
{
  if (GetDialogContent(id))
    dlgInfoBoxAccessShowModal(XCSoarInterface::main_window, id);
  else SetupFocused(id);
}

void
InfoBoxManager::SetupFocused(const int id)
{
  int i;

  if (id < 0) i = GetFocused();
  else i = id;

  if (i < 0)
    return;

  const unsigned panel = GetCurrentPanel();
  int old_type = GetType(i, panel);

  ComboList list;
  for (unsigned i = 0; i < InfoBoxFactory::NUM_TYPES; i++)
    list.Append(i, gettext(InfoBoxFactory::GetName(i)));

  list.Sort();
  list.ComboPopupItemSavedIndex = list.LookUp(old_type);

  /* let the user select */

  TCHAR caption[20];
  _stprintf(caption, _T("%s: %d"), _("InfoBox"), i + 1);
  info_box_combo_list = &list;
  int result = ComboPicker(XCSoarInterface::main_window, caption, list,
                           OnInfoBoxHelp);
  if (result < 0)
    return;

  /* was there a modification? */

  int new_type = list[result].DataFieldIndex;
  if (new_type == old_type)
    return;

  /* yes: apply and save it */

  SetType(i, new_type, panel);
  DisplayInfoBox();
  Profile::SetInfoBoxManagerConfig(infoBoxManagerConfig);
}
