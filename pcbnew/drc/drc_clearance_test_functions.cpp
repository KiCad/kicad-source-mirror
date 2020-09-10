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
#include <trigo.h>
#include <drc/drc.h>
#include <class_board.h>
#include <class_track.h>
#include <class_drawsegment.h>
#include <class_marker_pcb.h>
#include <geometry/polygon_test_point_inside.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>

void DRC::doSingleViaDRC( BOARD_COMMIT& aCommit, VIA* aRefVia )
{
    BOARD_DESIGN_SETTINGS&  bds = m_pcb->GetDesignSettings();

    // test if the via size is smaller than minimum
    if( aRefVia->GetViaType() == VIATYPE::MICROVIA )
    {
        if( aRefVia->GetWidth() < bds.m_MicroViasMinSize )
        {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_VIA_ANNULUS );

            m_msg.Printf( drcItem->GetErrorText() + _( " (board minimum %s; actual %s)" ),
                          MessageTextFromValue( userUnits(), bds.m_MicroViasMinSize, true ),
                          MessageTextFromValue( userUnits(), aRefVia->GetWidth(), true ) );

            drcItem->SetErrorMessage( m_msg );
            drcItem->SetItems( aRefVia );

            MARKER_PCB* marker = new MARKER_PCB( drcItem, aRefVia->GetPosition() );
            addMarkerToPcb( aCommit, marker );
        }
    }
    else
    {
        if( aRefVia->GetWidth() < bds.m_ViasMinSize )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_VIA_ANNULUS );

            m_msg.Printf( drcItem->GetErrorText() + _( " (board minimum %s; actual %s)" ),
                          MessageTextFromValue( userUnits(), bds.m_ViasMinSize, true ),
                          MessageTextFromValue( userUnits(), aRefVia->GetWidth(), true ) );

            drcItem->SetErrorMessage( m_msg );
            drcItem->SetItems( aRefVia );

            MARKER_PCB* marker = new MARKER_PCB( drcItem, aRefVia->GetPosition() );
            addMarkerToPcb( aCommit, marker );
        }
    }

    // test if via's hole is bigger than its diameter
    // This test is necessary since the via hole size and width can be modified
    // and a default via hole can be bigger than some vias sizes
    if( aRefVia->GetDrillValue() > aRefVia->GetWidth() )
    {
        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_VIA_HOLE_BIGGER );

        m_msg.Printf( drcItem->GetErrorText() + _( " (diameter %s; drill %s)" ),
                      MessageTextFromValue( userUnits(), aRefVia->GetWidth(), true ),
                      MessageTextFromValue( userUnits(), aRefVia->GetDrillValue(), true ) );

        drcItem->SetErrorMessage( m_msg );
        drcItem->SetItems( aRefVia );

        MARKER_PCB* marker = new MARKER_PCB( drcItem, aRefVia->GetPosition() );
        addMarkerToPcb( aCommit, marker );
    }

    // test if the type of via is allowed due to design rules
    if( aRefVia->GetViaType() == VIATYPE::MICROVIA && !bds.m_MicroViasAllowed )
    {
        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_ALLOWED_ITEMS );

        m_msg.Printf( _( "Microvia not allowed (board design rule constraints)" ) );
        drcItem->SetErrorMessage( m_msg );
        drcItem->SetItems( aRefVia );

        MARKER_PCB* marker = new MARKER_PCB( drcItem, aRefVia->GetPosition() );
        addMarkerToPcb( aCommit, marker );
    }

    // test if the type of via is allowed due to design rules
    if( aRefVia->GetViaType() == VIATYPE::BLIND_BURIED && !bds.m_BlindBuriedViaAllowed )
    {
        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_ALLOWED_ITEMS );

        m_msg.Printf( _( "Blind/buried via not allowed (board design rule constraints)" ) );
        drcItem->SetErrorMessage( m_msg );
        drcItem->SetItems( aRefVia );

        MARKER_PCB* marker = new MARKER_PCB( drcItem, aRefVia->GetPosition() );
        addMarkerToPcb( aCommit, marker );
    }

    // For microvias: test if they are blind vias and only between 2 layers
    // because they are used for very small drill size and are drill by laser
    // and **only one layer** can be drilled
    if( aRefVia->GetViaType() == VIATYPE::MICROVIA )
    {
        PCB_LAYER_ID layer1, layer2;
        bool         err = true;

        aRefVia->LayerPair( &layer1, &layer2 );

        if( layer1 > layer2 )
            std::swap( layer1, layer2 );

        if( layer2 == B_Cu && layer1 == bds.GetCopperLayerCount() - 2 )
            err = false;
        else if( layer1 == F_Cu  &&  layer2 == In1_Cu  )
            err = false;
        else if ( layer1 == layer2 - 1 )
            err = false;

        if( err )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_PADSTACK );

            m_msg.Printf( _( "Microvia through too many layers (%s and %s not adjacent)" ),
                          m_pcb->GetLayerName( layer1 ),
                          m_pcb->GetLayerName( layer2 ) );

            drcItem->SetErrorMessage( m_msg );
            drcItem->SetItems( aRefVia );

            MARKER_PCB* marker = new MARKER_PCB( drcItem, aRefVia->GetPosition() );
            addMarkerToPcb( aCommit, marker );
        }
    }
}


