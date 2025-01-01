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

#ifndef PANEL_VIA_SIZE_H
#define PANEL_VIA_SIZE_H

#include "panel_via_size_base.h"

class PCB_CALCULATOR_SETTINGS;


class PANEL_VIA_SIZE : public PANEL_VIA_SIZE_BASE
{
public:
    PANEL_VIA_SIZE( wxWindow* parent, wxWindowID id = wxID_ANY,
                    const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                    long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
    ~PANEL_VIA_SIZE();

    // Methods from CALCULATOR_PANEL that must be overridden
    void LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void ThemeChanged() override;

    /**
     * Called when the user changes any value in the via calculator.
     */
    void OnViaCalculate( wxCommandEvent& event ) override;

    /**
     * Shows a list of current relative dielectric constant(Er) and select a value
     */
    void OnViaEpsilonR_Button( wxCommandEvent& event ) override;

    /**
     * Shows a list of current Specific resistance list (rho) and select a value
     */
    void OnViaRho_Button( wxCommandEvent& event ) override;

    /**
     * Update the Error message in via calculation panel.
     */
    void onUpdateViaCalcErrorText( wxUpdateUIEvent& event ) override;

    /**
     * Called when the user clicks the reset button; sets the parameters to their default values.
     */
    void OnViaResetButtonClick( wxCommandEvent& event ) override;

    /**
     * Display the results of the calculation.
     */
    void VSDisplayValues( double aViaResistance, double aVoltageDrop, double aPowerLoss,
                          double aEstimatedAmpacity, double aThermalResistance,
                          double aCapacitance, double aTimeDegradation, double aInductance,
                          double aReactance );

private:
};

#endif
