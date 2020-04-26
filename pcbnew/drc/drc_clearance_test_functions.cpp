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


static SEG::ecoord square( int a )
{
    return SEG::ecoord( a ) * a;
}


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


void DRC::doTrackDrc( TRACK* aRefSeg, TRACKS::iterator aStartIt, TRACKS::iterator aEndIt,
                      bool aTestZones )
{
    BOARD_DESIGN_SETTINGS& dsnSettings = m_pcb->GetDesignSettings();
    wxString  msg;

    SEG      refSeg( aRefSeg->GetStart(), aRefSeg->GetEnd() );
    LSET     layerMask = aRefSeg->GetLayerSet();
    EDA_RECT refSegBB = aRefSeg->GetBoundingBox();
    int      refSegWidth = aRefSeg->GetWidth();


    /******************************************/
    /* Phase 0 : via DRC tests :              */
    /******************************************/

    if( aRefSeg->Type() == PCB_VIA_T )
    {
        VIA *refvia = static_cast<VIA*>( aRefSeg );

        // test if the via size is smaller than minimum
        if( refvia->GetViaType() == VIATYPE::MICROVIA )
        {
            if( refvia->GetWidth() < dsnSettings.m_MicroViasMinSize )
            {
                DRC_ITEM* drcItem = new DRC_ITEM( DRCE_TOO_SMALL_MICROVIA );

                msg.Printf( drcItem->GetErrorText() + _( " (board minimum %s; actual %s)" ),
                            MessageTextFromValue( userUnits(), dsnSettings.m_MicroViasMinSize, true ),
                            MessageTextFromValue( userUnits(), refvia->GetWidth(), true ) );

                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( refvia );

                MARKER_PCB* marker = new MARKER_PCB( drcItem, refvia->GetPosition() );
                addMarkerToPcb( marker );
            }

            if( refvia->GetDrillValue() < dsnSettings.m_MicroViasMinDrill )
            {
                DRC_ITEM* drcItem = new DRC_ITEM( DRCE_TOO_SMALL_MICROVIA_DRILL );

                msg.Printf( drcItem->GetErrorText() + _( " (board minimum %s; actual %s)" ),
                            MessageTextFromValue( userUnits(), dsnSettings.m_MicroViasMinDrill, true ),
                            MessageTextFromValue( userUnits(), refvia->GetDrillValue(), true ) );

                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( refvia );

                MARKER_PCB* marker = new MARKER_PCB( drcItem, refvia->GetPosition() );
                addMarkerToPcb( marker );
            }
        }
        else
        {
            if( refvia->GetWidth() < dsnSettings.m_ViasMinSize )
            {
                DRC_ITEM* drcItem = new DRC_ITEM( DRCE_TOO_SMALL_VIA );

                msg.Printf( drcItem->GetErrorText() + _( " (board minimum %s; actual %s)" ),
                            MessageTextFromValue( userUnits(), dsnSettings.m_ViasMinSize, true ),
                            MessageTextFromValue( userUnits(), refvia->GetWidth(), true ) );

                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( refvia );

                MARKER_PCB* marker = new MARKER_PCB( drcItem, refvia->GetPosition() );
                addMarkerToPcb( marker );
            }

            if( refvia->GetDrillValue() < dsnSettings.m_ViasMinDrill )
            {
                DRC_ITEM* drcItem = new DRC_ITEM( DRCE_TOO_SMALL_VIA_DRILL );

                msg.Printf( drcItem->GetErrorText() + _( " (board minimum %s; actual %s)" ),
                            MessageTextFromValue( userUnits(), dsnSettings.m_ViasMinDrill, true ),
                            MessageTextFromValue( userUnits(), refvia->GetDrillValue(), true ) );

                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( refvia );

                MARKER_PCB* marker = new MARKER_PCB( drcItem, refvia->GetPosition() );
                addMarkerToPcb( marker );
            }
        }

        // test if via's hole is bigger than its diameter
        // This test is necessary since the via hole size and width can be modified
        // and a default via hole can be bigger than some vias sizes
        if( refvia->GetDrillValue() > refvia->GetWidth() )
        {
            DRC_ITEM* drcItem = new DRC_ITEM( DRCE_VIA_HOLE_BIGGER );

            msg.Printf( drcItem->GetErrorText() + _( " (diameter %s; drill %s)" ),
                        MessageTextFromValue( userUnits(), refvia->GetWidth(), true ),
                        MessageTextFromValue( userUnits(), refvia->GetDrillValue(), true ) );

            drcItem->SetErrorMessage( msg );
            drcItem->SetItems( refvia );

            MARKER_PCB* marker = new MARKER_PCB( drcItem, refvia->GetPosition() );
            addMarkerToPcb( marker );
        }

        // test if the type of via is allowed due to design rules
        if( refvia->GetViaType() == VIATYPE::MICROVIA && !dsnSettings.m_MicroViasAllowed )
        {
            DRC_ITEM* drcItem = new DRC_ITEM( DRCE_MICRO_VIA_NOT_ALLOWED );
            drcItem->SetItems( refvia );

            MARKER_PCB* marker = new MARKER_PCB( drcItem, refvia->GetPosition() );
            addMarkerToPcb( marker );
        }

        // test if the type of via is allowed due to design rules
        if( refvia->GetViaType() == VIATYPE::BLIND_BURIED && !dsnSettings.m_BlindBuriedViaAllowed )
        {
            DRC_ITEM* drcItem = new DRC_ITEM( DRCE_BURIED_VIA_NOT_ALLOWED );
            drcItem->SetItems( refvia );

            MARKER_PCB* marker = new MARKER_PCB( drcItem, refvia->GetPosition() );
            addMarkerToPcb( marker );
        }

        // For microvias: test if they are blind vias and only between 2 layers
        // because they are used for very small drill size and are drill by laser
        // and **only one layer** can be drilled
        if( refvia->GetViaType() == VIATYPE::MICROVIA )
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

            if( err )
            {
                DRC_ITEM* drcItem = new DRC_ITEM( DRCE_MICRO_VIA_INCORRECT_LAYER_PAIR );
                drcItem->SetItems( refvia );

                MARKER_PCB* marker = new MARKER_PCB( drcItem, refvia->GetPosition() );
                addMarkerToPcb( marker );
            }
        }

    }
    else    // This is a track segment
    {
        if( refSegWidth < dsnSettings.m_TrackMinWidth )
        {
            wxPoint refsegMiddle = ( aRefSeg->GetStart() + aRefSeg->GetEnd() ) / 2;

            DRC_ITEM* drcItem = new DRC_ITEM( DRCE_TOO_SMALL_TRACK_WIDTH );

            msg.Printf( drcItem->GetErrorText() + _( " (board minimum %s; actual %s)" ),
                        MessageTextFromValue( userUnits(), dsnSettings.m_TrackMinWidth, true ),
                        MessageTextFromValue( userUnits(), refSegWidth, true ) );

            drcItem->SetErrorMessage( msg );
            drcItem->SetItems( aRefSeg );

            MARKER_PCB* marker = new MARKER_PCB( drcItem, refsegMiddle );
            addMarkerToPcb( marker );
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
            inflatedBB.Inflate( pad->GetBoundingRadius() + aRefSeg->GetClearance( pad, nullptr ) );

            if( !inflatedBB.Contains( pad->GetPosition() ) )
                continue;

            if( pad->GetDrillSize().x > 0 )
            {
                /* Treat an oval hole as a line segment along the hole's major axis,
                 * shortened by half its minor axis.
                 * A circular hole is just a degenerate case of an oval hole.
                 */
                wxPoint slotStart;
                wxPoint slotEnd;
                int     slotWidth;

                pad->GetOblongDrillGeometry( slotStart, slotEnd, slotWidth );

                wxString clearanceSource;
                int      minClearance = aRefSeg->GetClearance( nullptr, &clearanceSource );
                SEG      slotSeg( slotStart, slotEnd );
                int      widths = ( slotWidth + refSegWidth ) / 2;
                int      center2centerAllowed = minClearance + widths;

                // Avoid square-roots if possible (for performance)
                SEG::ecoord center2center_squared = refSeg.SquaredDistance( slotSeg );

                if( center2center_squared < square( center2centerAllowed ) )
                {
                    int       actual = std::max( 0.0, sqrt( center2center_squared ) - widths );
                    DRC_ITEM* drcItem = new DRC_ITEM( DRCE_TRACK_NEAR_THROUGH_HOLE );

                    msg.Printf( drcItem->GetErrorText() + _( " (%s %s; actual %s)" ),
                                clearanceSource,
                                MessageTextFromValue( userUnits(), minClearance, true ),
                                MessageTextFromValue( userUnits(), actual, true ) );

                    drcItem->SetErrorMessage( msg );
                    drcItem->SetItems( aRefSeg, pad );

                    MARKER_PCB* marker = new MARKER_PCB( drcItem, getLocation( aRefSeg, slotSeg ) );
                    addMarkerToPcb( marker );

                    if( !m_reportAllTrackErrors )
                        return;
                }
            }

            if( !( pad->GetLayerSet() & layerMask ).any() )
                continue;

            // No need to check pads with the same net as the refSeg.
            if( pad->GetNetCode() && aRefSeg->GetNetCode() == pad->GetNetCode() )
                continue;

            wxString clearanceSource;
            int      minClearance = aRefSeg->GetClearance( pad, &clearanceSource );
            SEG      padSeg( pad->GetPosition(), pad->GetPosition() );
            int      actual;

            if( !checkClearanceSegmToPad( refSeg, refSegWidth, pad, minClearance, &actual ) )
            {
                DRC_ITEM* drcItem = new DRC_ITEM( DRCE_TRACK_NEAR_PAD );

                msg.Printf( drcItem->GetErrorText() + _( " (%s %s; actual %s)" ),
                            clearanceSource,
                            MessageTextFromValue( userUnits(), minClearance, true ),
                            MessageTextFromValue( userUnits(), actual, true ) );

                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( aRefSeg, pad );

                MARKER_PCB* marker = new MARKER_PCB( drcItem, getLocation( aRefSeg, padSeg ) );
                addMarkerToPcb( marker );

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
        if( !( layerMask & track->GetLayerSet() ).any() )
            continue;

        // Preflight based on inflated bounding boxes:
        EDA_RECT trackBB = track->GetBoundingBox();
        trackBB.Inflate( aRefSeg->GetClearance( track, nullptr ) );

        if( !trackBB.Intersects( refSegBB ) )
            continue;

        wxString clearanceSource;
        int      minClearance = aRefSeg->GetClearance( track, &clearanceSource );
        SEG      trackSeg( track->GetStart(), track->GetEnd() );
        int      widths = ( refSegWidth + track->GetWidth() ) / 2;
        int      center2centerAllowed = minClearance + widths;

        // Avoid square-roots if possible (for performance)
        SEG::ecoord  center2center_squared = refSeg.SquaredDistance( trackSeg );
        OPT_VECTOR2I intersection = refSeg.Intersect( trackSeg );

        // Check two tracks crossing first as it reports a DRCE without distances
        if( intersection )
        {
            DRC_ITEM* drcItem = new DRC_ITEM( DRCE_TRACKS_CROSSING );
            drcItem->SetErrorMessage( msg );
            drcItem->SetItems( aRefSeg, track );

            MARKER_PCB* marker = new MARKER_PCB( drcItem, (wxPoint) intersection.get() );
            addMarkerToPcb( marker );

            if( !m_reportAllTrackErrors )
                return;
        }
        else if( center2center_squared < square( center2centerAllowed ) )
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

            msg.Printf( drcItem->GetErrorText() + _( " (%s %s; actual %s)" ),
                        clearanceSource,
                        MessageTextFromValue( userUnits(), minClearance, true ),
                        MessageTextFromValue( userUnits(), actual, true ) );

            drcItem->SetErrorMessage( msg );
            drcItem->SetItems( aRefSeg, track );

            MARKER_PCB* marker = new MARKER_PCB( drcItem, getLocation( aRefSeg, trackSeg ) );
            addMarkerToPcb( marker );

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

            if( !( layerMask & zone->GetLayerSet() ).any() )
                continue;

            if( zone->GetNetCode() && zone->GetNetCode() == aRefSeg->GetNetCode() )
                continue;

            wxString        clearanceSource;
            int             minClearance = aRefSeg->GetClearance( zone, &clearanceSource );
            SHAPE_POLY_SET* outline = const_cast<SHAPE_POLY_SET*>( &zone->GetFilledPolysList() );

            int error = minClearance - outline->Distance( testSeg, refSegWidth );

            // to avoid false positive, due to rounding issues and approxiamtions
            // in distance and clearance calculations, use a small threshold for distance
            // (1 micron)
            #define THRESHOLD_DIST Millimeter2iu( 0.001 )

            if( error > THRESHOLD_DIST )
            {
                DRC_ITEM* drcItem = new DRC_ITEM( DRCE_TRACK_NEAR_ZONE );

                msg.Printf( drcItem->GetErrorText() + _( " (%s %s; actual %s)" ),
                            clearanceSource,
                            MessageTextFromValue( userUnits(), minClearance, true ),
                            MessageTextFromValue( userUnits(), minClearance - error, true ) );

                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( aRefSeg, zone );

                MARKER_PCB* marker = new MARKER_PCB( drcItem, getLocation( aRefSeg, zone ) );
                addMarkerToPcb( marker );
            }
        }
    }

    /***********************************************/
    /* Phase 4: test DRC with to board edge        */
    /***********************************************/
    {
        SEG testSeg( aRefSeg->GetStart(), aRefSeg->GetEnd() );

        wxString clearanceSource;
        int      minClearance = aRefSeg->GetClearance( nullptr, &clearanceSource );

        if( dsnSettings.m_CopperEdgeClearance > minClearance )
        {
            minClearance = dsnSettings.m_CopperEdgeClearance;
            clearanceSource = _( "board edge clearance" );
        }

        int halfWidth = refSegWidth / 2;
        int center2centerAllowed = minClearance + halfWidth;

        for( auto it = m_board_outlines.IterateSegmentsWithHoles(); it; it++ )
        {
            SEG::ecoord center2center_squared = testSeg.SquaredDistance(  *it );

            if( center2center_squared < square( center2centerAllowed ) )
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
                DRC_ITEM* drcItem = new DRC_ITEM( DRCE_TRACK_NEAR_EDGE );

                msg.Printf( drcItem->GetErrorText() + _( " (%s %s; actual %s)" ),
                            clearanceSource,
                            MessageTextFromValue( userUnits(), minClearance, true ),
                            MessageTextFromValue( userUnits(), actual, true ) );

                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( aRefSeg, edge );

                MARKER_PCB* marker = new MARKER_PCB( drcItem, (wxPoint) pt );
                addMarkerToPcb( marker );
            }
        }
    }
}


