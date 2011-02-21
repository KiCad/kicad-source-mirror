/********************************************************/
/* Routines generales de gestion des commandes usuelles */
/********************************************************/

/* controle.cpp */

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "protos.h"
#include "pcbnew_id.h"

#include "collectors.h"

//external funtions used here:
extern bool Magnetize( BOARD* m_Pcb, WinEDA_PcbFrame* frame,
                       int aCurrentTool, wxSize grid, wxPoint on_grid, wxPoint* curpos );


/**
 * Function AllAreModulesAndReturnSmallestIfSo
 * tests that all items in the collection are MODULEs and if so, returns the
 * smallest MODULE.
 * @return BOARD_ITEM* - The smallest or NULL.
 */
static BOARD_ITEM* AllAreModulesAndReturnSmallestIfSo( GENERAL_COLLECTOR* aCollector )
{
    int count = aCollector->GetCount();

    for( int i = 0; i<count;  ++i )
    {
        if( (*aCollector)[i]->Type() != TYPE_MODULE )
            return NULL;
    }

    // all are modules, now find smallest MODULE

    int minDim = 0x7FFFFFFF;
    int minNdx = 0;

    for( int i = 0;  i<count;  ++i )
    {
        MODULE* module = (MODULE*) (*aCollector)[i];

        int     lx = module->m_BoundaryBox.GetWidth();
        int     ly = module->m_BoundaryBox.GetHeight();

        int     lmin = MIN( lx, ly );

        if( lmin <= minDim )
        {
            minDim = lmin;
            minNdx = i;
        }
    }

    return (*aCollector)[minNdx];
}


