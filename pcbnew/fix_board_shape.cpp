#include "fix_board_shape.h"

#include <vector>
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

    // Search again for anything that's close, even something already used.  (The latter is
    // important for error reporting.)
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

    auto close_enough = []( const VECTOR2I& aLeft, const VECTOR2I& aRight, unsigned aLimit ) -> bool
    {
        return ( aLeft - aRight ).SquaredEuclideanNorm() <= SEG::Square( aLimit );
    };

    auto closer_to_first = []( const VECTOR2I& aRef, const VECTOR2I& aFirst,
                               const VECTOR2I& aSecond ) -> bool
    {
        return ( aRef - aFirst ).SquaredEuclideanNorm() < ( aRef - aSecond ).SquaredEuclideanNorm();
    };

    auto addSegment = [&]( const VECTOR2I start, const VECTOR2I end, int width, PCB_LAYER_ID layer )
    {
        std::unique_ptr<PCB_SHAPE> seg = std::make_unique<PCB_SHAPE>( nullptr, SHAPE_T::SEGMENT );
        seg->SetStart( start );
        seg->SetEnd( end );
        seg->SetWidth( width );
        seg->SetLayer( layer );

        aNewShapes.emplace_back( std::move( seg ) );
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

            if( seg0.Intersects( seg1 ) || seg0.Angle( seg1 ) > ANGLE_45 )
            {
                if( OPT_VECTOR2I inter = seg0.IntersectLines( seg1 ) )
                {
                    if( closer_to_first( *inter, seg0.A, seg0.B ) )
                        aPrevShape->SetStart( *inter );
                    else
                        aPrevShape->SetEnd( *inter );

                    if( closer_to_first( *inter, seg1.A, seg1.B ) )
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

            SHAPE_ARC arc =
                    SHAPE_ARC( arcShape->GetStart(), arcShape->GetArcMid(), arcShape->GetEnd(), 0 );

            EDA_ANGLE extAngle( 20, DEGREES_T );
            if( arc.IsClockwise() )
                extAngle = -extAngle;

            VECTOR2D  arcStart = arc.GetP0();
            EDA_ANGLE arcAngle = arc.GetCentralAngle();

            RotatePoint( arcStart, arc.GetCenter(), extAngle );
            arcAngle += extAngle * 2;

            arcAngle = std::clamp( arcAngle, -ANGLE_360, ANGLE_360 );

            SHAPE_ARC extarc( arc.GetCenter(), arcStart, arcAngle );
            SEG       seg( segShape->GetStart(), segShape->GetEnd() );

            std::vector<VECTOR2I> ips;
            std::vector<VECTOR2I> onSeg;
            extarc.IntersectLine( seg, &ips );

            for( const VECTOR2I& ip : ips )
            {
                if( seg.Distance( ip ) <= aChainingEpsilon )
                {
                    if( closer_to_first( ip, seg.A, seg.B ) )
                        segShape->SetStart( ip );
                    else
                        segShape->SetEnd( ip );

                    if( closer_to_first( ip, arc.GetP0(), arc.GetP1() ) )
                        arcShape->SetArcGeometry( ip, arc.GetArcMid(), arc.GetP1() );
                    else
                        arcShape->SetArcGeometry( arc.GetP0(), arc.GetArcMid(), ip );

                    success = true;
                }
            }

            if( !success )
            {
                VECTOR2I lineProj = seg.LineProject( arc.GetCenter() );

                if( closer_to_first( lineProj, seg.A, seg.B ) )
                    segShape->SetStart( lineProj );
                else
                    segShape->SetEnd( lineProj );

                CIRCLE   circ( arc.GetCenter(), arc.GetRadius() );
                VECTOR2I circProj = circ.NearestPoint( lineProj );

                if( closer_to_first( circProj, arc.GetP0(), arc.GetP1() ) )
                    arcShape->SetArcGeometry( circProj, arc.GetArcMid(), arc.GetP1() );
                else
                    arcShape->SetArcGeometry( arc.GetP0(), arc.GetArcMid(), circProj );


                addSegment( circProj, lineProj, segShape->GetWidth(), segShape->GetLayer() );
                success = true;
            }
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

        auto walkFrom = [&]( PCB_SHAPE* graphic, VECTOR2I startPt )
        {
            VECTOR2I prevPt = startPt;

            for( ;; )
            {
                // Get next closest segment.
                PCB_SHAPE* nextGraphic = findNext( graphic, prevPt, aShapeList, aChainingEpsilon );

                if( !nextGraphic )
                    break;

                VECTOR2I nstart = nextGraphic->GetStart();
                VECTOR2I nend = nextGraphic->GetEnd();

                if( !closer_to_first( prevPt, nstart, nend ) )
                    std::swap( nstart, nend );

                if( !connectPair( graphic, nextGraphic ) )
                    addSegment( prevPt, nstart, graphic->GetWidth(), graphic->GetLayer() );

                // Shape might've changed
                nstart = nextGraphic->GetStart();
                nend = nextGraphic->GetEnd();

                if( !closer_to_first( prevPt, nstart, nend ) )
                    std::swap( nstart, nend );

                prevPt = nend;
                graphic = nextGraphic;
                graphic->SetFlags( SKIP_STRUCT );
                startCandidates.erase( graphic );
            }
        };

        walkFrom( graphic, graphic->GetEnd() );
        walkFrom( graphic, graphic->GetStart() );
        startCandidates.erase( graphic );
    }
}
