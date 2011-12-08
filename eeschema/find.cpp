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
        if( *sheetFoundIn != *m_CurrentSheet )
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


SCH_ITEM* SCH_EDIT_FRAME::FindComponentAndItem( const wxString& aReference,
                                                bool            aSearchHierarchy,
                                                SCH_SEARCH_T    aSearchType,
                                                const wxString& aSearchText,
                                                bool            aWarpMouse )
{
    SCH_SHEET_PATH* sheet;
    SCH_SHEET_PATH* sheetWithComponentFound = NULL;
    SCH_ITEM*       item = NULL;
    SCH_COMPONENT*  Component = NULL;
    wxPoint         pos, curpos;
    bool            centerAndRedraw = false;
    bool            notFound = true;
    wxString        msg;
    LIB_PIN*        pin;
    SCH_SHEET_LIST  sheetList;

    sheet = sheetList.GetFirst();

    if( !aSearchHierarchy )
        sheet = m_CurrentSheet;

    for( ; sheet != NULL; sheet = sheetList.GetNext() )
    {
        item = (SCH_ITEM*) sheet->LastDrawList();

        for( ; ( item != NULL ) && ( notFound == true ); item = item->Next() )
        {
            if( item->Type() != SCH_COMPONENT_T )
                continue;

            SCH_COMPONENT* pSch = (SCH_COMPONENT*) item;

            if( aReference.CmpNoCase( pSch->GetRef( sheet ) ) == 0 )
            {
                Component = pSch;
                sheetWithComponentFound = sheet;

                switch( aSearchType )
                {
                default:
                case FIND_COMPONENT_ONLY:    // Find component only
                    notFound = false;
                    pos = pSch->GetPosition();
                    break;

                case FIND_PIN:               // find a pin
                    pos = pSch->GetPosition();  // temporary: will be changed if the pin is found.
                    pin = pSch->GetPin( aSearchText );

                    if( pin == NULL )
                        break;

                    notFound = false;
                    pos += pin->GetPosition();
                    break;

                case FIND_REFERENCE:         // find reference
                    notFound = false;
                    pos = pSch->GetField( REFERENCE )->GetPosition();
                    break;

                case FIND_VALUE:             // find value
                    pos = pSch->GetPosition();

                    if( aSearchText.CmpNoCase( pSch->GetField( VALUE )->m_Text ) != 0 )
                        break;

                    notFound = false;
                    pos = pSch->GetField( VALUE )->GetPosition();
                    break;
                }
            }
        }

        if( (aSearchHierarchy == false) || (notFound == false) )
            break;
    }

    if( Component )
    {
        sheet = sheetWithComponentFound;

        if( *sheet != *m_CurrentSheet )
        {
            sheet->LastScreen()->SetZoom( GetScreen()->GetZoom() );
            *m_CurrentSheet = *sheet;
            m_CurrentSheet->UpdateAllScreenReferences();
            centerAndRedraw = true;
        }

        wxPoint delta;
        pos  -= Component->GetPosition();
        delta = Component->GetTransform().TransformCoordinate( pos );
        pos   = delta + Component->GetPosition();


        /* There may be need to reframe the drawing */
        if( ! DrawPanel->IsPointOnDisplay( pos ) )
        {
            centerAndRedraw = true;
        }

        if( centerAndRedraw )
        {
            GetScreen()->SetCrossHairPosition(pos);
            RedrawScreen( pos, aWarpMouse );
        }

        else
        {
            INSTALL_UNBUFFERED_DC( dc, DrawPanel );

            DrawPanel->CrossHairOff( &dc );

            if( aWarpMouse )
                DrawPanel->MoveCursor( pos );

            GetScreen()->SetCrossHairPosition(pos);

            DrawPanel->CrossHairOn( &dc );
        }
    }


    /* Print diag */
    wxString msg_item;
    msg = aReference;

    switch( aSearchType )
    {
    default:
    case FIND_COMPONENT_ONLY:      // Find component only
        break;

    case FIND_PIN:                 // find a pin
        msg_item = _( "Pin " ) + aSearchText;
        break;

    case FIND_REFERENCE:           // find reference
        msg_item = _( "Ref " ) + aSearchText;
        break;

    case FIND_VALUE:               // find value
        msg_item = _( "Value " ) + aSearchText;
        break;

    case FIND_FIELD:               // find field. todo
        msg_item = _( "Field " ) + aSearchText;
        break;
    }

    if( Component )
    {
        if( !notFound )
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

    return item;
}


void SCH_EDIT_FRAME::OnFindSchematicItem( wxFindDialogEvent& aEvent )
{
    static wxPoint itemPosition;  // the actual position of the matched item.

    SCH_SHEET_LIST schematic;
    wxString msg;
    SCH_FIND_REPLACE_DATA searchCriteria;
    bool warpCursor = !( aEvent.GetFlags() & FR_NO_WARP_CURSOR );

    searchCriteria.SetFlags( aEvent.GetFlags() );
    searchCriteria.SetFindString( aEvent.GetFindString() );
    searchCriteria.SetReplaceString( aEvent.GetReplaceString() );

    if( searchCriteria != m_foundItems.GetFindReplaceData() )
    {
        if( aEvent.GetEventType() == wxEVT_COMMAND_FIND_CLOSE )
        {
            warpCursor = true;
        }
        else if( aEvent.GetFlags() & FR_CURRENT_SHEET_ONLY && g_RootSheet->CountSheets() > 1 )
        {
            m_foundItemIndex = 0;
            m_foundItems.Collect( searchCriteria, m_CurrentSheet );
        }
        else
        {
            m_foundItemIndex = 0;
            m_foundItems.Collect( searchCriteria );
        }
    }
    else
    {
        if( searchCriteria.GetFlags() & wxFR_DOWN )
        {
            if( !(searchCriteria.GetFlags() & FR_SEARCH_WRAP)
                && (m_foundItemIndex == (m_foundItems.GetCount() - 1)) )
                return;

            m_foundItemIndex += 1;

            if( (m_foundItemIndex >= m_foundItems.GetCount())
                && (searchCriteria.GetFlags() & FR_SEARCH_WRAP) )
                m_foundItemIndex = 0;
        }
        else
        {
            if( !(searchCriteria.GetFlags() & FR_SEARCH_WRAP) && (m_foundItemIndex == 0) )
                return;

            m_foundItemIndex -= 1;

            if( (m_foundItemIndex < 0) && (searchCriteria.GetFlags() & FR_SEARCH_WRAP) )
                m_foundItemIndex = m_foundItems.GetCount() - 1;
        }
    }

    if( m_foundItems.GetCount() != 0 )
    {
        SCH_FIND_COLLECTOR_DATA data = m_foundItems.GetFindData( m_foundItemIndex );

        wxLogTrace( traceFindReplace, wxT( "Found " ) + m_foundItems.GetText( m_foundItemIndex ) );

        SCH_SHEET_PATH* sheet = schematic.GetSheet( data.GetSheetPath() );

        wxCHECK_RET( sheet != NULL, wxT( "Could not find sheet path " ) +
                     data.GetSheetPath() );

        if( sheet->PathHumanReadable() != m_CurrentSheet->PathHumanReadable() )
        {
            sheet->LastScreen()->SetZoom( GetScreen()->GetZoom() );
            *m_CurrentSheet = *sheet;
            m_CurrentSheet->UpdateAllScreenReferences();
        }

        sheet->LastScreen()->SetCrossHairPosition( data.GetPosition() );

        RedrawScreen( data.GetPosition(), warpCursor );

        aEvent.SetFlags( aEvent.GetFlags() | FR_REPLACE_ITEM_FOUND );
        msg = m_foundItems.GetText( m_foundItemIndex );
    }
    else
    {
        aEvent.SetFlags( aEvent.GetFlags() & ~FR_REPLACE_ITEM_FOUND );
        msg.Printf( _( "No item found matching %s." ), GetChars( aEvent.GetFindString() ) );
    }

    SetStatusText( msg );
}


void SCH_EDIT_FRAME::OnFindReplace( wxFindDialogEvent& aEvent )
{
    wxCHECK_RET( m_foundItems.IsValidIndex( m_foundItemIndex ),
                 wxT( "No last find item to replace text." ) );

    SCH_FIND_COLLECTOR_DATA data = m_foundItems.GetFindData( m_foundItemIndex );

    wxLogTrace( traceFindReplace, wxT( "Replacing %s with %s in item %s" ),
                GetChars( aEvent.GetFindString() ), GetChars( aEvent.GetReplaceString() ),
                GetChars( m_foundItems.GetText( m_foundItemIndex ) ) );

    OnFindSchematicItem( aEvent );

    if( aEvent.GetEventType() == wxEVT_COMMAND_FIND_REPLACE_ALL )
    {
        wxLogTrace( traceFindReplace, wxT( "Replacing %s with %s in item %s" ),
                    GetChars( aEvent.GetFindString() ), GetChars( aEvent.GetReplaceString() ),
                    GetChars( m_foundItems.GetText( m_foundItemIndex ) ) );

        OnFindSchematicItem( aEvent );
    }
}
