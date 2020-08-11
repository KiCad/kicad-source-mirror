/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2020 KiCad Developers.
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
#include <class_board.h>
#include <class_drawsegment.h>
#include <class_pad.h>

#include <convert_basic_shapes_to_polygon.h>
#include <geometry/polygon_test_point_inside.h>

#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>

#include <drc_proto/drc_engine.h>
#include <drc_proto/drc_item.h>
#include <drc_proto/drc_rtree.h>
#include <drc_proto/drc_rule.h>
#include <drc_proto/drc_test_provider_clearance_base.h>

/*
    Copper clearance test. Checks all copper items (pads, vias, tracks, drawings, zones) for their electrical clearance.
    Errors generated:
    - DRCE_CLEARANCE
    - DRCE_TRACKS_CROSSING
    - DRCE_ZONES_INTERSECT

    TODO: improve zone clearance check (super slow)
*/

namespace test {

class DRC_TEST_PROVIDER_COPPER_CLEARANCE : public DRC_TEST_PROVIDER_CLEARANCE_BASE
{
public:
    DRC_TEST_PROVIDER_COPPER_CLEARANCE () :
        DRC_TEST_PROVIDER_CLEARANCE_BASE()
        {
        }

    virtual ~DRC_TEST_PROVIDER_COPPER_CLEARANCE()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "clearance";
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests copper item clearance";
    }

    virtual std::set<test::DRC_RULE_ID_T> GetMatchingRuleIds() const override;

private:
    void testPadClearances();
    void testTrackClearances();
    void testCopperTextAndGraphics();
    void testZones();
    void testCopperDrawItem( BOARD_ITEM* aItem );
    void doTrackDrc( TRACK* aRefSeg, TRACKS::iterator aStartIt,
                      TRACKS::iterator aEndIt, bool aTestZones );
    bool doPadToPadsDrc( D_PAD* aRefPad, D_PAD** aStart, D_PAD** aEnd, int x_limit );
};

};


bool test::DRC_TEST_PROVIDER_COPPER_CLEARANCE::Run()
{
    auto bds = m_drcEngine->GetDesignSettings();
    m_board = m_drcEngine->GetBoard();

    m_largestClearance = 0;

    for( auto rule : m_drcEngine->QueryRulesById( test::DRC_RULE_ID_T::DRC_RULE_ID_CLEARANCE ) )
    {
        drc_dbg(1, "process rule %p\n", rule );
        if( rule->GetConstraint().m_Value.HasMin() )
        {
            m_largestClearance = std::max( m_largestClearance, rule->GetConstraint().m_Value.Min() );
            drc_dbg(1, "min-copper-clearance %d\n", rule->GetConstraint().m_Value.Min() );
        }
    }

    ReportAux( "Worst clearance : %d nm", m_largestClearance );

    //m_largestClearance =

    ReportStage( ("Testing pad copper clerances"), 0, 2 );
    testPadClearances();
    ReportStage( ("Testing track/via copper clerances"), 1, 2 );
    testTrackClearances();
    ReportStage( ("Testing copper drawing/text clerances"), 1, 2 );
    testCopperTextAndGraphics();
    ReportStage( ("Testing copper zone clearances"), 1, 2 );
    testZones();

    return true;
}

void test::DRC_TEST_PROVIDER_COPPER_CLEARANCE::testCopperTextAndGraphics()
{
    // Test copper items for clearance violations with vias, tracks and pads

    for( BOARD_ITEM* brdItem : m_board->Drawings() )
    {
        if( IsCopperLayer( brdItem->GetLayer() ) )
            testCopperDrawItem( brdItem );
    }

    for( MODULE* module : m_board->Modules() )
    {
        TEXTE_MODULE& ref = module->Reference();
        TEXTE_MODULE& val = module->Value();

        if( ref.IsVisible() && IsCopperLayer( ref.GetLayer() ) )
            testCopperDrawItem( &ref );

        if( val.IsVisible() && IsCopperLayer( val.GetLayer() ) )
            testCopperDrawItem( &val );

        if( module->IsNetTie() )
            continue;

        for( BOARD_ITEM* item : module->GraphicalItems() )
        {
            if( IsCopperLayer( item->GetLayer() ) )
            {
                if( item->Type() == PCB_MODULE_TEXT_T && ( (TEXTE_MODULE*) item )->IsVisible() )
                    testCopperDrawItem( item );
                else if( item->Type() == PCB_MODULE_EDGE_T )
                    testCopperDrawItem( item );
            }
        }
    }
}


