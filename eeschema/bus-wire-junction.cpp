/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.fr
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

#include <core/kicad_algo.h>
#include <general.h>
#include <sch_bus_entry.h>
#include <sch_edit_frame.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_commit.h>
#include <tool/tool_manager.h>
#include <tools/sch_line_wire_bus_tool.h>
#include <tools/sch_selection_tool.h>
#include <trigo.h>


void SCH_EDIT_FRAME::TestDanglingEnds()
{
    std::function<void( SCH_ITEM* )> changeHandler =
            [&]( SCH_ITEM* aChangedItem ) -> void
            {
                GetCanvas()->GetView()->Update( aChangedItem, KIGFX::REPAINT );
            };

    GetScreen()->TestDanglingEnds( nullptr, &changeHandler );
}


bool SCH_EDIT_FRAME::TrimWire( SCH_COMMIT* aCommit, const VECTOR2I& aStart, const VECTOR2I& aEnd )
{
    if( aStart == aEnd )
        return false;

    SCH_SCREEN*            screen = GetScreen();
    std::vector<SCH_LINE*> wires;
    BOX2I                  bb( aStart );

    SCH_LINE_WIRE_BUS_TOOL* lwbTool = m_toolManager->GetTool<SCH_LINE_WIRE_BUS_TOOL>();
    bb.Merge( aEnd );

    // We cannot modify the RTree while iterating, so push the possible
    // wires into a separate structure.
    for( EDA_ITEM* item : screen->Items().Overlapping( bb ) )
    {
        SCH_LINE* line = static_cast<SCH_LINE*>( item );

        if( item->Type() == SCH_LINE_T && line->GetLayer() == LAYER_WIRE )
            wires.push_back( line );
    }

    for( SCH_LINE* line : wires )
    {
        // Don't remove wires that are already deleted or are currently being dragged
        if( line->GetEditFlags() & ( STRUCT_DELETED | IS_MOVING | SKIP_STRUCT ) )
            continue;

        if( !IsPointOnSegment( line->GetStartPoint(), line->GetEndPoint(), aStart ) ||
                !IsPointOnSegment( line->GetStartPoint(), line->GetEndPoint(), aEnd ) )
        {
            continue;
        }

        // Don't remove entire wires
        if( ( line->GetStartPoint() == aStart && line->GetEndPoint() == aEnd )
            || ( line->GetStartPoint() == aEnd && line->GetEndPoint() == aStart ) )
        {
            continue;
        }

        // Step 1: break the segment on one end.
        // Ensure that *line points to the segment containing aEnd
        SCH_LINE* new_line;
        lwbTool->BreakSegment( aCommit, line, aStart, &new_line, screen );

        if( IsPointOnSegment( new_line->GetStartPoint(), new_line->GetEndPoint(), aEnd ) )
            line = new_line;

        // Step 2: break the remaining segment.
        // Ensure that *line _also_ contains aStart.  This is our overlapping segment
        lwbTool->BreakSegment( aCommit, line, aEnd, &new_line, screen );

        if( IsPointOnSegment( new_line->GetStartPoint(), new_line->GetEndPoint(), aStart ) )
            line = new_line;

        RemoveFromScreen( line, screen );
        aCommit->Removed( line, screen );

        return true;
    }

    return false;
}


void SCH_EDIT_FRAME::DeleteJunction( SCH_COMMIT* aCommit, SCH_ITEM* aJunction )
{
    SCH_SCREEN*         screen = GetScreen();
    PICKED_ITEMS_LIST   undoList;
    SCH_SELECTION_TOOL* selectionTool = m_toolManager->GetTool<SCH_SELECTION_TOOL>();

    aJunction->SetFlags( STRUCT_DELETED );
    RemoveFromScreen( aJunction, screen );
    aCommit->Removed( aJunction, screen );

    /// Note that std::list or similar is required here as we may insert values in the
    /// loop below.  This will invalidate iterators in a std::vector or std::deque
    std::list<SCH_LINE*> lines;

    for( SCH_ITEM* item : screen->Items().Overlapping( SCH_LINE_T, aJunction->GetPosition() ) )
    {
        SCH_LINE* line = static_cast<SCH_LINE*>( item );

        if( ( line->IsWire() || line->IsBus() )
                && line->IsEndPoint( aJunction->GetPosition() )
                && !( line->GetEditFlags() & STRUCT_DELETED ) )
        {
            lines.push_back( line );
        }
    }

    alg::for_all_pairs( lines.begin(), lines.end(),
            [&]( SCH_LINE* firstLine, SCH_LINE* secondLine )
            {
                if( ( firstLine->GetEditFlags() & STRUCT_DELETED )
                        || ( secondLine->GetEditFlags() & STRUCT_DELETED )
                        || !secondLine->IsParallel( firstLine ) )
                {
                    return;
                }

                // Remove identical lines
                if( firstLine->IsEndPoint( secondLine->GetStartPoint() )
                        && firstLine->IsEndPoint( secondLine->GetEndPoint() ) )
                {
                    firstLine->SetFlags( STRUCT_DELETED );
                    return;
                }

                // Try to merge the remaining lines
                if( SCH_LINE* new_line = secondLine->MergeOverlap( screen, firstLine, false ) )
                {
                    firstLine->SetFlags( STRUCT_DELETED );
                    secondLine->SetFlags( STRUCT_DELETED );
                    AddToScreen( new_line, screen );
                    aCommit->Added( new_line, screen );

                    if( new_line->IsSelected() )
                        selectionTool->AddItemToSel( new_line, true /*quiet mode*/ );

                    lines.push_back( new_line );
                }
            } );

    for( SCH_LINE* line : lines )
    {
        if( line->GetEditFlags() & STRUCT_DELETED )
        {
            if( line->IsSelected() )
                selectionTool->RemoveItemFromSel( line, true /*quiet mode*/ );

            RemoveFromScreen( line, screen );
            aCommit->Removed( line, screen );
        }
    }
}

void SCH_EDIT_FRAME::UpdateHopOveredWires( SCH_ITEM* aItem )
{
    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> items;

    GetCanvas()->GetView()->Query( aItem->GetBoundingBox(), items );

    for( const auto& it : items )
    {
        if( !it.first->IsSCH_ITEM() )
            continue;

        SCH_ITEM* item = static_cast<SCH_ITEM*>( it.first );

        if( item == aItem )
            continue;

        if( item->IsType( { SCH_ITEM_LOCATE_WIRE_T } ) )
        {
            GetCanvas()->GetView()->Update( item );
        }
    }
}
