/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_edit_frame.h>
#include <widgets/bitmap_button.h>
#include <widgets/font_choice.h>
#include <widgets/color_swatch.h>
#include <widgets/wx_combobox.h>
#include <settings/color_settings.h>
#include <sch_textbox.h>
#include <confirm.h>
#include <schematic.h>
#include <sch_commit.h>
#include <dialogs/html_message_box.h>
#include <scintilla_tricks.h>
#include <dialog_text_properties.h>
#include <gr_text.h>


DIALOG_TEXT_PROPERTIES::DIALOG_TEXT_PROPERTIES( SCH_EDIT_FRAME* aParent, SCH_ITEM* aTextItem ) :
        DIALOG_TEXT_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_currentItem( aTextItem ),
        m_currentText( dynamic_cast<EDA_TEXT*>( aTextItem ) ),
        m_textSize( aParent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits ),
        m_borderWidth( aParent, m_borderWidthLabel, m_borderWidthCtrl, m_borderWidthUnits ),
        m_scintillaTricks( nullptr ),
        m_helpWindow( nullptr )
{
    COLOR_SETTINGS* colorSettings = m_frame->GetColorSettings();
    COLOR4D         schematicBackground = colorSettings->GetColor( LAYER_SCHEMATIC_BACKGROUND );

    if( aTextItem->Type() == SCH_TEXTBOX_T )
    {
        SetTitle( _( "Text Box Properties" ) );

        m_borderColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
        m_borderColorSwatch->SetSwatchBackground( schematicBackground );

        for( const std::pair<const PLOT_DASH_TYPE, lineTypeStruct>& typeEntry : lineTypeNames )
            m_borderStyleCombo->Append( typeEntry.second.name, KiBitmap( typeEntry.second.bitmap ) );

        m_borderStyleCombo->Append( DEFAULT_STYLE );

        m_fillColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
        m_fillColorSwatch->SetSwatchBackground( schematicBackground );

        if( m_frame->GetColorSettings()->GetOverrideSchItemColors() )
            m_infoBar->ShowMessage( _( "Note: individual item colors overridden in Preferences." ) );
    }
    else
    {
        m_hAlignCenter->Show( false );
        m_separator3->Show( false );
        m_vAlignTop->Show( false );
        m_vAlignCenter->Show( false );
        m_vAlignBottom->Show( false );

        wxSizer* parentSizer = m_vAlignTop->GetContainingSizer();
        parentSizer->Detach( m_hAlignCenter );
        parentSizer->Detach( m_separator3 );
        parentSizer->Detach( m_vAlignTop );
        parentSizer->Detach( m_vAlignCenter );
        parentSizer->Detach( m_vAlignBottom );
        parentSizer->Layout();

        m_borderCheckbox->Show( false );
        m_borderWidth.Show( false );
        m_borderColorLabel->Show( false );
        m_panelBorderColor->Show( false );
        m_borderStyleLabel->Show( false );
        m_borderStyleCombo->Show( false );
        m_fillColorLabel->Show( false );
        m_panelFillColor->Show( false );
        m_filledCtrl->Show( false );
    }

    m_textCtrl->SetEOLMode( wxSTC_EOL_LF );

    m_scintillaTricks = new SCINTILLA_TRICKS( m_textCtrl, wxT( "{}" ), false,
            // onAccept handler
            [this]()
            {
                wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
            },
            // onCharAdded handler
            [this]( wxStyledTextEvent& aEvent )
            {
                m_scintillaTricks->DoTextVarAutocomplete(
                        [this]( const wxString& crossRef, wxArrayString* tokens )
                        {
                            getContextualTextVars( crossRef, tokens );
                        } );
            } );

    m_textEntrySizer->AddGrowableRow( 0 );

    m_textColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_textColorSwatch->SetSwatchBackground( schematicBackground );

    SetInitialFocus( m_textCtrl );

    m_separator1->SetIsSeparator();

    m_bold->SetIsCheckButton();
    m_bold->SetBitmap( KiBitmap( BITMAPS::text_bold ) );
    m_italic->SetIsCheckButton();
    m_italic->SetBitmap( KiBitmap( BITMAPS::text_italic ) );

    m_separator2->SetIsSeparator();

    m_hAlignLeft->SetIsRadioButton();
    m_hAlignLeft->SetBitmap( KiBitmap( BITMAPS::text_align_left ) );
    m_hAlignCenter->SetIsRadioButton();
    m_hAlignCenter->SetBitmap( KiBitmap( BITMAPS::text_align_center ) );
    m_hAlignRight->SetIsRadioButton();
    m_hAlignRight->SetBitmap( KiBitmap( BITMAPS::text_align_right ) );
    m_separator3->SetIsSeparator();

    m_vAlignTop->SetIsRadioButton();
    m_vAlignTop->SetBitmap( KiBitmap( BITMAPS::text_valign_top ) );
    m_vAlignCenter->SetIsRadioButton();
    m_vAlignCenter->SetBitmap( KiBitmap( BITMAPS::text_valign_center ) );
    m_vAlignBottom->SetIsRadioButton();
    m_vAlignBottom->SetBitmap( KiBitmap( BITMAPS::text_valign_bottom ) );

    m_separator4->SetIsSeparator();

    m_horizontal->SetIsRadioButton();
    m_horizontal->SetBitmap( KiBitmap( BITMAPS::text_horizontal ) );
    m_vertical->SetIsRadioButton();
    m_vertical->SetBitmap( KiBitmap( BITMAPS::text_vertical ) );

    m_separator5->SetIsSeparator();

    SCH_SHEET_LIST sheetList = m_frame->Schematic().GetSheets();
    sheetList.SortByPageNumbers( false );

    for( const SCH_SHEET_PATH& sheet : sheetList )
    {
        wxString sheetPageNum = sheet.GetPageNumber();
        wxString sheetName = sheet.size() == 1 ? _( "<root sheet>" ) : sheet.Last()->GetName();

        m_hyperlinkCombo->Append( wxT( "#" ) + sheetPageNum,
                                  wxString::Format( _( "Page %s (%s)" ), sheetPageNum, sheetName ) );
        m_pageNumbers.push_back( sheetPageNum );
    }

    m_hyperlinkCombo->Append( wxT( "---" ) );
    m_hyperlinkCombo->Append( wxT( "file://" ), wxT( "file://..." ) );
    m_hyperlinkCombo->Append( wxT( "http://" ), wxT( "http://..." ) );
    m_hyperlinkCombo->Append( wxT( "https://" ), wxT( "https://..." ) );

    SetupStandardButtons();
    Layout();

    m_hAlignLeft->Bind( wxEVT_BUTTON, &DIALOG_TEXT_PROPERTIES::onHAlignButton, this );
    m_hAlignCenter->Bind( wxEVT_BUTTON, &DIALOG_TEXT_PROPERTIES::onHAlignButton, this );
    m_hAlignRight->Bind( wxEVT_BUTTON, &DIALOG_TEXT_PROPERTIES::onHAlignButton, this );
    m_vAlignTop->Bind( wxEVT_BUTTON, &DIALOG_TEXT_PROPERTIES::onVAlignButton, this );
    m_vAlignCenter->Bind( wxEVT_BUTTON, &DIALOG_TEXT_PROPERTIES::onVAlignButton, this );
    m_vAlignBottom->Bind( wxEVT_BUTTON, &DIALOG_TEXT_PROPERTIES::onVAlignButton, this );
    m_horizontal->Bind( wxEVT_BUTTON, &DIALOG_TEXT_PROPERTIES::onTextAngleButton, this );
    m_vertical->Bind( wxEVT_BUTTON, &DIALOG_TEXT_PROPERTIES::onTextAngleButton, this );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_TEXT_PROPERTIES::~DIALOG_TEXT_PROPERTIES()
{
    delete m_scintillaTricks;

    if( m_helpWindow )
        m_helpWindow->Destroy();
}


void DIALOG_TEXT_PROPERTIES::getContextualTextVars( const wxString& aCrossRef,
                                                    wxArrayString*  aTokens )
{
    if( !aCrossRef.IsEmpty() )
    {
        SCH_SHEET_LIST     sheets = m_frame->Schematic().GetSheets();
        SCH_REFERENCE_LIST refs;
        SCH_SYMBOL*        refSymbol = nullptr;

        sheets.GetSymbols( refs );

        for( int jj = 0; jj < (int) refs.GetCount(); jj++ )
        {
            SCH_REFERENCE& ref = refs[jj];

            if( ref.GetSymbol()->GetRef( &ref.GetSheetPath(), true ) == aCrossRef )
            {
                refSymbol = ref.GetSymbol();
                break;
            }
        }

        if( refSymbol )
            refSymbol->GetContextualTextVars( aTokens );
    }
    else
    {
        SCHEMATIC* schematic = m_currentItem->Schematic();

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


bool DIALOG_TEXT_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    SCHEMATIC& schematic = m_frame->Schematic();

    m_hyperlinkCb->SetValue( m_currentText->HasHyperlink() );
    m_hyperlinkCombo->SetValue( m_currentText->GetHyperlink() );

    // show text variable cross-references in a human-readable format
    m_textCtrl->SetValue( schematic.ConvertKIIDsToRefs( m_currentText->GetText() ) );
    m_textCtrl->EmptyUndoBuffer();

    m_excludeFromSim->SetValue( m_currentItem->GetExcludeFromSim() );

    m_fontCtrl->SetFontSelection( m_currentText->GetFont() );
    m_textSize.SetValue( m_currentText->GetTextWidth() );
    m_textColorSwatch->SetSwatchColor( m_currentText->GetTextColor(), false );

    m_bold->Check( m_currentText->IsBold() );
    m_italic->Check( m_currentText->IsItalic() );

    if( m_currentItem->Type() == SCH_TEXTBOX_T )
    {
        SCH_TEXTBOX* textBox = static_cast<SCH_TEXTBOX*>( m_currentItem );

        switch( m_currentText->GetHorizJustify() )
        {
        case GR_TEXT_H_ALIGN_LEFT:   m_hAlignLeft->Check();   break;
        case GR_TEXT_H_ALIGN_CENTER: m_hAlignCenter->Check(); break;
        case GR_TEXT_H_ALIGN_RIGHT:  m_hAlignRight->Check();  break;
        }

        switch( m_currentText->GetVertJustify() )
        {
        case GR_TEXT_V_ALIGN_TOP:    m_vAlignTop->Check();    break;
        case GR_TEXT_V_ALIGN_CENTER: m_vAlignCenter->Check(); break;
        case GR_TEXT_V_ALIGN_BOTTOM: m_vAlignBottom->Check(); break;
        }

        if( m_currentText->GetTextAngle() == ANGLE_VERTICAL )
            m_vertical->Check();
        else
            m_horizontal->Check();

        m_borderCheckbox->SetValue( textBox->GetWidth() >= 0 );

        if( textBox->GetWidth() >= 0 )
            m_borderWidth.SetValue( textBox->GetWidth() );

        m_borderColorSwatch->SetSwatchColor( textBox->GetStroke().GetColor(), false );

        int style = static_cast<int>( textBox->GetStroke().GetPlotStyle() );

        if( style == -1 )
            m_borderStyleCombo->SetStringSelection( DEFAULT_STYLE );
        else if( style < (int) lineTypeNames.size() )
            m_borderStyleCombo->SetSelection( style );
        else
            wxFAIL_MSG( "Line type not found in the type lookup map" );

        m_borderWidth.Enable( textBox->GetWidth() >= 0 );
        m_borderColorLabel->Enable( textBox->GetWidth() >= 0 );
        m_borderColorSwatch->Enable( textBox->GetWidth() >= 0 );
        m_borderStyleLabel->Enable( textBox->GetWidth() >= 0 );
        m_borderStyleCombo->Enable( textBox->GetWidth() >= 0 );

        m_filledCtrl->SetValue( textBox->IsFilled() );
        m_fillColorSwatch->SetSwatchColor( textBox->GetFillColor(), false );

        m_fillColorLabel->Enable( textBox->IsFilled() );
        m_fillColorSwatch->Enable( textBox->IsFilled() );
    }
    else
    {
        switch( static_cast<SCH_TEXT*>( m_currentItem )->GetTextSpinStyle() )
        {
        case TEXT_SPIN_STYLE::RIGHT:
            m_hAlignLeft->Check( true );    // Spin style to right means text aligned left
            m_horizontal->Check( true );
            break;
        case TEXT_SPIN_STYLE::LEFT:
            m_hAlignRight->Check( true );   // Spin style to left means text aligned right
            m_horizontal->Check( true );
            break;
        case TEXT_SPIN_STYLE::UP:
            m_hAlignLeft->Check( true );  // Spin style up means text aligned to bottom
            m_vertical->Check( true );
            break;
        case TEXT_SPIN_STYLE::BOTTOM:       // Spin style down means text aligned to top
            m_hAlignRight->Check( true );
            m_vertical->Check( true );
            break;
        }
    }

    return true;
}


void DIALOG_TEXT_PROPERTIES::onBorderChecked( wxCommandEvent& aEvent )
{
    bool border = m_borderCheckbox->GetValue();

    if( border && m_borderWidth.GetValue() < 0 )
        m_borderWidth.SetValue( m_frame->eeconfig()->m_Drawing.default_line_thickness );

    m_borderWidth.Enable( border );
    m_borderColorLabel->Enable( border );
    m_borderColorSwatch->Enable( border );
    m_borderStyleLabel->Enable( border );
    m_borderStyleCombo->Enable( border );
}


void DIALOG_TEXT_PROPERTIES::onFillChecked( wxCommandEvent& aEvent )
{
    bool fill = m_filledCtrl->GetValue();

    m_fillColorLabel->Enable( fill );
    m_fillColorSwatch->Enable( fill );
}


void DIALOG_TEXT_PROPERTIES::onHyperlinkChecked( wxCommandEvent& aEvent )
{
    if( aEvent.IsChecked() && m_hyperlinkCombo->GetValue().IsEmpty() )
    {
        m_hyperlinkCombo->ChangeValue( m_lastLink );
    }
    else if( !aEvent.IsChecked() && !m_hyperlinkCombo->GetValue().IsEmpty() )
    {
        m_lastLink = m_hyperlinkCombo->GetValue();
        m_hyperlinkCombo->SetValue( wxEmptyString );
    }

    aEvent.Skip();
}


void DIALOG_TEXT_PROPERTIES::onHyperlinkText( wxCommandEvent& event )
{
    if( !m_hyperlinkCombo->GetValue().IsEmpty() )
        m_hyperlinkCb->SetValue( true );
}


void DIALOG_TEXT_PROPERTIES::onHyperlinkCombo( wxCommandEvent& aEvent )
{
    if( aEvent.GetSelection() >= 0 )
    {
        m_hyperlinkCb->SetValue( true );
        m_hyperlinkCombo->SetInsertionPointEnd();
    }
}


void DIALOG_TEXT_PROPERTIES::onHAlignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_hAlignLeft, m_hAlignCenter, m_hAlignRight } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


void DIALOG_TEXT_PROPERTIES::onVAlignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_vAlignTop, m_vAlignCenter, m_vAlignBottom } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


