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

#include <atomic>
#include <common.h>
#include <pcb_shape.h>
#include <pcb_board_outline.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <geometry/seg.h>
#include <geometry/shape_segment.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include <drc/drc_rtree.h>
#include <thread_pool.h>
#include <mutex>

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

    bool testAgainstEdge( BOARD_ITEM* item, SHAPE* itemShape, PCB_LAYER_ID shapeLayer, BOARD_ITEM* other,
                          DRC_CONSTRAINT_T aConstraintType, PCB_DRC_CODE aErrorCode );

private:
    std::vector<PAD*> m_castellatedPads;
    int               m_largestEdgeClearance;
    int               m_epsilon;
    DRC_RTREE         m_edgesTree;

    std::map<BOARD_ITEM*, SILK_DISPOSITION> m_silkDisposition;
    std::mutex                             m_silkMutex;
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

    {
        std::lock_guard<std::mutex> lock( m_silkMutex );
        m_silkDisposition[aItem] = disposition;
    }

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


bool DRC_TEST_PROVIDER_EDGE_CLEARANCE::testAgainstEdge( BOARD_ITEM* item, SHAPE* itemShape, PCB_LAYER_ID shapeLayer,
                                                        BOARD_ITEM* edge, DRC_CONSTRAINT_T aConstraintType,
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
        if( itemShape->Collide( shape.get(), std::max( 0, minClearance - m_epsilon ), &actual, &pos ) )
        {
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
            reportTwoItemGeometry( drcItem, pos, edge, item, shapeLayer, actual );

            if( aErrorCode == DRCE_SILK_EDGE_CLEARANCE )
            {
                std::lock_guard<std::mutex> lock( m_silkMutex );
                m_silkDisposition[item] = CROSSES_EDGE;
            }

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
                        for( SHAPE* subshape : shape->MakeEffectiveShapes( true ) )
                        {
                            if( SHAPE_SEGMENT* segment = dynamic_cast<SHAPE_SEGMENT*>( subshape ) )
                            {
                                edges.emplace_back( static_cast<PCB_SHAPE*>( shape->Clone() ) );
                                edges.back()->SetShape( SHAPE_T::SEGMENT );
                                edges.back()->SetStart( segment->GetStart() );
                                edges.back()->SetEnd( segment->GetEnd() );
                                edges.back()->SetStroke( stroke );
                            }
                            else if( SHAPE_ARC* arc = dynamic_cast<SHAPE_ARC*>( subshape ) )
                            {
                                edges.emplace_back( static_cast<PCB_SHAPE*>( shape->Clone() ) );
                                edges.back()->SetShape( SHAPE_T::ARC );
                                edges.back()->SetArcGeometry( arc->GetP0(), arc->GetArcMid(), arc->GetP1() );
                                edges.back()->SetStroke( stroke );
                            }
                            else
                            {
                                wxFAIL_MSG(
                                        wxString::Format( "Unexpected effective shape type %d for rounded rectangle",
                                                          (int) subshape->Type() ) );
                                continue;
                            }
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

    m_edgesTree.Build();

    /*
     * Collect all testable (item, layer, shape) tuples, then test against edges in parallel.
     * Flattening to per-layer work units ensures even distribution across threads, since
     * zones with many layers become many separate work units rather than one heavy item.
     * Pre-fetching shapes avoids per-zone mutex contention during parallel testing.
     */
    struct WORK_UNIT
    {
        BOARD_ITEM*            item;
        PCB_LAYER_ID           shapeLayer;
        std::shared_ptr<SHAPE> shape;
    };

    std::vector<WORK_UNIT> workUnits;

    forEachGeometryItem( s_allBasicItems, LSET::AllLayersMask(),
            [&]( BOARD_ITEM *item ) -> bool
            {
                if( isInvisibleText( item ) )
                    return true;

                if( item->Type() == PCB_PAD_T )
                {
                    PAD* pad = static_cast<PAD*>( item );

                    if( pad->GetProperty() == PAD_PROP::CASTELLATED
                        || pad->GetAttribute() == PAD_ATTRIB::CONN )
                    {
                        return true;
                    }
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

                for( PCB_LAYER_ID layer : layersToTest )
                {
                    workUnits.push_back(
                            { item, layer, item->GetEffectiveShape( layer ) } );
                }

                return true;
            } );

    std::atomic<size_t> done( 0 );
    size_t              count = workUnits.size();

    auto processWorkUnit =
            [&]( const int idx ) -> size_t
            {
                if( m_drcEngine->IsCancelled() )
                {
                    done.fetch_add( 1 );
                    return 0;
                }

                bool testCopper = !m_drcEngine->IsErrorLimitExceeded( DRCE_EDGE_CLEARANCE );
                bool testSilk = !m_drcEngine->IsErrorLimitExceeded( DRCE_SILK_EDGE_CLEARANCE );

                if( !testCopper && !testSilk )
                {
                    done.fetch_add( 1 );
                    return 0;
                }

                WORK_UNIT&  wu = workUnits[idx];
                BOARD_ITEM* item = wu.item;

                for( PCB_LAYER_ID testLayer : { Edge_Cuts, Margin } )
                {
                    if( testCopper && item->IsOnCopperLayer() )
                    {
                        m_edgesTree.QueryColliding( item, wu.shapeLayer, testLayer, nullptr,
                                [&]( BOARD_ITEM* edge ) -> bool
                                {
                                    return testAgainstEdge( item, wu.shape.get(),
                                                           wu.shapeLayer, edge,
                                                           EDGE_CLEARANCE_CONSTRAINT,
                                                           DRCE_EDGE_CLEARANCE );
                                },
                                m_largestEdgeClearance );
                    }

                    if( testSilk
                        && ( item->IsOnLayer( F_SilkS )
                             || item->IsOnLayer( B_SilkS ) ) )
                    {
                        m_edgesTree.QueryColliding( item, wu.shapeLayer, testLayer, nullptr,
                                [&]( BOARD_ITEM* edge ) -> bool
                                {
                                    return testAgainstEdge( item, wu.shape.get(),
                                                           wu.shapeLayer, edge,
                                                           SILK_CLEARANCE_CONSTRAINT,
                                                           DRCE_SILK_EDGE_CLEARANCE );
                                },
                                m_largestEdgeClearance );
                    }
                }

                if( testSilk
                    && ( item->IsOnLayer( F_SilkS ) || item->IsOnLayer( B_SilkS ) ) )
                {
                    bool needsResolution = false;

                    {
                        std::lock_guard<std::mutex> lock( m_silkMutex );
                        needsResolution = m_silkDisposition[item] == UNKNOWN;
                    }

                    if( needsResolution && m_board->BoardOutline()->HasOutline() )
                    {
                        resolveSilkDisposition( item, wu.shape.get(),
                                                m_board->BoardOutline()->GetOutline() );
                    }
                }

                done.fetch_add( 1 );
                return 1;
            };

    thread_pool& tp = GetKiCadThreadPool();
    size_t       numBlocks = count;
    auto         futures = tp.submit_loop( 0, count, processWorkUnit, numBlocks );

    while( done < count )
    {
        reportProgress( done, count );

        if( m_drcEngine->IsCancelled() )
        {
            for( auto& f : futures )
                f.wait();

            break;
        }

        std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );
    }

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_EDGE_CLEARANCE> dummy;
}
