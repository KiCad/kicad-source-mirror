/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * @file operations_on_items_lists.cpp
 * @brief Functions used in block commands, or undo/redo, to move, mirror, delete, copy ...
 *        lists of schematic items.
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <class_drawpanel.h>
#include <sch_edit_frame.h>

#include <general.h>
#include <list_operations.h>
#include <sch_bus_entry.h>
#include <sch_marker.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_sheet.h>
#include <sch_component.h>
#include <sch_junction.h>


void SetSchItemParent( SCH_ITEM* Struct, SCH_SCREEN* Screen )
{
    switch( Struct->Type() )
    {
    case SCH_JUNCTION_T:
    case SCH_TEXT_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
    case SCH_COMPONENT_T:
    case SCH_LINE_T:
    case SCH_BUS_BUS_ENTRY_T:
    case SCH_BUS_WIRE_ENTRY_T:
    case SCH_SHEET_T:
    case SCH_MARKER_T:
    case SCH_NO_CONNECT_T:
        Struct->SetParent( Screen );
        break;

    case SCH_SHEET_PIN_T:
        break;

    default:
        break;
    }
}


void RotateListOfItems( PICKED_ITEMS_LIST& aItemsList, const wxPoint& rotationPoint )
{
    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) aItemsList.GetPickedItem( ii );
        item->Rotate( rotationPoint );      // Place it in its new position.
        item->ClearFlags();
    }
}


void MirrorY( PICKED_ITEMS_LIST& aItemsList, const wxPoint& aMirrorPoint )
{
    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) aItemsList.GetPickedItem( ii );
        item->MirrorY( aMirrorPoint.x );      // Place it in its new position.
        item->ClearFlags();
    }
}


void MirrorX( PICKED_ITEMS_LIST& aItemsList, const wxPoint& aMirrorPoint )
{
    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) aItemsList.GetPickedItem( ii );
        item->MirrorX( aMirrorPoint.y );      // Place it in its new position.
        item->ClearFlags();
    }
}


void MoveItemsInList( PICKED_ITEMS_LIST& aItemsList, const wxPoint& aMoveVector )
{
    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) aItemsList.GetPickedItem( ii );
        item->Move( aMoveVector );
    }
}


void SCH_EDIT_FRAME::CheckListConnections( PICKED_ITEMS_LIST& aItemsList, bool aAppend )
{
    std::vector< wxPoint > pts;
    std::vector< wxPoint > connections;

    GetSchematicConnections( connections );
    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) aItemsList.GetPickedItem( ii );
        std::vector< wxPoint > new_pts;

        if( !item->IsConnectable() )
            continue;

        item->GetConnectionPoints( new_pts );
        pts.insert( pts.end(), new_pts.begin(), new_pts.end() );

        // If the item is a line, we also add any connection points from the rest of the schematic
        // that terminate on the line after it is moved.
        if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = (SCH_LINE*) item;
            for( auto i : connections )
                if( IsPointOnSegment( line->GetStartPoint(), line->GetEndPoint(), i ) )
                    pts.push_back( i );
        }
        else
        {
            // Clean up any wires that short non-wire connections in the list
            for( auto point = new_pts.begin(); point != new_pts.end(); point++ )
            {
                for( auto second_point = point + 1; second_point != new_pts.end(); second_point++ )
                {
                    aAppend |= TrimWire( *point, *second_point, aAppend );
                }
            }
        }
    }

    // We always have some overlapping connection points.  Drop duplicates here
    std::sort( pts.begin(), pts.end(),
            []( const wxPoint& a, const wxPoint& b ) -> bool
            { return a.x < b.x || (a.x == b.x && a.y < b.y); } );
    pts.erase( unique( pts.begin(), pts.end() ), pts.end() );

    for( auto point : pts )
    {
        if( GetScreen()->IsJunctionNeeded( point, true ) )
        {
            AddJunction( point, aAppend );
            aAppend = true;
        }
    }
}


void SCH_EDIT_FRAME::DeleteItemsInList( PICKED_ITEMS_LIST& aItemsList, bool aAppend )
{
    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) aItemsList.GetPickedItem( ii );

        if( item->GetFlags() & STRUCT_DELETED )
            continue;

        DeleteItem( item, aAppend );
        aAppend = true;
    }

    GetScreen()->ClearDrawingState();
}


