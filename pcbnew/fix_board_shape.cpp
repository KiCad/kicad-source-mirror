/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#include "fix_board_shape.h"

#include <vector>
#include <algorithm>
#include <pcb_shape.h>
#include <geometry/circle.h>


/**
 * Searches for a PCB_SHAPE matching a given end point or start point in a list.
 * @param aShape The starting shape.
 * @param aPoint The starting or ending point to search for.
 * @param aList The list to remove from.
 * @param aLimit is the distance from \a aPoint that still constitutes a valid find.
 * @return PCB_SHAPE* - The first PCB_SHAPE that has a start or end point matching
 *   aPoint, otherwise NULL if none.
 */
static PCB_SHAPE* findNext( PCB_SHAPE* aShape, const VECTOR2I& aPoint,
                            const std::vector<PCB_SHAPE*>& aList, unsigned aLimit )
{
    // Look for an unused, exact hit
    for( PCB_SHAPE* graphic : aList )
    {
        if( graphic == aShape || ( graphic->GetFlags() & SKIP_STRUCT ) != 0 )
            continue;

        if( aPoint == graphic->GetStart() || aPoint == graphic->GetEnd() )
            return graphic;
    }

    // Search again for anything that's close.
    VECTOR2I    pt( aPoint );
    SEG::ecoord closest_dist_sq = SEG::Square( aLimit );
    PCB_SHAPE*  closest_graphic = nullptr;
    SEG::ecoord d_sq;

    for( PCB_SHAPE* graphic : aList )
    {
        if( graphic == aShape || ( graphic->GetFlags() & SKIP_STRUCT ) != 0 )
            continue;

        d_sq = ( pt - graphic->GetStart() ).SquaredEuclideanNorm();

        if( d_sq < closest_dist_sq )
        {
            closest_dist_sq = d_sq;
            closest_graphic = graphic;
        }

        d_sq = ( pt - graphic->GetEnd() ).SquaredEuclideanNorm();

        if( d_sq < closest_dist_sq )
        {
            closest_dist_sq = d_sq;
            closest_graphic = graphic;
        }
    }

    return closest_graphic; // Note: will be nullptr if nothing within aLimit
}


