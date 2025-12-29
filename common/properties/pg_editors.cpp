/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <properties/std_optional_variants.h>
#include <properties/eda_angle_variant.h>
#include <properties/pg_editors.h>
#include <properties/pg_properties.h>
#include <widgets/color_swatch.h>
#include <widgets/unit_binder.h>
#include <bitmaps.h>
#include <frame_type.h>
#include <kiway_player.h>
#include <kiway.h>
#include <wx/filedlg.h>
#include <wx/intl.h>
#include <eda_doc.h>

#include <wx/button.h>
#include <wx/bmpbuttn.h>

#include <wx/log.h>

const wxString PG_UNIT_EDITOR::EDITOR_NAME = wxS( "KiCadUnitEditor" );
const wxString PG_CHECKBOX_EDITOR::EDITOR_NAME = wxS( "KiCadCheckboxEditor" );
const wxString PG_COLOR_EDITOR::EDITOR_NAME = wxS( "KiCadColorEditor" );
const wxString PG_RATIO_EDITOR::EDITOR_NAME = wxS( "KiCadRatioEditor" );
const wxString PG_FPID_EDITOR::EDITOR_NAME = wxS( "KiCadFpidEditor" );
const wxString PG_URL_EDITOR::EDITOR_NAME = wxS( "KiCadUrlEditor" );


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
    if( !aFrame )
        return EDITOR_NAME + "NoFrame";

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

#if wxCHECK_VERSION( 3, 3, 0 )
    wxString text = aProperty->GetValueAsString( wxPGPropValFormatFlags::EditableValue );
#else
    wxString text = aProperty->GetValueAsString( wxPG_EDITABLE_VALUE );
#endif
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
    else if( dynamic_cast<PGPROPERTY_AREA*>( aProperty) != nullptr )
    {
        m_unitBinder->SetDataType( EDA_DATA_TYPE::AREA );
    }
    else if( dynamic_cast<PGPROPERTY_ANGLE*>( aProperty ) != nullptr )
    {
        m_unitBinder->SetCoordType( ORIGIN_TRANSFORMS::NOT_A_COORD );
        m_unitBinder->SetUnits( EDA_UNITS::DEGREES );
    }
    else if( dynamic_cast<PGPROPERTY_TIME*>( aProperty ) != nullptr )
    {
        m_unitBinder->SetUnits( EDA_UNITS::PS );
    }

    UpdateControl( aProperty, win );

    return ret;
}


