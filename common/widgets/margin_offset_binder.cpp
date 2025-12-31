/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/margin_offset_binder.h>

#include <wx/stattext.h>
#include <wx/textentry.h>
#include <wx/regex.h>
#include <eda_units.h>
#include <eda_draw_frame.h>

using namespace EDA_UNIT_UTILS::UI;


MARGIN_OFFSET_BINDER::MARGIN_OFFSET_BINDER( EDA_DRAW_FRAME* aParent, wxStaticText* aLabel,
                                            wxWindow* aValueCtrl, wxStaticText* aUnitLabel ) :
        MARGIN_OFFSET_BINDER( aParent, aParent, aLabel, aValueCtrl, aUnitLabel )
{
}


MARGIN_OFFSET_BINDER::MARGIN_OFFSET_BINDER( UNITS_PROVIDER* aUnitsProvider,
                                            wxWindow* aEventSource, wxStaticText* aLabel,
                                            wxWindow* aValueCtrl, wxStaticText* aUnitLabel ) :
        m_label( aLabel ),
        m_valueCtrl( aValueCtrl ),
        m_eventSource( aEventSource ),
        m_unitLabel( aUnitLabel ),
        m_iuScale( &aUnitsProvider->GetIuScale() ),
        m_units( aUnitsProvider->GetUserUnits() ),
        m_eval( aUnitsProvider->GetUserUnits() ),
        m_cachedOffset( std::nullopt ),
        m_cachedRatio( std::nullopt ),
        m_needsParsing( true )
{
    if( m_valueCtrl )
    {
        m_valueCtrl->Connect( wxEVT_SET_FOCUS,
                              wxFocusEventHandler( MARGIN_OFFSET_BINDER::onSetFocus ),
                              nullptr, this );
        m_valueCtrl->Connect( wxEVT_KILL_FOCUS,
                              wxFocusEventHandler( MARGIN_OFFSET_BINDER::onKillFocus ),
                              nullptr, this );
    }

    if( m_eventSource )
    {
        m_eventSource->Connect( EDA_EVT_UNITS_CHANGED,
                                wxCommandEventHandler( MARGIN_OFFSET_BINDER::onUnitsChanged ),
                                nullptr, this );
    }

    // Update the unit label to indicate combined format
    if( m_unitLabel )
        m_unitLabel->SetLabel( wxEmptyString );
}


MARGIN_OFFSET_BINDER::~MARGIN_OFFSET_BINDER()
{
    if( m_valueCtrl )
    {
        m_valueCtrl->Disconnect( wxEVT_SET_FOCUS,
                                 wxFocusEventHandler( MARGIN_OFFSET_BINDER::onSetFocus ),
                                 nullptr, this );
        m_valueCtrl->Disconnect( wxEVT_KILL_FOCUS,
                                 wxFocusEventHandler( MARGIN_OFFSET_BINDER::onKillFocus ),
                                 nullptr, this );
    }

    if( m_eventSource )
    {
        m_eventSource->Disconnect( EDA_EVT_UNITS_CHANGED,
                                   wxCommandEventHandler( MARGIN_OFFSET_BINDER::onUnitsChanged ),
                                   nullptr, this );
    }
}


void MARGIN_OFFSET_BINDER::SetOffsetValue( std::optional<int> aValue )
{
    m_cachedOffset = aValue;
    m_needsParsing = false;
    setTextValue( formatValue( m_cachedOffset, m_cachedRatio ) );
}


void MARGIN_OFFSET_BINDER::SetRatioValue( std::optional<double> aRatio )
{
    m_cachedRatio = aRatio;
    m_needsParsing = false;
    setTextValue( formatValue( m_cachedOffset, m_cachedRatio ) );
}


std::optional<int> MARGIN_OFFSET_BINDER::GetOffsetValue() const
{
    if( m_needsParsing )
    {
        wxString input = getTextValue();
        parseInput( input, m_cachedOffset, m_cachedRatio );
        m_needsParsing = false;
    }

    return m_cachedOffset;
}


std::optional<double> MARGIN_OFFSET_BINDER::GetRatioValue() const
{
    if( m_needsParsing )
    {
        wxString input = getTextValue();
        parseInput( input, m_cachedOffset, m_cachedRatio );
        m_needsParsing = false;
    }

    return m_cachedRatio;
}


bool MARGIN_OFFSET_BINDER::IsNull() const
{
    wxString text = getTextValue();
    text.Trim( true ).Trim( false );
    return text.IsEmpty();
}


void MARGIN_OFFSET_BINDER::Enable( bool aEnable )
{
    if( m_label )
        m_label->Enable( aEnable );

    if( m_valueCtrl )
        m_valueCtrl->Enable( aEnable );

    if( m_unitLabel )
        m_unitLabel->Enable( aEnable );
}


void MARGIN_OFFSET_BINDER::Show( bool aShow, bool aResize )
{
    if( m_label )
    {
        m_label->Show( aShow );

        if( aResize )
        {
            if( aShow )
                m_label->SetSize( -1, -1 );
            else
                m_label->SetSize( 0, 0 );
        }
    }

    if( m_valueCtrl )
    {
        m_valueCtrl->Show( aShow );

        if( aResize )
        {
            if( aShow )
                m_valueCtrl->SetSize( -1, -1 );
            else
                m_valueCtrl->SetSize( 0, 0 );
        }
    }

    if( m_unitLabel )
    {
        m_unitLabel->Show( aShow );

        if( aResize )
        {
            if( aShow )
                m_unitLabel->SetSize( -1, -1 );
            else
                m_unitLabel->SetSize( 0, 0 );
        }
    }
}


