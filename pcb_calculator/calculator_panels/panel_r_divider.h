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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PANEL_R_DIVIDER_H
#define PANEL_R_DIVIDER_H

#include <utility>
#include <vector>

#include <r_divider_solver.h>

#include "panel_r_divider_base.h"

class PCB_CALCULATOR_SETTINGS;


class PANEL_R_DIVIDER : public PANEL_R_DIVIDER_BASE
{
public:
    PANEL_R_DIVIDER( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL,
                     const wxString& name = wxEmptyString );
    ~PANEL_R_DIVIDER();

    // Methods from CALCULATOR_PANEL that must be overridden
    void LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void ThemeChanged() override;

    // Event handlers from PANEL_R_DIVIDER_BASE
    void OnModeChange( wxCommandEvent& aEvent ) override;
    void OnESeriesSelection( wxCommandEvent& aEvent ) override;
    void OnCalculate( wxCommandEvent& aEvent ) override;

private:
    enum COLUMN
    {
        COL_R1 = 0,
        COL_R2,
        COL_TARGET,
        COL_TARGET_ERROR,
        COL_IMPEDANCE,
        COL_IMPEDANCE_ERROR
    };

    /// The E-series radio buttons paired with their ESERIES enum values
    std::vector<std::pair<wxRadioButton*, int>> seriesButtons() const;

    bool isVoltageMode() const { return m_rbModeVoltage->GetValue(); }
    bool isParallelImpedance() const { return m_impedanceParallel->GetValue(); }

    /**
     * Enable the input fields of the selected mode, disable the others, and relabel the results columns.
     */
    void updateModeUI();

    void fillResults( const std::vector<R_DIVIDER_RESULT>& aResults, double aVin );

    void clearResults();

    /**
     * Work out the minimum width of each results column and fix the grid's minimum size from them.
     */
    void sizeColumns();

    /**
     * Widen the results columns so that they fill the grid.
     */
    void stretchColumns();

    int              m_series;       // ESERIES enum value
    std::vector<int> m_minColWidths; // Results column widths before stretching
};

#endif // PANEL_R_DIVIDER_H