void SCH_EDIT_FRAME::DeleteItem( SCH_ITEM* aItem, bool aAppend )
{
    wxCHECK_RET( aItem != NULL, wxT( "Cannot delete invalid item." ) );
    wxCHECK_RET( !( aItem->GetFlags() & STRUCT_DELETED ),
                 wxT( "Cannot delete item that is already deleted." ) );

    // Here, aItem is not null.
    SCH_SCREEN* screen = GetScreen();

    if( aItem->Type() == SCH_SHEET_PIN_T )
    {
        // This item is attached to a node, and is not accessible by the global list directly.
        SCH_SHEET* sheet = (SCH_SHEET*) aItem->GetParent();
        wxCHECK_RET( (sheet != NULL) && (sheet->Type() == SCH_SHEET_T),
                     wxT( "Sheet label has invalid parent item." ) );
        SaveCopyInUndoList( (SCH_ITEM*) sheet, UR_CHANGED, aAppend );
        sheet->RemovePin( (SCH_SHEET_PIN*) aItem );
        m_canvas->RefreshDrawingRect( sheet->GetBoundingBox() );
    }
    else if( aItem->Type() == SCH_JUNCTION_T )
    {
        DeleteJunction( aItem, aAppend );
    }
    else
    {
        aItem->SetFlags( STRUCT_DELETED );
        SaveCopyInUndoList( aItem, UR_DELETED, aAppend );
        screen->Remove( aItem );

        std::vector< wxPoint > pts;
        aItem->GetConnectionPoints( pts );
        for( auto point : pts )
        {
            SCH_ITEM* junction = screen->GetItem( point, 0, SCH_JUNCTION_T );
            if( junction && !screen->IsJunctionNeeded( point ) )
                DeleteJunction( junction, true );
        }

        m_canvas->RefreshDrawingRect( aItem->GetBoundingBox() );
    }
}


void DuplicateItemsInList( SCH_SCREEN* screen, PICKED_ITEMS_LIST& aItemsList,
                           const wxPoint& aMoveVector )
{
    SCH_ITEM* newitem;

    if( aItemsList.GetCount() == 0 )
        return;

    // Keep trace of existing sheet paths. Duplicate block can modify this list
    bool hasSheetCopied = false;
    SCH_SHEET_LIST initial_sheetpathList( g_RootSheet );


    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        newitem = DuplicateStruct( (SCH_ITEM*) aItemsList.GetPickedItem( ii ) );
        aItemsList.SetPickedItem( newitem, ii );
        aItemsList.SetPickedItemStatus( UR_NEW, ii );
        {
            switch( newitem->Type() )
            {
            case SCH_JUNCTION_T:
            case SCH_LINE_T:
            case SCH_BUS_BUS_ENTRY_T:
            case SCH_BUS_WIRE_ENTRY_T:
            case SCH_TEXT_T:
            case SCH_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
            case SCH_HIERARCHICAL_LABEL_T:
            case SCH_SHEET_PIN_T:
            case SCH_MARKER_T:
            case SCH_NO_CONNECT_T:
            default:
                break;

            case SCH_SHEET_T:
            {
                SCH_SHEET* sheet = (SCH_SHEET*) newitem;
                // Duplicate sheet names and sheet time stamps are not valid.  Use a time stamp
                // based sheet name and update the time stamp for each sheet in the block.
                timestamp_t timeStamp = GetNewTimeStamp();

                sheet->SetName( wxString::Format( wxT( "sheet%8.8lX" ), (unsigned long)timeStamp ) );
                sheet->SetTimeStamp( timeStamp );
                hasSheetCopied = true;
                break;
            }

            case SCH_COMPONENT_T:
                ( (SCH_COMPONENT*) newitem )->SetTimeStamp( GetNewTimeStamp() );
                ( (SCH_COMPONENT*) newitem )->ClearAnnotation( NULL );
                break;
            }

            SetSchItemParent( newitem, screen );
            screen->Append( newitem );
        }
    }

    MoveItemsInList( aItemsList, aMoveVector );

    if( hasSheetCopied )
    {
        // We clear annotation of new sheet paths.
        // Annotation of new components added in current sheet is already cleared.
        SCH_SCREENS screensList( g_RootSheet );
        screensList.ClearAnnotationOfNewSheetPaths( initial_sheetpathList );
    }
}


SCH_ITEM* DuplicateStruct( SCH_ITEM* aDrawStruct, bool aClone )
{
    wxCHECK_MSG( aDrawStruct != NULL, NULL,
                 wxT( "Cannot duplicate NULL schematic item!  Bad programmer." ) );

    SCH_ITEM* NewDrawStruct = (SCH_ITEM*) aDrawStruct->Clone();

    if( aClone )
        NewDrawStruct->SetTimeStamp( aDrawStruct->GetTimeStamp() );

    return NewDrawStruct;
}
