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

#include <sim/sim_property.h>
#include <sim/sim_value.h>
#include <ki_exception.h>
#include <confirm.h>
#include <wx/combo.h>
#include <wx/combobox.h>
#include <wx/notebook.h>


wxBEGIN_EVENT_TABLE( SIM_VALIDATOR, wxValidator )
    EVT_KEY_DOWN( SIM_VALIDATOR::onKeyDown )
wxEND_EVENT_TABLE()


void SIM_VALIDATOR::navigate( int flags )
{
    wxWindow* textCtrl = GetWindow();
    if( !textCtrl )
        return;

    wxPropertyGrid* paramGrid = dynamic_cast<wxPropertyGrid*>( textCtrl->GetParent() );
    if( !paramGrid )
        return;

    wxPropertyGridManager* paramGridMgr =
        dynamic_cast<wxPropertyGridManager*>( paramGrid->GetParent() );
    if( !paramGridMgr )
        return;

#ifdef __WXGTK__
    // Workaround for wxWindow::Navigate() working differently on GTK. Same workaround is
    // in the WxWidgets source code.
    if( flags == wxNavigationKeyEvent::IsBackward )
    {
        if( wxWindow* sibling = paramGridMgr->GetPrevSibling() )
        {
            sibling->SetFocusFromKbd();
            return;
        }
    }
    else if( flags == wxNavigationKeyEvent::IsForward )
    {
        if( wxWindow* sibling = paramGridMgr->GetNextSibling() )
        {
            sibling->SetFocusFromKbd();
            return;
        }
    }

    // We didn't find the sibling, so instead we try another workaround by by finding the notebook
    // we are in, and jumping out of it.
    for( wxWindow* window = paramGridMgr; window; window = window->GetParent() )
    {
        if( wxNotebook* notebook = dynamic_cast<wxNotebook*>( window ) )
        {
            if( flags == wxNavigationKeyEvent::IsBackward )
            {
                for( wxWindow* sibling = notebook->GetNextSibling();
                     sibling;
                     sibling = sibling->GetNextSibling() )
                {
                    if( sibling->IsFocusable() )
                    {
                        sibling->SetFocusFromKbd();
                        return;
                    }
                }
            }
            else if( flags == wxNavigationKeyEvent::IsForward )
            {
                for( wxWindow* sibling = notebook->GetNextSibling();
                     sibling;
                     sibling = sibling->GetNextSibling() )
                {
                    if( sibling->IsFocusable() )
                    {
                        sibling->SetFocusFromKbd();
                        return;
                    }
                }
            }
        }
    }

#else
    paramGridMgr->Navigate( flags );
#endif
}


void SIM_VALIDATOR::onKeyDown( wxKeyEvent& aEvent )
{
    // Because wxPropertyGrid has special handling for the tab key, wxPropertyGrid::DedicateKey()
    // and wxPropertyGrid::AddActionTrigger() don't work for it. So instead we translate it to an
    // (up or down) arrow key, which has proper handling (select next or previous property) defined
    // by the aforementioned functions.
    
    if( aEvent.GetKeyCode() == WXK_TAB )
    {
        // However, before that, if this is the first or last property, we instead want to navigate
        // to the next or previous widget.
        wxWindow* textCtrl = GetWindow();
        if( !textCtrl )
        {
            aEvent.Skip();
            return;
        }

        wxPropertyGrid* paramGrid = dynamic_cast<wxPropertyGrid*>( textCtrl->GetParent() );
        if( !paramGrid )
        {
            aEvent.Skip();
            return;
        }

        wxPropertyGridIterator it = paramGrid->GetIterator( wxPG_ITERATE_VISIBLE,
                                                            paramGrid->GetSelection() );
        if( !it.AtEnd() )
            it.Next();

        bool isFirst = paramGrid->GetSelection() == paramGrid->wxPropertyGridInterface::GetFirst();
        bool isLast = it.AtEnd();

        if( isFirst && aEvent.ShiftDown() )
        {
            navigate( wxNavigationKeyEvent::IsBackward );
            return;
        }
        
        if( isLast && !aEvent.ShiftDown() )
        {
            navigate( wxNavigationKeyEvent::IsForward );
            return;
        }

        if( aEvent.GetModifiers() == wxMOD_SHIFT )
        {
            aEvent.m_shiftDown = false;
            aEvent.m_keyCode = WXK_UP;
        }
        else
            aEvent.m_keyCode = WXK_DOWN;
    }

    aEvent.Skip();
}


wxBEGIN_EVENT_TABLE( SIM_BOOL_VALIDATOR, SIM_VALIDATOR )
wxEND_EVENT_TABLE()


bool SIM_BOOL_VALIDATOR::Validate( wxWindow* aParent )
{
    return true;
}


wxBEGIN_EVENT_TABLE( SIM_STRING_VALIDATOR, SIM_VALIDATOR )
    EVT_TEXT( wxID_ANY, SIM_STRING_VALIDATOR::onText )
    EVT_CHAR( SIM_STRING_VALIDATOR::onChar )
    EVT_MOUSE_EVENTS( SIM_STRING_VALIDATOR::onMouse )
wxEND_EVENT_TABLE()


SIM_STRING_VALIDATOR::SIM_STRING_VALIDATOR( SIM_VALUE::TYPE aValueType,
                                            SIM_VALUE_GRAMMAR::NOTATION aNotation )
    : SIM_VALIDATOR(),
      m_valueType( aValueType ),
      m_notation( aNotation )
{
    wxTextEntry* textEntry = getTextEntry();
    if( !textEntry )
        return;

    m_prevText = textEntry->GetValue();
    m_prevInsertionPoint = textEntry->GetInsertionPoint();
}


