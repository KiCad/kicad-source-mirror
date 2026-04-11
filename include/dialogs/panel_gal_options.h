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

#include <dialogs/panel_gal_options_base.h>
#include <gal/gal_display_options.h>

class wxBoxSizer;
class wxRadioBox;
class wxSpinCtrlDouble;
class wxChoice;
class wxCheckBox;
class wxStaticText;
class EDA_DRAW_FRAME;
class APP_SETTINGS_BASE;


class PANEL_GAL_OPTIONS: public PANEL_GAL_OPTIONS_BASE
{
public:

    PANEL_GAL_OPTIONS( wxWindow* aParent, APP_SETTINGS_BASE* aAppSettings );

    /**
     * Load the panel controls from the given opt
     */
    bool TransferDataToWindow() override;

    /**
     * Read the options set in the UI into the given options object
     */
    bool TransferDataFromWindow() override;

    bool ResetPanel( APP_SETTINGS_BASE* aAppSettings );

private:
    std::vector<double> m_gridThicknessList;    // List of available grid thickness
    APP_SETTINGS_BASE* m_cfg;
};