void DIALOG_TEXT_PROPERTIES::onTextAngleButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_horizontal, m_vertical } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


bool DIALOG_TEXT_PROPERTIES::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    // Don't allow text to disappear; it can be difficult to correct if you can't select it
    if( !m_textSize.Validate( 0.01, 1000.0, EDA_UNITS::MILLIMETRES ) )
        return false;

    SCH_COMMIT commit( m_frame );
    wxString   text;

    /* save old text in undo list if not already in edit */
    if( m_currentItem->GetEditFlags() == 0 )
        commit.Modify( m_currentItem, m_frame->GetScreen() );

    m_frame->GetCanvas()->Refresh();

    // convert any text variable cross-references to their UUIDs
    text = m_frame->Schematic().ConvertRefsToKIIDs( m_textCtrl->GetValue() );

#ifdef __WXMAC__
    // On macOS CTRL+Enter produces '\r' instead of '\n' regardless of EOL setting
    text.Replace( "\r", "\n" );
#elif defined( __WINDOWS__ )
    // On Windows, a new line is coded as \r\n.  We use only \n in kicad files and in
    // drawing routines so strip the \r char.
    text.Replace( "\r", "" );
#endif

    if( m_currentItem->Type() == SCH_TEXTBOX_T )
    {
        // Textboxes have a defined extent and so are allowed to be empty
        m_currentText->SetText( text );
    }
    else if( !text.IsEmpty() )
    {
        m_currentText->SetText( text );
    }
    else
    {
        // Other text items do not have defined extents, and so will disappear if empty
        DisplayError( this, _( "Text can not be empty." ) );
        return false;
    }

    m_currentItem->SetExcludeFromSim( m_excludeFromSim->GetValue() );

    if( !m_currentText->ValidateHyperlink( m_hyperlinkCombo->GetValue() ) )
    {
        DisplayError( this, _( "Invalid hyperlink destination. Please enter either a valid URL "
                               "(e.g. file:// or http(s)://) or \"#<page number>\" to create "
                               "a hyperlink to a page in this schematic." ) );
        return false;
    }
    else
    {
        m_currentText->SetHyperlink( m_hyperlinkCombo->GetValue() );
    }

    if( m_currentText->GetTextWidth() != m_textSize.GetValue() )
        m_currentText->SetTextSize( VECTOR2I( m_textSize.GetValue(), m_textSize.GetValue() ) );

    if( m_fontCtrl->HaveFontSelection() )
    {
        m_currentText->SetFont( m_fontCtrl->GetFontSelection( m_bold->IsChecked(),
                                                              m_italic->IsChecked() ) );
    }

    if( m_bold->IsChecked() != m_currentText->IsBold() )
    {
        if( m_bold->IsChecked() )
        {
            m_currentText->SetBold( true );
            m_currentText->SetTextThickness( GetPenSizeForBold( m_currentText->GetTextWidth() ) );
        }
        else
        {
            m_currentText->SetBold( false );
            m_currentText->SetTextThickness( 0 ); // Use default pen width
        }
    }

    m_currentText->SetItalic( m_italic->IsChecked() );
    m_currentText->SetTextColor( m_textColorSwatch->GetSwatchColor() );

    if( m_currentItem->Type() == SCH_TEXT_T )
    {
        SCH_TEXT* textItem = static_cast<SCH_TEXT*>( m_currentItem );

        if( m_hAlignRight->IsChecked() )
        {
            if( m_vertical->IsChecked() )
                textItem->SetTextSpinStyle( TEXT_SPIN_STYLE::BOTTOM );
            else
                textItem->SetTextSpinStyle( TEXT_SPIN_STYLE::LEFT );
        }
        else
        {
            if( m_vertical->IsChecked() )
                textItem->SetTextSpinStyle( TEXT_SPIN_STYLE::UP );
            else
                textItem->SetTextSpinStyle( TEXT_SPIN_STYLE::RIGHT );
        }
    }
    else
    {
        SCH_TEXTBOX* textBox = static_cast<SCH_TEXTBOX*>( m_currentItem );

        if( m_hAlignRight->IsChecked() )
            m_currentText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        else if( m_hAlignCenter->IsChecked() )
            m_currentText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        else
            m_currentText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );

        if( m_vAlignBottom->IsChecked() )
            m_currentText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        else if( m_vAlignCenter->IsChecked() )
            m_currentText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        else
            m_currentText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );

        if( m_vertical->IsChecked() )
            m_currentText->SetTextAngle( ANGLE_VERTICAL );
        else
            m_currentText->SetTextAngle( ANGLE_HORIZONTAL );

        STROKE_PARAMS stroke = textBox->GetStroke();

        if( m_borderCheckbox->GetValue() )
            stroke.SetWidth( std::max( 0, m_borderWidth.GetIntValue() ) );
        else
            stroke.SetWidth( -1 );

        auto it = lineTypeNames.begin();
        std::advance( it, m_borderStyleCombo->GetSelection() );

        if( it == lineTypeNames.end() )
            stroke.SetPlotStyle( PLOT_DASH_TYPE::DEFAULT );
        else
            stroke.SetPlotStyle( it->first );

        stroke.SetColor( m_borderColorSwatch->GetSwatchColor() );

        textBox->SetStroke( stroke );

        textBox->SetFillMode( m_filledCtrl->GetValue() ? FILL_T::FILLED_WITH_COLOR
                                                       : FILL_T::NO_FILL );
        textBox->SetFillColor( m_fillColorSwatch->GetSwatchColor() );
    }

    if( !commit.Empty() )
        commit.Push( _( "Edit Text" ), SKIP_CONNECTIVITY );

    return true;
}


void DIALOG_TEXT_PROPERTIES::OnFormattingHelp( wxHyperlinkEvent& aEvent )
{
    m_helpWindow = SCH_TEXT::ShowSyntaxHelp( this );
}


void DIALOG_TEXT_PROPERTIES::onMultiLineTCLostFocus( wxFocusEvent& event )
{
    if( m_scintillaTricks )
        m_scintillaTricks->CancelAutocomplete();

    event.Skip();
}