void ConnectBoardShapes( std::vector<PCB_SHAPE*>&                 aShapeList,
                         std::vector<std::unique_ptr<PCB_SHAPE>>& aNewShapes, int aChainingEpsilon )
{
    if( aShapeList.size() == 0 )
        return;

    (void) aNewShapes;

#if 0
    // Not used, but not removed, just in case
    auto close_enough = []( const VECTOR2I& aLeft, const VECTOR2I& aRight, unsigned aLimit ) -> bool
    {
        return ( aLeft - aRight ).SquaredEuclideanNorm() <= SEG::Square( aLimit );
    };
#endif

    auto closer_to_first = []( const VECTOR2I& aRef, const VECTOR2I& aFirst,
                               const VECTOR2I& aSecond ) -> bool
    {
        return ( aRef - aFirst ).SquaredEuclideanNorm() < ( aRef - aSecond ).SquaredEuclideanNorm();
    };

    auto min_distance_sq = []( const VECTOR2I& aRef, const VECTOR2I& aFirst,
                               const VECTOR2I& aSecond ) -> SEG::ecoord
    {
        return std::min( ( aRef - aFirst ).SquaredEuclideanNorm(),
                         ( aRef - aSecond ).SquaredEuclideanNorm() );
    };

    auto connectPair = [&]( PCB_SHAPE* aPrevShape, PCB_SHAPE* aShape )
    {
        bool success = false;

        SHAPE_T shape0 = aPrevShape->GetShape();
        SHAPE_T shape1 = aShape->GetShape();

        if( shape0 == SHAPE_T::SEGMENT && shape1 == SHAPE_T::SEGMENT )
        {
            SEG seg0( aPrevShape->GetStart(), aPrevShape->GetEnd() );
            SEG seg1( aShape->GetStart(), aShape->GetEnd() );
            SEG::ecoord d[4];
            d[0] = ( seg0.A - seg1.A ).SquaredEuclideanNorm();
            d[1] = ( seg0.A - seg1.B ).SquaredEuclideanNorm();
            d[2] = ( seg0.B - seg1.A ).SquaredEuclideanNorm();
            d[3] = ( seg0.B - seg1.B ).SquaredEuclideanNorm();

            int idx = std::min_element( d, d + 4 ) - d;
            int i0 = idx / 2;
            int i1 = idx % 2;

            if( seg0.Intersects( seg1 ) || seg0.Angle( seg1 ) > ANGLE_45 )
            {
                if( OPT_VECTOR2I inter = seg0.IntersectLines( seg1 ) )
                {
                    if( i0 == 0 )
                        aPrevShape->SetStart( *inter );
                    else
                        aPrevShape->SetEnd( *inter );

                    if( i1 == 0 )
                        aShape->SetStart( *inter );
                    else
                        aShape->SetEnd( *inter );

                    success = true;
                }
            }
        }
        else if( ( shape0 == SHAPE_T::ARC && shape1 == SHAPE_T::SEGMENT )
                 || ( shape0 == SHAPE_T::SEGMENT && shape1 == SHAPE_T::ARC ) )
        {
            PCB_SHAPE* arcShape = shape0 == SHAPE_T::ARC ? aPrevShape : aShape;
            PCB_SHAPE* segShape = shape0 == SHAPE_T::SEGMENT ? aPrevShape : aShape;

            VECTOR2I arcPts[2] = { arcShape->GetStart(), arcShape->GetEnd() };
            VECTOR2I segPts[2] = { segShape->GetStart(), segShape->GetEnd() };

            SEG::ecoord d[4];
            d[0] = ( segPts[0] - arcPts[0] ).SquaredEuclideanNorm();
            d[1] = ( segPts[0] - arcPts[1] ).SquaredEuclideanNorm();
            d[2] = ( segPts[1] - arcPts[0] ).SquaredEuclideanNorm();
            d[3] = ( segPts[1] - arcPts[1] ).SquaredEuclideanNorm();

            int idx = std::min_element( d, d + 4 ) - d;

            switch( idx )
            {
            case 0: segShape->SetStart( arcPts[0] ); break;
            case 1: segShape->SetStart( arcPts[1] ); break;
            case 2: segShape->SetEnd( arcPts[0] ); break;
            case 3: segShape->SetEnd( arcPts[1] ); break;
            }

            success = true;
        }
        else if( shape0 == SHAPE_T::ARC && shape1 == SHAPE_T::ARC )
        {
            PCB_SHAPE* arc0 = aPrevShape;
            PCB_SHAPE* arc1 = aShape;

            VECTOR2I pts0[2] = { arc0->GetStart(), arc0->GetEnd() };
            VECTOR2I pts1[2] = { arc1->GetStart(), arc1->GetEnd() };

            SEG::ecoord d[4];
            d[0] = ( pts0[0] - pts1[0] ).SquaredEuclideanNorm();
            d[1] = ( pts0[0] - pts1[1] ).SquaredEuclideanNorm();
            d[2] = ( pts0[1] - pts1[0] ).SquaredEuclideanNorm();
            d[3] = ( pts0[1] - pts1[1] ).SquaredEuclideanNorm();

            int idx = std::min_element( d, d + 4 ) - d;
            int i0 = idx / 2;
            int i1 = idx % 2;
            VECTOR2I middle = ( pts0[i0] + pts1[i1] ) / 2;

            if( i0 == 0 )
                arc0->SetArcGeometry( middle, arc0->GetArcMid(), arc0->GetEnd() );
            else
                arc0->SetArcGeometry( arc0->GetStart(), arc0->GetArcMid(), middle );

            if( i1 == 0 )
                arc1->SetArcGeometry( middle, arc1->GetArcMid(), arc1->GetEnd() );
            else
                arc1->SetArcGeometry( arc1->GetStart(), arc1->GetArcMid(), middle );

            success = true;
        }
        else if( ( shape0 == SHAPE_T::BEZIER && shape1 == SHAPE_T::ARC )
                 || ( shape0 == SHAPE_T::ARC && shape1 == SHAPE_T::BEZIER ) )
        {
            PCB_SHAPE* bezShape = shape0 == SHAPE_T::BEZIER ? aPrevShape : aShape;
            PCB_SHAPE* arcShape = shape0 == SHAPE_T::ARC ? aPrevShape : aShape;

            VECTOR2I bezPts[2] = { bezShape->GetStart(), bezShape->GetEnd() };
            VECTOR2I arcPts[2] = { arcShape->GetStart(), arcShape->GetEnd() };

            SEG::ecoord d[4];
            d[0] = ( bezPts[0] - arcPts[0] ).SquaredEuclideanNorm();
            d[1] = ( bezPts[0] - arcPts[1] ).SquaredEuclideanNorm();
            d[2] = ( bezPts[1] - arcPts[0] ).SquaredEuclideanNorm();
            d[3] = ( bezPts[1] - arcPts[1] ).SquaredEuclideanNorm();

            int idx = std::min_element( d, d + 4 ) - d;

            switch( idx )
            {
            case 0:
            {
                VECTOR2I delta = arcPts[0] - bezPts[0];
                bezShape->SetStart( arcPts[0] );
                bezShape->SetBezierC1( bezShape->GetBezierC1() + delta );
                break;
            }
            case 1:
            {
                VECTOR2I delta = arcPts[1] - bezPts[0];
                bezShape->SetStart( arcPts[1] );
                bezShape->SetBezierC1( bezShape->GetBezierC1() + delta );
                break;
            }
            case 2:
            {
                VECTOR2I delta = arcPts[0] - bezPts[1];
                bezShape->SetEnd( arcPts[0] );
                bezShape->SetBezierC2( bezShape->GetBezierC2() + delta );
                break;
            }
            case 3:
            {
                VECTOR2I delta = arcPts[1] - bezPts[1];
                bezShape->SetEnd( arcPts[1] );
                bezShape->SetBezierC2( bezShape->GetBezierC2() + delta );
                break;
            }
            }

            success = true;
        }
        else if( ( shape0 == SHAPE_T::BEZIER && shape1 == SHAPE_T::SEGMENT )
                 || ( shape0 == SHAPE_T::SEGMENT && shape1 == SHAPE_T::BEZIER ) )
        {
            PCB_SHAPE* bezShape = shape0 == SHAPE_T::BEZIER ? aPrevShape : aShape;
            PCB_SHAPE* segShape = shape0 == SHAPE_T::SEGMENT ? aPrevShape : aShape;

            VECTOR2I bezPts[2] = { bezShape->GetStart(), bezShape->GetEnd() };
            VECTOR2I segPts[2] = { segShape->GetStart(), segShape->GetEnd() };

            SEG::ecoord d[4];
            d[0] = ( segPts[0] - bezPts[0] ).SquaredEuclideanNorm();
            d[1] = ( segPts[0] - bezPts[1] ).SquaredEuclideanNorm();
            d[2] = ( segPts[1] - bezPts[0] ).SquaredEuclideanNorm();
            d[3] = ( segPts[1] - bezPts[1] ).SquaredEuclideanNorm();

            int idx = std::min_element( d, d + 4 ) - d;

            switch( idx )
            {
            case 0: segShape->SetStart( bezPts[0] ); break;
            case 1: segShape->SetStart( bezPts[1] ); break;
            case 2: segShape->SetEnd( bezPts[0] ); break;
            case 3: segShape->SetEnd( bezPts[1] ); break;
            }

            success = true;
        }

        return success;
    };

    PCB_SHAPE* graphic = nullptr;

    std::set<PCB_SHAPE*> startCandidates;
    for( PCB_SHAPE* shape : aShapeList )
    {
        if( shape->GetShape() == SHAPE_T::SEGMENT || shape->GetShape() == SHAPE_T::ARC
            || shape->GetShape() == SHAPE_T::BEZIER )
        {
            shape->ClearFlags( SKIP_STRUCT );
            startCandidates.emplace( shape );
        }
    }

    while( startCandidates.size() )
    {
        graphic = *startCandidates.begin();

        auto walkFrom = [&]( PCB_SHAPE* curr_graphic, VECTOR2I startPt )
        {
            VECTOR2I prevPt = startPt;

            for( ;; )
            {
                // Get next closest segment.
                PCB_SHAPE* nextGraphic =
                        findNext( curr_graphic, prevPt, aShapeList, aChainingEpsilon );

                if( !nextGraphic )
                    break;

                connectPair( curr_graphic, nextGraphic );

                prevPt = closer_to_first( prevPt, nextGraphic->GetStart(), nextGraphic->GetEnd() )
                                                                ? nextGraphic->GetEnd()
                                                                : nextGraphic->GetStart();
                curr_graphic = nextGraphic;
                curr_graphic->SetFlags( SKIP_STRUCT );
                startCandidates.erase( curr_graphic );
            }
        };

        const VECTOR2I ptEnd = graphic->GetEnd();
        const VECTOR2I ptStart = graphic->GetStart();

        PCB_SHAPE* grAtEnd = findNext( graphic, ptEnd, aShapeList, aChainingEpsilon );
        PCB_SHAPE* grAtStart = findNext( graphic, ptStart, aShapeList, aChainingEpsilon );

        bool beginFromEndPt = true;

        // We need to start walking from a point that is closest to a point of another shape.
        if( grAtEnd && grAtStart )
        {
            SEG::ecoord dAtEnd = min_distance_sq( ptEnd, grAtEnd->GetStart(), grAtEnd->GetEnd() );

            SEG::ecoord dAtStart =
                    min_distance_sq( ptStart, grAtStart->GetStart(), grAtStart->GetEnd() );

            beginFromEndPt = dAtEnd <= dAtStart;
        }
        else if( grAtEnd )
            beginFromEndPt = true;
        else if( grAtStart )
            beginFromEndPt = false;

        if( beginFromEndPt )
        {
            // Do not inline GetEnd / GetStart as endpoints may update
            walkFrom( graphic, graphic->GetEnd() );
            walkFrom( graphic, graphic->GetStart() );
        }
        else
        {
            walkFrom( graphic, graphic->GetStart() );
            walkFrom( graphic, graphic->GetEnd() );
        }

        startCandidates.erase( graphic );
    }
}
