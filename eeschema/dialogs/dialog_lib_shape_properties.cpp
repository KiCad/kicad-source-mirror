/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2006-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <lib_item.h>
#include <dialog_lib_shape_properties.h>
#include <symbol_edit_frame.h>
#include <symbol_editor_settings.h>
#include <confirm.h>
#include <lib_shape.h>
#include <widgets/color_swatch.h>
#include <settings/color_settings.h>
#include <sch_painter.h>

DIALOG_LIB_SHAPE_PROPERTIES::DIALOG_LIB_SHAPE_PROPERTIES( SYMBOL_EDIT_FRAME* aParent,
                                                          LIB_SHAPE* aShape ) :
    DIALOG_LIB_SHAPE_PROPERTIES_BASE( aParent ),
    m_frame( aParent ),
    m_shape( aShape ),
    m_borderWidth( aParent, m_borderWidthLabel, m_borderWidthCtrl, m_borderWidthUnits, true )
{
    wxASSERT( aShape );

    SetTitle( m_shape->GetTypeName() + wxT( " " ) + GetTitle() );
    m_helpLabel->SetFont( KIUI::GetInfoFont( this ).Italic() );

    COLOR_SETTINGS* colorSettings = m_frame->GetColorSettings();
    COLOR4D         schematicBackground = colorSettings->GetColor( LAYER_SCHEMATIC_BACKGROUND );

    m_borderColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_borderColorSwatch->SetSwatchBackground( schematicBackground );

    for( const std::pair<const PLOT_DASH_TYPE, lineTypeStruct>& typeEntry : lineTypeNames )
        m_borderStyleCombo->Append( typeEntry.second.name, KiBitmap( typeEntry.second.bitmap ) );

    m_borderStyleCombo->Append( DEFAULT_STYLE );

    m_fillColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_fillColorSwatch->SetSwatchBackground( schematicBackground );

    m_helpLabel->SetFont( KIUI::GetInfoFont( this ).Italic() );

    SetInitialFocus( m_borderWidthCtrl );

    // Required under wxGTK if we want to dismiss the dialog with the ESC key
    SetFocus();

    SetupStandardButtons();

    if( !aParent->IsSymbolEditable() || aParent->IsSymbolAlias() )
    {
        m_sdbSizerCancel->SetDefault();
        m_sdbSizerOK->SetLabel( _( "Read Only" ) );
        m_sdbSizerOK->Enable( false );
    }

    m_borderColorSwatch->Bind( COLOR_SWATCH_CHANGED, &DIALOG_LIB_SHAPE_PROPERTIES::onBorderSwatch, this );
    m_fillColorSwatch->Bind( COLOR_SWATCH_CHANGED, &DIALOG_LIB_SHAPE_PROPERTIES::onFillSwatch, this );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_LIB_SHAPE_PROPERTIES::~DIALOG_LIB_SHAPE_PROPERTIES()
{
    m_borderColorSwatch->Unbind( COLOR_SWATCH_CHANGED, &DIALOG_LIB_SHAPE_PROPERTIES::onBorderSwatch, this );
    m_fillColorSwatch->Unbind( COLOR_SWATCH_CHANGED, &DIALOG_LIB_SHAPE_PROPERTIES::onFillSwatch, this );
}


bool DIALOG_LIB_SHAPE_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    LIB_SYMBOL* symbol = m_shape->GetParent();

    m_checkBorder->SetValue( m_shape->GetWidth() >= 0 );

    if( m_shape->GetWidth() >= 0 )
        m_borderWidth.SetValue( m_shape->GetWidth() );

    m_borderWidth.Enable( m_shape->GetWidth() >= 0 );
    m_helpLabel->Enable( m_shape->GetWidth() >= 0 );
    m_borderColorLabel->Enable( m_shape->GetWidth() >= 0 );
    m_borderColorSwatch->Enable( m_shape->GetWidth() >= 0 );
    m_borderStyleLabel->Enable( m_shape->GetWidth() >= 0 );
    m_borderStyleCombo->Enable( m_shape->GetWidth() >= 0 );

    m_borderColorSwatch->SetSwatchColor( m_shape->GetStroke().GetColor(), false );

    int style = static_cast<int>( m_shape->GetStroke().GetPlotStyle() );

    if( style == -1 )
        m_borderStyleCombo->SetStringSelection( DEFAULT_STYLE );
    else if( style < (int) lineTypeNames.size() )
        m_borderStyleCombo->SetSelection( style );
    else
        wxFAIL_MSG( "Line type not found in the type lookup map" );

    m_privateCheckbox->SetValue( m_shape->IsPrivate() );
    m_checkApplyToAllUnits->SetValue(
            symbol && symbol->GetUnitCount() > 1 && m_shape->GetUnit() == 0 );
    m_checkApplyToAllUnits->Enable( symbol && symbol->GetUnitCount() > 1 );
    m_checkApplyToAllConversions->SetValue( m_shape->GetConvert() == 0 );

    bool enblConvOptStyle = symbol && symbol->HasConversion();

    // If a symbol contains no conversion-specific pins or graphic items, symbol->HasConversion()
    // will return false.
    // But when creating a new symbol, with DeMorgan option set, the ApplyToAllConversions
    // must be enabled in order to be able to create graphic items shared by all body styles.
    if( m_frame->GetShowDeMorgan() )
        enblConvOptStyle = true;

    m_checkApplyToAllConversions->Enable( enblConvOptStyle );

    m_rbFillNone->Enable( true );
    m_rbFillOutline->Enable( true );
    m_rbFillBackground->Enable( true );
    m_rbFillCustom->Enable( true );
    m_fillColorSwatch->Enable( true );

    if( m_shape->GetFillMode() == FILL_T::FILLED_SHAPE )
    {
        m_rbFillOutline->SetValue( true );

        COLOR4D color = m_shape->GetStroke().GetColor();

        if( color == COLOR4D::UNSPECIFIED )
            color = m_frame->GetRenderSettings()->GetLayerColor( LAYER_DEVICE );

        m_fillColorSwatch->SetSwatchColor( color, false );
    }
    else if( m_shape->GetFillMode() == FILL_T::FILLED_WITH_BG_BODYCOLOR )
    {
        m_rbFillBackground->SetValue( true );

        COLOR4D color = m_frame->GetRenderSettings()->GetLayerColor( LAYER_DEVICE_BACKGROUND );
        m_fillColorSwatch->SetSwatchColor( color, false );
    }
    else if( m_shape->GetFillMode() == FILL_T::FILLED_WITH_COLOR )
    {
        m_rbFillCustom->SetValue( true );
        m_fillColorSwatch->SetSwatchColor( m_shape->GetFillColor(), false );
    }
    else
    {
        m_rbFillNone->SetValue( true );
        m_fillColorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
    }

    return true;
}


