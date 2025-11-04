/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <widgets/button_row_panel.h>
#include <widgets/ui_common.h>

#include <wx/button.h>
#include <wx/sizer.h>


BUTTON_ROW_PANEL::BUTTON_ROW_PANEL( wxWindow* aWindow, const BTN_DEF_LIST& aLeftBtns,
                                    const BTN_DEF_LIST& aRightBtns ) :
        wxPanel( aWindow, wxID_ANY )
{
    m_sizer = new wxBoxSizer( wxHORIZONTAL );

    addButtons( true, aLeftBtns );

    // add the spacer
    m_sizer->Add( 0, 0, 1, wxEXPAND, KIUI::GetStdMargin() );

    addButtons( false, aRightBtns );

    SetSizer( m_sizer );
    Layout();
}


void BUTTON_ROW_PANEL::addButtons( bool aLeft, const BTN_DEF_LIST& aDefs )
{
    // No button expands to fill horizontally
    const int btn_proportion = 0;

    for( size_t i = 0; i < aDefs.size(); ++i )
    {
        const auto& def = aDefs[i];
        wxButton* btn = new wxButton( this, def.m_id, def.m_text );

        int this_style = wxTOP | wxBOTTOM;

        if( ( aLeft && i > 0 ) || ( !aLeft ) )
            this_style |= wxLEFT;

        if( ( aLeft ) || ( !aLeft && i < aDefs.size() - 1 ) )
            this_style |= wxRIGHT;

        btn->SetToolTip( def.m_tooltip );

        m_sizer->Add( btn, btn_proportion, this_style, KIUI::GetStdMargin() );

        btn->Bind( wxEVT_COMMAND_BUTTON_CLICKED, def.m_callback );
    }
}
