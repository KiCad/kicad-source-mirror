/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2018 Jean-Pierre Charras jp.charras at wanadoo.fr
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

#include "dialog_text_properties.h"

#include <wx/valnum.h>
#include <wx/hyperlink.h>

#include <widgets/bitmap_button.h>
#include <widgets/font_choice.h>
#include <dialogs/html_message_box.h>
#include <confirm.h>
#include <board_commit.h>
#include <board.h>
#include <pcb_barcode.h>
#include <footprint.h>
#include <pcb_text.h>
#include <project.h>
#include <pcb_edit_frame.h>
#include <pcb_layer_box_selector.h>
#include <scintilla_tricks.h>
#include <string_utils.h>


DIALOG_TEXT_PROPERTIES::DIALOG_TEXT_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, PCB_TEXT* aText ) :
        DIALOG_TEXT_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_item( aText ),
        m_textWidth( aParent, m_SizeXLabel, m_SizeXCtrl, m_SizeXUnits ),
        m_textHeight( aParent, m_SizeYLabel, m_SizeYCtrl, m_SizeYUnits ),
        m_thickness( aParent, m_ThicknessLabel, m_ThicknessCtrl, m_ThicknessUnits ),
        m_posX( aParent, m_PositionXLabel, m_PositionXCtrl, m_PositionXUnits ),
        m_posY( aParent, m_PositionYLabel, m_PositionYCtrl, m_PositionYUnits ),
        m_orientation( aParent, m_OrientLabel, m_OrientCtrl, nullptr )
{
    wxString title;

    // Configure display origin transforms
    if( m_item->GetParentFootprint() )
    {
        m_posX.SetCoordType( ORIGIN_TRANSFORMS::REL_X_COORD );
        m_posY.SetCoordType( ORIGIN_TRANSFORMS::REL_Y_COORD );
    }
    else
    {
        m_posX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
        m_posY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );
    }

    m_MultiLineText->SetEOLMode( wxSTC_EOL_LF );

#ifdef _WIN32
    // Without this setting, on Windows, some esoteric unicode chars create display issue
    // in a wxStyledTextCtrl.
    // for SetTechnology() info, see https://www.scintilla.org/ScintillaDoc.html#SCI_SETTECHNOLOGY
    m_MultiLineText->SetTechnology( wxSTC_TECHNOLOGY_DIRECTWRITE );
