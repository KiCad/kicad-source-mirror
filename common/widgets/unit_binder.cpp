/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/combobox.h>
#include <wx/stattext.h>
#include <wx/textentry.h>
#include <eda_units.h>
#include <eda_draw_frame.h>
#include <confirm.h>
#include <dialog_shim.h>

#include "widgets/unit_binder.h"
#include "wx/dcclient.h"

using namespace EDA_UNIT_UTILS::UI;


wxDEFINE_EVENT( DELAY_FOCUS, wxCommandEvent );


UNIT_BINDER::UNIT_BINDER( EDA_DRAW_FRAME* aParent, wxStaticText* aLabel, wxWindow* aValueCtrl,
                          wxStaticText* aUnitLabel, bool allowEval, bool aBindFrameEvents ) :
        UNIT_BINDER( aParent, aParent, aLabel, aValueCtrl, aUnitLabel, allowEval, aBindFrameEvents )
{
}

UNIT_BINDER::UNIT_BINDER( UNITS_PROVIDER* aUnitsProvider, wxWindow* aEventSource,
                          wxStaticText* aLabel, wxWindow* aValueCtrl, wxStaticText* aUnitLabel,
                          bool aAllowEval, bool aBindFocusEvent ) :
        m_bindFocusEvent( aBindFocusEvent ),
        m_label( aLabel ),
        m_valueCtrl( aValueCtrl ),
        m_eventSource( aEventSource ),
        m_unitLabel( aUnitLabel ),
        m_iuScale( &aUnitsProvider->GetIuScale() ),
        m_units( aUnitsProvider->GetUserUnits() ),
        m_negativeZero( false ),
        m_dataType( EDA_DATA_TYPE::DISTANCE ),
        m_precision( 0 ),
        m_eval( aUnitsProvider->GetUserUnits() ),
        m_allowEval( aAllowEval && ( !m_valueCtrl || dynamic_cast<wxTextEntry*>( m_valueCtrl ) ) ),
        m_needsEval( false ),
        m_selStart( 0 ),
        m_selEnd( 0 ),
        m_unitsInValue( false ),
        m_originTransforms( aUnitsProvider->GetOriginTransforms() ),
        m_coordType( ORIGIN_TRANSFORMS::NOT_A_COORD )
{
    if( m_valueCtrl )
    {
        // Register the UNIT_BINDER for control state save/restore
        wxWindow* parent = m_valueCtrl->GetParent();

        while( parent && !dynamic_cast<DIALOG_SHIM*>( parent ) )
            parent = parent->GetParent();

        if( parent )
            static_cast<DIALOG_SHIM*>( parent )->RegisterUnitBinder( this, m_valueCtrl );
    }

    wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( m_valueCtrl );

    if( textEntry )
    {
        wxClientDC dc( m_valueCtrl );

        // Gives enough room to display a value in inches in textEntry
        // 3 digits + '.' + 10 digits
        wxSize     minSize = m_valueCtrl->GetMinSize();
        int        minWidth = dc.GetTextExtent( wxT( "XXX.XXXXXXXXXX" ) ).GetWidth();

        if( minSize.GetWidth() < minWidth )
            m_valueCtrl->SetMinSize( wxSize( minWidth, minSize.GetHeight() ) );

        // Use ChangeValue() instead of SetValue() so we don't generate events.
        if( m_negativeZero )
            textEntry->ChangeValue( wxT( "-0" ) );
        else
            textEntry->ChangeValue( wxT( "0" ) );
    }

    if( m_unitLabel )
        m_unitLabel->SetLabel( EDA_UNIT_UTILS::GetLabel( m_units, m_dataType ) );

    if( m_valueCtrl )
    {
        m_valueCtrl->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( UNIT_BINDER::onSetFocus ), nullptr, this );
        m_valueCtrl->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( UNIT_BINDER::onKillFocus ), nullptr, this );
        m_valueCtrl->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( UNIT_BINDER::onClick ), nullptr, this );
        m_valueCtrl->Connect( wxEVT_COMBOBOX, wxCommandEventHandler( UNIT_BINDER::onComboBox ), nullptr, this );
    }

    if( m_bindFocusEvent )
        Connect( DELAY_FOCUS, wxCommandEventHandler( UNIT_BINDER::delayedFocusHandler ), nullptr, this );

    if( m_eventSource )
    {
        m_eventSource->Connect( EDA_EVT_UNITS_CHANGED, wxCommandEventHandler( UNIT_BINDER::onUnitsChanged ),
                               nullptr, this );
    }
}


