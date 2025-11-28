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
#include <macros.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <zone.h>
#include <advanced_config.h>
#include <geometry/geometry_utils.h>
#include <geometry/seg.h>
#include <geometry/shape_segment.h>
#include <drc/drc_rtree.h>
#include <drc/drc_item.h>
#include <drc/drc_test_provider.h>

/*
    Physical clearance tests.

    Errors generated:
    - DRCE_PHYSICAL_CLEARANCE
    - DRCE_PHYSICAL_HOLE_CLEARANCE
*/

class DRC_TEST_PROVIDER_PHYSICAL_CLEARANCE : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_PHYSICAL_CLEARANCE () :
            DRC_TEST_PROVIDER()
    {}

    virtual ~DRC_TEST_PROVIDER_PHYSICAL_CLEARANCE() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "physical_clearance" ); };

private:
    int testItemAgainstItem( BOARD_ITEM* aItem, SHAPE* aItemShape, PCB_LAYER_ID aLayer,
                              BOARD_ITEM* aOther );

    void testItemAgainstZones( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer );

    void testShapeLineChain( const SHAPE_LINE_CHAIN& aOutline, int aLineWidth, PCB_LAYER_ID aLayer,
                             BOARD_ITEM* aParentItem, DRC_CONSTRAINT& aConstraint );

    void testZoneLayer( ZONE* aZone, PCB_LAYER_ID aLayer, DRC_CONSTRAINT& aConstraint );

private:
    DRC_RTREE          m_itemTree;
};


