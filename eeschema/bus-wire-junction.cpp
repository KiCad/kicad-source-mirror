/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 2004-2023 KiCad Developers, see change_log.txt for contributors.
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
#include <sch_screen.h>
#include <sch_view.h>
#include <sch_commit.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>
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
        BreakSegment( aCommit, line, aStart, &new_line, screen );

        if( IsPointOnSegment( new_line->GetStartPoint(), new_line->GetEndPoint(), aEnd ) )
            line = new_line;

        // Step 2: break the remaining segment.
        // Ensure that *line _also_ contains aStart.  This is our overlapping segment
        BreakSegment( aCommit, line, aEnd, &new_line, screen );

        if( IsPointOnSegment( new_line->GetStartPoint(), new_line->GetEndPoint(), aStart ) )
            line = new_line;

        RemoveFromScreen( line, screen );
        aCommit->Removed( line, screen );

        return true;
    }

    return false;
}


void SCH_EDIT_FRAME::SchematicCleanUp( SCH_COMMIT* aCommit, SCH_SCREEN* aScreen )
{
    EE_SELECTION_TOOL*           selectionTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
    std::vector<SCH_LINE*>       lines;
    std::vector<SCH_JUNCTION*>   junctions;
    std::vector<SCH_NO_CONNECT*> ncs;
    bool                         changed = true;

    if( aScreen == nullptr )
        aScreen = GetScreen();

    auto remove_item = [&]( SCH_ITEM* aItem ) -> void
                       {
                           changed = true;

                           if( !( aItem->GetFlags() & STRUCT_DELETED ) )
                           {
                               aItem->SetFlags( STRUCT_DELETED );

                               if( aItem->IsSelected() )
                                   selectionTool->RemoveItemFromSel( aItem, true /*quiet mode*/ );

                               RemoveFromScreen( aItem, aScreen );
                               aCommit->Removed( aItem, aScreen );
                           }
                       };

    BreakSegmentsOnJunctions( aCommit, aScreen );

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_JUNCTION_T ) )
    {
        if( !aScreen->IsExplicitJunction( item->GetPosition() ) )
            remove_item( item );
        else
            junctions.push_back( static_cast<SCH_JUNCTION*>( item ) );
    }

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_NO_CONNECT_T ) )
        ncs.push_back( static_cast<SCH_NO_CONNECT*>( item ) );

    alg::for_all_pairs( junctions.begin(), junctions.end(),
            [&]( SCH_JUNCTION* aFirst, SCH_JUNCTION* aSecond )
            {
                if( ( aFirst->GetEditFlags() & STRUCT_DELETED )
                        || ( aSecond->GetEditFlags() & STRUCT_DELETED ) )
                {
                    return;
                }

                if( aFirst->GetPosition() == aSecond->GetPosition() )
                    remove_item( aSecond );
            } );

    alg::for_all_pairs( ncs.begin(), ncs.end(),
            [&]( SCH_NO_CONNECT* aFirst, SCH_NO_CONNECT* aSecond )
            {
                if( ( aFirst->GetEditFlags() & STRUCT_DELETED )
                        || ( aSecond->GetEditFlags() & STRUCT_DELETED ) )
                {
                    return;
                }

                if( aFirst->GetPosition() == aSecond->GetPosition() )
                    remove_item( aSecond );
            } );


    while( changed )
    {
        changed = false;
        lines.clear();

        for( SCH_ITEM* item : aScreen->Items().OfType( SCH_LINE_T ) )
        {
            if( item->GetLayer() == LAYER_WIRE || item->GetLayer() == LAYER_BUS )
                lines.push_back( static_cast<SCH_LINE*>( item ) );
        }

        for( auto it1 = lines.begin(); it1 != lines.end(); ++it1 )
        {
            SCH_LINE* firstLine = *it1;

            if( firstLine->GetEditFlags() & STRUCT_DELETED )
                continue;

            if( firstLine->IsNull() )
            {
                remove_item( firstLine );
                continue;
            }

            auto it2 = it1;

            for( ++it2; it2 != lines.end(); ++it2 )
            {
                SCH_LINE* secondLine = *it2;

                if( secondLine->GetFlags() & STRUCT_DELETED )
                    continue;

                if( !secondLine->IsParallel( firstLine )
                        || !secondLine->IsStrokeEquivalent( firstLine )
                        || secondLine->GetLayer() != firstLine->GetLayer() )
                {
                    continue;
                }

                // Remove identical lines
                if( firstLine->IsEndPoint( secondLine->GetStartPoint() )
                        && firstLine->IsEndPoint( secondLine->GetEndPoint() ) )
                {
                    remove_item( secondLine );
                    continue;
                }

                // See if we can merge an overlap (or two colinear touching segments with
                // no junction where they meet).
                SCH_LINE* mergedLine = secondLine->MergeOverlap( aScreen, firstLine, true );

                if( mergedLine != nullptr )
                {
                    remove_item( firstLine );
                    remove_item( secondLine );

                    AddToScreen( mergedLine, aScreen );
                    aCommit->Added( mergedLine, aScreen );

                    if( firstLine->IsSelected() || secondLine->IsSelected() )
                        selectionTool->AddItemToSel( mergedLine, true /*quiet mode*/ );

                    break;
                }
            }
        }
    }
}