wxObject* SIM_STRING_VALIDATOR::Clone() const
{
    return new SIM_STRING_VALIDATOR( *this );
}


bool SIM_STRING_VALIDATOR::Validate( wxWindow* aParent )
{
    if( !m_validatorWindow->IsEnabled() )
        return true;

    wxTextEntry* const textEntry = getTextEntry();
    if( !textEntry )
        return false;

    return isValid( textEntry->GetValue() );
}


bool SIM_STRING_VALIDATOR::TransferToWindow()
{
    return true;
}


bool SIM_STRING_VALIDATOR::TransferFromWindow()
{
    return true;
}


bool SIM_STRING_VALIDATOR::isValid( const wxString& aString )
{
    return SIM_VALUE_GRAMMAR::IsValid( aString, m_valueType, m_notation );
}


wxTextEntry* SIM_STRING_VALIDATOR::getTextEntry()
{
    if( !m_validatorWindow )
        return nullptr;

    // Taken from wxTextValidator.

    if( wxDynamicCast( m_validatorWindow, wxTextCtrl ) )
        return ( wxTextCtrl* ) m_validatorWindow;

    if( wxDynamicCast( m_validatorWindow, wxComboBox ) )
        return ( wxComboBox* ) m_validatorWindow;

    if( wxDynamicCast( m_validatorWindow, wxComboCtrl ) )
        return ( wxComboCtrl* ) m_validatorWindow;

    wxFAIL_MSG(
        "SIM_STRING_VALIDATOR can only be used with wxTextCtrl, wxComboBox, or wxComboCtrl"
    );

    return nullptr;
}


void SIM_STRING_VALIDATOR::onText( wxCommandEvent& aEvent )
{
    wxTextEntry* textEntry = getTextEntry();
    if( !textEntry )
        return;

    if( !isValid( textEntry->GetValue() ) )
    {
        textEntry->ChangeValue( m_prevText );
        textEntry->SetInsertionPoint( m_prevInsertionPoint );
    }

    m_prevText = textEntry->GetValue();
    m_prevInsertionPoint = textEntry->GetInsertionPoint();
}


void SIM_STRING_VALIDATOR::onChar( wxKeyEvent& aEvent )
{
    aEvent.Skip();

    wxTextEntry* textEntry = getTextEntry();
    if( !textEntry )
        return;

    m_prevInsertionPoint = textEntry->GetInsertionPoint();
}


void SIM_STRING_VALIDATOR::onMouse( wxMouseEvent& aEvent )
{
    aEvent.Skip();

    wxTextEntry* textEntry = getTextEntry();
    if( !textEntry )
        return;

    m_prevInsertionPoint = textEntry->GetInsertionPoint();
}


SIM_BOOL_PROPERTY::SIM_BOOL_PROPERTY( const wxString& aLabel, const wxString& aName,
                                      std::shared_ptr<SIM_LIBRARY> aLibrary,
                                      std::shared_ptr<SIM_MODEL> aModel,
                                      int aParamIndex )
    : wxBoolProperty( aLabel, aName ),
      m_library( aLibrary ),
      m_model( aModel ),
      m_paramIndex( aParamIndex )
{
    auto simValue = dynamic_cast<SIM_VALUE_INST<bool>*>(
        m_model->GetParam( m_paramIndex ).value.get() );

    wxCHECK( simValue, /*void*/ );

    SetValue( *simValue == true );
}


wxValidator* SIM_BOOL_PROPERTY::DoGetValidator() const
{
    return new SIM_BOOL_VALIDATOR();
}


void SIM_BOOL_PROPERTY::OnSetValue()
{
    wxPGProperty::OnSetValue();

    auto simValue = dynamic_cast<SIM_VALUE_INST<bool>*>(
        m_model->GetParam( m_paramIndex ).value.get() );

    wxCHECK( simValue, /*void*/ );

    if( m_model->GetBaseModel() && *simValue == m_value.GetBool() )
        m_model->SetParamValue( m_paramIndex, "" );
    else
        m_model->SetParamValue( m_paramIndex, m_value.GetBool() ? "1" : "0" );
}


SIM_STRING_PROPERTY::SIM_STRING_PROPERTY( const wxString& aLabel, const wxString& aName,
                                          std::shared_ptr<SIM_LIBRARY> aLibrary,
                                          std::shared_ptr<SIM_MODEL> aModel,
                                          int aParamIndex,
                                          SIM_VALUE::TYPE aValueType,
                                          SIM_VALUE_GRAMMAR::NOTATION aNotation )
    : wxStringProperty( aLabel, aName ),
      m_valueType( aValueType ),
      m_notation( aNotation ),
      m_library( aLibrary ),
      m_model( aModel ),
      m_paramIndex( aParamIndex )
{
    SetValueFromString( GetParam().value->ToString() );
}


wxValidator* SIM_STRING_PROPERTY::DoGetValidator() const
{
    return new SIM_STRING_VALIDATOR( m_valueType, m_notation );
}


bool SIM_STRING_PROPERTY::StringToValue( wxVariant& aVariant, const wxString& aText, int aArgFlags ) const
{
    wxString paramValueStr = m_model->GetBaseParam( m_paramIndex ).value->ToString();
    aVariant = aText;

    // TODO: Don't use string comparison.
    if( m_model->GetBaseModel() && ( aText == "" || aText == paramValueStr ) )
    {
        if( !m_model->SetParamValue( m_paramIndex, "" ) ) // Nullify.
            return false;

        aVariant = paramValueStr; // Use the inherited value (if it exists) if null.
    }
    else
    {
        m_model->SetParamValue( m_paramIndex, aText );
        aVariant = GetParam().value->ToString();
    }

    return true;
}
