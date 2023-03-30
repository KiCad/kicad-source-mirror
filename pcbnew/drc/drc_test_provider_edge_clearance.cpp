/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2022 KiCad Developers.
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

#include <common.h>
#include <pcb_shape.h>
#include <footprint.h>
#include <geometry/seg.h>
#include <geometry/shape_segment.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider_clearance_base.h>
#include "drc_rtree.h"

/*
    Board edge clearance test. Checks all items for their mechanical clearances against the board
    edge.
    Errors generated:
    - DRCE_EDGE_CLEARANCE
    - DRCE_SILK_EDGE_CLEARANCE

    TODO:
    - separate holes to edge check
    - tester only looks for edge crossings. it doesn't check if items are inside/outside the board
      area.
    - pad test missing!
*/

class DRC_TEST_PROVIDER_EDGE_CLEARANCE : public DRC_TEST_PROVIDER_CLEARANCE_BASE
{
public:
    DRC_TEST_PROVIDER_EDGE_CLEARANCE () :
            DRC_TEST_PROVIDER_CLEARANCE_BASE(),
            m_largestEdgeClearance( 0 )
    {
    }

    virtual ~DRC_TEST_PROVIDER_EDGE_CLEARANCE()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return wxT( "edge_clearance" );
    }

    virtual const wxString GetDescription() const override
    {
        return wxT( "Tests items vs board edge clearance" );
    }

private:
    bool testAgainstEdge( BOARD_ITEM* item, SHAPE* itemShape, BOARD_ITEM* other,
                          DRC_CONSTRAINT_T aConstraintType, PCB_DRC_CODE aErrorCode );

private:
    std::vector<PAD*> m_castellatedPads;
    int               m_largestEdgeClearance;
};


bool DRC_TEST_PROVIDER_EDGE_CLEARANCE::testAgainstEdge( BOARD_ITEM* item, SHAPE* itemShape,
                                                        BOARD_ITEM* edge,
                                                        DRC_CONSTRAINT_T aConstraintType,
                                                        PCB_DRC_CODE aErrorCode )
{
    std::shared_ptr<SHAPE> shape;

    if( edge->Type() == PCB_PAD_T )
        shape = edge->GetEffectiveHoleShape();
    else
        shape = edge->GetEffectiveShape( Edge_Cuts );

    auto     constraint = m_drcEngine->EvalRules( aConstraintType, edge, item, UNDEFINED_LAYER );
    int      minClearance = constraint.GetValue().Min();
    int      actual;
    VECTOR2I pos;

    if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && minClearance >= 0 )
    {
        if( itemShape->Collide( shape.get(), minClearance, &actual, &pos ) )
        {
            // Exact clearance is allowed
            if( minClearance > 0 && actual == minClearance )
                return true;

            if( item->Type() == PCB_TRACE_T || item->Type() == PCB_ARC_T )
            {
                // Edge collisions are allowed inside the holes of castellated pads
                for( PAD* castellatedPad : m_castellatedPads )
                {
                    if( castellatedPad->GetEffectiveHoleShape()->Collide( pos ) )
                        return true;
                }
            }

            std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( aErrorCode );

            // Only report clearance info if there is any; otherwise it's just a straight collision
            if( minClearance > 0 )
            {
                wxString msg = formatMsg( _( "(%s clearance %s; actual %s)" ),
                                          constraint.GetName(),
                                          minClearance,
                                          actual );

                drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
            }

            drce->SetItems( edge->m_Uuid, item->m_Uuid );
            drce->SetViolatingRule( constraint.GetParentRule() );

            reportViolation( drce, pos, Edge_Cuts );
            return false;       // don't report violations with multiple edges; one is enough
        }
    }

    return true;
}


