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
#include <wx/textentry.h>
#include <limits>
#include <base_units.h>
#include <draw_frame.h>
#include <confirm.h>

#include "widgets/unit_binder.h"

UNIT_BINDER::UNIT_BINDER( EDA_DRAW_FRAME* aParent,
                          wxStaticText* aLabel, wxWindow* aValue, wxStaticText* aUnitLabel,
                          bool aUseMils, int aMin, int aMax, bool allowEval ) :
    m_label( aLabel ),
    m_value( aValue ),
    m_unitLabel( aUnitLabel ),
    m_showMessage( true ),
    m_eval( aParent->GetUserUnits(), aUseMils )
{
    // Fix the units (to the current units) for the life of the binder
    m_units = aParent->GetUserUnits();
    m_useMils = aUseMils;
    m_min = aMin;
    m_max = aMax;
    m_allowEval = allowEval && dynamic_cast<wxTextEntry*>( m_value );

    auto textEntry = dynamic_cast<wxTextEntry*>( m_value );
    if( textEntry )
    {
        // Use ChangeValue() instead of SetValue() so we don't generate events.
        textEntry->ChangeValue( wxT( "0" ) );
    }

    m_unitLabel->SetLabel( GetAbbreviatedUnitsLabel( m_units, m_useMils ) );

    m_value->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( UNIT_BINDER::onSetFocus ), NULL, this );
    m_value->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( UNIT_BINDER::onKillFocus ), NULL, this );
    m_value->Connect( wxEVT_TEXT_ENTER, wxCommandEventHandler( UNIT_BINDER::onTextEnter ), NULL, this );
}


void UNIT_BINDER::SetUnits( EDA_UNITS_T aUnits, bool aUseMils )
{
    m_units = aUnits;
    m_useMils = aUseMils;
    m_unitLabel->SetLabel( GetAbbreviatedUnitsLabel( m_units, m_useMils ) );
}


void UNIT_BINDER::onSetFocus( wxFocusEvent& aEvent )
{
    auto textEntry = dynamic_cast<wxTextEntry*>( m_value );

    if( m_allowEval && textEntry )
    {
        wxString oldStr = m_eval.OriginalText();

        if( oldStr.length() )
            textEntry->SetValue( oldStr );
    }

    aEvent.Skip();
}


void UNIT_BINDER::onKillFocus( wxFocusEvent& aEvent )
{
    // The ship is going down; no need to do anything...
    if( !aEvent.GetWindow() || aEvent.GetWindow()->GetId() == wxID_CANCEL )
        return;

    if( m_allowEval )
        evaluate();

    Validate( true );

    aEvent.Skip();
}


void UNIT_BINDER::onTextEnter( wxCommandEvent& aEvent )
{
    if( m_allowEval )
        evaluate();

    // Send an OK event to the parent dialog
    wxCommandEvent event( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK );
    wxPostEvent( m_value->GetParent(), event );
}


void UNIT_BINDER::evaluate()
{
    auto textEntry = dynamic_cast<wxTextEntry*>( m_value );

    if( !textEntry )
        return;

    if( textEntry->GetValue().IsEmpty() )
        textEntry->ChangeValue( "0" );

    if( m_eval.Process( textEntry->GetValue() ) )
        textEntry->ChangeValue( m_eval.Result() );
}


wxString valueDescriptionFromLabel( wxStaticText* aLabel )
{
    wxString desc = aLabel->GetLabel();

    desc.EndsWith( wxT( ":" ), &desc );
    return desc;
}


void UNIT_BINDER::delayedFocusHandler( wxIdleEvent& )
{
    m_value->SetFocus();
    m_showMessage = true;

    m_value->Unbind( wxEVT_IDLE, &UNIT_BINDER::delayedFocusHandler, this );
}


bool UNIT_BINDER::Validate( bool setFocusOnError )
{
    auto textEntry = dynamic_cast<wxTextEntry*>( m_value );

    if( !textEntry || textEntry->GetValue() == INDETERMINATE )
        return true;

    if( m_min > INT_MIN && GetValue() < m_min )
    {
        if( m_showMessage )
        {
            wxString msg = wxString::Format( _( "%s must be larger than %s." ),
                                             valueDescriptionFromLabel( m_label ),
                                             StringFromValue( m_units, m_min, true ) );
            DisplayError( m_value->GetParent(), msg );
        }

        if( setFocusOnError )
        {
            textEntry->SelectAll();
            // Don't focus directly; we might be inside a KillFocus event handler
            m_value->Bind( wxEVT_IDLE, &UNIT_BINDER::delayedFocusHandler, this );
            m_showMessage = false;
        }

        return false;
    }

    if( m_max < INT_MAX && GetValue() > m_max )
    {
        if( m_showMessage )
        {
            wxString msg = wxString::Format( _( "%s must be smaller than %s." ),
                                             valueDescriptionFromLabel( m_label ),
                                             StringFromValue( m_units, m_max, true ) );
            DisplayError( m_value->GetParent(), msg );
        }

        if( setFocusOnError )
        {
            textEntry->SelectAll();
            // Don't focus directly; we might be inside a KillFocus event handler
            m_value->Bind( wxEVT_IDLE, &UNIT_BINDER::delayedFocusHandler, this );
            m_showMessage = false;
        }

        return false;
    }

    return true;
}


void UNIT_BINDER::SetValue( int aValue )
{
    SetValue( StringFromValue( m_units, aValue, false, m_useMils ) );
}


void UNIT_BINDER::SetValue( wxString aValue )
{
    if( dynamic_cast<wxTextEntry*>( m_value ) )
        dynamic_cast<wxTextEntry*>( m_value )->SetValue( aValue );
    else if( dynamic_cast<wxStaticText*>( m_value ) )
        dynamic_cast<wxStaticText*>( m_value )->SetLabel( aValue );

    if( m_allowEval )
        m_eval.Clear();

    m_unitLabel->SetLabel( GetAbbreviatedUnitsLabel( m_units, m_useMils ) );
}


int UNIT_BINDER::GetValue() const
{
    wxString s;

    if( dynamic_cast<wxTextEntry*>( m_value ) )
        s = dynamic_cast<wxTextEntry*>( m_value )->GetValue();
    else if( dynamic_cast<wxStaticText*>( m_value ) )
        s = dynamic_cast<wxStaticText*>( m_value )->GetLabel();

    return ValueFromString( m_units, s, m_useMils );
}


bool UNIT_BINDER::IsIndeterminate() const
{
    if( dynamic_cast<wxTextEntry*>( m_value ) )
        return dynamic_cast<wxTextEntry*>( m_value )->GetValue() == INDETERMINATE;

    return false;
}


void UNIT_BINDER::SetLabel( const wxString& aLabel )
{
    m_label->SetLabel( aLabel );
}


void UNIT_BINDER::Enable( bool aEnable )
{
    m_label->Enable( aEnable );
    m_value->Enable( aEnable );
    m_unitLabel->Enable( aEnable );
}


void UNIT_BINDER::Show( bool aShow )
{
    m_label->Show( aShow );
    m_value->Show( aShow );
    m_unitLabel->Show( aShow );
}

