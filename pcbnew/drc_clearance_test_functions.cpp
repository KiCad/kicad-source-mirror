/**
 * @file drc_clearance_test_functions.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2015 KiCad Developers, see change_log.txt for contributors.
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

/****************************/
/* DRC control				*/
/****************************/

#include <fctsys.h>
#include <wxPcbStruct.h>
#include <trigo.h>

#include <pcbnew.h>
#include <drc_stuff.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_zone.h>
#include <class_marker_pcb.h>
#include <math_for_graphics.h>
#include <polygon_test_point_inside.h>


/* compare 2 trapezoids (can be rectangle) and return true if distance > aDist
 * i.e if for each edge of the first polygon distance from each edge of the other polygon
 * is >= aDist
 */
bool trapezoid2trapezoidDRC( wxPoint aTref[4], wxPoint aTcompare[4], int aDist )
{
    /* Test if one polygon is contained in the other and thus the polygon overlap.
     * This case is not covered by the following check if one polygond is
     * completely contained in the other (because edges don't intersect)!
     */
    if( TestPointInsidePolygon( aTref, 4, aTcompare[0] ) )
        return false;

    if( TestPointInsidePolygon( aTcompare, 4, aTref[0] ) )
        return false;

    int ii, jj, kk, ll;

    for( ii = 0, jj = 3; ii<4; jj = ii, ii++ )          // for all edges in aTref
    {
        for( kk = 0, ll = 3; kk < 4; ll = kk, kk++ )    // for all edges in aTcompare
        {
            double d;
            int    intersect = TestForIntersectionOfStraightLineSegments( aTref[ii].x,
                                                                          aTref[ii].y,
                                                                          aTref[jj].x,
                                                                          aTref[jj].y,
                                                                          aTcompare[kk].x,
                                                                          aTcompare[kk].y,
                                                                          aTcompare[ll].x,
                                                                          aTcompare[ll].y,
                                                                          NULL, NULL, &d );
            if( intersect || (d< aDist) )
                return false;
        }
    }

    return true;
}


/* compare a trapezoids (can be rectangle) and a segment and return true if distance > aDist
 */
bool trapezoid2segmentDRC( wxPoint aTref[4], wxPoint aSegStart, wxPoint aSegEnd, int aDist )
{
    /* Test if the segment is contained in the polygon.
     * This case is not covered by the following check if the segment is
     * completely contained in the polygon (because edges don't intersect)!
     */
    if( TestPointInsidePolygon( aTref, 4, aSegStart ) )
        return false;

    int ii, jj;

    for( ii = 0, jj = 3; ii < 4; jj = ii, ii++ )  // for all edges in aTref
    {
        double d;
        int    intersect = TestForIntersectionOfStraightLineSegments( aTref[ii].x,
                                                                      aTref[ii].y,
                                                                      aTref[jj].x,
                                                                      aTref[jj].y,
                                                                      aSegStart.x,
                                                                      aSegStart.y,
                                                                      aSegEnd.x,
                                                                      aSegEnd.y,
                                                                      NULL, NULL, &d );
        if( intersect || (d< aDist) )
            return false;
    }

    return true;
}


/* compare a trapezoid to a point and return true if distance > aDist
 * do not use this function for horizontal or vertical rectangles
 * because there is a faster an easier way to compare the distance
 */
bool trapezoid2pointDRC( wxPoint aTref[4], wxPoint aPcompare, int aDist )
{
    /* Test if aPcompare point is contained in the polygon.
     * This case is not covered by the following check if this point is inside the polygon
     */
    if( TestPointInsidePolygon( aTref, 4, aPcompare ) )
    {
        return false;
    }

    // Test distance between aPcompare and each segment of the polygon:
    for( int ii = 0, jj = 3; ii < 4; jj = ii, ii++ )  // for all edge in polygon
    {
        if( TestSegmentHit( aTref[ii], aTref[jj], aPcompare, aDist ) )
            return false;
    }

    return true;
}


