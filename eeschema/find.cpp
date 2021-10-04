/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2021 KiCad Developers, see change_log.txt for contributors.
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
#include <fctsys.h>
#include <pgm_base.h>
#include <sch_draw_panel.h>
#include <confirm.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <sch_edit_frame.h>
#include <base_units.h>
#include <trace_helpers.h>
#include <sch_view.h>
#include <general.h>
#include <class_library.h>
#include <lib_pin.h>
#include <sch_marker.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <symbol_lib_table.h>

#include <kicad_device_context.h>

#include <dialogs/dialog_schematic_find.h>


void SCH_EDIT_FRAME::OnFindDrcMarker( wxFindDialogEvent& event )
{
    static SCH_MARKER* lastMarker = NULL;

    wxString           msg;
    SCH_SHEET_LIST     schematic( g_RootSheet );
    SCH_SHEET_PATH*    sheetFoundIn = NULL;
    bool               wrap = ( event.GetFlags() & FR_SEARCH_WRAP ) != 0;
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

        SetCrossHairPosition( lastMarker->GetPosition() );


        CenterScreen( lastMarker->GetPosition(), warpCursor );

        msg.Printf( _( "Design rule check marker found in sheet %s at %s, %s" ),
                    sheetFoundIn->Path(),
                    MessageTextFromValue( m_UserUnits, lastMarker->GetPosition().x ),
                    MessageTextFromValue( m_UserUnits, lastMarker->GetPosition().y ) );
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
                                                const wxString& aSearchText )
{
    SCH_SHEET_PATH* sheet = NULL;
    SCH_SHEET_PATH* sheetWithComponentFound = NULL;
    SCH_ITEM*       item = NULL;
    SCH_COMPONENT*  Component = NULL;
    wxPoint         pos;
    bool            notFound = true;
    LIB_PIN*        pin = nullptr;
    SCH_SHEET_LIST  sheetList( g_RootSheet );
    EDA_ITEM*       foundItem = nullptr;

    if( !aSearchHierarchy )
        sheetList.push_back( *m_CurrentSheet );
    else
        sheetList.BuildSheetList( g_RootSheet, false );

    for( SCH_SHEET_PATHS_ITER it = sheetList.begin(); it != sheetList.end(); ++it )
    {
        sheet = &(*it);
        item = (*it).LastDrawList();

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
                    foundItem = Component;
                    break;

                case FIND_PIN:               // find a pin
                    pos = pSch->GetPosition();  // temporary: will be changed if the pin is found.
                    pin = pSch->GetPin( aSearchText );

                    if( pin == NULL )
                        break;

                    notFound = false;
                    pos += pin->GetPosition();
                    foundItem = Component;
                    break;

                case FIND_REFERENCE:         // find reference
                    notFound = false;
                    pos = pSch->GetField( REFERENCE )->GetPosition();
                    foundItem = pSch->GetField( REFERENCE );
                    break;

                case FIND_VALUE:             // find value
                    pos = pSch->GetPosition();

                    if( aSearchText.CmpNoCase( pSch->GetField( VALUE )->GetShownText() ) != 0 )
                        break;

                    notFound = false;
                    pos = pSch->GetField( VALUE )->GetPosition();
                    foundItem = pSch->GetField( VALUE );

                    break;
                }
            }
        }

        if( notFound == false )
            break;
    }

    if( Component )
    {
        sheet = sheetWithComponentFound;

        if( *sheet != *m_CurrentSheet )
        {
            sheet->LastScreen()->SetZoom( GetScreen()->GetZoom() );
            *m_CurrentSheet = *sheet;
            DisplayCurrentSheet();
        }

        wxPoint delta;
        pos  -= Component->GetPosition();
        delta = Component->GetTransform().TransformCoordinate( pos );
        pos   = delta + Component->GetPosition();

        SetCrossHairPosition( pos );
        CenterScreen( pos, false );
    }

    /* Print diag */
    wxString msg_item;
    wxString msg;

    switch( aSearchType )
    {
    default:
    case FIND_COMPONENT_ONLY: msg_item = _( "component" );                         break;
    case FIND_PIN:            msg_item.Printf( _( "pin %s" ), aSearchText );       break;
    case FIND_REFERENCE:      msg_item.Printf( _( "reference %s" ), aSearchText ); break;
    case FIND_VALUE:          msg_item.Printf( _( "value %s" ), aSearchText );     break;
    case FIND_FIELD:          msg_item.Printf( _( "field %s" ), aSearchText );     break;
    }

    if( Component )
    {
        if( !notFound )
            msg.Printf( _( "%s %s found" ), aReference, msg_item );
        else
            msg.Printf( _( "%s found but %s not found" ), aReference, msg_item );
    }
    else
        msg.Printf( _( "Component %s not found" ), aReference );

    SetStatusText( msg );

    // highlight selection if foundItem is not null, or clear any highlighted selection
    GetCanvas()->GetView()->HighlightItem( foundItem, pin );
    GetCanvas()->Refresh();

    return item;
}