BOARD_ITEM* WinEDA_BasePcbFrame::PcbGeneralLocateAndDisplay( int aHotKeyCode )
{
    BOARD_ITEM* item;

    GENERAL_COLLECTORS_GUIDE guide = GetCollectorsGuide();

    // Assign to scanList the proper item types desired based on tool type
    // or hotkey that is in play.

    const KICAD_T* scanList = NULL;

    if( aHotKeyCode )
    {
        // @todo: add switch here and add calls to PcbGeneralLocateAndDisplay( int aHotKeyCode )
        // when searching is needed from a hotkey handler
    }
    else if( m_ID_current_state == 0 )
    {
        switch( m_HTOOL_current_state )
        {
        case ID_TOOLBARH_PCB_MODE_MODULE:
            scanList = GENERAL_COLLECTOR::ModuleItems;
            break;

        default:
            scanList = DisplayOpt.DisplayZonesMode == 0 ?
                       GENERAL_COLLECTOR::AllBoardItems :
                       GENERAL_COLLECTOR::AllButZones;
            break;
        }
    }
    else
    {
        switch( m_ID_current_state )
        {
        case ID_PCB_SHOW_1_RATSNEST_BUTT:
            scanList = GENERAL_COLLECTOR::PadsOrModules;
            break;

        case ID_TRACK_BUTT:
            scanList = GENERAL_COLLECTOR::Tracks;
            break;

        case ID_PCB_MODULE_BUTT:
            scanList = GENERAL_COLLECTOR::ModuleItems;
            break;

        default:
            scanList = DisplayOpt.DisplayZonesMode == 0 ?
                       GENERAL_COLLECTOR::AllBoardItems :
                       GENERAL_COLLECTOR::AllButZones;
        }
    }

    m_Collector->Collect( m_Pcb, scanList, GetScreen()->RefPos( true ), guide );

#if 0
    // debugging: print out the collected items, showing their priority order too.
    for( int i = 0; i<m_Collector->GetCount();  ++i )
        (*m_Collector)[i]->Show( 0, std::cout );
#endif

    /* Remove redundancies: sometime, zones are found twice,
     * because zones can be filled by overlapping segments (this is a fill option)
     */
    unsigned long timestampzone = 0;

    for( int ii = 0;  ii < m_Collector->GetCount(); ii++ )
    {
        item = (*m_Collector)[ii];

        if( item->Type() != TYPE_ZONE )
            continue;

        /* Found a TYPE ZONE */
        if( item->m_TimeStamp == timestampzone )    // Remove it, redundant, zone already found
        {
            m_Collector->Remove( ii );
            ii--;
        }
        else
            timestampzone = item->m_TimeStamp;
    }

    if( m_Collector->GetCount() <= 1 )
    {
        item = (*m_Collector)[0];
        SetCurItem( item );
    }

    // If the count is 2, and first item is a pad or moduletext, and the 2nd item is its
    // parent module:
    else if( m_Collector->GetCount() == 2
             && ( (*m_Collector)[0]->Type() == TYPE_PAD || (*m_Collector)[0]->Type() ==
                 TYPE_TEXTE_MODULE )
             && (*m_Collector)[1]->Type() == TYPE_MODULE && (*m_Collector)[0]->GetParent()==
             (*m_Collector)[1] )
    {
        item = (*m_Collector)[0];
        SetCurItem( item );
    }
    // if all are modules, find the smallest one amoung the primary choices
    else if( ( item = AllAreModulesAndReturnSmallestIfSo( m_Collector ) ) != NULL )
    {
        SetCurItem( item );
    }

    else    // we can't figure out which item user wants, do popup menu so user can choose
    {
        wxMenu itemMenu;

        /* Give a title to the selection menu. This is also a cancel menu item */
        wxMenuItem * item_title = new wxMenuItem(&itemMenu, -1, _( "Selection Clarification" ) );

#ifdef __WINDOWS__
        wxFont bold_font(*wxNORMAL_FONT);
        bold_font.SetWeight(wxFONTWEIGHT_BOLD);
        bold_font.SetStyle( wxFONTSTYLE_ITALIC);
        item_title->SetFont(bold_font);
#endif

        itemMenu.Append(item_title);
        itemMenu.AppendSeparator();

        int limit = MIN( MAX_ITEMS_IN_PICKER, m_Collector->GetCount() );

        for( int i = 0;  i<limit;  ++i )
        {
            wxString     text;
            const char** xpm;

            item = (*m_Collector)[i];

            text = item->MenuText( m_Pcb );
            xpm  = item->MenuIcon();

            ADD_MENUITEM( &itemMenu, ID_POPUP_PCB_ITEM_SELECTION_START + i, text, xpm );
        }

        /* @todo: rather than assignment to true, these should be increment and decrement
         * operators throughout _everywhere_.
         *  That way we can handle nesting.
         *  But I tried that and found there cases where the assignment to true (converted to
         * a m_IgnoreMouseEvents++ )
         *  was not balanced with the -- (now m_IgnoreMouseEvents=false), so I had to revert.
         *  Somebody should track down these and make them balanced.
         *  DrawPanel->m_IgnoreMouseEvents = true;
         */

        // this menu's handler is void WinEDA_BasePcbFrame::ProcessItemSelection()
        // and it calls SetCurItem() which in turn calls DisplayInfo() on the item.
        DrawPanel->m_AbortRequest = true;   // changed in false if an item
        PopupMenu( &itemMenu ); // m_AbortRequest = false if an item is selected

        DrawPanel->MoveCursorToCrossHair();

//        DrawPanel->m_IgnoreMouseEvents = false;

        // The function ProcessItemSelection() has set the current item, return it.
        item = GetCurItem();
    }

    return item;
}


