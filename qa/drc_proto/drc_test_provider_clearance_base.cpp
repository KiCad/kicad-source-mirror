#include <common.h>
#include <class_board.h>
#include <class_drawsegment.h>
#include <class_pad.h>

#include <convert_basic_shapes_to_polygon.h>
#include <geometry/polygon_test_point_inside.h>

#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_rect.h>

#include <drc_proto/drc_engine.h>
#include <drc_proto/drc_test_provider_clearance_base.h>
#include <drc_proto/drc_item.h>
#include <drc_proto/drc_rule.h>

const int UI_EPSILON = Mils2iu( 5 );

wxPoint test::DRC_TEST_PROVIDER_CLEARANCE_BASE::getLocation( TRACK* aTrack, ZONE_CONTAINER* aConflictZone )
{
    SHAPE_POLY_SET* conflictOutline;

    if( aConflictZone->IsFilled() )
        conflictOutline = const_cast<SHAPE_POLY_SET*>( &aConflictZone->GetFilledPolysList() );
    else
        conflictOutline = aConflictZone->Outline();

    wxPoint pt1 = aTrack->GetPosition();
    wxPoint pt2 = aTrack->GetEnd();

    // If the mid-point is in the zone, then that's a fine place for the marker
    if( conflictOutline->SquaredDistance( ( pt1 + pt2 ) / 2 ) == 0 )
        return ( pt1 + pt2 ) / 2;

    // Otherwise do a binary search for a "good enough" marker location
    else
    {
        while( GetLineLength( pt1, pt2 ) > UI_EPSILON )
        {
            if( conflictOutline->SquaredDistance( pt1 ) < conflictOutline->SquaredDistance( pt2 ) )
                pt2 = ( pt1 + pt2 ) / 2;
            else
                pt1 = ( pt1 + pt2 ) / 2;
        }

        // Once we're within UI_EPSILON pt1 and pt2 are "equivalent"
        return pt1;
    }
}

wxPoint test::DRC_TEST_PROVIDER_CLEARANCE_BASE::getLocation( TRACK* aTrack, const SEG& aConflictSeg )
{
    wxPoint pt1 = aTrack->GetPosition();
    wxPoint pt2 = aTrack->GetEnd();

    // Do a binary search along the track for a "good enough" marker location
    while( GetLineLength( pt1, pt2 ) > UI_EPSILON )
    {
        if( aConflictSeg.SquaredDistance( pt1 ) < aConflictSeg.SquaredDistance( pt2 ) )
            pt2 = ( pt1 + pt2 ) / 2;
        else
            pt1 = ( pt1 + pt2 ) / 2;
    }

    // Once we're within UI_EPSILON pt1 and pt2 are "equivalent"
    return pt1;
}


/*
 * Test if distance between a segment and a pad is > minClearance.  Return the actual
 * distance if it is less.
 */
bool test::DRC_TEST_PROVIDER_CLEARANCE_BASE::checkClearanceSegmToPad( const SEG& refSeg, int refSegWidth, const D_PAD* pad,
                                   int minClearance, int* aActualDist )
{
    if( ( pad->GetShape() == PAD_SHAPE_CIRCLE || pad->GetShape() == PAD_SHAPE_OVAL ) )
    {
        /* Treat an oval pad as a line segment along the hole's major axis,
         * shortened by half its minor axis.
         * A circular pad is just a degenerate case of an oval hole.
         */
        wxPoint padStart, padEnd;
        int     padWidth;

        pad->GetOblongGeometry( pad->GetSize(), &padStart, &padEnd, &padWidth );
        padStart += pad->ShapePos();
        padEnd += pad->ShapePos();

        SEG padSeg( padStart, padEnd );
        int widths = ( padWidth + refSegWidth ) / 2;
        int center2centerAllowed = minClearance + widths;

        // Avoid square-roots if possible (for performance)
        SEG::ecoord center2center_squared = refSeg.SquaredDistance( padSeg );

        if( center2center_squared < SEG::Square( center2centerAllowed ) )
        {
            *aActualDist = std::max( 0.0, sqrt( center2center_squared ) - widths );
            return false;
        }
    }
    else if( ( pad->GetShape() == PAD_SHAPE_RECT || pad->GetShape() == PAD_SHAPE_ROUNDRECT )
            && ( (int) pad->GetOrientation() % 900 == 0 ) )
    {
        EDA_RECT padBBox = pad->GetBoundingBox();
        int     widths = refSegWidth / 2;

        // Note a ROUNDRECT pad with a corner radius = r can be treated as a smaller
        // RECT (size - 2*r) with a clearance increased by r
        if( pad->GetShape() == PAD_SHAPE_ROUNDRECT )
        {
            padBBox.Inflate( - pad->GetRoundRectCornerRadius() );
            widths += pad->GetRoundRectCornerRadius();
        }

        SHAPE_RECT padShape( padBBox.GetPosition(), padBBox.GetWidth(), padBBox.GetHeight() );
        int        actual;

        if( padShape.DoCollide( refSeg, minClearance + widths, &actual ) )
        {
            *aActualDist = std::max( 0, actual - widths );
            return false;
        }
    }
    else        // Convert the rest to polygons
    {
        SHAPE_POLY_SET polyset;

        BOARD* board = pad->GetBoard();
        int    maxError = board ? board->GetDesignSettings().m_MaxError : ARC_HIGH_DEF;

        pad->TransformShapeWithClearanceToPolygon( polyset, 0, maxError );

        const SHAPE_LINE_CHAIN& refpoly = polyset.COutline( 0 );
        int                     widths = refSegWidth / 2;
        int                     actual;

        if( !poly2segmentDRC( (wxPoint*) &refpoly.CPoint( 0 ), refpoly.PointCount(),
                              (wxPoint) refSeg.A, (wxPoint) refSeg.B,
                              minClearance + widths, &actual ) )
        {
            *aActualDist = std::max( 0, actual - widths );
            return false;
        }
    }

    return true;
}


