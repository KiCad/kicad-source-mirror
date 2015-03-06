/**
 * @file dialog_new_dataitem.cpp
 * @brief a dialog called on creating a new plage layout data item.
*/

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <fctsys.h>
#include <common.h>
#include <class_drawpanel.h>

#include <pl_editor_frame.h>
#include <class_worksheet_dataitem.h>
#include <dialog_new_dataitem_base.h>

class DIALOG_NEW_DATAITEM : public DIALOG_NEW_DATAITEM_BASE
{
    WORKSHEET_DATAITEM* m_item;

public:
    DIALOG_NEW_DATAITEM( PL_EDITOR_FRAME* aCaller, WORKSHEET_DATAITEM* aItem );

private:
    void OnCancelClick( wxCommandEvent& event );
    void OnOKClick( wxCommandEvent& event );

    void initDlg();
};


int InvokeDialogNewItem( PL_EDITOR_FRAME* aCaller, WORKSHEET_DATAITEM* aItem )
{
    DIALOG_NEW_DATAITEM dlg( aCaller, aItem );
    return dlg.ShowModal();
}

DIALOG_NEW_DATAITEM::DIALOG_NEW_DATAITEM( PL_EDITOR_FRAME* aCaller,
                                          WORKSHEET_DATAITEM* aItem )
    : DIALOG_NEW_DATAITEM_BASE( aCaller )
{
    m_item = aItem;
    initDlg();

    GetSizer()->SetSizeHints( this );
    Centre();
}

void DIALOG_NEW_DATAITEM::OnCancelClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL);
}

void DIALOG_NEW_DATAITEM::OnOKClick( wxCommandEvent& event )
{
    if( m_item->GetType() == WORKSHEET_DATAITEM::WS_TEXT )
    {
        WORKSHEET_DATAITEM_TEXT* text = ((WORKSHEET_DATAITEM_TEXT*)m_item);
        text->m_TextBase = m_textCtrlText->GetValue();
        // For multiline texts, replace the '\n' char by the "\\n" sequence",
        // in internal string
        text->m_TextBase.Replace( wxT("\n"), wxT("\\n") );
    }

    wxString msg;

    // Import Start point
    double dtmp;
    msg = m_textCtrlPosX->GetValue();
    msg.ToDouble( &dtmp );
    m_item->m_Pos.m_Pos.x = dtmp;

    msg = m_textCtrlPosY->GetValue();
    msg.ToDouble( &dtmp );
    m_item->m_Pos.m_Pos.y = dtmp;

    switch( m_choiceCornerPos->GetSelection() )
    {
        case 2: m_item->m_Pos.m_Anchor = RB_CORNER; break;
        case 0: m_item->m_Pos.m_Anchor = RT_CORNER; break;
        case 3: m_item->m_Pos.m_Anchor = LB_CORNER; break;
        case 1: m_item->m_Pos.m_Anchor = LT_CORNER; break;
    }

    // Import End point
    msg = m_textCtrlEndX->GetValue();
    msg.ToDouble( &dtmp );
    m_item->m_End.m_Pos.x = dtmp;

    msg = m_textCtrlEndY->GetValue();
    msg.ToDouble( &dtmp );
    m_item->m_End.m_Pos.y = dtmp;

    switch( m_choiceCornerEnd->GetSelection() )
    {
        case 2: m_item->m_End.m_Anchor = RB_CORNER; break;
        case 0: m_item->m_End.m_Anchor = RT_CORNER; break;
        case 3: m_item->m_End.m_Anchor = LB_CORNER; break;
        case 1: m_item->m_End.m_Anchor = LT_CORNER; break;
    }

    EndModal( wxID_OK);
}

void DIALOG_NEW_DATAITEM::initDlg()
{
    // Disable useless widgets, depending on WORKSHEET_DATAITEM type
    switch( m_item->GetType() )
    {
        case WORKSHEET_DATAITEM::WS_SEGMENT:
        case WORKSHEET_DATAITEM::WS_RECT:
            m_textCtrlText->Enable( false );
            break;

        case WORKSHEET_DATAITEM::WS_BITMAP:
        case WORKSHEET_DATAITEM::WS_POLYPOLYGON:
            m_textCtrlText->Enable( false );
            // fall through
        case WORKSHEET_DATAITEM::WS_TEXT:
            m_textCtrlEndX->Enable( false );
            m_textCtrlEndY->Enable( false );
            m_choiceCornerEnd->Enable( false );
            break;
    }

    wxString msg;

    // Position/ start point
    msg.Printf( wxT("%.3f"), m_item->m_Pos.m_Pos.x );
    m_textCtrlPosX->SetValue( msg );
    msg.Printf( wxT("%.3f"), m_item->m_Pos.m_Pos.y );
    m_textCtrlPosY->SetValue( msg );
    switch(  m_item->m_Pos.m_Anchor )
    {
        case RB_CORNER:      // right bottom corner
            m_choiceCornerPos->SetSelection( 2 ); break;
        case RT_CORNER:      // right top corner
            m_choiceCornerPos->SetSelection( 0 ); break;
        case LB_CORNER:      // left bottom corner
            m_choiceCornerPos->SetSelection( 3 ); break;
        case LT_CORNER:      // left top corner
            m_choiceCornerPos->SetSelection( 1 ); break;
    }

    // End point
    msg.Printf( wxT("%.3f"), m_item->m_End.m_Pos.x );
    m_textCtrlEndX->SetValue( msg );
    msg.Printf( wxT("%.3f"), m_item->m_End.m_Pos.y );
    m_textCtrlEndY->SetValue( msg );
    switch( m_item->m_End.m_Anchor )
    {
        case RB_CORNER:      // right bottom corner
            m_choiceCornerEnd->SetSelection( 2 ); break;
        case RT_CORNER:      // right top corner
            m_choiceCornerEnd->SetSelection( 0 ); break;
        case LB_CORNER:      // left bottom corner
            m_choiceCornerEnd->SetSelection( 3 ); break;
        case LT_CORNER:      // left top corner
            m_choiceCornerEnd->SetSelection( 1 ); break;
    }

    if( m_item->GetType() == WORKSHEET_DATAITEM::WS_TEXT )
        m_textCtrlText->SetValue( ((WORKSHEET_DATAITEM_TEXT*)m_item)->m_TextBase );
}