UNIT_BINDER::~UNIT_BINDER()
{
    if( m_valueCtrl )
    {
        m_valueCtrl->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( UNIT_BINDER::onSetFocus ), nullptr, this );
        m_valueCtrl->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( UNIT_BINDER::onKillFocus ), nullptr, this );
        m_valueCtrl->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( UNIT_BINDER::onClick ), nullptr, this );
        m_valueCtrl->Disconnect( wxEVT_COMBOBOX, wxCommandEventHandler( UNIT_BINDER::onComboBox ), nullptr, this );
    }

    if( m_bindFocusEvent )
        Disconnect( DELAY_FOCUS, wxCommandEventHandler( UNIT_BINDER::delayedFocusHandler ), nullptr, this );

    if( m_eventSource )
    {
        m_eventSource->Disconnect( EDA_EVT_UNITS_CHANGED, wxCommandEventHandler( UNIT_BINDER::onUnitsChanged ),
                                   nullptr, this );
    }
}


void UNIT_BINDER::SetUnits( EDA_UNITS aUnits )
{
    m_units = aUnits;

    m_eval.SetDefaultUnits( m_units );
    m_eval.LocaleChanged();  // In case locale changed since last run

    if( m_unitLabel )
        m_unitLabel->SetLabel( EDA_UNIT_UTILS::GetLabel( m_units, m_dataType ) );
}


void UNIT_BINDER::SetPrecision( int aLength )
{
    m_precision = std::min( aLength, 6 );
}


void UNIT_BINDER::SetDataType( EDA_DATA_TYPE aDataType )
{
    m_dataType = aDataType;

    if( m_unitLabel )
        m_unitLabel->SetLabel( EDA_UNIT_UTILS::GetLabel( m_units, m_dataType ) );
}


void UNIT_BINDER::onUnitsChanged( wxCommandEvent& aEvent )
{
    EDA_BASE_FRAME* provider = static_cast<EDA_BASE_FRAME*>( aEvent.GetClientData() );

    if( !UnitsInvariant() )
    {
        int temp = GetIntValue();

        wxComboBox* const          combo = dynamic_cast<wxComboBox*>( m_valueCtrl );
        std::vector<long long int> comboValues;

        // Read out the current values
        if( combo )
        {
            for( unsigned int i = 0; i < combo->GetCount(); i++ )
            {
                const wxString value = combo->GetString( i );
                long long int  conv = ValueFromString( *m_iuScale, m_units, value, m_dataType );
                comboValues.push_back( conv );
            }
        }

        SetUnits( provider->GetUserUnits() );
        m_iuScale = &provider->GetIuScale();

        // Re-populate the combo box with updated values
        if( combo )
        {
            SetOptionsList( comboValues );
        }

        if( !IsIndeterminate() )
            SetValue( temp );
    }

    aEvent.Skip();
}


void UNIT_BINDER::onClick( wxMouseEvent& aEvent )
{
    wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( m_valueCtrl );

    if( textEntry && ( textEntry->GetValue() == INDETERMINATE_ACTION
                    || textEntry->GetValue() == INDETERMINATE_STATE ) )
    {
        // These are tokens, not strings, so do a select all
        textEntry->SelectAll();
    }

    // Needed at least on Windows to avoid hanging
    aEvent.Skip();
}


void UNIT_BINDER::onComboBox( wxCommandEvent& aEvent )
{
    wxComboBox* combo = dynamic_cast<wxComboBox*>( m_valueCtrl );
    wxCHECK( combo, /*void*/ );

    const wxString      value = combo->GetStringSelection();
    const long long int conv = ValueFromString( *m_iuScale, m_units, value, m_dataType );

    CallAfter(
            [this, conv]
            {
                SetValue( conv );
            } );

    aEvent.Skip();
}


void UNIT_BINDER::onSetFocus( wxFocusEvent& aEvent )
{
    wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( m_valueCtrl );

    if( textEntry )
    {
        if( m_allowEval )
        {
            wxString oldStr = m_eval.OriginalText();

            if( oldStr.length() && oldStr != textEntry->GetValue() )
            {
                textEntry->ChangeValue( oldStr );
                textEntry->SetSelection( m_selStart, m_selEnd );
            }

            m_needsEval = true;
        }

        if( textEntry->GetValue() == INDETERMINATE_ACTION
                || textEntry->GetValue() == INDETERMINATE_STATE )
        {
            // These are tokens, not strings, so do a select all
            textEntry->SelectAll();
        }
    }

    aEvent.Skip();
}