bool DRC::checkClearancePadToPad( D_PAD* aRefPad, D_PAD* aPad, int aMinClearance, int* aActual )
{
    // relativePadPos is the aPad shape position relative to the aRefPad shape position
    wxPoint relativePadPos = aPad->ShapePos() - aRefPad->ShapePos();

    int center2center = KiROUND( EuclideanNorm( relativePadPos ) );

    // Quick test: Clearance is OK if the bounding circles are further away than aMinClearance
    if( center2center - aRefPad->GetBoundingRadius() - aPad->GetBoundingRadius() >= aMinClearance )
        return true;

    /* Here, pads are near and DRC depends on the pad shapes.  We must compare distance using
     * a fine shape analysis.
     * Because a circle or oval shape is the easier shape to test, swap pads to have aRefPad be
     * a PAD_SHAPE_CIRCLE or PAD_SHAPE_OVAL.  If aRefPad = TRAPEZOID and aPad = RECT, also swap.
     */
    bool swap_pads;
    swap_pads = false;

    // swap pads to make comparisons easier
    // Note also a ROUNDRECT pad with a corner radius = r can be considered as
    // a smaller RECT (size - 2*r) with a clearance increased by r
    // priority is aRefPad = ROUND then OVAL then RECT/ROUNDRECT then other
    if( aRefPad->GetShape() != aPad->GetShape() && aRefPad->GetShape() != PAD_SHAPE_CIRCLE )
    {
        // pad ref shape is here oval, rect, roundrect, chamfered rect, trapezoid or custom
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
            case PAD_SHAPE_CHAMFERED_RECT:
            case PAD_SHAPE_CUSTOM:
                break;
        }
    }

    if( swap_pads )
    {
        std::swap( aRefPad, aPad );
        relativePadPos = -relativePadPos;
    }

    bool diag = true;

    if( ( aRefPad->GetShape() == PAD_SHAPE_CIRCLE || aRefPad->GetShape() == PAD_SHAPE_OVAL ) )
    {
        /* Treat an oval pad as a line segment along the hole's major axis,
         * shortened by half its minor axis.
         * A circular pad is just a degenerate case of an oval hole.
         */
        wxPoint refPadStart = aRefPad->GetPosition() + aRefPad->GetOffset();
        wxPoint refPadEnd = aRefPad->GetPosition() + aRefPad->GetOffset();
        int     refPadWidth;

        if( aRefPad->GetSize().x > aRefPad->GetSize().y )
        {
            refPadWidth = aRefPad->GetSize().y;
            refPadStart.x -= ( aRefPad->GetSize().x - refPadWidth ) / 2;
            refPadEnd.x += ( aRefPad->GetSize().x - refPadWidth ) / 2;
        }
        else
        {
            refPadWidth = aRefPad->GetSize().x;
            refPadStart.y -= ( aRefPad->GetSize().y - refPadWidth ) / 2;
            refPadEnd.y += ( aRefPad->GetSize().y - refPadWidth ) / 2;
        }

        SEG refPadSeg( refPadStart, refPadEnd );
        diag = checkClearanceSegmToPad( refPadSeg, refPadWidth, aPad, aMinClearance, aActual );
    }
    else
    {
        int dist_extra = 0;

        // corners of aRefPad (used only for rect/roundrect/trap pad)
        wxPoint polyref[4];
        // corners of aRefPad (used only for custom pad)
        SHAPE_POLY_SET polysetref;
        // corners of aPad (used only for rect/roundrect/trap pad)
        wxPoint polycompare[4];
        // corners of aPad (used only custom pad)
        SHAPE_POLY_SET polysetcompare;

        // pad_angle = pad orient relative to the aRefPad orient
        double pad_angle = aRefPad->GetOrientation() + aPad->GetOrientation();
        NORMALIZE_ANGLE_POS( pad_angle );

        if( aRefPad->GetShape() == PAD_SHAPE_ROUNDRECT )
        {
            int padRadius = aRefPad->GetRoundRectCornerRadius();
            dist_extra = padRadius;
            GetRoundRectCornerCenters( polyref, padRadius, wxPoint( 0, 0 ), aRefPad->GetSize(),
                                       aRefPad->GetOrientation() );
        }
        else if( aRefPad->GetShape() == PAD_SHAPE_CHAMFERED_RECT )
        {
            BOARD* board = aRefPad->GetBoard();
            int maxError = board ? board->GetDesignSettings().m_MaxError : ARC_HIGH_DEF;

            // The reference pad can be rotated.  Calculate the rotated coordinates.
            // (note, the ref pad position is the origin of coordinates for this drc test)
            int padRadius = aRefPad->GetRoundRectCornerRadius();

            TransformRoundChamferedRectToPolygon( polysetref, wxPoint( 0, 0 ), aRefPad->GetSize(),
                                                  aRefPad->GetOrientation(),
                                                  padRadius, aRefPad->GetChamferRectRatio(),
                                                  aRefPad->GetChamferPositions(), maxError );
        }
        else if( aRefPad->GetShape() == PAD_SHAPE_CUSTOM )
        {
            polysetref.Append( aRefPad->GetCustomShapeAsPolygon() );

            // The reference pad can be rotated.  Calculate the rotated coordinates.
            // (note, the ref pad position is the origin of coordinates for this drc test)
            aRefPad->CustomShapeAsPolygonToBoardPosition( &polysetref, wxPoint( 0, 0 ),
                                                          aRefPad->GetOrientation() );
        }
        else
        {
            // BuildPadPolygon has meaning for rect a trapeziod shapes and returns the 4 corners.
            aRefPad->BuildPadPolygon( polyref, wxSize( 0, 0 ), aRefPad->GetOrientation() );
        }

        switch( aPad->GetShape() )
        {
        case PAD_SHAPE_ROUNDRECT:
        case PAD_SHAPE_RECT:
        case PAD_SHAPE_CHAMFERED_RECT:
        case PAD_SHAPE_TRAPEZOID:
        case PAD_SHAPE_CUSTOM:
            if( aPad->GetShape() == PAD_SHAPE_ROUNDRECT )
            {
                int padRadius = aPad->GetRoundRectCornerRadius();
                dist_extra = padRadius;
                GetRoundRectCornerCenters( polycompare, padRadius, relativePadPos, aPad->GetSize(),
                                           aPad->GetOrientation() );
            }
            else if( aPad->GetShape() == PAD_SHAPE_CHAMFERED_RECT )
            {
                BOARD* board = aRefPad->GetBoard();
                int maxError = board ? board->GetDesignSettings().m_MaxError : ARC_HIGH_DEF;

                // The pad to compare can be rotated. Calculate the rotated coordinates.
                // ( note, the pad to compare position is the relativePadPos for this drc test)
                int padRadius = aPad->GetRoundRectCornerRadius();

                TransformRoundChamferedRectToPolygon( polysetcompare, relativePadPos,
                                                      aPad->GetSize(), aPad->GetOrientation(),
                                                      padRadius, aPad->GetChamferRectRatio(),
                                                      aPad->GetChamferPositions(), maxError );
            }
            else if( aPad->GetShape() == PAD_SHAPE_CUSTOM )
            {
                polysetcompare.Append( aPad->GetCustomShapeAsPolygon() );

                // The pad to compare can be rotated. Calculate the rotated coordinates.
                // ( note, the pad to compare position is the relativePadPos for this drc test)
                aPad->CustomShapeAsPolygonToBoardPosition( &polysetcompare, relativePadPos,
                                                           aPad->GetOrientation() );
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
                                   polycompare, 4, aMinClearance + dist_extra, aActual ) )
                {
                    *aActual = std::max( 0, *aActual - dist_extra );
                    diag = false;
                }
            }
            else if( polysetref.OutlineCount() == 0 && polysetcompare.OutlineCount())
            {
                const SHAPE_LINE_CHAIN& cmppoly = polysetcompare.COutline( 0 );
                // And now test polygons:
                if( !poly2polyDRC((wxPoint*) &cmppoly.CPoint( 0 ), cmppoly.PointCount(),
                                  polyref, 4, aMinClearance + dist_extra, aActual ) )
                {
                    *aActual = std::max( 0, *aActual - dist_extra );
                    diag = false;
                }
            }
            else if( polysetref.OutlineCount() && polysetcompare.OutlineCount() )
            {
                const SHAPE_LINE_CHAIN& refpoly = polysetref.COutline( 0 );
                const SHAPE_LINE_CHAIN& cmppoly = polysetcompare.COutline( 0 );

                // And now test polygons:
                if( !poly2polyDRC((wxPoint*) &refpoly.CPoint( 0 ), refpoly.PointCount(),
                                  (wxPoint*) &cmppoly.CPoint( 0 ), cmppoly.PointCount(),
                                  aMinClearance + dist_extra, aActual ) )
                {
                    *aActual = std::max( 0, *aActual - dist_extra );
                    diag = false;
                }
            }
            else
            {
                if( !poly2polyDRC( polyref, 4, polycompare, 4, aMinClearance + dist_extra, aActual ) )
                {
                    *aActual = std::max( 0, *aActual - dist_extra );
                    diag = false;
                }
            }
            break;

        default:
            wxLogDebug( wxT( "DRC::checkClearancePadToPad: unexpected pad shape %d" ), aPad->GetShape() );
            break;
        }
    }

    return diag;
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
        wxPoint padStart = pad->GetPosition() + pad->GetOffset();   // JEY TODO: needs to handle rotation....
        wxPoint padEnd = pad->GetPosition() + pad->GetOffset();
        int     padHalfWidth;

        if( pad->GetSize().x > pad->GetSize().y )
        {
            padHalfWidth = pad->GetSize().y / 2;
            padStart.x -= ( pad->GetSize().x / 2 ) - padHalfWidth;
            padEnd.x += ( pad->GetSize().x / 2 ) - padHalfWidth;
        }
        else
        {
            padHalfWidth = pad->GetSize().x / 2;
            padStart.y -= ( pad->GetSize().y / 2 ) - padHalfWidth;
            padEnd.y += ( pad->GetSize().y / 2 ) - padHalfWidth;
        }

        SEG padSeg( padStart, padEnd );
        int widths = padHalfWidth + ( refSegWidth / 2 );
        int center2centerAllowed = minClearance + widths;

        // Avoid square-roots if possible (for performance)
        SEG::ecoord center2center_squared = refSeg.SquaredDistance( padSeg );

        if( center2center_squared < square( center2centerAllowed ) )
        {
            *aActualDist = std::max( 0.0, sqrt( center2center_squared ) - widths );
            return false;
        }
    }
    else if( pad->GetShape() == PAD_SHAPE_RECT || pad->GetShape() == PAD_SHAPE_ROUNDRECT )
    {
        EDA_RECT padBBox = pad->GetBoundingBox();
        int     widths = refSegWidth / 2;

        // Note a ROUNDRECT pad with a corner radius = r can be treated as a smaller
        // RECT (size - 2*r) with a clearance increased by r
        if( pad->GetShape() == PAD_SHAPE_ROUNDRECT )
        {
            padBBox.Inflate( -2 * pad->GetRoundRectCornerRadius() );
            widths += pad->GetRoundRectCornerRadius();
        }

        SHAPE_RECT padShape( padBBox.GetPosition(), padBBox.GetWidth(), padBBox.GetHeight() );
        int        actual;

        if( padShape.DoCollide( refSeg, minClearance + widths, &actual ) )
        {
            *aActualDist = actual - widths;
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
            *aActualDist = actual - widths;
            return false;
        }
    }

    return true;
}

