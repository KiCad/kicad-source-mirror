/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2018-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <sch_validators.h>
#include <dialog_edit_sheet_pin.h>


static wxString sheetPinTypes[] =
{
    _( "Input" ),
    _( "Output" ),
    _( "Bidirectional" ),
    _( "Tri-state" ),
    _( "Passive" )
};


DIALOG_EDIT_SHEET_PIN::DIALOG_EDIT_SHEET_PIN( SCH_EDIT_FRAME* parent, SCH_SHEET_PIN* aPin ) :
    DIALOG_EDIT_SHEET_PIN_BASE( parent ),
    m_frame( parent ),
    m_sheetPin( aPin ),
    m_textWidth( parent, m_widthLabel, m_widthCtrl, m_widthUnits, true ),
    m_textHeight( parent, m_heightLabel, m_heightCtrl, m_heightUnits, true )
{
    for( const wxString& sheetPinType : sheetPinTypes )
        m_choiceConnectionType->Append( sheetPinType );

    m_choiceConnectionType->SetSelection( 0 );
    m_textName->SetFocus();
    m_sdbSizerOK->SetDefault();

    // Set invalid label characters list:
    SCH_NETNAME_VALIDATOR validator;
    m_textName->SetValidator( validator );

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();

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


bool DIALOG_EDIT_SHEET_PIN::TransferDataToWindow()
{
    m_textName->SetValue( UnescapeString( m_sheetPin->GetText() ) );
    m_textName->SelectAll();
    m_textWidth.SetValue( m_sheetPin->GetTextWidth() );
    m_textHeight.SetValue( m_sheetPin->GetTextHeight() );
    m_choiceConnectionType->SetSelection( m_sheetPin->GetShape() );

    return true;
}


bool DIALOG_EDIT_SHEET_PIN::TransferDataFromWindow()
{
    if( !m_sheetPin->IsNew() )
        m_frame->SaveCopyInUndoList( (SCH_ITEM*) m_sheetPin->GetParent(), UR_CHANGED );

    m_sheetPin->SetText( EscapeString( m_textName->GetValue(), CTX_NETNAME ) );
    m_sheetPin->SetTextSize( wxSize( m_textWidth.GetValue(), m_textHeight.GetValue() ) );

    auto shape = static_cast<PINSHEETLABEL_SHAPE>( m_choiceConnectionType->GetCurrentSelection() );
    m_sheetPin->SetShape( shape );

    m_frame->RefreshItem( m_sheetPin );
    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();

    return true;
}


void DIALOG_EDIT_SHEET_PIN::onOKButton( wxCommandEvent& event )
{
    event.Skip();
}
