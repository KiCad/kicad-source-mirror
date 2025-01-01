/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PANEL_R_CALCULATOR_H
#define PANEL_R_CALCULATOR_H

#include <eseries.h>
#include "panel_r_calculator_base.h"
#include "resistor_substitution_utils.h"

class PCB_CALCULATOR_SETTINGS;


class PANEL_R_CALCULATOR : public PANEL_R_CALCULATOR_BASE
{
public:
    PANEL_R_CALCULATOR( wxWindow* parent, wxWindowID id = wxID_ANY,
                        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                        long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
    ~PANEL_R_CALCULATOR();

    // Methods from CALCULATOR_PANEL that must be overridden
    void LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void ThemeChanged() override;

    /**
     * Called on calculate button and executes all E-series calculations
     */
    void OnCalculateESeries( wxCommandEvent& event ) override;

    /**
     * Radio Buttons to select the E-series for the resistor calculator.
     *
     * @param event contains the radio button state.
     */
    void OnESeriesSelection( wxCommandEvent& event ) override;

private:
    RES_EQUIV_CALC m_eSeries;
};

#endif
