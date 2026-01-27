/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "sch_item_alignment.h"

#include <sch_item.h>
#include <sch_line.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_screen.h>
#include <tools/ee_grid_helper.h>

#include <map>
#include <set>


void MoveSchematicItem( EDA_ITEM* aItem, const VECTOR2I& aDelta )
{
    switch( aItem->Type() )
    {
    case SCH_LINE_T:
    {
        SCH_LINE* line = static_cast<SCH_LINE*>( aItem );

        if( aItem->HasFlag( STARTPOINT ) )
            line->MoveStart( aDelta );

        if( aItem->HasFlag( ENDPOINT ) )
            line->MoveEnd( aDelta );

        break;
    }

    case SCH_SHEET_PIN_T:
    {
        SCH_SHEET_PIN* pin = static_cast<SCH_SHEET_PIN*>( aItem );
        pin->SetStoredPos( pin->GetStoredPos() + aDelta );
        pin->ConstrainOnEdge( pin->GetStoredPos(), true );
        break;
    }

    default:
        static_cast<SCH_ITEM*>( aItem )->Move( aDelta );
        break;
    }
}


void AlignSchematicItemsToGrid( SCH_SCREEN* aScreen,
                                const std::vector<EDA_ITEM*>& aItems,
                                EE_GRID_HELPER& aGrid,
                                GRID_HELPER_GRIDS aSelectionGrid,
                                const SCH_ALIGNMENT_CALLBACKS& aCallbacks )
{
    // When both sheets are selected, wires drag with pins. When only one sheet is selected,
    // pins stay at the original Y so wires stretch horizontally without skewing. When wires
    // are also selected, they align independently and pins follow their endpoints.
    //
    // We record original wire endpoints and query the R-tree before moving each sheet, then
    // update storedPos after the move to prevent double-movement.
    struct WireEndpoint
    {
        SCH_LINE* wire;
        int       endpointFlag;  // STARTPOINT or ENDPOINT
    };
    std::map<VECTOR2I, std::vector<WireEndpoint>> pinPosToWires;

    // Wires may move while processing earlier sheets, so we save their original positions.
    struct OriginalWireEndpoints
    {
        VECTOR2I start;
        VECTOR2I end;
    };
    std::map<SCH_LINE*, OriginalWireEndpoints> originalWireEndpoints;

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_LINE_T ) )
    {
        SCH_LINE* line = static_cast<SCH_LINE*>( item );
        originalWireEndpoints[line] = { line->GetStartPoint(), line->GetEndPoint() };
    }

    std::set<VECTOR2I> selectedSheetPinPositions;

    for( EDA_ITEM* item : aItems )
    {
        if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
                selectedSheetPinPositions.insert( pin->GetPosition() );
        }
    }

    for( EDA_ITEM* item : aItems )
    {
        if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );

            if( selectedSheetPinPositions.count( line->GetStartPoint() ) )
                pinPosToWires[line->GetStartPoint()].push_back( { line, STARTPOINT } );

            if( selectedSheetPinPositions.count( line->GetEndPoint() ) )
                pinPosToWires[line->GetEndPoint()].push_back( { line, ENDPOINT } );
        }
    }

    for( EDA_ITEM* item : aItems )
    {
        if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE*             line = static_cast<SCH_LINE*>( item );
            std::vector<int>      flags{ STARTPOINT, ENDPOINT };
            std::vector<VECTOR2I> pts{ line->GetStartPoint(), line->GetEndPoint() };

            for( int ii = 0; ii < 2; ++ii )
            {
                EDA_ITEMS drag_items{ item };
                line->ClearFlags();
                line->SetFlags( SELECTED );
                line->SetFlags( flags[ii] );

                if( aCallbacks.m_getConnectedDragItems )
                    aCallbacks.m_getConnectedDragItems( line, pts[ii], drag_items );

                std::set<EDA_ITEM*> unique_items( drag_items.begin(), drag_items.end() );

                VECTOR2I delta = aGrid.AlignGrid( pts[ii], aSelectionGrid ) - pts[ii];

                if( delta != VECTOR2I( 0, 0 ) )
                {
                    for( EDA_ITEM* dragItem : unique_items )
                    {
                        if( dragItem->GetParent() && dragItem->GetParent()->IsSelected() )
                            continue;

                        aCallbacks.m_doMoveItem( dragItem, delta );
                    }
                }
            }
        }
        else if( item->Type() == SCH_FIELD_T || item->Type() == SCH_TEXT_T )
        {
            VECTOR2I delta = aGrid.AlignGrid( item->GetPosition(), aSelectionGrid )
                             - item->GetPosition();

            if( delta != VECTOR2I( 0, 0 ) )
                aCallbacks.m_doMoveItem( item, delta );
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );
            VECTOR2I   topLeft = sheet->GetPosition();
            VECTOR2I   bottomRight = topLeft + sheet->GetSize();
            VECTOR2I   tl_delta = aGrid.AlignGrid( topLeft, aSelectionGrid ) - topLeft;
            VECTOR2I   br_delta = aGrid.AlignGrid( bottomRight, aSelectionGrid ) - bottomRight;

            // Query connected items before moving the sheet since R-tree needs original positions.
            std::map<SCH_SHEET_PIN*, VECTOR2I> originalPinPositions;
            std::map<SCH_SHEET_PIN*, EDA_ITEMS> pinDragItems;

            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
            {
                originalPinPositions[pin] = pin->GetPosition();

                if( aCallbacks.m_getConnectedDragItems )
                    aCallbacks.m_getConnectedDragItems( pin, pin->GetPosition(), pinDragItems[pin] );
            }

            if( tl_delta != VECTOR2I( 0, 0 ) || br_delta != VECTOR2I( 0, 0 ) )
            {
                aCallbacks.m_doMoveItem( sheet, tl_delta );

                VECTOR2I newSize = (VECTOR2I) sheet->GetSize() - tl_delta + br_delta;
                sheet->SetSize( VECTOR2I( newSize.x, newSize.y ) );

                if( aCallbacks.m_updateItem )
                    aCallbacks.m_updateItem( sheet );
            }

            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
            {
                pin->SetStoredPos( pin->GetPosition() );
            }

            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
            {
                VECTOR2I originalPos = originalPinPositions[pin];
                VECTOR2I pinPos = pin->GetPosition();
                VECTOR2I targetPos;
                bool     canDragWires = true;

                // If the pin was connected to a selected wire, follow that wire's aligned position.
                auto it = pinPosToWires.find( originalPos );

                if( it != pinPosToWires.end() && !it->second.empty() )
                {
                    WireEndpoint& we = it->second[0];

                    if( we.endpointFlag == STARTPOINT )
                        targetPos = we.wire->GetStartPoint();
                    else
                        targetPos = we.wire->GetEndPoint();
                }
                else
                {
                    // Check if any connected wire has its other end at an unselected item.
                    for( EDA_ITEM* dragItem : pinDragItems[pin] )
                    {
                        if( dragItem->Type() != SCH_LINE_T )
                            continue;

                        SCH_LINE* wire = static_cast<SCH_LINE*>( dragItem );
                        auto      origIt = originalWireEndpoints.find( wire );

                        if( origIt == originalWireEndpoints.end() )
                            continue;

                        VECTOR2I otherEnd = ( origIt->second.start == originalPos )
                                                    ? origIt->second.end
                                                    : origIt->second.start;

                        if( selectedSheetPinPositions.find( otherEnd )
                            == selectedSheetPinPositions.end() )
                        {
                            canDragWires = false;
                            break;
                        }
                    }

                    if( !canDragWires && !pinDragItems[pin].empty() )
                    {
                        // Keep pin at original Y so the wire stretches horizontally without skewing.
                        targetPos = VECTOR2I( pinPos.x, originalPos.y );
                    }
                    else
                    {
                        targetPos = aGrid.AlignGrid( pinPos, aSelectionGrid );
                    }
                }

                VECTOR2I delta = targetPos - pinPos;
                VECTOR2I totalDelta = targetPos - originalPos;

                if( delta != VECTOR2I( 0, 0 ) )
                    aCallbacks.m_doMoveItem( pin, delta );

                for( EDA_ITEM* dragItem : pinDragItems[pin] )
                {
                    if( dragItem->GetParent() && dragItem->GetParent()->IsSelected() )
                        continue;

                    if( totalDelta != VECTOR2I( 0, 0 ) )
                        aCallbacks.m_doMoveItem( dragItem, totalDelta );
                }
            }
        }
        else
        {
            SCH_ITEM*             schItem = static_cast<SCH_ITEM*>( item );
            std::vector<VECTOR2I> connections = schItem->GetConnectionPoints();
            EDA_ITEMS             drag_items;

            if( aCallbacks.m_getConnectedDragItems )
            {
                for( const VECTOR2I& point : connections )
                    aCallbacks.m_getConnectedDragItems( schItem, point, drag_items );
            }

            std::map<VECTOR2I, int> shifts;
            VECTOR2I                most_common( 0, 0 );
            int                     max_count = 0;

            for( const VECTOR2I& conn : connections )
            {
                VECTOR2I gridpt = aGrid.AlignGrid( conn, aSelectionGrid ) - conn;

                shifts[gridpt]++;

                if( shifts[gridpt] > max_count )
                {
                    most_common = gridpt;
                    max_count = shifts[most_common];
                }
            }

            if( most_common != VECTOR2I( 0, 0 ) )
            {
                aCallbacks.m_doMoveItem( item, most_common );

                for( EDA_ITEM* dragItem : drag_items )
                {
                    if( dragItem->GetParent() && dragItem->GetParent()->IsSelected() )
                        continue;

                    aCallbacks.m_doMoveItem( dragItem, most_common );
                }
            }
        }
    }
}
