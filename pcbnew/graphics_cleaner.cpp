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

#include <fctsys.h>
#include <reporter.h>
#include <board_commit.h>
#include <cleanup_item.h>
#include <class_drawsegment.h>
#include <class_edge_mod.h>
#include <graphics_cleaner.h>


GRAPHICS_CLEANER::GRAPHICS_CLEANER( DRAWINGS& aDrawings, MODULE* aParentModule,
                                    BOARD_COMMIT& aCommit ) :
        m_drawings( aDrawings ),
        m_parentModule( aParentModule ),
        m_commit( aCommit ),
        m_dryRun( true ),
        m_itemsList( nullptr )
{
}


void GRAPHICS_CLEANER::CleanupBoard( bool aDryRun, std::vector<CLEANUP_ITEM*>* aItemsList,
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


bool GRAPHICS_CLEANER::isNullSegment( DRAWSEGMENT* aSegment )
{
    switch( aSegment->GetShape() )
    {
    case S_SEGMENT:
    case S_RECT:
        return aSegment->GetStart() == aSegment->GetEnd();

    case S_CIRCLE:
        return aSegment->GetRadius() == 0;

    case S_ARC:
        return aSegment->GetCenter().x == aSegment->GetArcStart().x
                   && aSegment->GetCenter().y == aSegment->GetArcStart().y;

    case S_POLYGON:
        return aSegment->GetPointCount() == 0;

    case S_CURVE:
        aSegment->RebuildBezierToSegmentsPointsList( aSegment->GetWidth() );
        return aSegment->GetBezierPoints().empty();

    default:
        wxFAIL_MSG( wxString::Format( "unknown DRAWSEGMENT shape: %d", aSegment->GetShape() ) );
        return false;
    }
}


bool GRAPHICS_CLEANER::areEquivalent( DRAWSEGMENT* aSegment1, DRAWSEGMENT* aSegment2 )
{
    if( aSegment1->GetShape() != aSegment2->GetShape()
            || aSegment1->GetLayer() != aSegment2->GetLayer()
            || aSegment1->GetWidth() != aSegment2->GetWidth() )
    {
        return false;
    }

    switch( aSegment1->GetShape() )
    {
    case S_SEGMENT:
    case S_RECT:
    case S_CIRCLE:
        return aSegment1->GetStart() == aSegment2->GetStart()
                && aSegment1->GetEnd() == aSegment2->GetEnd();

    case S_ARC:
        return aSegment1->GetCenter() == aSegment2->GetCenter()
                && aSegment1->GetArcStart() == aSegment2->GetArcStart()
                && aSegment1->GetAngle() == aSegment2->GetAngle();

    case S_POLYGON:
        // TODO
        return false;

    case S_CURVE:
        return aSegment1->GetBezControl1() == aSegment2->GetBezControl1()
                && aSegment1->GetBezControl2() == aSegment2->GetBezControl2()
                && aSegment1->GetBezierPoints() == aSegment2->GetBezierPoints();

    default:
        wxFAIL_MSG( wxString::Format( "unknown DRAWSEGMENT shape: %d", aSegment1->GetShape() ) );
        return false;
    }
}


void GRAPHICS_CLEANER::cleanupSegments()
{
    std::set<BOARD_ITEM*> toRemove;

    // Remove duplicate segments (2 superimposed identical segments):
    for( auto it = m_drawings.begin(); it != m_drawings.end(); it++ )
    {
        DRAWSEGMENT* segment = dynamic_cast<DRAWSEGMENT*>( *it );

        if( !segment || segment->GetShape() != S_SEGMENT || segment->HasFlag( IS_DELETED ) )
            continue;

        if( isNullSegment( segment ) )
        {
            CLEANUP_ITEM* item = new CLEANUP_ITEM( CLEANUP_NULL_GRAPHIC );
            item->SetItems( segment );
            m_itemsList->push_back( item );

            toRemove.insert( segment );
            continue;
        }

        for( auto it2 = it + 1; it2 != m_drawings.end(); it2++ )
        {
            DRAWSEGMENT* segment2 = dynamic_cast<DRAWSEGMENT*>( *it2 );

            if( !segment2 || segment2->HasFlag( IS_DELETED ) )
                continue;

            if( areEquivalent( segment, segment2 ) )
            {
                CLEANUP_ITEM* item = new CLEANUP_ITEM( CLEANUP_DUPLICATE_GRAPHIC );
                item->SetItems( segment2 );
                m_itemsList->push_back( item );

                segment2->SetFlags( IS_DELETED );
                toRemove.insert( segment2 );
            }
        }
    }

    if( !m_dryRun )
        removeItems( toRemove );
}


void GRAPHICS_CLEANER::mergeRects()
{
    struct SIDE_CANDIDATE
    {
        SIDE_CANDIDATE( DRAWSEGMENT* aSeg ) :
            start( aSeg->GetStart() ),
            end( aSeg->GetEnd() ),
            seg( aSeg )
        {
            if( start.x > end.x || start.y > end.y )
                std::swap( start, end );
        }

        wxPoint start;
        wxPoint end;
        DRAWSEGMENT* seg;
    };

    std::set<BOARD_ITEM*> toRemove;

    auto markForRemoval = [&]( SIDE_CANDIDATE* aSide )
                          {
                              toRemove.insert( aSide->seg );
                              aSide->seg->SetFlags( IS_DELETED );
                          };

    std::vector<SIDE_CANDIDATE*> sides;
    std::map<wxPoint, std::vector<SIDE_CANDIDATE*>> ptMap;

    // First load all the candidates into the side vector and layer maps
    for( BOARD_ITEM* item : m_drawings )
    {
        DRAWSEGMENT* seg = dynamic_cast<DRAWSEGMENT*>( item );

        if( !seg || seg->GetShape() != S_SEGMENT )
            continue;

        if( seg->GetStart().x == seg->GetEnd().x || seg->GetStart().y == seg->GetEnd().y )
        {
            sides.emplace_back( new SIDE_CANDIDATE( seg ) );
            ptMap[ sides.back()->start ].push_back( sides.back() );
        }
    }

    // Now go through the sides and try and match lines into rectangles
    for( SIDE_CANDIDATE* side : sides )
    {
        if( side->seg->HasFlag( IS_DELETED ) )
            continue;

        SIDE_CANDIDATE* left = nullptr;
        SIDE_CANDIDATE* top = nullptr;
        SIDE_CANDIDATE* right = nullptr;
        SIDE_CANDIDATE* bottom = nullptr;

        auto viable = [&]( SIDE_CANDIDATE* aCandidate ) -> bool
                      {
                          return aCandidate->seg->GetLayer() == side->seg->GetLayer()
                              && aCandidate->seg->GetWidth() == side->seg->GetWidth()
                              && !aCandidate->seg->HasFlag( IS_DELETED );
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
                if( candidate != top && viable( candidate ) )
                {
                    right = candidate;
                    break;
                }
            }

            for( SIDE_CANDIDATE* candidate : ptMap[ left->end ] )
            {
                if( candidate != left && viable( candidate ) )
                {
                    bottom = candidate;
                    break;
                }
            }

            if( right && bottom )
            {
                markForRemoval( left );
                markForRemoval( top );
                markForRemoval( right );
                markForRemoval( bottom );

                CLEANUP_ITEM* item = new CLEANUP_ITEM( CLEANUP_LINES_TO_RECT );
                item->SetItems( left->seg, top->seg, right->seg, bottom->seg );
                m_itemsList->push_back( item );

                if( !m_dryRun )
                {
                    DRAWSEGMENT* rect;

                    if( m_parentModule )
                        rect = new EDGE_MODULE( m_parentModule );
                    else
                        rect = new DRAWSEGMENT();

                    rect->SetShape( S_RECT );
                    rect->SetStart( top->start );
                    rect->SetEnd( bottom->end );
                    rect->SetLayer( top->seg->GetLayer() );
                    rect->SetWidth( top->seg->GetWidth() );

                    m_commit.Add( rect );
                }
            }
        }
    }

    if( !m_dryRun )
        removeItems( toRemove );

    for( SIDE_CANDIDATE* side : sides )
        delete side;
}


void GRAPHICS_CLEANER::removeItems( std::set<BOARD_ITEM*>& aItems )
{
    for( BOARD_ITEM* item : aItems )
    {
        if( m_parentModule )
            m_parentModule->Remove( item );
        else
            item->GetParent()->Remove( item );

        m_commit.Removed( item );
    }
}