bool DRC::doTrackDrc( TRACK* aRefSeg, TRACK* aStart, bool testPads )
{
    TRACK*    track;
    wxPoint   delta;           // lenght on X and Y axis of segments
    LSET layerMask;
    int       net_code_ref;
    wxPoint   shape_pos;

    NETCLASSPTR netclass = aRefSeg->GetNetClass();
    BOARD_DESIGN_SETTINGS& dsnSettings = m_pcb->GetDesignSettings();

    /* In order to make some calculations more easier or faster,
     * pads and tracks coordinates will be made relative to the reference segment origin
     */
    wxPoint origin = aRefSeg->GetStart();  // origin will be the origin of other coordinates

    m_segmEnd   = delta = aRefSeg->GetEnd() - origin;
    m_segmAngle = 0;

    layerMask    = aRefSeg->GetLayerSet();
    net_code_ref = aRefSeg->GetNetCode();

    // Phase 0 : Test vias
    if( aRefSeg->Type() == PCB_VIA_T )
    {
        const VIA *refvia = static_cast<const VIA*>( aRefSeg );
        // test if the via size is smaller than minimum
        if( refvia->GetViaType() == VIA_MICROVIA )
        {
            if( refvia->GetWidth() < dsnSettings.m_MicroViasMinSize )
            {
                m_currentMarker = fillMarker( refvia, NULL,
                                              DRCE_TOO_SMALL_MICROVIA, m_currentMarker );
                return false;
            }
        }
        else
        {
            if( refvia->GetWidth() < dsnSettings.m_ViasMinSize )
            {
                m_currentMarker = fillMarker( refvia, NULL,
                                              DRCE_TOO_SMALL_VIA, m_currentMarker );
                return false;
            }
        }

        // test if via's hole is bigger than its diameter
        // This test is necessary since the via hole size and width can be modified
        // and a default via hole can be bigger than some vias sizes
        if( refvia->GetDrillValue() > refvia->GetWidth() )
        {
            m_currentMarker = fillMarker( refvia, NULL,
                                          DRCE_VIA_HOLE_BIGGER, m_currentMarker );
            return false;
        }

        // For microvias: test if they are blind vias and only between 2 layers
        // because they are used for very small drill size and are drill by laser
        // and **only one layer** can be drilled
        if( refvia->GetViaType() == VIA_MICROVIA )
        {
            LAYER_ID    layer1, layer2;
            bool        err = true;

            refvia->LayerPair( &layer1, &layer2 );

            if( layer1 > layer2 )
                std::swap( layer1, layer2 );

            if( layer2 == B_Cu && layer1 == m_pcb->GetDesignSettings().GetCopperLayerCount() - 2 )
                err = false;
            else if( layer1 == F_Cu  &&  layer2 == In1_Cu  )
                err = false;

            if( err )
            {
                m_currentMarker = fillMarker( refvia, NULL,
                                              DRCE_MICRO_VIA_INCORRECT_LAYER_PAIR, m_currentMarker );
                return false;
            }
        }
    }
    else    // This is a track segment
    {
        if( aRefSeg->GetWidth() < dsnSettings.m_TrackMinWidth )
        {
            m_currentMarker = fillMarker( aRefSeg, NULL,
                                          DRCE_TOO_SMALL_TRACK_WIDTH, m_currentMarker );
            return false;
        }
    }

    // for a non horizontal or vertical segment Compute the segment angle
    // in tenths of degrees and its length
    if( delta.x || delta.y )
    {
        // Compute the segment angle in 0,1 degrees
        m_segmAngle = ArcTangente( delta.y, delta.x );

        // Compute the segment length: we build an equivalent rotated segment,
        // this segment is horizontal, therefore dx = length
        RotatePoint( &delta, m_segmAngle );    // delta.x = length, delta.y = 0
    }

    m_segmLength = delta.x;

    /******************************************/
    /* Phase 1 : test DRC track to pads :     */
    /******************************************/

    /* Use a dummy pad to test DRC tracks versus holes, for pads not on all copper layers
     * but having a hole
     * This dummy pad has the size and shape of the hole
     * to test tracks to pad hole DRC, using checkClearanceSegmToPad test function.
     * Therefore, this dummy pad is a circle or an oval.
     * A pad must have a parent because some functions expect a non null parent
     * to find the parent board, and some other data
     */
    MODULE  dummymodule( m_pcb );    // Creates a dummy parent
    D_PAD   dummypad( &dummymodule );

    dummypad.SetLayerSet( LSET::AllCuMask() );     // Ensure the hole is on all layers

    // Compute the min distance to pads
    if( testPads )
    {
        for( unsigned ii = 0;  ii<m_pcb->GetPadCount();  ++ii )
        {
            D_PAD* pad = m_pcb->GetPad( ii );

            /* No problem if pads are on an other layer,
             * But if a drill hole exists	(a pad on a single layer can have a hole!)
             * we must test the hole
             */
            if( !( pad->GetLayerSet() & layerMask ).any() )
            {
                /* We must test the pad hole. In order to use the function
                 * checkClearanceSegmToPad(),a pseudo pad is used, with a shape and a
                 * size like the hole
                 */
                if( pad->GetDrillSize().x == 0 )
                    continue;

                dummypad.SetSize( pad->GetDrillSize() );
                dummypad.SetPosition( pad->GetPosition() );
                dummypad.SetShape( pad->GetDrillShape()  == PAD_DRILL_SHAPE_OBLONG ?
                                   PAD_SHAPE_OVAL : PAD_SHAPE_CIRCLE );
                dummypad.SetOrientation( pad->GetOrientation() );

                m_padToTestPos = dummypad.GetPosition() - origin;

                if( !checkClearanceSegmToPad( &dummypad, aRefSeg->GetWidth(),
                                              netclass->GetClearance() ) )
                {
                    m_currentMarker = fillMarker( aRefSeg, pad,
                                                  DRCE_TRACK_NEAR_THROUGH_HOLE, m_currentMarker );
                    return false;
                }

                continue;
            }

            // The pad must be in a net (i.e pt_pad->GetNet() != 0 )
            // but no problem if the pad netcode is the current netcode (same net)
            if( pad->GetNetCode()                       // the pad must be connected
               && net_code_ref == pad->GetNetCode() )   // the pad net is the same as current net -> Ok
                continue;

            // DRC for the pad
            shape_pos = pad->ShapePos();
            m_padToTestPos = shape_pos - origin;

            if( !checkClearanceSegmToPad( pad, aRefSeg->GetWidth(), aRefSeg->GetClearance( pad ) ) )
            {
                m_currentMarker = fillMarker( aRefSeg, pad,
                                              DRCE_TRACK_NEAR_PAD, m_currentMarker );
                return false;
            }
        }
    }

    /***********************************************/
    /* Phase 2: test DRC with other track segments */
    /***********************************************/

    // At this point the reference segment is the X axis

    // Test the reference segment with other track segments
    wxPoint segStartPoint;
    wxPoint segEndPoint;
    for( track = aStart; track; track = track->Next() )
    {
        // No problem if segments have the same net code:
        if( net_code_ref == track->GetNetCode() )
            continue;

        // No problem if segment are on different layers :
        if( !( layerMask & track->GetLayerSet() ).any() )
            continue;

        // the minimum distance = clearance plus half the reference track
        // width plus half the other track's width
        int w_dist = aRefSeg->GetClearance( track );
        w_dist += (aRefSeg->GetWidth() + track->GetWidth()) / 2;

        // If the reference segment is a via, we test it here
        if( aRefSeg->Type() == PCB_VIA_T )
        {
            delta = track->GetEnd() - track->GetStart();
            segStartPoint = aRefSeg->GetStart() - track->GetStart();

            if( track->Type() == PCB_VIA_T )
            {
                // Test distance between two vias, i.e. two circles, trivial case
                if( EuclideanNorm( segStartPoint ) < w_dist )
                {
                    m_currentMarker = fillMarker( aRefSeg, track,
                                                  DRCE_VIA_NEAR_VIA, m_currentMarker );
                    return false;
                }
            }
            else    // test via to segment
            {
                // Compute l'angle du segment a tester;
                double angle = ArcTangente( delta.y, delta.x );

                // Compute new coordinates ( the segment become horizontal)
                RotatePoint( &delta, angle );
                RotatePoint( &segStartPoint, angle );

                if( !checkMarginToCircle( segStartPoint, w_dist, delta.x ) )
                {
                    m_currentMarker = fillMarker( track, aRefSeg,
                                                  DRCE_VIA_NEAR_TRACK, m_currentMarker );
                    return false;
                }
            }

            continue;
        }

        /* We compute segStartPoint, segEndPoint = starting and ending point coordinates for
         * the segment to test in the new axis : the new X axis is the
         * reference segment.  We must translate and rotate the segment to test
         */
        segStartPoint = track->GetStart() - origin;
        segEndPoint   = track->GetEnd() - origin;
        RotatePoint( &segStartPoint, m_segmAngle );
        RotatePoint( &segEndPoint, m_segmAngle );
        if( track->Type() == PCB_VIA_T )
        {
            if( checkMarginToCircle( segStartPoint, w_dist, m_segmLength ) )
                continue;

            m_currentMarker = fillMarker( aRefSeg, track,
                                          DRCE_TRACK_NEAR_VIA, m_currentMarker );
            return false;
        }

        /*	We have changed axis:
         *  the reference segment is Horizontal.
         *  3 cases : the segment to test can be parallel, perpendicular or have an other direction
         */
        if( segStartPoint.y == segEndPoint.y ) // parallel segments
        {
            if( abs( segStartPoint.y ) >= w_dist )
                continue;

            // Ensure segStartPoint.x <= segEndPoint.x
            if( segStartPoint.x > segEndPoint.x )
                std::swap( segStartPoint.x, segEndPoint.x );

            if( segStartPoint.x > (-w_dist) && segStartPoint.x < (m_segmLength + w_dist) )    /* possible error drc */
            {
                // the start point is inside the reference range
                //      X........
                //    O--REF--+

                // Fine test : we consider the rounded shape of each end of the track segment:
                if( segStartPoint.x >= 0 && segStartPoint.x <= m_segmLength )
                {
                    m_currentMarker = fillMarker( aRefSeg, track,
                                                  DRCE_TRACK_ENDS1, m_currentMarker );
                    return false;
                }

                if( !checkMarginToCircle( segStartPoint, w_dist, m_segmLength ) )
                {
                    m_currentMarker = fillMarker( aRefSeg, track,
                                                  DRCE_TRACK_ENDS2, m_currentMarker );
                    return false;
                }
            }

            if( segEndPoint.x > (-w_dist) && segEndPoint.x < (m_segmLength + w_dist) )
            {
                // the end point is inside the reference range
                //  .....X
                //    O--REF--+
                // Fine test : we consider the rounded shape of the ends
                if( segEndPoint.x >= 0 && segEndPoint.x <= m_segmLength )
                {
                    m_currentMarker = fillMarker( aRefSeg, track,
                                                  DRCE_TRACK_ENDS3, m_currentMarker );
                    return false;
                }

                if( !checkMarginToCircle( segEndPoint, w_dist, m_segmLength ) )
                {
                    m_currentMarker = fillMarker( aRefSeg, track,
                                                  DRCE_TRACK_ENDS4, m_currentMarker );
                    return false;
                }
            }

            if( segStartPoint.x <=0 && segEndPoint.x >= 0 )
            {
            // the segment straddles the reference range (this actually only
            // checks if it straddles the origin, because the other cases where already
            // handled)
            //  X.............X
            //    O--REF--+
                m_currentMarker = fillMarker( aRefSeg, track,
                                              DRCE_TRACK_SEGMENTS_TOO_CLOSE, m_currentMarker );
                return false;
            }
        }
        else if( segStartPoint.x == segEndPoint.x ) // perpendicular segments
        {
            if( ( segStartPoint.x <= (-w_dist) ) || ( segStartPoint.x >= (m_segmLength + w_dist) ) )
                continue;

            // Test if segments are crossing
            if( segStartPoint.y > segEndPoint.y )
                std::swap( segStartPoint.y, segEndPoint.y );

            if( (segStartPoint.y < 0) && (segEndPoint.y > 0) )
            {
                m_currentMarker = fillMarker( aRefSeg, track,
                                              DRCE_TRACKS_CROSSING, m_currentMarker );
                return false;
            }

            // At this point the drc error is due to an end near a reference segm end
            if( !checkMarginToCircle( segStartPoint, w_dist, m_segmLength ) )
            {
                m_currentMarker = fillMarker( aRefSeg, track,
                                              DRCE_ENDS_PROBLEM1, m_currentMarker );
                return false;
            }
            if( !checkMarginToCircle( segEndPoint, w_dist, m_segmLength ) )
            {
                m_currentMarker = fillMarker( aRefSeg, track,
                                              DRCE_ENDS_PROBLEM2, m_currentMarker );
                return false;
            }
        }
        else    // segments quelconques entre eux
        {
            // calcul de la "surface de securite du segment de reference
            // First rought 'and fast) test : the track segment is like a rectangle

            m_xcliplo = m_ycliplo = -w_dist;
            m_xcliphi = m_segmLength + w_dist;
            m_ycliphi = w_dist;

            // A fine test is needed because a serment is not exactly a
            // rectangle, it has rounded ends
            if( !checkLine( segStartPoint, segEndPoint ) )
            {
                /* 2eme passe : the track has rounded ends.
                 * we must a fine test for each rounded end and the
                 * rectangular zone
                 */

                m_xcliplo = 0;
                m_xcliphi = m_segmLength;

                if( !checkLine( segStartPoint, segEndPoint ) )
                {
                    m_currentMarker = fillMarker( aRefSeg, track,
                                                  DRCE_ENDS_PROBLEM3, m_currentMarker );
                    return false;
                }
                else    // The drc error is due to the starting or the ending point of the reference segment
                {
                    // Test the starting and the ending point
                    segStartPoint = track->GetStart();
                    segEndPoint   = track->GetEnd();
                    delta = segEndPoint - segStartPoint;

                    // Compute the segment orientation (angle) en 0,1 degre
                    double angle = ArcTangente( delta.y, delta.x );

                    // Compute the segment lenght: delta.x = lenght after rotation
                    RotatePoint( &delta, angle );

                    /* Comute the reference segment coordinates relatives to a
                     *  X axis = current tested segment
                     */
                    wxPoint relStartPos = aRefSeg->GetStart() - segStartPoint;
                    wxPoint relEndPos   = aRefSeg->GetEnd() - segStartPoint;

                    RotatePoint( &relStartPos, angle );
                    RotatePoint( &relEndPos, angle );

                    if( !checkMarginToCircle( relStartPos, w_dist, delta.x ) )
                    {
                        m_currentMarker = fillMarker( aRefSeg, track,
                                                      DRCE_ENDS_PROBLEM4, m_currentMarker );
                        return false;
                    }

                    if( !checkMarginToCircle( relEndPos, w_dist, delta.x ) )
                    {
                        m_currentMarker = fillMarker( aRefSeg, track,
                                                      DRCE_ENDS_PROBLEM5, m_currentMarker );
                        return false;
                    }
                }
            }
        }
    }

    return true;
}


