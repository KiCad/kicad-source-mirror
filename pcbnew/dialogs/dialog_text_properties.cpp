/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2018 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 2010-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <dialog_text_properties.h>
#include <confirm.h>
#include <board_commit.h>
#include <board.h>
#include <footprint.h>
#include <pcb_text.h>
#include <fp_text.h>
#include <pcbnew.h>
#include <project.h>
#include <pcb_edit_frame.h>
#include <pcb_layer_box_selector.h>
#include <wx/valnum.h>
#include <scintilla_tricks.h>


DIALOG_TEXT_PROPERTIES::DIALOG_TEXT_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, BOARD_ITEM* aItem ) :
        DIALOG_TEXT_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_item( aItem ),
        m_edaText( nullptr ),
        m_fpText( nullptr ),
        m_pcbText( nullptr ),
        m_textWidth( aParent, m_SizeXLabel, m_SizeXCtrl, m_SizeXUnits ),
        m_textHeight( aParent, m_SizeYLabel, m_SizeYCtrl, m_SizeYUnits ),
        m_thickness( aParent, m_ThicknessLabel, m_ThicknessCtrl, m_ThicknessUnits ),
        m_posX( aParent, m_PositionXLabel, m_PositionXCtrl, m_PositionXUnits ),
        m_posY( aParent, m_PositionYLabel, m_PositionYCtrl, m_PositionYUnits ),
        m_orientation( aParent, m_OrientLabel, m_OrientCtrl, nullptr )
{
    wxString title;

    // Configure display origin transforms
    m_posX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_posY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    m_MultiLineText->SetEOLMode( wxSTC_EOL_LF );

#ifdef _WIN32
    // Without this setting, on Windows, some esoteric unicode chars create display issue
    // in a wxStyledTextCtrl.
    // for SetTechnology() info, see https://www.scintilla.org/ScintillaDoc.html#SCI_SETTECHNOLOGY
    m_MultiLineText->SetTechnology(wxSTC_TECHNOLOGY_DIRECTWRITE);
#endif

    m_scintillaTricks = new SCINTILLA_TRICKS( m_MultiLineText, wxT( "{}" ), false,
            [this]()
            {
                wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
            } );

    // A hack which causes Scintilla to auto-size the text editor canvas
    // See: https://github.com/jacobslusser/ScintillaNET/issues/216
    m_MultiLineText->SetScrollWidth( 1 );
    m_MultiLineText->SetScrollWidthTracking( true );

    if( m_item->Type() == PCB_FP_TEXT_T )
    {
        m_fpText = (FP_TEXT*) m_item;
        m_edaText = static_cast<EDA_TEXT*>( m_fpText );

        switch( m_fpText->GetType() )
        {
        case FP_TEXT::TEXT_is_REFERENCE: title = _( "Footprint Reference Properties" ); break;
        case FP_TEXT::TEXT_is_VALUE:     title = _( "Footprint Value Properties" );     break;
        case FP_TEXT::TEXT_is_DIVERS:    title = _( "Footprint Text Properties" );      break;
        }

        switch( m_fpText->GetType() )
        {
        case FP_TEXT::TEXT_is_REFERENCE: m_TextLabel->SetLabel( _( "Reference:" ) ); break;
        case FP_TEXT::TEXT_is_VALUE:     m_TextLabel->SetLabel( _( "Value:" ) );     break;
        case FP_TEXT::TEXT_is_DIVERS:    m_TextLabel->SetLabel( _( "Text:" ) );      break;
        }

        SetInitialFocus( m_SingleLineText );
        m_MultiLineSizer->Show( false );

        // Do not allow locking items in the footprint editor
        m_cbLocked->Show( false );

        m_tabOrder = {
            m_SingleLineText,
            m_LayerSelectionCtrl,
            m_SizeXCtrl,
            m_SizeYCtrl,
            m_ThicknessCtrl,
            m_Visible,
            m_cbKnockout,
            m_KeepUpright,
            m_PositionXCtrl,
            m_PositionYCtrl,
            m_OrientCtrl,
            m_sdbSizerOK,
            m_sdbSizerCancel
        };
    }
    else
    {
        title = _( "Text Properties" );

        m_pcbText = (PCB_TEXT*) aItem;
        m_edaText = static_cast<EDA_TEXT*>( m_pcbText );

        SetInitialFocus( m_MultiLineText );
        m_SingleLineSizer->Show( false );

        // This option makes sense only for footprint texts; texts on board are always visible.
        m_Visible->SetValue( true );
        m_Visible->Show( false );

        m_KeepUpright->Show( false );
        m_statusLine->Show( false );

        m_tabOrder = {
            m_MultiLineText,
            m_cbLocked,
            m_LayerSelectionCtrl,
            m_SizeXCtrl,
            m_SizeYCtrl,
            m_ThicknessCtrl,
            m_cbKnockout,
            m_PositionXCtrl,
            m_PositionYCtrl,
            m_OrientCtrl,
            m_sdbSizerOK,
            m_sdbSizerCancel
        };
    }

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

    m_valignBottom->SetIsRadioButton();
    m_valignBottom->SetBitmap( KiBitmap( BITMAPS::text_valign_bottom ) );
    m_valignCenter->SetIsRadioButton();
    m_valignCenter->SetBitmap( KiBitmap( BITMAPS::text_valign_center ) );
    m_valignTop->SetIsRadioButton();
    m_valignTop->SetBitmap( KiBitmap( BITMAPS::text_valign_top ) );

    m_separator3->SetIsSeparator();

    m_mirrored->SetIsCheckButton();
    m_mirrored->SetBitmap( KiBitmap( BITMAPS::text_mirrored ) );

    SetTitle( title );
    m_hash_key = title;

    // Configure the layers list selector.  Note that footprints are built outside the current
    // board and so we may need to show all layers if the text is on an unactivated layer.
    if( !m_frame->GetBoard()->IsLayerEnabled( m_item->GetLayer() ) )
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
        m_OrientCtrl->SetString( ii, wxString::Format( wxT( "%.1f" ), rot_list[ii] ) );

    // Set font sizes
    m_statusLine->SetFont( KIUI::GetInfoFont( this ) );

    SetupStandardButtons();

    // wxTextCtrls fail to generate wxEVT_CHAR events when the wxTE_MULTILINE flag is set,
    // so we have to listen to wxEVT_CHAR_HOOK events instead.
    Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_TEXT_PROPERTIES::OnCharHook ),
             nullptr, this );

    m_MultiLineText->Bind( wxEVT_STC_CHARADDED,
                           &DIALOG_TEXT_PROPERTIES::onScintillaCharAdded, this );
    m_MultiLineText->Bind( wxEVT_STC_AUTOCOMP_CHAR_DELETED,
                           &DIALOG_TEXT_PROPERTIES::onScintillaCharAdded, this );

    finishDialogSettings();
}


