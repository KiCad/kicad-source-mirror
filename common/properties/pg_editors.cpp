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
#include <dialogs/dialog_color_picker.h>
#include <properties/eda_angle_variant.h>
#include <properties/pg_editors.h>
#include <properties/pg_properties.h>
#include <widgets/color_swatch.h>
#include <widgets/unit_binder.h>

#include <wx/log.h>

const wxString PG_UNIT_EDITOR::EDITOR_NAME = wxS( "KiCadUnitEditor" );
const wxString PG_CHECKBOX_EDITOR::EDITOR_NAME = wxS( "KiCadCheckboxEditor" );
const wxString PG_COLOR_EDITOR::EDITOR_NAME = wxS( "KiCadColorEditor" );


PG_UNIT_EDITOR::PG_UNIT_EDITOR( EDA_DRAW_FRAME* aFrame ) :
        wxPGTextCtrlEditor(),
        m_frame( aFrame )
{
    m_unitBinder = std::make_unique<PROPERTY_EDITOR_UNIT_BINDER>( m_frame );
    m_unitBinder->SetUnits( m_frame->GetUserUnits() );

    m_editorName = BuildEditorName( m_frame );
}


PG_UNIT_EDITOR::~PG_UNIT_EDITOR()
{
}


wxString PG_UNIT_EDITOR::BuildEditorName( EDA_DRAW_FRAME* aFrame )
{
    return EDITOR_NAME + aFrame->GetName();
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
    wxWindow* win = aPropGrid->GenerateEditorTextCtrl( aPos, aSize, text, nullptr, 0,
                                                       aProperty->GetMaxLength() );
    wxPGWindowList ret( win, nullptr );

    m_unitBinder->SetControl( win );
    m_unitBinder->RequireEval();
    m_unitBinder->SetUnits( m_frame->GetUserUnits() );

    if( PGPROPERTY_DISTANCE* prop = dynamic_cast<PGPROPERTY_DISTANCE*>( aProperty ) )
    {
        m_unitBinder->SetCoordType( prop->CoordType() );
    }
    else if( dynamic_cast<PGPROPERTY_ANGLE*>( aProperty ) != nullptr )
    {
        m_unitBinder->SetCoordType( ORIGIN_TRANSFORMS::NOT_A_COORD );
        m_unitBinder->SetUnits( EDA_UNITS::DEGREES );
    }

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
    else if( var.GetType() == wxT( "EDA_ANGLE" ) )
    {
        EDA_ANGLE_VARIANT_DATA* angleData = static_cast<EDA_ANGLE_VARIANT_DATA*>( var.GetData() );
        m_unitBinder->ChangeAngleValue( angleData->Angle() );
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
    bool changed;

    if( dynamic_cast<PGPROPERTY_ANGLE*>( aProperty ) != nullptr )
    {
        EDA_ANGLE angle = m_unitBinder->GetAngleValue();

        if( aVariant.GetType() == wxT( "EDA_ANGLE" ) )
        {
            EDA_ANGLE_VARIANT_DATA* ad = static_cast<EDA_ANGLE_VARIANT_DATA*>( aVariant.GetData() );
            changed = ( aVariant.IsNull() || angle != ad->Angle() );

            if( changed )
            {
                ad->SetAngle( angle );
                m_unitBinder->SetAngleValue( angle );
            }
        }
        else
        {
            changed = ( aVariant.IsNull() || angle.AsDegrees() != aVariant.GetDouble() );

            if( changed )
            {
                aVariant = angle.AsDegrees();
                m_unitBinder->SetValue( angle.AsDegrees() );
            }
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


PG_CHECKBOX_EDITOR::PG_CHECKBOX_EDITOR() :
        wxPGCheckBoxEditor()
{
}


wxPGWindowList PG_CHECKBOX_EDITOR::CreateControls( wxPropertyGrid* aGrid, wxPGProperty* aProperty,
                                                   const wxPoint& aPos, const wxSize& aSize ) const
{
    // Override wx behavior and toggle unspecified checkboxes to "true"
    // CreateControls for a checkbox editor is only triggered when the user activates the checkbox
    // Set the value to false here; the base class will then trigger an event setting it true.
    if( aProperty->IsValueUnspecified() )
        aProperty->SetValueFromInt( 0 );

    return wxPGCheckBoxEditor::CreateControls( aGrid, aProperty, aPos, aSize );
}


bool PG_COLOR_EDITOR::OnEvent( wxPropertyGrid* aGrid, wxPGProperty* aProperty, wxWindow* aWindow,
                               wxEvent& aEvent ) const
{
    return false;
}


wxPGWindowList PG_COLOR_EDITOR::CreateControls( wxPropertyGrid* aGrid, wxPGProperty* aProperty,
                                                const wxPoint& aPos, const wxSize& aSize ) const
{
    auto colorProp = dynamic_cast<PGPROPERTY_COLOR4D*>( aProperty );

    if( !colorProp )
        return nullptr;

    KIGFX::COLOR4D color    = colorFromProperty( aProperty );
    KIGFX::COLOR4D defColor = colorFromVariant( colorProp->GetDefaultValue() );

    COLOR_SWATCH* editor = new COLOR_SWATCH( aGrid->GetPanel(), color, wxID_ANY,
                                             colorProp->GetBackgroundColor(), defColor,
                                             SWATCH_LARGE, true );
    editor->SetPosition( aPos );
    editor->SetSize( aSize );

    editor->Bind( COLOR_SWATCH_CHANGED,
                  [=]( wxCommandEvent& aEvt )
                  {
                      wxVariant val;
                      auto data = new COLOR4D_VARIANT_DATA( editor->GetSwatchColor() );
                      val.SetData( data );
                      aGrid->ChangePropertyValue( colorProp, val );
                  } );

    if( aGrid->GetInternalFlags() & wxPG_FL_ACTIVATION_BY_CLICK )
        aGrid->CallAfter( [=]() { editor->GetNewSwatchColor(); } );

    return editor;
}


void PG_COLOR_EDITOR::UpdateControl( wxPGProperty* aProperty, wxWindow* aCtrl ) const
{
    if( auto swatch = dynamic_cast<COLOR_SWATCH*>( aCtrl ) )
        swatch->SetSwatchColor( colorFromProperty( aProperty ), false );
}


KIGFX::COLOR4D PG_COLOR_EDITOR::colorFromVariant( const wxVariant& aVariant ) const
{
    KIGFX::COLOR4D color = KIGFX::COLOR4D::UNSPECIFIED;
    COLOR4D_VARIANT_DATA* data = nullptr;

    if( aVariant.IsType( wxS( "COLOR4D" ) ) )
    {
        data = static_cast<COLOR4D_VARIANT_DATA*>( aVariant.GetData() );
        color = data->Color();
    }

    return color;
}


KIGFX::COLOR4D PG_COLOR_EDITOR::colorFromProperty( wxPGProperty* aProperty ) const
{
    return colorFromVariant( aProperty->GetValue() );
}
