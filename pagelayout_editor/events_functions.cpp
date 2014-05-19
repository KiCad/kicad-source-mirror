/**
 * @file pagelayout_editor/events_functions.cpp
 * @brief page layout editor command event functions.
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
#include <worksheet_shape_builder.h>
#include <class_worksheet_dataitem.h>
#include <design_tree_frame.h>
#include <properties_frame.h>
#include <dialog_page_settings.h>
#include <invoke_pl_editor_dialog.h>


BEGIN_EVENT_TABLE( PL_EDITOR_FRAME, EDA_DRAW_FRAME )
    EVT_CLOSE( PL_EDITOR_FRAME::OnCloseWindow )

    // Desing tree event:
    EVT_TREE_SEL_CHANGED( ID_DESIGN_TREE_FRAME, PL_EDITOR_FRAME::OnTreeSelection )
    EVT_TREE_ITEM_MIDDLE_CLICK( ID_DESIGN_TREE_FRAME, PL_EDITOR_FRAME::OnTreeMiddleClick )
    EVT_TREE_ITEM_RIGHT_CLICK( ID_DESIGN_TREE_FRAME, PL_EDITOR_FRAME::OnTreeRightClick )

    // Menu Files:
    EVT_MENU( wxID_NEW, PL_EDITOR_FRAME::Files_io )
    EVT_MENU( wxID_OPEN, PL_EDITOR_FRAME::Files_io )
    EVT_MENU( wxID_SAVE, PL_EDITOR_FRAME::Files_io )
    EVT_MENU( wxID_SAVEAS, PL_EDITOR_FRAME::Files_io )
    EVT_MENU( wxID_FILE, PL_EDITOR_FRAME::Files_io )
    EVT_MENU( ID_LOAD_DEFAULT_PAGE_LAYOUT, PL_EDITOR_FRAME::Files_io )
    EVT_MENU( ID_APPEND_DESCR_FILE, PL_EDITOR_FRAME::Files_io )

    EVT_MENU( ID_GEN_PLOT, PL_EDITOR_FRAME::ToPlotter )

    EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, PL_EDITOR_FRAME::OnFileHistory )

    EVT_MENU( wxID_EXIT, PL_EDITOR_FRAME::OnQuit )

    // menu Preferences
    EVT_MENU_RANGE( ID_PREFERENCES_HOTKEY_START, ID_PREFERENCES_HOTKEY_END, PL_EDITOR_FRAME::Process_Config )
    EVT_MENU( ID_MENU_PL_EDITOR_SELECT_PREFERED_EDITOR, EDA_BASE_FRAME::OnSelectPreferredEditor )
    EVT_MENU( wxID_PREFERENCES, PL_EDITOR_FRAME::Process_Config )
    EVT_MENU( ID_MENU_SWITCH_BGCOLOR, PL_EDITOR_FRAME::Process_Config )
    EVT_MENU( ID_MENU_GRID_ONOFF, PL_EDITOR_FRAME::Process_Config )

    // Menu Help
    EVT_MENU( wxID_HELP, EDA_DRAW_FRAME::GetKicadHelp )
    EVT_MENU( wxID_ABOUT, EDA_DRAW_FRAME::GetKicadAbout )

    EVT_TOOL( wxID_CUT, PL_EDITOR_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_UNDO, PL_EDITOR_FRAME::GetLayoutFromUndoList )
    EVT_TOOL( wxID_REDO, PL_EDITOR_FRAME::GetLayoutFromRedoList )
    EVT_TOOL( wxID_PRINT, PL_EDITOR_FRAME::ToPrinter )
    EVT_TOOL( wxID_PREVIEW, PL_EDITOR_FRAME::ToPrinter )
    EVT_TOOL( ID_SHEET_SET, PL_EDITOR_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_SHOW_REAL_MODE, PL_EDITOR_FRAME::OnSelectTitleBlockDisplayMode )
    EVT_TOOL( ID_SHOW_PL_EDITOR_MODE, PL_EDITOR_FRAME::OnSelectTitleBlockDisplayMode )
    EVT_CHOICE( ID_SELECT_COORDINATE_ORIGIN, PL_EDITOR_FRAME::OnSelectCoordOriginCorner)
    EVT_CHOICE( ID_SELECT_PAGE_NUMBER, PL_EDITOR_FRAME::Process_Special_Functions)

    // Vertical toolbar:
    EVT_TOOL( ID_NO_TOOL_SELECTED, PL_EDITOR_FRAME::Process_Special_Functions )

    EVT_MENU_RANGE( ID_POPUP_START_RANGE, ID_POPUP_END_RANGE,
                    PL_EDITOR_FRAME::Process_Special_Functions )

    EVT_UPDATE_UI( ID_SHOW_REAL_MODE, PL_EDITOR_FRAME::OnUpdateTitleBlockDisplayNormalMode )
    EVT_UPDATE_UI( ID_SHOW_PL_EDITOR_MODE, PL_EDITOR_FRAME::OnUpdateTitleBlockDisplaySpecialMode )

END_EVENT_TABLE()


/* Handles the selection of tools, menu, and popup menu commands.
 */
