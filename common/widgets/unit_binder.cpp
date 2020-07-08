/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 CERN
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/clipbrd.h>
#include <wx/stattext.h>
#include <wx/textentry.h>
#include <limits>
#include <base_units.h>
#include <eda_draw_frame.h>
#include <confirm.h>

#include "widgets/unit_binder.h"

wxDEFINE_EVENT( DELAY_FOCUS, wxCommandEvent );

UNIT_BINDER::UNIT_BINDER( EDA_DRAW_FRAME* aParent,
                          wxStaticText* aLabel, wxWindow* aValue, wxStaticText* aUnitLabel,
                          bool aUseMils, bool allowEval ) :
        m_frame( aParent ),
        m_label( aLabel ),
        m_value( aValue ),
        m_unitLabel( aUnitLabel ),
        m_eval( aParent->GetUserUnits(), aUseMils )
{
    m_units     = aParent->GetUserUnits();
    m_useMils   = aUseMils;
    m_dataType  = EDA_DATA_TYPE::DISTANCE;
    m_allowEval = allowEval && dynamic_cast<wxTextEntry*>( m_value );
    m_needsEval = false;
    m_selStart  = 0;
    m_selEnd    = 0;

    wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( m_value );

    if( textEntry )
    {
        // Use ChangeValue() instead of SetValue() so we don't generate events.
        textEntry->ChangeValue( wxT( "0" ) );
    }

    m_unitLabel->SetLabel( GetAbbreviatedUnitsLabel( m_units, m_useMils, m_dataType ) );

    m_value->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( UNIT_BINDER::onSetFocus ), NULL, this );
    m_value->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( UNIT_BINDER::onKillFocus ), NULL, this );
    Connect( DELAY_FOCUS, wxCommandEventHandler( UNIT_BINDER::delayedFocusHandler ), NULL, this );

    m_frame->Connect( UNITS_CHANGED, wxCommandEventHandler( UNIT_BINDER::onUnitsChanged ), nullptr, this );
}


UNIT_BINDER::~UNIT_BINDER()
{
    m_frame->Disconnect( UNITS_CHANGED, wxCommandEventHandler( UNIT_BINDER::onUnitsChanged ), nullptr, this );
}


void UNIT_BINDER::SetUnits( EDA_UNITS aUnits, bool aUseMils )
{
    m_units = aUnits;
    m_useMils = aUseMils;
    m_unitLabel->SetLabel( GetAbbreviatedUnitsLabel( m_units, m_useMils, m_dataType ) );
}


void UNIT_BINDER::SetDataType( EDA_DATA_TYPE aDataType )
{
    m_dataType = aDataType;
    m_unitLabel->SetLabel( GetAbbreviatedUnitsLabel( m_units, m_useMils, m_dataType ) );
}


void UNIT_BINDER::onUnitsChanged( wxCommandEvent& aEvent )
{
    int temp = (int) GetValue();

    SetUnits( m_frame->GetUserUnits(), m_useMils );

    SetValue( temp );

    aEvent.Skip();
}


void UNIT_BINDER::onSetFocus( wxFocusEvent& aEvent )
{
    wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( m_value );

    if( m_allowEval && textEntry )
    {
        wxString oldStr = m_eval.OriginalText();

        if( oldStr.length() )
        {
            textEntry->SetValue( oldStr );
            textEntry->SetSelection( m_selStart, m_selEnd );
        }

        m_needsEval = true;
    }

    aEvent.Skip();
}


void UNIT_BINDER::onKillFocus( wxFocusEvent& aEvent )
{
    wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( m_value );

    if( m_allowEval && textEntry )
    {
        if( m_eval.Process( textEntry->GetValue() ) )
        {
            textEntry->GetSelection( &m_selStart, &m_selEnd );
            wxString sel = textEntry->GetStringSelection();

            textEntry->ChangeValue( m_eval.Result() );

#ifdef __WXGTK__
            // Manually copy the selected text to the primary selection clipboard
            if( wxTheClipboard->Open() )
            {
                bool clipTarget = wxTheClipboard->IsUsingPrimarySelection();
                wxTheClipboard->UsePrimarySelection( true );
                wxTheClipboard->SetData( new wxTextDataObject( sel ) );
                wxTheClipboard->UsePrimarySelection( clipTarget );
                wxTheClipboard->Close();
            }
#endif
        }

        m_needsEval = false;
    }

    aEvent.Skip();
}


wxString valueDescriptionFromLabel( wxStaticText* aLabel )
{
    wxString desc = aLabel->GetLabel();

    desc.EndsWith( wxT( ":" ), &desc );
    return desc;
}


void UNIT_BINDER::delayedFocusHandler( wxCommandEvent& )
{
    if( !m_errorMessage.IsEmpty() )
        DisplayError( m_value->GetParent(), m_errorMessage );

    m_errorMessage = wxEmptyString;
    m_value->SetFocus();
}


