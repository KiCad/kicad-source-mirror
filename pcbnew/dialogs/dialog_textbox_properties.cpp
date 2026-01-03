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

#include "dialog_textbox_properties.h"

#include <wx/hyperlink.h>

#include <widgets/bitmap_button.h>
#include <widgets/font_choice.h>
#include <dialogs/html_message_box.h>
#include <confirm.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <board.h>
#include <footprint.h>
#include <pcb_textbox.h>
#include <project.h>
#include <pcb_edit_frame.h>
#include <pcb_layer_box_selector.h>
#include <scintilla_tricks.h>
#include <string_utils.h>


DIALOG_TEXTBOX_PROPERTIES::DIALOG_TEXTBOX_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, PCB_TEXTBOX* aTextBox ) :
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
                            m_frame->GetContextualTextVars( m_textBox, xRef, tokens );
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

    m_syntaxHelp->Bind( wxEVT_HYPERLINK, &DIALOG_TEXTBOX_PROPERTIES::onSyntaxHelp, this );

    m_helpWindow = nullptr;

    if( m_textBox->GetParentFootprint() )
    {
        // Do not allow locking items in the footprint editor
        m_cbLocked->Show( false );
    }

    SetInitialFocus( m_MultiLineText );

    m_separator0->SetIsSeparator();

    m_bold->SetIsCheckButton();
    m_bold->SetBitmap( KiBitmapBundle( BITMAPS::text_bold ) );
    m_italic->SetIsCheckButton();
    m_italic->SetBitmap( KiBitmapBundle( BITMAPS::text_italic ) );

    m_separator1->SetIsSeparator();

    m_hAlignLeft->SetIsRadioButton();
    m_hAlignLeft->SetBitmap( KiBitmapBundle( BITMAPS::text_align_left ) );
    m_hAlignCenter->SetIsRadioButton();
    m_hAlignCenter->SetBitmap( KiBitmapBundle( BITMAPS::text_align_center ) );
    m_hAlignRight->SetIsRadioButton();
    m_hAlignRight->SetBitmap( KiBitmapBundle( BITMAPS::text_align_right ) );

    m_separator2->SetIsSeparator();

    m_mirrored->SetIsCheckButton();
    m_mirrored->SetBitmap( KiBitmapBundle( BITMAPS::text_mirrored ) );

    m_separator3->SetIsSeparator();

    m_vAlignTop->SetIsRadioButton();
    m_vAlignTop->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_top ) );
    m_vAlignCenter->SetIsRadioButton();
    m_vAlignCenter->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_center ) );
    m_vAlignBottom->SetIsRadioButton();
    m_vAlignBottom->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_bottom ) );

    m_separator4->SetIsSeparator();

    m_autoTextThickness->SetIsCheckButton();
    m_autoTextThickness->SetBitmap( KiBitmapBundle( BITMAPS::edit_cmp_symb_links ) );

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

    for( const auto& [lineStyle, lineStyleDesc] : lineTypeNames )
        m_borderStyleCombo->Append( lineStyleDesc.name, KiBitmapBundle( lineStyleDesc.bitmap ) );

    SetupStandardButtons();

    // wxTextCtrls fail to generate wxEVT_CHAR events when the wxTE_MULTILINE flag is set,
    // so we have to listen to wxEVT_CHAR_HOOK events instead.
    Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_TEXTBOX_PROPERTIES::OnCharHook ), nullptr, this );

    finishDialogSettings();
    Layout();
    bMainSizer->Fit( this );
}


DIALOG_TEXTBOX_PROPERTIES::~DIALOG_TEXTBOX_PROPERTIES()
{
    Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_TEXTBOX_PROPERTIES::OnCharHook ), nullptr, this );

    delete m_scintillaTricks;
    m_scintillaTricks = nullptr;

    if( m_helpWindow )
        m_helpWindow->Destroy();
}


