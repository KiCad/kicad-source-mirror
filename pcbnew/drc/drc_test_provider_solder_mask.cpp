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
#include <board_design_settings.h>
#include <board_connected_item.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_text.h>
#include <zone.h>
#include <geometry/seg.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider_clearance_base.h>
#include <drc/drc_rtree.h>

/*
    Solder mask tests. Checks for silkscreen which is clipped by mask openings and for bridges
    between mask apertures with different nets.
    Errors generated:
    - DRCE_SILK_CLEARANCE
    - DRCE_SOLDERMASK_BRIDGE
*/

class DRC_TEST_PROVIDER_SOLDER_MASK : public ::DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_SOLDER_MASK ():
            m_board( nullptr ),
            m_webWidth( 0 ),
            m_maxError( 0 ),
            m_largestClearance( 0 )
    {
        m_bridgeRule.m_Name = _( "board setup solder mask min width" );
    }

    virtual ~DRC_TEST_PROVIDER_SOLDER_MASK()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return wxT( "solder_mask_issues" );
    };

    virtual const wxString GetDescription() const override
    {
        return wxT( "Tests for silkscreen being clipped by solder mask and copper being exposed "
                    "by mask apertures of other nets" );
    }

private:
    void addItemToRTrees( BOARD_ITEM* aItem );
    void buildRTrees();

    void testSilkToMaskClearance();
    void testMaskBridges();

    void testItemAgainstItems( BOARD_ITEM* aItem, const BOX2I& aItemBBox,
                               PCB_LAYER_ID aRefLayer, PCB_LAYER_ID aTargetLayer );
    void testMaskItemAgainstZones( BOARD_ITEM* item, const BOX2I& itemBBox,
                                   PCB_LAYER_ID refLayer, PCB_LAYER_ID targetLayer );

    bool checkMaskAperture( BOARD_ITEM* aMaskItem, BOARD_ITEM* aTestItem, PCB_LAYER_ID aTestLayer,
                            int aTestNet, BOARD_ITEM** aCollidingItem );

    bool checkItemMask( BOARD_ITEM* aMaskItem, int aTestNet );

private:
    DRC_RULE m_bridgeRule;

    BOARD*   m_board;
    int      m_webWidth;
    int      m_maxError;
    int      m_largestClearance;

    std::unique_ptr<DRC_RTREE> m_fullSolderMaskRTree;
    std::unique_ptr<DRC_RTREE> m_itemTree;

    std::unordered_map<PTR_PTR_CACHE_KEY, LSET> m_checkedPairs;

    // Shapes used to define solder mask apertures don't have nets, so we assign them the
    // first object+net that bridges their aperture (after which any other nets will generate
    // violations).
    std::unordered_map<PTR_LAYER_CACHE_KEY, std::pair<BOARD_ITEM*, int>> m_maskApertureNetMap;
};


void DRC_TEST_PROVIDER_SOLDER_MASK::addItemToRTrees( BOARD_ITEM* aItem )
{
    ZONE* solderMask = m_board->m_SolderMask;

    if( aItem->Type() == PCB_ZONE_T )
    {
        ZONE* zone = static_cast<ZONE*>( aItem );

        for( PCB_LAYER_ID layer : { F_Mask, B_Mask } )
        {
            if( zone->IsOnLayer( layer ) )
            {
                solderMask->GetFill( layer )->BooleanAdd( *zone->GetFilledPolysList( layer ),
                                                          SHAPE_POLY_SET::PM_FAST );
            }
        }
    }
    else if( aItem->Type() == PCB_PAD_T )
    {
        for( PCB_LAYER_ID layer : { F_Mask, B_Mask } )
        {
            if( aItem->IsOnLayer( layer ) )
            {
                PAD* pad = static_cast<PAD*>( aItem );
                int  clearance = ( m_webWidth / 2 ) + pad->GetSolderMaskExpansion();

                aItem->TransformShapeToPolygon( *solderMask->GetFill( layer ), layer, clearance,
                                                m_maxError, ERROR_OUTSIDE );

                m_itemTree->Insert( aItem, layer, m_largestClearance );
            }
        }
    }
    else if( aItem->Type() == PCB_VIA_T )
    {
        for( PCB_LAYER_ID layer : { F_Mask, B_Mask } )
        {
            if( aItem->IsOnLayer( layer ) )
            {
                PCB_VIA* via = static_cast<PCB_VIA*>( aItem );
                int      clearance = ( m_webWidth / 2 ) + via->GetSolderMaskExpansion();

                via->TransformShapeToPolygon( *solderMask->GetFill( layer ), layer, clearance,
                                              m_maxError, ERROR_OUTSIDE );

                m_itemTree->Insert( aItem, layer, m_largestClearance );
            }
        }
    }
    else if( aItem->Type() == PCB_TEXT_T )
    {
        for( PCB_LAYER_ID layer : { F_Mask, B_Mask } )
        {
            if( aItem->IsOnLayer( layer ) )
            {
                const PCB_TEXT* text = static_cast<const PCB_TEXT*>( aItem );

                text->TransformTextToPolySet( *solderMask->GetFill( layer ), layer,
                                              m_webWidth / 2, m_maxError, ERROR_OUTSIDE );

                m_itemTree->Insert( aItem, layer, m_largestClearance );
            }
        }
    }
    else
    {
        for( PCB_LAYER_ID layer : { F_Mask, B_Mask } )
        {
            if( aItem->IsOnLayer( layer ) )
            {
                aItem->TransformShapeToPolygon( *solderMask->GetFill( layer ), layer,
                                                m_webWidth / 2, m_maxError, ERROR_OUTSIDE );

                m_itemTree->Insert( aItem, layer, m_largestClearance );
            }
        }
    }
}