void PG_UNIT_EDITOR::UpdateControl( wxPGProperty* aProperty, wxWindow* aCtrl ) const
{
    wxVariant var = aProperty->GetValue();

    if( var.GetType() == wxT( "std::optional<int>" ) )
    {
        auto* variantData = static_cast<STD_OPTIONAL_INT_VARIANT_DATA*>( var.GetData() );

        if( variantData->Value().has_value() )
            m_unitBinder->ChangeValue( variantData->Value().value() );
        else
            m_unitBinder->ChangeValue( wxEmptyString );
    }
    else if( var.GetType() == wxPG_VARIANT_TYPE_LONG )
    {
        m_unitBinder->ChangeValue( var.GetLong() );
    }
    else if( var.GetType() == wxPG_VARIANT_TYPE_LONGLONG )
    {
        m_unitBinder->ChangeDoubleValue( var.GetLongLong().ToDouble() );
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

    if( textVal == wxT( "<...>" ) )
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
    else if( dynamic_cast<PGPROPERTY_AREA*>( aProperty ) != nullptr )
    {
        wxLongLongNative result = m_unitBinder->GetValue();
        changed = ( aVariant.IsNull() || result != aVariant.GetLongLong() );

        if( changed )
        {
            aVariant = result;
            m_unitBinder->SetDoubleValue( result.ToDouble() );
        }
    }
    else if( aVariant.GetType() == wxT( "std::optional<int>" ) )
    {
        auto* variantData = static_cast<STD_OPTIONAL_INT_VARIANT_DATA*>( aVariant.GetData() );
        std::optional<int> result;

        if( m_unitBinder->IsNull() )
        {
            changed = ( aVariant.IsNull() || variantData->Value().has_value() );

            if( changed )
            {
                aVariant = wxVariant( std::optional<int>() );
                m_unitBinder->SetValue( wxEmptyString );
            }
        }
        else
        {
            result = std::optional<int>( m_unitBinder->GetValue() );
            changed = ( aVariant.IsNull() || result != variantData->Value() );

            if( changed )
            {
                aVariant = wxVariant( result );
                m_unitBinder->SetValue( result.value() );
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

    // Capture property name instead of pointer to avoid dangling pointer if grid is rebuilt
    wxString propName = colorProp->GetName();

    editor->Bind( COLOR_SWATCH_CHANGED,
                  [=]( wxCommandEvent& aEvt )
                  {
                      wxPGProperty* prop = aGrid->GetPropertyByName( propName );

                      if( prop )
                      {
                          wxVariant val;
                          auto data = new COLOR4D_VARIANT_DATA( editor->GetSwatchColor() );
                          val.SetData( data );
                          aGrid->ChangePropertyValue( prop, val );
                      }
                  } );

#if wxCHECK_VERSION( 3, 3, 0 )
    if( aGrid->GetInternalFlags() & wxPropertyGrid::wxPG_FL_ACTIVATION_BY_CLICK )
#else
    if( aGrid->GetInternalFlags() & wxPG_FL_ACTIVATION_BY_CLICK )
#endif
    {
        aGrid->CallAfter(
                [=]()
                {
                    editor->GetNewSwatchColor();

                    wxPGProperty* prop = aGrid->GetPropertyByName( propName );

                    if( prop )
                        aGrid->DrawItem( prop );
                } );
    }

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


bool PG_RATIO_EDITOR::GetValueFromControl( wxVariant& aVariant, wxPGProperty* aProperty,
                                           wxWindow* aCtrl ) const
{
    wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( aCtrl );
    wxCHECK_MSG( textCtrl, false, "PG_RATIO_EDITOR requires a text control!" );
    wxString textVal = textCtrl->GetValue();

    if( textVal == wxT( "<...>" ) )
    {
        aVariant.MakeNull();
        return true;
    }

    bool changed;

    if( aVariant.GetType() == wxT( "std::optional<double>" ) )
    {
        auto* variantData = static_cast<STD_OPTIONAL_DOUBLE_VARIANT_DATA*>( aVariant.GetData() );

        if( textVal.empty() )
        {
            changed = ( aVariant.IsNull() || variantData->Value().has_value() );

            if( changed )
                aVariant = wxVariant( std::optional<double>() );
        }
        else
        {
            double dblValue;
            textVal.ToDouble( &dblValue );
            std::optional<double> result( dblValue );
            changed = ( aVariant.IsNull() || result != variantData->Value() );

            if( changed )
            {
                aVariant = wxVariant( result );
                textCtrl->SetValue( wxString::Format( wxS( "%g" ), dblValue ) );
            }
        }
    }
    else
    {
        double result;
        textVal.ToDouble( &result );
        changed = ( aVariant.IsNull() || result != aVariant.GetDouble() );

        if( changed )
        {
            aVariant = result;
            textCtrl->SetValue( wxString::Format( wxS( "%g" ), result ) );
        }
    }

    // Changing unspecified always causes event (returning
    // true here should be enough to trigger it).
    if( !changed && aVariant.IsNull() )
        changed = true;

    return changed;
}


void PG_RATIO_EDITOR::UpdateControl( wxPGProperty* aProperty, wxWindow* aCtrl ) const
{
    wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( aCtrl );
    wxVariant   var = aProperty->GetValue();

    wxCHECK_MSG( textCtrl, /*void*/, wxT( "PG_RATIO_EDITOR must be used with a textCtrl!" ) );

    if( var.GetType() == wxT( "std::optional<double>" ) )
    {
        auto*    variantData = static_cast<STD_OPTIONAL_DOUBLE_VARIANT_DATA*>( var.GetData() );
        wxString strValue;

        if( variantData->Value().has_value() )
            strValue = wxString::Format( wxS( "%g" ), variantData->Value().value() );

        textCtrl->ChangeValue( strValue );
    }
    else if( var.GetType() == wxPG_VARIANT_TYPE_DOUBLE )
    {
        textCtrl->ChangeValue( wxString::Format( wxS( "%g" ), var.GetDouble() ) );
    }
    else if( !aProperty->IsValueUnspecified() )
    {
        wxFAIL_MSG( wxT( "PG_RATIO_EDITOR should only be used with scale-free numeric "
                         "properties!" ) );
    }
}


PG_FPID_EDITOR::PG_FPID_EDITOR( EDA_DRAW_FRAME* aFrame ) : m_frame( aFrame )
{
    m_editorName = BuildEditorName( aFrame );
}


void PG_FPID_EDITOR::UpdateFrame( EDA_DRAW_FRAME* aFrame )
{
    m_frame = aFrame;
    m_editorName = BuildEditorName( aFrame );
}


wxString PG_FPID_EDITOR::BuildEditorName( EDA_DRAW_FRAME* aFrame )
{
    if( !aFrame )
        return EDITOR_NAME + "NoFrame";

    return EDITOR_NAME + aFrame->GetName();
}


wxPGWindowList PG_FPID_EDITOR::CreateControls( wxPropertyGrid* aGrid, wxPGProperty* aProperty,
                                               const wxPoint& aPos, const wxSize& aSize ) const
{
    wxPGMultiButton* buttons = new wxPGMultiButton( aGrid, aSize );
    buttons->Add( KiBitmap( BITMAPS::small_library ) );
    buttons->Finalize( aGrid, aPos );
    wxSize textSize = buttons->GetPrimarySize();
    wxWindow* textCtrl = aGrid->GenerateEditorTextCtrl( aPos, textSize,
                                                       aProperty->GetValueAsString(), nullptr, 0,
                                                       aProperty->GetMaxLength() );
    wxPGWindowList ret( textCtrl, buttons );
    return ret;
}


bool PG_FPID_EDITOR::OnEvent( wxPropertyGrid* aGrid, wxPGProperty* aProperty, wxWindow* aCtrl,
                              wxEvent& aEvent ) const
{
    if( aEvent.GetEventType() == wxEVT_BUTTON )
    {
        wxString fpid = aProperty->GetValue().GetString();

        if( KIWAY_PLAYER* frame = m_frame->Kiway().Player( FRAME_FOOTPRINT_CHOOSER, true, m_frame ) )
        {
            if( frame->ShowModal( &fpid, m_frame ) )
                aGrid->ChangePropertyValue( aProperty, fpid );

            frame->Destroy();
        }

        return true;
    }

    return wxPGTextCtrlEditor::OnEvent( aGrid, aProperty, aCtrl, aEvent );
}


PG_URL_EDITOR::PG_URL_EDITOR( EDA_DRAW_FRAME* aFrame ) : m_frame( aFrame )
{
    m_editorName = BuildEditorName( aFrame );
}


void PG_URL_EDITOR::UpdateFrame( EDA_DRAW_FRAME* aFrame )
{
    m_frame = aFrame;
    m_editorName = BuildEditorName( aFrame );
}


wxString PG_URL_EDITOR::BuildEditorName( EDA_DRAW_FRAME* aFrame )
{
    if( !aFrame )
        return EDITOR_NAME + "NoFrame";

    return EDITOR_NAME + aFrame->GetName();
}


wxPGWindowList PG_URL_EDITOR::CreateControls( wxPropertyGrid* aGrid, wxPGProperty* aProperty,
                                              const wxPoint& aPos, const wxSize& aSize ) const
{
    wxPGMultiButton* buttons = new wxPGMultiButton( aGrid, aSize );
    // Use a folder icon when no datasheet is set; otherwise use a globe icon.
    wxString urlValue = aProperty->GetValueAsString();
    bool     hasUrl   = !( urlValue.IsEmpty() || urlValue == wxS( "~" ) );
    buttons->Add( KiBitmap( hasUrl ? BITMAPS::www : BITMAPS::small_folder ) );
    buttons->Finalize( aGrid, aPos );
    wxSize textSize = buttons->GetPrimarySize();
    wxWindow* textCtrl = aGrid->GenerateEditorTextCtrl( aPos, textSize,
                                                       aProperty->GetValueAsString(), nullptr, 0,
                                                       aProperty->GetMaxLength() );
    wxPGWindowList ret( textCtrl, buttons );
    return ret;
}


bool PG_URL_EDITOR::OnEvent( wxPropertyGrid* aGrid, wxPGProperty* aProperty, wxWindow* aCtrl,
                             wxEvent& aEvent ) const
{
    if( aEvent.GetEventType() == wxEVT_BUTTON )
    {
        wxString filename = aProperty->GetValue().GetString();

        if( filename.IsEmpty() || filename == wxS( "~" ) )
        {
            wxFileDialog openFileDialog( m_frame, _( "Open file" ), wxS( "" ), wxS( "" ),
                                         _( "All Files" ) + wxS( " (*.*)|*.*" ),
                                         wxFD_OPEN | wxFD_FILE_MUST_EXIST );

            if( openFileDialog.ShowModal() == wxID_OK )
            {
                filename = openFileDialog.GetPath();
                aGrid->ChangePropertyValue( aProperty, wxString::Format( wxS( "file://%s" ),
                                                                         filename ) );
            }
        }
        else
        {
            GetAssociatedDocument( m_frame, filename, &m_frame->Prj() );
        }

        // Update the button icon to reflect presence/absence of URL
        if( wxObject* src = aEvent.GetEventObject() )
        {
            wxString newUrl = aProperty->GetValueAsString();
            bool     hasUrl = !( newUrl.IsEmpty() || newUrl == wxS( "~" ) );
            auto     bmp    = KiBitmap( hasUrl ? BITMAPS::www : BITMAPS::small_folder );

            if( wxWindow* win = wxDynamicCast( src, wxWindow ) )
            {
                if( wxBitmapButton* bb = wxDynamicCast( win, wxBitmapButton ) )
                {
                    bb->SetBitmap( bmp );
                }
                else if( wxButton* b = wxDynamicCast( win, wxButton ) )
                {
                    b->SetBitmap( bmp );
                }
                else if( wxWindow* parent = win->GetParent() )
                {
                    if( wxPGMultiButton* buttons = wxDynamicCast( parent, wxPGMultiButton ) )
                    {
                        wxWindow* btn0 = buttons->GetButton( 0 );
                        if( wxBitmapButton* bb0 = wxDynamicCast( btn0, wxBitmapButton ) )
                            bb0->SetBitmap( bmp );
                        else if( wxButton* b0 = wxDynamicCast( btn0, wxButton ) )
                            b0->SetBitmap( bmp );
                    }
                }
            }
        }
        return true;
    }

    return wxPGTextCtrlEditor::OnEvent( aGrid, aProperty, aCtrl, aEvent );
}
