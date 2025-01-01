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

#ifndef KICAD_PANEL_SIMULATOR_PREFERENCES_H
#define KICAD_PANEL_SIMULATOR_PREFERENCES_H

#include "panel_simulator_preferences_base.h"
#include <sim/sim_preferences.h>


class PANEL_SIMULATOR_PREFERENCES : public PANEL_SIMULATOR_PREFERENCES_BASE
{
public:
    PANEL_SIMULATOR_PREFERENCES( wxWindow* aParent );
    ~PANEL_SIMULATOR_PREFERENCES();
    void ResetPanel() override;

protected:
    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;
    void onMouseDefaults( wxCommandEvent& ) override;
    void onTrackpadDefaults( wxCommandEvent& ) override;

private:
    static SIM_MOUSE_WHEEL_ACTION horizontalScrollSelectionToAction( int aSelection );

    static int actionToHorizontalScrollSelection( SIM_MOUSE_WHEEL_ACTION anAction );

    void applyMouseScrollActionsToPanel( const SIM_MOUSE_WHEEL_ACTION_SET& anActionSet );
};


#endif
