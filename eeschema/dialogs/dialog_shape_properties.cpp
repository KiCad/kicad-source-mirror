/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/color_swatch.h>
#include <stroke_params.h>
#include <sch_edit_frame.h>
#include <symbol_edit_frame.h>
#include <sch_shape.h>
#include <dialog_shape_properties.h>
#include <settings/color_settings.h>
#include <symbol_editor_settings.h>
#include <sch_commit.h>
#include <string_utils.h>


DIALOG_SHAPE_PROPERTIES::DIALOG_SHAPE_PROPERTIES( SCH_BASE_FRAME* aParent, SCH_SHAPE* aShape ) :
    DIALOG_SHAPE_PROPERTIES_BASE( aParent ),
    m_frame( aParent ),
    m_shape( aShape ),
    m_borderWidth( aParent, m_borderWidthLabel, m_borderWidthCtrl, m_borderWidthUnits, true )
{
    SetTitle( wxString::Format( GetTitle(), aShape->GetFriendlyName() ) );

    // DIALOG_SHIM needs a unique hash_key because classname is not sufficient because the
    // different shapes (and even whether or not we're within the symbol editor) cause different
    // dialog layouts).
    m_hash_key = TO_UTF8( GetTitle() + aParent->GetName() );

    m_helpLabel1->SetFont( KIUI::GetInfoFont( this ).Italic() );
    m_helpLabel2->SetFont( KIUI::GetInfoFont( this ).Italic() );

    COLOR_SETTINGS* colorSettings = m_frame->GetColorSettings();
    COLOR4D         schematicBackground = colorSettings->GetColor( LAYER_SCHEMATIC_BACKGROUND );

    m_borderColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_borderColorSwatch->SetSwatchBackground( schematicBackground );

    for( const auto& [ lineStyle, lineStyleDesc ] : lineTypeNames )
        m_borderStyleCombo->Append( lineStyleDesc.name, KiBitmapBundle( lineStyleDesc.bitmap ) );

    m_borderStyleCombo->Append( DEFAULT_STYLE );

    m_fillColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_fillColorSwatch->SetSwatchBackground( schematicBackground );

    KIGFX::COLOR4D canvas = m_frame->GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );
    m_borderColorSwatch->SetSwatchBackground( canvas.ToColour() );
    m_fillColorSwatch->SetSwatchBackground( canvas.ToColour() );

    if( m_frame->GetColorSettings()->GetOverrideSchItemColors() )
        m_infoBar->ShowMessage( _( "Note: individual item colors overridden in Preferences." ) );

    SetInitialFocus( m_borderWidthCtrl );

    // Required under wxGTK if we want to dismiss the dialog with the ESC key
    SetFocus();

    SetupStandardButtons();

    if( SYMBOL_EDIT_FRAME* symbolEditor = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame ) )
    {
        m_fillBook->SetSelection( 1 );

        if( !symbolEditor->IsSymbolEditable() || symbolEditor->IsSymbolAlias() )
        {
            m_sdbSizerCancel->SetDefault();
            m_sdbSizerOK->SetLabel( _( "Read Only" ) );
            m_sdbSizerOK->Enable( false );
        }
    }
    else
    {
        m_fillBook->SetSelection( 0 );
        m_symbolEditorSizer->Show( false );
    }

    m_borderColorSwatch->Bind( COLOR_SWATCH_CHANGED, &DIALOG_SHAPE_PROPERTIES::onBorderSwatch,
                               this );
    m_customColorSwatch->Bind( COLOR_SWATCH_CHANGED, &DIALOG_SHAPE_PROPERTIES::onCustomColorSwatch,
                               this );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_SHAPE_PROPERTIES::~DIALOG_SHAPE_PROPERTIES()
{
    m_borderColorSwatch->Unbind( COLOR_SWATCH_CHANGED, &DIALOG_SHAPE_PROPERTIES::onBorderSwatch,
                                 this );
    m_customColorSwatch->Unbind( COLOR_SWATCH_CHANGED, &DIALOG_SHAPE_PROPERTIES::onCustomColorSwatch,
                                 this );
}


