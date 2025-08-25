/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
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

#include <reporter.h>
#include <macros.h>
#include <board_commit.h>
#include <cleanup_item.h>
#include <pcb_shape.h>
#include <pad.h>
#include <footprint.h>
#include <graphics_cleaner.h>
#include <fix_board_shape.h>
#include <board_design_settings.h>
#include <tool/tool_manager.h>
#include <tools/pad_tool.h>

GRAPHICS_CLEANER::GRAPHICS_CLEANER( const DRAWINGS& aDrawings, FOOTPRINT* aParentFootprint,
                                    BOARD_COMMIT& aCommit, TOOL_MANAGER* aToolMgr ) :
        m_drawings( aDrawings ),
        m_parentFootprint( aParentFootprint ),
        m_commit( aCommit ),
        m_toolMgr( aToolMgr ),
        m_dryRun( true ),
        m_epsilon( 1 ),
        m_maxError( ARC_HIGH_DEF ),
        m_outlinesTolerance( 0 ),
        m_itemsList( nullptr )
{
}


void GRAPHICS_CLEANER::CleanupBoard( bool                                        aDryRun,
                                     std::vector<std::shared_ptr<CLEANUP_ITEM>>* aItemsList,
                                     bool aMergeRects, bool aDeleteRedundant, bool aMergePads,
                                     bool aFixBoardOutlines, int aTolerance )
{
    m_dryRun = aDryRun;
    m_itemsList = aItemsList;
    m_outlinesTolerance = aTolerance;

    m_epsilon = m_commit.GetBoard()->GetDesignSettings().GetDRCEpsilon();
    m_maxError = m_commit.GetBoard()->GetDesignSettings().m_MaxError;

    // Clear the flag used to mark some shapes as deleted, in dry run:
    for( BOARD_ITEM* drawing : m_drawings )
        drawing->ClearFlags( IS_DELETED );

    if( aDeleteRedundant )
        cleanupShapes();

    if( aFixBoardOutlines )
        fixBoardOutlines();

    if( aMergeRects )
        mergeRects();

    if( aMergePads )
        mergePads();

    // Clear the flag used to mark some shapes:
    for( BOARD_ITEM* drawing : m_drawings )
        drawing->ClearFlags( IS_DELETED );
}


bool equivalent( const VECTOR2I& a, const VECTOR2I& b, int epsilon )
{
    return abs( a.x - b.x ) < epsilon && abs( a.y - b.y ) < epsilon;
};


bool GRAPHICS_CLEANER::isNullShape( PCB_SHAPE* aShape )
{
    switch( aShape->GetShape() )
    {
    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECTANGLE:
    case SHAPE_T::ARC:
        return equivalent( aShape->GetStart(), aShape->GetEnd(), m_epsilon );

    case SHAPE_T::CIRCLE:
        return aShape->GetRadius() == 0;

    case SHAPE_T::POLY:
        return aShape->GetPointCount() == 0;

    case SHAPE_T::BEZIER:
        aShape->RebuildBezierToSegmentsPointsList( m_maxError );

        // If the Bezier points list contains 2 points, it is equivalent to a segment
        if( aShape->GetBezierPoints().size() == 2 )
            return equivalent( aShape->GetStart(), aShape->GetEnd(), m_epsilon );

        // If the Bezier points list contains 1 points, it is equivalent to a point
        return aShape->GetBezierPoints().size() < 2;

    default:
        UNIMPLEMENTED_FOR( aShape->SHAPE_T_asString() );
        return false;
    }
}


