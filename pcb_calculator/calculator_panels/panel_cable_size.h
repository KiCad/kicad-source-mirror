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

#ifndef PANEL_CABLE_SIZE_H
#define PANEL_CABLE_SIZE_H

#include "panel_cable_size_base.h"
#include <vector>

// Helper to convert values (in m2) to mm2, more useful to display these values
#define M2_to_MM2 1e6

class PCB_CALCULATOR_SETTINGS;

class CABLE_SIZE_ENTRY
{
public:
    CABLE_SIZE_ENTRY( const wxString& aName, double aRadius_meter );

    const wxString m_Name;
    const double   m_Radius; // stored in meters
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
    void OnConductorThermCoefChange( wxCommandEvent& aEvent ) override;
    void OnConductorThermCoefChange_Button( wxCommandEvent& aEvent ) override;
    void OnUpdateUnit( wxCommandEvent& aEvent ) override;
    void OnDiameterChange( wxCommandEvent& aEvent ) override;
    void OnAreaChange( wxCommandEvent& aEvent ) override;
    void OnLinResistanceChange( wxCommandEvent& aEvent ) override;
    void OnFrequencyChange( wxCommandEvent& aEvent ) override;
    void OnAmpacityChange( wxCommandEvent& aEvent ) override;
    void OnConductorTempChange( wxCommandEvent& aEvent ) override;
    void OnCurrentChange( wxCommandEvent& aEvent ) override;
    void OnLengthChange( wxCommandEvent& aEvent ) override;
    void OnResistanceDcChange( wxCommandEvent& aEvent ) override;
    void OnVDropChange( wxCommandEvent& aEvent ) override;
    void OnPowerChange( wxCommandEvent& aEvent ) override;
	void onUpdateCurrentDensity( wxScrollEvent& aEvent ) override;

private:
    void updateAll( double aRadius );
    void updateApplication();
    void printAll();
    void buildCableList();

    /// @return the area of the conductor in m2 by ampere, depending on m_amp_by_mm2 setting
    double m2_by_ampere()
    {
        return 1/m_amp_by_mm2/M2_to_MM2;
    }

private:
    std::vector<CABLE_SIZE_ENTRY> m_entries;

    bool m_updatingUI;
    bool m_updatingDiameter;
    bool m_updatingArea;
    bool m_updatingConductorMaterialResitivity;
    bool m_updatingLinResistance;
    bool m_updatingFrequency;
    bool m_updatingAmpacity;
    bool m_updatingCurrent;
    bool m_updatingLength;
    bool m_updatingResistanceDc;
    bool m_updatingRVdrop;
    bool m_updatingPower;

    bool m_imperial;

    // Stored in normalized units
    double m_conductorMaterialResitivity;
    double m_conductorMaterialResitivityRef;
    double m_conductorMaterialThermalCoef;
    double m_diameter;
    double m_conductorTemperature;
    double m_current;
    double m_length;
    double m_area;
    double m_linearResistance;
    double m_maxFrequency;
    double m_resistanceDc;
    double m_voltageDrop;
    double m_dissipatedPower;
    double m_ampacity;
    double m_amp_by_mm2;        // allowed current density in ampere by mm2
};

#endif