void WinEDA_PcbFrame::GeneralControle( wxDC* aDC, const wxPoint& aPosition )
{
    wxRealPoint gridSize;
    wxPoint     oldpos;
    int         hotkey = 0;
    wxPoint     pos = GetScreen()->GetNearestGridPosition( aPosition );

    // Save the board after the time out :
    int CurrentTime = time( NULL );

    if( !GetScreen()->IsModify() || GetScreen()->IsSave() )
    {
        /* If no change, reset the time out */
        g_SaveTime = CurrentTime;
    }

    if( (CurrentTime - g_SaveTime) > g_TimeOut )
    {
        wxString tmpFileName = GetScreen()->GetFileName();
        wxFileName fn = wxFileName( wxEmptyString, g_SaveFileName, PcbFileExtension );
        bool flgmodify = GetScreen()->IsModify();

        SavePcbFile( fn.GetFullPath() );

        if( flgmodify ) // Set the flags m_Modify cleared by SavePcbFile()
        {
            OnModify();
            GetScreen()->SetSave(); // Set the flags m_FlagSave cleared by SetModify()
        }

        GetScreen()->SetFileName( tmpFileName );
        SetTitle( GetScreen()->GetFileName() );
    }

    oldpos = GetScreen()->GetCrossHairPosition();

    gridSize = GetScreen()->GetGridSize();

    switch( g_KeyPressed )
    {
    case WXK_NUMPAD8:       /* Deplacement curseur vers le haut */
    case WXK_UP:
        pos.y -= wxRound( gridSize.y );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD2:       /* Deplacement curseur vers le bas */
    case WXK_DOWN:
        pos.y += wxRound( gridSize.y );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD4:       /* Deplacement curseur vers la gauche */
    case WXK_LEFT:
        pos.x -= wxRound( gridSize.x );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD6:      /* Deplacement curseur vers la droite */
    case WXK_RIGHT:
        pos.x += wxRound( gridSize.x );
        DrawPanel->MoveCursor( pos );
        break;

    default:
        hotkey = g_KeyPressed;
        break;
    }

    // Put cursor in new position, according to the zoom keys (if any).
    GetScreen()->SetCrossHairPosition( pos );

    /* Put cursor on grid or a pad centre if requested. If the tool DELETE is active the
     * cursor is left off grid this is better to reach items to delete off grid,
     */
    bool   keep_on_grid = true;

    if( m_ID_current_state == ID_PCB_DELETE_ITEM_BUTT )
        keep_on_grid = false;

    /* Cursor is left off grid if no block in progress and no moving object */
    if( GetScreen()->m_BlockLocate.m_State != STATE_NO_BLOCK )
        keep_on_grid = true;

    EDA_ITEM* DrawStruct = GetScreen()->GetCurItem();

    if( DrawStruct && DrawStruct->m_Flags )
        keep_on_grid = true;

    if( keep_on_grid )
    {
        wxPoint on_grid = GetScreen()->GetNearestGridPosition( pos );

        wxSize grid;
        grid.x = (int) GetScreen()->GetGridSize().x;
        grid.y = (int) GetScreen()->GetGridSize().y;

        if( Magnetize( m_Pcb, this, m_ID_current_state, grid, on_grid, &pos ) )
        {
            GetScreen()->SetCrossHairPosition( pos );
        }
        else
        {
            // If there's no intrusion and DRC is active, we pass the cursor
            // "as is", and let ShowNewTrackWhenMovingCursor figure out what to do.
            if( !Drc_On || !g_CurrentTrackSegment
                || g_CurrentTrackSegment != this->GetCurItem()
                || !LocateIntrusion( m_Pcb->m_Track, g_CurrentTrackSegment,
                                     GetScreen()->m_Active_Layer, GetScreen()->RefPos( true ) ) )
            {
                GetScreen()->SetCrossHairPosition( on_grid );
            }
        }
    }

    if( oldpos != GetScreen()->GetCrossHairPosition() )
    {
        pos = GetScreen()->GetCrossHairPosition();
        GetScreen()->SetCrossHairPosition( oldpos );
        DrawPanel->CrossHairOff( aDC );
        GetScreen()->SetCrossHairPosition( pos );
        DrawPanel->CrossHairOn( aDC );

        if( DrawPanel->IsMouseCaptured() )
        {
#ifdef USE_WX_OVERLAY
            wxDCOverlay oDC( DrawPanel->m_overlay, (wxWindowDC*)aDC );
            oDC.Clear();
            DrawPanel->m_mouseCaptureCallback( DrawPanel, aDC, aPosition, false );
#else
            DrawPanel->m_mouseCaptureCallback( DrawPanel, aDC, aPosition, true );
#endif
        }
#ifdef USE_WX_OVERLAY
        else
            DrawPanel->m_overlay.Reset();
#endif
    }

    if( hotkey )
    {
        OnHotKey( aDC, hotkey, aPosition );
    }

    UpdateStatusBar();    /* Display new cursor coordinates */
}
