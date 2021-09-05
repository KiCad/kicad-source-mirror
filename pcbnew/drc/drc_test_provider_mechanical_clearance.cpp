/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers.
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
#include <macros.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <zone.h>
#include <advanced_config.h>
#include <geometry/seg.h>
#include <geometry/shape_segment.h>
#include <drc/drc_engine.h>
#include <drc/drc_rtree.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider_clearance_base.h>

/*
    Mechanical clearance test.

    Errors generated:
    - DRCE_MECHANICAL_CLEARANCE
    - DRCE_MECHANICAL_HOLE_CLEARANCE
*/

class DRC_TEST_PROVIDER_MECHANICAL_CLEARANCE : public DRC_TEST_PROVIDER_CLEARANCE_BASE
{
public:
    DRC_TEST_PROVIDER_MECHANICAL_CLEARANCE () :
            DRC_TEST_PROVIDER_CLEARANCE_BASE()
    {
    }

    virtual ~DRC_TEST_PROVIDER_MECHANICAL_CLEARANCE()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "mechanical_clearance";
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests item clearances irrespective of nets";
    }

private:
    bool testItemAgainstItem( BOARD_ITEM* item, SHAPE* itemShape, PCB_LAYER_ID layer,
                              BOARD_ITEM* other );

    void testItemAgainstZones( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer );

    void testShapeLineChain( const SHAPE_LINE_CHAIN& aOutline, int aLineWidth,
                             BOARD_ITEM* aParentItem, DRC_CONSTRAINT& aConstraint );

    void testZoneLayer( ZONE* aZone, PCB_LAYER_ID aLayer, DRC_CONSTRAINT& aConstraint );

private:
    DRC_RTREE                m_itemTree;
    std::vector<BOARD_ITEM*> m_items;
    std::vector<ZONE*>       m_zones;
};