/* test DRC between 2 pads.
 * this function can be also used to test DRC between a pas and a hole,
 * because a hole is like a round pad.
 */
bool DRC::checkClearancePadToPad( D_PAD* aRefPad, D_PAD* aPad )
{
    int     dist;

    double  pad_angle;

    // Get the clearance between the 2 pads. this is the min distance between aRefPad and aPad
    int     dist_min = aRefPad->GetClearance( aPad );

    // relativePadPos is the aPad shape position relative to the aRefPad shape position
    wxPoint relativePadPos = aPad->ShapePos() - aRefPad->ShapePos();

    dist = KiROUND( EuclideanNorm( relativePadPos ) );

    // Quick test: Clearance is OK if the bounding circles are further away than "dist_min"
    if( (dist - aRefPad->GetBoundingRadius() - aPad->GetBoundingRadius()) >= dist_min )
        return true;

    /* Here, pads are near and DRC depend on the pad shapes
     * We must compare distance using a fine shape analysis
     * Because a circle or oval shape is the easier shape to test, try to have
     * aRefPad shape type = PAD_SHAPE_CIRCLE or PAD_SHAPE_OVAL.
     * if aRefPad = TRAP. and aPad = RECT, also swap pads
     * Swap aRefPad and aPad if needed
     */
    bool swap_pads;
    swap_pads = false;

    // swap pads to make comparisons easier
    // priority is aRefPad = ROUND then OVAL then RECT then other
    if( aRefPad->GetShape() != aPad->GetShape() && aRefPad->GetShape() != PAD_SHAPE_CIRCLE )
    {
        // pad ref shape is here oval, rect or trapezoid
        switch( aPad->GetShape() )
        {
            case PAD_SHAPE_CIRCLE:
                swap_pads = true;
                break;

            case PAD_SHAPE_OVAL:
                swap_pads = true;
                break;

            case PAD_SHAPE_RECT:
                if( aRefPad->GetShape() != PAD_SHAPE_OVAL )
                    swap_pads = true;
                break;

            default:
                break;
        }
    }

    if( swap_pads )
    {
        std::swap( aRefPad, aPad );
        relativePadPos = -relativePadPos;
    }

    /* Because pad exchange, aRefPad shape is PAD_SHAPE_CIRCLE or PAD_SHAPE_OVAL,
     * if one of the 2 pads was a PAD_SHAPE_CIRCLE or PAD_SHAPE_OVAL.
     * Therefore, if aRefPad is a PAD_SHAPE_SHAPE_RECT or a PAD_SHAPE_TRAPEZOID,
     * aPad is also a PAD_SHAPE_RECT or a PAD_SHAPE_TRAPEZOID
     */
    bool diag = true;

    switch( aRefPad->GetShape() )
    {
    case PAD_SHAPE_CIRCLE:

        /* One can use checkClearanceSegmToPad to test clearance
         * aRefPad is like a track segment with a null length and a witdth = GetSize().x
         */
        m_segmLength = 0;
        m_segmAngle  = 0;

        m_segmEnd.x = m_segmEnd.y = 0;

        m_padToTestPos = relativePadPos;
        diag = checkClearanceSegmToPad( aPad, aRefPad->GetSize().x, dist_min );
        break;

    case PAD_SHAPE_RECT:
        // pad_angle = pad orient relative to the aRefPad orient
        pad_angle = aRefPad->GetOrientation() + aPad->GetOrientation();
        NORMALIZE_ANGLE_POS( pad_angle );

        if( aPad->GetShape() == PAD_SHAPE_RECT )
        {
            wxSize size = aPad->GetSize();

            // The trivial case is if both rects are rotated by multiple of 90 deg
            // Most of time this is the case, and the test is fast
            if( ( (aRefPad->GetOrientation() == 0) || (aRefPad->GetOrientation() == 900)
                 || (aRefPad->GetOrientation() == 1800) || (aRefPad->GetOrientation() == 2700) )
               && ( (aPad->GetOrientation() == 0) || (aPad->GetOrientation() == 900) || (aPad->GetOrientation() == 1800)
                   || (aPad->GetOrientation() == 2700) ) )
            {
                if( (pad_angle == 900) || (pad_angle == 2700) )
                {
                    std::swap( size.x, size.y );
                }

                // Test DRC:
                diag = false;
                RotatePoint( &relativePadPos, aRefPad->GetOrientation() );
                relativePadPos.x = std::abs( relativePadPos.x );
                relativePadPos.y = std::abs( relativePadPos.y );

                if( ( relativePadPos.x - ( (size.x + aRefPad->GetSize().x) / 2 ) ) >= dist_min )
                    diag = true;

                if( ( relativePadPos.y - ( (size.y + aRefPad->GetSize().y) / 2 ) ) >= dist_min )
                    diag = true;
            }
            else    // at least one pad has any other orient. Test is more tricky
            {   // Use the trapezoid2trapezoidDRC which also compare 2 rectangles with any orientation
                wxPoint polyref[4];         // Shape of aRefPad
                wxPoint polycompare[4];     // Shape of aPad
                aRefPad->BuildPadPolygon( polyref, wxSize( 0, 0 ), aRefPad->GetOrientation() );
                aPad->BuildPadPolygon( polycompare, wxSize( 0, 0 ), aPad->GetOrientation() );

                // Move aPad shape to relativePadPos
                for( int ii = 0; ii < 4; ii++ )
                    polycompare[ii] += relativePadPos;

                // And now test polygons:
                if( !trapezoid2trapezoidDRC( polyref, polycompare, dist_min ) )
                    diag = false;
            }
        }
        else if( aPad->GetShape() == PAD_SHAPE_TRAPEZOID )
        {
            wxPoint polyref[4];         // Shape of aRefPad
            wxPoint polycompare[4];     // Shape of aPad
            aRefPad->BuildPadPolygon( polyref, wxSize( 0, 0 ), aRefPad->GetOrientation() );
            aPad->BuildPadPolygon( polycompare, wxSize( 0, 0 ), aPad->GetOrientation() );

            // Move aPad shape to relativePadPos
            for( int ii = 0; ii < 4; ii++ )
                polycompare[ii] += relativePadPos;

            // And now test polygons:
            if( !trapezoid2trapezoidDRC( polyref, polycompare, dist_min ) )
                diag = false;
        }
        else
        {
            // Should not occur, because aPad and aRefPad are swapped
            // to have only aPad shape RECT or TRAP and aRefPad shape TRAP or RECT.
            wxLogDebug( wxT( "DRC::checkClearancePadToPad: unexpected pad ref RECT @ %d, %d to pad shape %d @ %d, %d"),
                aRefPad->GetPosition().x, aRefPad->GetPosition().y,
                aPad->GetShape(), aPad->GetPosition().x, aPad->GetPosition().y );
        }
        break;

    case PAD_SHAPE_OVAL:     /* an oval pad is like a track segment */
    {
        /* Create a track segment with same dimensions as the oval aRefPad
         * and use checkClearanceSegmToPad function to test aPad to aRefPad clearance
         */
        int segm_width;
        m_segmAngle = aRefPad->GetOrientation();                // Segment orient.

        if( aRefPad->GetSize().y < aRefPad->GetSize().x )     // Build an horizontal equiv segment
        {
            segm_width   = aRefPad->GetSize().y;
            m_segmLength = aRefPad->GetSize().x - aRefPad->GetSize().y;
        }
        else        // Vertical oval: build an horizontal equiv segment and rotate 90.0 deg
        {
            segm_width   = aRefPad->GetSize().x;
            m_segmLength = aRefPad->GetSize().y - aRefPad->GetSize().x;
            m_segmAngle += 900;
        }

        /* the start point must be 0,0 and currently relativePadPos
         * is relative the center of pad coordinate */
        wxPoint segstart;
        segstart.x = -m_segmLength / 2;                 // Start point coordinate of the horizontal equivalent segment

        RotatePoint( &segstart, m_segmAngle );          // actual start point coordinate of the equivalent segment
        // Calculate segment end position relative to the segment origin
        m_segmEnd.x = -2 * segstart.x;
        m_segmEnd.y = -2 * segstart.y;

        // Recalculate the equivalent segment angle in 0,1 degrees
        // to prepare a call to checkClearanceSegmToPad()
        m_segmAngle = ArcTangente( m_segmEnd.y, m_segmEnd.x );

        // move pad position relative to the segment origin
        m_padToTestPos = relativePadPos - segstart;

        // Use segment to pad check to test the second pad:
        diag = checkClearanceSegmToPad( aPad, segm_width, dist_min );
        break;
    }

    case PAD_SHAPE_TRAPEZOID:

        // at this point, aPad is also a trapezoid, because all other shapes
        // have priority, and are already tested
        wxASSERT( aPad->GetShape() == PAD_SHAPE_TRAPEZOID );
        {
            wxPoint polyref[4];         // Shape of aRefPad
            wxPoint polycompare[4];     // Shape of aPad
            aRefPad->BuildPadPolygon( polyref, wxSize( 0, 0 ), aRefPad->GetOrientation() );
            aPad->BuildPadPolygon( polycompare, wxSize( 0, 0 ), aPad->GetOrientation() );

            // Move aPad shape to relativePadPos
            for( int ii = 0; ii < 4; ii++ )
                polycompare[ii] += relativePadPos;

            // And now test polygons:
            if( !trapezoid2trapezoidDRC( polyref, polycompare, dist_min ) )
                diag = false;
        }
        break;

    default:
        wxLogDebug( wxT( "DRC::checkClearancePadToPad: unexpected pad shape" ) );
        break;
    }

    return diag;
}


