/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_edit_frame.h>
#include <trigo.h>
#include <pcbnew.h>
#include <drc/drc.h>
#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_zone.h>
#include <class_drawsegment.h>
#include <class_marker_pcb.h>
#include <math_for_graphics.h>
#include <geometry/polygon_test_point_inside.h>
#include <convert_basic_shapes_to_polygon.h>
#include <board_commit.h>
#include <math/util.h>      // for KiROUND
#include <geometry/shape_rect.h>
#include <macros.h>


/**
 * compare 2 convex polygons and return true if distance > aDist (if no error DRC)
 * i.e if for each edge of the first polygon distance from each edge of the other polygon
 * is >= aDist
 */
bool poly2polyDRC( wxPoint* aTref, int aTrefCount, wxPoint* aTtest, int aTtestCount,
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


/*
 * compare a trapezoid (can be rectangle) and a segment and return true if distance > aDist
 */
bool poly2segmentDRC( wxPoint* aTref, int aTrefCount, wxPoint aSegStart, wxPoint aSegEnd,
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


void DRC::doTrackDrc( BOARD_COMMIT& aCommit, TRACK* aRefSeg, TRACKS::iterator aStartIt,
                      TRACKS::iterator aEndIt, bool aTestZones )
{
    BOARD_DESIGN_SETTINGS&     bds = m_pcb->GetDesignSettings();

    SEG          refSeg( aRefSeg->GetStart(), aRefSeg->GetEnd() );
    PCB_LAYER_ID refLayer = aRefSeg->GetLayer();
    LSET         refLayerSet = aRefSeg->GetLayerSet();

    EDA_RECT     refSegBB = aRefSeg->GetBoundingBox();
    int          refSegWidth = aRefSeg->GetWidth();


    /******************************************/
    /* Phase 0 : via DRC tests :              */
    /******************************************/

    if( aRefSeg->Type() == PCB_VIA_T )
    {
        VIA *refvia = static_cast<VIA*>( aRefSeg );
        int viaAnnulus = ( refvia->GetWidth() - refvia->GetDrill() ) / 2;
        int minAnnulus = refvia->GetMinAnnulus( &m_clearanceSource );

        // test if the via size is smaller than minimum
        if( refvia->GetViaType() == VIATYPE::MICROVIA )
        {
            if( viaAnnulus < minAnnulus )
            {
                DRC_ITEM* drcItem = new DRC_ITEM( DRCE_TOO_SMALL_VIA_ANNULUS );

                m_msg.Printf( drcItem->GetErrorText() + _( " (%s %s; actual %s)" ),
                              m_clearanceSource,
                              MessageTextFromValue( userUnits(), minAnnulus, true ),
                              MessageTextFromValue( userUnits(), viaAnnulus, true ) );

                drcItem->SetErrorMessage( m_msg );
                drcItem->SetItems( refvia );

                MARKER_PCB* marker = new MARKER_PCB( drcItem, refvia->GetPosition() );
                addMarkerToPcb( aCommit, marker );
            }

            if( refvia->GetWidth() < bds.m_MicroViasMinSize )
            {
                DRC_ITEM* drcItem = new DRC_ITEM( DRCE_TOO_SMALL_MICROVIA );

                m_msg.Printf( drcItem->GetErrorText() + _( " (board minimum %s; actual %s)" ),
                              MessageTextFromValue( userUnits(), bds.m_MicroViasMinSize, true ),
                              MessageTextFromValue( userUnits(), refvia->GetWidth(), true ) );

                drcItem->SetErrorMessage( m_msg );
                drcItem->SetItems( refvia );

                MARKER_PCB* marker = new MARKER_PCB( drcItem, refvia->GetPosition() );
                addMarkerToPcb( aCommit, marker );
            }
        }
        else
        {
            if( bds.m_ViasMinAnnulus > minAnnulus )
            {
                minAnnulus = bds.m_ViasMinAnnulus;
                m_clearanceSource = _( "board minimum" );
            }

            if( viaAnnulus < minAnnulus )
            {
                DRC_ITEM* drcItem = new DRC_ITEM( DRCE_TOO_SMALL_VIA_ANNULUS );

                m_msg.Printf( drcItem->GetErrorText() + _( " (%s %s; actual %s)" ),
                              m_clearanceSource,
                              MessageTextFromValue( userUnits(), minAnnulus, true ),
                              MessageTextFromValue( userUnits(), viaAnnulus, true ) );

                drcItem->SetErrorMessage( m_msg );
                drcItem->SetItems( refvia );

                MARKER_PCB* marker = new MARKER_PCB( drcItem, refvia->GetPosition() );
                addMarkerToPcb( aCommit, marker );
            }

            if( refvia->GetWidth() < bds.m_ViasMinSize )
            {
                DRC_ITEM* drcItem = new DRC_ITEM( DRCE_TOO_SMALL_VIA );

                m_msg.Printf( drcItem->GetErrorText() + _( " (board minimum %s; actual %s)" ),
                              MessageTextFromValue( userUnits(), bds.m_ViasMinSize, true ),
                              MessageTextFromValue( userUnits(), refvia->GetWidth(), true ) );

                drcItem->SetErrorMessage( m_msg );
                drcItem->SetItems( refvia );

                MARKER_PCB* marker = new MARKER_PCB( drcItem, refvia->GetPosition() );
                addMarkerToPcb( aCommit, marker );
            }
        }

        // test if via's hole is bigger than its diameter
        // This test is necessary since the via hole size and width can be modified
        // and a default via hole can be bigger than some vias sizes
        if( refvia->GetDrillValue() > refvia->GetWidth() )
        {
            DRC_ITEM* drcItem = new DRC_ITEM( DRCE_VIA_HOLE_BIGGER );

            m_msg.Printf( drcItem->GetErrorText() + _( " (diameter %s; drill %s)" ),
                          MessageTextFromValue( userUnits(), refvia->GetWidth(), true ),
                          MessageTextFromValue( userUnits(), refvia->GetDrillValue(), true ) );

            drcItem->SetErrorMessage( m_msg );
            drcItem->SetItems( refvia );

            MARKER_PCB* marker = new MARKER_PCB( drcItem, refvia->GetPosition() );
            addMarkerToPcb( aCommit, marker );
        }

        // test if the type of via is allowed due to design rules
        if( refvia->GetViaType() == VIATYPE::MICROVIA && !bds.m_MicroViasAllowed )
        {
            DRC_ITEM* drcItem = new DRC_ITEM( DRCE_MICROVIA_NOT_ALLOWED );

            m_msg.Printf( drcItem->GetErrorText() + _( " (board design rule constraints)" ) );
            drcItem->SetErrorMessage( m_msg );
            drcItem->SetItems( refvia );

            MARKER_PCB* marker = new MARKER_PCB( drcItem, refvia->GetPosition() );
            addMarkerToPcb( aCommit, marker );
        }

        // test if the type of via is allowed due to design rules
        if( refvia->GetViaType() == VIATYPE::BLIND_BURIED && !bds.m_BlindBuriedViaAllowed )
        {
            DRC_ITEM* drcItem = new DRC_ITEM( DRCE_BURIED_VIA_NOT_ALLOWED );

            m_msg.Printf( drcItem->GetErrorText() + _( " (board design rule constraints)" ) );
            drcItem->SetErrorMessage( m_msg );
            drcItem->SetItems( refvia );

            MARKER_PCB* marker = new MARKER_PCB( drcItem, refvia->GetPosition() );
            addMarkerToPcb( aCommit, marker );
        }

        // For microvias: test if they are blind vias and only between 2 layers
        // because they are used for very small drill size and are drill by laser
        // and **only one layer** can be drilled
        if( refvia->GetViaType() == VIATYPE::MICROVIA )
        {
            PCB_LAYER_ID layer1, layer2;
            bool         err = true;

            refvia->LayerPair( &layer1, &layer2 );

            if( layer1 > layer2 )
                std::swap( layer1, layer2 );

            if( layer2 == B_Cu && layer1 == bds.GetCopperLayerCount() - 2 )
                err = false;
            else if( layer1 == F_Cu  &&  layer2 == In1_Cu  )
                err = false;

            if( err )
            {
                DRC_ITEM* drcItem = new DRC_ITEM( DRCE_MICROVIA_TOO_MANY_LAYERS );

                m_msg.Printf( drcItem->GetErrorText() + _( " (%s and %s not adjacent)" ),
                              m_pcb->GetLayerName( layer1 ),
                              m_pcb->GetLayerName( layer2 ) );

                drcItem->SetErrorMessage( m_msg );
                drcItem->SetItems( refvia );

                MARKER_PCB* marker = new MARKER_PCB( drcItem, refvia->GetPosition() );
                addMarkerToPcb( aCommit, marker );
            }
        }

    }
    else    // This is a track segment
    {
        int minWidth, maxWidth;
        aRefSeg->GetWidthConstraints( &minWidth, &maxWidth, &m_clearanceSource );

        int errorCode = 0;
        int constraintWidth;

        if( refSegWidth < minWidth )
        {
            errorCode = DRCE_TOO_SMALL_TRACK_WIDTH;
            constraintWidth = minWidth;
        }
        else if( refSegWidth > maxWidth )
        {
            errorCode = DRCE_TOO_LARGE_TRACK_WIDTH;
            constraintWidth = maxWidth;
        }

        if( errorCode )
        {
            wxPoint refsegMiddle = ( aRefSeg->GetStart() + aRefSeg->GetEnd() ) / 2;

            DRC_ITEM* drcItem = new DRC_ITEM( errorCode );

            m_msg.Printf( drcItem->GetErrorText() + _( " (%s %s; actual %s)" ),
                          m_clearanceSource,
                          MessageTextFromValue( userUnits(), constraintWidth, true ),
                          MessageTextFromValue( userUnits(), refSegWidth, true ) );

            drcItem->SetErrorMessage( m_msg );
            drcItem->SetItems( aRefSeg );

            MARKER_PCB* marker = new MARKER_PCB( drcItem, refsegMiddle );
            addMarkerToPcb( aCommit, marker );
        }
    }


    /******************************************/
    /* Phase 1 : test DRC track to pads :     */
    /******************************************/

    // Compute the min distance to pads
    for( MODULE* mod : m_pcb->Modules() )
    {
        // Don't preflight at the module level.  Getting a module's bounding box goes
        // through all its pads anyway (so it's no faster), and also all its drawings
        // (so it's in fact slower).

        for( D_PAD* pad : mod->Pads() )
        {
            // Preflight based on bounding boxes.
            EDA_RECT inflatedBB = refSegBB;
            inflatedBB.Inflate( pad->GetBoundingRadius() + m_largestClearance );

            if( !inflatedBB.Contains( pad->GetPosition() ) )
                continue;

            if( !( pad->GetLayerSet() & refLayerSet ).any() )
                continue;

            // No need to check pads with the same net as the refSeg.
            if( pad->GetNetCode() && aRefSeg->GetNetCode() == pad->GetNetCode() )
                continue;

            if( pad->GetDrillSize().x > 0 )
            {
                // For hole testing we use a dummy pad which is a copy of the current pad
                // shrunk down to nothing but its hole.
                D_PAD dummypad( *pad );
                dummypad.SetSize( pad->GetDrillSize() );
                dummypad.SetShape( pad->GetDrillShape() == PAD_DRILL_SHAPE_OBLONG ?
                                                           PAD_SHAPE_OVAL : PAD_SHAPE_CIRCLE );
                // Ensure the hole is on all copper layers
                const static LSET all_cu = LSET::AllCuMask();
                dummypad.SetLayerSet( all_cu | dummypad.GetLayerSet() );

                int       minClearance;
                DRC_RULE* rule = GetRule( aRefSeg, &dummypad, CLEARANCE_CONSTRAINT );

                if( rule )
                {
                    m_clearanceSource = wxString::Format( _( "'%s' rule" ), rule->m_Name );
                    minClearance = rule->m_Clearance.Min;
                }
                else
                {
                    minClearance = aRefSeg->GetClearance( nullptr, &m_clearanceSource );
                }

                /* Treat an oval hole as a line segment along the hole's major axis,
                 * shortened by half its minor axis.
                 * A circular hole is just a degenerate case of an oval hole.
                 */
                wxPoint slotStart, slotEnd;
                int     slotWidth;

                pad->GetOblongGeometry( pad->GetDrillSize(), &slotStart, &slotEnd, &slotWidth );
                slotStart += pad->GetPosition();
                slotEnd += pad->GetPosition();

                SEG     slotSeg( slotStart, slotEnd );
                int     widths = ( slotWidth + refSegWidth ) / 2;
                int     center2centerAllowed = minClearance + widths + bds.GetDRCEpsilon();

                // Avoid square-roots if possible (for performance)
                SEG::ecoord center2center_squared = refSeg.SquaredDistance( slotSeg );

                if( center2center_squared < SEG::Square( center2centerAllowed ) )
                {
                    int       actual = std::max( 0.0, sqrt( center2center_squared ) - widths );
                    DRC_ITEM* drcItem = new DRC_ITEM( DRCE_TRACK_NEAR_HOLE );

                    m_msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                                  m_clearanceSource,
                                  MessageTextFromValue( userUnits(), minClearance, true ),
                                  MessageTextFromValue( userUnits(), actual, true ) );

                    drcItem->SetErrorMessage( m_msg );
                    drcItem->SetItems( aRefSeg, pad );

                    MARKER_PCB* marker = new MARKER_PCB( drcItem, GetLocation( aRefSeg, slotSeg ) );
                    addMarkerToPcb( aCommit, marker );

                    if( !m_reportAllTrackErrors )
                        return;
                }
            }

            int minClearance = aRefSeg->GetClearance( pad, &m_clearanceSource );
            int clearanceAllowed = minClearance - bds.GetDRCEpsilon();
            int actual;

            if( !checkClearanceSegmToPad( refSeg, refSegWidth, pad, clearanceAllowed, &actual ) )
            {
                actual = std::max( 0, actual );
                SEG       padSeg( pad->GetPosition(), pad->GetPosition() );
                DRC_ITEM* drcItem = new DRC_ITEM( DRCE_TRACK_NEAR_PAD );

                m_msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                              m_clearanceSource,
                              MessageTextFromValue( userUnits(), minClearance, true ),
                              MessageTextFromValue( userUnits(), actual, true ) );

                drcItem->SetErrorMessage( m_msg );
                drcItem->SetItems( aRefSeg, pad );

                MARKER_PCB* marker = new MARKER_PCB( drcItem, GetLocation( aRefSeg, padSeg ) );
                addMarkerToPcb( aCommit, marker );

                if( !m_reportAllTrackErrors )
                    return;
            }
        }
    }

    /***********************************************/
    /* Phase 2: test DRC with other track segments */
    /***********************************************/

    // Test the reference segment with other track segments
    for( auto it = aStartIt; it != aEndIt; it++ )
    {
        TRACK* track = *it;

        // No problem if segments have the same net code:
        if( aRefSeg->GetNetCode() == track->GetNetCode() )
            continue;

        // No problem if tracks are on different layers:
        // Note that while the general case of GetLayerSet intersection always works,
        // the others are much faster.
        bool sameLayers;

        if( aRefSeg->Type() == PCB_VIA_T )
        {
            if( track->Type() == PCB_VIA_T )
                sameLayers = ( refLayerSet & track->GetLayerSet() ).any();
            else
                sameLayers = refLayerSet.test( track->GetLayer() );
        }
        else
        {
            if( track->Type() == PCB_VIA_T )
                sameLayers = track->GetLayerSet().test( refLayer );
            else
                sameLayers = track->GetLayer() == refLayer;
        }

        if( !sameLayers )
            continue;

        // Preflight based on worst-case inflated bounding boxes:
        EDA_RECT trackBB = track->GetBoundingBox();
        trackBB.Inflate( m_largestClearance );

        if( !trackBB.Intersects( refSegBB ) )
            continue;

        int minClearance = aRefSeg->GetClearance( track, &m_clearanceSource );
        SEG trackSeg( track->GetStart(), track->GetEnd() );
        int widths = ( refSegWidth + track->GetWidth() ) / 2;
        int center2centerAllowed = minClearance + widths;

        // Avoid square-roots if possible (for performance)
        SEG::ecoord  center2center_squared = refSeg.SquaredDistance( trackSeg );
        OPT_VECTOR2I intersection = refSeg.Intersect( trackSeg );

        // Check two tracks crossing first as it reports a DRCE without distances
        if( intersection )
        {
            DRC_ITEM* drcItem = new DRC_ITEM( DRCE_TRACKS_CROSSING );
            drcItem->SetErrorMessage( m_msg );
            drcItem->SetItems( aRefSeg, track );

            MARKER_PCB* marker = new MARKER_PCB( drcItem, (wxPoint) intersection.get() );
            addMarkerToPcb( aCommit, marker );

            if( !m_reportAllTrackErrors )
                return;
        }
        else if( center2center_squared < SEG::Square( center2centerAllowed ) )
        {
            int errorCode = DRCE_TRACK_ENDS;

            if( aRefSeg->Type() == PCB_VIA_T && track->Type() == PCB_VIA_T )
                errorCode = DRCE_VIA_NEAR_VIA;
            else if( aRefSeg->Type() == PCB_VIA_T || track->Type() == PCB_VIA_T )
                errorCode = DRCE_VIA_NEAR_TRACK;
            else if( refSeg.ApproxParallel( trackSeg ) )
                errorCode = DRCE_TRACK_SEGMENTS_TOO_CLOSE;

            int       actual = std::max( 0.0, sqrt( center2center_squared ) - widths );
            DRC_ITEM* drcItem = new DRC_ITEM( errorCode );

            m_msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                          m_clearanceSource,
                          MessageTextFromValue( userUnits(), minClearance, true ),
                          MessageTextFromValue( userUnits(), actual, true ) );

            drcItem->SetErrorMessage( m_msg );
            drcItem->SetItems( aRefSeg, track );

            MARKER_PCB* marker = new MARKER_PCB( drcItem, GetLocation( aRefSeg, trackSeg ) );
            addMarkerToPcb( aCommit, marker );

            if( !m_reportAllTrackErrors )
                return;
        }
    }

    /***************************************/
    /* Phase 3: test DRC with copper zones */
    /***************************************/
    // Can be *very* time consumming.
    if( aTestZones )
    {
        SEG testSeg( aRefSeg->GetStart(), aRefSeg->GetEnd() );

        for( ZONE_CONTAINER* zone : m_pcb->Zones() )
        {
            if( zone->GetFilledPolysList().IsEmpty() || zone->GetIsKeepout() )
                continue;

            if( !( refLayerSet & zone->GetLayerSet() ).any() )
                continue;

            if( zone->GetNetCode() && zone->GetNetCode() == aRefSeg->GetNetCode() )
                continue;

            int             minClearance = aRefSeg->GetClearance( zone, &m_clearanceSource );
            int             widths = refSegWidth / 2;
            int             center2centerAllowed = minClearance + widths;
            SHAPE_POLY_SET* outline = const_cast<SHAPE_POLY_SET*>( &zone->GetFilledPolysList() );

            SEG::ecoord     center2center_squared = outline->SquaredDistance( testSeg );

            // to avoid false positive, due to rounding issues and approxiamtions
            // in distance and clearance calculations, use a small threshold for distance
            // (1 micron)
            #define THRESHOLD_DIST Millimeter2iu( 0.001 )

            if( center2center_squared + THRESHOLD_DIST < SEG::Square( center2centerAllowed ) )
            {
                int       actual = std::max( 0.0, sqrt( center2center_squared ) - widths );
                DRC_ITEM* drcItem = new DRC_ITEM( DRCE_TRACK_NEAR_ZONE );

                m_msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                              m_clearanceSource,
                              MessageTextFromValue( userUnits(), minClearance, true ),
                              MessageTextFromValue( userUnits(), actual, true ) );

                drcItem->SetErrorMessage( m_msg );
                drcItem->SetItems( aRefSeg, zone );

                MARKER_PCB* marker = new MARKER_PCB( drcItem, GetLocation( aRefSeg, zone ) );
                addMarkerToPcb( aCommit, marker );
            }
        }
    }

    /***********************************************/
    /* Phase 4: test DRC with to board edge        */
    /***********************************************/
    if( m_board_outline_valid )
    {
        int minClearance = bds.m_CopperEdgeClearance;
        m_clearanceSource = _( "board edge" );

        static DRAWSEGMENT dummyEdge;
        dummyEdge.SetLayer( Edge_Cuts );

        if( aRefSeg->GetRuleClearance( &dummyEdge, &minClearance, &m_clearanceSource ) )
            /* minClearance and m_clearanceSource set in GetRuleClearance() */;

        SEG testSeg( aRefSeg->GetStart(), aRefSeg->GetEnd() );
        int halfWidth = refSegWidth / 2;
        int center2centerAllowed = minClearance + halfWidth;

        for( auto it = m_board_outlines.IterateSegmentsWithHoles(); it; it++ )
        {
            SEG::ecoord center2center_squared = testSeg.SquaredDistance(  *it );

            if( center2center_squared < SEG::Square( center2centerAllowed ) )
            {
                VECTOR2I pt = testSeg.NearestPoint( *it );

                KICAD_T        types[] = { PCB_LINE_T, EOT };
                DRAWSEGMENT*   edge = nullptr;
                INSPECTOR_FUNC inspector =
                        [&] ( EDA_ITEM* item, void* testData )
                        {
                            DRAWSEGMENT* test_edge = dynamic_cast<DRAWSEGMENT*>( item );

                            if( !test_edge || test_edge->GetLayer() != Edge_Cuts )
                                return SEARCH_RESULT::CONTINUE;

                            if( test_edge->HitTest( (wxPoint) pt, minClearance + halfWidth ) )
                            {
                                edge = test_edge;
                                return SEARCH_RESULT::QUIT;
                            }

                            return SEARCH_RESULT::CONTINUE;
                        };

                // Best-efforts search for edge segment
                BOARD::IterateForward<BOARD_ITEM*>( m_pcb->Drawings(), inspector, nullptr, types );

                int       actual = std::max( 0.0, sqrt( center2center_squared ) - halfWidth );
                int       errorCode = ( aRefSeg->Type() == PCB_VIA_T ) ? DRCE_VIA_NEAR_EDGE
                                                                       : DRCE_TRACK_NEAR_EDGE;
                DRC_ITEM* drcItem = new DRC_ITEM( errorCode );

                m_msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                              m_clearanceSource,
                              MessageTextFromValue( userUnits(), minClearance, true ),
                              MessageTextFromValue( userUnits(), actual, true ) );

                drcItem->SetErrorMessage( m_msg );
                drcItem->SetItems( aRefSeg, edge );

                MARKER_PCB* marker = new MARKER_PCB( drcItem, (wxPoint) pt );
                addMarkerToPcb( aCommit, marker );
            }
        }
    }
}


bool DRC::checkClearancePadToPad( D_PAD* aRefPad, D_PAD* aPad, int aMinClearance, int* aActual )
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


/*
 * Test if distance between a segment and a pad is > minClearance.  Return the actual
 * distance if it is less.
 */
bool DRC::checkClearanceSegmToPad( const SEG& refSeg, int refSegWidth, const D_PAD* pad,
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