void test::DRC_TEST_PROVIDER_COPPER_CLEARANCE::testCopperDrawItem( BOARD_ITEM* aItem )
{
    EDA_RECT            bbox;
    std::shared_ptr<SHAPE> itemShape;
    DRAWSEGMENT*        drawItem = dynamic_cast<DRAWSEGMENT*>( aItem );
    EDA_TEXT*           textItem = dynamic_cast<EDA_TEXT*>( aItem );

    if( drawItem )
    {
        bbox = drawItem->GetBoundingBox();
        itemShape = drawItem->GetEffectiveShape();
    }
    else if( textItem )
    {
        bbox = textItem->GetTextBox();
        itemShape = textItem->GetEffectiveTextShape();
    }
    else
    {
        wxFAIL_MSG( "unknown item type in testCopperDrawItem()" );
        return;
    }

    SHAPE_RECT bboxShape( bbox.GetX(), bbox.GetY(), bbox.GetWidth(), bbox.GetHeight() );

    //if( itemShape->Empty() )
      //  return;

    // Test tracks and vias
    for( auto track : m_board->Tracks() )
    {
        if( !track->IsOnLayer( aItem->GetLayer() ) )
            continue;

        auto rule = m_drcEngine->EvalRulesForItems( test::DRC_RULE_ID_T::DRC_RULE_ID_CLEARANCE, aItem, track );
        auto minClearance = rule->GetConstraint().GetValue().Min();
        int     actual = INT_MAX;
        wxPoint pos;

        SHAPE_SEGMENT trackSeg( track->GetStart(), track->GetEnd(), track->GetWidth() );

        // Fast test to detect a track segment candidate inside the text bounding box
        if( !bboxShape.Collide( &trackSeg, 0 ) )
            continue;

        if( !itemShape->Collide( &trackSeg, minClearance, &actual ) )
            continue;

        pos = (wxPoint) itemShape->Centre();

        if( actual < INT_MAX )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );
            

            wxString msg;
            msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                          rule->GetName(),
                          MessageTextFromValue( userUnits(), minClearance, true ),
                          MessageTextFromValue( userUnits(), actual, true ) );

            drcItem->SetErrorMessage( msg );
            drcItem->SetItems( track, aItem );
            drcItem->SetViolatingRule( rule );

            ReportWithMarker( drcItem, pos );
        }
    }

    // Test pads
    for( auto pad : m_board->GetPads() )
    {
        if( !pad->IsOnLayer( aItem->GetLayer() ) )
            continue;

        // Graphic items are allowed to act as net-ties within their own footprint
        if( drawItem && pad->GetParent() == drawItem->GetParent() )
            continue;

        auto rule = m_drcEngine->EvalRulesForItems( test::DRC_RULE_ID_T::DRC_RULE_ID_CLEARANCE, aItem, pad );
        auto minClearance = rule->GetConstraint().GetValue().Min();

        int actual = INT_MAX;

        int bb_radius = pad->GetBoundingRadius() + minClearance;

        // Fast test to detect a pad candidate inside the text bounding box
        // Finer test (time consumming) is made only for pads near the text.
        if( !bboxShape.Collide( SEG( pad->GetPosition(), pad->GetPosition() ), bb_radius ) )
            continue;

        if( !pad->GetEffectiveShape()->Collide( itemShape.get(), minClearance, &actual ) )
            continue;

        if( actual < INT_MAX )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );


            wxString msg;

            msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                          rule->GetName(),
                          MessageTextFromValue( userUnits(), minClearance, true ),
                          MessageTextFromValue( userUnits(), actual, true ) );

            drcItem->SetErrorMessage( msg );
            drcItem->SetItems( pad, aItem );
            drcItem->SetViolatingRule( rule );

            ReportWithMarker( drcItem, pad->GetPosition() );
        }
    }
}