void DRC_TEST_PROVIDER_SOLDER_MASK::buildRTrees()
{
    ZONE*  solderMask = m_board->m_SolderMask;
    LSET   layers = { 4, F_Mask, B_Mask, F_Cu, B_Cu };

    const size_t progressDelta = 500;
    int          count = 0;
    int          ii = 0;

    solderMask->GetFill( F_Mask )->RemoveAllContours();
    solderMask->GetFill( B_Mask )->RemoveAllContours();

    m_fullSolderMaskRTree = std::make_unique<DRC_RTREE>();
    m_itemTree = std::make_unique<DRC_RTREE>();

    forEachGeometryItem( s_allBasicItems, layers,
            [&]( BOARD_ITEM* item ) -> bool
            {
                ++count;
                return true;
            } );

    forEachGeometryItem( s_allBasicItems, layers,
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( !reportProgress( ii++, count, progressDelta ) )
                    return false;

                addItemToRTrees( item );
                return true;
            } );

    solderMask->GetFill( F_Mask )->Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
    solderMask->GetFill( B_Mask )->Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

    int numSegs = GetArcToSegmentCount( m_webWidth / 2, m_maxError, FULL_CIRCLE );

    solderMask->GetFill( F_Mask )->Deflate( m_webWidth / 2, numSegs );
    solderMask->GetFill( B_Mask )->Deflate( m_webWidth / 2, numSegs );

    solderMask->SetFillFlag( F_Mask, true );
    solderMask->SetFillFlag( B_Mask, true );
    solderMask->SetIsFilled( true );

    solderMask->CacheTriangulation();

    m_fullSolderMaskRTree->Insert( solderMask, F_Mask );
    m_fullSolderMaskRTree->Insert( solderMask, B_Mask );

    m_checkedPairs.clear();
}


void DRC_TEST_PROVIDER_SOLDER_MASK::testSilkToMaskClearance()
{
    LSET   silkLayers = { 2, F_SilkS, B_SilkS };

    const size_t progressDelta = 250;
    int          count = 0;
    int          ii = 0;

    forEachGeometryItem( s_allBasicItems, silkLayers,
            [&]( BOARD_ITEM* item ) -> bool
            {
                ++count;
                return true;
            } );

    forEachGeometryItem( s_allBasicItems, silkLayers,
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_SILK_CLEARANCE ) )
                    return false;

                if( !reportProgress( ii++, count, progressDelta ) )
                    return false;

                if( isInvisibleText( item ) )
                    return true;

                for( PCB_LAYER_ID layer : silkLayers.Seq() )
                {
                    if( !item->IsOnLayer( layer ) )
                        continue;

                    PCB_LAYER_ID   maskLayer = layer == F_SilkS ? F_Mask : B_Mask;
                    BOX2I          itemBBox = item->GetBoundingBox();
                    DRC_CONSTRAINT constraint = m_drcEngine->EvalRules( SILK_CLEARANCE_CONSTRAINT,
                                                                        item, nullptr, maskLayer );
                    int            clearance = constraint.GetValue().Min();
                    int            actual;
                    VECTOR2I       pos;

                    if( constraint.GetSeverity() == RPT_SEVERITY_IGNORE || clearance < 0 )
                        return true;

                    std::shared_ptr<SHAPE> itemShape = item->GetEffectiveShape( layer );

                    if( m_fullSolderMaskRTree->QueryColliding( itemBBox, itemShape.get(), maskLayer,
                                                               clearance, &actual, &pos ) )
                    {
                        auto drce = DRC_ITEM::Create( DRCE_SILK_CLEARANCE );

                        if( clearance > 0 )
                        {
                            wxString msg = formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                      constraint.GetName(),
                                                      clearance,
                                                      actual );

                            drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
                        }

                        drce->SetItems( item );
                        drce->SetViolatingRule( constraint.GetParentRule() );

                        reportViolation( drce, pos, layer );
                    }
                }

                return true;
            } );
}