bool DIALOG_SHAPE_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    if( m_shape->GetWidth() >= 0 )
    {
        m_borderCheckbox->SetValue( true );
        m_borderWidth.SetValue( m_shape->GetWidth() );
    }
    else
    {
        m_borderCheckbox->SetValue( false );

        m_borderWidth.Enable( false );
        m_borderColorLabel->Enable( false );
        m_borderColorSwatch->Enable( false );
        m_borderStyleLabel->Enable( false );
        m_borderStyleCombo->Enable( false );
    }

    m_borderColorSwatch->SetSwatchColor( m_shape->GetStroke().GetColor(), false );

    int style = static_cast<int>( m_shape->GetStroke().GetLineStyle() );

    if( style == -1 )
        m_borderStyleCombo->SetStringSelection( DEFAULT_STYLE );
    else if( style < (int) lineTypeNames.size() )
        m_borderStyleCombo->SetSelection( style );
    else
        wxFAIL_MSG( "Line type not found in the type lookup map" );

    if( SYMBOL_EDIT_FRAME* symbolEditor = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame ) )
    {
        m_rbFillNone->Enable( true );
        m_rbFillOutline->Enable( true );
        m_rbFillBackground->Enable( true );
        m_rbFillCustom->Enable( true );
        m_customColorSwatch->Enable( true );

        if( m_shape->GetFillMode() == FILL_T::FILLED_SHAPE )
        {
            m_rbFillOutline->SetValue( true );

            COLOR4D color = m_shape->GetStroke().GetColor();

            if( color == COLOR4D::UNSPECIFIED )
                color = m_frame->GetRenderSettings()->GetLayerColor( LAYER_DEVICE );

            m_customColorSwatch->SetSwatchColor( color, false );
        }
        else if( m_shape->GetFillMode() == FILL_T::FILLED_WITH_BG_BODYCOLOR )
        {
            m_rbFillBackground->SetValue( true );

            COLOR4D color = m_frame->GetRenderSettings()->GetLayerColor( LAYER_DEVICE_BACKGROUND );
            m_customColorSwatch->SetSwatchColor( color, false );
        }
        else if( m_shape->GetFillMode() == FILL_T::FILLED_WITH_COLOR )
        {
            m_rbFillCustom->SetValue( true );
            m_customColorSwatch->SetSwatchColor( m_shape->GetFillColor(), false );
        }
        else
        {
            m_rbFillNone->SetValue( true );
            m_customColorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
        }

        const SYMBOL* symbol = m_shape->GetParentSymbol();

        m_privateCheckbox->SetValue( m_shape->IsPrivate() );
        m_checkApplyToAllUnits->SetValue( symbol->GetUnitCount() > 1 && m_shape->GetUnit() == 0 );
        m_checkApplyToAllUnits->Enable( symbol->GetUnitCount() > 1 );
        m_checkApplyToAllBodyStyles->SetValue( m_shape->GetBodyStyle() == 0 );

        bool enableAlternateBodyStyle = symbol->HasAlternateBodyStyle();

        // If a symbol contains no body-style-specific pins or graphic items,
        // symbol->HasAlternateBodyStyle() will return false.
        // But when creating a new symbol, with DeMorgan option set, the m_checkApplyToAllBodyStyles
        // must be enabled in order to be able to create graphic items shared by all body styles.
        if( symbolEditor->GetShowDeMorgan() )
            enableAlternateBodyStyle = true;

        m_checkApplyToAllBodyStyles->Enable( enableAlternateBodyStyle );
    }
    else
    {
        m_filledCtrl->SetValue( m_shape->IsFilled() );
        m_fillColorSwatch->SetSwatchColor( m_shape->GetFillColor(), false );
    }

    return true;
}


void DIALOG_SHAPE_PROPERTIES::onBorderChecked( wxCommandEvent& event )
{
    bool border = m_borderCheckbox->GetValue();

    if( border && m_borderWidth.GetValue() < 0 )
    {
        int defaultInMils;

        if( SYMBOL_EDIT_FRAME* symbolEditor = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame ) )
            defaultInMils = symbolEditor->libeditconfig()->m_Defaults.line_width;
        else
            defaultInMils = m_frame->eeconfig()->m_Drawing.default_line_thickness;

        m_borderWidth.SetValue( schIUScale.MilsToIU( defaultInMils ) );
    }

    m_borderWidth.Enable( border );
    m_borderColorLabel->Enable( border );
    m_borderColorSwatch->Enable( border );
    m_borderStyleLabel->Enable( border );
    m_borderStyleCombo->Enable( border );
}