bool DRC_TEST_PROVIDER_MECHANICAL_CLEARANCE::Run()
{
    m_board = m_drcEngine->GetBoard();
    m_itemTree.clear();
    m_zones.clear();
    m_items.clear();

    int            errorMax = m_board->GetDesignSettings().m_MaxError;
    DRC_CONSTRAINT worstConstraint;

    if( m_drcEngine->QueryWorstConstraint( MECHANICAL_CLEARANCE_CONSTRAINT, worstConstraint ) )
        m_largestClearance = worstConstraint.GetValue().Min();

    if( m_drcEngine->QueryWorstConstraint( MECHANICAL_HOLE_CLEARANCE_CONSTRAINT, worstConstraint ) )
        m_largestClearance = std::max( m_largestClearance, worstConstraint.GetValue().Min() );

    if( m_largestClearance <= 0 )
    {
        reportAux( "No Clearance constraints found. Tests not run." );
        return true;   // continue with other tests
    }

    for( ZONE* zone : m_board->Zones() )
    {
        if( !zone->GetIsRuleArea() )
        {
            m_zones.push_back( zone );
            m_largestClearance = std::max( m_largestClearance, zone->GetLocalClearance() );
        }
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
            m_largestClearance = std::max( m_largestClearance, pad->GetLocalClearance() );

        for( ZONE* zone : footprint->Zones() )
        {
            if( !zone->GetIsRuleArea() )
            {
                m_zones.push_back( zone );
                m_largestClearance = std::max( m_largestClearance, zone->GetLocalClearance() );
            }
        }
    }

    reportAux( "Worst clearance : %d nm", m_largestClearance );

    // This is the number of tests between 2 calls to the progress bar
    size_t delta = 50;
    size_t count = 0;
    size_t ii = 0;

    auto countItems =
            [&]( BOARD_ITEM* item ) -> bool
            {
                ++count;
                return true;
            };

    auto addToItemTree =
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( !reportProgress( ii++, count, delta ) )
                    return false;

                m_items.push_back( item );

                LSET layers = item->GetLayerSet();

                // Special-case pad holes which pierce all the copper layers
                if( item->Type() == PCB_PAD_T )
                {
                    PAD* pad = static_cast<PAD*>( item );

                    if( pad->GetDrillSizeX() > 0 && pad->GetDrillSizeY() > 0 )
                        layers |= LSET::AllCuMask();
                }

                for( PCB_LAYER_ID layer : layers.Seq() )
                    m_itemTree.Insert( item, layer, m_largestClearance );

                return true;
            };

    if( !reportPhase( _( "Gathering items..." ) ) )
        return false;   // DRC cancelled

    static const std::vector<KICAD_T> itemTypes = {
        PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T, PCB_PAD_T, PCB_SHAPE_T, PCB_FP_SHAPE_T,
        PCB_TEXT_T, PCB_FP_TEXT_T, PCB_DIMENSION_T, PCB_DIM_ALIGNED_T, PCB_DIM_LEADER_T,
        PCB_DIM_CENTER_T,  PCB_DIM_RADIAL_T, PCB_DIM_ORTHOGONAL_T
    };

    forEachGeometryItem( itemTypes, LSET::AllLayersMask(), countItems );
    forEachGeometryItem( itemTypes, LSET::AllLayersMask(), addToItemTree );

    std::map< std::pair<BOARD_ITEM*, BOARD_ITEM*>, int> checkedPairs;

    auto testItem =
            [&]( BOARD_ITEM* item, PCB_LAYER_ID layer )
            {
                std::shared_ptr<SHAPE> itemShape = item->GetEffectiveShape( layer );

                m_itemTree.QueryColliding( item, layer, layer,
                        // Filter:
                        [&]( BOARD_ITEM* other ) -> bool
                        {
                            BOARD_ITEM* a = item;
                            BOARD_ITEM* b = other;

                            // store canonical order so we don't collide in both directions
                            // (a:b and b:a)
                            if( static_cast<void*>( a ) > static_cast<void*>( b ) )
                                std::swap( a, b );

                            if( checkedPairs.count( { a, b } ) )
                            {
                                return false;
                            }
                            else
                            {
                                checkedPairs[ { a, b } ] = 1;
                                return true;
                            }
                        },
                        // Visitor:
                        [&]( BOARD_ITEM* other ) -> bool
                        {
                            return testItemAgainstItem( item, itemShape.get(), layer, other );
                        },
                        m_largestClearance );

                testItemAgainstZones( item, layer );
            };

    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE )
            || !m_drcEngine->IsErrorLimitExceeded( DRCE_HOLE_CLEARANCE ) )
    {
        if( !reportPhase( _( "Checking mechanical clearances..." ) ) )
            return false;   // DRC cancelled

        ii = 0;

        for( BOARD_ITEM* item : m_items )
        {
            if( !reportProgress( ii++, m_items.size(), delta ) )
                break;

            for( PCB_LAYER_ID layer : item->GetLayerSet().Seq() )
                testItem( item, layer );
        }
    }

    count = 0;
    forEachGeometryItem( { PCB_ZONE_T, PCB_FP_ZONE_T, PCB_SHAPE_T, PCB_FP_SHAPE_T },
            LSET::AllCuMask(),
            [&]( BOARD_ITEM* item ) -> bool
            {
                PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item );
                ZONE*      zone = dynamic_cast<ZONE*>( item );

                if( shape )
                {
                    switch( shape->GetShape() )
                    {
                    case SHAPE_T::POLY:
                    case SHAPE_T::BEZIER:
                    case SHAPE_T::ARC:
                        if( IsCopperLayer( shape->GetLayer() ) )
                            count++;

                        break;

                    default:
                        break;
                    }
                }

                if( zone )
                {
                    count += ( item->GetLayerSet() & LSET::AllCuMask() ).count();
                }

                return true;
            } );

    ii = 0;
    forEachGeometryItem( { PCB_ZONE_T, PCB_FP_ZONE_T, PCB_SHAPE_T, PCB_FP_SHAPE_T },
            LSET::AllCuMask(),
            [&]( BOARD_ITEM* item ) -> bool
            {
                for( PCB_LAYER_ID layer : item->GetLayerSet().Seq() )
                {
                    if( IsCopperLayer( layer ) )
                    {
                        if( !reportProgress( ii++, count, delta ) )
                            return false;

                        PCB_SHAPE*     shape = dynamic_cast<PCB_SHAPE*>( item );
                        ZONE*          zone = dynamic_cast<ZONE*>( item );
                        DRC_CONSTRAINT c = m_drcEngine->EvalRules( MECHANICAL_CLEARANCE_CONSTRAINT,
                                                                   item, nullptr, layer );

                        if( shape )
                        {
                            switch( shape->GetShape() )
                            {
                            case SHAPE_T::POLY:
                                testShapeLineChain( shape->GetPolyShape().Outline( 0 ),
                                                    shape->GetWidth(), item, c );
                                break;

                            case SHAPE_T::BEZIER:
                            {
                                SHAPE_LINE_CHAIN asPoly;

                                shape->RebuildBezierToSegmentsPointsList( shape->GetWidth() );

                                for( const wxPoint& pt : shape->GetBezierPoints() )
                                    asPoly.Append( pt );

                                testShapeLineChain( asPoly, shape->GetWidth(), item, c );
                                break;
                            }

                            case SHAPE_T::ARC:
                            {
                                SHAPE_LINE_CHAIN asPoly;

                                wxPoint center = shape->GetCenter();
                                double  angle  = -shape->GetArcAngle();
                                double  r      = shape->GetRadius();
                                int     steps  = GetArcToSegmentCount( r, errorMax, angle / 10.0 );

                                asPoly.Append( shape->GetStart() );

                                for( int step = 1; step <= steps; ++step )
                                {
                                    double rotation = ( angle * step ) / steps;

                                    wxPoint pt = shape->GetStart();
                                    RotatePoint( &pt, center, rotation );
                                    asPoly.Append( pt );
                                }

                                testShapeLineChain( asPoly, shape->GetWidth(), item, c );
                                break;
                            }

                            default:
                                UNIMPLEMENTED_FOR( shape->SHAPE_T_asString() );
                            }
                        }

                        if( zone )
                        {
                            testZoneLayer( static_cast<ZONE*>( item ), layer, c );
                        }
                    }
                }

                return true;
            } );

    reportRuleStatistics();

    return true;
}