void PL_EDITOR_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    int id = event.GetId();
    int idx;
    wxString msg;
    WORKSHEET_LAYOUT& pglayout = WORKSHEET_LAYOUT::GetTheInstance();
    WORKSHEET_DATAITEM* item = NULL;
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    switch( id )
    {
    case ID_NO_TOOL_SELECTED:
        SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );

    case ID_SELECT_PAGE_NUMBER:
        m_canvas->Refresh();
        break;

    case ID_SHEET_SET:
        {
        DIALOG_PAGES_SETTINGS dlg( this );
        dlg.SetWksFileName( GetCurrFileName() );
        dlg.EnableWksFileNamePicker( false );
        dlg.ShowModal();

        cmd.SetId( ID_ZOOM_PAGE );
        wxPostEvent( this, cmd );
        }
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        break;

    case ID_POPUP_DESIGN_TREE_ITEM_DELETE:
    case ID_POPUP_ITEM_DELETE:
    case wxID_CUT:
        // Delete item, and select the previous item
        item = m_treePagelayout->GetPageLayoutSelectedItem();

        if( item == NULL )
            break;

        SaveCopyInUndoList();
        idx = pglayout.GetItemIndex( item );
        pglayout.Remove( item );
        RebuildDesignTree();

        if( id == ID_POPUP_DESIGN_TREE_ITEM_DELETE )
        {
            item = pglayout.GetItem( (unsigned) (idx-1) );

            if( item )
                m_treePagelayout->SelectCell( item );
        }

        item = NULL;
        OnModify();
        m_canvas->Refresh();
        break;

    case ID_POPUP_ITEM_ADD_LINE:
        SaveCopyInUndoList();
        idx =  m_treePagelayout->GetSelectedItemIndex();
        item = AddPageLayoutItem( WORKSHEET_DATAITEM::WS_SEGMENT, idx );
        if( InvokeDialogNewItem( this, item ) == wxID_CANCEL )
        {
            RemoveLastCommandInUndoList();
            pglayout.Remove( item );
            RebuildDesignTree();
            item = NULL;
        }
        else
        {
            // Put the new item in move mode, after putting the cursor
            // on the start point:
            wxPoint position = item->GetStartPosUi();
            SetCrossHairPosition( position, false );
            position = GetCrossHairPosition();

            if( m_canvas->IsPointOnDisplay( position ) )
                m_canvas->MoveCursorToCrossHair();
            else
                RedrawScreen( position, true );

            item->SetFlags( NEW_ITEM );
            MoveItem( item );
        }
        break;

    case ID_POPUP_ITEM_ADD_RECT:
        SaveCopyInUndoList();
        idx =  m_treePagelayout->GetSelectedItemIndex();
        item = AddPageLayoutItem( WORKSHEET_DATAITEM::WS_RECT, idx );
        if( InvokeDialogNewItem( this, item ) == wxID_CANCEL )
        {
            RemoveLastCommandInUndoList();
            pglayout.Remove( item );
            RebuildDesignTree();
            item = NULL;
        }
        else
        {
            // Put the new item in move mode, after putting the cursor
            // on the start point:
            wxPoint position = item->GetStartPosUi();
            SetCrossHairPosition( position, false );
            position = GetCrossHairPosition();

            if( m_canvas->IsPointOnDisplay( position ) )
                m_canvas->MoveCursorToCrossHair();
            else
                RedrawScreen( position, true );

            item->SetFlags( NEW_ITEM );
            MoveItem( item );
        }
        break;

    case ID_POPUP_ITEM_ADD_TEXT:
        SaveCopyInUndoList();
        idx =  m_treePagelayout->GetSelectedItemIndex();
        item = AddPageLayoutItem( WORKSHEET_DATAITEM::WS_TEXT, idx );
        if( InvokeDialogNewItem( this, item ) == wxID_CANCEL )
        {
            RemoveLastCommandInUndoList();
            pglayout.Remove( item );
            RebuildDesignTree();
            item = NULL;
        }
        else
        {
            // Put the new text in move mode:
            item->SetFlags( NEW_ITEM | LOCATE_STARTPOINT );
            MoveItem( item );
        }
        break;

    case ID_POPUP_ITEM_ADD_BITMAP:
        SaveCopyInUndoList();
        idx =  m_treePagelayout->GetSelectedItemIndex();
        item = AddPageLayoutItem( WORKSHEET_DATAITEM::WS_BITMAP, idx );
        if( item && InvokeDialogNewItem( this, item ) == wxID_CANCEL )
        {
            RemoveLastCommandInUndoList();
            pglayout.Remove( item );
            RebuildDesignTree();
            item = NULL;
        }
        if( item )
        {
            // Put the new text in move mode:
            item->SetFlags( NEW_ITEM | LOCATE_STARTPOINT );
            MoveItem( item );
        }
        break;

    case ID_POPUP_ITEM_APPEND_PAGE_LAYOUT:
        cmd.SetId( ID_APPEND_DESCR_FILE );
        wxPostEvent( this, cmd );
        break;

    case ID_POPUP_ITEM_PLACE:
        item = GetScreen()->GetCurItem();
        PlaceItem( item );
        break;

    case ID_POPUP_ITEM_PLACE_CANCEL:
        if(  m_canvas->IsMouseCaptured() )
             m_canvas->EndMouseCapture();
        break;

    case ID_POPUP_ITEM_MOVE_START_POINT:
        item = m_treePagelayout->GetPageLayoutSelectedItem();
        // Ensure flags are properly set
        item->ClearFlags( LOCATE_ENDPOINT );
        item->SetFlags( LOCATE_STARTPOINT );
        MoveItem( item );
        break;

    case ID_POPUP_ITEM_MOVE_END_POINT:
        item = m_treePagelayout->GetPageLayoutSelectedItem();
        // Ensure flags are properly set
        item->ClearFlags( LOCATE_STARTPOINT );
        item->SetFlags( LOCATE_ENDPOINT );
        MoveItem( item );
        break;

    case ID_POPUP_ITEM_MOVE:
        item = m_treePagelayout->GetPageLayoutSelectedItem();
        item->ClearFlags( LOCATE_ENDPOINT|LOCATE_STARTPOINT );
        MoveItem( item );
        break;

    default:
        wxMessageBox( wxT( "PL_EDITOR_FRAME::Process_Special_Functions error" ) );
        break;
    }

    if( item )
    {
        OnModify();
        m_propertiesPagelayout->CopyPrmsFromItemToPanel( item );
        m_treePagelayout->SelectCell( item );
    }

}