bool isNullAperture( BOARD_ITEM* aItem )
{
    if( aItem->Type() == PCB_PAD_T )
    {
        PAD* pad = static_cast<PAD*>( aItem );

        if( pad->GetAttribute() == PAD_ATTRIB::NPTH
                && ( pad->GetShape() == PAD_SHAPE::CIRCLE || pad->GetShape() == PAD_SHAPE::OVAL )
                && pad->GetSize().x <= pad->GetDrillSize().x
                && pad->GetSize().y <= pad->GetDrillSize().y )
        {
            return true;
        }
    }

    return false;
}


// Simple mask apertures aren't associated with copper items, so they only constitute a bridge
// when they expose other copper items having at least two distinct nets.  We use a map to record
// the first net exposed by each mask aperture (on each copper layer).
//
// Note that this algorithm is also used for free pads.

bool isMaskAperture( BOARD_ITEM* aItem )
{
    if( aItem->Type() == PCB_PAD_T && static_cast<PAD*>( aItem )->IsFreePad() )
        return true;

    static const LSET saved( 2, F_Mask, B_Mask );

    LSET maskLayers = aItem->GetLayerSet() & saved;
    LSET otherLayers = aItem->GetLayerSet() & ~saved;

    return maskLayers.count() > 0 && otherLayers.count() == 0;
}


bool DRC_TEST_PROVIDER_SOLDER_MASK::checkMaskAperture( BOARD_ITEM* aMaskItem, BOARD_ITEM* aTestItem,
                                                       PCB_LAYER_ID aTestLayer, int aTestNet,
                                                       BOARD_ITEM** aCollidingItem )
{
    if( aTestLayer == F_Mask && !aTestItem->IsOnLayer( F_Cu ) )
        return false;

    if( aTestLayer == B_Mask && !aTestItem->IsOnLayer( B_Cu ) )
        return false;

    FOOTPRINT* fp = aMaskItem->GetParentFootprint();

    if( fp && ( fp->GetAttributes() & FP_ALLOW_SOLDERMASK_BRIDGES ) > 0 )
    {
        // Mask apertures in footprints which allow soldermask bridges are ignored entirely.
        return false;
    }

    PTR_LAYER_CACHE_KEY key = { aMaskItem, aTestLayer };

    auto ii = m_maskApertureNetMap.find( key );

    if( ii == m_maskApertureNetMap.end() )
    {
        m_maskApertureNetMap[ key ] = { aTestItem, aTestNet };

        // First net; no bridge yet....
        return false;
    }

    if( ii->second.second == aTestNet && aTestNet > 0 )
    {
        // Same net; still no bridge...
        return false;
    }

    if( fp && ii->second.first->Type() == PCB_PAD_T && aTestItem->Type() == PCB_PAD_T )
    {
        PAD* alreadyEncounteredPad = static_cast<PAD*>( ii->second.first );
        PAD* thisPad = static_cast<PAD*>( aTestItem );

        if( alreadyEncounteredPad->SharesNetTieGroup( thisPad ) )
            return false;
    }

    *aCollidingItem = ii->second.first;
    return true;
}


bool DRC_TEST_PROVIDER_SOLDER_MASK::checkItemMask( BOARD_ITEM* aMaskItem, int aTestNet )
{
    FOOTPRINT* fp = aMaskItem->GetParentFootprint();

    wxCHECK( fp, false );

    if( ( fp->GetAttributes() & FP_ALLOW_SOLDERMASK_BRIDGES ) > 0 )
    {
        // If we're allowing bridges then we're allowing bridges.  Nothing to check.
        return false;
    }

    // Graphic items are used to implement net-ties between pads of a group within a net-tie
    // footprint.  They must be allowed to intrude into their pad's mask aperture.
    if( aTestNet < 0 && aMaskItem->Type() == PCB_PAD_T && fp->IsNetTie() )
    {
        std::map<wxString, int> padNumberToGroupIdxMap = fp->MapPadNumbersToNetTieGroups();

        if( padNumberToGroupIdxMap[ static_cast<PAD*>( aMaskItem )->GetNumber() ] >= 0 )
            return false;
    }

    return true;
}


