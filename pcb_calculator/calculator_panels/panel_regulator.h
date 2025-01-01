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

#ifndef PANEL_REGULATOR_H
#define PANEL_REGULATOR_H
#include "panel_regulator_base.h"
#include "class_regulator_data.h"

class  PCB_CALCULATOR_SETTINGS;

class PANEL_REGULATOR : public PANEL_REGULATOR_BASE
{
public:
    PANEL_REGULATOR( wxWindow* parent, wxWindowID id = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                     long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
    ~PANEL_REGULATOR();


public:
    void OnRegulatorCalcButtonClick( wxCommandEvent& event ) override;
    void OnRegulatorResetButtonClick( wxCommandEvent& event ) override;
    void OnRegulTypeSelection( wxCommandEvent& event ) override;
    void OnRegulatorSelection( wxCommandEvent& event ) override;
    void OnDataFileSelection( wxCommandEvent& event ) override;
    void OnAddRegulator( wxCommandEvent& event ) override;
    void OnEditRegulator( wxCommandEvent& event ) override;
    void OnRemoveRegulator( wxCommandEvent& event ) override;
    void OnCopyCB( wxCommandEvent& event ) override;

    // Methods from CALCULATOR_PANEL that must be overridden
    void LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void ThemeChanged() override;

    /**
     * Update the regulator page dialog display.
     *
     * Enable the current regulator drawings and the formula used for calculations.
     */
    void RegulatorPageUpdate();

    /**
     * If m_lastSelectedRegulatorName is empty, just calls RegulatorPageUpdate()
     */
    void SelectLastSelectedRegulator();

    void RegulatorsSolve();

    /**
     * @return the full filename of the selected pcb_calculator data file
     */
    const wxString GetDataFilename();

    /**
     * Initialize the full filename of the selected pcb_calculator data file
     * force the standard extension of the file (.pcbcalc).
     *
     * @param aFilename is the full filename, with or without extension.
     */
    void SetDataFilename( const wxString& aFilename );

    // R/W data files:
    bool ReadDataFile();
    bool WriteDataFile();

    static double round_to( double value, double precision = 0.001 );

public:
    REGULATOR_LIST  m_RegulatorList;        // the list of known regulators
    wxString        m_lastSelectedRegulatorName;
    bool            m_RegulatorListChanged; // Set when m_RegulatorList is modified
                                            // and the corresponding file must be rewritten

};

#endif
