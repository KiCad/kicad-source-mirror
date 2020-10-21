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
#include <pcb_shape.h>
#include <class_pad.h>
#include <class_track.h>

#include <geometry/shape_arc.h>
#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>

#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider_clearance_base.h>
#include <class_dimension.h>

/*
    Copper clearance test. Checks all copper items (pads, vias, tracks, drawings, zones) for their electrical clearance.
    Errors generated:
    - DRCE_CLEARANCE
    - DRCE_TRACKS_CROSSING
    - DRCE_ZONES_INTERSECT
    - DRCE_SHORTING_ITEMS

    TODO: improve zone clearance check (super slow)
*/

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

    virtual std::set<DRC_CONSTRAINT_TYPE_T> GetConstraintTypes() const override;

    int GetNumPhases() const override;

private:
    void testPadClearances();

    void testTrackClearances();

    void testCopperTextAndGraphics();

    void testZones();

    void testCopperDrawItem( BOARD_ITEM* aItem );

    void doTrackDrc( TRACK* aRefSeg, PCB_LAYER_ID aLayer, TRACKS::iterator aStartIt,
                     TRACKS::iterator aEndIt );

    /**
     * Test clearance of a pad hole with the pad hole of other pads.
     * @param aSortedPadsList is the sorted by X pos of all pads
     * @param aRefPadIdx is the index of pad to test inside aSortedPadsList
     * @param aX_limit is the max X pos of others pads that need to be tested
     * To speed up the test, aSortedPadsList is a pad list sorted by X position,
     * and only pads after the pad to test are tested, so this function must be called
     * for each pad for the first in list to the last in list
     */
    void doPadToPadsDrc( int aRefPadIdx, std::vector<D_PAD*>& aSortedPadsList, int aX_limit );
};


bool DRC_TEST_PROVIDER_COPPER_CLEARANCE::Run()
{
    m_board = m_drcEngine->GetBoard();
    DRC_CONSTRAINT worstClearanceConstraint;

    if( m_drcEngine->QueryWorstConstraint( DRC_CONSTRAINT_TYPE_CLEARANCE,
                                           worstClearanceConstraint, DRCCQ_LARGEST_MINIMUM ) )
    {
        m_largestClearance = worstClearanceConstraint.GetValue().Min();
    }
    else
    {
        reportAux( "No Clearance constraints found..." );
        return false;
    }

    reportAux( "Worst clearance : %d nm", m_largestClearance );

    if( !reportPhase( _( "Checking pad clearances..." ) ) )
        return false;

    testPadClearances();

    if( !reportPhase( _( "Checking track & via clearances..." ) ) )
        return false;

    testTrackClearances();

    if( !reportPhase( _( "Checking copper graphic & text clearances..." ) ) )
        return false;

    testCopperTextAndGraphics();

    if( !reportPhase( _( "Checking copper zone clearances..." ) ) )
        return false;

    testZones();

    reportRuleStatistics();

    return true;
}