bool SCH_EDIT_FRAME::IsSearchCacheObsolete( const SCH_FIND_REPLACE_DATA& aSearchCriteria )
{
    int mod_hash = Prj().SchSymbolLibTable()->GetModifyHash();

    // the cache is obsolete whenever any library changes.
    if( mod_hash != m_foundItems.GetLibHash() )
    {
        m_foundItems.SetForceSearch();
        m_foundItems.SetLibHash( mod_hash );
        return true;
    }
    else if( m_foundItems.IsSearchRequired( aSearchCriteria ) )
        return true;
    else
        return false;
}


void SCH_EDIT_FRAME::OnFindSchematicItem( wxFindDialogEvent& aEvent )
{
    SCH_FIND_REPLACE_DATA   searchCriteria;
    SCH_FIND_COLLECTOR_DATA data;

    searchCriteria.SetFlags( aEvent.GetFlags() );
    searchCriteria.SetFindString( aEvent.GetFindString() );
    searchCriteria.SetReplaceString( aEvent.GetReplaceString() );

    if( aEvent.GetEventType() == wxEVT_COMMAND_FIND_CLOSE )
    {
        if( m_foundItems.GetCount() == 0 )
            return;
    }
    else if( IsSearchCacheObsolete( searchCriteria ) )
    {
        if( aEvent.GetFlags() & FR_CURRENT_SHEET_ONLY && g_RootSheet->CountSheets() > 1 )
        {
            m_foundItems.Collect( searchCriteria, m_CurrentSheet );
        }
        else
        {
            m_foundItems.Collect( searchCriteria );
        }
    }
    else
    {
        EDA_ITEM* currentItem = m_foundItems.GetItem( data );

        if( currentItem != NULL )
            currentItem->SetForceVisible( false );

        m_foundItems.UpdateIndex();
    }

    updateFindReplaceView( aEvent );
}


