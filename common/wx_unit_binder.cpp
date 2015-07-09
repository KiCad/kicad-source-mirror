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
#include <wx/textctrl.h>
#include <limits>
#include <base_units.h>
#if wxCHECK_VERSION( 2, 9, 0 )
#include <wx/valnum.h>
#endif

#include <boost/optional.hpp>

#include "wx_unit_binder.h"

WX_UNIT_BINDER::WX_UNIT_BINDER( wxWindow* aParent, wxTextCtrl* aTextInput,
                                wxStaticText* aUnitLabel, wxSpinButton* aSpinButton ) :
    m_textCtrl( aTextInput ),
    m_unitLabel( aUnitLabel ),
    m_units( g_UserUnit ),
    m_step( 1 ),
    m_min( 0 ),
    m_max( 1 )
{
    // Use the currently selected units
    m_textCtrl->SetValue( wxT( "0" ) );
    m_unitLabel->SetLabel( GetAbbreviatedUnitsLabel( m_units ) );
}


WX_UNIT_BINDER::~WX_UNIT_BINDER()
{
}


void WX_UNIT_BINDER::SetValue( int aValue )
{
    wxString s = StringFromValue( m_units, aValue, false );

    m_textCtrl->SetValue( s );

    m_unitLabel->SetLabel( GetAbbreviatedUnitsLabel( m_units ) );
}


int WX_UNIT_BINDER::GetValue() const
{
    wxString s = m_textCtrl->GetValue();

    return ValueFromString( m_units, s );
}


bool WX_UNIT_BINDER::Valid() const
{
    double dummy;

    return m_textCtrl->GetValue().ToDouble( &dummy );
}


void WX_UNIT_BINDER::Enable( bool aEnable )
{
    m_textCtrl->Enable( aEnable );
    m_unitLabel->Enable( aEnable );
}