void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testCopperTextAndGraphics()
{
    // Test copper items for clearance violations with vias, tracks and pads

    for( BOARD_ITEM* brdItem : m_board->Drawings() )
    {
        if( IsCopperLayer( brdItem->GetLayer() ) )
            testCopperDrawItem( brdItem );
    }

    for( MODULE* module : m_board->Modules() )
    {
        FP_TEXT& ref = module->Reference();
        FP_TEXT& val = module->Value();

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
                if( item->Type() == PCB_FP_TEXT_T && ( (FP_TEXT*) item )->IsVisible() )
                    testCopperDrawItem( item );
                else if( item->Type() == PCB_FP_SHAPE_T )
                    testCopperDrawItem( item );
            }
        }
    }
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testCopperDrawItem( BOARD_ITEM* aItem )
{
    EDA_RECT               bbox;
    std::shared_ptr<SHAPE> itemShape;
    EDA_TEXT*              textItem = dynamic_cast<EDA_TEXT*>( aItem );
    PCB_LAYER_ID           layer = aItem->GetLayer();
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    if( textItem )
    {
        bbox = textItem->GetTextBox();
        itemShape = textItem->GetEffectiveTextShape();
    }
    else
    {
        bbox = aItem->GetBoundingBox();
        itemShape = aItem->GetEffectiveShape( layer );
    }

    SHAPE_RECT bboxShape( bbox.GetX(), bbox.GetY(), bbox.GetWidth(), bbox.GetHeight() );

    // Test tracks and vias
    for( TRACK* track : m_board->Tracks() )
    {
        if( !track->IsOnLayer( aItem->GetLayer() ) )
            continue;

        SHAPE_SEGMENT trackSeg( track->GetStart(), track->GetEnd(), track->GetWidth() );

        // Fast test to detect a track segment candidate inside the text bounding box
        if( !bboxShape.Collide( &trackSeg, m_largestClearance ) )
            continue;

        auto     constraint = m_drcEngine->EvalRulesForItems( DRC_CONSTRAINT_TYPE_CLEARANCE,
                                                              aItem, track, layer );
        int      minClearance = constraint.GetValue().Min();
        int      actual = INT_MAX;
        VECTOR2I pos;

        accountCheck( constraint );

        if( !itemShape->Collide( &trackSeg, minClearance, &actual, &pos ) )
            continue;

        if( actual < INT_MAX )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );

            m_msg.Printf( drcItem->GetErrorText() + wxS( " " ) + _( "(%s clearance %s; actual %s)" ),
                          constraint.GetName(),
                          MessageTextFromValue( userUnits(), minClearance ),
                          MessageTextFromValue( userUnits(), std::max( 0, actual ) ) );

            drcItem->SetErrorMessage( m_msg );
            drcItem->SetItems( track, aItem );
            drcItem->SetViolatingRule( constraint.GetParentRule() );

            reportViolation( drcItem, (wxPoint) pos );
        }
    }

    // Test pads
    for( D_PAD* pad : m_board->GetPads() )
    {
        if( !pad->IsOnLayer( layer ) )
            continue;

        // Graphic items are allowed to act as net-ties within their own footprint
        if( aItem->Type() == PCB_FP_SHAPE_T && pad->GetParent() == aItem->GetParent() )
            continue;

        // Fast test to detect a pad candidate inside the text bounding box
        // Finer test (time consuming) is made only for pads near the text.
        int bb_radius = pad->GetBoundingRadius() + m_largestClearance;

        if( !bboxShape.Collide( SEG( pad->GetPosition(), pad->GetPosition() ), bb_radius ) )
            continue;

        auto     constraint = m_drcEngine->EvalRulesForItems( DRC_CONSTRAINT_TYPE_CLEARANCE,
                                                              aItem, pad, layer );
        int      minClearance = constraint.GetValue().Min();
        int      actual;
        VECTOR2I pos;

        accountCheck( constraint );

        SHAPE_SEGMENT padCylinder;
        const SHAPE* padShape;

        if( pad->FlashLayer( layer ) )
        {
            padShape = pad->GetEffectiveShape().get();
        }
        else if( pad->GetAttribute() == PAD_ATTRIB_PTH )
        {
            // Note: drill size represents finish size, which means the actual holes size is the
            // plating thickness larger.
            padCylinder = *pad->GetEffectiveHoleShape();
            padCylinder.SetWidth( padCylinder.GetWidth() + bds.GetHolePlatingThickness() );
            padShape = &padCylinder;
        }
        else
        {
            continue;
        }

        if( !itemShape->Collide( padShape, minClearance, &actual, &pos ) )
            continue;

        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );

        m_msg.Printf( drcItem->GetErrorText() + wxS( " " ) + _( "(%s clearance %s; actual %s)" ),
                      constraint.GetName(),
                      MessageTextFromValue( userUnits(), minClearance ),
                      MessageTextFromValue( userUnits(), actual ) );

        drcItem->SetErrorMessage( m_msg );
        drcItem->SetItems( pad, aItem );
        drcItem->SetViolatingRule( constraint.GetParentRule() );

        reportViolation( drcItem, (wxPoint) pos );
    }
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testTrackClearances()
{
    // This is the number of tests between 2 calls to the progress bar
    const int delta = 25;
    int       count = m_board->Tracks().size();

    reportAux( "Testing %d tracks...", count );

    int ii = 0;

    for( auto seg_it = m_board->Tracks().begin(); seg_it != m_board->Tracks().end(); seg_it++ )
    {
        if( !reportProgress( ii++, m_board->Tracks().size(), delta ) )
            break;

        // Test segment against tracks and pads, optionally against copper zones
        for( PCB_LAYER_ID layer : (*seg_it)->GetLayerSet().Seq() )
        {
            doTrackDrc( *seg_it, layer, seg_it + 1, m_board->Tracks().end() );
        }
    }
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::doTrackDrc( TRACK* aRefSeg, PCB_LAYER_ID aLayer,
                                                     TRACKS::iterator aStartIt,
                                                     TRACKS::iterator aEndIt )
{
    BOARD_DESIGN_SETTINGS&  bds = m_board->GetDesignSettings();

    SHAPE_SEGMENT refSeg( aRefSeg->GetStart(), aRefSeg->GetEnd(), aRefSeg->GetWidth() );
    EDA_RECT      refSegInflatedBB = aRefSeg->GetBoundingBox();
    int           refSegWidth = aRefSeg->GetWidth();

    refSegInflatedBB.Inflate( m_largestClearance );

    /******************************************/
    /* Phase 1 : test DRC track to pads :     */
    /******************************************/

    // Compute the min distance to pads
    for( MODULE* mod : m_board->Modules() )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE ) )
            break;

        // Don't preflight at the module level.  Getting a module's bounding box goes
        // through all its pads anyway (so it's no faster), and also all its drawings
        // (so it's in fact slower).

        for( D_PAD* pad : mod->Pads() )
        {
            if( m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE ) )
                break;

            // Preflight based on bounding boxes.
            if( !refSegInflatedBB.Intersects( pad->GetBoundingBox() ) )
                continue;

            // No need to check pads with the same net as the refSeg.
            if( pad->GetNetCode() && aRefSeg->GetNetCode() == pad->GetNetCode() )
                continue;

            SHAPE_SEGMENT padCylinder;
            const SHAPE* padShape;

            if( pad->FlashLayer( aLayer ) )
            {
                padShape = pad->GetEffectiveShape().get();
            }
            else if( pad->GetAttribute() == PAD_ATTRIB_PTH )
            {
                // Note: drill size represents finish size, which means the actual holes size is the
                // plating thickness larger.
                padCylinder = *pad->GetEffectiveHoleShape();
                padCylinder.SetWidth( padCylinder.GetWidth() + bds.GetHolePlatingThickness() );
                padShape = &padCylinder;
            }
            else
            {
                continue;
            }

            auto     constraint = m_drcEngine->EvalRulesForItems( DRC_CONSTRAINT_TYPE_CLEARANCE,
                                                                  aRefSeg, pad, aLayer );
            int      minClearance = constraint.GetValue().Min();
            int      actual;
            VECTOR2I pos;

            accountCheck( constraint );

            if( padShape->Collide( &refSeg, minClearance - bds.GetDRCEpsilon(), &actual, &pos ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );

                m_msg.Printf( drcItem->GetErrorText() + wxS( " " ) + _( "(%s clearance %s; actual %s)" ),
                              constraint.GetName(),
                              MessageTextFromValue( userUnits(), minClearance ),
                              MessageTextFromValue( userUnits(), actual ) );

                drcItem->SetErrorMessage( m_msg );
                drcItem->SetItems( aRefSeg, pad );
                drcItem->SetViolatingRule( constraint.GetParentRule() );

                reportViolation( drcItem, (wxPoint) pos );
            }
        }
    }

    /***********************************************/
    /* Phase 2: test DRC with other track segments */
    /***********************************************/

    // Test the reference segment with other track segments
    for( auto it = aStartIt; it != aEndIt; it++ )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE ) )
            break;

        TRACK* track = *it;

        if( track->Type() == PCB_VIA_T )
        {
            if( !track->GetLayerSet().test( aLayer ) )
                continue;
        }
        else
        {
            if( track->GetLayer() != aLayer )
                continue;
        }

        // No problem if segments have the same net code:
        if( aRefSeg->GetNetCode() == track->GetNetCode() )
            continue;

        // Preflight based on worst-case inflated bounding boxes:
        if( !refSegInflatedBB.Intersects( track->GetBoundingBox() ) )
            continue;

        auto          constraint = m_drcEngine->EvalRulesForItems( DRC_CONSTRAINT_TYPE_CLEARANCE,
                                                                   aRefSeg, track, aLayer );
        int           minClearance = constraint.GetValue().Min();
        int           actual;
        SHAPE_SEGMENT trackSeg( track->GetStart(), track->GetEnd(), track->GetWidth() );
        VECTOR2I      pos;

        accountCheck( constraint );

        /// Check to see if the via has a pad on this layer
        if( track->Type() == PCB_VIA_T )
        {
            VIA* via = static_cast<VIA*>( track );

            if( !via->FlashLayer( aLayer ) )
                trackSeg.SetWidth( via->GetDrillValue() );
        }

        // Check two tracks crossing first as it reports a DRCE without distances
        if( OPT_VECTOR2I intersection = refSeg.GetSeg().Intersect( trackSeg.GetSeg() ) )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TRACKS_CROSSING );
            drcItem->SetItems( aRefSeg, track );
            drcItem->SetViolatingRule( constraint.GetParentRule() );

            reportViolation( drcItem, (wxPoint) intersection.get() );
        }
        else if( refSeg.Collide( &trackSeg, minClearance - bds.GetDRCEpsilon(), &actual, &pos ) )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );

            m_msg.Printf( drcItem->GetErrorText() + wxS( " " ) + _( "(%s clearance %s; actual %s)" ),
                          constraint.GetName(),
                          MessageTextFromValue( userUnits(), minClearance ),
                          MessageTextFromValue( userUnits(), actual ) );

            drcItem->SetErrorMessage( m_msg );
            drcItem->SetItems( aRefSeg, track );
            drcItem->SetViolatingRule( constraint.GetParentRule() );

            reportViolation( drcItem, (wxPoint) pos );

            if( !m_drcEngine->GetReportAllTrackErrors() )
                break;
        }
    }

    /***************************************/
    /* Phase 3: test DRC with copper zones */
    /***************************************/
    // Can be *very* time consuming.

    if( m_drcEngine->GetTestTracksAgainstZones()
            && ( aRefSeg->Type() != PCB_VIA_T
                    || static_cast<VIA*>( aRefSeg )->FlashLayer( aLayer )
                    || static_cast<VIA*>( aRefSeg )->GetDrill() > 0 ) )
    {
        for( ZONE_CONTAINER* zone : m_board->Zones() )
        {
            if( m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE ) )
                break;

            if( !zone->GetLayerSet().test( aLayer ) || zone->GetIsRuleArea() )
                continue;

            if( zone->GetNetCode() && zone->GetNetCode() == aRefSeg->GetNetCode() )
                continue;

            if( zone->GetFilledPolysList( aLayer ).IsEmpty() )
                continue;

            if( !refSegInflatedBB.Intersects( zone->GetCachedBoundingBox() ) )
                continue;

            int halfWidth = refSegWidth / 2;

            if( aRefSeg->Type() == PCB_VIA_T )
            {
                VIA* refVia = static_cast<VIA*>( aRefSeg );

                if( !refVia->FlashLayer( aLayer ) )
                    halfWidth = refVia->GetDrill() / 2 + bds.GetHolePlatingThickness();
            }

            auto constraint = m_drcEngine->EvalRulesForItems( DRC_CONSTRAINT_TYPE_CLEARANCE,
                                                              aRefSeg, zone, aLayer );
            int minClearance = constraint.GetValue().Min();
            int allowedDist  = minClearance + halfWidth - bds.GetDRCEpsilon();

            const SHAPE_POLY_SET& zonePoly = zone->GetFilledPolysList( aLayer );
            int                   actual = INT_MAX;
            VECTOR2I              location;

            accountCheck( constraint );

            if( aRefSeg->Type() == PCB_ARC_T )
            {
                std::shared_ptr<SHAPE> refShape = aRefSeg->GetEffectiveShape();
                SHAPE_ARC*             refArc = dynamic_cast<SHAPE_ARC*>( refShape.get() );
                SHAPE_LINE_CHAIN       refArcSegs = refArc->ConvertToPolyline( bds.m_MaxError );

                for( int i = 0; i < refArcSegs.SegmentCount(); ++i )
                {
                    SEG      refArcSeg = refArcSegs.Segment( i );
                    int      segActual;
                    VECTOR2I segLocation;

                    if( zonePoly.Collide( refArcSeg, allowedDist, &segActual, &segLocation ) )
                    {
                        if( segActual < actual )
                        {
                            actual = segActual;
                            location = segLocation;
                        }
                    }
                }
            }
            else
            {
                SEG testSeg( aRefSeg->GetStart(), aRefSeg->GetEnd() );

                zonePoly.Collide( testSeg, allowedDist, &actual, &location );
            }

            if( actual != INT_MAX )
            {
                actual = std::max( 0, actual - halfWidth );
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );

                m_msg.Printf( drcItem->GetErrorText() + wxS( " " ) + _( "(%s clearance %s; actual %s)" ),
                              constraint.GetName(),
                              MessageTextFromValue( userUnits(), minClearance ),
                              MessageTextFromValue( userUnits(), actual ) );

                drcItem->SetErrorMessage( m_msg );
                drcItem->SetItems( aRefSeg, zone );
                drcItem->SetViolatingRule( constraint.GetParentRule() );

                reportViolation( drcItem, (wxPoint) location );
            }
        }
    }
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testPadClearances( )
{
    const int delta = 25;  // This is the number of tests between 2 calls to the progress bar
    std::vector<D_PAD*> sortedPads;

    m_board->GetSortedPadListByXthenYCoord( sortedPads );

    reportAux( "Testing %d pads...", sortedPads.size());

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

    // Test the pads
    for( int idx = 0; idx < (int) sortedPads.size(); idx++ )
    {
        D_PAD* pad = sortedPads[idx];

        if( !reportProgress( idx, sortedPads.size(), delta ) )
            break;

        int x_limit = pad->GetPosition().x + pad->GetBoundingRadius() + max_size;

        doPadToPadsDrc( idx, sortedPads, x_limit );
    }
}

