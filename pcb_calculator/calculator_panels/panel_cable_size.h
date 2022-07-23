/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2022 Kicad Developers, see AUTHORS.txt for contributors.
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

#ifndef PANEL_CABLE_SIZE_H
#define PANEL_CABLE_SIZE_H

#include "panel_cable_size_base.h"
#include <vector>

class PCB_CALCULATOR_SETTINGS;

class CABLE_SIZE_ENTRY
{
public:
    CABLE_SIZE_ENTRY( wxString aName, double aRadius_meter );

    wxString m_Name;
    double   m_Radius;  // stored in meters
};

class PANEL_CABLE_SIZE : public PANEL_CABLE_SIZE_BASE
{
public:
    PANEL_CABLE_SIZE( wxWindow* parent, wxWindowID id = wxID_ANY,
                      const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                      long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
    ~PANEL_CABLE_SIZE();

    // Methods from CALCULATOR_PANEL that must be overridden
    void LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void ThemeChanged() override{};

    void OnCableSizeChange( wxCommandEvent& aEvent ) override;
    void OnConductorResistivityChange( wxCommandEvent& aEvent ) override;
    void OnConductorResistivity_Button( wxCommandEvent& aEvent ) override;
    void OnUpdateUnit( wxCommandEvent& aEvent ) override;
    void OnDiameterChange( wxCommandEvent& aEvent ) override;
    void OnAreaChange( wxCommandEvent& aEvent ) override;
    void OnLinResistanceChange( wxCommandEvent& aEvent ) override;
    void OnFrequencyChange( wxCommandEvent& aEvent ) override;
    void OnAmpacityChange( wxCommandEvent& aEvent ) override;
    void OnCurrentChange( wxCommandEvent& aEvent ) override;
    void OnLengthChange( wxCommandEvent& aEvent ) override;
    void OnResistanceChange( wxCommandEvent& aEvent ) override;
    void OnVDropChange( wxCommandEvent& aEvent ) override;
    void OnPowerChange( wxCommandEvent& aEvent ) override;

private:
    void updateAll( double aRadius );
    void updateApplication();
    void printAll();

private:
    std::vector<CABLE_SIZE_ENTRY> m_entries;

    bool m_updatingUI;
    bool m_updatingDiameter;
    bool m_updatingArea;
    bool m_updatingLinResistance;
    bool m_updatingFrequency;
    bool m_updatingAmpacity;
    bool m_updatingCurrent;
    bool m_updatingLength;
    bool m_updatingResistance;
    bool m_updatingRVdrop;
    bool m_updatingPower;

    bool m_imperial;

    // Stored in normalized units
    double m_button_ResistivityConductor;
    double m_conductorMaterialResitivity;
    double m_diameter;
    double m_current;
    double m_length;
    double m_area;
    double m_linearResistance;
    double m_maxFrequency;
    double m_resistance;
    double m_voltageDrop;
    double m_dissipatedPower;
    double m_ampacity;
};

#endif