bool UNIT_BINDER::Validate( double aMin, double aMax, EDA_UNITS aUnits, bool aUseMils )
{
    wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( m_value );

    if( !textEntry
            || textEntry->GetValue() == INDETERMINATE_ACTION
            || textEntry->GetValue() == INDETERMINATE_STATE )
    {
        return true;
    }

    // TODO: Validate() does not currently support m_dataType being anything other than DISTANCE
    // Note: aMin and aMax are not always given in internal units
    if( GetValue() < From_User_Unit( aUnits, aMin, aUseMils ) )
    {
        double val_min_iu = From_User_Unit( aUnits, aMin, aUseMils );
        m_errorMessage = wxString::Format( _( "%s must be at least %s." ),
                                           valueDescriptionFromLabel( m_label ),
                                           StringFromValue( m_units, val_min_iu, true, m_useMils ) );

        textEntry->SelectAll();
        // Don't focus directly; we might be inside a KillFocus event handler
        wxPostEvent( this, wxCommandEvent( DELAY_FOCUS ) );

        return false;
    }

    if( GetValue() > From_User_Unit( aUnits, aMax, aUseMils ) )
    {
        double val_max_iu = From_User_Unit( aUnits, aMax, aUseMils );
        m_errorMessage = wxString::Format( _( "%s must be less than %s." ),
                                           valueDescriptionFromLabel( m_label ),
                                           StringFromValue( m_units, val_max_iu, true, m_useMils ) );

        textEntry->SelectAll();
        // Don't focus directly; we might be inside a KillFocus event handler
        wxPostEvent( this, wxCommandEvent( DELAY_FOCUS ) );

        return false;
    }

    return true;
}


void UNIT_BINDER::SetValue( int aValue )
{
    SetValue( StringFromValue( m_units, aValue, false, m_useMils, m_dataType ) );
}


void UNIT_BINDER::SetDoubleValue( double aValue )
{
    SetValue( StringFromValue( m_units, aValue, false, m_useMils, m_dataType ) );
}


void UNIT_BINDER::SetValue( wxString aValue )
{
    wxTextEntry*  textEntry = dynamic_cast<wxTextEntry*>( m_value );
    wxStaticText* staticText = dynamic_cast<wxStaticText*>( m_value );

    if( textEntry )
        textEntry->SetValue( aValue );
    else if( staticText )
        staticText->SetLabel( aValue );

    if( m_allowEval )
        m_eval.Clear();

    m_unitLabel->SetLabel( GetAbbreviatedUnitsLabel( m_units, m_useMils, m_dataType ) );
}


void UNIT_BINDER::ChangeValue( int aValue )
{
    ChangeValue( StringFromValue( m_units, aValue, false, m_useMils ) );
}


void UNIT_BINDER::ChangeValue( const wxString& aValue )
{
    wxTextEntry*  textEntry = dynamic_cast<wxTextEntry*>( m_value );
    wxStaticText* staticText = dynamic_cast<wxStaticText*>( m_value );

    if( textEntry )
        textEntry->ChangeValue( aValue );
    else if( staticText )
        staticText->SetLabel( aValue );

    if( m_allowEval )
        m_eval.Clear();

    m_unitLabel->SetLabel( GetAbbreviatedUnitsLabel( m_units, m_useMils, m_dataType ) );
}


long long int UNIT_BINDER::GetValue()
{
    wxTextEntry*  textEntry = dynamic_cast<wxTextEntry*>( m_value );
    wxStaticText* staticText = dynamic_cast<wxStaticText*>( m_value );
    wxString      value;

    if( textEntry )
    {
        if( m_needsEval && m_eval.Process( textEntry->GetValue() ) )
            value = m_eval.Result();
        else
            value = textEntry->GetValue();
    }
    else if( staticText )
        value = staticText->GetLabel();
    else
        return 0;

    return ValueFromString( m_units, value, m_useMils, m_dataType );
}


double UNIT_BINDER::GetDoubleValue()
{
    wxTextEntry*  textEntry = dynamic_cast<wxTextEntry*>( m_value );
    wxStaticText* staticText = dynamic_cast<wxStaticText*>( m_value );
    wxString      value;

    if( textEntry )
    {
        if( m_needsEval && m_eval.Process( textEntry->GetValue() ) )
            value = m_eval.Result();
        else
            value = textEntry->GetValue();
    }
    else if( staticText )
        value = staticText->GetLabel();
    else
        return 0.0;

    return DoubleValueFromString( m_units, value, m_useMils, m_dataType );
}


bool UNIT_BINDER::IsIndeterminate() const
{
    wxTextEntry* te = dynamic_cast<wxTextEntry*>( m_value );

    if( te )
        return te->GetValue() == INDETERMINATE_STATE || te->GetValue() == INDETERMINATE_ACTION;

    return false;
}


wxString UNIT_BINDER::GetOriginalText() const
{
    wxTextEntry*  textEntry = dynamic_cast<wxTextEntry*>( m_value );
    wxStaticText* staticText = dynamic_cast<wxStaticText*>( m_value );

    if( m_allowEval )
        return m_eval.OriginalText();
    else if( textEntry )
        return textEntry->GetValue();
    else if( staticText )
        return staticText->GetLabel();
    else
        return wxEmptyString;
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


void UNIT_BINDER::Show( bool aShow, bool aResize )
{
    m_label->Show( aShow );
    m_value->Show( aShow );
    m_unitLabel->Show( aShow );

    if( aResize )
    {
        if( aShow )
        {
            m_label->SetSize( -1, -1 );
            m_value->SetSize( -1, -1 );
            m_unitLabel->SetSize( -1, -1 );
        }
        else
        {
            m_label->SetSize( 0, 0 );
            m_value->SetSize( 0, 0 );
            m_unitLabel->SetSize( 0, 0 );
        }
    }
}

