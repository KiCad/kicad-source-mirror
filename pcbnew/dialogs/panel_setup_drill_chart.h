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

#ifndef PANEL_SETUP_DRILL_CHART_H
#define PANEL_SETUP_DRILL_CHART_H

#include <dialogs/panel_setup_drill_chart_base.h>
#include <widgets/unit_binder.h>

class BOARD;
class BOARD_DESIGN_SETTINGS;
class PCB_EDIT_FRAME;


/**
 * Board-wide drill chart and map setup.
 * */
class PANEL_SETUP_DRILL_CHART : public PANEL_SETUP_DRILL_CHART_BASE
{
public:
    PANEL_SETUP_DRILL_CHART( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame );
    ~PANEL_SETUP_DRILL_CHART() override = default;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ImportSettingsFrom( BOARD* aBoard );

private:
    void onEditGroups( wxCommandEvent& aEvent ) override;

    void loadSettings( const BOARD_DESIGN_SETTINGS& aSettings );

    PCB_EDIT_FRAME* m_frame;

    UNIT_BINDER m_symbolSize;
    UNIT_BINDER m_symbolWidth;
};

#endif // PANEL_SETUP_DRILL_CHART_H
