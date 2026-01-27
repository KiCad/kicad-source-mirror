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


void AlignSchematicItemsToGrid( SCH_SCREEN* aScreen,
                                const std::vector<EDA_ITEM*>& aItems,
                                EE_GRID_HELPER& aGrid,
                                GRID_HELPER_GRIDS aSelectionGrid,
                                const SCH_ALIGNMENT_CALLBACKS& aCallbacks )
{
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
                // Pin already moved with the sheet (via SCH_SHEET::Move), so pin->GetPosition()
                // is the new position. We just need to calculate the additional delta to align
                // the pin to grid.
                VECTOR2I pinPos = pin->GetPosition();
                VECTOR2I delta = aGrid.AlignGrid( pinPos, aSelectionGrid ) - pinPos;

                if( delta != VECTOR2I( 0, 0 ) )
                {
                    EDA_ITEMS drag_items;

                    if( aCallbacks.m_getConnectedDragItems )
                    {
                        aCallbacks.m_getConnectedDragItems( pin, pin->GetConnectionPoints()[0],
                                                            drag_items );
                    }

                    aCallbacks.m_doMoveItem( pin, delta );

                    for( EDA_ITEM* dragItem : drag_items )
                    {
                        if( dragItem->GetParent() && dragItem->GetParent()->IsSelected() )
                            continue;

                        aCallbacks.m_doMoveItem( dragItem, delta );
                    }
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
