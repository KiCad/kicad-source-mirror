/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <incremental_text_ctrl.h>

/**
 * Check that a string looks like a floating point number that can
 * be dealt with.
 */
static bool validateFloatField( const wxString& aStr )
{
     // Skip empty fields
    if( aStr.size() == 0 )
        return false;

    // a single . or , doesn't count as number, although valid in a float
    if( aStr.size() == 1 )
    {
        if( (aStr.compare( "." ) == 0) ||
            (aStr.compare( "," ) == 0) )
            return false;
    }

    return true;
}


INCREMENTAL_TEXT_CTRL::INCREMENTAL_TEXT_CTRL() :
    m_minVal( 0.0 ),
    m_maxVal( 1.0 ),
    m_currentValue( 0.0 ),
    m_precision( 4 )
{}


void INCREMENTAL_TEXT_CTRL::SetStep( double aMin, double aMax,
    STEP_FUNCTION aStepFunc )
{
    wxASSERT( aMin <= aMax );

    m_minVal = std::min( aMin, aMax );
    m_maxVal = std::max( aMin, aMax );
    m_stepFunc = aStepFunc;

    // finally, clamp the current value and re-display
    updateTextValue();
}


void INCREMENTAL_TEXT_CTRL::updateTextValue()
{
    if( m_currentValue > m_maxVal )
        m_currentValue = m_maxVal;

    if( m_currentValue < m_minVal )
        m_currentValue = m_minVal;

    wxString fmt = wxString::Format( "%%.%df", m_precision );
    setTextCtrl( wxString::Format( fmt, m_currentValue ) );
}


void INCREMENTAL_TEXT_CTRL::incrementCtrlBy( double aInc )
{
    const wxString txt = getCtrlText();
    if( !validateFloatField( txt ) )
        return;

    txt.ToDouble( &m_currentValue );
    m_currentValue += aInc;

    updateTextValue();
}


void INCREMENTAL_TEXT_CTRL::incrementCtrl( bool aUp )
{
    incrementCtrlBy( m_stepFunc( aUp, m_currentValue ) );
}


void INCREMENTAL_TEXT_CTRL::SetPrecision( int aPrecision )
{
    m_precision = aPrecision;
}


void INCREMENTAL_TEXT_CTRL::SetValue( double aValue )
{
    m_currentValue = aValue;
    updateTextValue();
}


double INCREMENTAL_TEXT_CTRL::GetValue()
{
    // sanitise before handing the value - if the user did something
    // like close a window with outstanding text changes, we need
    // to clamp the value and re-interpret the text
    incrementCtrlBy( 0.0 );

    return m_currentValue;
}


SPIN_INCREMENTAL_TEXT_CTRL::SPIN_INCREMENTAL_TEXT_CTRL( wxSpinButton& aSpinBtn,
                       wxTextCtrl& aTextCtrl ):
        m_spinBtn( aSpinBtn ),
        m_textCtrl( aTextCtrl )
{
    (void) m_spinBtn;

    // set always enabled, otherwise it's very hard to keep in sync
    aSpinBtn.SetRange( -INT_MAX, INT_MAX );

    auto spinUpHandler = [this] ( wxSpinEvent& event )
    {
        incrementCtrl( true );
    };

    // spin up/down if a single step of the field
    auto spinDownHandler = [this] ( wxSpinEvent& event )
    {
        incrementCtrl( false );
    };

    auto mouseWheelHandler = [this] ( wxMouseEvent& aEvent )
    {
        incrementCtrl( aEvent.GetWheelRotation() >= 0 );
    };

    aSpinBtn.Bind( wxEVT_SPIN_UP, spinUpHandler );
    aSpinBtn.Bind( wxEVT_SPIN_DOWN, spinDownHandler );

    m_textCtrl.Bind( wxEVT_MOUSEWHEEL, mouseWheelHandler );

    m_textCtrl.Bind( wxEVT_KILL_FOCUS, &SPIN_INCREMENTAL_TEXT_CTRL::onFocusLoss, this );
}

SPIN_INCREMENTAL_TEXT_CTRL::~SPIN_INCREMENTAL_TEXT_CTRL()
{
    // this must be unbound, as kill focus can arrive after the
    // text control is gone
    m_textCtrl.Unbind( wxEVT_KILL_FOCUS, &SPIN_INCREMENTAL_TEXT_CTRL::onFocusLoss, this );
}


void SPIN_INCREMENTAL_TEXT_CTRL::onFocusLoss( wxFocusEvent& aEvent )
{
    // re-read the input and sanitize any user changes
    incrementCtrlBy( 0.0 );
}


void SPIN_INCREMENTAL_TEXT_CTRL::setTextCtrl( const wxString& val )
{
    m_textCtrl.SetValue( val );
}


wxString SPIN_INCREMENTAL_TEXT_CTRL::getCtrlText() const
{
    return m_textCtrl.GetValue();
}