/* test if distance between a segment is > aMinDist
 * segment start point is assumed in (0,0) and  segment start point in m_segmEnd
 * and its orientation is m_segmAngle (m_segmAngle must be already initialized)
 * and have aSegmentWidth.
 */
bool DRC::checkClearanceSegmToPad( const D_PAD* aPad, int aSegmentWidth, int aMinDist )
{
    wxSize  padHalfsize;            // half dimension of the pad
    wxPoint startPoint, endPoint;

    int segmHalfWidth = aSegmentWidth / 2;
    int distToLine = segmHalfWidth + aMinDist;

    padHalfsize.x = aPad->GetSize().x >> 1;
    padHalfsize.y = aPad->GetSize().y >> 1;

    if( aPad->GetShape() == PAD_SHAPE_TRAPEZOID )     // The size is bigger, due to GetDelta() extra size
    {
        padHalfsize.x += std::abs(aPad->GetDelta().y) / 2;   // Remember: GetDelta().y is the GetSize().x change
        padHalfsize.y += std::abs(aPad->GetDelta().x) / 2;   // Remember: GetDelta().x is the GetSize().y change
    }

    if( aPad->GetShape() == PAD_SHAPE_CIRCLE )
    {
        /* Easy case: just test the distance between segment and pad centre
         * calculate pad coordinates in the X,Y axis with X axis = segment to test
         */
        RotatePoint( &m_padToTestPos, m_segmAngle );
        return checkMarginToCircle( m_padToTestPos, distToLine + padHalfsize.x, m_segmLength );
    }

    /* calculate the bounding box of the pad, including the clearance and the segment width
     * if the line from 0 to m_segmEnd does not intersect this bounding box,
     * the clearance is always OK
     * But if intersect, a better analysis of the pad shape must be done.
     */
    m_xcliplo = m_padToTestPos.x - distToLine - padHalfsize.x;
    m_ycliplo = m_padToTestPos.y - distToLine - padHalfsize.y;
    m_xcliphi = m_padToTestPos.x + distToLine + padHalfsize.x;
    m_ycliphi = m_padToTestPos.y + distToLine + padHalfsize.y;

    startPoint.x = startPoint.y = 0;
    endPoint     = m_segmEnd;

    double orient = aPad->GetOrientation();

    RotatePoint( &startPoint, m_padToTestPos, -orient );
    RotatePoint( &endPoint, m_padToTestPos, -orient );

    if( checkLine( startPoint, endPoint ) )
        return true;

    /* segment intersects the bounding box. But there is not always a DRC error.
     * A fine analysis of the pad shape must be done.
     */
    switch( aPad->GetShape() )
    {
    default:
        return false;

    case PAD_SHAPE_OVAL:
    {
        /* an oval is a complex shape, but is a rectangle and 2 circles
         * these 3 basic shapes are more easy to test.
         *
         * In calculations we are using a vertical oval shape
         * (i.e. a vertical rounded segment)
         * for horizontal oval shapes, swap x and y size and rotate the shape
         */
        if( padHalfsize.x > padHalfsize.y )
        {
            std::swap( padHalfsize.x, padHalfsize.y );
            orient = AddAngles( orient, 900 );
        }

        // here, padHalfsize.x is the radius of rounded ends.

        int deltay = padHalfsize.y - padHalfsize.x;
        // here: padHalfsize.x = radius,
        // deltay = dist between the centre pad and the centre of a rounded end

        // Test the rectangular area between the two circles (the rounded ends)
        m_xcliplo = m_padToTestPos.x - distToLine - padHalfsize.x;
        m_ycliplo = m_padToTestPos.y - deltay;
        m_xcliphi = m_padToTestPos.x + distToLine + padHalfsize.x;
        m_ycliphi = m_padToTestPos.y + deltay;

        if( !checkLine( startPoint, endPoint ) )
        {
            return false;
        }

        // test the first circle
        startPoint.x = m_padToTestPos.x;         // startPoint = centre of the upper circle of the oval shape
        startPoint.y = m_padToTestPos.y + deltay;

        // Calculate the actual position of the circle, given the pad orientation:
        RotatePoint( &startPoint, m_padToTestPos, orient );

        // Calculate the actual position of the circle in the new X,Y axis:
        RotatePoint( &startPoint, m_segmAngle );

        if( !checkMarginToCircle( startPoint, padHalfsize.x + distToLine, m_segmLength ) )
        {
            return false;
        }

        // test the second circle
        startPoint.x = m_padToTestPos.x;         // startPoint = centre of the lower circle of the oval shape
        startPoint.y = m_padToTestPos.y - deltay;
        RotatePoint( &startPoint, m_padToTestPos, orient );
        RotatePoint( &startPoint, m_segmAngle );

        if( !checkMarginToCircle( startPoint, padHalfsize.x + distToLine, m_segmLength ) )
        {
            return false;
        }
    }
        break;

    case PAD_SHAPE_RECT:
        // the area to test is a rounded rectangle.
        // this can be done by testing 2 rectangles and 4 circles (the corners)

        // Testing the first rectangle dimx + distToLine, dimy:
        m_xcliplo = m_padToTestPos.x - padHalfsize.x - distToLine;
        m_ycliplo = m_padToTestPos.y - padHalfsize.y;
        m_xcliphi = m_padToTestPos.x + padHalfsize.x + distToLine;
        m_ycliphi = m_padToTestPos.y + padHalfsize.y;

        if( !checkLine( startPoint, endPoint ) )
            return false;

        // Testing the second rectangle dimx , dimy + distToLine
        m_xcliplo = m_padToTestPos.x - padHalfsize.x;
        m_ycliplo = m_padToTestPos.y - padHalfsize.y - distToLine;
        m_xcliphi = m_padToTestPos.x + padHalfsize.x;
        m_ycliphi = m_padToTestPos.y + padHalfsize.y + distToLine;

        if( !checkLine( startPoint, endPoint ) )
            return false;

        // testing the 4 circles which are the clearance area of each corner:

        // testing the left top corner of the rectangle
        startPoint.x = m_padToTestPos.x - padHalfsize.x;
        startPoint.y = m_padToTestPos.y - padHalfsize.y;
        RotatePoint( &startPoint, m_padToTestPos, orient );
        RotatePoint( &startPoint, m_segmAngle );

        if( !checkMarginToCircle( startPoint, distToLine, m_segmLength ) )
            return false;

        // testing the right top corner of the rectangle
        startPoint.x = m_padToTestPos.x + padHalfsize.x;
        startPoint.y = m_padToTestPos.y - padHalfsize.y;
        RotatePoint( &startPoint, m_padToTestPos, orient );
        RotatePoint( &startPoint, m_segmAngle );

        if( !checkMarginToCircle( startPoint, distToLine, m_segmLength ) )
            return false;

        // testing the left bottom corner of the rectangle
        startPoint.x = m_padToTestPos.x - padHalfsize.x;
        startPoint.y = m_padToTestPos.y + padHalfsize.y;
        RotatePoint( &startPoint, m_padToTestPos, orient );
        RotatePoint( &startPoint, m_segmAngle );

        if( !checkMarginToCircle( startPoint, distToLine, m_segmLength ) )
            return false;

        // testing the right bottom corner of the rectangle
        startPoint.x = m_padToTestPos.x + padHalfsize.x;
        startPoint.y = m_padToTestPos.y + padHalfsize.y;
        RotatePoint( &startPoint, m_padToTestPos, orient );
        RotatePoint( &startPoint, m_segmAngle );

        if( !checkMarginToCircle( startPoint, distToLine, m_segmLength ) )
            return false;

        break;

    case PAD_SHAPE_TRAPEZOID:
    {
        wxPoint poly[4];
        aPad->BuildPadPolygon( poly, wxSize( 0, 0 ), orient );

        // Move shape to m_padToTestPos
        for( int ii = 0; ii < 4; ii++ )
        {
            poly[ii] += m_padToTestPos;
            RotatePoint( &poly[ii], m_segmAngle );
        }

        if( !trapezoid2segmentDRC( poly, wxPoint( 0, 0 ), wxPoint(m_segmLength,0), distToLine ) )
            return false;
    }
    break;
    }

    return true;
}


