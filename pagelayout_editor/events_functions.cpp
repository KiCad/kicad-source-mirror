/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file pagelayout_editor/events_functions.cpp
 * @brief page layout editor command event functions.
 */

#include <fctsys.h>
#include <wx/treectrl.h>
#include <pgm_base.h>
#include <class_drawpanel.h>
#include <common.h>
#include <macros.h>

#include <pl_editor_frame.h>
#include <kicad_device_context.h>
#include <pl_editor_id.h>
#include <dialog_helpers.h>
#include <menus_helpers.h>
#include <ws_draw_item.h>
#include <worksheet_dataitem.h>
#include <dialog_page_settings.h>
#include <invoke_pl_editor_dialog.h>
#include <properties_frame.h>


BEGIN_EVENT_TABLE( PL_EDITOR_FRAME, EDA_DRAW_FRAME )
    EVT_CLOSE( PL_EDITOR_FRAME::OnCloseWindow )
    // Menu Files:
    EVT_MENU( wxID_NEW, PL_EDITOR_FRAME::Files_io )
    EVT_MENU( wxID_OPEN, PL_EDITOR_FRAME::Files_io )
    EVT_MENU( wxID_SAVE, PL_EDITOR_FRAME::Files_io )
    EVT_MENU( wxID_SAVEAS, PL_EDITOR_FRAME::Files_io )
    EVT_MENU( wxID_FILE, PL_EDITOR_FRAME::Files_io )
    EVT_MENU( ID_APPEND_DESCR_FILE, PL_EDITOR_FRAME::Files_io )

    EVT_MENU( ID_GEN_PLOT, PL_EDITOR_FRAME::ToPlotter )

    EVT_MENU_RANGE( ID_FILE1, ID_FILEMAX, PL_EDITOR_FRAME::OnFileHistory )

    EVT_MENU( wxID_EXIT, PL_EDITOR_FRAME::OnQuit )

    // menu Preferences
    EVT_MENU( ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST, PL_EDITOR_FRAME::Process_Config )
    EVT_MENU( wxID_PREFERENCES, PL_EDITOR_FRAME::Process_Config )

    EVT_TOOL( wxID_DELETE, PL_EDITOR_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_PRINT, PL_EDITOR_FRAME::ToPrinter )
    EVT_TOOL( wxID_PREVIEW, PL_EDITOR_FRAME::ToPrinter )
    EVT_TOOL( ID_SHEET_SET, PL_EDITOR_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_SHOW_REAL_MODE, PL_EDITOR_FRAME::OnSelectTitleBlockDisplayMode )
    EVT_TOOL( ID_SHOW_PL_EDITOR_MODE, PL_EDITOR_FRAME::OnSelectTitleBlockDisplayMode )
    EVT_CHOICE( ID_SELECT_COORDINATE_ORIGIN, PL_EDITOR_FRAME::OnSelectCoordOriginCorner)
    EVT_CHOICE( ID_SELECT_PAGE_NUMBER, PL_EDITOR_FRAME::Process_Special_Functions)

    EVT_MENU_RANGE( ID_POPUP_START_RANGE, ID_POPUP_END_RANGE,
                    PL_EDITOR_FRAME::Process_Special_Functions )

    EVT_UPDATE_UI( ID_SHOW_REAL_MODE, PL_EDITOR_FRAME::OnUpdateTitleBlockDisplayNormalMode )
    EVT_UPDATE_UI( ID_SHOW_PL_EDITOR_MODE, PL_EDITOR_FRAME::OnUpdateTitleBlockDisplaySpecialMode )
END_EVENT_TABLE()


/* Handles the selection of tools, menu, and popup menu commands.
 */
void PL_EDITOR_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    switch( event.GetId() )
    {
    case ID_SELECT_PAGE_NUMBER:
        m_canvas->Refresh();
        break;

    case ID_SHEET_SET:
        {
        DIALOG_PAGES_SETTINGS dlg( this, wxSize( MAX_PAGE_SIZE_EDITORS_MILS,
                                                 MAX_PAGE_SIZE_EDITORS_MILS ) );
        dlg.SetWksFileName( GetCurrFileName() );
        dlg.EnableWksFileNamePicker( false );
        dlg.ShowModal();

        cmd.SetId( ID_ZOOM_PAGE );
        wxPostEvent( this, cmd );
        }
        break;

    case ID_POPUP_ITEM_APPEND_PAGE_LAYOUT:
        cmd.SetId( ID_APPEND_DESCR_FILE );
        wxPostEvent( this, cmd );
        break;

    default:
        wxMessageBox( wxT( "PL_EDITOR_FRAME::Process_Special_Functions error" ) );
        break;
    }
}