#endif

    m_scintillaTricks = new SCINTILLA_TRICKS(
            m_MultiLineText, wxT( "{}" ), false,
            // onAcceptFn
            [this]( wxKeyEvent& aEvent )
            {
                wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
            },
            // onCharFn
            [this]( wxStyledTextEvent& aEvent )
            {
                m_scintillaTricks->DoTextVarAutocomplete(
                        // getTokensFn
                        [this]( const wxString& xRef, wxArrayString* tokens )
                        {
                            m_frame->GetContextualTextVars( m_item, xRef, tokens );
                        } );
            } );

    // A hack which causes Scintilla to auto-size the text editor canvas
    // See: https://github.com/jacobslusser/ScintillaNET/issues/216
    m_MultiLineText->SetScrollWidth( 1 );
    m_MultiLineText->SetScrollWidthTracking( true );

    // Add syntax help hyperlink
    m_syntaxHelp = new wxHyperlinkCtrl( this, wxID_ANY, _( "Syntax help" ), wxEmptyString, wxDefaultPosition,
                                        wxDefaultSize, wxHL_DEFAULT_STYLE );
    m_syntaxHelp->SetToolTip( _( "Show syntax help window" ) );
    m_MultiLineSizer->Add( m_syntaxHelp, 0, wxBOTTOM | wxRIGHT | wxLEFT, 3 );

    m_syntaxHelp->Bind( wxEVT_HYPERLINK, &DIALOG_TEXT_PROPERTIES::onSyntaxHelp, this );

    m_helpWindow = nullptr;

    if( m_item->GetParentFootprint() )
    {
        m_PositionXLabel->SetLabel( _( "Offset X:" ) );
        m_PositionYLabel->SetLabel( _( "Offset Y:" ) );

        if( m_item->Type() == PCB_FIELD_T )
        {
            PCB_FIELD* field = static_cast<PCB_FIELD*>( m_item );

            if( field->IsReference() )
            {
                title = _( "Footprint Reference Properties" );
                m_TextLabel->SetLabel( _( "Reference:" ) );
            }
            else if( field->IsValue() )
            {
                title = _( "Footprint Value Properties" );
                m_TextLabel->SetLabel( _( "Value:" ) );
            }
            else
            {
                title = _( "Footprint Field Properties" );
                m_TextLabel->SetLabel( _( "Text:" ) );
            }
        }
        else
        {
            title = _( "Footprint Text Properties" );
            m_TextLabel->SetLabel( _( "Text:" ) );
            m_Visible->Show( false );
        }

        SetInitialFocus( m_SingleLineText );
        m_MultiLineSizer->Show( false );

        // Do not allow locking items in the footprint editor
        m_cbLocked->Show( false );

        m_tabOrder = { m_SingleLineText, m_LayerSelectionCtrl, m_SizeXCtrl,     m_SizeYCtrl,     m_ThicknessCtrl,
                       m_Visible,        m_cbKnockout,         m_KeepUpright,   m_PositionXCtrl, m_PositionYCtrl,
                       m_OrientCtrl,     m_sdbSizerOK,         m_sdbSizerCancel };
    }
    else
    {
        title = _( "Text Properties" );

        SetInitialFocus( m_MultiLineText );
        m_SingleLineSizer->Show( false );

        m_Visible->Show( false );
        m_KeepUpright->Show( false );
        m_statusLine->Show( false );

        m_tabOrder = { m_MultiLineText, m_cbLocked,      m_LayerSelectionCtrl, m_SizeXCtrl,
                       m_SizeYCtrl,     m_ThicknessCtrl, m_cbKnockout,         m_PositionXCtrl,
                       m_PositionYCtrl, m_OrientCtrl,    m_sdbSizerOK,         m_sdbSizerCancel };
    }

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

    m_valignBottom->SetIsRadioButton();
    m_valignBottom->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_bottom ) );
    m_valignCenter->SetIsRadioButton();
    m_valignCenter->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_center ) );
    m_valignTop->SetIsRadioButton();
    m_valignTop->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_top ) );

    m_separator3->SetIsSeparator();

    m_mirrored->SetIsCheckButton();
    m_mirrored->SetBitmap( KiBitmapBundle( BITMAPS::text_mirrored ) );

    m_autoTextThickness->SetIsCheckButton();
    m_autoTextThickness->SetBitmap( KiBitmapBundle( BITMAPS::edit_cmp_symb_links ) );

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
    Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_TEXT_PROPERTIES::OnCharHook ), nullptr, this );

    finishDialogSettings();
}


DIALOG_TEXT_PROPERTIES::~DIALOG_TEXT_PROPERTIES()
{
    Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_TEXT_PROPERTIES::OnCharHook ), nullptr, this );

    delete m_scintillaTricks;
    m_scintillaTricks = nullptr;

    if( m_helpWindow )
        m_helpWindow->Destroy();
}


void PCB_BASE_EDIT_FRAME::ShowTextPropertiesDialog( PCB_TEXT* aText )
{
    DIALOG_TEXT_PROPERTIES dlg( this, aText );

    // QuasiModal required for Scintilla auto-complete
    dlg.ShowQuasiModal();
}


void DIALOG_TEXT_PROPERTIES::OnSetFocusText( wxFocusEvent& event )
{
    if( m_item->Type() == PCB_FIELD_T && static_cast<PCB_FIELD*>( m_item )->IsReference() )
    {
#ifdef __WXGTK__
        // Force an update of the text control before setting the text selection
        // This is needed because GTK seems to ignore the selection on first update
        //
        // Note that we can't do this on OSX as it tends to provoke Apple's
        // "[NSAlert runModal] may not be invoked inside of transaction begin/commit pair"
        // bug.  See: https://bugs.launchpad.net/kicad/+bug/1837225
        m_SingleLineText->Update();
#endif
        KIUI::SelectReferenceNumber( static_cast<wxTextEntry*>( m_SingleLineText ) );
    }
    else
        m_SingleLineText->SetSelection( -1, -1 );

    event.Skip();
}