DIALOG_TEXT_PROPERTIES::~DIALOG_TEXT_PROPERTIES()
{
    Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_TEXT_PROPERTIES::OnCharHook ),
                nullptr, this );

    delete m_scintillaTricks;
}


void PCB_BASE_EDIT_FRAME::ShowTextPropertiesDialog( BOARD_ITEM* aText )
{
    DIALOG_TEXT_PROPERTIES dlg( this, aText );
    dlg.ShowQuasiModal();
}


void DIALOG_TEXT_PROPERTIES::OnSetFocusText( wxFocusEvent& event )
{
#ifdef __WXGTK__
    // Force an update of the text control before setting the text selection
    // This is needed because GTK seems to ignore the selection on first update
    //
    // Note that we can't do this on OSX as it tends to provoke Apple's
    // "[NSAlert runModal] may not be invoked inside of transaction begin/commit pair"
    // bug.  See: https://bugs.launchpad.net/kicad/+bug/1837225
    if( m_fpText->GetType() == FP_TEXT::TEXT_is_REFERENCE )
        m_SingleLineText->Update();
#endif

    if( m_fpText->GetType() == FP_TEXT::TEXT_is_REFERENCE )
        KIUI::SelectReferenceNumber( static_cast<wxTextEntry*>( m_SingleLineText ) );
    else
        m_SingleLineText->SetSelection( -1, -1 );

    event.Skip();
}


