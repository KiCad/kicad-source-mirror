/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <widgets/bitmap_button.h>
#include <widgets/color_swatch.h>
#include <widgets/font_choice.h>
#include <settings/color_settings.h>
#include <sch_edit_frame.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_validators.h>
#include <sch_commit.h>
#include <dialog_sheet_pin_properties.h>
#include <dialogs/html_message_box.h>
#include <string_utils.h>
#include <gr_text.h>


DIALOG_SHEET_PIN_PROPERTIES::DIALOG_SHEET_PIN_PROPERTIES( SCH_EDIT_FRAME* parent, SCH_SHEET_PIN* aPin ) :
        DIALOG_SHEET_PIN_PROPERTIES_BASE( parent ),
        m_frame( parent ),
        m_sheetPin( aPin ),
        m_textSize( parent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits ),
        m_helpWindow( nullptr )
{
    COLOR_SETTINGS* colorSettings = m_frame->GetColorSettings();
    COLOR4D         schematicBackground = colorSettings->GetColor( LAYER_SCHEMATIC_BACKGROUND );

    m_separator1->SetIsSeparator();

    m_bold->SetIsCheckButton();
    m_bold->SetBitmap( KiBitmapBundle( BITMAPS::text_bold ) );
    m_italic->SetIsCheckButton();
    m_italic->SetBitmap( KiBitmapBundle( BITMAPS::text_italic ) );

    m_separator2->SetIsSeparator();

    m_textColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_textColorSwatch->SetSwatchBackground( schematicBackground );

    SetInitialFocus( m_comboName );
    SetupStandardButtons();

    // Set invalid label characters list:
    SCH_NETNAME_VALIDATOR validator( true );
    m_comboName->SetValidator( validator );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();

    // On some windows manager (Unity, XFCE), this dialog is not always raised, depending on how it is run.
    Raise();
}


DIALOG_SHEET_PIN_PROPERTIES::~DIALOG_SHEET_PIN_PROPERTIES()
{
    if( m_helpWindow )
        m_helpWindow->Destroy();
}


bool DIALOG_SHEET_PIN_PROPERTIES::TransferDataToWindow()
{
    SCH_SCREEN* screen = m_sheetPin->GetParent()->GetScreen();

    for( SCH_ITEM* item : screen->Items().OfType( SCH_HIER_LABEL_T ) )
    {
        wxString txt = static_cast<SCH_HIERLABEL*>( item )->GetText();

        if( m_comboName->FindString( txt, true ) == wxNOT_FOUND )
            m_comboName->Append( txt );
    }

    m_comboName->SetValue( UnescapeString( m_sheetPin->GetText() ) );
    m_comboName->SelectAll();
    m_fontCtrl->SetFontSelection( m_sheetPin->GetFont() );

    m_bold->Check( m_sheetPin->IsBold() );
    m_italic->Check( m_sheetPin->IsItalic() );

    // Currently, eeschema uses only the text width as text size
    // (only the text width is saved in files), and expects text width = text height
    m_textSize.SetValue( m_sheetPin->GetTextWidth() );

    m_textColorSwatch->SetSwatchColor( m_sheetPin->GetTextColor(), false );

    switch( m_sheetPin->GetShape() )
    {
    case LABEL_FLAG_SHAPE::L_INPUT:       m_input->SetValue( true );         break;
    case LABEL_FLAG_SHAPE::L_OUTPUT:      m_output->SetValue( true );        break;
    case LABEL_FLAG_SHAPE::L_BIDI:        m_bidirectional->SetValue( true ); break;
    case LABEL_FLAG_SHAPE::L_TRISTATE:    m_triState->SetValue( true );      break;
    case LABEL_FLAG_SHAPE::L_UNSPECIFIED: m_passive->SetValue( true );       break;
    default: wxFAIL_MSG( wxT( "Unknown sheet pin shape" ) );                 break;
    }

    return true;
}


bool DIALOG_SHEET_PIN_PROPERTIES::TransferDataFromWindow()
{
    SCH_COMMIT commit( m_frame );

    if( !m_sheetPin->IsNew() )
        commit.Modify( m_sheetPin->GetParent(), m_frame->GetScreen() );

    m_sheetPin->SetText( EscapeString( m_comboName->GetValue(), CTX_NETNAME ) );

    if( m_fontCtrl->HaveFontSelection() )
    {
        m_sheetPin->SetFont( m_fontCtrl->GetFontSelection( m_bold->IsChecked(), m_italic->IsChecked() ) );
    }

    // Currently, eeschema uses only the text width as text size,
    // and expects text width = text height
    m_sheetPin->SetTextSize( VECTOR2I( m_textSize.GetIntValue(), m_textSize.GetIntValue() ) );

    // Must come after SetTextSize()
    m_sheetPin->SetBold( m_bold->IsChecked() );
    m_sheetPin->SetItalic( m_italic->IsChecked() );

    m_sheetPin->SetTextColor( m_textColorSwatch->GetSwatchColor() );

    if( m_input->GetValue() )              m_sheetPin->SetShape( LABEL_FLAG_SHAPE::L_INPUT );
    else if( m_output->GetValue() )        m_sheetPin->SetShape( LABEL_FLAG_SHAPE::L_OUTPUT );
    else if( m_bidirectional->GetValue() ) m_sheetPin->SetShape( LABEL_FLAG_SHAPE::L_BIDI );
    else if( m_triState->GetValue() )      m_sheetPin->SetShape( LABEL_FLAG_SHAPE::L_TRISTATE );
    else if( m_passive->GetValue() )       m_sheetPin->SetShape( LABEL_FLAG_SHAPE::L_UNSPECIFIED );

    if( !commit.Empty() )
        commit.Push( _( "Edit Sheet Pin Properties" ) );

    return true;
}


void DIALOG_SHEET_PIN_PROPERTIES::onOKButton( wxCommandEvent& event )
{
    event.Skip();
}


void DIALOG_SHEET_PIN_PROPERTIES::OnSyntaxHelp( wxHyperlinkEvent& aEvent )
{
    m_helpWindow = SCH_LABEL_BASE::ShowSyntaxHelp( this );
}


void DIALOG_SHEET_PIN_PROPERTIES::onComboBox( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen = m_sheetPin->GetParent()->GetScreen();

    for( SCH_ITEM* item : screen->Items().OfType( SCH_HIER_LABEL_T ) )
    {
        SCH_HIERLABEL* hierLabelItem = static_cast<SCH_HIERLABEL*>( item );

        if( m_comboName->GetValue().CmpNoCase( hierLabelItem->GetText() ) == 0 )
        {
            switch( hierLabelItem->GetShape() )
            {
            case LABEL_FLAG_SHAPE::L_INPUT:       m_input->SetValue( true );         break;
            case LABEL_FLAG_SHAPE::L_OUTPUT:      m_output->SetValue( true );        break;
            case LABEL_FLAG_SHAPE::L_BIDI:        m_bidirectional->SetValue( true ); break;
            case LABEL_FLAG_SHAPE::L_TRISTATE:    m_triState->SetValue( true );      break;
            case LABEL_FLAG_SHAPE::L_UNSPECIFIED: m_passive->SetValue( true );       break;
            default: wxFAIL_MSG( wxT( "Unknown sheet pin shape" ) );                 break;
            }

            break;
        }
    }
}
