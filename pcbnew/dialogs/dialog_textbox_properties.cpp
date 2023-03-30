/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/bitmap_button.h>
#include <widgets/font_choice.h>
#include <dialog_textbox_properties.h>
#include <confirm.h>
#include <widgets/unit_binder.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <board.h>
#include <footprint.h>
#include <string_utils.h>
#include <pcb_textbox.h>
#include <pcbnew.h>
#include <pcb_edit_frame.h>
#include <pcb_layer_box_selector.h>
#include <math/util.h>      // for KiROUND
#include <scintilla_tricks.h>
#include "macros.h"

DIALOG_TEXTBOX_PROPERTIES::DIALOG_TEXTBOX_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent,
                                                      PCB_TEXTBOX* aTextBox ) :
        DIALOG_TEXTBOX_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_textBox( aTextBox ),
        m_textWidth( aParent, m_SizeXLabel, m_SizeXCtrl, m_SizeXUnits ),
        m_textHeight( aParent, m_SizeYLabel, m_SizeYCtrl, m_SizeYUnits ),
        m_thickness( aParent, m_ThicknessLabel, m_ThicknessCtrl, m_ThicknessUnits ),
        m_orientation( aParent, m_OrientLabel, m_OrientCtrl, nullptr ),
        m_borderWidth( aParent, m_borderWidthLabel, m_borderWidthCtrl, m_borderWidthUnits )
{
    m_MultiLineText->SetEOLMode( wxSTC_EOL_LF );

    m_scintillaTricks = new SCINTILLA_TRICKS( m_MultiLineText, wxT( "{}" ), false,
            [this]()
            {
                wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
            } );

    // A hack which causes Scintilla to auto-size the text editor canvas
    // See: https://github.com/jacobslusser/ScintillaNET/issues/216
    m_MultiLineText->SetScrollWidth( 1 );
    m_MultiLineText->SetScrollWidthTracking( true );

    if( m_textBox->GetParentFootprint() )
    {
        // Do not allow locking items in the footprint editor
        m_cbLocked->Show( false );
    }

    SetInitialFocus( m_MultiLineText );

    m_separator0->SetIsSeparator();

    m_bold->SetIsCheckButton();
    m_bold->SetBitmap( KiBitmap( BITMAPS::text_bold ) );
    m_italic->SetIsCheckButton();
    m_italic->SetBitmap( KiBitmap( BITMAPS::text_italic ) );

    m_separator1->SetIsSeparator();

    m_alignLeft->SetIsRadioButton();
    m_alignLeft->SetBitmap( KiBitmap( BITMAPS::text_align_left ) );
    m_alignCenter->SetIsRadioButton();
    m_alignCenter->SetBitmap( KiBitmap( BITMAPS::text_align_center ) );
    m_alignRight->SetIsRadioButton();
    m_alignRight->SetBitmap( KiBitmap( BITMAPS::text_align_right ) );

    m_separator2->SetIsSeparator();

    m_mirrored->SetIsCheckButton();
    m_mirrored->SetBitmap( KiBitmap( BITMAPS::text_mirrored ) );

    m_separator3->SetIsSeparator();

    // Configure the layers list selector.  Note that footprints are built outside the current
    // board and so we may need to show all layers if the text is on an unactivated layer.
    if( !m_frame->GetBoard()->IsLayerEnabled( m_textBox->GetLayer() ) )
        m_LayerSelectionCtrl->ShowNonActivatedLayers( true );

    m_LayerSelectionCtrl->SetLayersHotkeys( false );
    m_LayerSelectionCtrl->SetBoardFrame( m_frame );
    m_LayerSelectionCtrl->Resync();

    m_orientation.SetUnits( EDA_UNITS::DEGREES );
    m_orientation.SetPrecision( 3 );

    // Set predefined rotations in combo dropdown, according to the locale floating point
    // separator notation
    double rot_list[] = { 0.0, 90.0, -90.0, 180.0 };

    for( size_t ii = 0; ii < m_OrientCtrl->GetCount() && ii < 4; ++ii )
        m_OrientCtrl->SetString( ii, wxString::Format( "%.1f", rot_list[ii] ) );

    for( const std::pair<const PLOT_DASH_TYPE, lineTypeStruct>& typeEntry : lineTypeNames )
        m_borderStyleCombo->Append( typeEntry.second.name, KiBitmap( typeEntry.second.bitmap ) );

    m_borderStyleCombo->Append( DEFAULT_STYLE );

    SetupStandardButtons();

    // wxTextCtrls fail to generate wxEVT_CHAR events when the wxTE_MULTILINE flag is set,
    // so we have to listen to wxEVT_CHAR_HOOK events instead.
    Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_TEXTBOX_PROPERTIES::OnCharHook ),
             nullptr, this );

    finishDialogSettings();
}


