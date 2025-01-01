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

#ifndef PANEL_BOARD_CLASS_H
#define PANEL_BOARD_CLASS_H

#include "panel_board_class_base.h"

class PCB_CALCULATOR_SETTINGS;


class PANEL_BOARD_CLASS : public PANEL_BOARD_CLASS_BASE
{
public:
    PANEL_BOARD_CLASS( wxWindow* parent, wxWindowID id = wxID_ANY,
                       const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                       long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
    ~PANEL_BOARD_CLASS();

    // Methods from CALCULATOR_PANEL that must be overridden
    void LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void ThemeChanged() override;

    void OnBoardClassesUnitsSelection( wxCommandEvent& event ) override;
    void BoardClassesUpdateData( double aUnitScale );
};

#endif
