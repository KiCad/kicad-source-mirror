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

#include <gr_text.h>
#include <sch_edit_frame.h>
#include <widgets/bitmap_button.h>
#include <widgets/color_swatch.h>
#include <widgets/font_choice.h>
#include <settings/color_settings.h>
#include <sch_table.h>
#include <sch_text.h>
#include <sch_commit.h>
#include <tool/tool_manager.h>
#include <dialogs/html_message_box.h>
#include <dialog_tablecell_properties.h>

DIALOG_TABLECELL_PROPERTIES::DIALOG_TABLECELL_PROPERTIES( SCH_EDIT_FRAME* aFrame, std::vector<SCH_TABLECELL*> aCells ) :
        DIALOG_TABLECELL_PROPERTIES_BASE( aFrame ),
        m_frame( aFrame ),
        m_table( nullptr ),
        m_cells( std::move( aCells ) ),
        m_scintillaTricks( nullptr ),
        m_helpWindow( nullptr ),
        m_textSize( aFrame, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits ),
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
                            getContextualTextVars( xRef, tokens );
                        } );
            } );

    SetInitialFocus( m_cellText );

    m_table = static_cast<SCH_TABLE*>( m_cells[0]->GetParent() );

    if( m_frame->GetColorSettings()->GetOverrideSchItemColors() )
        m_infoBar->ShowMessage( _( "Note: individual item colors overridden in Preferences." ) );

    m_hAlignLeft->SetIsRadioButton();
    m_hAlignLeft->SetBitmap( KiBitmapBundle( BITMAPS::text_align_left ) );
    m_hAlignCenter->SetIsRadioButton();
    m_hAlignCenter->SetBitmap( KiBitmapBundle( BITMAPS::text_align_center ) );
    m_hAlignRight->SetIsRadioButton();
    m_hAlignRight->SetBitmap( KiBitmapBundle( BITMAPS::text_align_right ) );

    m_vAlignTop->SetIsRadioButton();
    m_vAlignTop->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_top ) );
    m_vAlignCenter->SetIsRadioButton();
    m_vAlignCenter->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_center ) );
    m_vAlignBottom->SetIsRadioButton();
    m_vAlignBottom->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_bottom ) );

    KIGFX::COLOR4D canvas = aFrame->GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );
    m_textColorSwatch->SetSwatchBackground( canvas );
    m_fillColorSwatch->SetSwatchBackground( canvas );

    SetupStandardButtons();
    Layout();

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
    delete m_scintillaTricks;
    m_scintillaTricks = nullptr;

    if( m_helpWindow )
        m_helpWindow->Destroy();
}

void DIALOG_TABLECELL_PROPERTIES::getContextualTextVars( const wxString& aCrossRef, wxArrayString* aTokens )
{
    SCHEMATIC* schematic = m_table->Schematic();

    if( !aCrossRef.IsEmpty() )
    {
        SCH_SYMBOL* refSymbol = nullptr;

        if( schematic )
        {
            SCH_REFERENCE_LIST refs;
            schematic->Hierarchy().GetSymbols( refs );

            for( int jj = 0; jj < (int) refs.GetCount(); jj++ )
            {
                SCH_REFERENCE& ref = refs[jj];

                if( ref.GetSymbol()->GetRef( &ref.GetSheetPath(), true ) == aCrossRef )
                {
                    refSymbol = ref.GetSymbol();
                    break;
                }
            }
        }

        if( refSymbol )
            refSymbol->GetContextualTextVars( aTokens );
    }
    else
    {
        if( schematic && schematic->CurrentSheet().Last() )
        {
            schematic->CurrentSheet().Last()->GetContextualTextVars( aTokens );
        }
        else
        {
            for( std::pair<wxString, wxString> entry : Prj().GetTextVars() )
                aTokens->push_back( entry.first );
        }
    }
}