bool DRC_TEST_PROVIDER_PHYSICAL_CLEARANCE::Run()
{
    m_board = m_drcEngine->GetBoard();
    m_itemTree.clear();

    int  errorMax = m_board->GetDesignSettings().m_MaxError;
    LSET boardCopperLayers = LSET::AllCuMask( m_board->GetCopperLayerCount() );

    if( m_board->m_DRCMaxPhysicalClearance <= 0 )
    {
        REPORT_AUX( wxT( "No physical clearance constraints found. Tests not run." ) );
        return true;   // continue with other tests
    }

    size_t progressDelta = 250;
    size_t count = 0;
    size_t ii = 0;

    if( !reportPhase( _( "Gathering physical items..." ) ) )
        return false;   // DRC cancelled

    static const std::vector<KICAD_T> itemTypes = {
        PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T,
        PCB_FOOTPRINT_T,
        PCB_PAD_T,
        PCB_SHAPE_T,
        PCB_FIELD_T, PCB_TEXT_T, PCB_TEXTBOX_T,
        PCB_TABLE_T, PCB_TABLECELL_T,
        PCB_DIMENSION_T,
        PCB_BARCODE_T
    };

    static const LSET courtyards( { F_CrtYd, B_CrtYd } );

    //
    // Generate a count for use in progress reporting.
    //

    forEachGeometryItem( itemTypes, LSET::AllLayersMask(),
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( isInvisibleText( item ) )
                    return true;

                ++count;
                return true;
            } );

    //
    // Generate a BOARD_ITEM RTree.
    //

    forEachGeometryItem( itemTypes, LSET::AllLayersMask(),
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( isInvisibleText( item ) )
                    return true;

                if( !reportProgress( ii++, count, progressDelta ) )
                    return false;

                LSET layers = item->GetLayerSet();

                // Special-case holes and edge-cuts which pierce all physical layers
                if( item->HasHole() )
                {
                    if( layers.Contains( F_Cu ) )
                        layers |= LSET( LSET::FrontBoardTechMask() ).set( F_CrtYd );

                    if( layers.Contains( B_Cu ) )
                        layers |= LSET( LSET::BackBoardTechMask() ).set( B_CrtYd );

                    if( layers.Contains( F_Cu ) && layers.Contains( B_Cu ) )
                        layers |= boardCopperLayers;
                }
                else if( item->Type() == PCB_FOOTPRINT_T )
                {
                    layers = courtyards;
                }
                else if( item->IsOnLayer( Edge_Cuts ) )
                {
                    layers |= LSET::PhysicalLayersMask() | courtyards;
                }

                for( PCB_LAYER_ID layer : layers )
                    m_itemTree.Insert( item, layer, m_board->m_DRCMaxPhysicalClearance, ATOMIC_TABLES );

                return true;
            } );

    std::unordered_map<PTR_PTR_CACHE_KEY, LSET> checkedPairs;
    progressDelta = 100;
    ii = 0;

    //
    // Run clearance checks -between- items.
    //

    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE )
            || !m_drcEngine->IsErrorLimitExceeded( DRCE_HOLE_CLEARANCE ) )
    {
        if( !reportPhase( _( "Checking physical clearances..." ) ) )
            return false;   // DRC cancelled

        forEachGeometryItem( itemTypes, LSET::AllLayersMask(),
                [&]( BOARD_ITEM* item ) -> bool
                {
                    if( isInvisibleText( item ) )
                        return true;

                    if( !reportProgress( ii++, count, progressDelta ) )
                        return false;

                    LSET layers = item->GetLayerSet();

                    if( item->Type() == PCB_FOOTPRINT_T )
                        layers = courtyards;

                    for( PCB_LAYER_ID layer : layers )
                    {
                        std::shared_ptr<SHAPE> itemShape = item->GetEffectiveShape( layer );

                        m_itemTree.QueryColliding( item, layer, layer,
                                // Filter:
                                [&]( BOARD_ITEM* other ) -> bool
                                {
                                    if( item->Type() == PCB_TABLECELL_T && item->GetParent() == other )
                                        return false;

                                    BOARD_ITEM* a = item;
                                    BOARD_ITEM* b = other;

                                    // store canonical order so we don't collide in both
                                    // directions (a:b and b:a)
                                    if( static_cast<void*>( a ) > static_cast<void*>( b ) )
                                        std::swap( a, b );

                                    auto it = checkedPairs.find( { a, b } );

                                    if( it != checkedPairs.end() && it->second.test( layer ) )
                                    {
                                        return false;
                                    }
                                    else
                                    {
                                        checkedPairs[ { a, b } ].set( layer );
                                        return true;
                                    }
                                },
                                // Visitor:
                                [&]( BOARD_ITEM* other ) -> bool
                                {
                                    if( testItemAgainstItem( item, itemShape.get(), layer,
                                                                other ) > 0 )
                                    {
                                        BOARD_ITEM* a = item;
                                        BOARD_ITEM* b = other;

                                        // store canonical order
                                        if( static_cast<void*>( a ) > static_cast<void*>( b ) )
                                            std::swap( a, b );

                                        // Once we record one DRC for error for physical clearance
                                        // we don't need to record more
                                        checkedPairs[ { a, b } ].set();
                                    }

                                    return !m_drcEngine->IsCancelled();
                                },
                                m_board->m_DRCMaxPhysicalClearance );

                        testItemAgainstZones( item, layer );
                    }

                    return true;
                } );
    }

    progressDelta = 100;
    count = 0;
    ii = 0;

    //
    // Generate a count for progress reporting.
    //

    forEachGeometryItem( { PCB_ZONE_T, PCB_SHAPE_T }, boardCopperLayers,
            [&]( BOARD_ITEM* item ) -> bool
            {
                ZONE* zone = dynamic_cast<ZONE*>( item );

                if( zone && zone->GetIsRuleArea() )
                    return true;    // Continue with other items

                count += ( item->GetLayerSet() & boardCopperLayers ).count();

                return true;
            } );

    //
    // Run clearance checks -within- polygonal items.
    //

    forEachGeometryItem( { PCB_ZONE_T, PCB_SHAPE_T }, boardCopperLayers,
            [&]( BOARD_ITEM* item ) -> bool
            {
                PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item );
                ZONE*      zone = dynamic_cast<ZONE*>( item );

                if( zone && zone->GetIsRuleArea() )
                    return true;    // Continue with other items

                for( PCB_LAYER_ID layer : item->GetLayerSet() )
                {
                    if( IsCopperLayer( layer ) )
                    {
                        if( !reportProgress( ii++, count, progressDelta ) )
                            return false;

                        DRC_CONSTRAINT c = m_drcEngine->EvalRules( PHYSICAL_CLEARANCE_CONSTRAINT, item, nullptr,
                                                                   layer );

                        if( shape )
                        {
                            switch( shape->GetShape() )
                            {
                            case SHAPE_T::POLY:
                                testShapeLineChain( shape->GetPolyShape().Outline( 0 ), shape->GetWidth(), layer,
                                                    item, c );
                                break;

                            case SHAPE_T::BEZIER:
                            {
                                SHAPE_LINE_CHAIN asPoly;

                                shape->RebuildBezierToSegmentsPointsList( errorMax );

                                for( const VECTOR2I& pt : shape->GetBezierPoints() )
                                    asPoly.Append( pt );

                                testShapeLineChain( asPoly, shape->GetWidth(), layer, item, c );
                                break;
                            }

                            case SHAPE_T::ARC:
                            {
                                SHAPE_LINE_CHAIN asPoly;

                                VECTOR2I  center = shape->GetCenter();
                                EDA_ANGLE angle  = -shape->GetArcAngle();
                                double    r      = shape->GetRadius();
                                int       steps  = GetArcToSegmentCount( r, errorMax, angle );

                                asPoly.Append( shape->GetStart() );

                                for( int step = 1; step <= steps; ++step )
                                {
                                    EDA_ANGLE rotation = ( angle * step ) / steps;
                                    VECTOR2I  pt = shape->GetStart();

                                    RotatePoint( pt, center, rotation );
                                    asPoly.Append( pt );
                                }

                                testShapeLineChain( asPoly, shape->GetWidth(), layer, item, c );
                                break;
                            }

                            // Simple shapes can't create self-intersections, and I'm not sure a user
                            // would want a report that one side of their rectangle was too close to
                            // the other side.
                            case SHAPE_T::RECTANGLE:
                            case SHAPE_T::SEGMENT:
                            case SHAPE_T::CIRCLE:
                                break;

                            default:
                                UNIMPLEMENTED_FOR( shape->SHAPE_T_asString() );
                            }
                        }

                        if( zone )
                            testZoneLayer( static_cast<ZONE*>( item ), layer, c );
                    }

                    if( m_drcEngine->IsCancelled() )
                        return false;
                }

                return !m_drcEngine->IsCancelled();
            } );

    return !m_drcEngine->IsCancelled();
}