void DIALOG_TEXT_PROPERTIES::onScintillaCharAdded( wxStyledTextEvent &aEvent )
{
    wxStyledTextCtrl* te = m_MultiLineText;
    wxArrayString     autocompleteTokens;
    int               text_pos = te->GetCurrentPos();
    int               start = te->WordStartPosition( text_pos, true );
    wxString          partial;

    auto textVarRef =
            [&]( int pos )
            {
                return pos >= 2 && te->GetCharAt( pos-2 ) == '$' && te->GetCharAt( pos-1 ) == '{';
            };

    // Check for cross-reference
    if( start > 1 && te->GetCharAt( start-1 ) == ':' )
    {
        int refStart = te->WordStartPosition( start-1, true );

        if( textVarRef( refStart ) )
        {
            partial = te->GetRange( start, text_pos );

            wxString ref = te->GetRange( refStart, start-1 );
            BOARD*   board = m_item->GetBoard();

            for( FOOTPRINT* candidate : board->Footprints() )
            {
                if( candidate->GetReference() == ref )
                {
                    candidate->GetContextualTextVars( &autocompleteTokens );
                    break;
                }
            }
        }
    }
    else if( textVarRef( start ) )
    {
        partial = te->GetTextRange( start, text_pos );

        BOARD* board = m_item->GetBoard();

        board->GetContextualTextVars( &autocompleteTokens );

        if( m_fpText )
        {
            if( FOOTPRINT* footprint = static_cast<FOOTPRINT*>( m_fpText->GetParentFootprint() ) )
                footprint->GetContextualTextVars( &autocompleteTokens );
        }

        for( std::pair<wxString, wxString> entry : board->GetProject()->GetTextVars() )
            autocompleteTokens.push_back( entry.first );
    }

    m_scintillaTricks->DoAutocomplete( partial, autocompleteTokens );
    m_MultiLineText->SetFocus();
}


