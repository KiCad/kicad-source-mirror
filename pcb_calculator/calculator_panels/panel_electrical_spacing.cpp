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

/* see
 * http://www.desmith.net/NMdS/Electronics/TraceWidth.html
 * http://www.ultracad.com/articles/pcbtemp.pdf
 * for more info
 */

#include <calculator_panels/panel_electrical_spacing.h>
#include <pcb_calculator_settings.h>
#include <string_utils.h>


PANEL_ELECTRICAL_SPACING::PANEL_ELECTRICAL_SPACING( wxWindow* parent, wxWindowID id,
                                const wxPoint& pos, const wxSize& size,
                                long style, const wxString& name ) :
        PANEL_ELECTRICAL_SPACING_BASE( parent, id, pos, size, style, name )
{
}


PANEL_ELECTRICAL_SPACING::~PANEL_ELECTRICAL_SPACING()
{
}


void PANEL_ELECTRICAL_SPACING::ThemeChanged()
{
    static_cast<CALCULATOR_PANEL*>( m_notebook1->GetPage( 0 ) )->ThemeChanged();
    static_cast<CALCULATOR_PANEL*>( m_notebook1->GetPage( 1 ) )->ThemeChanged();
}


void PANEL_ELECTRICAL_SPACING::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    static_cast<CALCULATOR_PANEL*>( m_notebook1->GetPage( 0 ) )->SaveSettings( aCfg );
    static_cast<CALCULATOR_PANEL*>( m_notebook1->GetPage( 1 ) )->SaveSettings( aCfg );
}


void PANEL_ELECTRICAL_SPACING::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    static_cast<CALCULATOR_PANEL*>( m_notebook1->GetPage( 0 ) )->LoadSettings( aCfg );
    static_cast<CALCULATOR_PANEL*>( m_notebook1->GetPage( 1 ) )->LoadSettings( aCfg );
}