void test::DRC_TEST_PROVIDER_COPPER_CLEARANCE::testTrackClearances()
{
    const int         delta = 500;  // This is the number of tests between 2 calls to the
                                    // progress bar
    int               count = m_board->Tracks().size();
    int               deltamax = count/delta;

    ReportProgress(0.0);
    ReportAux("Testing %d tracks...", count );

    int ii = 0;
    count = 0;

    for( auto seg_it = m_board->Tracks().begin(); seg_it != m_board->Tracks().end(); seg_it++ )
    {
        if( ii++ > delta )
        {
            ii = 0;
            count++;

            ReportProgress( (double) ii / (double ) count );
        }

        // Test new segment against tracks and pads, optionally against copper zones
        doTrackDrc( *seg_it, seg_it + 1, m_board->Tracks().end(), false /*fixme: control for copper zones*/ );
    }
}

void test::DRC_TEST_PROVIDER_COPPER_CLEARANCE::doTrackDrc( TRACK* aRefSeg, TRACKS::iterator aStartIt,
                      TRACKS::iterator aEndIt, bool aTestZones )
{
    BOARD_DESIGN_SETTINGS&     bds = m_board->GetDesignSettings();

    SHAPE_SEGMENT refSeg( aRefSeg->GetStart(), aRefSeg->GetEnd(), aRefSeg->GetWidth() );
    PCB_LAYER_ID refLayer = aRefSeg->GetLayer();
    LSET         refLayerSet = aRefSeg->GetLayerSet();

    EDA_RECT     refSegBB = aRefSeg->GetBoundingBox();
    int          refSegWidth = aRefSeg->GetWidth();


    /******************************************/
    /* Phase 0 : via DRC tests :              */
    /******************************************/

    // fixme: via annulus and other nin-coppper clearance tests moved elsewhere

    /******************************************/
    /* Phase 1 : test DRC track to pads :     */
    /******************************************/

    // Compute the min distance to pads
    for( MODULE* mod : m_board->Modules() )
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

            // fixme: hole to hole clearance moved elsewhere

            auto rule = m_drcEngine->EvalRulesForItems( test::DRC_RULE_ID_T::DRC_RULE_ID_CLEARANCE, aRefSeg, pad );
            auto minClearance = rule->GetConstraint().GetValue().Min();
            int clearanceAllowed = minClearance - bds.GetDRCEpsilon();
            int actual;

            auto padShape = pad->GetEffectiveShape();

            if( padShape->Collide( &refSeg, minClearance - bds.GetDRCEpsilon(), &actual ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );
                wxString msg;

                msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                              /*m_clearanceSource fixme*/ "",
                              MessageTextFromValue( userUnits(), minClearance, true ),
                              MessageTextFromValue( userUnits(), actual, true ) );

                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( aRefSeg, pad );
                drcItem->SetViolatingRule( rule );

                ReportWithMarker( drcItem, pad->GetPosition() );

                if( isErrorLimitExceeded( DRCE_CLEARANCE ) )
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

        auto rule = m_drcEngine->EvalRulesForItems( test::DRC_RULE_ID_T::DRC_RULE_ID_CLEARANCE, aRefSeg, track );
        auto minClearance = rule->GetConstraint().GetValue().Min();

        SHAPE_SEGMENT trackSeg( track->GetStart(), track->GetEnd(), track->GetWidth() );
        int actual;

        if( OPT_VECTOR2I intersection = refSeg.GetSeg().Intersect( trackSeg.GetSeg() ) )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TRACKS_CROSSING );

            // fixme
            drcItem->SetErrorMessage( "FIXME" );
            drcItem->SetItems( aRefSeg, track );
            drcItem->SetViolatingRule( rule );

            ReportWithMarker( drcItem, (wxPoint) intersection.get() );

            if( isErrorLimitExceeded( DRCE_TRACKS_CROSSING ) )
                return;
        }
        else if( refSeg.Collide( &trackSeg, minClearance, &actual ) )
        {
            wxPoint   pos = getLocation( aRefSeg, trackSeg.GetSeg() );
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );


            wxString msg;
            msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                          /*m_clearanceSource fixme*/"",
                          MessageTextFromValue( userUnits(), minClearance, true ),
                          MessageTextFromValue( userUnits(), actual, true ) );

            drcItem->SetErrorMessage( msg );
            drcItem->SetItems( aRefSeg, track );
            drcItem->SetViolatingRule( rule );

            ReportWithMarker( drcItem, pos );

            if( isErrorLimitExceeded( DRCE_CLEARANCE ) )
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

        for( ZONE_CONTAINER* zone : m_board->Zones() )
        {
            if( !( refLayerSet & zone->GetLayerSet() ).any() || zone->GetIsKeepout() )
                continue;

            for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
            {
                if( zone->GetFilledPolysList( layer ).IsEmpty() )
                    continue;

                if( zone->GetNetCode() && zone->GetNetCode() == aRefSeg->GetNetCode() )
                    continue;

                // fixme: per-layer onLayer() property

                auto rule = m_drcEngine->EvalRulesForItems( test::DRC_RULE_ID_T::DRC_RULE_ID_CLEARANCE, aRefSeg, zone );
                auto minClearance = rule->GetConstraint().GetValue().Min();
                int widths       = refSegWidth / 2;

                // to avoid false positive, due to rounding issues and approxiamtions
                // in distance and clearance calculations, use a small threshold for distance
                // (1 micron)
                #define THRESHOLD_DIST Millimeter2iu( 0.001 )

                int allowedDist  = minClearance + widths + THRESHOLD_DIST;
                int actual = INT_MAX;

                if( zone->GetFilledPolysList( layer ).Collide( testSeg, allowedDist, &actual ) )
                {
                    actual = std::max( 0, actual - widths );
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );
                    wxString msg;

                    msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                                rule->GetName(),
                                MessageTextFromValue( userUnits(), minClearance, true ),
                                MessageTextFromValue( userUnits(), actual, true ) );

                    drcItem->SetErrorMessage( msg );
                    drcItem->SetItems( aRefSeg, zone );
                    drcItem->SetViolatingRule( rule );

                    ReportWithMarker( drcItem, getLocation( aRefSeg, zone ) );
                }
            }
        }
    }