void DIALOG_LIB_SHAPE_PROPERTIES::onFill( wxCommandEvent& event )
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


void DIALOG_LIB_SHAPE_PROPERTIES::onBorderChecked( wxCommandEvent& event )
{
    bool border = m_checkBorder->GetValue();

    if( border && m_borderWidth.GetValue() < 0 )
        m_borderWidth.SetValue( schIUScale.MilsToIU( m_frame->libeditconfig()->m_Defaults.line_width ) );

    m_borderWidth.Enable( border );
    m_borderColorLabel->Enable( border );
    m_borderColorSwatch->Enable( border );
    m_borderStyleLabel->Enable( border );
    m_borderStyleCombo->Enable( border );
}


void DIALOG_LIB_SHAPE_PROPERTIES::onBorderSwatch( wxCommandEvent& aEvent )
{
    if( m_rbFillOutline->GetValue() )
        m_fillColorSwatch->SetSwatchColor( m_borderColorSwatch->GetSwatchColor(), false );
}


void DIALOG_LIB_SHAPE_PROPERTIES::onFillSwatch( wxCommandEvent& aEvent )
{
    m_rbFillCustom->SetValue( true );
}


bool DIALOG_LIB_SHAPE_PROPERTIES::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    STROKE_PARAMS stroke = m_shape->GetStroke();

    if( m_checkBorder->GetValue() )
    {
        if( !m_borderWidth.IsIndeterminate() )
            stroke.SetWidth( m_borderWidth.GetValue() );
    }
    else
    {
        stroke.SetWidth( -1 );
    }

    auto it = lineTypeNames.begin();
    std::advance( it, m_borderStyleCombo->GetSelection() );

    if( it == lineTypeNames.end() )
        stroke.SetPlotStyle( PLOT_DASH_TYPE::DEFAULT );
    else
        stroke.SetPlotStyle( it->first );

    stroke.SetColor( m_borderColorSwatch->GetSwatchColor() );

    m_shape->SetStroke( stroke );

    if( m_rbFillOutline->GetValue() )
        m_shape->SetFillMode( FILL_T::FILLED_SHAPE );
    else if( m_rbFillBackground->GetValue() )
        m_shape->SetFillMode( FILL_T::FILLED_WITH_BG_BODYCOLOR );
    else if( m_rbFillCustom->GetValue() )
        m_shape->SetFillMode( FILL_T::FILLED_WITH_COLOR );
    else
        m_shape->SetFillMode( FILL_T::NO_FILL );

    m_shape->SetFillColor( m_fillColorSwatch->GetSwatchColor() );

    m_shape->SetPrivate( m_privateCheckbox->GetValue() );

    if( GetApplyToAllConversions() )
        m_shape->SetConvert( 0 );
    else
        m_shape->SetConvert( m_frame->GetConvert() );

    if( GetApplyToAllUnits() )
        m_shape->SetUnit( 0 );
    else
        m_shape->SetUnit( m_frame->GetUnit() );

    return true;
}


bool DIALOG_LIB_SHAPE_PROPERTIES::GetApplyToAllConversions()
{
    return m_checkApplyToAllConversions->IsChecked();
}


bool DIALOG_LIB_SHAPE_PROPERTIES::GetApplyToAllUnits()
{
    return m_checkApplyToAllUnits->IsChecked();
}

