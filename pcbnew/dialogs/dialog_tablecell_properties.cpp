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
#include <pcb_tablecell.h>
#include <pcb_table.h>
#include <project.h>
#include <pcb_edit_frame.h>
#include <pcb_layer_box_selector.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <scintilla_tricks.h>
#include "dialog_tablecell_properties.h"

DIALOG_TABLECELL_PROPERTIES::DIALOG_TABLECELL_PROPERTIES( PCB_BASE_EDIT_FRAME*        aFrame,
                                                          std::vector<PCB_TABLECELL*> aCells ) :
        DIALOG_TABLECELL_PROPERTIES_BASE( aFrame ),
        m_frame( aFrame ),
        m_table( nullptr ),
        m_cells( std::move( aCells ) ),
        m_textHeight( aFrame, m_SizeYLabel, m_SizeYCtrl, m_SizeYUnits ),
        m_textWidth( aFrame, m_SizeXLabel, m_SizeXCtrl, m_SizeXUnits ),
        m_textThickness( aFrame, m_ThicknessLabel, m_ThicknessCtrl, m_ThicknessUnits ),
        m_marginLeft( aFrame, nullptr, m_marginLeftCtrl, nullptr ),
        m_marginTop( aFrame, nullptr, m_marginTopCtrl, m_marginTopUnits ),
        m_marginRight( aFrame, nullptr, m_marginRightCtrl, nullptr ),
        m_marginBottom( aFrame, nullptr, m_marginBottomCtrl, nullptr ),
        m_cellText( m_cellTextCtrl ),
        m_returnValue( TABLECELL_PROPS_CANCEL )
{
    wxASSERT( m_cells.size() > 0 && m_cells[0] );

    m_cellText->SetEOLMode( wxSTC_EOL_LF );

#ifdef _WIN32
    // Without this setting, on Windows, some esoteric unicode chars create display issue
    // in a wxStyledTextCtrl.
    // for SetTechnology() info, see https://www.scintilla.org/ScintillaDoc.html#SCI_SETTECHNOLOGY
    m_cellText->SetTechnology( wxSTC_TECHNOLOGY_DIRECTWRITE );
#endif

    m_scintillaTricks = new SCINTILLA_TRICKS(
            m_cellText, wxT( "{}" ), false,
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
                            m_frame->GetContextualTextVars( m_table, xRef, tokens );
                        } );
            } );

    // A hack which causes Scintilla to auto-size the text editor canvas
    // See: https://github.com/jacobslusser/ScintillaNET/issues/216
    m_cellText->SetScrollWidth( 1 );
    m_cellText->SetScrollWidthTracking( true );

    SetInitialFocus( m_cellText );

    m_table = static_cast<PCB_TABLE*>( m_cells[0]->GetParent() );

    m_hAlignLeft->SetIsRadioButton();
    m_hAlignLeft->SetBitmap( KiBitmapBundle( BITMAPS::text_align_left ) );
    m_hAlignCenter->SetIsRadioButton();
    m_hAlignCenter->SetBitmap( KiBitmapBundle( BITMAPS::text_align_center ) );
    m_hAlignRight->SetIsRadioButton();
    m_hAlignRight->SetBitmap( KiBitmapBundle( BITMAPS::text_align_right ) );

    m_separator0->SetIsSeparator();

    m_vAlignTop->SetIsRadioButton();
    m_vAlignTop->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_top ) );
    m_vAlignCenter->SetIsRadioButton();
    m_vAlignCenter->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_center ) );
    m_vAlignBottom->SetIsRadioButton();
    m_vAlignBottom->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_bottom ) );

    m_separator1->SetIsSeparator();

    m_bold->SetIsCheckButton();
    m_bold->SetBitmap( KiBitmapBundle( BITMAPS::text_bold ) );
    m_italic->SetIsCheckButton();
    m_italic->SetBitmap( KiBitmapBundle( BITMAPS::text_italic ) );

    m_autoTextThickness->SetIsCheckButton();
    m_autoTextThickness->SetBitmap( KiBitmapBundle( BITMAPS::edit_cmp_symb_links ) );

    SetupStandardButtons();
    Layout();

    m_helpWindow = nullptr;

    Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_TABLECELL_PROPERTIES::OnCharHook ), nullptr, this );

    m_hAlignLeft->Bind( wxEVT_BUTTON, &DIALOG_TABLECELL_PROPERTIES::onHAlignButton, this );
    m_hAlignCenter->Bind( wxEVT_BUTTON, &DIALOG_TABLECELL_PROPERTIES::onHAlignButton, this );
    m_hAlignRight->Bind( wxEVT_BUTTON, &DIALOG_TABLECELL_PROPERTIES::onHAlignButton, this );
    m_vAlignTop->Bind( wxEVT_BUTTON, &DIALOG_TABLECELL_PROPERTIES::onVAlignButton, this );
    m_vAlignCenter->Bind( wxEVT_BUTTON, &DIALOG_TABLECELL_PROPERTIES::onVAlignButton, this );
    m_vAlignBottom->Bind( wxEVT_BUTTON, &DIALOG_TABLECELL_PROPERTIES::onVAlignButton, this );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}