DIALOG_TEXTBOX_PROPERTIES::~DIALOG_TEXTBOX_PROPERTIES()
{
    Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_TEXTBOX_PROPERTIES::OnCharHook ),
                nullptr, this );

    delete m_scintillaTricks;
}


int PCB_BASE_EDIT_FRAME::ShowTextBoxPropertiesDialog( PCB_TEXTBOX* aTextBox )
{
    DIALOG_TEXTBOX_PROPERTIES dlg( this, aTextBox );
    return dlg.ShowQuasiModal();
}


bool DIALOG_TEXTBOX_PROPERTIES::TransferDataToWindow()
{
    BOARD*   board = m_frame->GetBoard();
    wxString converted = board->ConvertKIIDsToCrossReferences(
                                                        UnescapeString( m_textBox->GetText() ) );

    m_MultiLineText->SetValue( converted );
    m_MultiLineText->SetSelection( -1, -1 );
    m_MultiLineText->EmptyUndoBuffer();

    m_cbLocked->SetValue( m_textBox->IsLocked() );

    m_LayerSelectionCtrl->SetLayerSelection( m_textBox->GetLayer() );

    m_fontCtrl->SetFontSelection( m_textBox->GetFont() );

    m_textWidth.SetValue( m_textBox->GetTextSize().x );
    m_textHeight.SetValue( m_textBox->GetTextSize().y );
    m_thickness.SetValue( m_textBox->GetTextThickness() );

    m_bold->Check( m_textBox->IsBold() );
    m_italic->Check( m_textBox->IsItalic() );

    switch ( m_textBox->GetHorizJustify() )
    {
    case GR_TEXT_H_ALIGN_LEFT:   m_alignLeft->Check( true );   break;
    case GR_TEXT_H_ALIGN_CENTER: m_alignCenter->Check( true ); break;
    case GR_TEXT_H_ALIGN_RIGHT:  m_alignRight->Check( true );  break;
    }

    m_mirrored->Check( m_textBox->IsMirrored() );

    EDA_ANGLE orientation = m_textBox->GetTextAngle();
    m_orientation.SetAngleValue( orientation.Normalize180() );

    STROKE_PARAMS stroke = m_textBox->GetStroke();
    m_borderCheckbox->SetValue( stroke.GetWidth() >= 0 );

    if( stroke.GetWidth() >= 0 )
        m_borderWidth.SetValue( stroke.GetWidth() );

    PLOT_DASH_TYPE style = stroke.GetPlotStyle();

    if( style == PLOT_DASH_TYPE::DEFAULT )
        style = PLOT_DASH_TYPE::SOLID;

    if( (int) style < (int) lineTypeNames.size() )
        m_borderStyleCombo->SetSelection( (int) style );

    m_borderWidth.Enable( stroke.GetWidth() >= 0 );
    m_borderStyleLabel->Enable( stroke.GetWidth() >= 0 );
    m_borderStyleCombo->Enable( stroke.GetWidth() >= 0 );

    return DIALOG_TEXTBOX_PROPERTIES_BASE::TransferDataToWindow();
}


void DIALOG_TEXTBOX_PROPERTIES::onFontSelected( wxCommandEvent & aEvent )
{
    if( KIFONT::FONT::IsStroke( aEvent.GetString() ) )
    {
        m_thickness.Show( true );

        int textSize = std::min( m_textWidth.GetValue(), m_textHeight.GetValue() );
        int thickness = m_thickness.GetValue();

        m_bold->Check( abs( thickness - GetPenSizeForBold( textSize ) )
                        < abs( thickness - GetPenSizeForNormal( textSize ) ) );
    }
    else
    {
        m_thickness.Show( false );
    }
}


void DIALOG_TEXTBOX_PROPERTIES::onBoldToggle( wxCommandEvent & aEvent )
{
    int textSize = std::min( m_textWidth.GetValue(), m_textHeight.GetValue() );

    if( aEvent.IsChecked() )
        m_thickness.ChangeValue( GetPenSizeForBold( textSize ) );
    else
        m_thickness.ChangeValue( GetPenSizeForNormal( textSize ) );

    aEvent.Skip();
}


void DIALOG_TEXTBOX_PROPERTIES::onAlignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_alignLeft, m_alignCenter, m_alignRight } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


