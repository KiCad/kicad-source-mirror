/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file dialog_eeschema_options.cpp
 */

#include <fctsys.h>
#include <class_base_screen.h>

#include <dialog_eeschema_options.h>


DIALOG_EESCHEMA_OPTIONS::DIALOG_EESCHEMA_OPTIONS( wxWindow* parent ) :
    DIALOG_EESCHEMA_OPTIONS_BASE( parent )
{
    m_choiceUnits->SetFocus();
    m_sdbSizer1OK->SetDefault();

#if defined (KICAD_GOST)

    /* The GOST standard pre-defines certain schematic attributes that cannot be changed */

    m_spinBusWidth->Enable(false);

#endif

}


void DIALOG_EESCHEMA_OPTIONS::SetUnits( const wxArrayString& units, int select )
{
    wxASSERT( units.GetCount() > 0
              && ( select >= 0 && (size_t) select < units.GetCount() ) );

    m_choiceUnits->Append( units );
    m_choiceUnits->SetSelection( select );
}


void DIALOG_EESCHEMA_OPTIONS::SetGridSizes( const GRIDS& grid_sizes, int grid_id )
{
    wxASSERT( grid_sizes.size() > 0 );

    int select = wxNOT_FOUND;

    for( size_t i = 0; i < grid_sizes.size(); i++ )
    {
        wxString tmp;
        tmp.Printf( wxT( "%0.1f" ), grid_sizes[i].m_Size.x );
        m_choiceGridSize->Append( tmp );

        if( grid_sizes[i].m_Id == grid_id )
            select = (int) i;
    }

    m_choiceGridSize->SetSelection( select );
}

void DIALOG_EESCHEMA_OPTIONS::SetFieldName( int aNdx, wxString aName )
{
    switch( aNdx )
    {
    case 0:
        m_fieldName1->SetValue( aName );
        break;

    case 1:
        m_fieldName2->SetValue( aName );
        break;

    case 2:
        m_fieldName3->SetValue( aName );
        break;

    case 3:
        m_fieldName4->SetValue( aName );
        break;

    case 4:
        m_fieldName5->SetValue( aName );
        break;

    case 5:
        m_fieldName6->SetValue( aName );
        break;

    case 6:
        m_fieldName7->SetValue( aName );
        break;

    case 7:
        m_fieldName8->SetValue( aName );
        break;

    default:
        break;
    }
}

wxString DIALOG_EESCHEMA_OPTIONS::GetFieldName( int aNdx )
{
    wxString nme;

    switch ( aNdx )
    {
    case 0:
        nme = m_fieldName1->GetValue();
        break;

    case 1:
        nme = m_fieldName2->GetValue();
        break;

    case 2:
        nme = m_fieldName3->GetValue();
        break;

    case 3:
        nme = m_fieldName4->GetValue();
        break;

    case 4:
        nme = m_fieldName5->GetValue();
        break;

    case 5:
        nme = m_fieldName6->GetValue();
        break;

    case 6:
        nme = m_fieldName7->GetValue();
        break;

    case 7:
        nme = m_fieldName8->GetValue();
        break;

    default:
        break;
    }

    return nme;
}