void DRC_TEST_PROVIDER_MECHANICAL_CLEARANCE::testShapeLineChain( const SHAPE_LINE_CHAIN& aOutline,
                                                                 int aLineWidth,
                                                                 BOARD_ITEM* aParentItem,
                                                                 DRC_CONSTRAINT& aConstraint )
{
    // We don't want to collide with neighboring segments forming a curve until the concavity
    // approaches 180 degrees.
    double angleTolerance = DEG2RAD( 180.0 - ADVANCED_CFG::GetCfg().m_SliverAngleTolerance );
    int    epsilon = m_board->GetDesignSettings().GetDRCEpsilon();
    int    count = aOutline.SegmentCount();
    int    clearance = aConstraint.GetValue().Min();

    // Trigonometry is not cheap; cache seg angles
    std::vector<double> angles;
    angles.reserve( count );

    auto angleDiff =
            []( double a, double b ) -> double
            {
                if( a > b )
                    std::swap( a, b );

                double diff = b - a;

                if( diff > M_PI )
                    return 2 * M_PI - diff;
                else
                    return diff;
            };

    for( int ii = 0; ii < count; ++ii )
    {
        const SEG& seg = aOutline.CSegment( ii );

        // NB: don't store angles of really short segments (which could point anywhere)

        if( seg.SquaredLength() > SEG::Square( epsilon * 2 ) )
        {
            angles.push_back( ( seg.B - seg.A ).Angle() );
        }
        else if( ii > 0 )
        {
            angles.push_back( angles.back() );
        }
        else
        {
            for( int jj = 1; jj < count; ++jj )
            {
                const SEG& following = aOutline.CSegment( jj );

                if( following.SquaredLength() > SEG::Square( epsilon * 2 ) || jj == count - 1 )
                {
                    angles.push_back( ( following.B - following.A ).Angle() );
                    break;
                }
            }
        }
    }

    // Find collisions before reporting so that we can condense them into fewer reports.
    std::vector< std::pair<VECTOR2I, int> > collisions;

    for( int ii = 0; ii < count; ++ii )
    {
        const SEG seg = aOutline.CSegment( ii );
        double    segAngle = angles[ ii ];

        // Exclude segments on either side of us until we reach the angle tolerance
        int firstCandidate = ii + 1;
        int lastCandidate = count - 1;

        while( firstCandidate < count )
        {
            if( angleDiff( segAngle, angles[ firstCandidate ] ) < angleTolerance )
                firstCandidate++;
            else
                break;
        }

        if( aOutline.IsClosed() )
        {
            if( ii > 0 )
                lastCandidate = ii - 1;

            while( lastCandidate != std::min( firstCandidate, count - 1 ) )
            {
                if( angleDiff( segAngle, angles[ lastCandidate ] ) < angleTolerance )
                    lastCandidate = ( lastCandidate == 0 ) ? count - 1 : lastCandidate - 1;
                else
                    break;
            }
        }

        // Now run the collision between seg and each candidate seg in the candidate range.
        if( lastCandidate < ii )
            lastCandidate = count - 1;

        for( int jj = firstCandidate; jj <= lastCandidate; ++jj )
        {
            const SEG candidate = aOutline.CSegment( jj );
            int       actual;

            if( seg.Collide( candidate, clearance + aLineWidth - epsilon, &actual ) )
            {
                VECTOR2I firstPoint = seg.NearestPoint( candidate );
                VECTOR2I secondPoint = candidate.NearestPoint( seg );
                VECTOR2I pos = ( firstPoint + secondPoint ) / 2;

                if( !collisions.empty() &&
                        ( pos - collisions.back().first ).EuclideanNorm() < clearance * 2 )
                {
                    if( actual < collisions.back().second )
                    {
                        collisions.back().first = pos;
                        collisions.back().second = actual;
                    }
                    continue;
                }

                collisions.push_back( { pos, actual } );
            }
        }
    }

    for( std::pair<VECTOR2I, int> collision : collisions )
    {
        std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_CLEARANCE );

        m_msg.Printf( _( "(%s clearance %s; actual %s)" ),
                      aConstraint.GetName(),
                      MessageTextFromValue( userUnits(), clearance ),
                      MessageTextFromValue( userUnits(), collision.second ) );

        drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
        drce->SetItems( aParentItem );
        drce->SetViolatingRule( aConstraint.GetParentRule() );

        reportViolation( drce, (wxPoint) collision.first );
    }
}