/**
 * Helper function checkMarginToCircle
 * Check the distance between a circle (round pad, via or round end of track)
 * and a segment. the segment is expected starting at 0,0, and on the X axis
 * return true if distance >= aRadius
 */
bool DRC::checkMarginToCircle( wxPoint aCentre, int aRadius, int aLength )
{
    if( abs( aCentre.y ) > aRadius )     // trivial case
        return true;

    // Here, distance between aCentre and X axis is < aRadius
    if( (aCentre.x >= -aRadius ) && ( aCentre.x <= (aLength + aRadius) ) )
    {
        if( (aCentre.x >= 0) && (aCentre.x <= aLength) )
            return false;           // aCentre is between the starting point and the ending point of the segm

        if( aCentre.x > aLength )   // aCentre is after the ending point
            aCentre.x -= aLength;   // move aCentre to the starting point of the segment

        if( EuclideanNorm( aCentre ) < aRadius )
            // distance between aCentre and the starting point or the ending point is < aRadius
            return false;
    }

    return true;
}


// Helper function used in checkLine::
static inline int USCALE( unsigned arg, unsigned num, unsigned den )
{
    int ii;

    ii = KiROUND( ( (double) arg * num ) / den );
    return ii;
}


/** Helper function checkLine
 * Test if a line intersects a bounding box (a rectangle)
 * The rectangle is defined by m_xcliplo, m_ycliplo and m_xcliphi, m_ycliphi
 * return true if the line from aSegStart to aSegEnd is outside the bounding box
 */