void DRC_TEST_PROVIDER_SOLDER_MASK::testItemAgainstItems( BOARD_ITEM* aItem, const BOX2I& aItemBBox,
                                                          PCB_LAYER_ID aRefLayer,
                                                          PCB_LAYER_ID aTargetLayer )
{
    int itemNet = -1;

    if( aItem->IsConnected() )
        itemNet = static_cast<BOARD_CONNECTED_ITEM*>( aItem )->GetNetCode();

    BOARD_DESIGN_SETTINGS& bds = aItem->GetBoard()->GetDesignSettings();
    PAD*                   pad = dynamic_cast<PAD*>( aItem );
    PCB_VIA*               via = dynamic_cast<PCB_VIA*>( aItem );
    std::shared_ptr<SHAPE> itemShape = aItem->GetEffectiveShape( aRefLayer );

    m_itemTree->QueryColliding( aItem, aRefLayer, aTargetLayer,
            // Filter:
            [&]( BOARD_ITEM* other ) -> bool
            {
                FOOTPRINT* itemFP = aItem->GetParentFootprint();
                PAD*       otherPad = dynamic_cast<PAD*>( other );
                int        otherNet = -1;

                if( other->IsConnected() )
                    otherNet = static_cast<BOARD_CONNECTED_ITEM*>( other )->GetNetCode();

                if( otherNet > 0 && otherNet == itemNet )
                    return false;

                if( isNullAperture( other ) )
                    return false;

                if( itemFP && itemFP == other->GetParentFootprint() )
                {
                    // Board-wide exclusion
                    if( bds.m_AllowSoldermaskBridgesInFPs )
                        return false;

                    // Footprint-specific exclusion
                    if( ( itemFP->GetAttributes() & FP_ALLOW_SOLDERMASK_BRIDGES ) > 0 )
                        return false;
                }

                if( pad && otherPad && ( pad->SameLogicalPadAs( otherPad )
                                         || pad->SharesNetTieGroup( otherPad ) ) )
                {
                    return false;
                }

                BOARD_ITEM* a = aItem;
                BOARD_ITEM* b = other;

                // store canonical order so we don't collide in both directions (a:b and b:a)
                if( static_cast<void*>( a ) > static_cast<void*>( b ) )
                    std::swap( a, b );

                auto it = m_checkedPairs.find( { a, b } );

                if( it != m_checkedPairs.end() && it->second.test( aTargetLayer ) )
                {
                    return false;
                }
                else
                {
                    m_checkedPairs[ { a, b } ].set( aTargetLayer );
                    return true;
                }
            },
            // Visitor:
            [&]( BOARD_ITEM* other ) -> bool
            {
                PAD*     otherPad = dynamic_cast<PAD*>( other );
                PCB_VIA* otherVia = dynamic_cast<PCB_VIA*>( other );
                auto     otherShape = other->GetEffectiveShape( aTargetLayer );
                int      otherNet = -1;

                if( other->IsConnected() )
                    otherNet = static_cast<BOARD_CONNECTED_ITEM*>( other )->GetNetCode();

                int      actual;
                VECTOR2I pos;
                int      clearance = 0;

                if( aRefLayer == F_Mask || aRefLayer == B_Mask )
                {
                    // Aperture-to-aperture must enforce web-min-width
                    clearance = m_webWidth;
                }
                else // ( aRefLayer == F_Cu || aRefLayer == B_Cu )
                {
                    // Copper-to-aperture uses the solder-mask-to-copper-clearance
                    clearance = m_board->GetDesignSettings().m_SolderMaskToCopperClearance;
                }

                if( pad )
                    clearance += pad->GetSolderMaskExpansion();
                else if( via && !via->IsTented() )
                    clearance += via->GetSolderMaskExpansion();

                if( otherPad )
                    clearance += otherPad->GetSolderMaskExpansion();
                else if( otherVia && !otherVia->IsTented() )
                    clearance += otherVia->GetSolderMaskExpansion();

                if( itemShape->Collide( otherShape.get(), clearance, &actual, &pos ) )
                {
                    wxString    msg;
                    BOARD_ITEM* colliding = nullptr;

                    if( aTargetLayer == F_Mask )
                        msg = _( "Front solder mask aperture bridges items with different nets" );
                    else
                        msg = _( "Rear solder mask aperture bridges items with different nets" );

                    // Simple mask apertures aren't associated with copper items, so they only
                    // constitute a bridge when they expose other copper items having at least
                    // two distinct nets.
                    if( isMaskAperture( aItem ) )
                    {
                        if( checkMaskAperture( aItem, other, aRefLayer, otherNet, &colliding ) )
                        {
                            auto drce = DRC_ITEM::Create( DRCE_SOLDERMASK_BRIDGE );

                            drce->SetErrorMessage( msg );
                            drce->SetItems( aItem, colliding, other );
                            drce->SetViolatingRule( &m_bridgeRule );
                            reportViolation( drce, pos, aTargetLayer );
                        }
                    }
                    else if( isMaskAperture( other ) )
                    {
                        if( checkMaskAperture( other, aItem, aRefLayer, itemNet, &colliding ) )
                        {
                            auto drce = DRC_ITEM::Create( DRCE_SOLDERMASK_BRIDGE );

                            drce->SetErrorMessage( msg );
                            drce->SetItems( other, colliding, aItem );
                            drce->SetViolatingRule( &m_bridgeRule );
                            reportViolation( drce, pos, aTargetLayer );
                        }
                    }
                    else if( checkItemMask( other, itemNet ) )
                    {
                        auto drce = DRC_ITEM::Create( DRCE_SOLDERMASK_BRIDGE );

                        drce->SetErrorMessage( msg );
                        drce->SetItems( aItem, other );
                        drce->SetViolatingRule( &m_bridgeRule );
                        reportViolation( drce, pos, aTargetLayer );
                    }
                }

                return !m_drcEngine->IsCancelled();
            },
            m_largestClearance );
}


