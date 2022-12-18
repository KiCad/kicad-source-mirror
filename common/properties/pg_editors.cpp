/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <eda_draw_frame.h>
#include <properties/eda_angle_variant.h>
#include <properties/pg_editors.h>
#include <properties/pg_properties.h>
#include <widgets/unit_binder.h>

#include <wx/log.h>

const wxString PG_UNIT_EDITOR::EDITOR_NAME = wxS( "KiCadUnitEditor" );


PG_UNIT_EDITOR::PG_UNIT_EDITOR( EDA_DRAW_FRAME* aFrame ) :
        wxPGTextCtrlEditor(),
        m_frame( aFrame )
{
    m_unitBinder = std::make_unique<PROPERTY_EDITOR_UNIT_BINDER>( m_frame );
    m_unitBinder->SetUnits( m_frame->GetUserUnits() );
}


PG_UNIT_EDITOR::~PG_UNIT_EDITOR()
{
}


void PG_UNIT_EDITOR::UpdateFrame( EDA_DRAW_FRAME* aFrame )
{
    m_frame = aFrame;

    if( aFrame )
    {
        m_unitBinder = std::make_unique<PROPERTY_EDITOR_UNIT_BINDER>( m_frame );
        m_unitBinder->SetUnits( m_frame->GetUserUnits() );
    }
    else
    {
        m_unitBinder = nullptr;
    }
}


wxPGWindowList PG_UNIT_EDITOR::CreateControls( wxPropertyGrid* aPropGrid, wxPGProperty* aProperty,
                                               const wxPoint& aPos, const wxSize& aSize ) const
{
    wxASSERT( m_unitBinder );

    wxString text = aProperty->GetValueAsString( wxPG_EDITABLE_VALUE );
    wxWindow* win = aPropGrid->GenerateEditorTextCtrl(aPos, aSize, text, nullptr, 0, aProperty->GetMaxLength() );
    wxPGWindowList ret( win, nullptr );

    m_unitBinder->SetControl( win );
    m_unitBinder->RequireEval();
    m_unitBinder->SetUnits( m_frame->GetUserUnits() );

    if( PGPROPERTY_DISTANCE* prop = dynamic_cast<PGPROPERTY_DISTANCE*>( aProperty ) )
        m_unitBinder->SetCoordType( prop->CoordType() );
    else if( dynamic_cast<PGPROPERTY_ANGLE*>( aProperty ) )
        m_unitBinder->SetUnits( EDA_UNITS::DEGREES );

    UpdateControl( aProperty, win );

    return ret;
}


void PG_UNIT_EDITOR::UpdateControl( wxPGProperty* aProperty, wxWindow* aCtrl ) const
{
    wxVariant var = aProperty->GetValue();

    if( var.GetType() == wxPG_VARIANT_TYPE_LONG )
    {
        m_unitBinder->ChangeValue( var.GetLong() );
    }
    else if( var.GetType() == wxPG_VARIANT_TYPE_DOUBLE )
    {
        m_unitBinder->ChangeValue( var.GetDouble() );
    }
    else if( !aProperty->IsValueUnspecified() )
    {
        wxFAIL_MSG( wxT( "PG_UNIT_EDITOR should only be used with numeric properties!" ) );
    }
}


bool PG_UNIT_EDITOR::OnEvent( wxPropertyGrid* aPropGrid, wxPGProperty* aProperty,
                              wxWindow* aCtrl, wxEvent& aEvent ) const
{
    if( aEvent.GetEventType() == wxEVT_LEFT_UP )
    {
        if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( aCtrl ) )
        {
            if( !textCtrl->HasFocus() )
            {
                textCtrl->SelectAll();
                return false;
            }
        }
    }

    return wxPGTextCtrlEditor::OnEvent( aPropGrid, aProperty, aCtrl, aEvent );
}


bool PG_UNIT_EDITOR::GetValueFromControl( wxVariant& aVariant, wxPGProperty* aProperty,
                                          wxWindow* aCtrl ) const
{
    if( !m_unitBinder )
        return false;

    wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( aCtrl );
    wxCHECK_MSG( textCtrl, false, "PG_UNIT_EDITOR requires a text control!" );
    wxString textVal = textCtrl->GetValue();

    if( aProperty->UsesAutoUnspecified() && textVal.empty() )
    {
        aVariant.MakeNull();
        return true;
    }
    bool changed = false;

    if( dynamic_cast<PGPROPERTY_ANGLE*>( aProperty ) )
    {
        double result = m_unitBinder->GetAngleValue().AsDegrees();
        changed = ( aVariant.IsNull() || result != aVariant.GetDouble() );

        if( changed )
        {
            aVariant = result;
            m_unitBinder->SetValue( result );
        }
    }
    else
    {
        long result = m_unitBinder->GetValue();
        changed = ( aVariant.IsNull() || result != aVariant.GetLong() );

        if( changed )
        {
            aVariant = result;
            m_unitBinder->SetValue( result );
        }
    }

    // Changing unspecified always causes event (returning
    // true here should be enough to trigger it).
    if( !changed && aVariant.IsNull() )
        changed = true;

    return changed;
}
