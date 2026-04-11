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

#include "panel_gerbview_display_options_base.h"


class PANEL_GAL_OPTIONS;


class PANEL_GERBVIEW_DISPLAY_OPTIONS : public PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE
{
public:
    PANEL_GERBVIEW_DISPLAY_OPTIONS( wxWindow* aParent );
    ~PANEL_GERBVIEW_DISPLAY_OPTIONS() {};

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ResetPanel() override;

private:
    void loadSettings( GERBVIEW_SETTINGS* aCfg );

private:
    PANEL_GAL_OPTIONS* m_galOptsPanel;
};
