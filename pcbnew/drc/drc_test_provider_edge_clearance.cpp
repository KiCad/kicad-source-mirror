/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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
#include <pcb_board_outline.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pcb_track.h>
#include <geometry/seg.h>
#include <geometry/shape_segment.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include "drc_rtree.h"

/*
    Board edge clearance test. Checks all items for their mechanical clearances against the board
    edge.
    Errors generated:
    - DRCE_EDGE_CLEARANCE
    - DRCE_SILK_EDGE_CLEARANCE
*/

enum SILK_DISPOSITION {
    UNKNOWN = 0,
    ON_BOARD,
    OFF_BOARD,
    CROSSES_EDGE
};


class DRC_TEST_PROVIDER_EDGE_CLEARANCE : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_EDGE_CLEARANCE () :
            DRC_TEST_PROVIDER(),
            m_largestEdgeClearance( 0 ),
            m_epsilon( 0 )
    {}

    virtual ~DRC_TEST_PROVIDER_EDGE_CLEARANCE() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "edge_clearance" ); }

private:
    void resolveSilkDisposition( BOARD_ITEM* aItem, const SHAPE* aItemShape, const SHAPE_POLY_SET& aBoardOutline );

    bool testAgainstEdge( BOARD_ITEM* item, SHAPE* itemShape, BOARD_ITEM* other, DRC_CONSTRAINT_T aConstraintType,
                          PCB_DRC_CODE aErrorCode );

private:
    std::vector<PAD*> m_castellatedPads;
    int               m_largestEdgeClearance;
    int               m_epsilon;
    DRC_RTREE         m_edgesTree;

    std::map<BOARD_ITEM*, SILK_DISPOSITION> m_silkDisposition;
};


void DRC_TEST_PROVIDER_EDGE_CLEARANCE::resolveSilkDisposition( BOARD_ITEM* aItem, const SHAPE* aItemShape,
                                                               const SHAPE_POLY_SET& aBoardOutline )
{
    SILK_DISPOSITION disposition = UNKNOWN;

    if( aItemShape->Type() == SH_COMPOUND )
    {
        const SHAPE_COMPOUND* compound = static_cast<const SHAPE_COMPOUND*>( aItemShape );

        for( const SHAPE* elem : compound->Shapes() )
        {
            SILK_DISPOSITION elem_disposition = aBoardOutline.Contains( elem->Centre() ) ? ON_BOARD : OFF_BOARD;

            if( disposition == UNKNOWN )
            {
                disposition = elem_disposition;
            }
            else if( disposition != elem_disposition )
            {
                disposition = CROSSES_EDGE;
                break;
            }
        }
    }
    else
    {
        disposition = aBoardOutline.Contains( aItemShape->Centre() ) ? ON_BOARD : OFF_BOARD;
    }

    m_silkDisposition[aItem] = disposition;

    if( disposition == CROSSES_EDGE )
    {
        BOARD_ITEM* nearestEdge = nullptr;
        VECTOR2I    itemPos = aItem->GetCenter();
        VECTOR2I    nearestEdgePt = aBoardOutline.Outline( 0 ).NearestPoint( itemPos, false );

        for( int outlineIdx = 1; outlineIdx < aBoardOutline.OutlineCount(); ++outlineIdx )
        {
            VECTOR2I otherEdgePt = aBoardOutline.Outline( outlineIdx ).NearestPoint( itemPos, false );

            if( otherEdgePt.SquaredDistance( itemPos ) < nearestEdgePt.SquaredDistance( itemPos ) )
                nearestEdgePt = otherEdgePt;
        }

        for( BOARD_ITEM* edge : m_edgesTree.GetObjectsAt( nearestEdgePt, Edge_Cuts, m_epsilon ) )
        {
            if( edge->HitTest( nearestEdgePt, m_epsilon ) )
            {
                nearestEdge = edge;
                break;
            }
        }

        if( !nearestEdge )
            return;

        auto constraint = m_drcEngine->EvalRules( SILK_CLEARANCE_CONSTRAINT, nearestEdge, aItem, UNDEFINED_LAYER );
        int  minClearance = constraint.GetValue().Min();

        if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && minClearance >= 0 )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_SILK_EDGE_CLEARANCE );

            // Report clearance info if there is any, even though crossing is just a straight-up collision
            if( minClearance > 0 )
            {
                drcItem->SetErrorDetail( formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                    constraint.GetName(),
                                                    minClearance,
                                                    0 ) );
            }

            drcItem->SetItems( nearestEdge->m_Uuid, aItem->m_Uuid );
            drcItem->SetViolatingRule( constraint.GetParentRule() );
            reportTwoPointGeometry( drcItem, nearestEdgePt, nearestEdgePt, nearestEdgePt, aItem->GetLayer() );
        }
    }