bool DIALOG_TEXT_PROPERTIES::TransferDataToWindow()
{
    BOARD*     board = m_frame->GetBoard();
    FOOTPRINT* parentFP = m_item->GetParentFootprint();
    wxString   text = m_item->GetText();

    // show text variable cross-references in a human-readable format
    text = board->ConvertKIIDsToCrossReferences( UnescapeString( text ) );

    if( m_SingleLineText->IsShown() )
    {
        m_SingleLineText->SetValue( text );

        if( m_item->Type() == PCB_FIELD_T && static_cast<PCB_FIELD*>( m_item )->IsReference() )
            KIUI::SelectReferenceNumber( static_cast<wxTextEntry*>( m_SingleLineText ) );
        else
            m_SingleLineText->SetSelection( -1, -1 );
    }
    else if( m_MultiLineText->IsShown() )
    {
        m_MultiLineText->SetValue( text );
        m_MultiLineText->SetSelection( -1, -1 );
        m_MultiLineText->EmptyUndoBuffer();
    }

    if( parentFP )
    {
        m_statusLine->SetLabel( wxString::Format(
                _( "Footprint %s (%s), %s, rotated %.1f deg" ), parentFP->GetReference(), parentFP->GetValue(),
                parentFP->IsFlipped() ? _( "back side (mirrored)" ) : _( "front side" ),
                parentFP->GetOrientation().AsDegrees() ) );
    }
    else
    {
        m_statusLine->Show( false );
    }

    m_cbLocked->SetValue( m_item->IsLocked() );

    m_LayerSelectionCtrl->SetLayerSelection( m_item->GetLayer() );
    m_cbKnockout->SetValue( m_item->IsKnockout() );

    m_fontCtrl->SetFontSelection( m_item->GetFont() );

    m_textWidth.SetValue( m_item->GetTextSize().x );
    m_textHeight.SetValue( m_item->GetTextSize().y );

    if( m_item->GetAutoThickness() )
    {
        m_autoTextThickness->Check( m_item->GetAutoThickness() );
        m_thickness.SetValue( m_item->GetEffectiveTextPenWidth() );
        m_thickness.Enable( false );
    }
    else
    {
        m_thickness.SetValue( m_item->GetTextThickness() );
    }

    m_posX.SetValue( m_item->GetFPRelativePosition().x );
    m_posY.SetValue( m_item->GetFPRelativePosition().y );

    if( m_Visible->IsShown() )
        m_Visible->SetValue( m_item->IsVisible() );

    if( m_KeepUpright->IsShown() )
        m_KeepUpright->SetValue( m_item->IsKeepUpright() );

    m_bold->Check( m_item->IsBold() );
    m_italic->Check( m_item->IsItalic() );

    switch( m_item->GetHorizJustify() )
    {
    case GR_TEXT_H_ALIGN_LEFT: m_alignLeft->Check( true ); break;
    case GR_TEXT_H_ALIGN_CENTER: m_alignCenter->Check( true ); break;
    case GR_TEXT_H_ALIGN_RIGHT: m_alignRight->Check( true ); break;
    case GR_TEXT_H_ALIGN_INDETERMINATE: break;
    }

    switch( m_item->GetVertJustify() )
    {
    case GR_TEXT_V_ALIGN_BOTTOM: m_valignBottom->Check( true ); break;
    case GR_TEXT_V_ALIGN_CENTER: m_valignCenter->Check( true ); break;
    case GR_TEXT_V_ALIGN_TOP: m_valignTop->Check( true ); break;
    case GR_TEXT_V_ALIGN_INDETERMINATE: break;
    }

    m_mirrored->Check( m_item->IsMirrored() );

    EDA_ANGLE orientation = m_item->GetTextAngle();
    m_orientation.SetAngleValue( orientation.Normalize180() );

    return DIALOG_TEXT_PROPERTIES_BASE::TransferDataToWindow();
}


void DIALOG_TEXT_PROPERTIES::onFontSelected( wxCommandEvent& aEvent )
{
    if( KIFONT::FONT::IsStroke( aEvent.GetString() ) )
    {
        m_thickness.Show( true );
        m_autoTextThickness->Show( true );

        int textSize = std::min( m_textWidth.GetValue(), m_textHeight.GetValue() );
        int thickness = m_thickness.GetValue();

        m_bold->Check( abs( thickness - GetPenSizeForBold( textSize ) )
                       < abs( thickness - GetPenSizeForNormal( textSize ) ) );
    }
    else
    {
        m_thickness.Show( false );
        m_autoTextThickness->Show( false );
    }
}


void DIALOG_TEXT_PROPERTIES::onBoldToggle( wxCommandEvent& aEvent )
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


void DIALOG_TEXT_PROPERTIES::onTextSize( wxCommandEvent& aEvent )
{
    if( m_autoTextThickness->IsChecked() )
    {
        int textSize = std::min( m_textWidth.GetValue(), m_textHeight.GetValue() );
        int thickness;

        // Calculate the "best" thickness from text size and bold option:
        if( m_bold->IsChecked() )
            thickness = GetPenSizeForBold( textSize );
        else
            thickness = GetPenSizeForNormal( textSize );

        m_thickness.SetValue( thickness );
    }
}


