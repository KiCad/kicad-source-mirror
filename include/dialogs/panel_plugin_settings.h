/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KICAD_PANEL_PLUGIN_SETTINGS_H
#define KICAD_PANEL_PLUGIN_SETTINGS_H

#include <dialogs/panel_plugin_settings_base.h>

class PAGED_DIALOG;


class PANEL_PLUGIN_SETTINGS : public PANEL_PLUGIN_SETTINGS_BASE
{
public:
    PANEL_PLUGIN_SETTINGS( wxWindow* aParent );

    void ResetPanel() override;

protected:
    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

    void OnPythonInterpreterChanged( wxFileDirPickerEvent& event ) override;
    void OnBtnDetectAutomaticallyClicked( wxCommandEvent& aEvent ) override;
    void OnEnableApiChecked( wxCommandEvent& aEvent ) override;

private:
    void updateApiStatusText();
    void validatePythonInterpreter();

    bool m_pythonInterpreterValid;
};

#endif //KICAD_PANEL_PLUGIN_SETTINGS_H
