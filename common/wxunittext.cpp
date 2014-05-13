/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014  CERN
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

#include "wxunittext.h"
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <limits>
#include <base_units.h>
#if wxCHECK_VERSION( 2, 9, 0 )
#include <wx/valnum.h>
#endif
#include <boost/optional.hpp>

WX_UNIT_TEXT::WX_UNIT_TEXT( wxWindow* aParent, const wxString& aLabel, double aValue, double aStep ) :
    wxPanel( aParent, wxID_ANY ),
    m_step( aStep )
{
    // Use the currently selected units
    m_units = g_UserUnit;

    wxBoxSizer* sizer;
    sizer = new wxBoxSizer( wxHORIZONTAL );

    // Helper label
    m_inputLabel = new wxStaticText( this, wxID_ANY, aLabel,
                                     wxDefaultPosition, wxDefaultSize, 0 );
    wxSize size = m_inputLabel->GetMinSize();
    size.SetWidth( 150 );
    m_inputLabel->SetMinSize( size );
    sizer->Add( m_inputLabel, 1, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 5 );

    // Main input control
    m_inputValue = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                   wxTE_PROCESS_ENTER );

    SetValue( aValue );
    sizer->Add( m_inputValue, 0, wxALIGN_CENTER_VERTICAL | wxALL );

#if wxCHECK_VERSION( 2, 9, 0 )  // Sorry guys, I am tired of dealing with 2.8 compatibility
    wxFloatingPointValidator<double> validator( 4, NULL, wxNUM_VAL_NO_TRAILING_ZEROES );
    validator.SetRange( 0.0, std::numeric_limits<double>::max() );
    m_inputValue->SetValidator( validator );

    // Spin buttons for modifying values using the mouse
    m_spinButton = new wxSpinButton( this, wxID_ANY );
    m_spinButton->SetRange( std::numeric_limits<int>::min(), std::numeric_limits<int>::max() );

    m_spinButton->SetCanFocus( false );
    sizer->Add( m_spinButton, 0, wxALIGN_CENTER_VERTICAL | wxALL );

    Connect( wxEVT_SPIN_UP, wxSpinEventHandler( WX_UNIT_TEXT::onSpinUpEvent ), NULL, this );
    Connect( wxEVT_SPIN_DOWN, wxSpinEventHandler( WX_UNIT_TEXT::onSpinDownEvent ), NULL, this );
#endif

    sizer->AddSpacer( 5 );

    // Create units label
    m_unitLabel = new wxStaticText( this, wxID_ANY, GetUnitsLabel( g_UserUnit ),
                                    wxDefaultPosition, wxDefaultSize, 0 );
    sizer->Add( m_unitLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL );

    SetSizer( sizer );
    Layout();
}


WX_UNIT_TEXT::~WX_UNIT_TEXT()
{
}


void WX_UNIT_TEXT::SetUnits( EDA_UNITS_T aUnits, bool aConvert )
{
    assert( !aConvert );        // TODO conversion does not work yet

    m_unitLabel->SetLabel( GetUnitsLabel( g_UserUnit ) );
}


void WX_UNIT_TEXT::SetValue( double aValue )
{
    assert( aValue >= 0.0 );

    if( aValue >= 0.0 )
    {
        m_inputValue->SetValue( wxString( Double2Str( aValue ).c_str(), wxConvUTF8 ) );
        m_inputValue->MarkDirty();
    }
}


/*boost::optional<double> WX_UNIT_TEXT::GetValue( EDA_UNITS_T aUnit ) const
{
    if( aUnit == m_units )
        return GetValue();  // no conversion needed

    switch( m_units )
    {
    case MILLIMETRES:
        switch( aUnit )
        {
        case INCHES:
            iu = Mils2iu( GetValue() * 1000.0 );
            break;

        case UNSCALED_UNITS:
            iu = GetValue();
            break;
        }
        break;

    case INCHES:
        switch( aUnit )
        {
        case MILLIMETRES:
            return Mils2mm( GetValue() * 1000.0 );
            break;

        case UNSCALED_UNITS:
            return Mils2iu( GetValue() * 1000.0 );
            break;
        }
        break;

    case UNSCALED_UNITS:
        switch( aUnit )
        {
        case MILLIMETRES:
            return Iu2Mils( GetValue() ) / 1000.0;
            break;

//        case INCHES:
//            return
//            break;
        }
        break;
    }

    assert( false );        // seems that there are some conversions missing

    return 0.0;
}*/


boost::optional<double> WX_UNIT_TEXT::GetValue() const
{
    wxString text = m_inputValue->GetValue();
    double value;

    if( !text.ToDouble( &value ) )
        return boost::optional<double>();

    return boost::optional<double>( value );
}


void WX_UNIT_TEXT::onSpinUpEvent( wxSpinEvent& aEvent )
{
    SetValue( *GetValue() + m_step );
}


void WX_UNIT_TEXT::onSpinDownEvent( wxSpinEvent& aEvent )
{
    double newValue = *GetValue() - m_step;

    if( newValue >= 0.0 )
        SetValue( newValue );
}
