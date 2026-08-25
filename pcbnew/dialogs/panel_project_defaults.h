/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <panel_project_defaults_base.h>

class PCBNEW_SETTINGS;
class APP_SETTINGS_BASE;


class PANEL_PROJECT_DEFAULTS : public PANEL_PROJECT_DEFAULTS_BASE
{
public:
    PANEL_PROJECT_DEFAULTS( wxWindow* aParent, APP_SETTINGS_BASE* aAppSettings );
    ~PANEL_PROJECT_DEFAULTS() override;

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

    void ResetPanel() override;

private:
    void loadPCBSettings( PCBNEW_SETTINGS* aCfg );

private:
    bool m_isPCBEdit;
};