bool DIALOG_TABLECELL_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    bool              firstCell = true;
    GR_TEXT_H_ALIGN_T hAlign = GR_TEXT_H_ALIGN_INDETERMINATE;
    GR_TEXT_V_ALIGN_T vAlign = GR_TEXT_V_ALIGN_INDETERMINATE;

    for( SCH_TABLECELL* cell : m_cells )
    {
        wxString text = cell->GetText();

        m_cellText->SetValue( text );
        m_cellText->EmptyUndoBuffer();

        if( firstCell )
        {
            m_fontCtrl->SetFontSelection( cell->GetFont() );
            m_textSize.SetValue( cell->GetTextWidth() );

            m_bold->Set3StateValue( cell->IsBold() ? wxCHK_CHECKED : wxCHK_UNCHECKED );
            m_italic->Set3StateValue( cell->IsItalic() ? wxCHK_CHECKED : wxCHK_UNCHECKED );

            hAlign = cell->GetHorizJustify();
            vAlign = cell->GetVertJustify();

            m_textColorBook->SetSelection( 1 );
            m_textColorSwatch->SetSwatchColor( cell->GetTextColor(), false );
            m_textColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

            m_fillColorBook->SetSelection( 1 );

            if( cell->IsSolidFill() )
                m_fillColorSwatch->SetSwatchColor( cell->GetFillColor(), false );
            else
                m_fillColorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );

            m_fillColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

            m_marginLeft.SetValue( cell->GetMarginLeft() );
            m_marginTop.SetValue( cell->GetMarginTop() );
            m_marginRight.SetValue( cell->GetMarginRight() );
            m_marginBottom.SetValue( cell->GetMarginBottom() );

            firstCell = false;
        }
        else
        {
            if( cell->GetFont() != m_fontCtrl->GetFontSelection( cell->IsBold(), cell->IsItalic() ) )
                m_fontCtrl->SetSelection( -1 );

            if( cell->GetTextWidth() != m_textSize.GetValue() )
                m_textSize.SetValue( INDETERMINATE_STATE );

            wxCheckBoxState bold = cell->IsBold() ? wxCHK_CHECKED : wxCHK_UNCHECKED;

            if( bold != m_bold->Get3StateValue() )
                m_bold->Set3StateValue( wxCHK_UNDETERMINED );

            wxCheckBoxState italic = cell->IsItalic() ? wxCHK_CHECKED : wxCHK_UNCHECKED;

            if( italic != m_italic->Get3StateValue() )
                m_italic->Set3StateValue( wxCHK_UNDETERMINED );

            if( cell->GetHorizJustify() != hAlign )
                hAlign = GR_TEXT_H_ALIGN_INDETERMINATE;

            if( cell->GetVertJustify() != vAlign )
                vAlign = GR_TEXT_V_ALIGN_INDETERMINATE;

            if( cell->GetTextColor() != m_textColorSwatch->GetSwatchColor() )
            {
                m_textColorBook->SetSelection( 0 );
                m_textColorPopup->SetSelection( 0 );
            }

            COLOR4D fillColor = cell->IsSolidFill() ? cell->GetFillColor() : COLOR4D::UNSPECIFIED;

            if( fillColor != m_fillColorSwatch->GetSwatchColor() )
            {
                m_fillColorBook->SetSelection( 0 );
                m_fillColorPopup->SetSelection( 0 );
            }

            if( fillColor != m_fillColorSwatch->GetSwatchColor() )
                fillColor = COLOR4D::UNSPECIFIED;

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
        case GR_TEXT_H_ALIGN_LEFT:          m_hAlignLeft->Check();   break;
        case GR_TEXT_H_ALIGN_CENTER:        m_hAlignCenter->Check(); break;
        case GR_TEXT_H_ALIGN_RIGHT:         m_hAlignRight->Check();  break;
        case GR_TEXT_H_ALIGN_INDETERMINATE:                          break;
        }

        switch( vAlign )
        {
        case GR_TEXT_V_ALIGN_TOP:           m_vAlignTop->Check();    break;
        case GR_TEXT_V_ALIGN_CENTER:        m_vAlignCenter->Check(); break;
        case GR_TEXT_V_ALIGN_BOTTOM:        m_vAlignBottom->Check(); break;
        case GR_TEXT_V_ALIGN_INDETERMINATE:                          break;
        }
    }

    return true;
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


