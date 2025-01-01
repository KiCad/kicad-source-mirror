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

#ifndef PANEL_CORROSION_H
#define PANEL_CORROSION_H

#include "panel_galvanic_corrosion_base.h"
#include <vector>

class PCB_CALCULATOR_SETTINGS;

class CORROSION_TABLE_ENTRY
{
public:
    CORROSION_TABLE_ENTRY( const wxString& aName, const wxString& aSymbol, double aPotential );
    /** @brief Translatable name ( Copper ) */
    wxString m_name;
    /** @brief Chemical symbol (Cu), not translatable */
    wxString m_symbol;
    /** @brief potential in volts, relative to copper */
    double m_potential;
};

class PANEL_GALVANIC_CORROSION : public PANEL_GALVANIC_CORROSION_BASE
{
public:
    PANEL_GALVANIC_CORROSION( wxWindow* parent, wxWindowID id = wxID_ANY,
                              const wxPoint& pos = wxDefaultPosition,
                              const wxSize& size = wxDefaultSize,
                              long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
    ~PANEL_GALVANIC_CORROSION();

    std::vector<CORROSION_TABLE_ENTRY> m_entries;
    // Methods from CALCULATOR_PANEL that must be overriden
    void LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void ThemeChanged() override;
    void OnNomenclatureChange( wxCommandEvent& aEvent ) override;
    void OnCorFilterChange( wxCommandEvent& aEvent ) override;

private:
    void fillTable();

private:
    bool   m_symbolicStatus;
    double m_corFilterValue;
};

#endif