void UNIT_BINDER::onKillFocus( wxFocusEvent& aEvent )
{
    wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( m_valueCtrl );

    if( m_allowEval && textEntry )
    {
        wxString value = textEntry->GetValue();
        bool     success = m_eval.Process( value );

        if( success && !value.IsEmpty() )
        {
            textEntry->GetSelection( &m_selStart, &m_selEnd );

            value = m_eval.Result();

            if( m_unitsInValue && !value.IsEmpty() )
            {
                if( !( m_units == EDA_UNITS::DEGREES || m_units == EDA_UNITS::PERCENT ) )
                    value += wxT( " " );

                value += EDA_UNIT_UTILS::GetLabel( m_units, m_dataType );
            }

            textEntry->ChangeValue( value );

#ifdef __WXGTK__
            // Manually copy the selected text to the primary selection clipboard
            if( wxTheClipboard->Open() )
            {
                wxString sel = textEntry->GetStringSelection();
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
        DisplayErrorMessage( m_valueCtrl->GetParent(), m_errorMessage );

    m_errorMessage = wxEmptyString;
    m_valueCtrl->SetFocus();
}


bool UNIT_BINDER::Validate( double aMin, double aMax, EDA_UNITS aUnits )
{
    wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( m_valueCtrl );

    if( !textEntry
            || textEntry->GetValue() == INDETERMINATE_ACTION
            || textEntry->GetValue() == INDETERMINATE_STATE )
    {
        return true;
    }

    // TODO: Validate() does not currently support m_dataType being anything other than DISTANCE
    // Note: aMin and aMax are not always given in internal units
    if( GetValue() < FromUserUnit( *m_iuScale, aUnits, aMin ) )
    {
        double val_min_iu = FromUserUnit( *m_iuScale, aUnits, aMin );
        m_errorMessage = wxString::Format( _( "%s must be at least %s." ),
                                           valueDescriptionFromLabel( m_label ),
                                           StringFromValue( *m_iuScale, m_units, val_min_iu, true ) );

        textEntry->SelectAll();

        // Don't focus directly; we might be inside a KillFocus event handler
        wxPostEvent( this, wxCommandEvent( DELAY_FOCUS ) );

        return false;
    }

    if( GetValue() > FromUserUnit( *m_iuScale, aUnits, aMax ) )
    {
        double val_max_iu = FromUserUnit( *m_iuScale, aUnits, aMax );
        m_errorMessage = wxString::Format( _( "%s must be less than %s." ),
                                           valueDescriptionFromLabel( m_label ),
                                           StringFromValue( *m_iuScale, m_units, val_max_iu, true ) );

        textEntry->SelectAll();

        // Don't focus directly; we might be inside a KillFocus event handler
        wxPostEvent( this, wxCommandEvent( DELAY_FOCUS ) );

        return false;
    }

    return true;
}


void UNIT_BINDER::SetValue( long long int aValue )
{
    double   displayValue = m_originTransforms.ToDisplay( aValue, m_coordType );
    wxString textValue = StringFromValue( *m_iuScale, m_units, displayValue, false, m_dataType );

    if( displayValue == 0 && m_negativeZero )
        SetValue( wxT( "-" ) + textValue );
    else
        SetValue( textValue );
}


void UNIT_BINDER::SetDoubleValue( double aValue )
{
    double   displayValue = m_originTransforms.ToDisplay( aValue, m_coordType );
    wxString textValue = StringFromValue( *m_iuScale, m_units, setPrecision( displayValue, false ), false,
                                          m_dataType );

    if( displayValue == 0 && !std::signbit( displayValue ) && m_negativeZero )
        SetValue( wxT( "-" ) + textValue );
    else
        SetValue( textValue );
}


void UNIT_BINDER::SetAngleValue( const EDA_ANGLE& aValue )
{
    SetDoubleValue( m_originTransforms.ToDisplay( aValue, m_coordType ) );
}


void UNIT_BINDER::SetValue( const wxString& aValue )
{
    wxTextEntry*  textEntry = dynamic_cast<wxTextEntry*>( m_valueCtrl );
    wxStaticText* staticText = dynamic_cast<wxStaticText*>( m_valueCtrl );

    wxString value = aValue;

    if( m_unitsInValue && !value.IsEmpty() )
    {
        if( !( m_units == EDA_UNITS::DEGREES || m_units == EDA_UNITS::PERCENT ) )
            value += wxT( " " );

        value += EDA_UNIT_UTILS::GetLabel( m_units, m_dataType );
    }

    if( textEntry )
        textEntry->SetValue( value );
    else if( staticText )
        staticText->SetLabel( value );

    if( m_allowEval )
        m_eval.Clear();

    if( m_unitLabel )
        m_unitLabel->SetLabel( EDA_UNIT_UTILS::GetLabel( m_units, m_dataType ) );

}


wxString UNIT_BINDER::getTextForValue( long long int aValue ) const
{
    const double displayValue = m_originTransforms.ToDisplay( aValue, m_coordType );
    wxString     textValue = StringFromValue( *m_iuScale, m_units, setPrecision( displayValue, false ), false,
                                              m_dataType );

    if( displayValue == 0 && m_negativeZero )
        textValue = wxT( "-" ) + textValue;

    return textValue;
}


wxString UNIT_BINDER::getTextForDoubleValue( double aValue ) const
{
    const double displayValue = m_originTransforms.ToDisplay( aValue, m_coordType );
    wxString     textValue = StringFromValue(  *m_iuScale, m_units, setPrecision( displayValue, false ), false,
                                               m_dataType );

    if( displayValue == 0 && !std::signbit( displayValue ) && m_negativeZero )
        textValue = wxT( "-" ) + textValue;

    return textValue;
}


void UNIT_BINDER::ChangeValue( int aValue )
{
    ChangeValue( getTextForValue( aValue ) );
}


void UNIT_BINDER::ChangeDoubleValue( double aValue )
{
    ChangeValue( getTextForDoubleValue( aValue ) );
}


void UNIT_BINDER::ChangeAngleValue( const EDA_ANGLE& aValue )
{
    ChangeDoubleValue( m_originTransforms.ToDisplay( aValue, m_coordType ) );
}


void UNIT_BINDER::ChangeValue( const wxString& aValue )
{
    wxTextEntry*  textEntry = dynamic_cast<wxTextEntry*>( m_valueCtrl );
    wxStaticText* staticText = dynamic_cast<wxStaticText*>( m_valueCtrl );

    wxString value = aValue;

    if( m_unitsInValue && !value.IsEmpty() )
    {
        if( !( m_units == EDA_UNITS::DEGREES || m_units == EDA_UNITS::PERCENT ) )
            value += wxT( " " );

        value += EDA_UNIT_UTILS::GetLabel( m_units, m_dataType );
    }

    if( textEntry )
        textEntry->ChangeValue( value );
    else if( staticText )
        staticText->SetLabel( value );

    if( m_allowEval )
        m_eval.Clear();

    if( m_unitLabel )
        m_unitLabel->SetLabel( EDA_UNIT_UTILS::GetLabel( m_units, m_dataType ) );
}


long long int UNIT_BINDER::GetValue() const
{
    wxTextEntry*  textEntry = dynamic_cast<wxTextEntry*>( m_valueCtrl );
    wxStaticText* staticText = dynamic_cast<wxStaticText*>( m_valueCtrl );
    wxString      value;

    if( textEntry )
    {
        value = textEntry->GetValue();

        if( m_needsEval && !value.IsEmpty() && m_eval.Process( value ) )
            value = m_eval.Result();
        else
            value = textEntry->GetValue();
    }
    else if( staticText )
    {
        value = staticText->GetLabel();
    }
    else
    {
        return 0;
    }

    long long int displayValue = ValueFromString( *m_iuScale, m_units, value, m_dataType );
    return m_originTransforms.FromDisplay( displayValue, m_coordType );
}


double UNIT_BINDER::setPrecision( double aValue, bool aValueUsesUserUnits ) const
{
    if( m_precision > 1 )
    {
        int scale = pow( 10, m_precision );
        int64_t tmp = aValue;

        if( !aValueUsesUserUnits )
            tmp = ToUserUnit( *m_iuScale, m_units, aValue ) * scale;

        aValue = static_cast<double>( tmp ) / scale;

        if( !aValueUsesUserUnits )
            aValue = FromUserUnit( *m_iuScale, m_units, aValue );
    }

    return aValue;
}


double UNIT_BINDER::GetDoubleValue() const
{
    wxTextEntry*  textEntry = dynamic_cast<wxTextEntry*>( m_valueCtrl );
    wxStaticText* staticText = dynamic_cast<wxStaticText*>( m_valueCtrl );
    wxString      value;

    if( textEntry )
    {
        value = textEntry->GetValue();

        if( m_needsEval && !value.IsEmpty() && m_eval.Process( value ) )
            value = m_eval.Result();
        else
            value = textEntry->GetValue();
    }
    else if( staticText )
    {
        value = staticText->GetLabel();
    }
    else
    {
        return 0.0;
    }

    double displayValue = DoubleValueFromString( *m_iuScale, m_units, value, m_dataType );
    displayValue = setPrecision( displayValue, false );

    return m_originTransforms.FromDisplay( displayValue, m_coordType );
}


EDA_ANGLE UNIT_BINDER::GetAngleValue()
{
    return m_originTransforms.FromDisplay( EDA_ANGLE( GetDoubleValue(), DEGREES_T ), m_coordType );
}


void UNIT_BINDER::SetOptionsList( std::span<const long long int> aOptions )
{
    wxComboBox* cb = dynamic_cast<wxComboBox*>( m_valueCtrl );
    wxCHECK( cb, /* void */ );

    cb->Clear();

    for( long long int value : aOptions )
        cb->Append( getTextForValue( value ) );
}


void UNIT_BINDER::SetDoubleOptionsList( std::span<const double> aOptions )
{
    wxComboBox* cb = dynamic_cast<wxComboBox*>( m_valueCtrl );
    wxCHECK( cb, /* void */ );

    cb->Clear();

    for( double value : aOptions )
        cb->Append( getTextForDoubleValue( value ) );
}


bool UNIT_BINDER::IsIndeterminate() const
{
    wxTextEntry* te = dynamic_cast<wxTextEntry*>( m_valueCtrl );

    if( te )
        return te->GetValue() == INDETERMINATE_STATE || te->GetValue() == INDETERMINATE_ACTION;

    return false;
}


bool UNIT_BINDER::IsNull() const
{
    if( wxTextEntry* te = dynamic_cast<wxTextEntry*>( m_valueCtrl ) )
        return te->GetValue().IsEmpty();

    return false;
}


void UNIT_BINDER::SetNull()
{
    if( wxTextEntry* te = dynamic_cast<wxTextEntry*>( m_valueCtrl ) )
        return te->SetValue( wxEmptyString );
}


void UNIT_BINDER::SetLabel( const wxString& aLabel )
{
    m_label->SetLabel( aLabel );
}


void UNIT_BINDER::Enable( bool aEnable )
{
    if( m_label )
        m_label->Enable( aEnable );

    m_valueCtrl->Enable( aEnable );

    if( m_unitLabel )
        m_unitLabel->Enable( aEnable );
}


void UNIT_BINDER::Show( bool aShow, bool aResize )
{
    m_label->Show( aShow );
    m_valueCtrl->Show( aShow );

    if( m_unitLabel )
        m_unitLabel->Show( aShow );

    if( aResize )
    {
        if( aShow )
        {
            m_label->SetSize( -1, -1 );
            m_valueCtrl->SetSize( -1, -1 );

            if( m_unitLabel )
                m_unitLabel->SetSize( -1, -1 );
        }
        else
        {
            m_label->SetSize( 0, 0 );
            m_valueCtrl->SetSize( 0, 0 );

            if( m_unitLabel )
                m_unitLabel->SetSize( 0, 0 );
        }
    }
}


PROPERTY_EDITOR_UNIT_BINDER::PROPERTY_EDITOR_UNIT_BINDER( EDA_DRAW_FRAME* aParent ) :
        UNIT_BINDER( aParent, nullptr, nullptr, nullptr, true, false )
{
    m_unitsInValue = true;
}


PROPERTY_EDITOR_UNIT_BINDER::~PROPERTY_EDITOR_UNIT_BINDER()
{
}

void PROPERTY_EDITOR_UNIT_BINDER::SetControl( wxWindow* aControl )
{
    m_valueCtrl = aControl;

    if( m_valueCtrl )
    {
        m_valueCtrl->Bind( wxEVT_SET_FOCUS,  &PROPERTY_EDITOR_UNIT_BINDER::onSetFocus,  this );
        m_valueCtrl->Bind( wxEVT_KILL_FOCUS, &PROPERTY_EDITOR_UNIT_BINDER::onKillFocus, this );
        m_valueCtrl->Bind( wxEVT_LEFT_UP,    &PROPERTY_EDITOR_UNIT_BINDER::onClick,     this );

        m_valueCtrl->Bind( wxEVT_SHOW,
                           [&]( wxShowEvent& e )
                           {
                               if( !e.IsShown() )
                                   SetControl( nullptr );
                           } );
    }
}
