/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
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

#include <widgets/bitmap_button.h>
#include <widgets/font_choice.h>
#include <board.h>
#include <board_commit.h>
#include <pcb_dimension.h>
#include <pcb_base_edit_frame.h>
#include <pcb_layer_box_selector.h>
#include <widgets/unit_binder.h>
#include <wx/msgdlg.h>

#include "dialog_dimension_properties.h"



DIALOG_DIMENSION_PROPERTIES::DIALOG_DIMENSION_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent,
                                                          BOARD_ITEM* aItem ) :
        DIALOG_DIMENSION_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_cbLayerActual( m_cbLayer ),
        m_txtValueActual( m_txtValue ),
        m_textWidth( aParent, m_lblTextWidth, m_txtTextWidth, m_lblTextWidthUnits ),
        m_textHeight( aParent, m_lblTextHeight, m_txtTextHeight, m_lblTextHeightUnits ),
        m_textThickness( aParent, m_lblTextThickness, m_txtTextThickness, m_lblTextThicknessUnits ),
        m_textPosX( aParent, m_lblTextPosX, m_txtTextPosX, m_lblTextPosXUnits ),
        m_textPosY( aParent, m_lblTextPosY, m_txtTextPosY, m_lblTextPosYUnits ),
        m_orientation( aParent, m_lblTextOrientation, m_cbTextOrientation, nullptr ),
        m_lineThickness( aParent, m_lblLineThickness, m_txtLineThickness, m_lblLineThicknessUnits ),
        m_arrowLength( aParent, m_lblArrowLength, m_txtArrowLength, m_lblArrowLengthUnits ),
        m_extensionOffset( aParent, m_lblExtensionOffset, m_txtExtensionOffset, m_lblExtensionOffsetUnits ),
        m_extensionOvershoot( aParent, m_lblExtensionOvershoot, m_txtExtensionOvershoot, m_lblExtensionOvershootUnits )
{
    wxASSERT( BaseType( aItem->Type() ) == PCB_DIMENSION_T );
    m_dimension = static_cast<PCB_DIMENSION_BASE*>( aItem );
    m_previewDimension = static_cast<PCB_DIMENSION_BASE*>( m_dimension->Clone() );
    m_previewDimension->SetParent( m_frame->GetBoard() );

    switch( m_dimension->Type() )
    {
    case PCB_DIM_LEADER_T:
        // Hide the main format controls and keep the leader controls shown
        m_sizerFormat->GetStaticBox()->Hide();
        m_sizerCenter->GetStaticBox()->Hide();

        m_cbLayerActual = m_cbLeaderLayer;
        m_txtValueActual = m_txtLeaderValue;

        // Remove a fewings from text format
        m_lblTextPositionMode->Hide();
        m_cbTextPositionMode->Hide();
        break;

    case PCB_DIM_CENTER_T:
        m_sizerLeader->GetStaticBox()->Hide();
        m_sizerFormat->GetStaticBox()->Hide();
        m_sizerText->GetStaticBox()->Hide();

        m_lblArrowLength->Hide();
        m_txtArrowLength->Hide();
        m_lblArrowLengthUnits->Hide();

        m_lblExtensionOffset->Hide();
        m_txtExtensionOffset->Hide();
        m_lblExtensionOffsetUnits->Hide();

        m_cbLayerActual = m_cbCenterLayer;
        break;

    default:
        m_sizerLeader->GetStaticBox()->Hide();
        m_sizerCenter->GetStaticBox()->Hide();
        break;
    }

    m_separator0->SetIsSeparator();

    m_bold->SetIsCheckButton();
    m_bold->SetBitmap( KiBitmapBundle( BITMAPS::text_bold ) );
    m_italic->SetIsCheckButton();
    m_italic->SetBitmap( KiBitmapBundle( BITMAPS::text_italic ) );

    m_separator1->SetIsSeparator();

    m_alignLeft->SetIsRadioButton();
    m_alignLeft->SetBitmap( KiBitmapBundle( BITMAPS::text_align_left ) );
    m_alignCenter->SetIsRadioButton();
    m_alignCenter->SetBitmap( KiBitmapBundle( BITMAPS::text_align_center ) );
    m_alignRight->SetIsRadioButton();
    m_alignRight->SetBitmap( KiBitmapBundle( BITMAPS::text_align_right ) );

    m_separator2->SetIsSeparator();

    m_mirrored->SetIsCheckButton();
    m_mirrored->SetBitmap( KiBitmapBundle( BITMAPS::text_mirrored ) );

    m_separator3->SetIsSeparator();

    // Fix the size after hiding/showing some of the properties
    Layout();

    // Configure display origin transforms
    m_textPosX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_textPosY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    // Configure the layers list selector.  Note that footprints are built outside the current
    // board and so we may need to show all layers if the text is on an unactivated layer.
    if( !m_frame->GetBoard()->IsLayerEnabled( m_dimension->GetLayer() ) )
        m_cbLayerActual->ShowNonActivatedLayers( true );

    m_cbLayerActual->SetLayersHotkeys( false );
    m_cbLayerActual->SetBoardFrame( aParent );
    m_cbLayerActual->Resync();

    m_orientation.SetUnits( EDA_UNITS::DEGREES );
    m_orientation.SetPrecision( 3 );

    // Set predefined rotations in combo dropdown, according to the locale floating point
    // separator notation
    double rot_list[] = { 0.0, 90.0, -90.0, 180.0 };

    for( size_t ii = 0; ii < m_cbTextOrientation->GetCount() && ii < 4; ++ii )
        m_cbTextOrientation->SetString( ii, wxString::Format( "%.1f", rot_list[ii] ) );

    m_cbOverrideValue->Bind( wxEVT_CHECKBOX,
            [&]( wxCommandEvent& evt )
            {
                m_txtValue->Enable( m_cbOverrideValue->GetValue() );

                if( !m_cbOverrideValue->GetValue() )
                    m_txtValue->SetValue( m_dimension->GetValueText() );
            } );

    auto updateEventHandler =
            [&]( wxCommandEvent& evt )
            {
                if( !m_cbOverrideValue->GetValue() )
                    m_txtValue->ChangeValue( m_dimension->GetValueText() );

                updatePreviewText();
            };

    // No need to use m_txtValueActual here since we don't have previewing for leaders
    m_txtValue->Bind( wxEVT_TEXT, updateEventHandler );
    m_txtPrefix->Bind( wxEVT_TEXT, updateEventHandler );
    m_txtSuffix->Bind( wxEVT_TEXT, updateEventHandler );

    m_cbUnits->Bind( wxEVT_CHOICE, updateEventHandler );
    m_cbUnitsFormat->Bind( wxEVT_CHOICE, updateEventHandler );
    m_cbPrecision->Bind( wxEVT_CHOICE, updateEventHandler );
    m_cbSuppressZeroes->Bind( wxEVT_CHECKBOX, updateEventHandler );

    m_cbTextPositionMode->Bind( wxEVT_CHOICE,
            [&]( wxCommandEvent& aEvt )
            {
                // manual mode
                bool allowPositioning = ( m_cbTextPositionMode->GetSelection() == 2 );

                m_txtTextPosX->Enable( allowPositioning );
                m_txtTextPosY->Enable( allowPositioning );
            } );

    m_cbKeepAligned->Bind( wxEVT_CHECKBOX,
            [&]( wxCommandEvent& aEvt )
            {
                m_cbTextOrientation->Enable( !m_cbKeepAligned->GetValue() );
            } );

    SetupStandardButtons();

    finishDialogSettings();
}