bool DRC_TEST_PROVIDER_EDGE_CLEARANCE::Run()
{
    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_EDGE_CLEARANCE ) )
    {
        if( !reportPhase( _( "Checking copper to board edge clearances..." ) ) )
            return false;    // DRC cancelled
    }
    else if( m_drcEngine->IsErrorLimitExceeded( DRCE_SILK_EDGE_CLEARANCE ) )
    {
        if( !reportPhase( _( "Checking silk to board edge clearances..." ) ) )
            return false;    // DRC cancelled
    }
    else
    {
        reportAux( wxT( "Edge clearance violations ignored. Tests not run." ) );
        return true;         // continue with other tests
    }

    m_board = m_drcEngine->GetBoard();
    m_castellatedPads.clear();

    DRC_CONSTRAINT worstClearanceConstraint;

    if( m_drcEngine->QueryWorstConstraint( EDGE_CLEARANCE_CONSTRAINT, worstClearanceConstraint ) )
        m_largestEdgeClearance = worstClearanceConstraint.GetValue().Min();

    reportAux( wxT( "Worst clearance : %d nm" ), m_largestEdgeClearance );

    /*
     * Build an RTree of the various edges (including NPTH holes) and margins found on the board.
     */
    std::vector<std::unique_ptr<PCB_SHAPE>> edges;
    DRC_RTREE                               edgesTree;

    forEachGeometryItem( { PCB_SHAPE_T }, LSET( 2, Edge_Cuts, Margin ),
            [&]( BOARD_ITEM *item ) -> bool
            {
                PCB_SHAPE*    shape = static_cast<PCB_SHAPE*>( item );
                STROKE_PARAMS stroke = shape->GetStroke();

                if( item->IsOnLayer( Edge_Cuts ) )
                    stroke.SetWidth( 0 );

                if( shape->GetShape() == SHAPE_T::RECT && !shape->IsFilled() )
                {
                    // A single rectangle for the board would make the RTree useless, so convert
                    // to 4 edges
                    edges.emplace_back( static_cast<PCB_SHAPE*>( shape->Clone() ) );
                    edges.back()->SetShape( SHAPE_T::SEGMENT );
                    edges.back()->SetEndX( shape->GetStartX() );
                    edges.back()->SetStroke( stroke );
                    edges.back()->SetParentGroup( nullptr );
                    edges.emplace_back( static_cast<PCB_SHAPE*>( shape->Clone() ) );
                    edges.back()->SetShape( SHAPE_T::SEGMENT );
                    edges.back()->SetEndY( shape->GetStartY() );
                    edges.back()->SetStroke( stroke );
                    edges.back()->SetParentGroup( nullptr );
                    edges.emplace_back( static_cast<PCB_SHAPE*>( shape->Clone() ) );
                    edges.back()->SetShape( SHAPE_T::SEGMENT );
                    edges.back()->SetStartX( shape->GetEndX() );
                    edges.back()->SetStroke( stroke );
                    edges.back()->SetParentGroup( nullptr );
                    edges.emplace_back( static_cast<PCB_SHAPE*>( shape->Clone() ) );
                    edges.back()->SetShape( SHAPE_T::SEGMENT );
                    edges.back()->SetStartY( shape->GetEndY() );
                    edges.back()->SetStroke( stroke );
                    edges.back()->SetParentGroup( nullptr );
                }
                else if( shape->GetShape() == SHAPE_T::POLY && !shape->IsFilled() )
                {
                    // A single polygon for the board would make the RTree useless, so convert
                    // to n edges.
                    SHAPE_LINE_CHAIN poly = shape->GetPolyShape().Outline( 0 );

                    for( size_t ii = 0; ii < poly.GetSegmentCount(); ++ii )
                    {
                        SEG seg = poly.CSegment( ii );
                        edges.emplace_back( static_cast<PCB_SHAPE*>( shape->Clone() ) );
                        edges.back()->SetShape( SHAPE_T::SEGMENT );
                        edges.back()->SetStart( seg.A );
                        edges.back()->SetEnd( seg.B );
                        edges.back()->SetStroke( stroke );
                        edges.back()->SetParentGroup( nullptr );
                    }
                }
                else
                {
                    edges.emplace_back( static_cast<PCB_SHAPE*>( shape->Clone() ) );
                    edges.back()->SetStroke( stroke );
                    edges.back()->SetParentGroup( nullptr );
                }

                return true;
            } );

    for( const std::unique_ptr<PCB_SHAPE>& edge : edges )
    {
        for( PCB_LAYER_ID layer : { Edge_Cuts, Margin } )
        {
            if( edge->IsOnLayer( layer ) )
                edgesTree.Insert( edge.get(), layer, m_largestEdgeClearance );
        }
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            if( pad->GetAttribute() == PAD_ATTRIB::NPTH && pad->HasHole() )
            {
                // edge-clearances are for milling tolerances (drilling tolerances are handled
                // by hole-clearances)
                if( pad->GetDrillSizeX() != pad->GetDrillSizeY() )
                    edgesTree.Insert( pad, Edge_Cuts, m_largestEdgeClearance );
            }

            if( pad->GetProperty() == PAD_PROP::CASTELLATED )
                m_castellatedPads.push_back( pad );
        }
    }

    /*
     * Test copper and silk items against the set of edges.
     */
    const int progressDelta = 200;
    int       count = 0;
    int       ii = 0;

    forEachGeometryItem( s_allBasicItemsButZones, LSET::AllLayersMask(),
            [&]( BOARD_ITEM *item ) -> bool
            {
                count++;
                return true;
            } );

    forEachGeometryItem( s_allBasicItemsButZones, LSET::AllLayersMask(),
            [&]( BOARD_ITEM *item ) -> bool
            {
                bool testCopper = !m_drcEngine->IsErrorLimitExceeded( DRCE_EDGE_CLEARANCE );
                bool testSilk = !m_drcEngine->IsErrorLimitExceeded( DRCE_SILK_EDGE_CLEARANCE );

                if( !testCopper && !testSilk )
                    return false;       // All limits exceeded; we're done

                if( !reportProgress( ii++, count, progressDelta ) )
                    return false;       // DRC cancelled; we're done

                if( isInvisibleText( item ) )
                    return true;        // Continue with other items

                if( item->Type() == PCB_PAD_T )
                {
                    PAD* pad = static_cast<PAD*>( item );

                    if( pad->GetProperty() == PAD_PROP::CASTELLATED
                        || pad->GetAttribute() == PAD_ATTRIB::CONN )
                    {
                        return true;    // Continue with other items
                    }
                }

                const std::shared_ptr<SHAPE>& itemShape = item->GetEffectiveShape();

                for( PCB_LAYER_ID testLayer : { Edge_Cuts, Margin } )
                {
                    if( testCopper && item->IsOnCopperLayer() )
                    {
                        edgesTree.QueryColliding( item, UNDEFINED_LAYER, testLayer, nullptr,
                                [&]( BOARD_ITEM* edge ) -> bool
                                {
                                    return testAgainstEdge( item, itemShape.get(), edge,
                                                            EDGE_CLEARANCE_CONSTRAINT,
                                                            DRCE_EDGE_CLEARANCE );
                                },
                                m_largestEdgeClearance );
                    }

                    if( testSilk && ( item->IsOnLayer( F_SilkS ) || item->IsOnLayer( B_SilkS ) ) )
                    {
                        if( edgesTree.QueryColliding( item, UNDEFINED_LAYER, testLayer, nullptr,
                                [&]( BOARD_ITEM* edge ) -> bool
                                {
                                    return testAgainstEdge( item, itemShape.get(), edge,
                                                            SILK_CLEARANCE_CONSTRAINT,
                                                            DRCE_SILK_EDGE_CLEARANCE );
                                },
                                m_largestEdgeClearance ) )
                        {
                            // violations reported during QueryColliding
                        }
                        else
                        {
                            // TODO: check postion being outside board boundary
                        }
                    }
                }

                return true;
            } );

    reportRuleStatistics();

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_EDGE_CLEARANCE> dummy;
}