int PCB_BASE_EDIT_FRAME::ShowTextBoxPropertiesDialog( PCB_TEXTBOX* aTextBox )
{
    DIALOG_TEXTBOX_PROPERTIES dlg( this, aTextBox );

    // QuasiModal required for Scintilla auto-complete
    return dlg.ShowQuasiModal();
}


bool DIALOG_TEXTBOX_PROPERTIES::TransferDataToWindow()
{
    BOARD*   board = m_frame->GetBoard();
    wxString converted = board->ConvertKIIDsToCrossReferences( UnescapeString( m_textBox->GetText() ) );

    m_MultiLineText->SetValue( converted );
    m_MultiLineText->SetSelection( -1, -1 );
    m_MultiLineText->EmptyUndoBuffer();

    m_cbLocked->SetValue( m_textBox->IsLocked() );

    m_LayerSelectionCtrl->SetLayerSelection( m_textBox->GetLayer() );

    m_fontCtrl->SetFontSelection( m_textBox->GetFont() );

    m_textWidth.SetValue( m_textBox->GetTextSize().x );
    m_textHeight.SetValue( m_textBox->GetTextSize().y );

    if( m_textBox->GetAutoThickness() )
    {
        m_autoTextThickness->Check( m_textBox->GetAutoThickness() );
        m_thickness.SetValue( m_textBox->GetEffectiveTextPenWidth() );
        m_thickness.Enable( false );
    }
    else
    {
        m_thickness.SetValue( m_textBox->GetTextThickness() );
    }

    m_bold->Check( m_textBox->IsBold() );
    m_italic->Check( m_textBox->IsItalic() );

    switch( m_textBox->GetHorizJustify() )
    {
    case GR_TEXT_H_ALIGN_LEFT: m_hAlignLeft->Check( true ); break;
    case GR_TEXT_H_ALIGN_CENTER: m_hAlignCenter->Check( true ); break;
    case GR_TEXT_H_ALIGN_RIGHT: m_hAlignRight->Check( true ); break;
    case GR_TEXT_H_ALIGN_INDETERMINATE: break;
    }

    switch( m_textBox->GetVertJustify() )
    {
    case GR_TEXT_V_ALIGN_TOP: m_vAlignTop->Check( true ); break;
    case GR_TEXT_V_ALIGN_CENTER: m_vAlignCenter->Check( true ); break;
    case GR_TEXT_V_ALIGN_BOTTOM: m_vAlignBottom->Check( true ); break;
    case GR_TEXT_V_ALIGN_INDETERMINATE: break;
    }

    m_mirrored->Check( m_textBox->IsMirrored() );

    EDA_ANGLE orientation = m_textBox->GetTextAngle();
    m_orientation.SetAngleValue( orientation.Normalize180() );

    STROKE_PARAMS stroke = m_textBox->GetStroke();
    m_borderCheckbox->SetValue( m_textBox->IsBorderEnabled() );

    if( m_textBox->IsBorderEnabled() )
        m_borderWidth.SetValue( stroke.GetWidth() );

    LINE_STYLE style = stroke.GetLineStyle();

    if( style == LINE_STYLE::DEFAULT )
        style = LINE_STYLE::SOLID;

    if( (int) style < (int) lineTypeNames.size() )
        m_borderStyleCombo->SetSelection( (int) style );

    m_borderWidth.Enable( m_textBox->IsBorderEnabled() );
    m_borderStyleLabel->Enable( m_textBox->IsBorderEnabled() );
    m_borderStyleCombo->Enable( m_textBox->IsBorderEnabled() );

    return DIALOG_TEXTBOX_PROPERTIES_BASE::TransferDataToWindow();
}

void DIALOG_TEXTBOX_PROPERTIES::onValignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_vAlignTop, m_vAlignCenter, m_vAlignBottom } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}

void DIALOG_TEXTBOX_PROPERTIES::onFontSelected( wxCommandEvent& aEvent )
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


