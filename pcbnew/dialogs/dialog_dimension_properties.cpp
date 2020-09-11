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

    m_cbOverrideValue->Bind( wxEVT_CHECKBOX,
            [&]( wxCommandEvent& evt )
            {
                m_txtValue->Enable( m_cbOverrideValue->GetValue() );

                if( !m_cbOverrideValue->GetValue() )
                    m_txtValue->SetValue( getValueText() );
            } );

    auto updateEventHandler =
            [&]( wxCommandEvent& evt )
            {
                if( !m_cbOverrideValue->GetValue() )
                    m_txtValue->ChangeValue( getValueText() );

                updatePreviewText();
            };

    m_txtValue->Bind( wxEVT_TEXT, updateEventHandler );
    m_txtPrefix->Bind( wxEVT_TEXT, updateEventHandler );
    m_txtSuffix->Bind( wxEVT_TEXT, updateEventHandler );

    m_cbUnits->Bind( wxEVT_CHOICE, updateEventHandler );
    m_cbUnitsFormat->Bind( wxEVT_CHOICE, updateEventHandler );
    m_cbPrecision->Bind( wxEVT_CHOICE, updateEventHandler );
    m_cbSuppressZeroes->Bind( wxEVT_CHECKBOX, updateEventHandler );

    FinishDialogSettings();
}


DIALOG_DIMENSION_PROPERTIES::~DIALOG_DIMENSION_PROPERTIES()
{
}


bool DIALOG_DIMENSION_PROPERTIES::TransferDataToWindow()
{
    m_txtValue->Enable( m_dimension->GetOverrideValue() );
    m_cbOverrideValue->SetValue( m_dimension->GetOverrideValue() );

    EDA_UNITS  units;
    bool       useMils;
    m_dimension->GetUnits( units, useMils );

    m_cbUnits->SetSelection( units == EDA_UNITS::MILLIMETRES ? 2 : useMils ? 1 : 0 );
    m_cbUnitsFormat->SetSelection( static_cast<int>( m_dimension->GetUnitsFormat() ) );
    m_cbPrecision->SetSelection( static_cast<int>( m_dimension->GetPrecision() ) );

    m_txtPrefix->SetValue( m_dimension->GetPrefix() );
    m_txtSuffix->SetValue( m_dimension->GetSuffix() );

    if( m_cbLayer->SetLayerSelection( m_dimension->GetLayer() ) < 0 )
    {
        wxMessageBox( _( "This item was on a non-existing or forbidden layer.\n"
                         "It has been moved to the first allowed layer." ) );
        m_cbLayer->SetSelection( 0 );
    }

    m_cbSuppressZeroes->SetValue( m_dimension->GetSuppressZeroes() );

    TEXTE_PCB& text = m_dimension->Text();

    m_textWidth.SetValue( text.GetTextSize().x );
    m_textHeight.SetValue( text.GetTextSize().y );
    m_textThickness.SetValue( text.GetTextThickness() );
    m_textPosX.SetValue( text.GetTextPos().x );
    m_textPosY.SetValue( text.GetTextPos().y );
    m_orientValue = text.GetTextAngleDegrees();
    m_cbKeepAligned->SetValue( m_dimension->GetKeepTextAligned() );

    m_cbItalic->SetValue( text.IsItalic() );
    m_cbMirrored->SetValue( text.IsMirrored() );
    EDA_TEXT_HJUSTIFY_T hJustify = text.GetHorizJustify();
    m_cbJustification->SetSelection( (int) hJustify + 1 );

    m_lineThickness.SetValue( m_dimension->GetLineThickness() );
    m_arrowLength.SetValue( m_dimension->GetArrowLength() );

    // Do this last; it depends on the other settings
    m_txtValue->SetValue( getValueText() );

    return DIALOG_DIMENSION_PROPERTIES_BASE::TransferDataToWindow();
}


