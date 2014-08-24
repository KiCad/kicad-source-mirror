/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pcbnew/controle.cpp
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <wxPcbStruct.h>
#include <pcbnew_id.h>
#include <class_board.h>
#include <class_module.h>
#include <class_zone.h>

#include <pcbnew.h>
#include <protos.h>
#include <collectors.h>
#include <menus_helpers.h>

//external functions used here:
extern bool Magnetize( PCB_EDIT_FRAME* frame, int aCurrentTool,
                       wxSize aGridSize, wxPoint on_grid, wxPoint* curpos );


/**
 * Function AllAreModulesAndReturnSmallestIfSo
 * tests that all items in the collection are MODULEs and if so, returns the
 * smallest MODULE.
 * @return BOARD_ITEM* - The smallest or NULL.
 */
static BOARD_ITEM* AllAreModulesAndReturnSmallestIfSo( GENERAL_COLLECTOR* aCollector )
{
#if 0   // Dick: this is not consistent with name of this function, and does not
        // work correctly using 'M' (move hotkey) when another module's (2nd module) reference
        // is under a module (first module) and you want to move the reference.
        // Another way to fix this would be to
        // treat module text as copper layer content, and put the module text into
        // the primary list.  I like the coded behavior best.  If it breaks something
        // perhaps you need a different test before calling this function, which should
        // do what its name says it does.
    int count = aCollector->GetPrimaryCount();     // try to use preferred layer
    if( 0 == count ) count = aCollector->GetCount();
#else
    int count = aCollector->GetCount();
#endif

    for( int i = 0; i<count;  ++i )
    {
        if( (*aCollector)[i]->Type() != PCB_MODULE_T )
            return NULL;
    }

    // all are modules, now find smallest MODULE

    int minDim = 0x7FFFFFFF;
    int minNdx = 0;

    for( int i = 0;  i<count;  ++i )
    {
        MODULE* module = (MODULE*) (*aCollector)[i];

        int     lx = module->GetBoundingBox().GetWidth();
        int     ly = module->GetBoundingBox().GetHeight();

        int     lmin = std::min( lx, ly );

        if( lmin < minDim )
        {
            minDim = lmin;
            minNdx = i;
        }
    }

    return (*aCollector)[minNdx];
}


BOARD_ITEM* PCB_BASE_FRAME::PcbGeneralLocateAndDisplay( int aHotKeyCode )
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
    else if( GetToolId() == ID_NO_TOOL_SELECTED )
    {
        if( m_mainToolBar->GetToolToggled( ID_TOOLBARH_PCB_MODE_MODULE ) )
            scanList = GENERAL_COLLECTOR::Modules;
        else
            scanList = (DisplayOpt.DisplayZonesMode == 0) ?
                       GENERAL_COLLECTOR::AllBoardItems :
                       GENERAL_COLLECTOR::AllButZones;
    }
    else
    {
        switch( GetToolId() )
        {
        case ID_PCB_SHOW_1_RATSNEST_BUTT:
            scanList = GENERAL_COLLECTOR::PadsOrModules;
            break;

        case ID_TRACK_BUTT:
            scanList = GENERAL_COLLECTOR::Tracks;
            break;

        case ID_PCB_MODULE_BUTT:
            scanList = GENERAL_COLLECTOR::Modules;
            break;

        case ID_PCB_ZONES_BUTT:
        case ID_PCB_KEEPOUT_AREA_BUTT:
            scanList = GENERAL_COLLECTOR::Zones;
            break;

        default:
            scanList = DisplayOpt.DisplayZonesMode == 0 ?
                       GENERAL_COLLECTOR::AllBoardItems :
                       GENERAL_COLLECTOR::AllButZones;
        }
    }

    m_Collector->Collect( m_Pcb, scanList, RefPos( true ), guide );

#if 0
    // debugging: print out the collected items, showing their priority order too.
    for( int i = 0; i<m_Collector->GetCount();  ++i )
        (*m_Collector)[i]->Show( 0, std::cout );