bool GRAPHICS_CLEANER::areEquivalent( PCB_SHAPE* aShape1, PCB_SHAPE* aShape2 )
{
    if( aShape1->GetShape() != aShape2->GetShape()
            || aShape1->GetLayer() != aShape2->GetLayer()
            || aShape1->GetWidth() != aShape2->GetWidth() )
    {
        return false;
    }

    switch( aShape1->GetShape() )
    {
    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECTANGLE:
    case SHAPE_T::CIRCLE:
        return equivalent( aShape1->GetStart(), aShape2->GetStart(), m_epsilon )
                && equivalent( aShape1->GetEnd(), aShape2->GetEnd(), m_epsilon );

    case SHAPE_T::ARC:
        return equivalent( aShape1->GetCenter(), aShape2->GetCenter(), m_epsilon )
                && equivalent( aShape1->GetStart(), aShape2->GetStart(), m_epsilon )
                && equivalent( aShape1->GetEnd(), aShape2->GetEnd(), m_epsilon );

    case SHAPE_T::POLY:
        // TODO
        return false;

    case SHAPE_T::BEZIER:
        return equivalent( aShape1->GetStart(), aShape2->GetStart(), m_epsilon )
                && equivalent( aShape1->GetEnd(), aShape2->GetEnd(), m_epsilon )
                && equivalent( aShape1->GetBezierC1(), aShape2->GetBezierC1(), m_epsilon )
                && equivalent( aShape1->GetBezierC2(), aShape2->GetBezierC2(), m_epsilon );

    default:
        wxFAIL_MSG( wxT( "GRAPHICS_CLEANER::areEquivalent unimplemented for " )
                    + aShape1->SHAPE_T_asString() );
        return false;
    }
}


void GRAPHICS_CLEANER::cleanupShapes()
{
    // Remove duplicate shapes (2 superimposed identical shapes):
    for( auto it = m_drawings.begin(); it != m_drawings.end(); it++ )
    {
        PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( *it );

        if( !shape || shape->HasFlag( IS_DELETED ) )
            continue;

        if( isNullShape( shape ) )
        {
            std::shared_ptr<CLEANUP_ITEM> item = std::make_shared<CLEANUP_ITEM>( CLEANUP_NULL_GRAPHIC );
            item->SetItems( shape );
            m_itemsList->push_back( item );

            if( !m_dryRun )
                m_commit.Remove( shape );

            continue;
        }

        for( auto it2 = it + 1; it2 != m_drawings.end(); it2++ )
        {
            PCB_SHAPE* shape2 = dynamic_cast<PCB_SHAPE*>( *it2 );

            if( !shape2 || shape2->HasFlag( IS_DELETED ) )
                continue;

            if( areEquivalent( shape, shape2 ) )
            {
                std::shared_ptr<CLEANUP_ITEM> item = std::make_shared<CLEANUP_ITEM>( CLEANUP_DUPLICATE_GRAPHIC );
                item->SetItems( shape2 );
                m_itemsList->push_back( item );

                shape2->SetFlags(IS_DELETED );

                if( !m_dryRun )
                    m_commit.Remove( shape2 );
            }
        }
    }
}


void GRAPHICS_CLEANER::fixBoardOutlines()
{
    if( m_dryRun )
        return;

    std::vector<PCB_SHAPE*>                 shapeList;

    for( BOARD_ITEM* item : m_drawings )
    {
        PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item );

        if( !shape || !shape->IsOnLayer( Edge_Cuts ) )
            continue;

        shapeList.push_back( shape );

        if( !m_dryRun )
            m_commit.Modify( shape );
    }

    ConnectBoardShapes( shapeList, m_outlinesTolerance );

    std::vector<PCB_SHAPE*> items_to_select;
}