bool DIALOG_DIMENSION_PROPERTIES::TransferDataFromWindow()
{
    m_dimension->SetOverrideValue( m_cbOverrideValue->GetValue() );

    if( m_cbOverrideValue->GetValue() )
        m_dimension->SetText( m_txtValue->GetValue() );

    m_dimension->SetPrefix( m_txtPrefix->GetValue() );
    m_dimension->SetSuffix( m_txtSuffix->GetValue() );
    m_dimension->SetLayer( static_cast<PCB_LAYER_ID>( m_cbLayer->GetLayerSelection() ) );

    m_dimension->SetUnitsMode( static_cast<DIM_UNITS_MODE>( m_cbUnits->GetSelection() ) );
    m_dimension->SetUnitsFormat( static_cast<DIM_UNITS_FORMAT>( m_cbUnitsFormat->GetSelection() ) );
    m_dimension->SetPrecision( m_cbPrecision->GetSelection() );
    m_dimension->SetSuppressZeroes( m_cbSuppressZeroes->GetValue() );

    TEXTE_PCB& text = m_dimension->Text();

    // TODO(JE) text positioning modes
    wxPoint pos( m_textPosX.GetValue(), m_textPosY.GetValue() );
    text.SetPosition( pos );
    text.SetTextAngle( KiROUND( m_orientValue * 10.0 ) );
    text.SetTextWidth( m_textWidth.GetValue() );
    text.SetTextHeight( m_textHeight.GetValue() );
    text.SetTextThickness( m_textThickness.GetValue() );
    text.SetItalic( m_cbItalic->GetValue() );
    text.SetMirrored( m_cbMirrored->GetValue() );
    int justification = m_cbJustification->GetSelection() - 1;
    text.SetHorizJustify( static_cast<EDA_TEXT_HJUSTIFY_T>( justification ) );
    m_dimension->SetKeepTextAligned( m_cbKeepAligned->GetValue() );

    m_dimension->SetLineThickness( m_lineThickness.GetValue() );
    m_dimension->SetArrowLength( m_arrowLength.GetValue() );

    return true;
}


void DIALOG_DIMENSION_PROPERTIES::getUnitsSelection( EDA_UNITS& aUnits, bool& aUseMils )
{
    aUseMils = false;

    switch( m_cbUnits->GetSelection() )
    {
    case 0: // inches
        aUnits = EDA_UNITS::INCHES;
        break;

    case 1: // mils
        aUnits = EDA_UNITS::INCHES;
        aUseMils = true;
        break;

    case 2: // mm
        aUnits = EDA_UNITS::MILLIMETRES;
        break;

    case 3: // auto
        aUnits = m_frame->GetUserUnits();
        break;

    default:
        break;
    }
}


wxString DIALOG_DIMENSION_PROPERTIES::getValueText()
{
    int val = m_dimension->GetMeasuredValue();
    int precision = m_cbPrecision->GetSelection();

    EDA_UNITS units;
    bool useMils;
    getUnitsSelection( units, useMils );

    wxString text;
    wxString format = wxT( "%." ) + wxString::Format( "%i", precision ) + wxT( "f" );

    text.Printf( format, To_User_Unit( units, val, useMils ) );

    if( m_cbSuppressZeroes->GetValue() )
    {
        while( text[text.Length() - 1] == '0' || text[text.Length() - 1] == '.' )
            text.Remove( text.Length() - 1 );
    }

    return text;
}


void DIALOG_DIMENSION_PROPERTIES::updatePreviewText()
{
    wxString text = m_cbOverrideValue->GetValue() ? m_txtValue->GetValue() : getValueText();

    EDA_UNITS units;
    bool useMils;
    getUnitsSelection( units, useMils );

    switch( m_cbUnitsFormat->GetSelection() )
    {
    case 0: // no units
        break;

    case 1: // normal
        text += " ";
        text += GetAbbreviatedUnitsLabel( units, useMils );
        break;

    case 2: // parenthetical
        text += " (";
        text += GetAbbreviatedUnitsLabel( units, useMils );
        text += ")";
        break;
    }

    text.Prepend( m_txtPrefix->GetValue() );
    text.Append( m_txtSuffix->GetValue() );

    m_staticTextPreview->SetLabel( text );
}
