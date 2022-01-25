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
#include <confirm.h>
#include <lib_shape.h>
#include <widgets/color_swatch.h>
#include <sch_painter.h>

DIALOG_LIB_SHAPE_PROPERTIES::DIALOG_LIB_SHAPE_PROPERTIES( SYMBOL_EDIT_FRAME* aParent,
                                                          LIB_ITEM* aItem ) :
    DIALOG_LIB_SHAPE_PROPERTIES_BASE( aParent ),
    m_frame( aParent ),
    m_item( aItem ),
    m_lineWidth( aParent, m_widthLabel, m_widthCtrl, m_widthUnits, true )
{
    SetTitle( aItem->GetTypeName() + wxT( " " ) + GetTitle() );
    m_helpLabel->SetFont( KIUI::GetInfoFont( this ).Italic() );

    m_colorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

    SetInitialFocus( m_widthCtrl );

    // Required under wxGTK if we want to dismiss the dialog with the ESC key
    SetFocus();

    SetupStandardButtons();

    if( !aParent->IsSymbolEditable() || aParent->IsSymbolAlias() )
    {
        m_sdbSizerCancel->SetDefault();
        m_sdbSizerOK->SetLabel( _( "Read Only" ) );
        m_sdbSizerOK->Enable( false );
    }

    m_colorSwatch->Bind( COLOR_SWATCH_CHANGED, &DIALOG_LIB_SHAPE_PROPERTIES::onSwatch, this );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_LIB_SHAPE_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    LIB_SYMBOL* symbol = m_item->GetParent();
    EDA_SHAPE*  shape = dynamic_cast<EDA_SHAPE*>( m_item );

    if( shape )
        m_lineWidth.SetValue( shape->GetWidth() );

    m_privateCheckbox->SetValue( m_item->IsPrivate() );
    m_checkApplyToAllUnits->SetValue( m_item->GetUnit() == 0 );
    m_checkApplyToAllUnits->Enable( symbol && symbol->GetUnitCount() > 1 );
    m_checkApplyToAllConversions->SetValue( m_item->GetConvert() == 0 );

    bool enblConvOptStyle = symbol && symbol->HasConversion();

    // if a symbol contains no graphic items, symbol->HasConversion() returns false.
    // but when creating a new symbol, with DeMorgan option set, the ApplyToAllConversions
    // must be enabled even if symbol->HasConversion() returns false in order to be able
    // to create graphic items shared by all body styles
    if( m_frame->GetShowDeMorgan() )
        enblConvOptStyle = true;

    m_checkApplyToAllConversions->Enable( enblConvOptStyle );

    m_rbFillNone->Enable( shape != nullptr );
    m_rbFillOutline->Enable( shape != nullptr );
    m_rbFillBackground->Enable( shape != nullptr );
    m_rbFillCustom->Enable( shape != nullptr );
    m_colorSwatch->Enable( shape != nullptr );

    if( shape && shape->GetFillMode() == FILL_T::FILLED_SHAPE )
    {
        m_rbFillOutline->SetValue( true );

        COLOR4D color = m_frame->GetRenderSettings()->GetLayerColor( LAYER_DEVICE );
        m_colorSwatch->SetSwatchColor( color, false );
    }
    else if( shape && shape->GetFillMode() == FILL_T::FILLED_WITH_BG_BODYCOLOR )
    {
        m_rbFillBackground->SetValue( true );

        COLOR4D color = m_frame->GetRenderSettings()->GetLayerColor( LAYER_DEVICE_BACKGROUND );
        m_colorSwatch->SetSwatchColor( color, false );
    }
    else if( shape && shape->GetFillMode() == FILL_T::FILLED_WITH_COLOR )
    {
        m_rbFillCustom->SetValue( true );
        m_colorSwatch->SetSwatchColor( shape->GetFillColor(), false );
    }
    else
    {
        m_rbFillNone->SetValue( true );
        m_colorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
    }

    return true;
}


void DIALOG_LIB_SHAPE_PROPERTIES::onFill( wxCommandEvent& event )
{
    if( event.GetId() == NO_FILL )
    {
        m_rbFillNone->SetValue( true );
        m_colorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
    }
    else if( event.GetId() == FILLED_SHAPE )
    {
        m_rbFillOutline->SetValue( true );

        COLOR4D color = m_frame->GetRenderSettings()->GetLayerColor( LAYER_DEVICE );
        m_colorSwatch->SetSwatchColor( color, false );
    }
    else if( event.GetId() == FILLED_WITH_BG_BODYCOLOR )
    {
        m_rbFillBackground->SetValue( true );

        COLOR4D color = m_frame->GetRenderSettings()->GetLayerColor( LAYER_DEVICE_BACKGROUND );
        m_colorSwatch->SetSwatchColor( color, false );
    }
    else if( event.GetId() == FILLED_WITH_COLOR )
    {
        m_rbFillCustom->SetValue( true );
        m_colorSwatch->GetNewSwatchColor();
    }
}


void DIALOG_LIB_SHAPE_PROPERTIES::onSwatch( wxCommandEvent& aEvent )
{
    m_rbFillCustom->SetValue( true );
}


bool DIALOG_LIB_SHAPE_PROPERTIES::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    LIB_SHAPE* shape = dynamic_cast<LIB_SHAPE*>( m_item );

    if( shape )
    {
        if( m_rbFillOutline->GetValue() )
            shape->SetFillMode( FILL_T::FILLED_SHAPE );
        else if( m_rbFillBackground->GetValue() )
            shape->SetFillMode( FILL_T::FILLED_WITH_BG_BODYCOLOR );
        else if( m_rbFillCustom->GetValue() )
            shape->SetFillMode( FILL_T::FILLED_WITH_COLOR );
        else
            shape->SetFillMode( FILL_T::NO_FILL );

        shape->SetFillColor( m_colorSwatch->GetSwatchColor() );

        STROKE_PARAMS stroke = shape->GetStroke();
        stroke.SetWidth( m_lineWidth.GetValue() );
        shape->SetStroke( stroke );
    }

    m_item->SetPrivate( m_privateCheckbox->GetValue() );

    if( GetApplyToAllConversions() )
        m_item->SetConvert( 0 );
    else
        m_item->SetConvert( m_frame->GetConvert() );

    if( GetApplyToAllUnits() )
        m_item->SetUnit( 0 );
    else
        m_item->SetUnit( m_frame->GetUnit() );

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

