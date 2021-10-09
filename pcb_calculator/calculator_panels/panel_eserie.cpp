/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 1992-2021 Kicad Developers, see AUTHORS.txt for contributors.
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

/* see
 * http://www.desmith.net/NMdS/Electronics/TraceWidth.html
 * http://www.ultracad.com/articles/pcbtemp.pdf
 * for more info
 */

#include <calculator_panels/panel_eserie.h>
#include <pcb_calculator_settings.h>
#include <string_utils.h>

#include <i18n_utility.h>   // For _HKI definition
wxString eseries_help =
#include "eserie_help.h"


PANEL_E_SERIE::PANEL_E_SERIE( wxWindow* parent, wxWindowID id,
                                const wxPoint& pos, const wxSize& size,
                                long style, const wxString& name ) :
        PANEL_E_SERIE_BASE( parent, id, pos, size, style, name )
{
    m_reqResUnits->SetLabel( wxT( "kΩ" ) );
    m_exclude1Units->SetLabel( wxT( "kΩ" ) );
    m_exclude2Units->SetLabel( wxT( "kΩ" ) );

    // show markdown formula explanation in lower help panel
    wxString msg;
    ConvertMarkdown2Html( wxGetTranslation( eseries_help ), msg );
    m_panelESeriesHelp->SetPage( msg );

    // Needed on wxWidgets 3.0 to ensure sizers are correctly set
    GetSizer()->SetSizeHints( this );
}


PANEL_E_SERIE::~PANEL_E_SERIE()
{
}


void PANEL_E_SERIE::ThemeChanged()
{
    // Update the HTML window with the help text
    m_panelESeriesHelp->ThemeChanged();
}


void PANEL_E_SERIE::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
}


void PANEL_E_SERIE::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
}
