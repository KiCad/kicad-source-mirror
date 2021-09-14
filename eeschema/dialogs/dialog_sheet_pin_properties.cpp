/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2018-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_validators.h>
#include <dialog_sheet_pin_properties.h>
#include <dialogs/html_message_box.h>
#include <string_utils.h>


static wxString sheetPinTypes[] =
{
    _( "Input" ),
    _( "Output" ),
    _( "Bidirectional" ),
    _( "Tri-state" ),
    _( "Passive" )
};


DIALOG_SHEET_PIN_PROPERTIES::DIALOG_SHEET_PIN_PROPERTIES( SCH_EDIT_FRAME* parent,
                                                          SCH_SHEET_PIN* aPin ) :
        DIALOG_SHEET_PIN_PROPERTIES_BASE( parent ),
        m_frame( parent ),
        m_sheetPin( aPin ),
        m_textSize( parent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits ),
        m_helpWindow( nullptr )
{
    for( const wxString& sheetPinType : sheetPinTypes )
        m_choiceConnectionType->Append( sheetPinType );

    m_choiceConnectionType->SetSelection( 0 );
    SetInitialFocus( m_comboName );
    m_sdbSizerOK->SetDefault();

    // Set invalid label characters list:
    SCH_NETNAME_VALIDATOR validator( true );
    m_comboName->SetValidator( validator );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();

    /* This ugly hack fixes a bug in wxWidgets 2.8.7 and likely earlier versions for
     * the flex grid sizer in wxGTK that prevents the last column from being sized
     * correctly.  It doesn't cause any problems on win32 so it doesn't need to wrapped
     * in ugly #ifdef __WXGTK__ #endif.
     */
    Layout();
    Fit();
    SetMinSize( GetSize() );

    // On some windows manager (Unity, XFCE), this dialog is
    // not always raised, depending on this dialog is run.
    // Force it to be raised
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
    // Currently, eeschema uses only the text width as text size
    // (only the text width is saved in files), and expects text width = text height
    m_textSize.SetValue( m_sheetPin->GetTextWidth() );
    m_choiceConnectionType->SetSelection( static_cast<int>( m_sheetPin->GetShape() ) );

    return true;
}


bool DIALOG_SHEET_PIN_PROPERTIES::TransferDataFromWindow()
{
    if( !m_sheetPin->IsNew() )
    {
        SCH_SHEET* parentSheet = m_sheetPin->GetParent();
        m_frame->SaveCopyInUndoList( m_frame->GetScreen(), parentSheet, UNDO_REDO::CHANGED, false );
    }

    m_sheetPin->SetText( EscapeString( m_comboName->GetValue(), CTX_NETNAME ) );
    // Currently, eeschema uses only the text width as text size,
    // and expects text width = text height
    m_sheetPin->SetTextSize( wxSize( m_textSize.GetValue(), m_textSize.GetValue() ) );

    auto shape = static_cast<PINSHEETLABEL_SHAPE>( m_choiceConnectionType->GetCurrentSelection() );
    m_sheetPin->SetShape( shape );

    m_frame->UpdateItem( m_sheetPin );
    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();

    return true;
}


void DIALOG_SHEET_PIN_PROPERTIES::onOKButton( wxCommandEvent& event )
{
    event.Skip();
}


void DIALOG_SHEET_PIN_PROPERTIES::OnSyntaxHelp( wxHyperlinkEvent& aEvent )
{
    m_helpWindow = SCH_TEXT::ShowSyntaxHelp( this );
}


void DIALOG_SHEET_PIN_PROPERTIES::onComboBox( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen = m_sheetPin->GetParent()->GetScreen();

    for( SCH_ITEM* item : screen->Items().OfType( SCH_HIER_LABEL_T ) )
    {
        auto hierLabelItem = static_cast<SCH_HIERLABEL*>( item );

        if( m_comboName->GetValue().CmpNoCase( hierLabelItem->GetText() ) == 0 )
        {
            m_choiceConnectionType->SetSelection( static_cast<int>( hierLabelItem->GetShape() ) );
            break;
        }
    }
}