// fixme: board edge clearance to another rule
}


void test::DRC_TEST_PROVIDER_COPPER_CLEARANCE::testPadClearances( )
{
    auto bds = m_drcEngine->GetDesignSettings();
    std::vector<D_PAD*>    sortedPads;

    m_board->GetSortedPadListByXthenYCoord( sortedPads );

    ReportAux("Testing %d pads...", sortedPads.size() );

    for( auto p : sortedPads )

    if( sortedPads.empty() )
        return;

    // find the max size of the pads (used to stop the pad-to-pad tests)
    int max_size = 0;

    for( D_PAD* pad : sortedPads )
    {
        // GetBoundingRadius() is the radius of the minimum sized circle fully containing the pad
        int radius = pad->GetBoundingRadius();

        if( radius > max_size )
            max_size = radius;
    }

    // Better to be fast than accurate; this keeps us from having to look up / calculate the
    // actual clearances
    max_size += m_largestClearance;

    // Upper limit of pad list (limit not included)
    D_PAD** listEnd = &sortedPads[0] + sortedPads.size();

    int ii = 0;
    // Test the pads
    for( auto& pad : sortedPads )
    {
        if( ii % 100 == 0 )
            ReportProgress( (double) ii / (double) sortedPads.size() );

        ii++;
        int x_limit = pad->GetPosition().x + pad->GetBoundingRadius() + max_size;

        doPadToPadsDrc( pad, &pad, listEnd, x_limit );
    }
}