bool DIALOG_TEXT_PROPERTIES::TransferDataToWindow()
{
    if( m_SingleLineText->IsShown() )
    {
        m_SingleLineText->SetValue( m_edaText->GetText() );

        if( m_fpText && m_fpText->GetType() == FP_TEXT::TEXT_is_REFERENCE )
            KIUI::SelectReferenceNumber( static_cast<wxTextEntry*>( m_SingleLineText ) );
        else
            m_SingleLineText->SetSelection( -1, -1 );
    }
    else if( m_MultiLineText->IsShown() )
    {
        BOARD*   board = m_frame->GetBoard();
        wxString converted = board->ConvertKIIDsToCrossReferences(
                UnescapeString( m_edaText->GetText() ) );

        m_MultiLineText->SetValue( converted );
        m_MultiLineText->SetSelection( -1, -1 );
        m_MultiLineText->EmptyUndoBuffer();
    }

    if( m_item->Type() == PCB_FP_TEXT_T && m_fpText )
    {
        FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( m_fpText->GetParent() );
        wxString   msg;

        if( footprint )
        {
            msg.Printf( _( "Footprint %s (%s), %s, rotated %.1f deg"),
                        footprint->GetReference(),
                        footprint->GetValue(),
                        footprint->IsFlipped() ? _( "back side (mirrored)" ) : _( "front side" ),
                        footprint->GetOrientation().AsDegrees() );
        }

        m_statusLine->SetLabel( msg );
    }
    else
    {
        m_statusLine->Show( false );
    }

    m_cbLocked->SetValue( m_item->IsLocked() );

    m_LayerSelectionCtrl->SetLayerSelection( m_item->GetLayer() );
    m_cbKnockout->SetValue( m_item->IsKnockout() );

    m_fontCtrl->SetFontSelection( m_edaText->GetFont() );

    m_textWidth.SetValue( m_edaText->GetTextSize().x );
    m_textHeight.SetValue( m_edaText->GetTextSize().y );
    m_thickness.SetValue( m_edaText->GetTextThickness() );
    m_posX.SetValue( m_edaText->GetTextPos().x );
    m_posY.SetValue( m_edaText->GetTextPos().y );

    m_Visible->SetValue( m_edaText->IsVisible() );

    if( m_fpText )
        m_KeepUpright->SetValue( m_fpText->IsKeepUpright() );

    m_bold->Check( m_edaText->IsBold() );
    m_italic->Check( m_edaText->IsItalic() );

    switch ( m_edaText->GetHorizJustify() )
    {
    case GR_TEXT_H_ALIGN_LEFT:   m_alignLeft->Check( true );   break;
    case GR_TEXT_H_ALIGN_CENTER: m_alignCenter->Check( true ); break;
    case GR_TEXT_H_ALIGN_RIGHT:  m_alignRight->Check( true );  break;
    }

    switch ( m_edaText->GetVertJustify() )
    {
    case GR_TEXT_V_ALIGN_BOTTOM: m_valignBottom->Check( true ); break;
    case GR_TEXT_V_ALIGN_CENTER: m_valignCenter->Check( true ); break;
    case GR_TEXT_V_ALIGN_TOP:    m_valignTop->Check( true );    break;
    }

    m_mirrored->Check( m_edaText->IsMirrored() );

    EDA_ANGLE orientation = m_edaText->GetTextAngle();
    m_orientation.SetAngleValue( orientation.Normalize180() );

    return DIALOG_TEXT_PROPERTIES_BASE::TransferDataToWindow();
}


void DIALOG_TEXT_PROPERTIES::onFontSelected( wxCommandEvent & aEvent )
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


void DIALOG_TEXT_PROPERTIES::onBoldToggle( wxCommandEvent & aEvent )
{
    int textSize = std::min( m_textWidth.GetValue(), m_textHeight.GetValue() );

    if( aEvent.IsChecked() )
        m_thickness.ChangeValue( GetPenSizeForBold( textSize ) );
    else
        m_thickness.ChangeValue( GetPenSizeForNormal( textSize ) );

    aEvent.Skip();
}


void DIALOG_TEXT_PROPERTIES::onAlignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_alignLeft, m_alignCenter, m_alignRight } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


void DIALOG_TEXT_PROPERTIES::onValignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_valignBottom, m_valignCenter, m_valignTop } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


void DIALOG_TEXT_PROPERTIES::onThickness( wxCommandEvent& event )
{
    int textSize = std::min( m_textWidth.GetValue(), m_textHeight.GetValue() );
    int thickness = m_thickness.GetValue();

    m_bold->Check( abs( thickness - GetPenSizeForBold( textSize ) )
                    < abs( thickness - GetPenSizeForNormal( textSize ) ) );
}