void DIALOG_TEXTBOX_PROPERTIES::onThickness( wxCommandEvent& event )
{
    int textSize = std::min( m_textWidth.GetValue(), m_textHeight.GetValue() );
    int thickness = m_thickness.GetValue();

    m_bold->Check( abs( thickness - GetPenSizeForBold( textSize ) )
                    < abs( thickness - GetPenSizeForNormal( textSize ) ) );
}


void DIALOG_TEXTBOX_PROPERTIES::onBorderChecked( wxCommandEvent& event )
{
    bool border = m_borderCheckbox->GetValue();

    if( border && m_borderWidth.GetValue() <= 0 )
    {
        BOARD_DESIGN_SETTINGS& bds = m_textBox->GetBoard()->GetDesignSettings();
        m_borderWidth.SetValue( bds.GetLineThickness( m_textBox->GetLayer() ) );
    }

    m_borderWidth.Enable( border );
    m_borderStyleLabel->Enable( border );
    m_borderStyleCombo->Enable( border );
}


bool DIALOG_TEXTBOX_PROPERTIES::TransferDataFromWindow()
{
    if( !DIALOG_TEXTBOX_PROPERTIES_BASE::TransferDataFromWindow() )
        return false;

    if( !m_textWidth.Validate( TEXTS_MIN_SIZE, TEXTS_MAX_SIZE )
        || !m_textHeight.Validate( TEXTS_MIN_SIZE, TEXTS_MAX_SIZE ) )
    {
        return false;
    }

    BOARD_COMMIT commit( m_frame );
    commit.Modify( m_textBox );

    // If no other command in progress, prepare undo command
    // (for a command in progress, will be made later, at the completion of command)
    bool pushCommit = ( m_textBox->GetEditFlags() == 0 );

    // Set IN_EDIT flag to force undo/redo/abort proper operation and avoid new calls to
    // SaveCopyInUndoList for the same text if is moved, and then rotated, edited, etc....
    if( !pushCommit )
        m_textBox->SetFlags( IN_EDIT );

    BOARD*   board = m_frame->GetBoard();
    wxString txt = board->ConvertCrossReferencesToKIIDs( m_MultiLineText->GetValue() );

#ifdef __WXMAC__
    // On macOS CTRL+Enter produces '\r' instead of '\n' regardless of EOL setting.
    // Replace it now.
    txt.Replace( "\r", "\n" );
#elif defined( __WINDOWS__ )
    // On Windows, a new line is coded as \r\n.  We use only \n in kicad files and in
    // drawing routines so strip the \r char.
    txt.Replace( "\r", "" );
#endif

    m_textBox->SetText( EscapeString( txt, CTX_QUOTED_STR ) );
    m_textBox->SetLocked( m_cbLocked->GetValue() );
    m_textBox->SetLayer( ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) );

    if( m_fontCtrl->HaveFontSelection() )
    {
        m_textBox->SetFont( m_fontCtrl->GetFontSelection( m_bold->IsChecked(),
                                                          m_italic->IsChecked() ) );
    }

    m_textBox->SetTextSize( VECTOR2I( m_textWidth.GetValue(), m_textHeight.GetValue() ) );
    m_textBox->SetTextThickness( m_thickness.GetValue() );

    // Test for acceptable values for thickness and size and clamp if fails
    int maxPenWidth = Clamp_Text_PenSize( m_textBox->GetTextThickness(), m_textBox->GetTextSize() );

    if( m_textBox->GetTextThickness() > maxPenWidth )
    {
        DisplayError( this, _( "The text thickness is too large for the text size.\n"
                               "It will be clamped." ) );
        m_textBox->SetTextThickness( maxPenWidth );
    }

    m_textBox->SetTextAngle( m_orientation.GetAngleValue().Normalize() );
    m_textBox->SetBold( m_bold->IsChecked() );
    m_textBox->SetItalic( m_italic->IsChecked() );

    if( m_alignLeft->IsChecked() )
        m_textBox->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    else if( m_alignCenter->IsChecked() )
        m_textBox->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
    else
        m_textBox->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );

    m_textBox->SetMirrored( m_mirrored->IsChecked() );

    STROKE_PARAMS stroke = m_textBox->GetStroke();

    if( m_borderCheckbox->GetValue() )
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

    m_textBox->SetStroke( stroke );

    m_textBox->ClearBoundingBoxCache();
    m_textBox->ClearRenderCache();

    if( pushCommit )
        commit.Push( _( "Change text box properties" ) );

    return true;
}


void DIALOG_TEXTBOX_PROPERTIES::onMultiLineTCLostFocus( wxFocusEvent& event )
{
    if( m_scintillaTricks )
        m_scintillaTricks->CancelAutocomplete();

    event.Skip();
}