DIALOG_DIMENSION_PROPERTIES::~DIALOG_DIMENSION_PROPERTIES()
{
    delete m_previewDimension;
}


bool DIALOG_DIMENSION_PROPERTIES::TransferDataToWindow()
{
    BOARD*  board = m_frame->GetBoard();

    m_txtValue->Enable( m_dimension->GetOverrideTextEnabled() );
    m_cbOverrideValue->SetValue( m_dimension->GetOverrideTextEnabled() );

    switch( m_dimension->GetUnitsMode() )
    {
        case DIM_UNITS_MODE::INCH:      m_cbUnits->SetSelection( 0 ); break;
        case DIM_UNITS_MODE::MILS:      m_cbUnits->SetSelection( 1 ); break;
        case DIM_UNITS_MODE::MM:        m_cbUnits->SetSelection( 2 ); break;
        case DIM_UNITS_MODE::AUTOMATIC: m_cbUnits->SetSelection( 3 ); break;
    }

    switch ( m_dimension->GetArrowDirection() )
    {
        case DIM_ARROW_DIRECTION::INWARD:  m_cbArrowDirection->SetSelection( 0 ); break;
        case DIM_ARROW_DIRECTION::OUTWARD: m_cbArrowDirection->SetSelection( 1 ); break;
    }

    m_cbUnitsFormat->SetSelection( static_cast<int>( m_dimension->GetUnitsFormat() ) );
    m_cbPrecision->SetSelection( static_cast<int>( m_dimension->GetPrecision() ) );

    m_txtPrefix->SetValue( board->ConvertKIIDsToCrossReferences( m_dimension->GetPrefix() ) );
    m_txtSuffix->SetValue( board->ConvertKIIDsToCrossReferences( m_dimension->GetSuffix() ) );

    if( m_cbLayerActual->SetLayerSelection( m_dimension->GetLayer() ) < 0 )
    {
        wxMessageBox( _( "This item was on a non-existing layer.\n"
                         "It has been moved to the first defined layer." ) );
        m_cbLayerActual->SetSelection( 0 );
    }

    m_cbSuppressZeroes->SetValue( m_dimension->GetSuppressZeroes() );

    m_fontCtrl->SetFontSelection( m_dimension->GetFont() );

    m_textWidth.SetValue( m_dimension->GetTextSize().x );
    m_textHeight.SetValue( m_dimension->GetTextSize().y );
    m_textThickness.SetValue( m_dimension->GetTextThickness() );

    m_textPosX.SetValue( m_dimension->GetTextPos().x );
    m_textPosY.SetValue( m_dimension->GetTextPos().y );
    m_cbTextPositionMode->SetSelection( static_cast<int>( m_dimension->GetTextPositionMode() ) );

    if( m_dimension->GetTextPositionMode() != DIM_TEXT_POSITION::MANUAL )
    {
        m_txtTextPosX->Disable();
        m_txtTextPosY->Disable();
    }

    EDA_ANGLE orientation = m_dimension->GetTextAngle();
    m_orientation.SetAngleValue( orientation.Normalize180() );
    m_cbTextOrientation->Enable( !m_dimension->GetKeepTextAligned() );
    m_cbKeepAligned->SetValue( m_dimension->GetKeepTextAligned() );

    m_bold->Check( m_dimension->IsBold() );
    m_italic->Check( m_dimension->IsItalic() );

    switch ( m_dimension->GetHorizJustify() )
    {
    case GR_TEXT_H_ALIGN_LEFT:          m_alignLeft->Check( true );   break;
    case GR_TEXT_H_ALIGN_CENTER:        m_alignCenter->Check( true ); break;
    case GR_TEXT_H_ALIGN_RIGHT:         m_alignRight->Check( true );  break;
    case GR_TEXT_H_ALIGN_INDETERMINATE:                               break;
    }

    m_mirrored->Check( m_dimension->IsMirrored() );

    m_lineThickness.SetValue( m_dimension->GetLineThickness() );
    m_arrowLength.SetValue( m_dimension->GetArrowLength() );
    m_extensionOffset.SetValue( m_dimension->GetExtensionOffset() );

    if( PCB_DIM_ALIGNED* aligned = dynamic_cast<PCB_DIM_ALIGNED*>( m_dimension ) )
        m_extensionOvershoot.SetValue( aligned->GetExtensionHeight() );
    else
        m_extensionOvershoot.Show( false );

    // Do this last; it depends on the other settings
    if( m_dimension->GetOverrideTextEnabled() )
    {
        wxString txt = board->ConvertKIIDsToCrossReferences( m_dimension->GetOverrideText() );
        m_txtValueActual->SetValue( txt );
    }
    else
    {
        m_txtValueActual->SetValue( m_dimension->GetValueText() );
    }

    if( PCB_DIM_LEADER* leader = static_cast<PCB_DIM_LEADER*>( m_dimension ) )
        m_cbTextFrame->SetSelection( static_cast<int>( leader->GetTextBorder() ) );

    return DIALOG_DIMENSION_PROPERTIES_BASE::TransferDataToWindow();
}