void DRC::doSingleTrackDRC( BOARD_COMMIT& aCommit, TRACK* aRefSeg )
{
    SHAPE_SEGMENT refSeg( aRefSeg->GetStart(), aRefSeg->GetEnd(), aRefSeg->GetWidth() );
    EDA_RECT      refSegBB = aRefSeg->GetBoundingBox();
    int           refSegWidth = aRefSeg->GetWidth();

    int minWidth, maxWidth;
    aRefSeg->GetWidthConstraints( &minWidth, &maxWidth, &m_clearanceSource );

    int errorCode = 0;
    int constraintWidth;

    if( refSegWidth < minWidth )
    {
        errorCode = DRCE_TRACK_WIDTH;
        constraintWidth = minWidth;
    }
    else if( refSegWidth > maxWidth )
    {
        errorCode = DRCE_TRACK_WIDTH;
        constraintWidth = maxWidth;
    }

    if( errorCode )
    {
        wxPoint refsegMiddle = ( aRefSeg->GetStart() + aRefSeg->GetEnd() ) / 2;

        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( errorCode );

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


void DRC::doTrackDrc( BOARD_COMMIT& aCommit, TRACK* aRefSeg, TRACKS::iterator aStartIt,
                      TRACKS::iterator aEndIt, bool aTestZones, PCB_LAYER_ID aLayer )
{
    BOARD_DESIGN_SETTINGS&  bds = m_pcb->GetDesignSettings();
    SHAPE_SEGMENT           refSeg( aRefSeg->GetStart(), aRefSeg->GetEnd(), aRefSeg->GetWidth() );

    EDA_RECT    refSegBB = aRefSeg->GetBoundingBox();
    SEG         testSeg( aRefSeg->GetStart(), aRefSeg->GetEnd() );
    int         halfWidth = ( aRefSeg->GetWidth() + 1 ) / 2;

    /******************************************/
    /* Phase 0 : via DRC tests :              */
    /******************************************/

    if( aRefSeg->Type() == PCB_VIA_T )
    {
        VIA* refvia = static_cast<VIA*>( aRefSeg );
        int viaAnnulus = ( refvia->GetWidth() - refvia->GetDrill() ) / 2;
        int minAnnulus = refvia->GetMinAnnulus( aLayer, &m_clearanceSource );

        if( !refvia->IsPadOnLayer( aLayer ) )
        {
            halfWidth = ( refvia->GetDrillValue() + 1 ) / 2;
            refSegBB = EDA_RECT( refvia->GetStart(),
                    wxSize( refvia->GetDrillValue(), refvia->GetDrillValue() ) );
        }

        // test if the via size is smaller than minimum
        if( refvia->GetViaType() == VIATYPE::MICROVIA )
        {
            if( viaAnnulus < minAnnulus )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_VIA_ANNULUS );

                m_msg.Printf( _( "Via annulus too small (%s %s; actual %s)" ),
                              m_clearanceSource,
                              MessageTextFromValue( userUnits(), minAnnulus, true ),
                              MessageTextFromValue( userUnits(), viaAnnulus, true ) );

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
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_VIA_ANNULUS );

                m_msg.Printf( _( "Via annulus too small (%s %s; actual %s)" ),
                              m_clearanceSource,
                              MessageTextFromValue( userUnits(), minAnnulus, true ),
                              MessageTextFromValue( userUnits(), viaAnnulus, true ) );

                drcItem->SetErrorMessage( m_msg );
                drcItem->SetItems( refvia );

                MARKER_PCB* marker = new MARKER_PCB( drcItem, refvia->GetPosition() );
                addMarkerToPcb( aCommit, marker );
            }
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

            if( !( pad->IsOnLayer( aLayer ) ) )
                continue;

            // No need to check pads with the same net as the refSeg.
            if( pad->GetNetCode() && aRefSeg->GetNetCode() == pad->GetNetCode() )
                continue;

            if( pad->GetDrillSize().x > 0 )
            {
                const SHAPE_SEGMENT*  slot = pad->GetEffectiveHoleShape();
                const DRC_CONSTRAINT* constraint = GetConstraint( aRefSeg, pad,
                                                                  DRC_RULE_ID_CLEARANCE, aLayer,
                                                                  &m_clearanceSource );
                int minClearance;
                int actual;

                if( constraint )
                {
                    m_clearanceSource = wxString::Format( _( "'%s' rule" ), m_clearanceSource );
                    minClearance = constraint->m_Value.Min();
                }
                else
                {
                    minClearance = aRefSeg->GetClearance( aLayer, nullptr, &m_clearanceSource );
                }

                if( pad->GetAttribute() == PAD_ATTRIB_STANDARD )
                    minClearance += bds.GetHolePlatingThickness();

                if( slot->Collide( &refSeg, minClearance + bds.GetDRCEpsilon(), &actual ) )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );

                    m_msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                                  m_clearanceSource,
                                  MessageTextFromValue( userUnits(), minClearance, true ),
                                  MessageTextFromValue( userUnits(), actual, true ) );

                    drcItem->SetErrorMessage( m_msg );
                    drcItem->SetItems( aRefSeg, pad );

                    MARKER_PCB* marker = new MARKER_PCB( drcItem, pad->GetPosition() );
                    addMarkerToPcb( aCommit, marker );

                    if( !m_reportAllTrackErrors )
                        return;
                }
            }

            /// Skip checking pad copper when it has been removed
            if( !pad->IsPadOnLayer( aLayer ) )
                continue;

            int minClearance = aRefSeg->GetClearance( aLayer, pad, &m_clearanceSource );
            int actual;

            if( pad->Collide( &refSeg, minClearance - bds.GetDRCEpsilon(), &actual ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );

                m_msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                              m_clearanceSource,
                              MessageTextFromValue( userUnits(), minClearance, true ),
                              MessageTextFromValue( userUnits(), actual, true ) );

                drcItem->SetErrorMessage( m_msg );
                drcItem->SetItems( aRefSeg, pad );

                MARKER_PCB* marker = new MARKER_PCB( drcItem, pad->GetPosition() );
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

        if( !track->GetLayerSet().test( aLayer ) )
            continue;

        // Preflight based on worst-case inflated bounding boxes:
        EDA_RECT trackBB = track->GetBoundingBox();
        trackBB.Inflate( m_largestClearance );

        if( !trackBB.Intersects( refSegBB ) )
            continue;

        int           minClearance = aRefSeg->GetClearance( aLayer, track, &m_clearanceSource );
        int           actual;
        SHAPE_SEGMENT trackSeg( track->GetStart(), track->GetEnd(), track->GetWidth() );

        /// Check to see if the via has a pad on this layer
        if( track->Type() == PCB_VIA_T )
        {
            VIA* via = static_cast<VIA*>( track );

            if( !via->IsPadOnLayer( aLayer ) )
                trackSeg.SetWidth( via->GetDrillValue() );
        }

        // Check two tracks crossing first as it reports a DRCE without distances
        if( OPT_VECTOR2I intersection = refSeg.GetSeg().Intersect( trackSeg.GetSeg() ) )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TRACKS_CROSSING );
            drcItem->SetErrorMessage( m_msg );
            drcItem->SetItems( aRefSeg, track );

            MARKER_PCB* marker = new MARKER_PCB( drcItem, (wxPoint) intersection.get() );
            addMarkerToPcb( aCommit, marker );

            if( !m_reportAllTrackErrors )
                return;
        }
        else if( refSeg.Collide( &trackSeg, minClearance - bds.GetDRCEpsilon(), &actual ) )
        {
            wxPoint   pos = GetLocation( aRefSeg, trackSeg.GetSeg() );
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );

            m_msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                          m_clearanceSource,
                          MessageTextFromValue( userUnits(), minClearance, true ),
                          MessageTextFromValue( userUnits(), actual, true ) );

            drcItem->SetErrorMessage( m_msg );
            drcItem->SetItems( aRefSeg, track );

            MARKER_PCB* marker = new MARKER_PCB( drcItem, pos );
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
        for( ZONE_CONTAINER* zone : m_pcb->Zones() )
        {
            if( !zone->GetLayerSet().test( aLayer ) || zone->GetIsKeepout() )
                continue;

            if( zone->GetNetCode() && zone->GetNetCode() == aRefSeg->GetNetCode() )
                continue;

            if( zone->GetFilledPolysList( aLayer ).IsEmpty() )
                continue;

            int minClearance = aRefSeg->GetClearance( aLayer, zone, &m_clearanceSource );
            int allowedDist  = minClearance + halfWidth - bds.GetDRCEpsilon();
            int actual;

            if( zone->GetFilledPolysList( aLayer ).Collide( testSeg, allowedDist, &actual ) )
            {
                actual = std::max( 0, actual - halfWidth );
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );

                m_msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                              m_clearanceSource,
                              MessageTextFromValue( userUnits(), minClearance, true ),
                              MessageTextFromValue( userUnits(), actual, true ) );

                drcItem->SetErrorMessage( m_msg );
                drcItem->SetItems( aRefSeg, zone );

                MARKER_PCB* marker = new MARKER_PCB( drcItem, GetLocation( aLayer, aRefSeg, zone ) );
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
        dummyEdge.SetParent( m_pcb );
        dummyEdge.SetLayer( Edge_Cuts );

        aRefSeg->GetRuleClearance( &dummyEdge, aLayer, &minClearance, &m_clearanceSource );

        int center2centerAllowed = minClearance + halfWidth - bds.GetDRCEpsilon();

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

                int actual  = std::max( 0.0, sqrt( center2center_squared ) - halfWidth );
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_COPPER_EDGE_CLEARANCE );

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