void DRC_TEST_PROVIDER_SOLDER_MASK::testMaskItemAgainstZones( BOARD_ITEM* aItem,
                                                              const BOX2I& aItemBBox,
                                                              PCB_LAYER_ID aMaskLayer,
                                                              PCB_LAYER_ID aTargetLayer )
{
    for( ZONE* zone : m_board->m_DRCCopperZones )
    {
        if( !zone->GetLayerSet().test( aTargetLayer ) )
            continue;

        int zoneNet = zone->GetNetCode();

        if( aItem->IsConnected() )
        {
            BOARD_CONNECTED_ITEM* connectedItem = static_cast<BOARD_CONNECTED_ITEM*>( aItem );

            if( zoneNet == connectedItem->GetNetCode() && zoneNet > 0 )
                continue;
        }

        BOX2I inflatedBBox( aItemBBox );
        int   clearance = m_board->GetDesignSettings().m_SolderMaskToCopperClearance;

        if( aItem->Type() == PCB_PAD_T )
            clearance += static_cast<PAD*>( aItem )->GetSolderMaskExpansion();
        else if( aItem->Type() == PCB_VIA_T )
            clearance += static_cast<PCB_VIA*>( aItem )->GetSolderMaskExpansion();

        inflatedBBox.Inflate( clearance );

        if( !inflatedBBox.Intersects( zone->GetBoundingBox() ) )
            continue;

        DRC_RTREE* zoneTree = m_board->m_CopperZoneRTreeCache[ zone ].get();
        int        actual;
        VECTOR2I   pos;

        std::shared_ptr<SHAPE> itemShape = aItem->GetEffectiveShape( aMaskLayer );

        if( zoneTree && zoneTree->QueryColliding( aItemBBox, itemShape.get(), aTargetLayer,
                                                  clearance, &actual, &pos ) )
        {
            wxString    msg;
            BOARD_ITEM* colliding = nullptr;

            if( aMaskLayer == F_Mask )
                msg = _( "Front solder mask aperture bridges items with different nets" );
            else
                msg = _( "Rear solder mask aperture bridges items with different nets" );

            // Simple mask apertures aren't associated with copper items, so they only constitute
            // a bridge when they expose other copper items having at least two distinct nets.
            if( isMaskAperture( aItem ) && zoneNet >= 0 )
            {
                if( checkMaskAperture( aItem, zone, aTargetLayer, zoneNet, &colliding ) )
                {
                    auto drce = DRC_ITEM::Create( DRCE_SOLDERMASK_BRIDGE );

                    drce->SetErrorMessage( msg );
                    drce->SetItems( aItem, colliding, zone );
                    drce->SetViolatingRule( &m_bridgeRule );
                    reportViolation( drce, pos, aTargetLayer );
                }
            }
            else
            {
                auto drce = DRC_ITEM::Create( DRCE_SOLDERMASK_BRIDGE );

                drce->SetErrorMessage( msg );
                drce->SetItems( aItem, zone );
                drce->SetViolatingRule( &m_bridgeRule );
                reportViolation( drce, pos, aTargetLayer );
            }
        }

        if( m_drcEngine->IsCancelled() )
            return;
    }
}