void DRC_TEST_PROVIDER_PHYSICAL_CLEARANCE::testShapeLineChain( const SHAPE_LINE_CHAIN& aOutline,
                                                               int aLineWidth, PCB_LAYER_ID aLayer,
                                                               BOARD_ITEM* aParentItem,
                                                               DRC_CONSTRAINT& aConstraint )
{
    // We don't want to collide with neighboring segments forming a curve until the concavity
    // approaches 180 degrees.
    double angleTolerance = DEG2RAD( 180.0 - ADVANCED_CFG::GetCfg().m_SliverAngleTolerance );
    int    epsilon = m_board->GetDesignSettings().GetDRCEpsilon();
    int    count = aOutline.SegmentCount();
    int    clearance = aConstraint.GetValue().Min();

    if( aConstraint.GetSeverity() == RPT_SEVERITY_IGNORE || clearance - epsilon <= 0 )
        return;

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
            angles.push_back( EDA_ANGLE( seg.B - seg.A ).AsRadians() );
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
                    angles.push_back( EDA_ANGLE( following.B - following.A ).AsRadians() );
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

                if( !collisions.empty() && ( pos - collisions.back().first ).EuclideanNorm() < clearance * 2 )
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

    for( const std::pair<VECTOR2I, int>& collision : collisions )
    {
        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );
        VECTOR2I pt = collision.first;

        if( FOOTPRINT* parentFP = aParentItem->GetParentFootprint() )
        {
            RotatePoint( pt, parentFP->GetOrientation() );
            pt += parentFP->GetPosition();
        }

        wxString msg = formatMsg( _( "Internal clearance violation (%s clearance %s; actual %s)" ),
                                  aConstraint.GetName(),
                                  clearance,
                                  collision.second );

        drcItem->SetErrorMessage( msg );
        drcItem->SetItems( aParentItem );
        drcItem->SetViolatingRule( aConstraint.GetParentRule() );

        reportViolation( drcItem, pt, aLayer );
    }
}


