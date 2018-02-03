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
                          wxStaticText* aLabel, wxTextEntry* aTextEntry, wxStaticText* aUnitLabel,
                          bool aUseMils, int aMin, int aMax, bool allowEval ) :
    m_label( aLabel ),
    m_textEntry( aTextEntry ),
    m_unitLabel( aUnitLabel ),
    m_eval( aParent->GetUserUnits(), aUseMils )
{
    // Fix the units (to the current units) for the life of the binder
    m_units = aParent->GetUserUnits();
    m_useMils = aUseMils;
    m_min = aMin;
    m_max = aMax;
    m_allowEval = allowEval;

    m_textEntry->SetValue( wxT( "0" ) );
    m_unitLabel->SetLabel( GetAbbreviatedUnitsLabel( m_units, aUseMils ) );

    wxWindow* textInput = dynamic_cast<wxWindow*>( m_textEntry );
    textInput->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( UNIT_BINDER::onSetFocus ), NULL, this );
    textInput->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( UNIT_BINDER::onKillFocus ), NULL, this );
    textInput->Connect( wxEVT_TEXT_ENTER, wxCommandEventHandler( UNIT_BINDER::onTextEnter ), NULL, this );
}


UNIT_BINDER::~UNIT_BINDER()
{
}


void UNIT_BINDER::onSetFocus( wxFocusEvent& aEvent )
{
    if( m_allowEval )
    {
        auto oldStr = m_eval.textInput( this );

        if( oldStr )
            m_textEntry->SetValue( wxString::FromUTF8( oldStr ) );
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
    wxPostEvent( dynamic_cast<wxWindow*>( m_textEntry )->GetParent(), event );
}


void UNIT_BINDER::evaluate()
{
    if( m_textEntry->GetValue().IsEmpty() )
        m_textEntry->SetValue( "0" );

    if( m_eval.process( m_textEntry->GetValue().mb_str(), this ) )
        m_textEntry->SetValue( wxString::FromUTF8( m_eval.result() ) );
}


wxString valueDescriptionFromLabel( wxStaticText* aLabel )
{
    wxString desc = aLabel->GetLabel();

    desc.EndsWith( wxT( ":" ), &desc );
    return desc;
}


void UNIT_BINDER::delayedFocusHandler( wxIdleEvent& )
{
    wxWindow* textInput = dynamic_cast<wxWindow*>( m_textEntry );
    textInput->SetFocus();

    textInput->Unbind( wxEVT_IDLE, &UNIT_BINDER::delayedFocusHandler, this );
}


bool UNIT_BINDER::Validate( bool setFocusOnError )
{
    wxWindow* textInput = dynamic_cast<wxWindow*>( m_textEntry );

    if( m_min > INT_MIN && GetValue() < m_min )
    {
        wxString msg = wxString::Format( _( "%s must be larger than %s." ),
                                         valueDescriptionFromLabel( m_label ),
                                         StringFromValue( EDA_UNITS_T::MILLIMETRES, m_min, true ) );
        DisplayError( textInput->GetParent(), msg );

        if( setFocusOnError )
        {
            m_textEntry->SelectAll();
            // Don't focus directly; we might be inside a KillFocus event handler
            textInput->Bind( wxEVT_IDLE, &UNIT_BINDER::delayedFocusHandler, this );
        }

        return false;
    }

    if( m_max < INT_MAX && GetValue() > m_max )
    {
        wxString msg = wxString::Format( _( "%s must be smaller than %s." ),
                                         valueDescriptionFromLabel( m_label ),
                                         StringFromValue( EDA_UNITS_T::MILLIMETRES, m_max, true ) );
        DisplayError( textInput->GetParent(), msg );

        if( setFocusOnError )
        {
            m_textEntry->SelectAll();
            // Don't focus directly; we might be inside a KillFocus event handler
            textInput->Bind( wxEVT_IDLE, &UNIT_BINDER::delayedFocusHandler, this );
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
    m_textEntry->SetValue( aValue );

    if( m_allowEval )
        m_eval.clear();

    m_unitLabel->SetLabel( GetAbbreviatedUnitsLabel( m_units, m_useMils ) );
}


int UNIT_BINDER::GetValue() const
{
    wxString s = m_textEntry->GetValue();

    return ValueFromString( m_units, s, m_useMils );
}


bool UNIT_BINDER::IsIndeterminate() const
{
    return m_textEntry->GetValue() == INDETERMINATE;
}


void UNIT_BINDER::Enable( bool aEnable )
{
    m_label->Enable( aEnable );
    dynamic_cast<wxWindow*>( m_textEntry )->Enable( aEnable );
    m_unitLabel->Enable( aEnable );
}

