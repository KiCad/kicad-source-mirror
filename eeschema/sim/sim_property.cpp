/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


/**
 *
 * wxPropertyGrid property specializations for simulator.
 *
 */


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


SIM_BOOL_PROPERTY::SIM_BOOL_PROPERTY( const wxString& aLabel, const wxString& aName,
                                      SIM_MODEL& aModel, int aParamIndex ) :
        wxBoolProperty( aLabel, aName ),
        SIM_PROPERTY( aModel, aParamIndex )
{
    std::string value = m_model.GetParam( m_paramIndex ).value;
    SetValue( value == "1" );
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

    m_model.SetParamValue( m_paramIndex, m_value.GetBool() ? "1" : "0" );
}


SIM_STRING_PROPERTY::SIM_STRING_PROPERTY( const wxString& aLabel, const wxString& aName,
                                          SIM_MODEL& aModel, int aParamIndex,
                                          SIM_VALUE::TYPE aValueType,
                                          SIM_VALUE_GRAMMAR::NOTATION aNotation ) :
        wxStringProperty( aLabel, aName ),
        SIM_PROPERTY( aModel, aParamIndex ),
        m_valueType( aValueType )
{
    SetValueFromString( GetParam().value );
}


bool SIM_STRING_PROPERTY::OnEvent( wxPropertyGrid* propgrid, wxWindow* wnd_primary, wxEvent& event )
{
    if( event.GetEventType() == wxEVT_SET_FOCUS && allowEval() )
    {
        if( wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( wnd_primary ) )
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
    else if( event.GetEventType() == wxEVT_KILL_FOCUS && allowEval() )
    {
        if( wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( wnd_primary ) )
        {
            wxString strValue = textEntry->GetValue();

            if( !strValue.IsEmpty() && m_eval.Process( strValue ) )
            {
                double value = SIM_VALUE::ToDouble( m_eval.Result().ToStdString() );

                if( std::isnan( value ) || SIM_VALUE::Equal( value, strValue.ToStdString() ) )
                {
                    // Don't mess up user formatting if eval'ing didn't actually change the value.
                }
                else
                {
                    SetValueInEvent( m_eval.Result() );
                }
            }

            m_needsEval = false;
            return true;
        }
    }
    else if( event.GetEventType() == wxEVT_KEY_DOWN )
    {
        wxKeyEvent& keyEvent = dynamic_cast<wxKeyEvent&>( event );

        if( wxPropertyGrid* propGrid = dynamic_cast<wxPropertyGrid*>( wnd_primary->GetParent() ) )
        {
            if( keyEvent.GetKeyCode() == WXK_TAB )
            {
                propGrid->CommitChangesFromEditor();

                keyEvent.m_keyCode = keyEvent.ShiftDown() ? WXK_UP : WXK_DOWN;
                keyEvent.m_shiftDown = false;
            }

#ifdef __WXMAC__
            // This shouldn't be required, as wxPGTextCtrlEditor::OnTextCtrlEvent() should be
            // setting the value to modified.  But it doesn't on Mac (or the modified flag is
            // cleared at some point later), and even if it is set, the changes don't get
            // committed.
            // (We used to have code in DIALOG_SIM_MODEL to commit things on *some* actions, but
            // it wasn't complete and this appears to have at least a better hit rate.)
            propGrid->CallAfter(
                    [propGrid]()
                    {
                        propGrid->EditorsValueWasModified();
                        propGrid->CommitChangesFromEditor();
                    } );
#endif
        }
    }

    return false;
}


wxValidator* SIM_STRING_PROPERTY::DoGetValidator() const
{
    return new SIM_VALIDATOR();
}


bool SIM_STRING_PROPERTY::allowEval() const
{
    return m_valueType == SIM_VALUE::TYPE_INT
            || m_valueType == SIM_VALUE::TYPE_FLOAT;
}


bool SIM_STRING_PROPERTY::StringToValue( wxVariant& aVariant, const wxString& aText,
                                         int aArgFlags ) const
{
    if( m_disabled )
        return false;

    wxString text = aText;

    if( allowEval() && m_needsEval && m_eval.Process( aText ) )
    {
        if( !aText.IsEmpty() )
        {
            double value = SIM_VALUE::ToDouble( m_eval.Result().ToStdString() );

            if( std::isnan( value ) || SIM_VALUE::Equal( value, aText.ToStdString() ) )
            {
                // Don't mess up user formatting if eval'ing didn't actually change the value.
            }
            else
            {
                text = SIM_VALUE::Normalize( value );
            }
        }
    }

    m_model.SetParamValue( m_paramIndex, text.ToStdString() );
    aVariant = text.ToStdString();
    return true;
}


SIM_ENUM_PROPERTY::SIM_ENUM_PROPERTY( const wxString& aLabel, const wxString& aName,
                                      SIM_MODEL& aModel, int aParamIndex,
                                      const wxArrayString& aValues ) :
        wxEnumProperty( aLabel, aName, aValues ),
        SIM_PROPERTY( aModel, aParamIndex )
{
    for( int ii = 0; ii < (int) GetParam().info.enumValues.size(); ++ii )
    {
        if( GetParam().info.enumValues[ii] == GetParam().value )
        {
            SetValue( ii );
            return;
        }
    }

    SetValue( -1 );
}


#if wxCHECK_VERSION( 3, 3, 0 )
bool SIM_ENUM_PROPERTY::IntToValue( wxVariant& aVariant, int aNumber,
                                    wxPGPropValFormatFlags aArgFlags ) const
#else
bool SIM_ENUM_PROPERTY::IntToValue( wxVariant& aVariant, int aNumber, int aArgFlags ) const
#endif
{
    if( m_disabled )
        return false;

    m_model.SetParamValue( m_paramIndex, m_choices.GetLabel( aNumber ).ToStdString() );
    return wxEnumProperty::IntToValue( aVariant, aNumber, aArgFlags );
}