bool DRC::checkLine( wxPoint aSegStart, wxPoint aSegEnd )
{
#define WHEN_OUTSIDE return true
#define WHEN_INSIDE
    int temp;

    if( aSegStart.x > aSegEnd.x )
        std::swap( aSegStart, aSegEnd );

    if( (aSegEnd.x < m_xcliplo) || (aSegStart.x > m_xcliphi) )
    {
        WHEN_OUTSIDE;
    }

    if( aSegStart.y < aSegEnd.y )
    {
        if( (aSegEnd.y < m_ycliplo) || (aSegStart.y > m_ycliphi) )
        {
            WHEN_OUTSIDE;
        }

        if( aSegStart.y < m_ycliplo )
        {
            temp = USCALE( (aSegEnd.x - aSegStart.x), (m_ycliplo - aSegStart.y),
                           (aSegEnd.y - aSegStart.y) );

            if( (aSegStart.x += temp) > m_xcliphi )
            {
                WHEN_OUTSIDE;
            }

            aSegStart.y = m_ycliplo;
            WHEN_INSIDE;
        }

        if( aSegEnd.y > m_ycliphi )
        {
            temp = USCALE( (aSegEnd.x - aSegStart.x), (aSegEnd.y - m_ycliphi),
                           (aSegEnd.y - aSegStart.y) );

            if( (aSegEnd.x -= temp) < m_xcliplo )
            {
                WHEN_OUTSIDE;
            }

            aSegEnd.y = m_ycliphi;
            WHEN_INSIDE;
        }

        if( aSegStart.x < m_xcliplo )
        {
            temp = USCALE( (aSegEnd.y - aSegStart.y), (m_xcliplo - aSegStart.x),
                           (aSegEnd.x - aSegStart.x) );
            aSegStart.y += temp;
            aSegStart.x  = m_xcliplo;
            WHEN_INSIDE;
        }

        if( aSegEnd.x > m_xcliphi )
        {
            temp = USCALE( (aSegEnd.y - aSegStart.y), (aSegEnd.x - m_xcliphi),
                           (aSegEnd.x - aSegStart.x) );
            aSegEnd.y -= temp;
            aSegEnd.x  = m_xcliphi;
            WHEN_INSIDE;
        }
    }
    else
    {
        if( (aSegStart.y < m_ycliplo) || (aSegEnd.y > m_ycliphi) )
        {
            WHEN_OUTSIDE;
        }

        if( aSegStart.y > m_ycliphi )
        {
            temp = USCALE( (aSegEnd.x - aSegStart.x), (aSegStart.y - m_ycliphi),
                           (aSegStart.y - aSegEnd.y) );

            if( (aSegStart.x += temp) > m_xcliphi )
            {
                WHEN_OUTSIDE;
            }

            aSegStart.y = m_ycliphi;
            WHEN_INSIDE;
        }

        if( aSegEnd.y < m_ycliplo )
        {
            temp = USCALE( (aSegEnd.x - aSegStart.x), (m_ycliplo - aSegEnd.y),
                           (aSegStart.y - aSegEnd.y) );

            if( (aSegEnd.x -= temp) < m_xcliplo )
            {
                WHEN_OUTSIDE;
            }

            aSegEnd.y = m_ycliplo;
            WHEN_INSIDE;
        }

        if( aSegStart.x < m_xcliplo )
        {
            temp = USCALE( (aSegStart.y - aSegEnd.y), (m_xcliplo - aSegStart.x),
                           (aSegEnd.x - aSegStart.x) );
            aSegStart.y -= temp;
            aSegStart.x  = m_xcliplo;
            WHEN_INSIDE;
        }

        if( aSegEnd.x > m_xcliphi )
        {
            temp = USCALE( (aSegStart.y - aSegEnd.y), (aSegEnd.x - m_xcliphi),
                           (aSegEnd.x - aSegStart.x) );
            aSegEnd.y += temp;
            aSegEnd.x  = m_xcliphi;
            WHEN_INSIDE;
        }
    }

    if( ( (aSegEnd.x + aSegStart.x) / 2 <= m_xcliphi )
       && ( (aSegEnd.x + aSegStart.x) / 2 >= m_xcliplo ) \
       && ( (aSegEnd.y + aSegStart.y) / 2 <= m_ycliphi )
       && ( (aSegEnd.y + aSegStart.y) / 2 >= m_ycliplo ) )
    {
        return false;
    }
    else
    {
        return true;
    }
}