void DRC_TEST_PROVIDER_PHYSICAL_CLEARANCE::testZoneLayer( ZONE* aZone, PCB_LAYER_ID aLayer,
                                                          DRC_CONSTRAINT& aConstraint )
{
    int epsilon = m_board->GetDesignSettings().GetDRCEpsilon();
    int clearance = aConstraint.GetValue().Min();

    if( aConstraint.GetSeverity() == RPT_SEVERITY_IGNORE || clearance - epsilon <= 0 )
        return;

    SHAPE_POLY_SET fill = aZone->GetFilledPolysList( aLayer )->CloneDropTriangulation();

    // Turn fractured fill into outlines and holes
    fill.Simplify();

    for( int outlineIdx = 0; outlineIdx < fill.OutlineCount(); ++outlineIdx )
    {
        SHAPE_LINE_CHAIN* firstOutline = &fill.Outline( outlineIdx );

        //
        // Step one: outline to outline clearance violations
        //

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
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );
                    drcItem->SetErrorDetail( formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                        aConstraint.GetName(),
                                                        clearance,
                                                        actual ) );
                    drcItem->SetItems( aZone );
                    drcItem->SetViolatingRule( aConstraint.GetParentRule() );
                    reportViolation( drcItem, pos, aLayer );
                }
            }

            if( m_drcEngine->IsCancelled() )
                return;
        }

        //
        // Step two: interior hole clearance violations
        //

        for( int holeIdx = 0; holeIdx < fill.HoleCount( outlineIdx ); ++holeIdx )
        {
            testShapeLineChain( fill.Hole( outlineIdx, holeIdx ), 0, aLayer, aZone, aConstraint );

            if( m_drcEngine->IsCancelled() )
                return;
        }
    }
}