void DRC_TEST_PROVIDER_COPPER_CLEARANCE::doPadToPadsDrc( int aRefPadIdx,
                                                         std::vector<D_PAD*>& aSortedPadsList,
                                                         int aX_limit )
{
    const static LSET all_cu = LSET::AllCuMask();
    const BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    D_PAD*   refPad = aSortedPadsList[aRefPadIdx];
    LSET     layerMask = refPad->GetLayerSet() & all_cu;
    EDA_RECT refPadInflatedBB = refPad->GetBoundingBox();

    refPadInflatedBB.Inflate( m_largestClearance );

    for( int idx = aRefPadIdx; idx < (int)aSortedPadsList.size();  ++idx )
    {
        bool exceedClearance = m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE );
        bool exceedShorting = m_drcEngine->IsErrorLimitExceeded( DRCE_CLEARANCE );

        if( exceedClearance && exceedShorting )
            return;

        D_PAD* pad = aSortedPadsList[idx];

        if( pad == refPad )
            continue;

        // We can stop the test when pad->GetPosition().x > aX_limit
        // because the list is sorted by X poditions, and other pads are too far.
        if( pad->GetPosition().x > aX_limit )
            break;

        // The pad must be in a net (i.e pt_pad->GetNet() != 0 ),
        // But no problem if pads have the same netcode (same net)
        if( pad->GetNetCode() && ( refPad->GetNetCode() == pad->GetNetCode() ) )
            continue;

        // If pads are equivalent (ie: from the same footprint with the same pad number)...
        if( pad->SameLogicalPadAs( refPad ) )
        {
            // ...and have nets, then they must be the same net
            if( pad->GetNetCode() && refPad->GetNetCode()
                    && pad->GetNetCode() != refPad->GetNetCode()
                    && !exceedShorting )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_SHORTING_ITEMS );

                m_msg.Printf( drcItem->GetErrorText() + wxS( " " ) + _( "(nets %s and %s)" ),
                              pad->GetNetname(), refPad->GetNetname() );

                drcItem->SetErrorMessage( m_msg );
                drcItem->SetItems( pad, refPad );

                reportViolation( drcItem, refPad->GetPosition());
            }

            continue;
        }

        // if either pad has no drill and is only on technical layers, not a clearance violation
        if( ( ( pad->GetLayerSet() & layerMask ) == 0 && !pad->GetDrillSize().x ) ||
            ( ( refPad->GetLayerSet() & layerMask ) == 0 && !refPad->GetDrillSize().x ) )
        {
            continue;
        }

        if( !refPadInflatedBB.Intersects( pad->GetBoundingBox() ) )
            continue;

        for( PCB_LAYER_ID layer : refPad->GetLayerSet().Seq() )
        {
            if( exceedClearance )
                break;

            auto     constraint = m_drcEngine->EvalRulesForItems( DRC_CONSTRAINT_TYPE_CLEARANCE,
                                                                  refPad, pad, layer );
            int      minClearance = constraint.GetValue().Min();
            int      clearanceAllowed = minClearance - bds.GetDRCEpsilon();
            int      actual;
            VECTOR2I pos;

            accountCheck( constraint );

            SHAPE_SEGMENT refPadCylinder;
            const SHAPE*  refPadShape;

            if( refPad->FlashLayer( layer ) )
            {
                refPadShape = refPad->GetEffectiveShape().get();
            }
            else if( refPad->GetAttribute() == PAD_ATTRIB_PTH )
            {
                // Note: drill size represents finish size, which means the actual holes size is the
                // plating thickness larger.
                refPadCylinder = *pad->GetEffectiveHoleShape();
                refPadCylinder.SetWidth( refPadCylinder.GetWidth() + bds.GetHolePlatingThickness() );
                refPadShape = &refPadCylinder;
            }
            else
            {
                continue;
            }

            SHAPE_SEGMENT padCylinder;
            const SHAPE*  padShape;

            if( pad->FlashLayer( layer ) )
            {
                padShape = pad->GetEffectiveShape().get();
            }
            else if( pad->GetAttribute() == PAD_ATTRIB_PTH )
            {
                // Note: drill size represents finish size, which means the actual holes size is the
                // plating thickness larger.
                padCylinder = *pad->GetEffectiveHoleShape();
                padCylinder.SetWidth( padCylinder.GetWidth() + bds.GetHolePlatingThickness() );
                padShape = &padCylinder;
            }
            else
            {
                continue;
            }

            if( refPadShape->Collide( padShape, clearanceAllowed, &actual, &pos ) )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CLEARANCE );

                m_msg.Printf( drcItem->GetErrorText() + wxS( " " ) + _( "(%s clearance %s; actual %s)" ),
                              constraint.GetName(),
                              MessageTextFromValue( userUnits(), minClearance ),
                              MessageTextFromValue( userUnits(), actual ) );

                drcItem->SetErrorMessage( m_msg );
                drcItem->SetItems( refPad, pad );
                drcItem->SetViolatingRule( constraint.GetParentRule() );

                reportViolation( drcItem, (wxPoint) pos );
                break;
            }
        }
    }
}


