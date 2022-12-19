/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <wx/combo.h>

// Include simulator headers after wxWidgets headers to avoid conflicts with Windows headers
// (especially on msys2 + wxWidgets 3.0.x)
#include <sim/sim_property.h>
#include <sim/sim_value.h>
#include <ki_exception.h>


SIM_PROPERTY::SIM_PROPERTY( SIM_MODEL& aModel, int aParamIndex ) :
        m_model( aModel ),
        m_paramIndex( aParamIndex ),
        m_needsEval( false ),
        m_eval( EDA_UNITS::UNSCALED ),
        m_disabled( false )
{
}


void SIM_PROPERTY::Disable()
{
    m_disabled = true;
}


SIM_BOOL_PROPERTY::SIM_BOOL_PROPERTY( const wxString& aLabel, const wxString& aName, SIM_MODEL& aModel,
                                      int aParamIndex ) :
        wxBoolProperty( aLabel, aName ),
        SIM_PROPERTY( aModel, aParamIndex )
{
    auto simValue = dynamic_cast<SIM_VALUE_INST<bool>*>( m_model.GetParam( m_paramIndex ).value.get() );

    wxCHECK( simValue, /*void*/ );

    SetValue( *simValue == true );
}


wxValidator* SIM_BOOL_PROPERTY::DoGetValidator() const
{
    return new SIM_VALIDATOR();
}


void SIM_BOOL_PROPERTY::OnSetValue()
{
    wxPGProperty::OnSetValue();

    if( m_disabled )
        return;

    auto simValue = dynamic_cast<SIM_VALUE_INST<bool>*>( m_model.GetParam( m_paramIndex ).value.get() );

    wxCHECK( simValue, /*void*/ );

    if( m_model.GetBaseModel() && *simValue == m_value.GetBool() )
        m_model.SetParamValue( m_paramIndex, "" );
    else
        m_model.SetParamValue( m_paramIndex, m_value.GetBool() ? "1" : "0" );
}


SIM_STRING_PROPERTY::SIM_STRING_PROPERTY( const wxString& aLabel, const wxString& aName, SIM_MODEL& aModel,
                                          int aParamIndex, SIM_VALUE::TYPE aValueType,
                                          SIM_VALUE_GRAMMAR::NOTATION aNotation ) :
        wxStringProperty( aLabel, aName ),
        SIM_PROPERTY( aModel, aParamIndex ),
        m_valueType( aValueType ),
        m_notation( aNotation )
{
    SetValueFromString( GetParam().value->ToString() );
}


bool SIM_STRING_PROPERTY::OnEvent( wxPropertyGrid* propgrid, wxWindow* wnd_primary, wxEvent& event )
{
    if( event.GetEventType() == wxEVT_SET_FOCUS )
    {
        wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( wnd_primary );

        if( textEntry )
        {
            wxString oldStr = m_eval.OriginalText();

            if( oldStr.length() && oldStr != textEntry->GetValue() )
            {
                SetValueInEvent( oldStr );
                textEntry->SetValue( oldStr );
            }

            m_needsEval = true;
            return true;
        }
    }
    else if( event.GetEventType() == wxEVT_KILL_FOCUS )
    {
        wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( wnd_primary );

        if( textEntry && m_eval.Process( textEntry->GetValue() ) )
        {
            SetValueInEvent( m_eval.Result() );
            m_needsEval = false;
            return true;
        }
    }
    else if( event.GetEventType() == wxEVT_KEY_DOWN )
    {
        wxKeyEvent&     keyEvent = dynamic_cast<wxKeyEvent&>( event );
        wxPropertyGrid* propGrid = dynamic_cast<wxPropertyGrid*>( wnd_primary->GetParent() );

        if( propGrid )
        {
            if( keyEvent.GetKeyCode() == WXK_TAB )
            {
                propGrid->CommitChangesFromEditor();

                keyEvent.m_keyCode = keyEvent.ShiftDown() ? WXK_UP : WXK_DOWN;
                keyEvent.m_shiftDown = false;
            }
        }
    }

    return false;
}


wxValidator* SIM_STRING_PROPERTY::DoGetValidator() const
{
    return new SIM_VALIDATOR();
}


bool SIM_STRING_PROPERTY::StringToValue( wxVariant& aVariant, const wxString& aText, int aArgFlags ) const
{
    if( m_disabled )
        return false;

    wxString evaledText;

    if( m_needsEval && m_eval.Process( aText ) )
        evaledText = m_eval.Result();
    else
        evaledText = aText;

    m_model.SetParamValue( m_paramIndex, std::string( evaledText.ToUTF8() ) );
    aVariant = GetParam().value->ToString();

    return true;
}


static wxArrayString convertStringsToWx( const std::vector<std::string>& aStrings )
{
    wxArrayString result;

    for( const std::string& string : aStrings )
        result.Add( string );

    return result;
}


SIM_ENUM_PROPERTY::SIM_ENUM_PROPERTY( const wxString& aLabel, const wxString& aName, SIM_MODEL& aModel,
                                      int aParamIndex ) :
        wxEnumProperty( aLabel, aName, convertStringsToWx( aModel.GetParam( aParamIndex ).info.enumValues ) ),
        SIM_PROPERTY( aModel, aParamIndex )
{
    auto it = std::find( GetParam().info.enumValues.begin(), GetParam().info.enumValues.end(),
                         GetParam().value->ToString() );

    // we need the force cast for msvc because wxVariant lacks 64-bit methods due to `long`
    SetValue( static_cast<int>( std::distance( GetParam().info.enumValues.begin(), it ) ) );
}


bool SIM_ENUM_PROPERTY::IntToValue( wxVariant& aVariant, int aNumber, int aArgFlags ) const
{
    if( m_disabled )
        return false;

    m_model.SetParamValue( m_paramIndex, GetParam().info.enumValues.at( aNumber ) );
    return wxEnumProperty::IntToValue( aVariant, aNumber, aArgFlags );
}