void DIALOG_SHAPE_PROPERTIES::onFillChecked( wxCommandEvent& aEvent )
{
    bool fill = m_filledCtrl->GetValue();

    m_fillColorLabel->Enable( fill );
    m_fillColorSwatch->Enable( fill );
}


void DIALOG_SHAPE_PROPERTIES::onFillRadioButton( wxCommandEvent& event )
{
    if( event.GetId() == NO_FILL )
    {
        m_rbFillNone->SetValue( true );
        m_fillColorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
    }
    else if( event.GetId() == FILLED_SHAPE )
    {
        m_rbFillOutline->SetValue( true );

        COLOR4D color = m_shape->GetStroke().GetColor();

        if( color == COLOR4D::UNSPECIFIED )
            color = m_frame->GetRenderSettings()->GetLayerColor( LAYER_DEVICE );

        m_fillColorSwatch->SetSwatchColor( color, false );
    }
    else if( event.GetId() == FILLED_WITH_BG_BODYCOLOR )
    {
        m_rbFillBackground->SetValue( true );

        COLOR4D color = m_frame->GetRenderSettings()->GetLayerColor( LAYER_DEVICE_BACKGROUND );
        m_fillColorSwatch->SetSwatchColor( color, false );
    }
    else if( event.GetId() == FILLED_WITH_COLOR )
    {
        m_rbFillCustom->SetValue( true );
        m_fillColorSwatch->GetNewSwatchColor();
    }
}


void DIALOG_SHAPE_PROPERTIES::onBorderSwatch( wxCommandEvent& aEvent )
{
    if( m_rbFillOutline->GetValue() )
        m_fillColorSwatch->SetSwatchColor( m_borderColorSwatch->GetSwatchColor(), false );
}


void DIALOG_SHAPE_PROPERTIES::onCustomColorSwatch( wxCommandEvent& aEvent )
{
    m_rbFillCustom->SetValue( true );
}


bool DIALOG_SHAPE_PROPERTIES::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    SCH_COMMIT commit( m_frame );

    if( !m_shape->IsNew() )
        commit.Modify( m_shape, m_frame->GetScreen() );

    STROKE_PARAMS stroke = m_shape->GetStroke();

    if( m_borderCheckbox->GetValue() )
    {
        if( !m_borderWidth.IsIndeterminate() )
            stroke.SetWidth( std::max( 0, m_borderWidth.GetIntValue() ) );
    }
    else
    {
        stroke.SetWidth( -1 );
    }

    auto it = lineTypeNames.begin();
    std::advance( it, m_borderStyleCombo->GetSelection() );

    if( it == lineTypeNames.end() )
        stroke.SetLineStyle( LINE_STYLE::DEFAULT );
    else
        stroke.SetLineStyle( it->first );

    stroke.SetColor( m_borderColorSwatch->GetSwatchColor() );

    m_shape->SetStroke( stroke );

    if( SYMBOL_EDIT_FRAME* symbolEditor = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame ) )
    {
        if( m_rbFillOutline->GetValue() )
            m_shape->SetFillMode( FILL_T::FILLED_SHAPE );
        else if( m_rbFillBackground->GetValue() )
            m_shape->SetFillMode( FILL_T::FILLED_WITH_BG_BODYCOLOR );
        else if( m_rbFillCustom->GetValue() )
            m_shape->SetFillMode( FILL_T::FILLED_WITH_COLOR );
        else
            m_shape->SetFillMode( FILL_T::NO_FILL );

        m_shape->SetFillColor( m_customColorSwatch->GetSwatchColor() );

        m_shape->SetPrivate( m_privateCheckbox->GetValue() );

        if( m_checkApplyToAllBodyStyles->IsChecked() )
            m_shape->SetBodyStyle( 0 );
        else
            m_shape->SetBodyStyle( symbolEditor->GetBodyStyle() );

        if( m_checkApplyToAllUnits->IsChecked() )
            m_shape->SetUnit( 0 );
        else
            m_shape->SetUnit( symbolEditor->GetUnit() );
    }
    else
    {
        if( m_filledCtrl->GetValue() )
            m_shape->SetFillMode( FILL_T::FILLED_WITH_COLOR );
        else
            m_shape->SetFillMode( FILL_T::NO_FILL );

        m_shape->SetFillColor( m_fillColorSwatch->GetSwatchColor() );
    }

    if( !commit.Empty() )
        commit.Push( wxString::Format( _( "Edit %s" ), m_shape->GetFriendlyName() ) );

    return true;
}