void DIALOG_TEXT_PROPERTIES::onAutoTextThickness( wxCommandEvent& aEvent )
{
    if( aEvent.IsChecked() )
    {
        m_autoTextThickness->Check( true );

        wxCommandEvent dummy;
        onTextSize( dummy );

        m_thickness.Enable( false );
    }
    else
    {
        m_thickness.Enable( true );
    }
}


bool DIALOG_TEXT_PROPERTIES::TransferDataFromWindow()
{
    if( !DIALOG_TEXT_PROPERTIES_BASE::TransferDataFromWindow() )
        return false;

    int minSize = pcbIUScale.mmToIU( TEXT_MIN_SIZE_MM );
    int maxSize = pcbIUScale.mmToIU( TEXT_MAX_SIZE_MM );

    if( !m_textWidth.Validate( minSize, maxSize ) || !m_textHeight.Validate( minSize, maxSize ) )
        return false;

    BOARD*       board = m_frame->GetBoard();
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
        {
            // convert any text variable cross-references to their UUIDs
            wxString txt = board->ConvertCrossReferencesToKIIDs( m_SingleLineText->GetValue() );

            m_item->SetText( txt );
        }
    }
    else if( m_MultiLineText->IsShown() )
    {
        if( !m_MultiLineText->GetValue().IsEmpty() )
        {
            // convert any text variable cross-references to their UUIDs
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
            m_item->SetText( EscapeString( txt, CTX_QUOTED_STR ) );
        }
    }

    m_item->SetLocked( m_cbLocked->GetValue() );

    m_item->SetLayer( ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) );
    m_item->SetIsKnockout( m_cbKnockout->GetValue() );

    if( m_fontCtrl->HaveFontSelection() )
    {
        m_item->SetFont( m_fontCtrl->GetFontSelection( m_bold->IsChecked(), m_italic->IsChecked() ) );
    }

    m_item->SetTextSize( VECTOR2I( m_textWidth.GetValue(), m_textHeight.GetValue() ) );

    if( m_autoTextThickness->IsChecked() )
    {
        m_item->SetAutoThickness( true );
    }
    else
    {
        m_item->SetTextThickness( m_thickness.GetValue() );

        // Test for acceptable values for thickness and size and clamp if fails
        int maxPenWidth = ClampTextPenSize( m_item->GetTextThickness(), m_item->GetTextSize() );

        if( m_item->GetTextThickness() > maxPenWidth )
        {
            DisplayError( this, _( "The text thickness is too large for the text size.\n"
                                   "It will be clamped." ) );
            m_item->SetTextThickness( maxPenWidth );
        }
    }

    m_item->SetFPRelativePosition( VECTOR2I( m_posX.GetValue(), m_posY.GetValue() ) );
    m_item->SetTextAngle( m_orientation.GetAngleValue().Normalize() );

    if( m_Visible->IsShown() )
        m_item->SetVisible( m_Visible->GetValue() );

    if( m_KeepUpright->IsShown() )
        m_item->SetKeepUpright( m_KeepUpright->GetValue() );

    m_item->SetBoldFlag( m_bold->IsChecked() );
    m_item->SetItalicFlag( m_italic->IsChecked() );

    if( m_alignLeft->IsChecked() )
        m_item->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    else if( m_alignCenter->IsChecked() )
        m_item->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
    else
        m_item->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );

    if( m_valignBottom->IsChecked() )
        m_item->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
    else if( m_valignCenter->IsChecked() )
        m_item->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
    else
        m_item->SetVertJustify( GR_TEXT_V_ALIGN_TOP );

    m_item->SetMirrored( m_mirrored->IsChecked() );

    if( pushCommit )
        commit.Push( _( "Edit Text Properties" ) );

    return true;
}


void DIALOG_TEXT_PROPERTIES::onMultiLineTCLostFocus( wxFocusEvent& event )
{
    if( m_scintillaTricks )
        m_scintillaTricks->CancelAutocomplete();

    event.Skip();
}


void DIALOG_TEXT_PROPERTIES::onSyntaxHelp( wxHyperlinkEvent& aEvent )
{
    m_helpWindow = PCB_TEXT::ShowSyntaxHelp( this );
}
