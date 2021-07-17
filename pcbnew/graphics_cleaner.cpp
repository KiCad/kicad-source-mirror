/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <fp_shape.h>
#include <graphics_cleaner.h>


GRAPHICS_CLEANER::GRAPHICS_CLEANER( DRAWINGS& aDrawings, FOOTPRINT* aParentFootprint,
                                    BOARD_COMMIT& aCommit ) :
        m_drawings( aDrawings ),
        m_parentFootprint( aParentFootprint ),
        m_commit( aCommit ),
        m_dryRun( true ),
        m_itemsList( nullptr )
{
}


void GRAPHICS_CLEANER::CleanupBoard( bool aDryRun,
                                     std::vector<std::shared_ptr<CLEANUP_ITEM>>* aItemsList,
                                     bool aMergeRects, bool aDeleteRedundant )
{
    m_dryRun = aDryRun;
    m_itemsList = aItemsList;

    // Clear the flag used to mark some segments as deleted, in dry run:
    for( BOARD_ITEM* drawing : m_drawings )
        drawing->ClearFlags( IS_DELETED );

    if( aDeleteRedundant )
        cleanupSegments();

    if( aMergeRects )
        mergeRects();

    // Clear the flag used to mark some segments:
    for( BOARD_ITEM* drawing : m_drawings )
        drawing->ClearFlags( IS_DELETED );
}


bool GRAPHICS_CLEANER::isNullShape( PCB_SHAPE* aShape )
{
    switch( aShape->GetShape() )
    {
    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECT:
    case SHAPE_T::ARC:
        return aShape->GetStart() == aShape->GetEnd();

    case SHAPE_T::CIRCLE:
        return aShape->GetRadius() == 0;

    case SHAPE_T::POLY:
        return aShape->GetPointCount() == 0;

    case SHAPE_T::BEZIER:
        aShape->RebuildBezierToSegmentsPointsList( aShape->GetWidth() );
        return aShape->GetBezierPoints().empty();

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
    case SHAPE_T::RECT:
    case SHAPE_T::CIRCLE:
        return aShape1->GetStart() == aShape2->GetStart()
                && aShape1->GetEnd() == aShape2->GetEnd();

    case SHAPE_T::ARC:
        return aShape1->GetCenter() == aShape2->GetCenter()
                && aShape1->GetStart() == aShape2->GetStart()
                && aShape1->GetArcAngle() == aShape2->GetArcAngle();

    case SHAPE_T::POLY:
        // TODO
        return false;

    case SHAPE_T::BEZIER:
        return aShape1->GetBezierC1() == aShape2->GetBezierC1()
                && aShape1->GetBezierC2() == aShape2->GetBezierC2()
                && aShape1->GetBezierPoints() == aShape2->GetBezierPoints();

    default:
        wxFAIL_MSG( "GRAPHICS_CLEANER::areEquivalent unimplemented for "
                    + aShape1->SHAPE_T_asString() );
        return false;
    }
}


void GRAPHICS_CLEANER::cleanupSegments()
{
    // Remove duplicate segments (2 superimposed identical segments):
    for( auto it = m_drawings.begin(); it != m_drawings.end(); it++ )
    {
        PCB_SHAPE* segment = dynamic_cast<PCB_SHAPE*>( *it );

        if( !segment || segment->GetShape() != SHAPE_T::SEGMENT || segment->HasFlag( IS_DELETED ) )
            continue;

        if( isNullShape( segment ) )
        {
            std::shared_ptr<CLEANUP_ITEM> item = std::make_shared<CLEANUP_ITEM>( CLEANUP_NULL_GRAPHIC );
            item->SetItems( segment );
            m_itemsList->push_back( item );

            if( !m_dryRun )
                m_commit.Remove( segment );

            continue;
        }

        for( auto it2 = it + 1; it2 != m_drawings.end(); it2++ )
        {
            PCB_SHAPE* segment2 = dynamic_cast<PCB_SHAPE*>( *it2 );

            if( !segment2 || segment2->HasFlag( IS_DELETED ) )
                continue;

            if( areEquivalent( segment, segment2 ) )
            {
                std::shared_ptr<CLEANUP_ITEM> item = std::make_shared<CLEANUP_ITEM>( CLEANUP_DUPLICATE_GRAPHIC );
                item->SetItems( segment2 );
                m_itemsList->push_back( item );

                segment2->SetFlags( IS_DELETED );

                if( !m_dryRun )
                    m_commit.Remove( segment2 );
            }
        }
    }
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

        wxPoint    start;
        wxPoint    end;
        PCB_SHAPE* shape;
    };

    std::vector<SIDE_CANDIDATE*> sides;
    std::map<wxPoint, std::vector<SIDE_CANDIDATE*>> ptMap;

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
                    PCB_SHAPE* rect;

                    if( m_parentFootprint )
                        rect = new FP_SHAPE( m_parentFootprint );
                    else
                        rect = new PCB_SHAPE();

                    rect->SetShape( SHAPE_T::RECT );
                    rect->SetFilled( false );
                    rect->SetStart( top->start );
                    rect->SetEnd( bottom->end );
                    rect->SetLayer( top->shape->GetLayer() );
                    rect->SetWidth( top->shape->GetWidth() );

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

