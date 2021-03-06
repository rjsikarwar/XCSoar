/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Form/DataField/Enum.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Form/List.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Dialogs/Device/DeviceEditWidget.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Profile/DeviceConfig.hpp"
#include "Device/Register.hpp"
#include "Device/Driver.hpp"
#include "Asset.hpp"
#include "UtilsSettings.hpp"
#include "DevicesConfigPanel.hpp"
#include "Language/Language.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Util/StaticString.hpp"
#include "Util/Macros.hpp"

class DevicesConfigPanel final
  : public ListWidget, private DeviceEditWidget::Listener {
  TwoWidgets &container;
  DeviceEditWidget &edit;

  unsigned current_device;
  bool current_modified;

  gcc_pure
  DeviceEditWidget &GetEditWidget() {
    return edit;
  }

  gcc_pure
  const DeviceEditWidget &GetEditWidget() const {
    return edit;
  }

  bool SaveDeviceConfig();

public:
  DevicesConfigPanel(TwoWidgets &_container, DeviceEditWidget &_edit)
    :container(_container), edit(_edit),
     current_device(0), current_modified(false) {
    edit.SetListener(this);
  }

  ~DevicesConfigPanel() {
    if (IsDefined())
      DeleteWindow();
  }

  const DeviceConfig &GetDeviceConfig(unsigned i) const {
    assert(i < NUMDEV);

    return CommonInterface::GetSystemSettings().devices[i];
  }

  const DeviceConfig &GetListItemConfig(unsigned i) const {
    assert(i < NUMDEV);

    return i == current_device
      ? GetEditWidget().GetConfig()
      : GetDeviceConfig(i);
  }

  void SetDeviceConfig(unsigned i, const DeviceConfig &config) const {
    assert(i < NUMDEV);

    CommonInterface::SetSystemSettings().devices[i] = config;
    Profile::SetDeviceConfig(i, config);
    DevicePortChanged = true;
  }

  void ShowDevice(unsigned idx);

  virtual void Initialise(ContainerWindow &parent,
                          const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;

private:
  /* virtual methods from List::Handler */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) override;
  virtual void OnCursorMoved(unsigned index) override;
  virtual bool CanActivateItem(unsigned index) const override;
  virtual void OnActivateItem(unsigned index) override;

  /* virtual methods from DeviceEditWidget::Listener */
  virtual void OnModified(DeviceEditWidget &widget) override;
};

bool
DevicesConfigPanel::SaveDeviceConfig()
{
  bool changed = current_modified;
  DeviceEditWidget &widget = GetEditWidget();
  if (!widget.Save(changed))
    return false;

  if (changed)
    SetDeviceConfig(current_device, widget.GetConfig());

  current_modified = false;
  return true;
}

void
DevicesConfigPanel::ShowDevice(unsigned idx)
{
  assert(idx < NUMDEV);

  if (idx == current_device)
    return;

  if (!SaveDeviceConfig())
    return;

  current_device = idx;
  current_modified = false;
  GetEditWidget().SetConfig(GetDeviceConfig(current_device));
}

bool
DevicesConfigPanel::CanActivateItem(unsigned index) const
{
  return true;
}

void
DevicesConfigPanel::OnActivateItem(unsigned idx)
{
  ShowDevice(idx);
  GetEditWidget().SetFocus();
}

void
DevicesConfigPanel::OnCursorMoved(unsigned idx)
{
  ShowDevice(idx);
  container.UpdateLayout();
}

void
DevicesConfigPanel::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                unsigned idx)
{
  const DeviceConfig &config = GetListItemConfig(idx);

  const UPixelScalar margin = Layout::GetTextPadding();

  TCHAR port_name_buffer[128];
  const TCHAR *port_name =
    config.GetPortName(port_name_buffer, ARRAY_SIZE(port_name_buffer));

  StaticString<256> text(_T("A: "));
  text[0u] += idx;

  if (config.UsesDriver()) {
    const TCHAR *driver_name = FindDriverDisplayName(config.driver_name);

    text.AppendFormat(_("%s on %s"), driver_name, port_name);
  } else {
    text.append(port_name);
  }

  canvas.DrawText(rc.left + margin, rc.top + margin, text);
}

void
DevicesConfigPanel::Initialise(ContainerWindow &parent, const PixelRect &rc)
{
  CreateList(parent, UIGlobals::GetDialogLook(),
             rc, Layout::Scale(18)).SetLength(NUMDEV);
}

bool
DevicesConfigPanel::Save(bool &_changed)
{
  bool changed = false;

  if (!SaveDeviceConfig())
    return false;

  if (DevicePortChanged)
    changed = true;

  _changed |= changed;

  return true;
}

void
DevicesConfigPanel::OnModified(DeviceEditWidget &widget)
{
  bool changed = false;
  if (GetEditWidget().Save(changed) && changed) {
    current_modified = true;
    GetList().Invalidate();
  }

  container.UpdateLayout();
}

class DevicesConfigPanel2 : public TwoWidgets {
public:
  DevicesConfigPanel2() {
    const DeviceConfig &config =
      CommonInterface::GetSystemSettings().devices[0];
    DeviceEditWidget *edit = new DeviceEditWidget(config);
    DevicesConfigPanel *list = new DevicesConfigPanel(*this, *edit);
    TwoWidgets::Set(list, edit);
  }
};

Widget *
CreateDevicesConfigPanel()
{
  return new DevicesConfigPanel2();
}
