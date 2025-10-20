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

#include <widgets/color_swatch.h>
#include <stroke_params.h>
#include <sch_edit_frame.h>
#include <symbol_edit_frame.h>
#include <sch_shape.h>
#include <sch_rule_area.h>
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
    // dialog layouts.
    m_hash_key = TO_UTF8( GetTitle() + aParent->GetName() );

    m_helpLabel1->SetFont( KIUI::GetInfoFont( this ).Italic() );
    m_helpLabel2->SetFont( KIUI::GetInfoFont( this ).Italic() );

    COLOR_SETTINGS* colorSettings = m_frame->GetColorSettings();
    COLOR4D         schematicBackground = colorSettings->GetColor( LAYER_SCHEMATIC_BACKGROUND );

    m_borderColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_borderColorSwatch->SetSwatchBackground( schematicBackground );

    for( const auto& [ lineStyle, lineStyleDesc ] : lineTypeNames )
        m_borderStyleCombo->Append( lineStyleDesc.name, KiBitmapBundle( lineStyleDesc.bitmap ) );

    m_fillColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_fillColorSwatch->SetSwatchBackground( schematicBackground );

    KIGFX::COLOR4D canvas = m_frame->GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );
    m_borderColorSwatch->SetSwatchBackground( canvas.ToColour() );
    m_fillColorSwatch->SetSwatchBackground( canvas.ToColour() );

    if( m_frame->GetColorSettings()->GetOverrideSchItemColors() )
        m_infoBar->ShowMessage( _( "Note: individual item colors overridden in Preferences." ) );

    m_ruleAreaSizer->Show( dynamic_cast<SCH_RULE_AREA*>( aShape ) != nullptr );

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

    m_borderColorSwatch->Bind( COLOR_SWATCH_CHANGED, &DIALOG_SHAPE_PROPERTIES::onBorderSwatch, this );
    m_customColorSwatch->Bind( COLOR_SWATCH_CHANGED, &DIALOG_SHAPE_PROPERTIES::onCustomColorSwatch, this );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_SHAPE_PROPERTIES::~DIALOG_SHAPE_PROPERTIES()
{
    m_borderColorSwatch->Unbind( COLOR_SWATCH_CHANGED, &DIALOG_SHAPE_PROPERTIES::onBorderSwatch, this );
    m_customColorSwatch->Unbind( COLOR_SWATCH_CHANGED, &DIALOG_SHAPE_PROPERTIES::onCustomColorSwatch, this );
}


bool DIALOG_SHAPE_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    if( SCH_RULE_AREA* ruleArea = dynamic_cast<SCH_RULE_AREA*>( m_shape ) )
    {
        m_cbExcludeFromSim->SetValue( ruleArea->GetExcludedFromSim() );
        m_cbExcludeFromBom->SetValue( ruleArea->GetExcludedFromBOM() );
        m_cbExcludeFromBoard->SetValue( ruleArea->GetExcludedFromBoard() );
        m_cbDNP->SetValue( ruleArea->GetDNP() );
    }

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
        m_borderStyleCombo->SetStringSelection( DEFAULT_LINE_STYLE_LABEL );
    else if( style < (int) lineTypeNames.size() )
        m_borderStyleCombo->SetSelection( style );
    else
        wxFAIL_MSG( wxT( "Line type not found in the type lookup map" ) );

    if( dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame ) )
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
        m_checkApplyToAllUnits->SetValue( symbol->IsMultiUnit() && m_shape->GetUnit() == 0 );
        m_checkApplyToAllUnits->Enable( symbol->IsMultiUnit() );
        m_checkApplyToAllBodyStyles->SetValue( symbol->IsMultiBodyStyle() && m_shape->GetBodyStyle() == 0 );
        m_checkApplyToAllBodyStyles->Enable( symbol->IsMultiBodyStyle() );
    }
    else
    {
        m_fillCtrl->SetSelection( m_shape->GetFillModeProp() );
        m_fillColorSwatch->SetSwatchColor( m_shape->GetFillColor(), false );
    }

    m_fillColorLabel->Enable( m_fillCtrl->GetSelection() != UI_FILL_MODE::NONE );
    m_fillColorSwatch->Enable( m_fillCtrl->GetSelection() != UI_FILL_MODE::NONE );

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


void DIALOG_SHAPE_PROPERTIES::onFillChoice( wxCommandEvent& event )
{
    m_fillColorLabel->Enable( m_fillCtrl->GetSelection() != UI_FILL_MODE::NONE );
    m_fillColorSwatch->Enable( m_fillCtrl->GetSelection() != UI_FILL_MODE::NONE );
}


void DIALOG_SHAPE_PROPERTIES::onFillRadioButton( wxCommandEvent& event )
{
    if( event.GetId() == NO_FILL )
    {
        m_rbFillNone->SetValue( true );
        m_customColorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
    }
    else if( event.GetId() == FILLED_SHAPE )
    {
        m_rbFillOutline->SetValue( true );

        COLOR4D color = m_borderColorSwatch->GetSwatchColor();

        if( color == COLOR4D::UNSPECIFIED || !m_rbFillOutline->GetValue() )
            color = m_frame->GetRenderSettings()->GetLayerColor( LAYER_DEVICE );

        m_customColorSwatch->SetSwatchColor( color, false );
    }
    else if( event.GetId() == FILLED_WITH_BG_BODYCOLOR )
    {
        m_rbFillBackground->SetValue( true );

        COLOR4D color = m_frame->GetRenderSettings()->GetLayerColor( LAYER_DEVICE_BACKGROUND );
        m_customColorSwatch->SetSwatchColor( color, false );
    }
    else if( event.GetId() == FILLED_WITH_COLOR )
    {
        m_rbFillCustom->SetValue( true );
        m_customColorSwatch->GetNewSwatchColor();
    }
}


void DIALOG_SHAPE_PROPERTIES::onBorderSwatch( wxCommandEvent& aEvent )
{
    if( m_rbFillOutline->GetValue() )
        m_fillColorSwatch->SetSwatchColor( m_borderColorSwatch->GetSwatchColor(), false );

    if( m_rbFillOutline->IsEnabled() && m_rbFillOutline->GetValue() )
    {
        COLOR4D color = COLOR4D::UNSPECIFIED;

        if( m_rbFillOutline->GetValue() )
            color = m_fillColorSwatch->GetSwatchColor();

        if( color == COLOR4D::UNSPECIFIED )
            color = m_frame->GetRenderSettings()->GetLayerColor( LAYER_DEVICE );

        m_customColorSwatch->SetSwatchColor( color, false );
    }
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

    if( SCH_RULE_AREA* ruleArea = dynamic_cast<SCH_RULE_AREA*>( m_shape ) )
    {
        ruleArea->SetExcludedFromSim( m_cbExcludeFromSim->GetValue() );
        ruleArea->SetExcludedFromBOM( m_cbExcludeFromBom->GetValue() );
        ruleArea->SetExcludedFromBoard( m_cbExcludeFromBoard->GetValue() );
        ruleArea->SetDNP( m_cbDNP->GetValue() );
    }

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
        stroke.SetLineStyle( LINE_STYLE::SOLID );
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
        m_shape->SetFillModeProp( (UI_FILL_MODE) m_fillCtrl->GetSelection() );
        m_shape->SetFillColor( m_fillColorSwatch->GetSwatchColor() );
    }

    if( !commit.Empty() )
        commit.Push( wxString::Format( _( "Edit %s" ), m_shape->GetFriendlyName() ) );

    return true;
}