#if 0
    // If you want "Silk outside board edge" errors:
    else if( disposition == OFF_BOARD )
    {
        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_SILK_EDGE_CLEARANCE );
        drcItem->SetErrorMessage( _( "Silkscreen outside board edge" ) );

        drcItem->SetItems( aItem->m_Uuid );
        reportTwoPointGeometry( drcItem, aItem->GetCenter(), aItem->GetCenter(), aItem->GetCenter(),
                                aItem->GetLayer() );
    }
#endif
}


bool DRC_TEST_PROVIDER_EDGE_CLEARANCE::testAgainstEdge( BOARD_ITEM* item, SHAPE* itemShape, BOARD_ITEM* edge,
                                                        DRC_CONSTRAINT_T aConstraintType, PCB_DRC_CODE aErrorCode )
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

            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( aErrorCode );

            // Only report clearance info if there is any; otherwise it's just a straight collision
            if( minClearance > 0 )
            {
                drcItem->SetErrorDetail( formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                    constraint.GetName(),
                                                    minClearance,
                                                    actual ) );
            }

            drcItem->SetItems( edge->m_Uuid, item->m_Uuid );
            drcItem->SetViolatingRule( constraint.GetParentRule() );
            reportTwoItemGeometry( drcItem, pos, edge, item, Edge_Cuts, actual );

            if( aErrorCode == DRCE_SILK_EDGE_CLEARANCE )
                m_silkDisposition[item] = CROSSES_EDGE;

            if( item->Type() == PCB_TRACE_T || item->Type() == PCB_ARC_T )
                return m_drcEngine->GetReportAllTrackErrors();
            else
                return false;   // don't report violations with multiple edges; one is enough
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
    else if( !m_drcEngine->IsErrorLimitExceeded( DRCE_SILK_EDGE_CLEARANCE ) )
    {
        if( !reportPhase( _( "Checking silk to board edge clearances..." ) ) )
            return false;    // DRC cancelled
    }
    else
    {
        REPORT_AUX( wxT( "Edge clearance violations ignored. Tests not run." ) );
        return true;         // continue with other tests
    }

    m_board = m_drcEngine->GetBoard();
    m_castellatedPads.clear();
    m_epsilon = m_board->GetDesignSettings().GetDRCEpsilon();
    m_edgesTree.clear();
    m_silkDisposition.clear();

    DRC_CONSTRAINT worstClearanceConstraint;

    if( m_drcEngine->QueryWorstConstraint( EDGE_CLEARANCE_CONSTRAINT, worstClearanceConstraint ) )
        m_largestEdgeClearance = worstClearanceConstraint.GetValue().Min();

    /*
     * Build an RTree of the various edges (including NPTH holes) and margins found on the board.
     */
    std::vector<std::unique_ptr<PCB_SHAPE>> edges;

    forEachGeometryItem( { PCB_SHAPE_T }, LSET( { Edge_Cuts, Margin } ),
            [&]( BOARD_ITEM *item ) -> bool
            {
                PCB_SHAPE*    shape = static_cast<PCB_SHAPE*>( item );
                STROKE_PARAMS stroke = shape->GetStroke();

                if( item->IsOnLayer( Edge_Cuts ) )
                    stroke.SetWidth( 0 );

                if( shape->GetShape() == SHAPE_T::RECTANGLE && !shape->IsSolidFill() )
                {
                    // A single rectangle for the board would defeat the RTree, so convert to edges
                    if( shape->GetCornerRadius() > 0 )
                    {
                        for( SHAPE* seg : shape->MakeEffectiveShapes( true ) )
                        {
                            wxCHECK2( dynamic_cast<SHAPE_SEGMENT*>( seg ), continue );

                            edges.emplace_back( static_cast<PCB_SHAPE*>( shape->Clone() ) );
                            edges.back()->SetShape( SHAPE_T::SEGMENT );
                            edges.back()->SetStart( seg->GetStart() );
                            edges.back()->SetEnd( seg->GetEnd() );
                            edges.back()->SetStroke( stroke );
                        }
                    }
                    else
                    {
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
                    }
                }
                else if( shape->GetShape() == SHAPE_T::POLY && !shape->IsSolidFill() )
                {
                    // A single polygon for the board would defeat the RTree, so convert to edges.
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
                else
                {
                    edges.emplace_back( static_cast<PCB_SHAPE*>( shape->Clone() ) );
                    edges.back()->SetStroke( stroke );
                }

                return true;
            } );

    for( const std::unique_ptr<PCB_SHAPE>& edge : edges )
    {
        for( PCB_LAYER_ID layer : { Edge_Cuts, Margin } )
        {
            if( edge->IsOnLayer( layer ) )
                m_edgesTree.Insert( edge.get(), layer, m_largestEdgeClearance );
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
                    m_edgesTree.Insert( pad, Edge_Cuts, m_largestEdgeClearance );
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

    forEachGeometryItem( s_allBasicItems, LSET::AllLayersMask(),
            [&]( BOARD_ITEM *item ) -> bool
            {
                count++;
                return true;
            } );

    forEachGeometryItem( s_allBasicItems, LSET::AllLayersMask(),
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

                    if( pad->GetProperty() == PAD_PROP::CASTELLATED || pad->GetAttribute() == PAD_ATTRIB::CONN )
                        return true;    // Continue with other items
                }

                std::vector<PCB_LAYER_ID> layersToTest;

                switch( item->Type() )
                {
                case PCB_PAD_T:
                    layersToTest = static_cast<PAD*>( item )->Padstack().UniqueLayers();
                    break;

                case PCB_VIA_T:
                    layersToTest = static_cast<PCB_VIA*>( item )->Padstack().UniqueLayers();
                    break;

                case PCB_ZONE_T:
                    for( PCB_LAYER_ID layer : item->GetLayerSet() )
                        layersToTest.push_back( layer );

                    break;

                default:
                    layersToTest = { UNDEFINED_LAYER };
                }

                for( PCB_LAYER_ID shapeLayer : layersToTest )
                {
                    const std::shared_ptr<SHAPE>& itemShape = item->GetEffectiveShape( shapeLayer );

                    for( PCB_LAYER_ID testLayer : { Edge_Cuts, Margin } )
                    {
                        if( testCopper && item->IsOnCopperLayer() )
                        {
                            m_edgesTree.QueryColliding( item, shapeLayer, testLayer, nullptr,
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
                            m_edgesTree.QueryColliding( item, shapeLayer, testLayer, nullptr,
                                    [&]( BOARD_ITEM* edge ) -> bool
                                    {
                                        return testAgainstEdge( item, itemShape.get(), edge,
                                                                SILK_CLEARANCE_CONSTRAINT,
                                                                DRCE_SILK_EDGE_CLEARANCE );
                                    },
                                    m_largestEdgeClearance );
                        }
                    }

                    if( testSilk && ( item->IsOnLayer( F_SilkS ) || item->IsOnLayer( B_SilkS ) ) )
                    {
                        if( m_silkDisposition[item] == UNKNOWN && m_board->BoardOutline()->HasOutline() )
                            resolveSilkDisposition( item, itemShape.get(), m_board->BoardOutline()->GetOutline() );
                    }
                }

                return true;
            } );

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_EDGE_CLEARANCE> dummy;
}