void DIALOG_TABLECELL_PROPERTIES::onTextColorPopup( wxCommandEvent& aEvent )
{
    if( aEvent.GetSelection() == 1 )
    {
        m_textColorBook->SetSelection( 1 );
        m_textColorSwatch->GetNewSwatchColor();
    }
}


void DIALOG_TABLECELL_PROPERTIES::onFillColorPopup( wxCommandEvent& aEvent )
{
    if( aEvent.GetSelection() == 1 )
    {
        m_fillColorBook->SetSelection( 1 );
        m_fillColorSwatch->GetNewSwatchColor();
    }
}


bool DIALOG_TABLECELL_PROPERTIES::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    SCH_COMMIT commit( m_frame );

    /* save table in undo list if not already in edit */
    if( m_table->GetEditFlags() == 0 )
        commit.Modify( m_table, m_frame->GetScreen() );

    for( SCH_TABLECELL* cell : m_cells )
    {
        wxString text = m_cellTextCtrl->GetValue();

#ifdef __WXMAC__
        // On macOS CTRL+Enter produces '\r' instead of '\n' regardless of EOL setting
        text.Replace( "\r", "\n" );
#elif defined( __WINDOWS__ )
        // On Windows, a new line is coded as \r\n.  We use only \n in KiCad files and in
        // drawing routines so strip the \r char.
        text.Replace( "\r", "" );
#endif

        cell->SetText( text );

        if( m_bold->Get3StateValue() == wxCHK_CHECKED )
            cell->SetBold( true );
        else if( m_bold->Get3StateValue() == wxCHK_UNCHECKED )
            cell->SetBold( false );

        if( m_italic->Get3StateValue() == wxCHK_CHECKED )
            cell->SetItalic( true );
        else if( m_italic->Get3StateValue() == wxCHK_UNCHECKED )
            cell->SetItalic( false );

        if( m_fontCtrl->HaveFontSelection() )
            cell->SetFont( m_fontCtrl->GetFontSelection( cell->IsBold(), cell->IsItalic() ) );

        if( !m_textSize.IsIndeterminate() )
            cell->SetTextSize( VECTOR2I( m_textSize.GetIntValue(), m_textSize.GetIntValue() ) );

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

        if( m_textColorBook->GetSelection() == 1 )
            cell->SetTextColor( m_textColorSwatch->GetSwatchColor() );

        if( m_fillColorBook->GetSelection() == 1 )
        {
            COLOR4D fillColor = m_fillColorSwatch->GetSwatchColor();

            if( fillColor == COLOR4D::UNSPECIFIED )
            {
                cell->SetFillMode( FILL_T::NO_FILL );
            }
            else
            {
                cell->SetFillMode( FILL_T::FILLED_WITH_COLOR );
                cell->SetFillColor( fillColor );
            }
        }

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
        commit.Push( _( "Edit Table Cell Properties" ) );

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

void DIALOG_TABLECELL_PROPERTIES::onMultiLineTCLostFocus( wxFocusEvent& event )
{
    if( m_scintillaTricks )
        m_scintillaTricks->CancelAutocomplete();

    event.Skip();
}


void DIALOG_TABLECELL_PROPERTIES::OnFormattingHelp( wxHyperlinkEvent& aEvent )
{
    if( m_helpWindow )
    {
        m_helpWindow->Raise();
        m_helpWindow->Show( true );
        return;
    }

    m_helpWindow = SCH_TEXT::ShowSyntaxHelp( this );
}