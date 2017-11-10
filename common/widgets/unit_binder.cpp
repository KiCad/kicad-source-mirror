/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 CERN
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
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

#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/textentry.h>
#include <limits>
#include <base_units.h>
#include <wx/valnum.h>

#include "widgets/unit_binder.h"

UNIT_BINDER::UNIT_BINDER( wxWindow* aParent, wxTextEntry* aTextInput,
                                wxStaticText* aUnitLabel, wxSpinButton* aSpinButton ) :
    m_textEntry( aTextInput ),
    m_unitLabel( aUnitLabel ),
    m_units( g_UserUnit ),
    m_step( 1 ),
    m_min( 0 ),
    m_max( 1 )
{
    // Use the currently selected units
    m_textEntry->SetValue( wxT( "0" ) );
    m_unitLabel->SetLabel( GetAbbreviatedUnitsLabel( m_units ) );
}


UNIT_BINDER::~UNIT_BINDER()
{
}


void UNIT_BINDER::SetValue( int aValue )
{
    wxString s = StringFromValue( m_units, aValue, false );

    m_textEntry->SetValue( s );

    m_unitLabel->SetLabel( GetAbbreviatedUnitsLabel( m_units ) );
}


int UNIT_BINDER::GetValue() const
{
    wxString s = m_textEntry->GetValue();

    return ValueFromString( m_units, s );
}


bool UNIT_BINDER::Valid() const
{
    double dummy;

    return m_textEntry->GetValue().ToDouble( &dummy );
}


void UNIT_BINDER::Enable( bool aEnable )
{
    wxWindow* wxWin = dynamic_cast<wxWindow*>( m_textEntry );
    wxASSERT( wxWin );

    // Most text input entry widgets inherit from wxTextEntry and wxWindow, so it should be fine.
    // Still, it is better to be safe than sorry.
    if( wxWin )
        wxWin->Enable( aEnable );

    m_unitLabel->Enable( aEnable );
}