void SCH_EDIT_FRAME::BreakSegment( SCH_COMMIT* aCommit, SCH_LINE* aSegment, const VECTOR2I& aPoint,
                                   SCH_LINE** aNewSegment, SCH_SCREEN* aScreen )
{
    // Save the copy of aSegment before breaking it
    aCommit->Modify( aSegment, aScreen );

    SCH_LINE* newSegment = aSegment->BreakAt( aPoint );

    aSegment->SetFlags( IS_CHANGED | IS_BROKEN );
    newSegment->SetFlags( IS_NEW | IS_BROKEN );

    AddToScreen( newSegment, aScreen );
    aCommit->Added( newSegment, aScreen );

    *aNewSegment = newSegment;
}


bool SCH_EDIT_FRAME::BreakSegments( SCH_COMMIT* aCommit, const VECTOR2I& aPos, SCH_SCREEN* aScreen )
{
    bool      brokenSegments = false;
    SCH_LINE* new_line;

    for( SCH_LINE* wire : aScreen->GetBusesAndWires( aPos, true ) )
    {
        BreakSegment( aCommit, wire, aPos, &new_line, aScreen );
        brokenSegments = true;
    }

    return brokenSegments;
}


bool SCH_EDIT_FRAME::BreakSegmentsOnJunctions( SCH_COMMIT* aCommit, SCH_SCREEN* aScreen )
{
    bool brokenSegments = false;

    std::set<VECTOR2I> point_set;

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_JUNCTION_T ) )
        point_set.insert( item->GetPosition() );

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_BUS_WIRE_ENTRY_T ) )
    {
        SCH_BUS_WIRE_ENTRY* entry = static_cast<SCH_BUS_WIRE_ENTRY*>( item );
        point_set.insert( entry->GetPosition() );
        point_set.insert( entry->GetEnd() );
    }

    for( const VECTOR2I& pt : point_set )
    {
        BreakSegments( aCommit, pt, aScreen );
        brokenSegments = true;
    }

    return brokenSegments;
}


void SCH_EDIT_FRAME::DeleteJunction( SCH_COMMIT* aCommit, SCH_ITEM* aJunction )
{
    SCH_SCREEN*        screen = GetScreen();
    PICKED_ITEMS_LIST  undoList;
    EE_SELECTION_TOOL* selectionTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();

    aJunction->SetFlags( STRUCT_DELETED );
    RemoveFromScreen( aJunction, screen );
    aCommit->Removed( aJunction, screen );

    /// Note that std::list or similar is required here as we may insert values in the
    /// loop below.  This will invalidate iterators in a std::vector or std::deque
    std::list<SCH_LINE*> lines;

    for( SCH_ITEM* item : screen->Items().Overlapping( SCH_LINE_T, aJunction->GetPosition() ) )
    {
        SCH_LINE* line = static_cast<SCH_LINE*>( item );

        if( line->IsType( { SCH_ITEM_LOCATE_WIRE_T, SCH_ITEM_LOCATE_BUS_T } )
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


SCH_JUNCTION* SCH_EDIT_FRAME::AddJunction( SCH_COMMIT* aCommit, SCH_SCREEN* aScreen,
                                           const VECTOR2I& aPos )
{
    SCH_JUNCTION* junction = new SCH_JUNCTION( aPos );

    AddToScreen( junction, aScreen );
    aCommit->Added( junction, aScreen );

    BreakSegments( aCommit, aPos, aScreen );

    return junction;
}


