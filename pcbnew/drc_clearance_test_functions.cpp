/**
 * @file drc_clearance_test_functions.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * DRC control: these functions make a DRC between pads, tracks and pads versus tracks
 */

#include <fctsys.h>
#include <pcb_edit_frame.h>
#include <trigo.h>

#include <pcbnew.h>
#include <drc.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_zone.h>
#include <class_marker_pcb.h>
#include <math_for_graphics.h>
#include <polygon_test_point_inside.h>
#include <convert_basic_shapes_to_polygon.h>
#include <board_commit.h>


/* compare 2 convex polygons and return true if distance > aDist
 * i.e if for each edge of the first polygon distance from each edge of the other polygon
 * is >= aDist
 */
bool poly2polyDRC( wxPoint* aTref, int aTrefCount,
                       wxPoint* aTcompare, int aTcompareCount, int aDist )
{
    /* Test if one polygon is contained in the other and thus the polygon overlap.
     * This case is not covered by the following check if one polygone is
     * completely contained in the other (because edges don't intersect)!
     */
    if( TestPointInsidePolygon( aTref, aTrefCount, aTcompare[0] ) )
        return false;

    if( TestPointInsidePolygon( aTcompare, aTcompareCount, aTref[0] ) )
        return false;

    for( int ii = 0, jj = aTrefCount - 1; ii < aTrefCount; jj = ii, ii++ )
    {   // for all edges in aTref
        for( int kk = 0, ll = aTcompareCount - 1; kk < aTcompareCount; ll = kk, kk++ )
        {   // for all edges in aTcompare
            double d;
            int    intersect = TestForIntersectionOfStraightLineSegments(
                                aTref[ii].x, aTref[ii].y, aTref[jj].x, aTref[jj].y,
                                aTcompare[kk].x, aTcompare[kk].y, aTcompare[ll].x, aTcompare[ll].y,
                                NULL, NULL, &d );

            if( intersect || ( d < aDist ) )
                return false;
        }
    }

    return true;
}

/* compare a trapezoids (can be rectangle) and a segment and return true if distance > aDist
 */
bool poly2segmentDRC( wxPoint* aTref, int aTrefCount, wxPoint aSegStart, wxPoint aSegEnd, int aDist )
{
    /* Test if the segment is contained in the polygon.
     * This case is not covered by the following check if the segment is
     * completely contained in the polygon (because edges don't intersect)!
     */
    if( TestPointInsidePolygon( aTref, aTrefCount, aSegStart ) )
        return false;

    for( int ii = 0, jj = aTrefCount-1; ii < aTrefCount; jj = ii, ii++ )
    {   // for all edges in polygon
        double d;
        int    intersect = TestForIntersectionOfStraightLineSegments(
                                aTref[ii].x, aTref[ii].y, aTref[jj].x, aTref[jj].y,
                                aSegStart.x, aSegStart.y, aSegEnd.x, aSegEnd.y,
                                NULL, NULL, &d );

        if( intersect || ( d < aDist) )
            return false;
    }

    return true;
}

/* compare a polygon to a point and return true if distance > aDist
 * do not use this function for horizontal or vertical rectangles
 * because there is a faster an easier way to compare the distance
 */
bool convex2pointDRC( wxPoint* aTref, int aTrefCount, wxPoint aPcompare, int aDist )
{
    /* Test if aPcompare point is contained in the polygon.
     * This case is not covered by the following check if this point is inside the polygon
     */
    if( TestPointInsidePolygon( aTref, aTrefCount, aPcompare ) )
    {
        return false;
    }

    // Test distance between aPcompare and each segment of the polygon:
    for( int ii = 0, jj = aTrefCount - 1; ii < aTrefCount; jj = ii, ii++ )  // for all edge in polygon
    {
        if( TestSegmentHit( aPcompare, aTref[ii], aTref[jj], aDist ) )
            return false;
    }

    return true;
}