bool DIALOG_DIMENSION_PROPERTIES::TransferDataFromWindow()
{
    if( !DIALOG_DIMENSION_PROPERTIES_BASE::TransferDataFromWindow() )
        return false;

    BOARD_COMMIT commit( m_frame );
    commit.Modify( m_dimension );

    // If no other command in progress, prepare undo command
    // (for a command in progress, will be made later, at the completion of command)
    bool pushCommit = ( m_dimension->GetEditFlags() == 0 );

    /* set flag in edit to force undo/redo/abort proper operation,
     * and avoid new calls to SaveCopyInUndoList for the same dimension
     * this can occurs when a dimension is moved, and then rotated, edited ..
    */
    if( !pushCommit )
        m_dimension->SetFlags( IN_EDIT );

    updateDimensionFromDialog( m_dimension );

    if( pushCommit )
        commit.Push( _( "Edit Dimension Properties" ) );

    return true;
}


void DIALOG_DIMENSION_PROPERTIES::onFontSelected( wxCommandEvent & aEvent )
{
    if( KIFONT::FONT::IsStroke( aEvent.GetString() ) )
    {
        m_textThickness.Show( true );

        int textSize = std::min( m_textWidth.GetValue(), m_textHeight.GetValue() );
        int thickness = m_textThickness.GetValue();

        m_bold->Check( abs( thickness - GetPenSizeForBold( textSize ) )
                        < abs( thickness - GetPenSizeForNormal( textSize ) ) );
    }
    else
    {
        m_textThickness.Show( false );
    }
}