bool test::DRC_TEST_PROVIDER_COPPER_CLEARANCE::doPadToPadsDrc( D_PAD* aRefPad, D_PAD** aStart, D_PAD** aEnd,
                          int x_limit )
{
    const static LSET all_cu = LSET::AllCuMask();

    LSET layerMask = aRefPad->GetLayerSet() & all_cu;

    for( D_PAD** pad_list = aStart;  pad_list<aEnd;  ++pad_list )
    {
        D_PAD* pad = *pad_list;

        if( pad == aRefPad )
            continue;

        // We can stop the test when pad->GetPosition().x > x_limit
        // because the list is sorted by X values
        if( pad->GetPosition().x > x_limit )
            break;

        // The pad must be in a net (i.e pt_pad->GetNet() != 0 ),
        // But no problem if pads have the same netcode (same net)
        if( pad->GetNetCode() && ( aRefPad->GetNetCode() == pad->GetNetCode() ) )
            continue;

        // if pads are from the same footprint
        if( pad->GetParent() == aRefPad->GetParent() )
        {
            // and have the same pad number ( equivalent pads  )
            if( pad->PadNameEqual( aRefPad ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_SHORTING_ITEMS );
                wxString msg;
                msg.Printf( drcItem->GetErrorText() + _( " (nets %s and %s)" ),
                            pad->GetNetCode(), aRefPad->GetNetCode() );

                drcItem->SetErrorMessage( msg );
                drcItem->SetItems( pad, aRefPad );
                drcItem->SetViolatingRule( nullptr ); // fixme: is this correct?

                ReportWithMarker( drcItem, aRefPad->GetPosition() );
                continue;
            }
        }

        // if either pad has no drill and is only on technical layers, not a clearance violation
        if( ( ( pad->GetLayerSet() & layerMask ) == 0 && !pad->GetDrillSize().x ) ||
            ( ( aRefPad->GetLayerSet() & layerMask ) == 0 && !aRefPad->GetDrillSize().x ) )
        {
            continue;
        }

        auto rule = m_drcEngine->EvalRulesForItems( test::DRC_RULE_ID_T::DRC_RULE_ID_CLEARANCE, aRefPad, pad );
        auto minClearance = rule->GetConstraint().GetValue().Min();

        drc_dbg(4, "pad %p vs %p constraint %d\n", aRefPad, pad, minClearance );

        int  clearanceAllowed = minClearance - m_drcEngine->GetDesignSettings()->GetDRCEpsilon();
        int  actual;

        auto refPadShape = aRefPad->GetEffectiveShape();

        if( refPadShape->Collide( pad->GetEffectiveShape().get(), clearanceAllowed, &actual ) )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );
            wxString msg;
            msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                          /*m_clearanceSource fixme*/ "",
                          MessageTextFromValue( userUnits(), minClearance, true ),
                          MessageTextFromValue( userUnits(), actual, true ) );

            drcItem->SetErrorMessage( msg );
            drcItem->SetItems( aRefPad, pad );
            drcItem->SetViolatingRule( rule );

            ReportWithMarker( drcItem, aRefPad->GetPosition() );
            return false;
        }
    }

    return true;
}