/*
 * Function moveItem: called when the mouse cursor is moving
 * moves the item currently selected (or the start point or the end point)
 * to the cursor position
DPOINT initialPosition;         // The initial position of the item to move, in mm
wxPoint initialPositionUi;      // The initial position of the item to move, in Ui
wxPoint initialCursorPosition;  // The initial position of the cursor

static void moveItem( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition, bool aErase )
{
    PL_EDITOR_SCREEN* screen = (PL_EDITOR_SCREEN*) aPanel->GetScreen();
    WORKSHEET_DATAITEM *item = screen->GetCurItem();

    wxCHECK_RET( (item != NULL), wxT( "Cannot move NULL item." ) );
    wxPoint position = aPanel->GetParent()->GetCrossHairPosition()
                      - ( initialCursorPosition - initialPositionUi );
    wxPoint previous_position;

    if( (item->GetFlags() & LOCATE_STARTPOINT) )
    {
        previous_position = item->GetStartPosUi();
        item->MoveStartPointToUi( position );
    }
    else if( (item->GetFlags() & LOCATE_ENDPOINT) )
    {
        previous_position = item->GetEndPosUi();
        item->MoveEndPointToUi( position );
    }
    else
    {
        previous_position = item->GetStartPosUi();
        item->MoveToUi( position );
    }

    // Draw the item item at it's new position, if it is modified,
    // (does not happen each time the mouse is moved, because the
    // item is placed on grid)
    // to avoid useless computation time.
    if(  previous_position != position )
        aPanel->Refresh();
}


 * Function abortMoveItem: called when an item is currently moving,
 * and when the user aborts the move command.
 * Restores the initial position of the item
static void abortMoveItem( EDA_DRAW_PANEL* aPanel, wxDC* aDC )
{
    PL_EDITOR_SCREEN* screen = (PL_EDITOR_SCREEN*) aPanel->GetScreen();
    WORKSHEET_DATAITEM *item = screen->GetCurItem();

    if( item->GetFlags() & NEW_ITEM )
    {
        PL_EDITOR_FRAME* plframe = (PL_EDITOR_FRAME*) aPanel->GetParent();
        plframe->RemoveLastCommandInUndoList();
        WORKSHEET_LAYOUT& pglayout = WORKSHEET_LAYOUT::GetTheInstance();
        pglayout.Remove( item );
    }
    else
    {
        if( (item->GetFlags() & LOCATE_STARTPOINT) )
        {
            item->MoveStartPointTo( initialPosition );
        }
        else if( (item->GetFlags() & LOCATE_ENDPOINT) )
        {
            item->MoveEndPointTo( initialPosition );
        }
        else
            item->MoveTo( initialPosition );
    }

    aPanel->SetMouseCapture( NULL, NULL );
    screen->SetCurItem( NULL );
    aPanel->Refresh();
}


void PL_EDITOR_FRAME::MoveItem( WORKSHEET_DATAITEM* aItem )
{
    wxCHECK_RET( aItem != NULL, wxT( "Cannot move NULL item" ) );
    initialPosition = aItem->GetStartPos();
    initialPositionUi = aItem->GetStartPosUi();
    initialCursorPosition = GetCrossHairPosition();

    if( (aItem->GetFlags() & LOCATE_ENDPOINT) )
    {
        initialPosition = aItem->GetEndPos();
        initialPositionUi = aItem->GetEndPosUi();
    }

    if( aItem->GetFlags() & (LOCATE_STARTPOINT|LOCATE_ENDPOINT) )
    {
        SetCrossHairPosition( initialPositionUi, false );
        initialCursorPosition = GetCrossHairPosition();

        if( m_canvas->IsPointOnDisplay( initialCursorPosition ) )
        {
            m_canvas->MoveCursorToCrossHair();
            m_canvas->Refresh();
        }
        else
        {
            RedrawScreen( initialCursorPosition, true );
        }
    }

    m_canvas->SetMouseCapture( moveItem, abortMoveItem );
    GetScreen()->SetCurItem( aItem );
}


* Save in Undo list the layout, and place an item being moved.
* @param aItem is the item moved
void PL_EDITOR_FRAME::PlaceItem( WORKSHEET_DATAITEM* aItem )
{
    DPOINT currStartPos = aItem->GetStartPos();
    DPOINT currEndPos = aItem->GetEndPos();

    aItem->ClearFlags( NEW_ITEM );

    // Save the curren layout before changes
    if( (aItem->GetFlags() & LOCATE_STARTPOINT) )
    {
        aItem->MoveStartPointTo( initialPosition );
    }
    else if( aItem->GetFlags() & LOCATE_ENDPOINT )
    {
        aItem->MoveEndPointTo( initialPosition );
    }
    else
        aItem->MoveTo( initialPosition );

    SaveCopyInUndoList();

    // Re-place the item
    aItem->MoveStartPointTo( currStartPos );
    aItem->MoveEndPointTo( currEndPos );

    m_canvas->SetMouseCapture( NULL, NULL );
    GetScreen()->SetCurItem( NULL );
}
*/