void DRC_TEST_PROVIDER_MECHANICAL_CLEARANCE::testZoneLayer( ZONE* aZone, PCB_LAYER_ID aLayer,
                                                            DRC_CONSTRAINT& aConstraint )
{
    int            epsilon = m_board->GetDesignSettings().GetDRCEpsilon();
    int            clearance = aConstraint.GetValue().Min();
    SHAPE_POLY_SET fill = aZone->GetFilledPolysList( aLayer );

    if( aConstraint.GetSeverity() == RPT_SEVERITY_IGNORE || clearance - epsilon <= 0 )
        return;

    // Turn fractured fill into outlines and holes
    fill.Simplify( SHAPE_POLY_SET::PM_FAST );

    for( int outlineIdx = 0; outlineIdx < fill.OutlineCount(); ++outlineIdx )
    {
        SHAPE_LINE_CHAIN* firstOutline = &fill.Outline( outlineIdx );

        // Step one: outline to outline clearance violations

        for( int ii = outlineIdx + 1; ii < fill.OutlineCount(); ++ii )
        {
            SHAPE_LINE_CHAIN* secondOutline = &fill.Outline( ii );

            for( int jj = 0; jj < secondOutline->SegmentCount(); ++jj )
            {
                SEG      secondSeg = secondOutline->Segment( jj );
                int      actual;
                VECTOR2I pos;

                if( firstOutline->Collide( secondSeg, clearance - epsilon, &actual, &pos ) )
                {
                    std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_CLEARANCE );

                    m_msg.Printf( _( "(%s clearance %s; actual %s)" ),
                                  aConstraint.GetName(),
                                  MessageTextFromValue( userUnits(), clearance ),
                                  MessageTextFromValue( userUnits(), actual ) );

                    drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
                    drce->SetItems( aZone );
                    drce->SetViolatingRule( aConstraint.GetParentRule() );

                    reportViolation( drce, (wxPoint) pos );
                }
            }
        }

        // Step two: interior hole clearance violations

        for( int holeIdx = 0; holeIdx < fill.HoleCount( outlineIdx ); ++holeIdx )
        {
            testShapeLineChain( fill.Hole( outlineIdx, holeIdx ), 0, aZone, aConstraint );
        }
    }
}


