/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
 * @file eeschema/find.cpp
 * @brief Functions for searching for a schematic item.
 */

/*
 *  Search a text (text, value, reference) within a component or
 *  search a component in libraries, a marker ...,
 *  in current sheet or whole the project
 */
#include "fctsys.h"
#include "appl_wxstruct.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "class_library.h"
#include "lib_pin.h"
#include "sch_marker.h"
#include "sch_component.h"
#include "sch_sheet.h"
#include "sch_sheet_path.h"

#include "kicad_device_context.h"

#include <boost/foreach.hpp>

#include "dialogs/dialog_schematic_find.h"


void SCH_EDIT_FRAME::OnFindDrcMarker( wxFindDialogEvent& event )
{
    static SCH_MARKER* lastMarker = NULL;

    wxString           msg;
    SCH_SHEET_LIST     schematic;
    SCH_SHEET_PATH*    sheetFoundIn = NULL;
    bool               wrap = ( event.GetFlags() & FR_SEARCH_WRAP ) != 0;
    wxRect             clientRect( wxPoint( 0, 0 ), GetClientSize() );
    bool               warpCursor = ( ( event.GetId() == wxEVT_COMMAND_FIND_CLOSE ) ||
                                      !( event.GetFlags() & FR_NO_WARP_CURSOR ) );

    if( event.GetFlags() & FR_CURRENT_SHEET_ONLY )
    {
        sheetFoundIn = m_CurrentSheet;
        lastMarker = (SCH_MARKER*) m_CurrentSheet->FindNextItem( SCH_MARKER_T, lastMarker, wrap );
    }
    else
    {
        lastMarker = (SCH_MARKER*) schematic.FindNextItem( SCH_MARKER_T, &sheetFoundIn,
                                                           lastMarker, wrap );
    }

    if( lastMarker != NULL )
    {
        if( sheetFoundIn != GetSheet() )
        {
            sheetFoundIn->LastScreen()->SetZoom( GetScreen()->GetZoom() );
            *m_CurrentSheet = *sheetFoundIn;
            m_CurrentSheet->UpdateAllScreenReferences();
        }

        sheetFoundIn->LastScreen()->SetCrossHairPosition( lastMarker->GetPosition() );

        RedrawScreen( lastMarker->GetPosition(), warpCursor );

        wxString path = sheetFoundIn->Path();
        wxString units = GetAbbreviatedUnitsLabel();
        double x = To_User_Unit( g_UserUnit, (double) lastMarker->GetPosition().x,
                                 m_InternalUnits );
        double y = To_User_Unit( g_UserUnit, (double) lastMarker->GetPosition().y,
                                 m_InternalUnits );
        msg.Printf( _( "Design rule check marker found in sheet %s at %0.3f%s, %0.3f%s" ),
                    GetChars( path ), x, GetChars( units ), y, GetChars( units) );
        SetStatusText( msg );
    }
    else
    {
        SetStatusText( _( "No more markers were found." ) );
    }
}