bool test::DRC_TEST_PROVIDER_CLEARANCE_BASE::checkClearancePadToPad( D_PAD* aRefPad, D_PAD* aPad, int aMinClearance, int* aActual )
{
    int center2center = KiROUND( EuclideanNorm( aPad->ShapePos() - aRefPad->ShapePos() ) );

    // Quick test: Clearance is OK if the bounding circles are further away than aMinClearance
    if( center2center - aRefPad->GetBoundingRadius() - aPad->GetBoundingRadius() >= aMinClearance )
        return true;

    // JEY TODO:
    // TOM TODO: MTV only works as a proxy for actual-distance for convex shapes

    VECTOR2I mtv;
    VECTOR2I maxMtv( 0, 0 );

    for( const std::shared_ptr<SHAPE>& aShape : aRefPad->GetEffectiveShapes() )
    {
        for( const std::shared_ptr<SHAPE>& bShape : aPad->GetEffectiveShapes() )
        {
            if( aShape->Collide( bShape.get(), aMinClearance, mtv ) )
            {
                if( mtv.SquaredEuclideanNorm() > maxMtv.SquaredEuclideanNorm() )
                    maxMtv = mtv;
            }
        }
    }

    if( maxMtv.x > 0 || maxMtv.y > 0 )
    {
        *aActual = std::max( 0, aMinClearance - maxMtv.EuclideanNorm() );
        return false;
    }

    return true;
}


bool test::DRC_TEST_PROVIDER_CLEARANCE_BASE::poly2segmentDRC( wxPoint* aTref, int aTrefCount, wxPoint aSegStart, wxPoint aSegEnd,
                      int aDist, int* aActual )
{
    /* Test if the segment is contained in the polygon.
     * This case is not covered by the following check if the segment is
     * completely contained in the polygon (because edges don't intersect)!
     */
    if( TestPointInsidePolygon( aTref, aTrefCount, aSegStart ) )
    {
        *aActual = 0;
        return false;
    }

    for( int ii = 0, jj = aTrefCount-1; ii < aTrefCount; jj = ii, ii++ )
    {   // for all edges in polygon
        double d;

        if( TestForIntersectionOfStraightLineSegments( aTref[ii].x, aTref[ii].y, aTref[jj].x,
                                                       aTref[jj].y, aSegStart.x, aSegStart.y,
                                                       aSegEnd.x, aSegEnd.y, NULL, NULL, &d ) )
        {
            *aActual = 0;
            return false;
        }

        if( d < aDist )
        {
            *aActual = KiROUND( d );
            return false;
        }
    }

    return true;
}


/**
 * compare 2 convex polygons and return true if distance > aDist (if no error DRC)
 * i.e if for each edge of the first polygon distance from each edge of the other polygon
 * is >= aDist
 */
bool test::DRC_TEST_PROVIDER_CLEARANCE_BASE::poly2polyDRC( wxPoint* aTref, int aTrefCount, wxPoint* aTtest, int aTtestCount,
                   int aAllowedDist, int* actualDist )
{
    /* Test if one polygon is contained in the other and thus the polygon overlap.
     * This case is not covered by the following check if one polygone is
     * completely contained in the other (because edges don't intersect)!
     */
    if( TestPointInsidePolygon( aTref, aTrefCount, aTtest[0] ) )
    {
        *actualDist = 0;
        return false;
    }

    if( TestPointInsidePolygon( aTtest, aTtestCount, aTref[0] ) )
    {
        *actualDist = 0;
        return false;
    }

    for( int ii = 0, jj = aTrefCount - 1; ii < aTrefCount; jj = ii, ii++ )
    {
        // for all edges in aTref
        for( int kk = 0, ll = aTtestCount - 1; kk < aTtestCount; ll = kk, kk++ )
        {
            // for all edges in aTtest
            double d;
            int    intersect = TestForIntersectionOfStraightLineSegments(
                                        aTref[ii].x, aTref[ii].y, aTref[jj].x, aTref[jj].y,
                                        aTtest[kk].x, aTtest[kk].y, aTtest[ll].x, aTtest[ll].y,
                                        nullptr, nullptr, &d );

            if( intersect )
            {
                *actualDist = 0;
                return false;
            }

            if( d < aAllowedDist )
            {
                *actualDist = KiROUND( d );
                return false;
            }
        }
    }

    return true;
}