void MARGIN_OFFSET_BINDER::onSetFocus( wxFocusEvent& aEvent )
{
    m_needsParsing = true;
    aEvent.Skip();
}


void MARGIN_OFFSET_BINDER::onKillFocus( wxFocusEvent& aEvent )
{
    // Parse and reformat the value on focus loss
    wxString input = getTextValue();
    std::optional<int> offset;
    std::optional<double> ratio;

    if( parseInput( input, offset, ratio ) )
    {
        m_cachedOffset = offset;
        m_cachedRatio = ratio;
        m_needsParsing = false;
        setTextValue( formatValue( m_cachedOffset, m_cachedRatio ) );
    }

    aEvent.Skip();
}


void MARGIN_OFFSET_BINDER::onUnitsChanged( wxCommandEvent& aEvent )
{
    EDA_BASE_FRAME* provider = static_cast<EDA_BASE_FRAME*>( aEvent.GetClientData() );

    // Preserve current values
    std::optional<int> offset = GetOffsetValue();
    std::optional<double> ratio = GetRatioValue();

    // Update units
    m_units = provider->GetUserUnits();
    m_iuScale = &provider->GetIuScale();
    m_eval.SetDefaultUnits( m_units );

    // Reformat with new units
    setTextValue( formatValue( offset, ratio ) );

    aEvent.Skip();
}


bool MARGIN_OFFSET_BINDER::parseInput( const wxString& aInput, std::optional<int>& aOffset,
                                        std::optional<double>& aRatio ) const
{
    aOffset = std::nullopt;
    aRatio = std::nullopt;

    wxString input = aInput;
    input.Trim( true ).Trim( false );

    if( input.IsEmpty() )
        return true;

    // Strategy: find all terms separated by + and - operators
    // Each term is either a distance value (with optional unit) or a percentage

    // First, normalize the input by adding spaces around operators
    wxString normalized;
    bool lastWasOperator = true;  // Start as true to handle leading negative

    for( size_t i = 0; i < input.Length(); ++i )
    {
        wxChar ch = input[i];

        if( ( ch == '+' || ch == '-' ) && !lastWasOperator )
        {
            // This is an operator between terms
            normalized += ' ';
            normalized += ch;
            normalized += ' ';
            lastWasOperator = true;
        }
        else
        {
            normalized += ch;
            lastWasOperator = ( ch == '+' || ch == '-' );
        }
    }

    // Split into terms
    wxArrayString terms;
    wxString currentTerm;

    for( size_t i = 0; i < normalized.Length(); ++i )
    {
        wxChar ch = normalized[i];

        if( ch == ' ' )
        {
            if( !currentTerm.IsEmpty() )
            {
                currentTerm.Trim( true ).Trim( false );
                terms.Add( currentTerm );
                currentTerm.Clear();
            }
        }
        else
        {
            currentTerm += ch;
        }
    }

    if( !currentTerm.IsEmpty() )
    {
        currentTerm.Trim( true ).Trim( false );
        terms.Add( currentTerm );
    }

    // Process each term
    double totalOffset = 0.0;
    double totalRatio = 0.0;
    bool hasOffset = false;
    bool hasRatio = false;
    double sign = 1.0;

    for( const wxString& term : terms )
    {
        if( term == "+" )
        {
            sign = 1.0;
            continue;
        }
        else if( term == "-" )
        {
            sign = -1.0;
            continue;
        }

        // Check if this term is a percentage
        if( term.EndsWith( "%" ) )
        {
            wxString numPart = term.Left( term.Length() - 1 );
            numPart.Trim( true ).Trim( false );
            double value;

            if( numPart.ToDouble( &value ) )
            {
                totalRatio += sign * value / 100.0;
                hasRatio = true;
            }
        }
        else
        {
            // Try to parse as a distance value using the evaluator
            NUMERIC_EVALUATOR eval( m_units );

            if( eval.Process( term ) )
            {
                wxString result = eval.Result();
                double value;

                if( result.ToDouble( &value ) )
                {
                    // Convert from user units to internal units
                    int iuValue = EDA_UNIT_UTILS::UI::ValueFromString( *m_iuScale, m_units, term );
                    totalOffset += sign * iuValue;
                    hasOffset = true;
                }
            }
        }

        sign = 1.0;  // Reset sign for next term
    }

    if( hasOffset )
        aOffset = static_cast<int>( totalOffset );

    if( hasRatio )
        aRatio = totalRatio;

    return hasOffset || hasRatio || input.IsEmpty();
}


wxString MARGIN_OFFSET_BINDER::formatValue( std::optional<int> aOffset,
                                             std::optional<double> aRatio ) const
{
    wxString result;

    // Format offset value
    if( aOffset.has_value() && aOffset.value() != 0 )
    {
        result = EDA_UNIT_UTILS::UI::StringFromValue( *m_iuScale, m_units, aOffset.value() );
    }

    // Format ratio value
    if( aRatio.has_value() && std::abs( aRatio.value() ) > 1e-9 )
    {
        double percent = aRatio.value() * 100.0;

        if( !result.IsEmpty() )
        {
            // Add the ratio with appropriate sign
            if( percent >= 0 )
                result += wxString::Format( " + %.4g%%", percent );
            else
                result += wxString::Format( " - %.4g%%", -percent );
        }
        else
        {
            result = wxString::Format( "%.4g%%", percent );
        }
    }

    return result;
}


wxString MARGIN_OFFSET_BINDER::getTextValue() const
{
    wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( m_valueCtrl );

    if( textEntry )
        return textEntry->GetValue();

    return wxEmptyString;
}


void MARGIN_OFFSET_BINDER::setTextValue( const wxString& aValue )
{
    wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( m_valueCtrl );

    if( textEntry )
        textEntry->ChangeValue( aValue );
}