void DRC_TEST_PROVIDER_COPPER_CLEARANCE::testZones()
{
    const int delta = 50;  // This is the number of tests between 2 calls to the progress bar

    SHAPE_POLY_SET  buffer;
    SHAPE_POLY_SET* boardOutline = nullptr;

    if( m_board->GetBoardPolygonOutlines( buffer ) )
        boardOutline = &buffer;

    // Test copper areas for valid netcodes -> fixme, goes to connectivity checks

    for( int layer_id = F_Cu; layer_id <= B_Cu; ++layer_id )
    {
        PCB_LAYER_ID layer = static_cast<PCB_LAYER_ID>( layer_id );
        std::vector<SHAPE_POLY_SET> smoothed_polys;
        smoothed_polys.resize( m_board->GetAreaCount() );

        // Skip over layers not used on the current board
        if( !m_board->IsLayerEnabled( layer ) )
            continue;

        for( int ii = 0; ii < m_board->GetAreaCount(); ii++ )
        {
            ZONE_CONTAINER* zoneRef = m_board->GetArea( ii );

            if( zoneRef->IsOnLayer( layer ) )
                zoneRef->BuildSmoothedPoly( smoothed_polys[ii], layer, boardOutline );
        }

        // iterate through all areas
        for( int ia = 0; ia < m_board->GetAreaCount(); ia++ )
        {
            if( !reportProgress( ia, m_board->GetAreaCount(), delta ) )
                break;

            ZONE_CONTAINER* zoneRef = m_board->GetArea( ia );

            if( !zoneRef->IsOnLayer( layer ) )
                continue;

            // If we are testing a single zone, then iterate through all other zones
            // Otherwise, we have already tested the zone combination
            for( int ia2 = ia + 1; ia2 < m_board->GetAreaCount(); ia2++ )
            {
                ZONE_CONTAINER* zoneToTest = m_board->GetArea( ia2 );

                if( zoneRef == zoneToTest )
                    continue;

                // test for same layer
                if( !zoneToTest->IsOnLayer( layer ) )
                    continue;

                // Test for same net
                if( zoneRef->GetNetCode() == zoneToTest->GetNetCode() && zoneRef->GetNetCode() >= 0 )
                    continue;

                // test for different priorities
                if( zoneRef->GetPriority() != zoneToTest->GetPriority() )
                    continue;

                // test for different types
                if( zoneRef->GetIsRuleArea() != zoneToTest->GetIsRuleArea() )
                    continue;

                // Examine a candidate zone: compare zoneToTest to zoneRef

                // Get clearance used in zone to zone test.
                auto constraint = m_drcEngine->EvalRulesForItems( DRC_CONSTRAINT_TYPE_CLEARANCE,
                                                                  zoneRef, zoneToTest );
                int  zone2zoneClearance = constraint.GetValue().Min();

                accountCheck( constraint );

                // Keepout areas have no clearance, so set zone2zoneClearance to 1
                // ( zone2zoneClearance = 0  can create problems in test functions)
                if( zoneRef->GetIsRuleArea() ) // fixme: really?
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
                        drcItem->SetViolatingRule( constraint.GetParentRule() );

                        reportViolation( drcItem, pt );
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
                        drcItem->SetViolatingRule( constraint.GetParentRule() );

                        reportViolation( drcItem, pt );
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

                        m_msg.Printf( drcItem->GetErrorText() + wxS( " " ) + _( "(%s clearance %s; actual %s)" ),
                                      constraint.GetName(),
                                      MessageTextFromValue( userUnits(), zone2zoneClearance ),
                                      MessageTextFromValue( userUnits(), conflict.second ) );

                        drcItem->SetErrorMessage( m_msg );
                    }

                    drcItem->SetItems( zoneRef, zoneToTest );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drcItem, conflict.first );
                }
            }
        }
    }
}


int DRC_TEST_PROVIDER_COPPER_CLEARANCE::GetNumPhases() const
{
    return 4;
}


std::set<DRC_CONSTRAINT_TYPE_T> DRC_TEST_PROVIDER_COPPER_CLEARANCE::GetConstraintTypes() const
{
    return { DRC_CONSTRAINT_TYPE_CLEARANCE };
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_COPPER_CLEARANCE> dummy;
}