bool DRC_TEST_PROVIDER_MECHANICAL_CLEARANCE::testItemAgainstItem( BOARD_ITEM* item,
                                                                  SHAPE* itemShape,
                                                                  PCB_LAYER_ID layer,
                                                                  BOARD_ITEM* other )
{
    bool           testClearance = !m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE );
    bool           testHoles = !m_drcEngine->IsErrorLimitExceeded( DRCE_HOLE_CLEARANCE );
    DRC_CONSTRAINT constraint;
    int            clearance = 0;
    int            actual;
    VECTOR2I       pos;

    std::shared_ptr<SHAPE> otherShape = DRC_ENGINE::GetShape( other, layer );

    if( testClearance )
    {
        constraint = m_drcEngine->EvalRules( MECHANICAL_CLEARANCE_CONSTRAINT, item, other, layer );
        clearance = constraint.GetValue().Min();
    }

    if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance > 0 )
    {
        if( itemShape->Collide( otherShape.get(), clearance, &actual, &pos ) )
        {
            std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_CLEARANCE );

            m_msg.Printf( _( "(%s clearance %s; actual %s)" ),
                          constraint.GetName(),
                          MessageTextFromValue( userUnits(), clearance ),
                          MessageTextFromValue( userUnits(), actual ) );

            drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
            drce->SetItems( item, other );
            drce->SetViolatingRule( constraint.GetParentRule() );

            reportViolation( drce, (wxPoint) pos );
        }
    }

    if( testHoles )
    {
        std::unique_ptr<SHAPE_SEGMENT> itemHoleShape;
        std::unique_ptr<SHAPE_SEGMENT> otherHoleShape;
        clearance = 0;

        if( item->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( item );
            pos = via->GetPosition();

            if( via->GetLayerSet().Contains( layer ) )
                itemHoleShape.reset( new SHAPE_SEGMENT( pos, pos, via->GetDrill() ) );
        }
        else if( item->Type() == PCB_PAD_T )
        {
            PAD* pad = static_cast<PAD*>( item );

            if( pad->GetDrillSize().x )
                itemHoleShape.reset( new SHAPE_SEGMENT( *pad->GetEffectiveHoleShape() ) );
        }

        if( other->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( other );
            pos = via->GetPosition();

            if( via->GetLayerSet().Contains( layer ) )
                otherHoleShape.reset( new SHAPE_SEGMENT( pos, pos, via->GetDrill() ) );
        }
        else if( other->Type() == PCB_PAD_T )
        {
            PAD* pad = static_cast<PAD*>( other );

            if( pad->GetDrillSize().x )
                otherHoleShape.reset( new SHAPE_SEGMENT( *pad->GetEffectiveHoleShape() ) );
        }

        if( itemHoleShape || otherHoleShape )
        {
            constraint = m_drcEngine->EvalRules( MECHANICAL_HOLE_CLEARANCE_CONSTRAINT, other, item,
                                                 layer );
            clearance = constraint.GetValue().Min();
        }

        if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance > 0 )
        {
            if( itemHoleShape && itemHoleShape->Collide( otherShape.get(), clearance, &actual, &pos ) )
            {
                std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );

                m_msg.Printf( _( "(%s clearance %s; actual %s)" ),
                              constraint.GetName(),
                              MessageTextFromValue( userUnits(), clearance ),
                              MessageTextFromValue( userUnits(), actual ) );

                drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
                drce->SetItems( item, other );
                drce->SetViolatingRule( constraint.GetParentRule() );

                reportViolation( drce, (wxPoint) pos );
            }

            if( otherHoleShape && otherHoleShape->Collide( itemShape, clearance, &actual, &pos ) )
            {
                std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );

                m_msg.Printf( _( "(%s clearance %s; actual %s)" ),
                              constraint.GetName(),
                              MessageTextFromValue( userUnits(), clearance ),
                              MessageTextFromValue( userUnits(), actual ) );

                drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
                drce->SetItems( item, other );
                drce->SetViolatingRule( constraint.GetParentRule() );

                reportViolation( drce, (wxPoint) pos );
            }
        }
    }

    return true;
}


