/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_get_footprint_by_name.h"

#include <wx/arrstr.h>


DIALOG_GET_FOOTPRINT_BY_NAME::DIALOG_GET_FOOTPRINT_BY_NAME( PCB_BASE_FRAME* aParent,
                                                            wxArrayString&  aFpList ) :
        DIALOG_GET_FOOTPRINT_BY_NAME_BASE( aParent )
{
    m_choiceFpList->Append( aFpList );

    m_multipleHint->SetFont( KIUI::GetInfoFont( this ).Italic() );

    // Hide help string until someone implements successive placement (#2227)
    m_multipleHint->Show( false );

    SetInitialFocus( m_SearchTextCtrl );

    SetupStandardButtons();

    // Dialog should not shrink beyond it's minimal size.
    GetSizer()->SetSizeHints( this );
}


wxString DIALOG_GET_FOOTPRINT_BY_NAME::GetValue() const
{
    return m_SearchTextCtrl->GetValue();
}


void DIALOG_GET_FOOTPRINT_BY_NAME::OnSelectFootprint( wxCommandEvent& aEvent )
{
    if( m_choiceFpList->GetSelection() >= 0 )
    {
        m_SearchTextCtrl->SetValue(
                m_choiceFpList->GetString( m_choiceFpList->GetSelection() ).BeforeFirst( ' ' ) );
    }
}