void DIALOG_TEXTBOX_PROPERTIES::onBoldToggle( wxCommandEvent& aEvent )
{
    int textSize = std::min( m_textWidth.GetValue(), m_textHeight.GetValue() );

    if( aEvent.IsChecked() )
        m_thickness.ChangeValue( GetPenSizeForBold( textSize ) );
    else
        m_thickness.ChangeValue( GetPenSizeForNormal( textSize ) );

    aEvent.Skip();
}


void DIALOG_TEXTBOX_PROPERTIES::onHalignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_hAlignLeft, m_hAlignCenter, m_hAlignRight } )
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


void DIALOG_TEXTBOX_PROPERTIES::onTextSize( wxCommandEvent& aEvent )
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


void DIALOG_TEXTBOX_PROPERTIES::onAutoTextThickness( wxCommandEvent& aEvent )
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

    int minSize = pcbIUScale.mmToIU( TEXT_MIN_SIZE_MM );
    int maxSize = pcbIUScale.mmToIU( TEXT_MAX_SIZE_MM );

    if( !m_textWidth.Validate( minSize, maxSize ) || !m_textHeight.Validate( minSize, maxSize ) )
        return false;

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
        m_textBox->SetFont( m_fontCtrl->GetFontSelection( m_bold->IsChecked(), m_italic->IsChecked() ) );
    }

    m_textBox->SetTextSize( VECTOR2I( m_textWidth.GetValue(), m_textHeight.GetValue() ) );

    if( m_autoTextThickness->IsChecked() )
    {
        m_textBox->SetAutoThickness( true );
    }
    else
    {
        m_textBox->SetTextThickness( m_thickness.GetValue() );

        // Test for acceptable values for thickness and size and clamp if fails
        int maxPenWidth = ClampTextPenSize( m_textBox->GetTextThickness(), m_textBox->GetTextSize() );

        if( m_textBox->GetTextThickness() > maxPenWidth )
        {
            DisplayError( this, _( "The text thickness is too large for the text size.\n"
                                   "It will be clamped." ) );
            m_textBox->SetTextThickness( maxPenWidth );
        }
    }

    m_textBox->SetTextAngle( m_orientation.GetAngleValue().Normalize() );
    m_textBox->SetBoldFlag( m_bold->IsChecked() );
    m_textBox->SetItalicFlag( m_italic->IsChecked() );

    if( m_hAlignLeft->IsChecked() )
        m_textBox->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    else if( m_hAlignCenter->IsChecked() )
        m_textBox->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
    else
        m_textBox->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );

    if( m_vAlignTop->IsChecked() )
        m_textBox->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
    else if( m_vAlignCenter->IsChecked() )
        m_textBox->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
    else
        m_textBox->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );

    m_textBox->SetMirrored( m_mirrored->IsChecked() );

    m_textBox->SetBorderEnabled( m_borderCheckbox->GetValue() );
    STROKE_PARAMS stroke = m_textBox->GetStroke();

    if( !m_borderWidth.IsIndeterminate() )
        stroke.SetWidth( m_borderWidth.GetValue() );

    auto it = lineTypeNames.begin();
    std::advance( it, m_borderStyleCombo->GetSelection() );

    if( it == lineTypeNames.end() )
        stroke.SetLineStyle( LINE_STYLE::SOLID );
    else
        stroke.SetLineStyle( it->first );

    m_textBox->SetStroke( stroke );

    m_textBox->ClearBoundingBoxCache();
    m_textBox->ClearRenderCache();

    if( pushCommit )
        commit.Push( _( "Edit Text Box Properties" ) );

    return true;
}


void DIALOG_TEXTBOX_PROPERTIES::onMultiLineTCLostFocus( wxFocusEvent& event )
{
    if( m_scintillaTricks )
        m_scintillaTricks->CancelAutocomplete();

    event.Skip();
}


void DIALOG_TEXTBOX_PROPERTIES::onSyntaxHelp( wxHyperlinkEvent& aEvent )
{
    m_helpWindow = PCB_TEXT::ShowSyntaxHelp( this );
}