SCH_ITEM* SCH_EDIT_FRAME::FindComponentAndItem( const wxString& component_reference,
                                                bool Find_in_hierarchy,
                                                int SearchType,
                                                const wxString& text_to_find,
                                                bool mouseWarp )
{
    SCH_SHEET_PATH* sheet;
    SCH_SHEET_PATH* sheetWithComponentFound = NULL;
    SCH_ITEM*       DrawList     = NULL;
    SCH_COMPONENT*  Component    = NULL;
    wxPoint         pos, curpos;
    bool            DoCenterAndRedraw = false;
    bool            NotFound = true;
    wxString        msg;
    LIB_PIN*        pin;
    SCH_SHEET_LIST  SheetList;

    sheet = SheetList.GetFirst();

    if( !Find_in_hierarchy )
        sheet = m_CurrentSheet;

    for( ; sheet != NULL; sheet = SheetList.GetNext() )
    {
        DrawList = (SCH_ITEM*) sheet->LastDrawList();

        for( ; ( DrawList != NULL ) && ( NotFound == true ); DrawList = DrawList->Next() )
        {
            if( DrawList->Type() != SCH_COMPONENT_T )
                continue;

            SCH_COMPONENT* pSch = (SCH_COMPONENT*) DrawList;

            if( component_reference.CmpNoCase( pSch->GetRef( sheet ) ) == 0 )
            {
                Component = pSch;
                sheetWithComponentFound = sheet;

                switch( SearchType )
                {
                default:
                case 0:             // Find component only
                    NotFound = false;
                    pos = pSch->GetPosition();
                    break;

                case 1:                 // find a pin
                    pos = pSch->GetPosition();  // temporary: will be changed if the pin is found.
                    pin = pSch->GetPin( text_to_find );

                    if( pin == NULL )
                        break;

                    NotFound = false;
                    pos += pin->GetPosition();
                    break;

                case 2:     // find reference
                    NotFound = false;
                    pos = pSch->GetField( REFERENCE )->GetPosition();
                    break;

                case 3:     // find value
                    pos = pSch->GetPosition();

                    if( text_to_find.CmpNoCase( pSch->GetField( VALUE )->m_Text ) != 0 )
                        break;

                    NotFound = false;
                    pos = pSch->GetField( VALUE )->GetPosition();
                    break;
                }
            }
        }

        if( (Find_in_hierarchy == false) || (NotFound == false) )
            break;
    }

    if( Component )
    {
        sheet = sheetWithComponentFound;
        if( *sheet != *GetSheet() )
        {
            sheet->LastScreen()->SetZoom( GetScreen()->GetZoom() );
            *m_CurrentSheet = *sheet;
            m_CurrentSheet->UpdateAllScreenReferences();
            DoCenterAndRedraw = true;
        }

        wxPoint delta;
        pos  -= Component->GetPosition();
        delta = Component->GetTransform().TransformCoordinate( pos );
        pos   = delta + Component->GetPosition();


        /* There may be need to reframe the drawing */
        if( ! DrawPanel->IsPointOnDisplay( pos ) )
        {
            DoCenterAndRedraw = true;
        }

        if( DoCenterAndRedraw )
        {
            GetScreen()->SetCrossHairPosition(pos);
            RedrawScreen( pos, mouseWarp );
        }

        else
        {
            INSTALL_UNBUFFERED_DC( dc, DrawPanel );

            DrawPanel->CrossHairOff( &dc );

            if( mouseWarp )
                DrawPanel->MoveCursor( pos );

            GetScreen()->SetCrossHairPosition(pos);

            DrawPanel->CrossHairOn( &dc );
        }
    }


    /* Print diag */
    wxString msg_item;
    msg = component_reference;

    switch( SearchType )
    {
    default:
    case 0:
        break;      // Find component only

    case 1:         // find a pin
        msg_item = _( "Pin " ) + text_to_find;
        break;

    case 2:     // find reference
        msg_item = _( "Ref " ) + text_to_find;
        break;

    case 3:     // find value
        msg_item = _( "Value " ) + text_to_find;
        break;

    case 4:     // find field. todo
        msg_item = _( "Field " ) + text_to_find;
        break;
    }

    if( Component )
    {
        if( !NotFound )
        {
            if( !msg_item.IsEmpty() )
                msg += wxT( " " ) + msg_item;
            msg += _( " found" );
        }
        else
        {
            msg += _( " found" );
            if( !msg_item.IsEmpty() )
            {
                msg += wxT( " but " ) + msg_item + _( " not found" );
            }
        }
    }
    else
    {
        if( !msg_item.IsEmpty() )
            msg += wxT( " " ) + msg_item;
        msg += _( " not found" );
    }

    SetStatusText( msg );

    return DrawList;
}


void SCH_EDIT_FRAME::OnFindSchematicItem( wxFindDialogEvent& aEvent )
{
    static SCH_ITEM*  lastItem = NULL;  /* last item found when searching a match
                                         * note: the actual matched item can be a
                                         * part of lastItem (for instance a field in a component
                                         */
    static wxPoint    lastItemPosition; // the actual position of the matched sub item

    SCH_SHEET_LIST    schematic;
    wxString          msg;
    SCH_SHEET_PATH*   sheetFoundIn = NULL;
    wxFindReplaceData searchCriteria;
    bool              warpCursor = !( aEvent.GetFlags() & FR_NO_WARP_CURSOR );

    searchCriteria.SetFlags( aEvent.GetFlags() );
    searchCriteria.SetFindString( aEvent.GetFindString() );
    searchCriteria.SetReplaceString( aEvent.GetReplaceString() );

    if( aEvent.GetEventType() == wxEVT_COMMAND_FIND_CLOSE )
    {
        sheetFoundIn = m_CurrentSheet;
        warpCursor = true;
    }
    else if( aEvent.GetFlags() & FR_CURRENT_SHEET_ONLY && g_RootSheet->CountSheets() > 1 )
    {
        sheetFoundIn = m_CurrentSheet;
        lastItem = m_CurrentSheet->MatchNextItem( searchCriteria, lastItem, &lastItemPosition );
    }
    else
    {
        lastItem = schematic.MatchNextItem( searchCriteria, &sheetFoundIn, lastItem,
                                            &lastItemPosition );
    }

    if( lastItem != NULL )
    {
        if( sheetFoundIn != GetSheet() )
        {
            sheetFoundIn->LastScreen()->SetZoom( GetScreen()->GetZoom() );
            *m_CurrentSheet = *sheetFoundIn;
            m_CurrentSheet->UpdateAllScreenReferences();
        }

        sheetFoundIn->LastScreen()->SetCrossHairPosition( lastItemPosition );

        RedrawScreen( lastItemPosition, warpCursor );

        msg = aEvent.GetFindString() + _( " found in " ) + sheetFoundIn->PathHumanReadable();
        SetStatusText( msg );
    }
    else
    {
        msg.Printf( _( "No item found matching %s." ), GetChars( aEvent.GetFindString() ) );
        SetStatusText( msg );
    }
}