/* called when the user select one of the 4 page corner as corner
 * reference (or the left top paper corner)
 */
void PL_EDITOR_FRAME::OnSelectCoordOriginCorner( wxCommandEvent& event )
{
    m_originSelectChoice = m_originSelectBox->GetSelection();
    UpdateStatusBar();  // Update grid origin
    m_canvas->Refresh();
}


void PL_EDITOR_FRAME::OnSelectTitleBlockDisplayMode( wxCommandEvent& event )
{
    WORKSHEET_DATAITEM::m_SpecialMode = (event.GetId() == ID_SHOW_PL_EDITOR_MODE);
    m_canvas->Refresh();
}


void PL_EDITOR_FRAME::OnQuit( wxCommandEvent& event )
{
    Close( true );
}


void PL_EDITOR_FRAME::ToPlotter(wxCommandEvent& event)
{
    wxMessageBox( wxT( "Not yet available" ) );
}


void PL_EDITOR_FRAME::ToPrinter(wxCommandEvent& event)
{
    // static print data and page setup data, to remember settings during the session
    static wxPrintData* s_PrintData;
    static wxPageSetupDialogData* s_pageSetupData = (wxPageSetupDialogData*) NULL;

    const PAGE_INFO& pageInfo = GetPageSettings();

    if( s_PrintData == NULL )  // First print
    {
        s_PrintData = new wxPrintData();
        s_PrintData->SetQuality( wxPRINT_QUALITY_HIGH );      // Default resolution = HIGH;
    }

    if( !s_PrintData->Ok() )
    {
        wxMessageBox( _( "Error Init Printer info" ) );
        return;
    }

    if( s_pageSetupData == NULL )
        s_pageSetupData = new wxPageSetupDialogData( *s_PrintData );

    s_pageSetupData->SetPaperId( pageInfo.GetPaperId() );
    s_pageSetupData->GetPrintData().SetOrientation( pageInfo.GetWxOrientation() );

    if( pageInfo.IsCustom() )
    {
        if( pageInfo.IsPortrait() )
            s_pageSetupData->SetPaperSize( wxSize( Mils2mm( pageInfo.GetWidthMils() ),
                                                   Mils2mm( pageInfo.GetHeightMils() ) ) );
        else
            s_pageSetupData->SetPaperSize( wxSize( Mils2mm( pageInfo.GetHeightMils() ),
                                                   Mils2mm( pageInfo.GetWidthMils() ) ) );
    }

    *s_PrintData = s_pageSetupData->GetPrintData();

    if( event.GetId() == wxID_PREVIEW )
        InvokeDialogPrintPreview( this, s_PrintData );
    else
        InvokeDialogPrint( this, s_PrintData, s_pageSetupData );
}


void PL_EDITOR_FRAME::OnUpdateTitleBlockDisplayNormalMode( wxUpdateUIEvent& event )
{
    event.Check( WORKSHEET_DATAITEM::m_SpecialMode == false );
}


void PL_EDITOR_FRAME::OnUpdateTitleBlockDisplaySpecialMode( wxUpdateUIEvent& event )
{
    event.Check( WORKSHEET_DATAITEM::m_SpecialMode == true );
}