#endif

    /* Remove redundancies: sometime, legacy zones are found twice,
     * because zones can be filled by overlapping segments (this is a fill option)
     * Trigger the selection of the current edge for new-style zones
     */
    time_t timestampzone = 0;

    for( int ii = 0;  ii < m_Collector->GetCount(); ii++ )
    {
        item = (*m_Collector)[ii];

        switch( item->Type() )
        {
        case PCB_ZONE_T:
            // Found a TYPE ZONE
            if( item->GetTimeStamp() == timestampzone )    // Remove it, redundant, zone already found
            {
                m_Collector->Remove( ii );
                ii--;
            }
            else
            {
                timestampzone = item->GetTimeStamp();
            }
            break;

        case PCB_ZONE_AREA_T:
            {
                /* We need to do the selection now because the menu text
                 * depends on it */
                ZONE_CONTAINER *zone = static_cast<ZONE_CONTAINER*>( item );
                zone->SetSelectedCorner( RefPos( true ) );
            }
            break;

        default:
            break;
        }
    }

    if( m_Collector->GetCount() <= 1 )
    {
        item = (*m_Collector)[0];
        SetCurItem( item );
    }

    // If the count is 2, and first item is a pad or module text, and the 2nd item is its
    // parent module:
    else if( m_Collector->GetCount() == 2
             && ( (*m_Collector)[0]->Type() == PCB_PAD_T || (*m_Collector)[0]->Type() ==
                 PCB_MODULE_TEXT_T )
             && (*m_Collector)[1]->Type() == PCB_MODULE_T && (*m_Collector)[0]->GetParent()==
             (*m_Collector)[1] )
    {
        item = (*m_Collector)[0];
        SetCurItem( item );
    }
    // if all are modules, find the smallest one among the primary choices
    else if( ( item = AllAreModulesAndReturnSmallestIfSo( m_Collector ) ) != NULL )
    {
        SetCurItem( item );
    }

    else    // we can't figure out which item user wants, do popup menu so user can choose
    {
        wxMenu itemMenu;

        // Give a title to the selection menu. This is also a cancel menu item
        wxMenuItem * item_title = new wxMenuItem( &itemMenu, -1, _( "Selection Clarification" ) );

#ifdef __WINDOWS__
        wxFont bold_font( *wxNORMAL_FONT );
        bold_font.SetWeight( wxFONTWEIGHT_BOLD );
        bold_font.SetStyle( wxFONTSTYLE_ITALIC );
        item_title->SetFont( bold_font );
#endif

        itemMenu.Append( item_title );
        itemMenu.AppendSeparator();

        int limit = std::min( MAX_ITEMS_IN_PICKER, m_Collector->GetCount() );

        for( int i = 0;  i<limit;  ++i )
        {
            wxString    text;
            item = (*m_Collector)[i];

            text = item->GetSelectMenuText();

            BITMAP_DEF xpm = item->GetMenuImage();

            AddMenuItem( &itemMenu, ID_POPUP_PCB_ITEM_SELECTION_START + i, text, KiBitmap( xpm ) );
        }

        /* @todo: rather than assignment to true, these should be increment and decrement
         * operators throughout _everywhere_.
         *  That way we can handle nesting.
         *  But I tried that and found there cases where the assignment to true (converted to
         * a m_IgnoreMouseEvents++ )
         *  was not balanced with the -- (now m_IgnoreMouseEvents=false), so I had to revert.
         *  Somebody should track down these and make them balanced.
         *  m_canvas->SetIgnoreMouseEvents( true );
         */

        // this menu's handler is void PCB_BASE_FRAME::ProcessItemSelection()
        // and it calls SetCurItem() which in turn calls DisplayInfo() on the item.
        m_canvas->SetAbortRequest( true );   // changed in false if an item is selected
        PopupMenu( &itemMenu );

        m_canvas->MoveCursorToCrossHair();

        // The function ProcessItemSelection() has set the current item, return it.
        if( m_canvas->GetAbortRequest() )     // Nothing selected
            item = NULL;
        else
            item = GetCurItem();
    }

    return item;
}


void PCB_EDIT_FRAME::GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey )
{
    // Filter out the 'fake' mouse motion after a keyboard movement
    if( !aHotKey && m_movingCursorWithKeyboard )
    {
        m_movingCursorWithKeyboard = false;
        return;
    }

    // when moving mouse, use the "magnetic" grid, unless the shift+ctrl keys is pressed
    // for next cursor position
    // ( shift or ctrl key down are PAN command with mouse wheel)
    bool snapToGrid = true;
    if( !aHotKey && wxGetKeyState( WXK_SHIFT ) && wxGetKeyState( WXK_CONTROL ) )
        snapToGrid = false;

    wxPoint oldpos = GetCrossHairPosition();
    wxPoint pos = aPosition;
    GeneralControlKeyMovement( aHotKey, &pos, snapToGrid );

    // Put cursor in new position, according to the zoom keys (if any).
    SetCrossHairPosition( pos, snapToGrid );

    /* Put cursor on grid or a pad centre if requested. If the tool DELETE is active the
     * cursor is left off grid this is better to reach items to delete off grid,
     */
    if( GetToolId() == ID_PCB_DELETE_ITEM_BUTT )
        snapToGrid = false;

    // Cursor is left off grid if no block in progress
    if( GetScreen()->m_BlockLocate.GetState() != STATE_NO_BLOCK )
        snapToGrid = true;

    wxPoint curs_pos = pos;

    wxRealPoint gridSize = GetScreen()->GetGridSize();
    wxSize igridsize;
    igridsize.x = KiROUND( gridSize.x );
    igridsize.y = KiROUND( gridSize.y );

    if( Magnetize( this, GetToolId(), igridsize, curs_pos, &pos ) )
    {
        SetCrossHairPosition( pos, false );
    }
    else
    {
        // If there's no intrusion and DRC is active, we pass the cursor
        // "as is", and let ShowNewTrackWhenMovingCursor figure out what to do.
        if( !g_Drc_On || !g_CurrentTrackSegment ||
            (BOARD_ITEM*)g_CurrentTrackSegment != this->GetCurItem() ||
            !LocateIntrusion( m_Pcb->m_Track, g_CurrentTrackSegment,
                              GetScreen()->m_Active_Layer, RefPos( true ) ) )
        {
            SetCrossHairPosition( curs_pos, snapToGrid );
        }
    }

    RefreshCrossHair( oldpos, aPosition, aDC );

    if( aHotKey )
    {
        OnHotKey( aDC, aHotKey, aPosition );
    }

    UpdateStatusBar();    // Display new cursor coordinates
}