int DRC_TEST_PROVIDER_PHYSICAL_CLEARANCE::testItemAgainstItem( BOARD_ITEM* aItem, SHAPE* aItemShape,
                                                               PCB_LAYER_ID aLayer, BOARD_ITEM* aOther )
{
    bool           testClearance = !m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE );
    bool           testHoles = !m_drcEngine->IsErrorLimitExceeded( DRCE_HOLE_CLEARANCE );
    DRC_CONSTRAINT constraint;
    int            clearance = 0;
    int            actual;
    int            violations = 0;
    VECTOR2I       pos;
    LSET           boardCopperLayers = LSET::AllCuMask( m_board->GetCopperLayerCount() );

    std::shared_ptr<SHAPE> otherShapeStorage = aOther->GetEffectiveShape( aLayer );
    SHAPE*                 otherShape = otherShapeStorage.get();

    if( testClearance )
    {
        constraint = m_drcEngine->EvalRules( PHYSICAL_CLEARANCE_CONSTRAINT, aItem, aOther, aLayer );
        clearance = constraint.GetValue().Min();
    }

    if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance > 0 )
    {
        // Collide (and generate violations) based on a well-defined order so that exclusion
        // checking against previously-generated violations will work.
        if( aItem->m_Uuid > aOther->m_Uuid )
        {
            std::swap( aItem, aOther );
            std::swap( aItemShape, otherShape );
        }

        if( aItemShape->Collide( otherShape, clearance, &actual, &pos ) )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );
            drcItem->SetErrorDetail( formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                constraint.GetName(),
                                                clearance,
                                                actual ) );
            drcItem->SetItems( aItem, aOther );
            drcItem->SetViolatingRule( constraint.GetParentRule() );
            reportTwoShapeGeometry( drcItem, pos, aItemShape, otherShape, aLayer, actual );
            ++violations;
        }
    }

    if( testHoles )
    {
        std::shared_ptr<SHAPE_SEGMENT> itemHoleShape;
        std::shared_ptr<SHAPE_SEGMENT> otherHoleShape;
        clearance = 0;

        if( aItem->Type() == PCB_VIA_T )
        {
            LSET layers = aItem->GetLayerSet();

            if( layers.Contains( F_Cu ) )
                layers |= LSET( LSET::FrontBoardTechMask() ).set( F_CrtYd );

            if( layers.Contains( B_Cu ) )
                layers |= LSET( LSET::BackBoardTechMask() ).set( B_CrtYd );

            if( layers.Contains( F_Cu ) && layers.Contains( B_Cu ) )
                layers |= boardCopperLayers;

            wxCHECK_MSG( layers.Contains( aLayer ), violations,
                         wxT( "Bug!  Vias should only be checked for layers on which they exist" ) );

            itemHoleShape = aItem->GetEffectiveHoleShape();
        }
        else if( aItem->HasHole() )
        {
            itemHoleShape = aItem->GetEffectiveHoleShape();
        }

        if( aOther->Type() == PCB_VIA_T )
        {
            LSET layers = aOther->GetLayerSet();

            if( layers.Contains( F_Cu ) )
                layers |= LSET( LSET::FrontBoardTechMask() ).set( F_CrtYd );

            if( layers.Contains( B_Cu ) )
                layers |= LSET( LSET::BackBoardTechMask() ).set( B_CrtYd );

            if( layers.Contains( F_Cu ) && layers.Contains( B_Cu ) )
                layers |= boardCopperLayers;

            wxCHECK_MSG( layers.Contains( aLayer ), violations,
                         wxT( "Bug!  Vias should only be checked for layers on which they exist" ) );

            otherHoleShape = aOther->GetEffectiveHoleShape();
        }
        else if( aOther->HasHole() )
        {
            otherHoleShape = aOther->GetEffectiveHoleShape();
        }

        if( itemHoleShape || otherHoleShape )
        {
            constraint = m_drcEngine->EvalRules( PHYSICAL_HOLE_CLEARANCE_CONSTRAINT, aOther, aItem, aLayer );
            clearance = constraint.GetValue().Min();
        }

        if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance > 0 )
        {
            if( itemHoleShape && itemHoleShape->Collide( otherShape, clearance, &actual, &pos ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );
                drcItem->SetErrorDetail( formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                    constraint.GetName(),
                                                    clearance ,
                                                    actual ) );
                drcItem->SetItems( aItem, aOther );
                drcItem->SetViolatingRule( constraint.GetParentRule() );
                reportTwoShapeGeometry( drcItem, pos, itemHoleShape.get(), otherShape, aLayer, actual );
                ++violations;
            }

            if( otherHoleShape && otherHoleShape->Collide( aItemShape, clearance, &actual, &pos ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );
                drcItem->SetErrorDetail( formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                    constraint.GetName(),
                                                    clearance,
                                                    actual ) );
                drcItem->SetItems( aItem, aOther );
                drcItem->SetViolatingRule( constraint.GetParentRule() );
                reportTwoShapeGeometry( drcItem, pos, otherHoleShape.get(), aItemShape, aLayer, actual );
                ++violations;
            }
        }
    }

    return violations;
}


