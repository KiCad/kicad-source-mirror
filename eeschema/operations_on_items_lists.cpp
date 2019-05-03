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
#include <sch_draw_panel.h>
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
#include <tool/selection.h>

void SetSchItemParent( SCH_ITEM* Struct, SCH_SCREEN* Screen )
{
    switch( Struct->Type() )
    {
    case SCH_JUNCTION_T:
    case SCH_TEXT_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
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


void SCH_EDIT_FRAME::CheckConnections( SELECTION& aSelection, bool aUndoAppend )
{
    std::vector< wxPoint > pts;
    std::vector< wxPoint > connections;

    GetSchematicConnections( connections );
    for( unsigned ii = 0; ii < aSelection.GetSize(); ii++ )
    {
        SCH_ITEM* item = static_cast<SCH_ITEM*>( aSelection.GetItem( ii ) );
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
                    aUndoAppend |= TrimWire( *point, *second_point, aUndoAppend );
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
            AddJunction( point, aUndoAppend );
            aUndoAppend = true;
        }
    }
}


void SCH_EDIT_FRAME::DeleteItem( SCH_ITEM* aItem, bool aAppend )
{
    wxCHECK_RET( aItem != NULL, wxT( "Cannot delete invalid item." ) );
    wxCHECK_RET( !( aItem->GetEditFlags() & STRUCT_DELETED ),
                 wxT( "Cannot delete item that is already deleted." ) );

    // Here, aItem is not null.
    SCH_SCREEN* screen = GetScreen();

    if( aItem->Type() == SCH_SHEET_PIN_T )
    {
        // This item is attached to its parent hierarchical sheet,
        // and is not accessible by the global list directly and cannot be removed from this list.
        SCH_SHEET* sheet = (SCH_SHEET*) aItem->GetParent();
        wxCHECK_RET( (sheet != NULL) && (sheet->Type() == SCH_SHEET_T),
                     wxT( "Sheet label has invalid parent item." ) );
        SaveCopyInUndoList( (SCH_ITEM*) sheet, UR_CHANGED, aAppend );
        sheet->RemovePin( (SCH_SHEET_PIN*) aItem );
        RefreshItem( sheet );
    }
    else if( aItem->Type() == SCH_JUNCTION_T )
    {
        DeleteJunction( aItem, aAppend );
    }
    else
    {
        aItem->SetFlags( STRUCT_DELETED );
        SaveCopyInUndoList( aItem, UR_DELETED, aAppend );
        RemoveFromScreen( aItem );

        std::vector< wxPoint > pts;
        aItem->GetConnectionPoints( pts );
        for( auto point : pts )
        {
            SCH_ITEM* junction = screen->GetItem( point, 0, SCH_JUNCTION_T );
            if( junction && !screen->IsJunctionNeeded( point ) )
                DeleteJunction( junction, true );
        }
    }
}


SCH_ITEM* DuplicateItem( SCH_ITEM* aItem, bool doClone )
{
    wxCHECK_MSG( aItem != NULL, NULL, "Cannot duplicate NULL schematic item!  Bad programmer." );

    SCH_ITEM* newItem = (SCH_ITEM*) aItem->Clone();

    if( doClone )
        newItem->SetTimeStamp( aItem->GetTimeStamp() );

    newItem->ClearFlags( SELECTED | HIGHLIGHTED | BRIGHTENED );

    if( newItem->Type() == SCH_COMPONENT_T )
    {
        for( SCH_PIN& pin : static_cast<SCH_COMPONENT*>( newItem )->GetPins() )
            pin.ClearFlags( SELECTED | HIGHLIGHTED | BRIGHTENED );

        std::vector<SCH_FIELD*> fields;
        static_cast<SCH_COMPONENT*>( newItem )->GetFields( fields, false );

        for( SCH_FIELD* field : fields )
            field->ClearFlags( SELECTED | HIGHLIGHTED | BRIGHTENED );
    }

    if( newItem->Type() == SCH_SHEET_T )
    {
        for( SCH_SHEET_PIN& pin : static_cast<SCH_SHEET*>( newItem )->GetPins() )
            pin.ClearFlags( SELECTED | HIGHLIGHTED | BRIGHTENED );
    }

    return newItem;
}