bool DIALOG_TEXT_PROPERTIES::TransferDataFromWindow()
{
    if( !DIALOG_TEXT_PROPERTIES_BASE::TransferDataFromWindow() )
        return false;

    if( !m_textWidth.Validate( TEXTS_MIN_SIZE, TEXTS_MAX_SIZE )
        || !m_textHeight.Validate( TEXTS_MIN_SIZE, TEXTS_MAX_SIZE ) )
    {
        return false;
    }

    BOARD_COMMIT commit( m_frame );
    commit.Modify( m_item );

    // If no other command in progress, prepare undo command
    // (for a command in progress, will be made later, at the completion of command)
    bool pushCommit = ( m_item->GetEditFlags() == 0 );

    // Set IN_EDIT flag to force undo/redo/abort proper operation and avoid new calls to
    // SaveCopyInUndoList for the same text if is moved, and then rotated, edited, etc....
    if( !pushCommit )
        m_item->SetFlags( IN_EDIT );

    // Set the new text content
    if( m_SingleLineText->IsShown() )
    {
        if( !m_SingleLineText->GetValue().IsEmpty() )
            m_edaText->SetText( m_SingleLineText->GetValue() );
    }
    else if( m_MultiLineText->IsShown() )
    {
        if( !m_MultiLineText->GetValue().IsEmpty() )
        {
            BOARD*   board = m_frame->GetBoard();
            wxString txt = board->ConvertCrossReferencesToKIIDs( m_MultiLineText->GetValue() );

#ifdef __WXMAC__
            // On macOS CTRL+Enter produces '\r' instead of '\n' regardless of EOL setting.
            // Replace it now.
            txt.Replace( wxT( "\r" ), wxT( "\n" ) );
#elif defined( __WINDOWS__ )
            // On Windows, a new line is coded as \r\n.  We use only \n in kicad files and in
            // drawing routines so strip the \r char.
            txt.Replace( wxT( "\r" ), wxT( "" ) );
#endif
            m_edaText->SetText( EscapeString( txt, CTX_QUOTED_STR ) );
        }
    }

    m_item->SetLocked( m_cbLocked->GetValue() );

    m_item->SetLayer( ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) );
    m_item->SetIsKnockout( m_cbKnockout->GetValue() );

    if( m_fontCtrl->HaveFontSelection() )
    {
        m_edaText->SetFont( m_fontCtrl->GetFontSelection( m_bold->IsChecked(),
                                                          m_italic->IsChecked() ) );
    }

    m_edaText->SetTextSize( wxSize( m_textWidth.GetValue(), m_textHeight.GetValue() ) );
    m_edaText->SetTextThickness( m_thickness.GetValue() );
    m_edaText->SetTextPos( wxPoint( m_posX.GetValue(), m_posY.GetValue() ) );

    if( m_fpText )
        m_fpText->SetLocalCoord();

    // Test for acceptable values for thickness and size and clamp if fails
    int maxPenWidth = Clamp_Text_PenSize( m_edaText->GetTextThickness(), m_edaText->GetTextSize() );

    if( m_edaText->GetTextThickness() > maxPenWidth )
    {
        DisplayError( this, _( "The text thickness is too large for the text size.\n"
                               "It will be clamped." ) );
        m_edaText->SetTextThickness( maxPenWidth );
    }

    m_edaText->SetTextAngle( m_orientation.GetAngleValue().Normalize() );

    m_edaText->SetVisible( m_Visible->GetValue() );

    if( m_fpText )
        m_fpText->SetKeepUpright( m_KeepUpright->GetValue() );

    m_edaText->SetBold( m_bold->IsChecked() );
    m_edaText->SetItalic( m_italic->IsChecked() );

    if( m_alignLeft->IsChecked() )
        m_edaText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    else if( m_alignCenter->IsChecked() )
        m_edaText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
    else
        m_edaText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );

    if( m_valignBottom->IsChecked() )
        m_edaText->SetVertJustify ( GR_TEXT_V_ALIGN_BOTTOM );
    else if( m_valignCenter->IsChecked() )
        m_edaText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
    else
        m_edaText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );

    m_edaText->SetMirrored( m_mirrored->IsChecked() );

    if( pushCommit )
        commit.Push( _( "Change text properties" ) );

    return true;
}


void DIALOG_TEXT_PROPERTIES::onMultiLineTCLostFocus( wxFocusEvent& event )
{
    if( m_scintillaTricks )
        m_scintillaTricks->CancelAutocomplete();

    event.Skip();
}
