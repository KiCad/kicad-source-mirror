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
            DRC_TEST_PROVIDER_CLEARANCE_BASE()
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
};


bool DRC_TEST_PROVIDER_EDGE_CLEARANCE::testAgainstEdge( BOARD_ITEM* item, SHAPE* itemShape,
                                                        BOARD_ITEM* edge,
                                                        DRC_CONSTRAINT_T aConstraintType,
                                                        PCB_DRC_CODE aErrorCode )
{
    const std::shared_ptr<SHAPE>& shape = edge->GetEffectiveShape( Edge_Cuts );
    const SHAPE*                  edgeShape = shape.get();

    if( edge->Type() == PCB_PAD_T )
        edgeShape = static_cast<PAD*>( edge )->GetEffectiveHoleShape();

    auto     constraint = m_drcEngine->EvalRules( aConstraintType, edge, item, UNDEFINED_LAYER );
    int      minClearance = constraint.GetValue().Min();
    int      actual;
    VECTOR2I pos;

    if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && minClearance >= 0 )
    {
        if( itemShape->Collide( edgeShape, minClearance, &actual, &pos ) )
        {
            std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( aErrorCode );

            // Only report clearance info if there is any; otherwise it's just a straight collision
            if( minClearance > 0 )
            {
                m_msg.Printf( _( "(%s clearance %s; actual %s)" ),
                              constraint.GetName(),
                              MessageTextFromValue( userUnits(), minClearance ),
                              MessageTextFromValue( userUnits(), actual ) );

                drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
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

    DRC_CONSTRAINT worstClearanceConstraint;

    if( m_drcEngine->QueryWorstConstraint( EDGE_CLEARANCE_CONSTRAINT, worstClearanceConstraint ) )
        m_largestClearance = worstClearanceConstraint.GetValue().Min();

    reportAux( wxT( "Worst clearance : %d nm" ), m_largestClearance );

    std::vector<std::unique_ptr<PCB_SHAPE>> edges;
    DRC_RTREE                               edgesTree;

    forEachGeometryItem( { PCB_SHAPE_T, PCB_FP_SHAPE_T }, LSET( 2, Edge_Cuts, Margin ),
            [&]( BOARD_ITEM *item ) -> bool
            {
                PCB_SHAPE*    shape = static_cast<PCB_SHAPE*>( item );
                STROKE_PARAMS stroke( 0 );

                if( shape->GetShape() == SHAPE_T::RECT )
                {
                    // A single rectangle for the board would make the RTree useless, so convert
                    // to 4 edges
                    edges.emplace_back( static_cast<PCB_SHAPE*>( shape->Clone() ) );
                    edges.back()->SetShape( SHAPE_T::SEGMENT );
                    edges.back()->SetEndX( shape->GetStartX() );
                    edges.back()->SetStroke( stroke );
                    edges.emplace_back( static_cast<PCB_SHAPE*>( shape->Clone() ) );
                    edges.back()->SetShape( SHAPE_T::SEGMENT );
                    edges.back()->SetEndY( shape->GetStartY() );
                    edges.back()->SetStroke( stroke );
                    edges.emplace_back( static_cast<PCB_SHAPE*>( shape->Clone() ) );
                    edges.back()->SetShape( SHAPE_T::SEGMENT );
                    edges.back()->SetStartX( shape->GetEndX() );
                    edges.back()->SetStroke( stroke );
                    edges.emplace_back( static_cast<PCB_SHAPE*>( shape->Clone() ) );
                    edges.back()->SetShape( SHAPE_T::SEGMENT );
                    edges.back()->SetStartY( shape->GetEndY() );
                    edges.back()->SetStroke( stroke );
                    return true;
                }
                else if( shape->GetShape() == SHAPE_T::POLY )
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
                    }
                }

                edges.emplace_back( static_cast<PCB_SHAPE*>( shape->Clone() ) );
                edges.back()->SetStroke( stroke );
                return true;
            } );

    for( const std::unique_ptr<PCB_SHAPE>& edge : edges )
    {
        for( PCB_LAYER_ID layer : { Edge_Cuts, Margin } )
        {
            if( edge->IsOnLayer( layer ) )
                edgesTree.Insert( edge.get(), layer, m_largestClearance );
        }
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
                edgesTree.Insert( pad, Edge_Cuts, m_largestClearance );
        }
    }

    // This is the number of tests between 2 calls to the progress bar
    const int delta = 100;
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
                    return false;    // We're done

                if( !reportProgress( ii++, count, delta ) )
                    return false;    // DRC cancelled; we're done

                if( isInvisibleText( item ) )
                    return true;     // Continue with other items

                if( item->Type() == PCB_PAD_T
                        && static_cast<PAD*>( item )->GetProperty() == PAD_PROP::CASTELLATED )
                {
                    return true;
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
                                m_largestClearance );
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
                                m_largestClearance ) )
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