void DRC_TEST_PROVIDER_PHYSICAL_CLEARANCE::testItemAgainstZones( BOARD_ITEM* aItem,
                                                                 PCB_LAYER_ID aLayer )
{
    for( ZONE* zone : m_board->m_DRCZones )
    {
        if( !zone->GetLayerSet().test( aLayer ) )
            continue;

        BOX2I itemBBox = aItem->GetBoundingBox();
        BOX2I worstCaseBBox = itemBBox;

        worstCaseBBox.Inflate( m_board->m_DRCMaxClearance );

        if( !worstCaseBBox.Intersects( zone->GetBoundingBox() ) )
            continue;

        bool testClearance = !m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE );
        bool testHoles = !m_drcEngine->IsErrorLimitExceeded( DRCE_HOLE_CLEARANCE );

        if( !testClearance && !testHoles )
            return;

        DRC_RTREE*     zoneRTree = m_board->m_CopperZoneRTreeCache[ zone ].get();
        DRC_CONSTRAINT constraint;
        bool           colliding;
        int            clearance = -1;
        int            actual;
        VECTOR2I       pos;

        if( testClearance )
        {
            constraint = m_drcEngine->EvalRules( PHYSICAL_CLEARANCE_CONSTRAINT, aItem, zone, aLayer );
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

                    std::shared_ptr<SHAPE_SEGMENT> hole = pad->GetEffectiveHoleShape();
                    int                            size = hole->GetWidth();

                    itemShape = std::make_shared<SHAPE_SEGMENT>( hole->GetSeg(), size );
                }
            }

            if( IsCopperLayer( aLayer ) && zoneRTree )
            {
                colliding = zoneRTree->QueryColliding( itemBBox, itemShape.get(), aLayer, clearance,
                                                       &actual, &pos );
            }
            else
            {
                colliding = zone->Outline()->Collide( itemShape.get(), clearance, &actual, &pos );
            }

            if( colliding )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );
                drcItem->SetErrorDetail( formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                    constraint.GetName(),
                                                    clearance,
                                                    actual ) );
                drcItem->SetItems( aItem, zone );
                drcItem->SetViolatingRule( constraint.GetParentRule() );
                reportTwoItemGeometry( drcItem, pos, aItem, zone, aLayer, actual );
            }
        }

        if( testHoles )
        {
            std::shared_ptr<SHAPE_SEGMENT> holeShape;

            if( aItem->Type() == PCB_VIA_T )
            {
                if( aItem->GetLayerSet().Contains( aLayer ) )
                    holeShape = aItem->GetEffectiveHoleShape();
            }
            else if( aItem->HasHole() )
            {
                holeShape = aItem->GetEffectiveHoleShape();
            }

            if( holeShape )
            {
                constraint = m_drcEngine->EvalRules( PHYSICAL_HOLE_CLEARANCE_CONSTRAINT, aItem, zone, aLayer );
                clearance = constraint.GetValue().Min();

                if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance > 0 )
                {
                    if( IsCopperLayer( aLayer ) && zoneRTree )
                    {
                        colliding = zoneRTree->QueryColliding( itemBBox, holeShape.get(), aLayer,
                                                               clearance, &actual, &pos );
                    }
                    else
                    {
                        colliding = zone->Outline()->Collide( holeShape.get(), clearance, &actual, &pos );
                    }

                    if( colliding )
                    {
                        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_HOLE_CLEARANCE );
                        drcItem->SetErrorDetail( formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                            constraint.GetName(),
                                                            clearance,
                                                            actual ) );
                        drcItem->SetItems( aItem, zone );
                        drcItem->SetViolatingRule( constraint.GetParentRule() );

                        std::shared_ptr<SHAPE> zoneShape = zone->GetEffectiveShape( aLayer );
                        reportTwoShapeGeometry( drcItem, pos, holeShape.get(), zoneShape.get(), aLayer, actual );
                    }
                }
            }
        }

        if( m_drcEngine->IsCancelled() )
            return;
    }
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_PHYSICAL_CLEARANCE> dummy;
}