void SCH_EDIT_FRAME::OnFindReplace( wxFindDialogEvent& aEvent )
{
    SCH_ITEM*               item;
    SCH_SHEET_PATH*         sheet;
    SCH_SHEET_LIST          schematic( g_RootSheet );
    SCH_FIND_COLLECTOR_DATA data;
    SCH_FIND_REPLACE_DATA   searchCriteria;

    searchCriteria.SetFlags( aEvent.GetFlags() );
    searchCriteria.SetFindString( aEvent.GetFindString() );
    searchCriteria.SetReplaceString( aEvent.GetReplaceString() );
    m_foundItems.SetReplaceString( aEvent.GetReplaceString() );

    if( IsSearchCacheObsolete( searchCriteria ) )
    {
        if( aEvent.GetFlags() & FR_CURRENT_SHEET_ONLY && g_RootSheet->CountSheets() > 1 )
        {
            m_foundItems.Collect( searchCriteria, m_CurrentSheet );
        }
        else
        {
            m_foundItems.Collect( searchCriteria );
        }
    }

    if( aEvent.GetEventType() == wxEVT_COMMAND_FIND_REPLACE_ALL )
    {
        while( ( item = (SCH_ITEM*) m_foundItems.GetItem( data ) ) != NULL )
        {
            SCH_ITEM* undoItem = data.GetParent();

            // Don't save child items in undo list.
            if( undoItem == NULL )
                undoItem = item;

            SetUndoItem( undoItem );

            sheet = schematic.GetSheetByPath( data.GetSheetPath() );

            wxCHECK_RET( sheet != NULL, wxT( "Could not find sheet path " ) + data.GetSheetPath() );

            if( m_foundItems.ReplaceItem( sheet ) )
            {
                GetCanvas()->GetView()->Update( undoItem, KIGFX::ALL );
                OnModify();
                SaveUndoItemInUndoList( undoItem );
                updateFindReplaceView( aEvent );
            }

            m_foundItems.IncrementIndex();

            if( m_foundItems.PassedEnd() )
                break;
        }
    }
    else
    {
        item = (SCH_ITEM*) m_foundItems.GetItem( data );

        wxCHECK_RET( item != NULL, wxT( "Invalid replace item in find collector list." ) );

        SCH_ITEM* undoItem = data.GetParent();

        if( undoItem == NULL )
            undoItem = item;

        SetUndoItem( undoItem );

        sheet = schematic.GetSheetByPath( data.GetSheetPath() );

        wxCHECK_RET( sheet != NULL, wxT( "Could not find sheet path " ) + data.GetSheetPath() );

        if( m_foundItems.ReplaceItem( sheet ) )
        {
            GetCanvas()->GetView()->Update( undoItem, KIGFX::ALL );
            OnModify();
            SaveUndoItemInUndoList( undoItem );
            updateFindReplaceView( aEvent );

            // A single replace is part of the search; it does not dirty it.
            m_foundItems.SetForceSearch( false );
        }

        m_foundItems.IncrementIndex();
    }

    // End the replace if we are at the end if the list.  This prevents an infinite loop if
    // wrap search is selected and all of the items have been replaced with a value that
    // still satisfies the search criteria.
    if( m_foundItems.PassedEnd() )
        aEvent.SetFlags( aEvent.GetFlags() & ~FR_REPLACE_ITEM_FOUND );
}


void SCH_EDIT_FRAME::updateFindReplaceView( wxFindDialogEvent& aEvent )
{
    wxString                msg;
    SCH_SHEET_LIST          schematic( g_RootSheet );
    SCH_FIND_COLLECTOR_DATA data;
    bool                    warpCursor = !( aEvent.GetFlags() & FR_NO_WARP_CURSOR );

    if( m_foundItems.GetItem( data ) != NULL )
    {
        wxLogTrace( traceFindReplace, wxT( "Found " ) + m_foundItems.GetText( MILLIMETRES ) );

        SCH_SHEET_PATH* sheet = schematic.GetSheetByPath( data.GetSheetPath() );

        wxCHECK_RET( sheet != NULL, wxT( "Could not find sheet path " ) +
                     data.GetSheetPath() );

        SCH_ITEM* item = (SCH_ITEM*)m_foundItems.GetItem( data );

        // Make the item temporarily visible just in case it's hide flag is set.  This
        // has no effect on objects that don't support hiding.  If this is a close find
        // dialog event, clear the temporary visibility flag.
        if( item )
        {
            if( aEvent.GetEventType() == wxEVT_COMMAND_FIND_CLOSE )
                item->SetForceVisible( false );
            else if( item->Type() == SCH_FIELD_T && !( (SCH_FIELD*) item )->IsVisible() )
                item->SetForceVisible( true );
        }

        if( sheet->PathHumanReadable() != m_CurrentSheet->PathHumanReadable() )
        {
            sheet->LastScreen()->SetZoom( GetScreen()->GetZoom() );
            *m_CurrentSheet = *sheet;
            m_CurrentSheet->UpdateAllScreenReferences();
            SetScreen( sheet->LastScreen() );
            sheet->LastScreen()->TestDanglingEnds();
        }

        SetCrossHairPosition( data.GetPosition() );
        CenterScreen( data.GetPosition(), warpCursor );

        msg = m_foundItems.GetText( m_UserUnits );

        if( aEvent.GetFlags() & FR_SEARCH_REPLACE )
            aEvent.SetFlags( aEvent.GetFlags() | FR_REPLACE_ITEM_FOUND );
    }
    else
    {
        if( aEvent.GetFlags() & FR_SEARCH_REPLACE )
            aEvent.SetFlags( aEvent.GetFlags() & ~FR_REPLACE_ITEM_FOUND );

        msg.Printf( _( "No item found matching %s." ), GetChars( aEvent.GetFindString() ) );
    }

    *m_findReplaceStatus = msg;
    SetStatusText( msg );
}