void GRAPHICS_CLEANER::mergeRects()
{
    struct SIDE_CANDIDATE
    {
        SIDE_CANDIDATE( PCB_SHAPE* aShape ) :
            start( aShape->GetStart() ),
            end( aShape->GetEnd() ),
            shape( aShape )
        {
            if( start.x > end.x || start.y > end.y )
                std::swap( start, end );
        }

        VECTOR2I   start;
        VECTOR2I   end;
        PCB_SHAPE* shape;
    };

    std::vector<SIDE_CANDIDATE*> sides;
    std::map<VECTOR2I, std::vector<SIDE_CANDIDATE*>> ptMap;

    // First load all the candidates into the side vector and layer maps
    for( BOARD_ITEM* item : m_drawings )
    {
        PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item );

        if( !shape || isNullShape( shape ) || shape->GetShape() != SHAPE_T::SEGMENT )
            continue;

        if( shape->GetStart().x == shape->GetEnd().x || shape->GetStart().y == shape->GetEnd().y )
        {
            sides.emplace_back( new SIDE_CANDIDATE( shape ) );
            ptMap[ sides.back()->start ].push_back( sides.back() );
        }
    }

    // Now go through the sides and try and match lines into rectangles
    for( SIDE_CANDIDATE* side : sides )
    {
        if( side->shape->HasFlag( IS_DELETED ) )
            continue;

        SIDE_CANDIDATE* left = nullptr;
        SIDE_CANDIDATE* top = nullptr;
        SIDE_CANDIDATE* right = nullptr;
        SIDE_CANDIDATE* bottom = nullptr;

        auto viable = [&]( SIDE_CANDIDATE* aCandidate ) -> bool
                      {
                          return aCandidate->shape->GetLayer() == side->shape->GetLayer()
                              && aCandidate->shape->GetWidth() == side->shape->GetWidth()
                              && !aCandidate->shape->HasFlag( IS_DELETED );
                      };

        if( side->start.x == side->end.x )
        {
            // We've found a possible left; see if we have a top
            //
            left = side;

            for( SIDE_CANDIDATE* candidate : ptMap[ left->start ] )
            {
                if( candidate != left && viable( candidate ) )
                {
                    top = candidate;
                    break;
                }
            }
        }
        else if( side->start.y == side->end.y )
        {
            // We've found a possible top; see if we have a left
            //
            top = side;

            for( SIDE_CANDIDATE* candidate : ptMap[ top->start ] )
            {
                if( candidate != top && viable( candidate ) )
                {
                    left = candidate;
                    break;
                }
            }
        }

        if( top && left )
        {
            // See if we can fill in the other two sides
            //
            for( SIDE_CANDIDATE* candidate : ptMap[ top->end ] )
            {
                if( candidate != top && candidate != left && viable( candidate ) )
                {
                    right = candidate;
                    break;
                }
            }

            for( SIDE_CANDIDATE* candidate : ptMap[ left->end ] )
            {
                if( candidate != top && candidate != left && viable( candidate ) )
                {
                    bottom = candidate;
                    break;
                }
            }

            if( right && bottom && right->end == bottom->end )
            {
                left->shape->SetFlags( IS_DELETED );
                top->shape->SetFlags( IS_DELETED );
                right->shape->SetFlags( IS_DELETED );
                bottom->shape->SetFlags( IS_DELETED );

                std::shared_ptr<CLEANUP_ITEM> item = std::make_shared<CLEANUP_ITEM>( CLEANUP_LINES_TO_RECT );
                item->SetItems( left->shape, top->shape, right->shape, bottom->shape );
                m_itemsList->push_back( item );

                if( !m_dryRun )
                {
                    PCB_SHAPE* rect = new PCB_SHAPE( m_parentFootprint );

                    rect->SetShape( SHAPE_T::RECTANGLE );
                    rect->SetFilled( false );
                    rect->SetStart( top->start );
                    rect->SetEnd( bottom->end );
                    rect->SetLayer( top->shape->GetLayer() );
                    rect->SetStroke( top->shape->GetStroke() );

                    m_commit.Add( rect );
                    m_commit.Remove( left->shape );
                    m_commit.Remove( top->shape );
                    m_commit.Remove( right->shape );
                    m_commit.Remove( bottom->shape );
                }
            }
        }
    }

    for( SIDE_CANDIDATE* side : sides )
        delete side;
}


void GRAPHICS_CLEANER::mergePads()
{
    wxCHECK_MSG( m_parentFootprint, /*void*/, wxT( "mergePads() is FootprintEditor only" ) );

    PAD_TOOL*               padTool = m_toolMgr->GetTool<PAD_TOOL>();
    std::map<wxString, int> padToNetTieGroupMap = m_parentFootprint->MapPadNumbersToNetTieGroups();

    for( PAD* pad : m_parentFootprint->Pads() )
    {
        // Don't merge a pad that's in a net-tie pad group.  (We don't care which group.)
        if( padToNetTieGroupMap[ pad->GetNumber() ] >= 0 )
            continue;

        if( m_commit.GetStatus( m_parentFootprint ) == 0 )
            m_commit.Modify( m_parentFootprint );

        std::vector<PCB_SHAPE*> shapes = padTool->RecombinePad( pad, m_dryRun );

        if( !shapes.empty() )
        {
            std::shared_ptr<CLEANUP_ITEM> item = std::make_shared<CLEANUP_ITEM>( CLEANUP_MERGE_PAD );

            for( PCB_SHAPE* shape : shapes )
                item->AddItem( shape );

            item->AddItem( pad );

            m_itemsList->push_back( item );
        }
    }
}