void DIALOG_DIMENSION_PROPERTIES::onBoldToggle( wxCommandEvent & aEvent )
{
    int textSize = std::min( m_textWidth.GetValue(), m_textHeight.GetValue() );

    if( aEvent.IsChecked() )
        m_textThickness.ChangeValue( GetPenSizeForBold( textSize ) );
    else
        m_textThickness.ChangeValue( GetPenSizeForNormal( textSize ) );

    aEvent.Skip();
}


void DIALOG_DIMENSION_PROPERTIES::onAlignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_alignLeft, m_alignCenter, m_alignRight } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


void DIALOG_DIMENSION_PROPERTIES::onThickness( wxCommandEvent& event )
{
    int textSize = std::min( m_textWidth.GetValue(), m_textHeight.GetValue() );
    int thickness = m_textThickness.GetValue();

    m_bold->Check( abs( thickness - GetPenSizeForBold( textSize ) )
                    < abs( thickness - GetPenSizeForNormal( textSize ) ) );
}


void DIALOG_DIMENSION_PROPERTIES::updateDimensionFromDialog( PCB_DIMENSION_BASE* aTarget )
{
    BOARD* board = m_frame->GetBoard();

    aTarget->SetOverrideTextEnabled( m_cbOverrideValue->GetValue() );

    if( m_cbOverrideValue->GetValue() )
    {
        wxString txt = board->ConvertCrossReferencesToKIIDs( m_txtValueActual->GetValue() );
        aTarget->SetOverrideText( txt );
    }

    aTarget->SetPrefix( board->ConvertCrossReferencesToKIIDs( m_txtPrefix->GetValue() ) );
    aTarget->SetSuffix( board->ConvertCrossReferencesToKIIDs( m_txtSuffix->GetValue() ) );
    aTarget->SetLayer( static_cast<PCB_LAYER_ID>( m_cbLayerActual->GetLayerSelection() ) );

    switch ( m_cbArrowDirection->GetSelection() ) {
        case 0: aTarget->SetArrowDirection( DIM_ARROW_DIRECTION::INWARD );  break;
        case 1: aTarget->SetArrowDirection( DIM_ARROW_DIRECTION::OUTWARD ); break;
    }

    switch( m_cbUnits->GetSelection() )
    {
    case 0: aTarget->SetUnitsMode( DIM_UNITS_MODE::INCH );      break;
    case 1: aTarget->SetUnitsMode( DIM_UNITS_MODE::MILS );      break;
    case 2: aTarget->SetUnitsMode( DIM_UNITS_MODE::MM );        break;
    case 3: aTarget->SetUnitsMode( DIM_UNITS_MODE::AUTOMATIC ); break;
    }

    aTarget->SetUnitsFormat( static_cast<DIM_UNITS_FORMAT>( m_cbUnitsFormat->GetSelection() ) );
    aTarget->SetPrecision( static_cast<DIM_PRECISION>( m_cbPrecision->GetSelection() ) );
    aTarget->SetSuppressZeroes( m_cbSuppressZeroes->GetValue() );

    DIM_TEXT_POSITION tpm = static_cast<DIM_TEXT_POSITION>( m_cbTextPositionMode->GetSelection() );
    aTarget->SetTextPositionMode( tpm );

    if( tpm == DIM_TEXT_POSITION::MANUAL )
    {
        VECTOR2I pos( m_textPosX.GetValue(), m_textPosY.GetValue() );
        aTarget->SetTextPos( pos );
    }

    aTarget->SetKeepTextAligned( m_cbKeepAligned->GetValue() );

    aTarget->SetTextAngle( m_orientation.GetAngleValue().Normalize() );
    aTarget->SetTextWidth( m_textWidth.GetValue() );
    aTarget->SetTextHeight( m_textHeight.GetValue() );
    aTarget->SetTextThickness( m_textThickness.GetValue() );

    if( m_fontCtrl->HaveFontSelection() )
        aTarget->SetFont( m_fontCtrl->GetFontSelection( m_bold->IsChecked(), m_italic->IsChecked() ) );

    // Must come after SetTextWidth/Height()
    aTarget->SetBold( m_bold->IsChecked() );
    aTarget->SetItalic( m_italic->IsChecked() );

    if( m_alignLeft->IsChecked() )
        aTarget->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    else if( m_alignCenter->IsChecked() )
        aTarget->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
    else
        aTarget->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );

    aTarget->SetMirrored( m_mirrored->IsChecked() );

    aTarget->SetLineThickness( m_lineThickness.GetValue() );
    aTarget->SetArrowLength( m_arrowLength.GetValue() );
    aTarget->SetExtensionOffset( m_extensionOffset.GetValue() );

    if( PCB_DIM_ALIGNED* aligned = dynamic_cast<PCB_DIM_ALIGNED*>( aTarget ) )
        aligned->SetExtensionHeight( m_extensionOvershoot.GetValue() );

    if( PCB_DIM_LEADER* leader = dynamic_cast<PCB_DIM_LEADER*>( aTarget ) )
        leader->SetTextBorder( static_cast<DIM_TEXT_BORDER>( m_cbTextFrame->GetSelection()));

    aTarget->Update();
}


void DIALOG_DIMENSION_PROPERTIES::updatePreviewText()
{
    updateDimensionFromDialog( m_previewDimension );
    m_staticTextPreview->SetLabel( m_previewDimension->GetShownText( true ) );
}
