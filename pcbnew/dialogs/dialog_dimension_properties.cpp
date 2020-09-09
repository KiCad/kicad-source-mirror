/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <class_board.h>
#include <class_dimension.h>
#include <pcb_base_edit_frame.h>
#include <pcb_layer_box_selector.h>
#include <widgets/unit_binder.h>

#include "dialog_dimension_properties.h"



DIALOG_DIMENSION_PROPERTIES::DIALOG_DIMENSION_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent,
                                                          BOARD_ITEM* aItem ) :
        DIALOG_DIMENSION_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_textWidth( aParent, m_lblTextWidth, m_txtTextWidth, m_lblTextWidthUnits, true ),
        m_textHeight( aParent, m_lblTextHeight, m_txtTextHeight, m_lblTextHeightUnits, true ),
        m_textThickness( aParent, m_lblTextThickness, m_txtTextThickness,
                         m_lblTextHeightUnits, true ),
        m_textPosX( aParent, m_lblTextPosX, m_txtTextPosX, m_lblTextPosXUnits ),
        m_textPosY( aParent, m_lblTextPosY, m_txtTextPosY, m_lblTextPosYUnits ),
        m_orientValidator( 1, &m_orientValue ),
        m_lineThickness( aParent, m_lblLineThickness, m_txtLineThickness,
                         m_lblLineThicknessUnits, true ),
        m_arrowLength( aParent, m_lblArrowLength, m_txtArrowLength, m_lblArrowLengthUnits, true )
{
    wxASSERT( aItem->Type() == PCB_DIMENSION_T );
    m_dimension = static_cast<DIMENSION*>( aItem );

    // Configure display origin transforms
    m_textPosX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_textPosY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    // Configure the layers list selector.  Note that footprints are built outside the current
    // board and so we may need to show all layers if the text is on an unactivated layer.
    if( !m_frame->GetBoard()->IsLayerEnabled( m_dimension->GetLayer() ) )
        m_cbLayer->ShowNonActivatedLayers( true );

    m_cbLayer->SetLayersHotkeys( false );
    m_cbLayer->SetNotAllowedLayerSet( LSET::ForbiddenTextLayers() );
    m_cbLayer->SetBoardFrame( aParent );
    m_cbLayer->Resync();

    m_orientValue = 0.0;
    m_orientValidator.SetRange( -360.0, 360.0 );
    m_cbTextOrientation->SetValidator( m_orientValidator );
    m_orientValidator.SetWindow( m_cbTextOrientation );

    // Handle decimal separators in combo dropdown
    for( size_t i = 0; i < m_cbTextOrientation->GetCount(); ++i )
    {
        wxString item = m_cbTextOrientation->GetString( i );
        item.Replace( '.', localeconv()->decimal_point[0] );
        m_cbTextOrientation->SetString( i, item );
    }

    m_sdbSizerOK->SetDefault();

    FinishDialogSettings();
}


DIALOG_DIMENSION_PROPERTIES::~DIALOG_DIMENSION_PROPERTIES()
{
}


bool DIALOG_DIMENSION_PROPERTIES::TransferDataToWindow()
{
    m_txtValue->SetValue( m_dimension->GetText() );

    EDA_UNITS  units;
    bool       useMils;
    m_dimension->GetUnits( units, useMils );

    m_cbUnits->SetSelection( units == EDA_UNITS::MILLIMETRES ? 2 : useMils ? 1 : 0 );

    m_txtPrefix->SetValue( m_dimension->GetPrefix() );
    m_txtSuffix->SetValue( m_dimension->GetSuffix() );

    if( m_cbLayer->SetLayerSelection( m_dimension->GetLayer() ) < 0 )
    {
        wxMessageBox( _( "This item was on a non-existing or forbidden layer.\n"
                         "It has been moved to the first allowed layer." ) );
        m_cbLayer->SetSelection( 0 );
    }

    TEXTE_PCB& text = m_dimension->Text();

    m_textWidth.SetValue( text.GetTextSize().x );
    m_textHeight.SetValue( text.GetTextSize().y );
    m_textThickness.SetValue( text.GetTextThickness() );
    m_textPosX.SetValue( text.GetTextPos().x );
    m_textPosY.SetValue( text.GetTextPos().y );

    m_cbItalic->SetValue( text.IsItalic() );
    EDA_TEXT_HJUSTIFY_T hJustify = text.GetHorizJustify();
    m_cbJustification->SetSelection( (int) hJustify + 1 );
    m_orientValue = text.GetTextAngleDegrees();
    m_cbMirrored->SetValue( text.IsMirrored() );

    m_lineThickness.SetValue( m_dimension->GetLineThickness() );

    return DIALOG_DIMENSION_PROPERTIES_BASE::TransferDataToWindow();
}


bool DIALOG_DIMENSION_PROPERTIES::TransferDataFromWindow()
{
#if 0
    switch( m_DimensionUnitsOpt->GetSelection() )
        {
        case 0:
            dimension->SetUnits( EDA_UNITS::INCHES, false );
            break;
        case 1:
            dimension->SetUnits( EDA_UNITS::INCHES, true );
            break;
        case 2:
            dimension->SetUnits( EDA_UNITS::MILLIMETRES, false );
            break;
        default: break;
        }
#endif
    return true;
}

#if 0
void DIALOG_TEXT_PROPERTIES::OnDimensionTextChange( wxCommandEvent& event )
{
    EDA_UNITS units = EDA_UNITS::UNSCALED;
    bool useMils;

    FetchUnitsFromString( m_DimensionText->GetValue(), units, useMils );

    if( units != EDA_UNITS::UNSCALED )
        m_DimensionUnitsOpt->SetSelection( units == EDA_UNITS::MILLIMETRES ? 2 : useMils ? 1 : 0 );
}

void DIALOG_TEXT_PROPERTIES::OnDimensionUnitsChange( wxCommandEvent& event )
{
    DIMENSION* dimension = (DIMENSION*) m_item;
    EDA_UNITS  units;
    bool useMils;

    // Get default units in case dimension text doesn't contain units.
    dimension->GetUnits( units, useMils );

    double value = ValueFromString( units, m_DimensionText->GetValue(), useMils );

    switch( event.GetSelection() )
    {
    case 0:
        units = EDA_UNITS::INCHES;
        useMils = false;
        break;
    case 1:
        units = EDA_UNITS::INCHES;
        useMils = true;
        break;
    case 2:
        units = EDA_UNITS::MILLIMETRES;
        useMils = false;
        break;
    default: break;
    }

    m_DimensionText->SetValue( StringFromValue( units, value, true, useMils ) );
}
#endif