bool DRC::doTrackDrc( TRACK* aRefSeg, TRACK* aStart, bool aTestPads, bool aTestZones )
{
    TRACK*    track;
    wxPoint   delta;           // length on X and Y axis of segments
    LSET layerMask;
    int       net_code_ref;
    wxPoint   shape_pos;

    std::vector<MARKER_PCB*> markers;

    auto commitMarkers = [&]()
    {
        // In legacy routing mode, do not add markers to the board.
        // only shows the drc error message
        if( m_drcInLegacyRoutingMode )
        {
            while( markers.size() > 0 )
            {
                m_pcbEditorFrame->SetMsgPanel( markers.back() );
                delete  markers.back();
                markers.pop_back();
            }
        }
        else
        {
            BOARD_COMMIT commit( m_pcbEditorFrame );

            for( auto marker : markers )
                commit.Add( marker );

            commit.Push( wxEmptyString, false, false );
        }
    };

    // Returns false if we should return false from call site, or true to continue
    auto handleNewMarker = [&]() -> bool
    {
        if( !m_reportAllTrackErrors )
        {
            if( markers.size() > 0 )
                commitMarkers();

            return false;
        }
        else
            return true;
    };

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

    /******************************************/
    /* Phase 0 : via DRC tests :              */
    /******************************************/

    if( aRefSeg->Type() == PCB_VIA_T )
    {
        VIA *refvia = static_cast<VIA*>( aRefSeg );
        wxPoint refviaPos = refvia->GetPosition();

        // test if the via size is smaller than minimum
        if( refvia->GetViaType() == VIA_MICROVIA )
        {
            if( refvia->GetWidth() < dsnSettings.m_MicroViasMinSize )
            {
                markers.push_back(
                        m_markerFactory.NewMarker( refviaPos, refvia, DRCE_TOO_SMALL_MICROVIA ) );

                if( !handleNewMarker() )
                    return false;
            }

            if( refvia->GetDrillValue() < dsnSettings.m_MicroViasMinDrill )
            {
                markers.push_back( m_markerFactory.NewMarker(
                        refviaPos, refvia, DRCE_TOO_SMALL_MICROVIA_DRILL ) );

                if( !handleNewMarker() )
                    return false;
            }
        }
        else
        {
            if( refvia->GetWidth() < dsnSettings.m_ViasMinSize )
            {
                markers.push_back(
                        m_markerFactory.NewMarker( refviaPos, refvia, DRCE_TOO_SMALL_VIA ) );

                if( !handleNewMarker() )
                    return false;
            }

            if( refvia->GetDrillValue() < dsnSettings.m_ViasMinDrill )
            {
                markers.push_back(
                        m_markerFactory.NewMarker( refviaPos, refvia, DRCE_TOO_SMALL_VIA_DRILL ) );

                if( !handleNewMarker() )
                    return false;
            }
        }

        // test if via's hole is bigger than its diameter
        // This test is necessary since the via hole size and width can be modified
        // and a default via hole can be bigger than some vias sizes
        if( refvia->GetDrillValue() > refvia->GetWidth() )
        {
            markers.push_back(
                    m_markerFactory.NewMarker( refviaPos, refvia, DRCE_VIA_HOLE_BIGGER ) );

            if( !handleNewMarker() )
                return false;
        }

        // test if the type of via is allowed due to design rules
        if( refvia->GetViaType() == VIA_MICROVIA && !dsnSettings.m_MicroViasAllowed )
        {
            markers.push_back(
                    m_markerFactory.NewMarker( refviaPos, refvia, DRCE_MICRO_VIA_NOT_ALLOWED ) );
            if( !handleNewMarker() )
                return false;
        }

        // test if the type of via is allowed due to design rules
        if( refvia->GetViaType() == VIA_BLIND_BURIED && !dsnSettings.m_BlindBuriedViaAllowed )
        {
            markers.push_back(
                    m_markerFactory.NewMarker( refviaPos, refvia, DRCE_BURIED_VIA_NOT_ALLOWED ) );

            if( !handleNewMarker() )
                return false;
        }

        // For microvias: test if they are blind vias and only between 2 layers
        // because they are used for very small drill size and are drill by laser
        // and **only one layer** can be drilled
        if( refvia->GetViaType() == VIA_MICROVIA )
        {
            PCB_LAYER_ID    layer1, layer2;
            bool        err = true;

            refvia->LayerPair( &layer1, &layer2 );

            if( layer1 > layer2 )
                std::swap( layer1, layer2 );

            if( layer2 == B_Cu && layer1 == dsnSettings.GetCopperLayerCount() - 2 )
                err = false;
            else if( layer1 == F_Cu  &&  layer2 == In1_Cu  )
                err = false;
            else if( static_cast<int>( layer2 ) - static_cast<int>( layer1 ) == 1 )
                err = false;

            if( err )
            {
                markers.push_back( m_markerFactory.NewMarker(
                        refviaPos, refvia, DRCE_MICRO_VIA_INCORRECT_LAYER_PAIR ) );

                if( !handleNewMarker() )
                    return false;
            }
        }

    }
    else    // This is a track segment
    {
        if( aRefSeg->GetWidth() < dsnSettings.m_TrackMinWidth )
        {
            wxPoint refsegMiddle = ( aRefSeg->GetStart() + aRefSeg->GetEnd() ) / 2;

            markers.push_back( m_markerFactory.NewMarker(
                    refsegMiddle, aRefSeg, DRCE_TOO_SMALL_TRACK_WIDTH ) );

            if( !handleNewMarker() )
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
    if( aTestPads )
    {
        unsigned pad_count = m_pcb->GetPadCount();

        auto pads = m_pcb->GetPads();

        for( unsigned ii = 0; ii < pad_count; ++ii )
        {
            D_PAD* pad = pads[ii];
            SEG padSeg( pad->GetPosition(), pad->GetPosition() );


            /* No problem if pads are on another layer,
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
                dummypad.SetShape( pad->GetDrillShape() == PAD_DRILL_SHAPE_OBLONG ?
                                   PAD_SHAPE_OVAL : PAD_SHAPE_CIRCLE );
                dummypad.SetOrientation( pad->GetOrientation() );

                m_padToTestPos = dummypad.GetPosition() - origin;

                if( !checkClearanceSegmToPad( &dummypad, aRefSeg->GetWidth(),
                                              netclass->GetClearance() ) )
                {
                    markers.push_back( m_markerFactory.NewMarker(
                            aRefSeg, pad, padSeg, DRCE_TRACK_NEAR_THROUGH_HOLE ) );

                    if( !handleNewMarker() )
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
                markers.push_back(
                        m_markerFactory.NewMarker( aRefSeg, pad, padSeg, DRCE_TRACK_NEAR_PAD ) );

                if( !handleNewMarker() )
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
        w_dist += ( aRefSeg->GetWidth() + track->GetWidth() ) / 2;

        // Due to many double to int conversions during calculations, which
        // create rounding issues,
        // the exact clearance margin cannot be really known.
        // To avoid false bad DRC detection due to these rounding issues,
        // slightly decrease the w_dist (remove one nanometer is enough !)
        w_dist -= 1;

        // If the reference segment is a via, we test it here
        if( aRefSeg->Type() == PCB_VIA_T )
        {
            delta = track->GetEnd() - track->GetStart();
            segStartPoint = aRefSeg->GetStart() - track->GetStart();
            wxPoint pos = aRefSeg->GetPosition();

            if( track->Type() == PCB_VIA_T )
            {
                // Test distance between two vias, i.e. two circles, trivial case
                if( EuclideanNorm( segStartPoint ) < w_dist )
                {
                    markers.push_back(
                            m_markerFactory.NewMarker( pos, aRefSeg, track, DRCE_VIA_NEAR_VIA ) );

                    if( !handleNewMarker() )
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
                    markers.push_back(
                            m_markerFactory.NewMarker( pos, aRefSeg, track, DRCE_VIA_NEAR_TRACK ) );

                    if( !handleNewMarker() )
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

        SEG seg( segStartPoint, segEndPoint );

        if( track->Type() == PCB_VIA_T )
        {
            if( checkMarginToCircle( segStartPoint, w_dist, m_segmLength ) )
                continue;

            markers.push_back(
                    m_markerFactory.NewMarker( aRefSeg, track, seg, DRCE_TRACK_NEAR_VIA ) );

            if( !handleNewMarker() )
                return false;
        }

        /*	We have changed axis:
         *  the reference segment is Horizontal.
         *  3 cases : the segment to test can be parallel, perpendicular or have another direction
         */
        if( segStartPoint.y == segEndPoint.y ) // parallel segments
        {
            if( abs( segStartPoint.y ) >= w_dist )
                continue;

            // Ensure segStartPoint.x <= segEndPoint.x
            if( segStartPoint.x > segEndPoint.x )
                std::swap( segStartPoint.x, segEndPoint.x );

            if( segStartPoint.x > ( -w_dist ) && segStartPoint.x < ( m_segmLength + w_dist ) )
            {
                // the start point is inside the reference range
                //      X........
                //    O--REF--+

                // Fine test : we consider the rounded shape of each end of the track segment:
                if( segStartPoint.x >= 0 && segStartPoint.x <= m_segmLength )
                {
                    markers.push_back(
                            m_markerFactory.NewMarker( aRefSeg, track, seg, DRCE_TRACK_ENDS1 ) );

                    if( !handleNewMarker() )
                        return false;
                }

                if( !checkMarginToCircle( segStartPoint, w_dist, m_segmLength ) )
                {
                    markers.push_back(
                            m_markerFactory.NewMarker( aRefSeg, track, seg, DRCE_TRACK_ENDS2 ) );

                    if( !handleNewMarker() )
                        return false;
                }
            }

            if( segEndPoint.x > ( -w_dist ) && segEndPoint.x < ( m_segmLength + w_dist ) )
            {
                // the end point is inside the reference range
                //  .....X
                //    O--REF--+
                // Fine test : we consider the rounded shape of the ends
                if( segEndPoint.x >= 0 && segEndPoint.x <= m_segmLength )
                {
                    markers.push_back(
                            m_markerFactory.NewMarker( aRefSeg, track, seg, DRCE_TRACK_ENDS3 ) );

                    if( !handleNewMarker() )
                        return false;
                }

                if( !checkMarginToCircle( segEndPoint, w_dist, m_segmLength ) )
                {
                    markers.push_back(
                            m_markerFactory.NewMarker( aRefSeg, track, seg, DRCE_TRACK_ENDS4 ) );

                    if( !handleNewMarker() )
                        return false;
                }
            }

            if( segStartPoint.x <= 0 && segEndPoint.x >= 0 )
            {
                // the segment straddles the reference range (this actually only
                // checks if it straddles the origin, because the other cases where already
                // handled)
                //  X.............X
                //    O--REF--+
                markers.push_back( m_markerFactory.NewMarker(
                        aRefSeg, track, seg, DRCE_TRACK_SEGMENTS_TOO_CLOSE ) );

                if( !handleNewMarker() )
                    return false;
            }
        }
        else if( segStartPoint.x == segEndPoint.x ) // perpendicular segments
        {
            if( segStartPoint.x <= -w_dist || segStartPoint.x >= m_segmLength + w_dist )
                continue;

            // Test if segments are crossing
            if( segStartPoint.y > segEndPoint.y )
                std::swap( segStartPoint.y, segEndPoint.y );

            if( ( segStartPoint.y < 0 ) && ( segEndPoint.y > 0 ) )
            {
                MARKER_PCB* m = m_markerFactory.NewMarker( aRefSeg, track, seg,
                                                           DRCE_TRACKS_CROSSING );
                m->SetPosition( wxPoint( track->GetStart().x, aRefSeg->GetStart().y ) );
                markers.push_back( m );

                if( !handleNewMarker() )
                    return false;
            }

            // At this point the drc error is due to an end near a reference segm end
            if( !checkMarginToCircle( segStartPoint, w_dist, m_segmLength ) )
            {
                markers.push_back(
                        m_markerFactory.NewMarker( aRefSeg, track, seg, DRCE_ENDS_PROBLEM1 ) );

                if( !handleNewMarker() )
                    return false;
            }
            if( !checkMarginToCircle( segEndPoint, w_dist, m_segmLength ) )
            {
                markers.push_back(
                        m_markerFactory.NewMarker( aRefSeg, track, seg, DRCE_ENDS_PROBLEM2 ) );

                if( !handleNewMarker() )
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
                    wxPoint failurePoint;
                    MARKER_PCB* m;

                    if( SegmentIntersectsSegment( aRefSeg->GetStart(), aRefSeg->GetEnd(),
                                                  track->GetStart(), track->GetEnd(),
                                                  &failurePoint ) )
                    {
                        m = m_markerFactory.NewMarker( aRefSeg, track, seg, DRCE_TRACKS_CROSSING );
                        m->SetPosition( failurePoint );
                    }
                    else
                    {
                        m = m_markerFactory.NewMarker( aRefSeg, track, seg, DRCE_ENDS_PROBLEM3 );
                    }

                    markers.push_back( m );

                    if( !handleNewMarker() )
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

                    // Compute the segment length: delta.x = length after rotation
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
                        markers.push_back( m_markerFactory.NewMarker(
                                aRefSeg, track, seg, DRCE_ENDS_PROBLEM4 ) );

                        if( !handleNewMarker() )
                            return false;
                    }

                    if( !checkMarginToCircle( relEndPos, w_dist, delta.x ) )
                    {
                        markers.push_back( m_markerFactory.NewMarker(
                                aRefSeg, track, seg, DRCE_ENDS_PROBLEM5 ) );

                        if( !handleNewMarker() )
                            return false;
                    }
                }
            }
        }
    }

    /***************************************/
    /* Phase 3: test DRC with copper zones */
    /***************************************/
    // Can be *very* time consumming.
    if( aTestZones )
    {
        SEG refSeg( aRefSeg->GetStart(), aRefSeg->GetEnd() );

        for( ZONE_CONTAINER* zone : m_pcb->Zones() )
        {
            if( zone->GetFilledPolysList().IsEmpty() || zone->GetIsKeepout() )
                continue;

            if( !( layerMask & zone->GetLayerSet() ).any() )
                continue;

            if( zone->GetNetCode() && zone->GetNetCode() == net_code_ref )
                continue;

            int clearance = zone->GetClearance( aRefSeg );
            SHAPE_POLY_SET* outline = const_cast<SHAPE_POLY_SET*>( &zone->GetFilledPolysList() );

            if( outline->Distance( refSeg, aRefSeg->GetWidth() ) < clearance )
                addMarkerToPcb( m_markerFactory.NewMarker( aRefSeg, zone, DRCE_TRACK_NEAR_ZONE ) );
        }
    }

    /***********************************************/
    /* Phase 4: test DRC with to board edge        */
    /***********************************************/
    {
        SEG test_seg( aRefSeg->GetStart(), aRefSeg->GetEnd() );

        // the minimum distance = clearance plus half the reference track
        // width.  Board edges do not have width or clearance values, so we
        // look for simple crossing.
        SEG::ecoord w_dist = aRefSeg->GetClearance() + aRefSeg->GetWidth() / 2;
        w_dist *= w_dist;

        for( auto it = m_board_outlines.IterateSegmentsWithHoles(); it; it++ )
        {
            if( test_seg.SquaredDistance( *it ) < w_dist )
            {
                auto pt = test_seg.NearestPoint( *it );
                markers.push_back( m_markerFactory.NewMarker(
                        wxPoint( pt.x, pt.y ), aRefSeg, DRCE_TRACK_NEAR_EDGE ) );

                if( !handleNewMarker() )
                    return false;
            }
        }
    }


    if( markers.size() > 0 )
    {
        commitMarkers();
        return false;
    }
    else
        return true;
}


bool DRC::doEdgeZoneDrc( ZONE_CONTAINER* aArea, int aCornerIndex )
{
    if( !aArea->IsOnCopperLayer() )    // Cannot have a Drc error if not on copper layer
        return true;
    // Get polygon, contour and vertex index.
    SHAPE_POLY_SET::VERTEX_INDEX index;

    // If the vertex does not exist, there is no conflict
    if( !aArea->Outline()->GetRelativeIndices( aCornerIndex, &index ) )
        return true;

    // Retrieve the selected contour
    SHAPE_LINE_CHAIN contour;
    contour = aArea->Outline()->Polygon( index.m_polygon )[index.m_contour];

    // Retrieve the segment that starts at aCornerIndex-th corner.
    SEG selectedSegment = contour.Segment( index.m_vertex );

    VECTOR2I start = selectedSegment.A;
    VECTOR2I end = selectedSegment.B;

    // iterate through all areas
    for( int ia2 = 0; ia2 < m_pcb->GetAreaCount(); ia2++ )
    {
        ZONE_CONTAINER* area_to_test   = m_pcb->GetArea( ia2 );
        int             zone_clearance = std::max( area_to_test->GetZoneClearance(),
                                                   aArea->GetZoneClearance() );

        // test for same layer
        if( area_to_test->GetLayer() != aArea->GetLayer() )
            continue;

        // Test for same net
        if( ( aArea->GetNetCode() == area_to_test->GetNetCode() ) && (aArea->GetNetCode() >= 0) )
            continue;

        // test for same priority
        if( area_to_test->GetPriority() != aArea->GetPriority() )
            continue;

        // test for same type
        if( area_to_test->GetIsKeepout() != aArea->GetIsKeepout() )
            continue;

        // For keepout, there is no clearance, so use a minimal value for it
        // use 1, not 0 as value to avoid some issues in tests
        if( area_to_test->GetIsKeepout() )
            zone_clearance = 1;

        // test for ending line inside area_to_test
        if( area_to_test->Outline()->Contains( end ) )
        {
            wxPoint pos( end.x, end.y );
            m_currentMarker =
                    m_markerFactory.NewMarker( pos, aArea, area_to_test, DRCE_ZONES_INTERSECT );
            return false;
        }

        // now test spacing between areas
        int ax1    = start.x;
        int ay1    = start.y;
        int ax2    = end.x;
        int ay2    = end.y;

        // Iterate through all edges in the polygon.
        SHAPE_POLY_SET::SEGMENT_ITERATOR iterator;
        for( iterator = area_to_test->Outline()->IterateSegmentsWithHoles(); iterator; iterator++ )
        {
            SEG segment = *iterator;

            int bx1 = segment.A.x;
            int by1 = segment.A.y;
            int bx2 = segment.B.x;
            int by2 = segment.B.y;

            int x, y;   // variables containing the intersecting point coordinates
            int d = GetClearanceBetweenSegments( bx1, by1, bx2, by2,
                                                 0,
                                                 ax1, ay1, ax2, ay2,
                                                 0,
                                                 zone_clearance,
                                                 &x, &y );

            if( d < zone_clearance )
            {
                // COPPERAREA_COPPERAREA error : edge intersect or too close
                m_currentMarker = m_markerFactory.NewMarker(
                        wxPoint( x, y ), aArea, area_to_test, DRCE_ZONES_TOO_CLOSE );
                return false;
            }

        }
    }

    return true;
}


bool DRC::checkClearancePadToPad( D_PAD* aRefPad, D_PAD* aPad )
{
    int     dist;
    double pad_angle;

    // Get the clearance between the 2 pads. this is the min distance between aRefPad and aPad
    int     dist_min = aRefPad->GetClearance( aPad );

    // relativePadPos is the aPad shape position relative to the aRefPad shape position
    wxPoint relativePadPos = aPad->ShapePos() - aRefPad->ShapePos();

    dist = KiROUND( EuclideanNorm( relativePadPos ) );

    // Quick test: Clearance is OK if the bounding circles are further away than "dist_min"
    int delta = dist - aRefPad->GetBoundingRadius() - aPad->GetBoundingRadius();

    if( delta >= dist_min )
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
    // Note also a ROUNDRECT pad with a corner radius = r can be considered as
    // a smaller RECT (size - 2*r) with a clearance increased by r
    // priority is aRefPad = ROUND then OVAL then RECT/ROUNDRECT then other
    if( aRefPad->GetShape() != aPad->GetShape() && aRefPad->GetShape() != PAD_SHAPE_CIRCLE )
    {
        // pad ref shape is here oval, rect, roundrect, trapezoid or custom
        switch( aPad->GetShape() )
        {
            case PAD_SHAPE_CIRCLE:
                swap_pads = true;
                break;

            case PAD_SHAPE_OVAL:
                swap_pads = true;
                break;

            case PAD_SHAPE_RECT:
            case PAD_SHAPE_ROUNDRECT:
                if( aRefPad->GetShape() != PAD_SHAPE_OVAL )
                    swap_pads = true;
                break;

            case PAD_SHAPE_TRAPEZOID:
            case PAD_SHAPE_CUSTOM:
                break;
        }
    }

    if( swap_pads )
    {
        std::swap( aRefPad, aPad );
        relativePadPos = -relativePadPos;
    }

    // corners of aRefPad (used only for rect/roundrect/trap pad)
    wxPoint polyref[4];
    // corners of aRefPad (used only for custom pad)
    SHAPE_POLY_SET polysetref;

    // corners of aPad (used only for rect/roundrect/trap pad)
    wxPoint polycompare[4];
    // corners of aPad (used only custom pad)
    SHAPE_POLY_SET polysetcompare;

    /* Because pad exchange, aRefPad shape is PAD_SHAPE_CIRCLE or PAD_SHAPE_OVAL,
     * if one of the 2 pads was a PAD_SHAPE_CIRCLE or PAD_SHAPE_OVAL.
     * Therefore, if aRefPad is a PAD_SHAPE_RECT, PAD_SHAPE_ROUNDRECT or a PAD_SHAPE_TRAPEZOID,
     * aPad is also a PAD_SHAPE_RECT, PAD_SHAPE_ROUNDRECT or a PAD_SHAPE_TRAPEZOID
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

    case PAD_SHAPE_TRAPEZOID:
    case PAD_SHAPE_ROUNDRECT:
    case PAD_SHAPE_RECT:
    case PAD_SHAPE_CUSTOM:
        // pad_angle = pad orient relative to the aRefPad orient
        pad_angle = aRefPad->GetOrientation() + aPad->GetOrientation();
        NORMALIZE_ANGLE_POS( pad_angle );

        if( aRefPad->GetShape() == PAD_SHAPE_ROUNDRECT )
        {
            int padRadius = aRefPad->GetRoundRectCornerRadius();
            dist_min += padRadius;
            GetRoundRectCornerCenters( polyref, padRadius, wxPoint( 0, 0 ),
                                aRefPad->GetSize(), aRefPad->GetOrientation() );
        }
        else if( aRefPad->GetShape() == PAD_SHAPE_CUSTOM )
        {
            polysetref.Append( aRefPad->GetCustomShapeAsPolygon() );

            // The reference pad can be rotated. calculate the rotated
            // coordiantes ( note, the ref pad position is the origin of
            // coordinates for this drc test)
            aRefPad->CustomShapeAsPolygonToBoardPosition( &polysetref,
                        wxPoint( 0, 0 ), aRefPad->GetOrientation() );
        }
        else
        {
            // BuildPadPolygon has meaning for rect a trapeziod shapes
            // and returns the 4 corners
            aRefPad->BuildPadPolygon( polyref, wxSize( 0, 0 ), aRefPad->GetOrientation() );
        }

        switch( aPad->GetShape() )
        {
        case PAD_SHAPE_ROUNDRECT:
        case PAD_SHAPE_RECT:
        case PAD_SHAPE_TRAPEZOID:
        case PAD_SHAPE_CUSTOM:
            if( aPad->GetShape() == PAD_SHAPE_ROUNDRECT )
            {
                int padRadius = aPad->GetRoundRectCornerRadius();
                dist_min += padRadius;
                GetRoundRectCornerCenters( polycompare, padRadius, relativePadPos,
                                    aPad->GetSize(), aPad->GetOrientation() );
            }
            else if( aPad->GetShape() == PAD_SHAPE_CUSTOM )
            {
                polysetcompare.Append( aPad->GetCustomShapeAsPolygon() );

                // The pad to compare can be rotated. calculate the rotated
                // coordinattes ( note, the pad to compare position
                // is the relativePadPos for this drc test
                aPad->CustomShapeAsPolygonToBoardPosition( &polysetcompare,
                            relativePadPos, aPad->GetOrientation() );
            }
            else
            {
                aPad->BuildPadPolygon( polycompare, wxSize( 0, 0 ), aPad->GetOrientation() );

                // Move aPad shape to relativePadPos
                for( int ii = 0; ii < 4; ii++ )
                    polycompare[ii] += relativePadPos;
            }
            // And now test polygons: We have 3 cases:
            // one poly is complex and the other is basic (has only 4 corners)
            // both polys are complex
            // both polys are basic (have only 4 corners) the most usual case
            if( polysetref.OutlineCount() && polysetcompare.OutlineCount() == 0)
            {
                const SHAPE_LINE_CHAIN& refpoly = polysetref.COutline( 0 );
                // And now test polygons:
                if( !poly2polyDRC( (wxPoint*) &refpoly.CPoint( 0 ), refpoly.PointCount(),
                            polycompare, 4, dist_min ) )
                    diag = false;
            }
            else if( polysetref.OutlineCount() == 0 && polysetcompare.OutlineCount())
            {
                const SHAPE_LINE_CHAIN& cmppoly = polysetcompare.COutline( 0 );
                // And now test polygons:
                if( !poly2polyDRC( (wxPoint*) &cmppoly.CPoint( 0 ), cmppoly.PointCount(),
                            polyref, 4, dist_min ) )
                    diag = false;
            }
            else if( polysetref.OutlineCount() && polysetcompare.OutlineCount() )
            {
                const SHAPE_LINE_CHAIN& refpoly = polysetref.COutline( 0 );
                const SHAPE_LINE_CHAIN& cmppoly = polysetcompare.COutline( 0 );

                // And now test polygons:
                if( !poly2polyDRC( (wxPoint*) &refpoly.CPoint( 0 ), refpoly.PointCount(),
                            (wxPoint*) &cmppoly.CPoint( 0 ), cmppoly.PointCount(), dist_min ) )
                    diag = false;
            }
            else if( !poly2polyDRC( polyref, 4, polycompare, 4, dist_min ) )
                diag = false;
            break;

        default:
            wxLogDebug( wxT( "DRC::checkClearancePadToPad: unexpected pad shape %d" ), aPad->GetShape() );
            break;
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

    default:
        wxLogDebug( wxT( "DRC::checkClearancePadToPad: unknown pad shape" ) );
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
    // Note:
    // we are using a horizontal segment for test, because we know here
    // only the length and orientation+ of the segment
    // Therefore the coordinates of the  shape of pad to compare
    // must be calculated in a axis system rotated by m_segmAngle
    // and centered to the segment origin, before they can be tested
    // against the segment
    // We are using:
    // m_padToTestPos the position of the pad shape in this axis system
    // m_segmAngle the axis system rotation

    int segmHalfWidth = aSegmentWidth / 2;
    int distToLine = segmHalfWidth + aMinDist;

    wxSize  padHalfsize;    // half dimension of the pad

    if( aPad->GetShape() == PAD_SHAPE_CUSTOM )
    {
        // For a custom pad, the pad size has no meaning, we only can
        // use the bounding radius
        padHalfsize.x = padHalfsize.y = aPad->GetBoundingRadius();
    }
    else
    {
        padHalfsize = aPad->GetSize() / 2;
    }

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

    wxPoint startPoint( 0, 0 );
    wxPoint endPoint = m_segmEnd;

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
    case PAD_SHAPE_CIRCLE:
        // This case was already tested, so it cannot be found here.
        // it is here just to avoid compil warning, and to remember
        // it is already tested.
        return false;

    case PAD_SHAPE_OVAL:
    {
        /* an oval is a complex shape, but is a rectangle and 2 circles
         * these 3 basic shapes are more easy to test.
         *
         * In calculations we are using a vertical or horizontal oval shape
         * (i.e. a vertical or horizontal rounded segment)
         */
        wxPoint cstart = m_padToTestPos;
        wxPoint cend = m_padToTestPos;   // center of each circle
        int delta = std::abs( padHalfsize.y - padHalfsize.x );
        int radius = std::min( padHalfsize.y, padHalfsize.x );

        if( padHalfsize.x > padHalfsize.y ) // horizontal equivalent segment
        {
            cstart.x -= delta;
            cend.x += delta;
            // Build the rectangular clearance area between the two circles
            // the rect starts at cstart.x and ends at cend.x and its height
            // is (radius + distToLine)*2
            m_xcliplo = cstart.x;
            m_ycliplo = cstart.y - radius - distToLine;
            m_xcliphi = cend.x;
            m_ycliphi = cend.y + radius + distToLine;
        }
        else    // vertical equivalent segment
        {
            cstart.y -= delta;
            cend.y += delta;
            // Build the rectangular clearance area between the two circles
            // the rect starts at cstart.y and ends at cend.y and its width
            // is (radius + distToLine)*2
            m_xcliplo = cstart.x - distToLine - radius;
            m_ycliplo = cstart.y;
            m_xcliphi = cend.x + distToLine + radius;
            m_ycliphi = cend.y;
        }

        // Test the rectangular clearance area between the two circles (the rounded ends)
        // If the segment legth is zero, only check the endpoints, skip the rectangle
        if( m_segmLength && !checkLine( startPoint, endPoint ) )
        {
            return false;
        }

        // test the first end
        // Calculate the actual position of the circle, given the pad orientation:
        RotatePoint( &cstart, m_padToTestPos, orient );

        // Calculate the actual position of the circle in the new X,Y axis, relative
        // to the segment:
        RotatePoint( &cstart, m_segmAngle );

        if( !checkMarginToCircle( cstart, radius + distToLine, m_segmLength ) )
        {
            return false;
        }

        // test the second end
        RotatePoint( &cend, m_padToTestPos, orient );
        RotatePoint( &cend, m_segmAngle );

        if( !checkMarginToCircle( cend, radius + distToLine, m_segmLength ) )
        {
            return false;
        }
    }
        break;

    case PAD_SHAPE_ROUNDRECT:
        {
        // a round rect is a smaller rect, with a clearance augmented by the corners radius
        int r = aPad->GetRoundRectCornerRadius();
        padHalfsize.x -= r;
        padHalfsize.y -= r;
        distToLine += r;
        }
        // Fall through
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

        if( !poly2segmentDRC( poly, 4, wxPoint( 0, 0 ),
                              wxPoint(m_segmLength,0), distToLine ) )
            return false;
        }
        break;

    case PAD_SHAPE_CUSTOM:
        {
        SHAPE_POLY_SET polyset;
        polyset.Append( aPad->GetCustomShapeAsPolygon() );
        // The pad can be rotated. calculate the coordinates
        // relatives to the segment being tested
        // Note, the pad position relative to the segment origin
        // is m_padToTestPos
        aPad->CustomShapeAsPolygonToBoardPosition( &polyset,
                    m_padToTestPos, orient );

        // Rotate all coordinates by m_segmAngle, because the segment orient
        // is m_segmAngle
        // we are using a horizontal segment for test, because we know here
        // only the lenght and orientation+ of the segment
        // therefore all coordinates of the pad to test must be rotated by
        // m_segmAngle (they are already relative to the segment origin)
        aPad->CustomShapeAsPolygonToBoardPosition( &polyset,
                    wxPoint( 0, 0 ), m_segmAngle );

        const SHAPE_LINE_CHAIN& refpoly = polyset.COutline( 0 );

        if( !poly2segmentDRC( (wxPoint*) &refpoly.CPoint( 0 ),
                              refpoly.PointCount(),
                              wxPoint( 0, 0 ), wxPoint(m_segmLength,0),
                              distToLine ) )
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
    if( abs( aCentre.y ) >= aRadius )     // trivial case
        return true;

    // Here, distance between aCentre and X axis is < aRadius
    if( (aCentre.x > -aRadius ) && ( aCentre.x < (aLength + aRadius) ) )
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
    double result;

    // Trivial check first
    if( !arg || !num)
        return 0;

    // If arg and num are both non-zero but den is zero, we return effective infinite
    if( !den )
        return INT_MAX;

    result = ( (double) arg * num ) / den;

    // Ensure that our result doesn't overflow into the sign bit
    if( result > INT_MAX )
        return INT_MAX;

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

    if( (aSegEnd.x <= m_xcliplo) || (aSegStart.x >= m_xcliphi) )
    {
        WHEN_OUTSIDE;
    }

    if( aSegStart.y < aSegEnd.y )
    {
        if( (aSegEnd.y <= m_ycliplo) || (aSegStart.y >= m_ycliphi) )
        {
            WHEN_OUTSIDE;
        }

        if( aSegStart.y < m_ycliplo )
        {
            temp = USCALE( (aSegEnd.x - aSegStart.x), (m_ycliplo - aSegStart.y),
                           (aSegEnd.y - aSegStart.y) );

            if( (aSegStart.x += temp) >= m_xcliphi )
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

            if( (aSegEnd.x -= temp) <= m_xcliplo )
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
        if( (aSegStart.y <= m_ycliplo) || (aSegEnd.y >= m_ycliphi) )
        {
            WHEN_OUTSIDE;
        }

        if( aSegStart.y > m_ycliphi )
        {
            temp = USCALE( (aSegEnd.x - aSegStart.x), (aSegStart.y - m_ycliphi),
                           (aSegStart.y - aSegEnd.y) );

            if( (aSegStart.x += temp) >= m_xcliphi )
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

            if( (aSegEnd.x -= temp) <= m_xcliplo )
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

    // Do not divide here to avoid rounding errors
    if( ( (aSegEnd.x + aSegStart.x) < m_xcliphi * 2 )
       && ( (aSegEnd.x + aSegStart.x) > m_xcliplo * 2) \
       && ( (aSegEnd.y + aSegStart.y) < m_ycliphi * 2 )
       && ( (aSegEnd.y + aSegStart.y) > m_ycliplo * 2 ) )
    {
        return false;
    }
    else
    {
        return true;
    }
}