DIALOG_TABLECELL_PROPERTIES::~DIALOG_TABLECELL_PROPERTIES()
{
    Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_TABLECELL_PROPERTIES::OnCharHook ), nullptr, this );

    delete m_scintillaTricks;
    m_scintillaTricks = nullptr;

    if( m_helpWindow )
        m_helpWindow->Destroy();
}


bool DIALOG_TABLECELL_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    bool              firstCell = true;
    GR_TEXT_H_ALIGN_T hAlign = GR_TEXT_H_ALIGN_INDETERMINATE;
    GR_TEXT_V_ALIGN_T vAlign = GR_TEXT_V_ALIGN_INDETERMINATE;
    int               textThickness = 0;
    int               effectivePenWidth = 0;

    for( PCB_TABLECELL* cell : m_cells )
    {
        if( firstCell )
        {
            m_cellTextCtrl->SetValue( cell->GetText() );

            m_fontCtrl->SetFontSelection( cell->GetFont() );
            m_textWidth.SetValue( cell->GetTextWidth() );
            m_textHeight.SetValue( cell->GetTextHeight() );
            textThickness = cell->GetTextThickness();
            effectivePenWidth = cell->GetEffectiveTextPenWidth();

            hAlign = cell->GetHorizJustify();
            vAlign = cell->GetVertJustify();

            m_marginLeft.SetValue( cell->GetMarginLeft() );
            m_marginTop.SetValue( cell->GetMarginTop() );
            m_marginRight.SetValue( cell->GetMarginRight() );
            m_marginBottom.SetValue( cell->GetMarginBottom() );

            // wxCheckBoxState bold = cell->IsBold() ? wxCHK_CHECKED : wxCHK_UNCHECKED;
            m_bold->Check( cell->IsBold() );

            // wxCheckBoxState italic = cell->IsItalic() ? wxCHK_CHECKED : wxCHK_UNCHECKED;
            m_italic->Check( cell->IsItalic() );

            firstCell = false;
        }
        else
        {
            if( cell->GetText() != m_cellTextCtrl->GetValue() )
                m_cellTextCtrl->SetValue( INDETERMINATE_STATE );

            if( cell->GetFont() != m_fontCtrl->GetFontSelection( cell->IsBold(), cell->IsItalic() ) )
                m_fontCtrl->SetSelection( -1 );

            if( cell->GetTextWidth() != m_textWidth.GetValue() )
                m_textWidth.SetValue( INDETERMINATE_STATE );

            if( cell->GetTextHeight() != m_textHeight.GetValue() )
                m_textHeight.SetValue( INDETERMINATE_STATE );

            if( cell->GetTextThickness() != textThickness )
                textThickness = -1;

            if( cell->GetEffectiveTextPenWidth() != effectivePenWidth )
                effectivePenWidth = -1;

            if( cell->GetHorizJustify() != hAlign )
                hAlign = GR_TEXT_H_ALIGN_INDETERMINATE;

            if( cell->GetVertJustify() != vAlign )
                vAlign = GR_TEXT_V_ALIGN_INDETERMINATE;

            if( cell->GetMarginLeft() != m_marginLeft.GetIntValue() )
                m_marginLeft.SetValue( INDETERMINATE_STATE );

            if( cell->GetMarginTop() != m_marginTop.GetIntValue() )
                m_marginTop.SetValue( INDETERMINATE_STATE );

            if( cell->GetMarginRight() != m_marginRight.GetIntValue() )
                m_marginRight.SetValue( INDETERMINATE_STATE );

            if( cell->GetMarginBottom() != m_marginBottom.GetIntValue() )
                m_marginBottom.SetValue( INDETERMINATE_STATE );
        }

        switch( hAlign )
        {
        case GR_TEXT_H_ALIGN_LEFT: m_hAlignLeft->Check(); break;
        case GR_TEXT_H_ALIGN_CENTER: m_hAlignCenter->Check(); break;
        case GR_TEXT_H_ALIGN_RIGHT: m_hAlignRight->Check(); break;
        case GR_TEXT_H_ALIGN_INDETERMINATE: break;
        }

        switch( vAlign )
        {
        case GR_TEXT_V_ALIGN_TOP: m_vAlignTop->Check(); break;
        case GR_TEXT_V_ALIGN_CENTER: m_vAlignCenter->Check(); break;
        case GR_TEXT_V_ALIGN_BOTTOM: m_vAlignBottom->Check(); break;
        case GR_TEXT_V_ALIGN_INDETERMINATE: break;
        }
    }

    m_textThickness.SetValue( INDETERMINATE_STATE );
    m_autoTextThickness->Check( false );

    if( textThickness == 0 )
    {
        if( effectivePenWidth > 0 )
            m_textThickness.SetValue( effectivePenWidth );

        m_autoTextThickness->Check( true );
        m_textThickness.Enable( false );
    }
    else if( textThickness > 0 )
    {
        m_textThickness.SetValue( textThickness );
    }

    return true;
}

