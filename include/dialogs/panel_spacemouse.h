/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#pragma once

#include <dialogs/panel_spacemouse_base.h>

class COMMON_SETTINGS;

class PANEL_SPACEMOUSE : public PANEL_SPACEMOUSE_BASE
{
public:
    PANEL_SPACEMOUSE( wxWindow* aParent );
    ~PANEL_SPACEMOUSE() = default;

    void ResetPanel() override;

protected:
    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

private:
    void applySettingsToPanel( const COMMON_SETTINGS& aSettings );
};