void test::DRC_TEST_PROVIDER_COPPER_CLEARANCE::testZones()
{
    // Test copper areas for valid netcodes -> fixme, goes to connectivity checks

    std::vector<SHAPE_POLY_SET> smoothed_polys;
    smoothed_polys.resize( m_board->GetAreaCount() );

    for( int ii = 0; ii < m_board->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = m_board->GetArea( ii );
        ZONE_CONTAINER* zoneRef = m_board->GetArea( ii );

        zoneRef->BuildSmoothedPoly( smoothed_polys[ii], zoneRef->GetLayer() );
    }

    // iterate through all areas
    for( int ia = 0; ia < m_board->GetAreaCount(); ia++ )
    {
        ZONE_CONTAINER* zoneRef = m_board->GetArea( ia );

        if( !zoneRef->IsOnCopperLayer() )
            continue;

        // If we are testing a single zone, then iterate through all other zones
        // Otherwise, we have already tested the zone combination
        for( int ia2 = ia + 1; ia2 < m_board->GetAreaCount(); ia2++ )
        {
            ZONE_CONTAINER* zoneToTest = m_board->GetArea( ia2 );

            if( zoneRef == zoneToTest )
                continue;

            // test for same layer
            if( zoneRef->GetLayer() != zoneToTest->GetLayer() )
                continue;

            // Test for same net
            if( zoneRef->GetNetCode() == zoneToTest->GetNetCode() && zoneRef->GetNetCode() >= 0 )
                continue;

            // test for different priorities
            if( zoneRef->GetPriority() != zoneToTest->GetPriority() )
                continue;

            // test for different types
            if( zoneRef->GetIsKeepout() != zoneToTest->GetIsKeepout() )
                continue;

            // Examine a candidate zone: compare zoneToTest to zoneRef

            // Get clearance used in zone to zone test.
            auto rule = m_drcEngine->EvalRulesForItems( test::DRC_RULE_ID_T::DRC_RULE_ID_CLEARANCE, zoneRef, zoneToTest );
            auto zone2zoneClearance = rule->GetConstraint().GetValue().Min();

            // Keepout areas have no clearance, so set zone2zoneClearance to 1
            // ( zone2zoneClearance = 0  can create problems in test functions)
            if( zoneRef->GetIsKeepout() ) // fixme: really?
                zone2zoneClearance = 1;

            // test for some corners of zoneRef inside zoneToTest
            for( auto iterator = smoothed_polys[ia].IterateWithHoles(); iterator; iterator++ )
            {
                VECTOR2I currentVertex = *iterator;
                wxPoint pt( currentVertex.x, currentVertex.y );

                if( smoothed_polys[ia2].Contains( currentVertex ) )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_ZONES_INTERSECT );
                    drcItem->SetItems( zoneRef, zoneToTest );
                    drcItem->SetViolatingRule( rule );

                    ReportWithMarker( drcItem, pt );
                }
            }

            // test for some corners of zoneToTest inside zoneRef
            for( auto iterator = smoothed_polys[ia2].IterateWithHoles(); iterator; iterator++ )
            {
                VECTOR2I currentVertex = *iterator;
                wxPoint pt( currentVertex.x, currentVertex.y );

                if( smoothed_polys[ia].Contains( currentVertex ) )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_ZONES_INTERSECT );
                    drcItem->SetItems( zoneToTest, zoneRef );
                    drcItem->SetViolatingRule( rule );

                    ReportWithMarker( drcItem, pt );
                }
            }

            // Iterate through all the segments of refSmoothedPoly
            std::map<wxPoint, int> conflictPoints;

            for( auto refIt = smoothed_polys[ia].IterateSegmentsWithHoles(); refIt; refIt++ )
            {
                // Build ref segment
                SEG refSegment = *refIt;

                // Iterate through all the segments in smoothed_polys[ia2]
                for( auto testIt = smoothed_polys[ia2].IterateSegmentsWithHoles(); testIt; testIt++ )
                {
                    // Build test segment
                    SEG testSegment = *testIt;
                    wxPoint pt;

                    int ax1, ay1, ax2, ay2;
                    ax1 = refSegment.A.x;
                    ay1 = refSegment.A.y;
                    ax2 = refSegment.B.x;
                    ay2 = refSegment.B.y;

                    int bx1, by1, bx2, by2;
                    bx1 = testSegment.A.x;
                    by1 = testSegment.A.y;
                    bx2 = testSegment.B.x;
                    by2 = testSegment.B.y;

                    int d = GetClearanceBetweenSegments( bx1, by1, bx2, by2,
                                                         0,
                                                         ax1, ay1, ax2, ay2,
                                                         0,
                                                         zone2zoneClearance,
                                                         &pt.x, &pt.y );

                    if( d < zone2zoneClearance )
                    {
                        if( conflictPoints.count( pt ) )
                            conflictPoints[ pt ] = std::min( conflictPoints[ pt ], d );
                        else
                            conflictPoints[ pt ] = d;
                    }
                }
            }

            for( const std::pair<const wxPoint, int>& conflict : conflictPoints )
            {
                int       actual = conflict.second;
                std::shared_ptr<DRC_ITEM> drcItem;

                if( actual <= 0 )
                {
                    drcItem = DRC_ITEM::Create( DRCE_ZONES_INTERSECT );
                }
                else
                {
                    drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );
                    wxString msg;

                    msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                                  /* fixme */"",
                                  MessageTextFromValue( userUnits(), zone2zoneClearance, true ),
                                  MessageTextFromValue( userUnits(), conflict.second, true ) );

                    drcItem->SetErrorMessage( msg );

                }

                drcItem->SetViolatingRule( rule );
                drcItem->SetItems( zoneRef, zoneToTest );

                ReportWithMarker( drcItem, conflict.first );
            }
        }
    }
}


std::set<test::DRC_RULE_ID_T> test::DRC_TEST_PROVIDER_COPPER_CLEARANCE::GetMatchingRuleIds() const
{
    return { DRC_RULE_ID_T::DRC_RULE_ID_CLEARANCE };
}


namespace detail
{
    static test::DRC_REGISTER_TEST_PROVIDER<test::DRC_TEST_PROVIDER_COPPER_CLEARANCE> dummy;
}