void DRC_TEST_PROVIDER_SOLDER_MASK::testMaskBridges()
{
    LSET   copperAndMaskLayers = { 4, F_Mask, B_Mask, F_Cu, B_Cu };

    const size_t progressDelta = 250;
    int          count = 0;
    int          ii = 0;

    forEachGeometryItem( s_allBasicItemsButZones, copperAndMaskLayers,
            [&]( BOARD_ITEM* item ) -> bool
            {
                ++count;
                return true;
            } );

    forEachGeometryItem( s_allBasicItemsButZones, copperAndMaskLayers,
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_SOLDERMASK_BRIDGE ) )
                    return false;

                if( !reportProgress( ii++, count, progressDelta ) )
                    return false;

                BOX2I itemBBox = item->GetBoundingBox();

                if( item->IsOnLayer( F_Mask ) && !isNullAperture( item ) )
                {
                    // Test for aperture-to-aperture collisions
                    testItemAgainstItems( item, itemBBox, F_Mask, F_Mask );

                    // Test for aperture-to-zone collisions
                    testMaskItemAgainstZones( item, itemBBox, F_Mask, F_Cu );
                }
                else if( item->IsOnLayer( F_Cu ) )
                {
                    // Test for copper-item-to-aperture collisions
                    testItemAgainstItems( item, itemBBox, F_Cu, F_Mask );
                }

                if( item->IsOnLayer( B_Mask ) && !isNullAperture( item ) )
                {
                    // Test for aperture-to-aperture collisions
                    testItemAgainstItems( item, itemBBox, B_Mask, B_Mask );

                    // Test for aperture-to-zone collisions
                    testMaskItemAgainstZones( item, itemBBox, B_Mask, B_Cu );
                }
                else if( item->IsOnLayer( B_Cu ) )
                {
                    // Test for copper-item-to-aperture collisions
                    testItemAgainstItems( item, itemBBox, B_Cu, B_Mask );
                }

                return true;
            } );
}


bool DRC_TEST_PROVIDER_SOLDER_MASK::Run()
{
    if( m_drcEngine->IsErrorLimitExceeded( DRCE_SILK_CLEARANCE )
            && m_drcEngine->IsErrorLimitExceeded( DRCE_SOLDERMASK_BRIDGE ) )
    {
        reportAux( wxT( "Solder mask violations ignored. Tests not run." ) );
        return true;    // continue with other tests
    }

    m_board = m_drcEngine->GetBoard();
    m_webWidth = m_board->GetDesignSettings().m_SolderMaskMinWidth;
    m_maxError = m_board->GetDesignSettings().m_MaxError;
    m_largestClearance = 0;

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
            m_largestClearance = std::max( m_largestClearance, pad->GetSolderMaskExpansion() );
    }

    // Order is important here: m_webWidth must be added in before m_largestCourtyardClearance is
    // maxed with the various SILK_CLEARANCE_CONSTRAINTS.
    m_largestClearance += m_largestClearance + m_webWidth;

    DRC_CONSTRAINT worstClearanceConstraint;

    if( m_drcEngine->QueryWorstConstraint( SILK_CLEARANCE_CONSTRAINT, worstClearanceConstraint ) )
        m_largestClearance = std::max( m_largestClearance, worstClearanceConstraint.m_Value.Min() );

    reportAux( wxT( "Worst clearance : %d nm" ), m_largestClearance );

    if( !reportPhase( _( "Building solder mask..." ) ) )
        return false;   // DRC cancelled

    m_checkedPairs.clear();
    m_maskApertureNetMap.clear();

    buildRTrees();

    if( !reportPhase( _( "Checking solder mask to silk clearance..." ) ) )
        return false;   // DRC cancelled

    testSilkToMaskClearance();

    if( !reportPhase( _( "Checking solder mask web integrity..." ) ) )
        return false;   // DRC cancelled

    testMaskBridges();

    reportRuleStatistics();

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_SOLDER_MASK> dummy;
}