void DIALOG_TABLECELL_PROPERTIES::onBoldToggle( wxCommandEvent& aEvent )
{
    int textSize = std::min( m_textWidth.GetValue(), m_textHeight.GetValue() );

    if( aEvent.IsChecked() )
        m_textThickness.ChangeValue( GetPenSizeForBold( textSize ) );
    else
        m_textThickness.ChangeValue( GetPenSizeForNormal( textSize ) );

    aEvent.Skip();
}

void DIALOG_TABLECELL_PROPERTIES::onTextSize( wxCommandEvent& aEvent )
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

        m_textThickness.SetValue( thickness );
    }
}


void DIALOG_TABLECELL_PROPERTIES::onAutoTextThickness( wxCommandEvent& aEvent )
{
    if( aEvent.IsChecked() )
    {
        m_autoTextThickness->Check( true );

        if( !m_textWidth.IsIndeterminate() && !m_textHeight.IsIndeterminate() )
        {
            wxCommandEvent dummy;
            onTextSize( dummy );
        }

        m_textThickness.Enable( false );
    }
    else
    {
        m_textThickness.Enable( true );
    }
}


void DIALOG_TABLECELL_PROPERTIES::onHAlignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_hAlignLeft, m_hAlignCenter, m_hAlignRight } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


void DIALOG_TABLECELL_PROPERTIES::onVAlignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_vAlignTop, m_vAlignCenter, m_vAlignBottom } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


bool DIALOG_TABLECELL_PROPERTIES::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    BOARD_COMMIT commit( m_frame );
    commit.Modify( m_table );

    // If no other command in progress, prepare undo command
    // (for a command in progress, will be made later, at the completion of command)
    bool pushCommit = ( m_table->GetEditFlags() == 0 );

    // Set IN_EDIT flag to force undo/redo/abort proper operation and avoid new calls to
    // SaveCopyInUndoList for the same text if is moved, and then rotated, edited, etc....
    if( !pushCommit )
        m_table->SetFlags( IN_EDIT );

    for( PCB_TABLECELL* cell : m_cells )
    {
        if( m_cellTextCtrl->GetValue() != INDETERMINATE_STATE )
        {
            wxString txt = m_cellTextCtrl->GetValue();

#ifdef __WXMAC__
            // On macOS CTRL+Enter produces '\r' instead of '\n' regardless of EOL setting.
            // Replace it now.
            txt.Replace( "\r", "\n" );
#elif defined( __WINDOWS__ )
            // On Windows, a new line is coded as \r\n.  We use only \n in kicad files and in
            // drawing routines so strip the \r char.
            txt.Replace( "\r", "" );
#endif

            cell->SetText( txt );
        }

        cell->SetBold( m_bold->IsChecked() );
        cell->SetItalic( m_italic->IsChecked() );

        if( m_fontCtrl->HaveFontSelection() )
            cell->SetFont( m_fontCtrl->GetFontSelection( cell->IsBold(), cell->IsItalic() ) );

        if( !m_textWidth.IsIndeterminate() )
            cell->SetTextWidth( m_textWidth.GetIntValue() );

        if( !m_textHeight.IsIndeterminate() )
            cell->SetTextHeight( m_textHeight.GetIntValue() );

        if( m_autoTextThickness->IsChecked() )
            cell->SetAutoThickness( true );
        else if( !m_textThickness.IsIndeterminate() )
            cell->SetTextThickness( m_textThickness.GetIntValue() );

        if( m_hAlignLeft->IsChecked() )
            cell->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        else if( m_hAlignRight->IsChecked() )
            cell->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        else if( m_hAlignCenter->IsChecked() )
            cell->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );

        if( m_vAlignTop->IsChecked() )
            cell->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        else if( m_vAlignBottom->IsChecked() )
            cell->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        else if( m_vAlignCenter->IsChecked() )
            cell->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );

        if( !m_marginLeft.IsIndeterminate() )
            cell->SetMarginLeft( m_marginLeft.GetIntValue() );

        if( !m_marginTop.IsIndeterminate() )
            cell->SetMarginTop( m_marginTop.GetIntValue() );

        if( !m_marginRight.IsIndeterminate() )
            cell->SetMarginRight( m_marginRight.GetIntValue() );

        if( !m_marginBottom.IsIndeterminate() )
            cell->SetMarginBottom( m_marginBottom.GetIntValue() );
    }

    if( !commit.Empty() )
        commit.Push( _( "Edit Table Cell Properties" ), SKIP_CONNECTIVITY );

    m_returnValue = TABLECELL_PROPS_OK;
    return true;
}


void DIALOG_TABLECELL_PROPERTIES::onEditTable( wxCommandEvent& aEvent )
{
    if( TransferDataFromWindow() )
    {
        m_returnValue = TABLECELL_PROPS_EDIT_TABLE;
        Close();
    }
}


void DIALOG_TABLECELL_PROPERTIES::onSyntaxHelp( wxHyperlinkEvent& aEvent )
{
    if( m_helpWindow )
    {
        m_helpWindow->Raise();
        m_helpWindow->Show( true );
        return;
    }

    m_helpWindow = PCB_TEXT::ShowSyntaxHelp( this );
}