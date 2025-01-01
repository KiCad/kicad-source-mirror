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

#ifndef PANEL_WAVELENGTH_H
#define PANEL_WAVELENGTH_H

#include "panel_wavelength_base.h"

class PCB_CALCULATOR_SETTINGS;

class PANEL_WAVELENGTH : public PANEL_WAVELENGTH_BASE
{
public:
    PANEL_WAVELENGTH( wxWindow* parent, wxWindowID id = wxID_ANY,
                      const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                      long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
    ~PANEL_WAVELENGTH(){};

    // Methods from CALCULATOR_PANEL that must be overriden
    void LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void ThemeChanged() override{};

    void OnFrequencyChange( wxCommandEvent& event ) override;
    void OnPeriodChange( wxCommandEvent& event ) override;
    void OnWavelengthVacuumChange( wxCommandEvent& event ) override;
    void OnWavelengthMediumChange( wxCommandEvent& event ) override;
    void OnPermittivityChange( wxCommandEvent& event ) override;
    void OnPermeabilityChange( wxCommandEvent& event ) override;
    void OnButtonPermittivity( wxCommandEvent& event ) override;

private:
    void update( double aFrequency );
    void updateUnits( wxCommandEvent& aEvent ) override;

    double m_permittivity = 1;
    double m_permeability = 1;
    double m_frequency = 1;

    bool m_updatingFrequency = false;
    bool m_updatingPeriod = false;
    bool m_updatingWavelengthVacuum = false;
    bool m_updatingWavelengthMedium = false;
    bool m_updatingSpeed = false;

    bool m_updatingUI = false;
};

#endif
