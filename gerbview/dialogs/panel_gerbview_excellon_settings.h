/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "panel_gerbview_excellon_settings_base.h"


struct EXCELLON_DEFAULTS;


class PANEL_GERBVIEW_EXCELLON_SETTINGS : public PANEL_GERBVIEW_EXCELLON_SETTINGS_BASE
{
public:
    PANEL_GERBVIEW_EXCELLON_SETTINGS( wxWindow* aParent );
    ~PANEL_GERBVIEW_EXCELLON_SETTINGS() {};

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
    void ResetPanel() override;

    void applySettingsToPanel( const EXCELLON_DEFAULTS& aSettings );
};