/*
 * Function moveItem: called when the mouse cursor is moving
 * moves the item currently selected (or the start point or the end point)
 * to the cursor position
 */
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
    if( aPanel && ( previous_position != position ) )
        aPanel->Refresh();
}

/*
 * Function abortMoveItem: called when an item is currently moving,
 * and when the user aborts the move command.
 * Restores the initial position of the item
 */
static void abortMoveItem( EDA_DRAW_PANEL* aPanel, wxDC* aDC )
{
    PL_EDITOR_SCREEN* screen = (PL_EDITOR_SCREEN*) aPanel->GetScreen();
    WORKSHEET_DATAITEM *item = screen->GetCurItem();

    if( (item->GetFlags() & NEW_ITEM ) )
    {
        PL_EDITOR_FRAME* plframe = (PL_EDITOR_FRAME*) aPanel->GetParent();
        plframe->RemoveLastCommandInUndoList();
        WORKSHEET_LAYOUT& pglayout = WORKSHEET_LAYOUT::GetTheInstance();
        pglayout.Remove( item );
        plframe->RebuildDesignTree();
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


/**
* Save in Undo list the layout, and place an item being moved.
* @param aItem is the item moved
*/
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
    else if( (aItem->GetFlags() & LOCATE_ENDPOINT) )
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
    wxMessageBox( wxT("Not yet available"));
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

void PL_EDITOR_FRAME::OnTreeSelection( wxTreeEvent& event )
{
    WORKSHEET_DATAITEM* item = GetSelectedItem();

    if( item )
        m_propertiesPagelayout->CopyPrmsFromItemToPanel( item );

    m_canvas->Refresh();
}

void PL_EDITOR_FRAME::OnTreeMiddleClick( wxTreeEvent& event )
{
}

extern void AddNewItemsCommand( wxMenu* aMainMenu );
void PL_EDITOR_FRAME::OnTreeRightClick( wxTreeEvent& event )
{
    m_treePagelayout->SelectCell( event.GetItem() );

    wxMenu popMenu;
    AddNewItemsCommand( &popMenu );

    popMenu.AppendSeparator();
    AddMenuItem( &popMenu, ID_POPUP_DESIGN_TREE_ITEM_DELETE, _( "Delete" ),
                 KiBitmap( delete_xpm ) );

    PopupMenu( &popMenu );
}


void PL_EDITOR_FRAME::OnUpdateTitleBlockDisplayNormalMode( wxUpdateUIEvent& event )
{
    event.Check( WORKSHEET_DATAITEM::m_SpecialMode == false );
}

void PL_EDITOR_FRAME::OnUpdateTitleBlockDisplaySpecialMode( wxUpdateUIEvent& event )
{
    event.Check( WORKSHEET_DATAITEM::m_SpecialMode == true );
}