void DRC_TEST_PROVIDER_MECHANICAL_CLEARANCE::testItemAgainstZones( BOARD_ITEM* aItem,
                                                                   PCB_LAYER_ID aLayer )
{
    for( ZONE* zone : m_zones )
    {
        if( !zone->GetLayerSet().test( aLayer ) )
            continue;

        if( aItem->GetBoundingBox().Intersects( zone->GetCachedBoundingBox() ) )
        {
            bool testClearance = !m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE );
            bool testHoles = !m_drcEngine->IsErrorLimitExceeded( DRCE_HOLE_CLEARANCE );

            if( !testClearance && !testHoles )
                return;

            DRC_RTREE*     zoneTree = m_board->m_CopperZoneRTrees[ zone ].get();
            EDA_RECT       itemBBox = aItem->GetBoundingBox();
            DRC_CONSTRAINT constraint;
            bool           colliding;
            int            clearance = -1;
            int            actual;
            VECTOR2I       pos;

            if( testClearance )
            {
                constraint = m_drcEngine->EvalRules( MECHANICAL_CLEARANCE_CONSTRAINT, aItem, zone,
                                                     aLayer );
                clearance = constraint.GetValue().Min();
            }

            if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance > 0 )
            {
                std::shared_ptr<SHAPE> itemShape = aItem->GetEffectiveShape( aLayer );

                if( aItem->Type() == PCB_PAD_T )
                {
                    PAD* pad = static_cast<PAD*>( aItem );

                    if( !pad->FlashLayer( aLayer ) )
                    {
                        if( pad->GetDrillSize().x == 0 && pad->GetDrillSize().y == 0 )
                            continue;

                        const SHAPE_SEGMENT* hole = pad->GetEffectiveHoleShape();
                        int                  size = hole->GetWidth();

                        // Note: drill size represents finish size, which means the actual hole
                        // size is the plating thickness larger.
                        if( pad->GetAttribute() == PAD_ATTRIB::PTH )
                            size += m_board->GetDesignSettings().GetHolePlatingThickness();

                        itemShape = std::make_shared<SHAPE_SEGMENT>( hole->GetSeg(), size );
                    }
                }

                if( zoneTree )
                {
                    colliding = zoneTree->QueryColliding( itemBBox, itemShape.get(), aLayer,
                                                          clearance, &actual, &pos );
                }
                else
                {
                    colliding = zone->Outline()->Collide( itemShape.get(), clearance, &actual,
                                                          &pos );
                }

                if( colliding )
                {
                    std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_CLEARANCE );

                    m_msg.Printf( _( "(%s clearance %s; actual %s)" ),
                                  constraint.GetName(),
                                  MessageTextFromValue( userUnits(), clearance ),
                                  MessageTextFromValue( userUnits(), actual ) );

                    drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
                    drce->SetItems( aItem, zone );
                    drce->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drce, (wxPoint) pos );
                }
            }

            if( testHoles && ( aItem->Type() == PCB_VIA_T || aItem->Type() == PCB_PAD_T ) )
            {
                std::unique_ptr<SHAPE_SEGMENT> holeShape;

                if( aItem->Type() == PCB_VIA_T )
                {
                    PCB_VIA* via = static_cast<PCB_VIA*>( aItem );
                    pos = via->GetPosition();

                    if( via->GetLayerSet().Contains( aLayer ) )
                        holeShape.reset( new SHAPE_SEGMENT( pos, pos, via->GetDrill() ) );
                }
                else if( aItem->Type() == PCB_PAD_T )
                {
                    PAD* pad = static_cast<PAD*>( aItem );

                    if( pad->GetDrillSize().x )
                        holeShape.reset( new SHAPE_SEGMENT( *pad->GetEffectiveHoleShape() ) );
                }

                if( holeShape )
                {
                    constraint = m_drcEngine->EvalRules( MECHANICAL_HOLE_CLEARANCE_CONSTRAINT,
                                                         aItem, zone, aLayer );
                    clearance = constraint.GetValue().Min();

                    if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE
                            && clearance > 0
                            && zoneTree->QueryColliding( itemBBox, holeShape.get(), aLayer,
                                                         clearance, &actual, &pos ) )
                    {
                        std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );

                        m_msg.Printf( _( "(%s clearance %s; actual %s)" ),
                                      constraint.GetName(),
                                      MessageTextFromValue( userUnits(), clearance ),
                                      MessageTextFromValue( userUnits(), actual ) );

                        drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
                        drce->SetItems( aItem, zone );
                        drce->SetViolatingRule( constraint.GetParentRule() );

                        reportViolation( drce, (wxPoint) pos );
                    }
                }
            }
        }
    }
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_MECHANICAL_CLEARANCE> dummy;
}
