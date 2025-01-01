/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <wx/aui/aui.h>
#include <dcode.h>

#include "dcode_selection_box.h"


DCODE_SELECTION_BOX::DCODE_SELECTION_BOX( wxAuiToolBar* aParent, wxWindowID aId,
                                          const wxPoint& aLocation, const wxSize& aSize,
                                          const wxArrayString* aChoices ) :
    wxComboBox( aParent, aId, wxEmptyString, aLocation, aSize, 0, nullptr, wxCB_READONLY )
{
    if( aChoices )
        // Append aChoices here is by far faster than use aChoices inside
        // the wxComboBox constructor
        Append( *aChoices );
}


DCODE_SELECTION_BOX::~DCODE_SELECTION_BOX()
{
}


int DCODE_SELECTION_BOX::GetSelectedDCodeId()
{
    int ii = GetSelection();

    if( ii > 0 )
    {
        // in strings displayed by the combo box, the dcode number
        // is the second word. get it:
        wxString msg = GetString( ii ).AfterFirst( ' ' ).BeforeFirst( ' ' );
        long id;

        if( msg.ToLong( &id ) )
            return id;
    }

    return 0;
}


void DCODE_SELECTION_BOX::SetDCodeSelection( int aDCodeId )
{
    wxString msg;

    for( unsigned index = 1; index < GetCount(); ++index )
    {
        msg = GetString( index ).AfterFirst( ' ' ).BeforeFirst( ' ' );
        long id;

        if( msg.ToLong(&id) && id == aDCodeId )
        {
            SetSelection( index );
            return;
        }
    }

    SetSelection( 0 );
}


void DCODE_SELECTION_BOX::AppendDCodeList( const wxArrayString& aChoices )
{
    Append( aChoices );
}
