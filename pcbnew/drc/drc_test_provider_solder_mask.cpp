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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <board_design_settings.h>
#include <board_connected_item.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <pcb_text.h>
#include <thread_pool.h>
#include <zone.h>
#include <geometry/seg.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include <drc/drc_rtree.h>

#include <algorithm>
#include <array>
#include <set>
#include <unordered_set>
#include <vector>

/*
    Solder mask tests. Checks for silkscreen which is clipped by mask openings and for bridges
    between mask apertures with different nets.
    Errors generated:
    - DRCE_SILK_MASK_CLEARANCE
    - DRCE_SOLDERMASK_BRIDGE
*/


static void addItemPolysWithEndings( BOARD_ITEM* aItem, SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer, int aClearance,
                                     int aError, ERROR_LOC aErrorLoc )
{
    if( aItem->Type() == PCB_SHAPE_T )
    {
        PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( aItem );
        shape->TransformWithLineEndingsToPolygon( aBuffer, aClearance, aError, aErrorLoc );
    }
    else
    {
        aItem->TransformShapeToPolygon( aBuffer, aLayer, aClearance, aError, aErrorLoc );
    }
}


class DRC_TEST_PROVIDER_SOLDER_MASK : public ::DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_SOLDER_MASK ():
            m_board( nullptr ),
            m_webWidth( 0 ),
            m_maxError( 0 ),
            m_largestClearance( 0 ),
            m_bridgeLimit( 0 )
    {
        m_bridgeRule.m_Name = _( "board setup solder mask min width" );
    }

    virtual ~DRC_TEST_PROVIDER_SOLDER_MASK() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "solder_mask_issues" ); };

private:
    void addItemToRTrees( BOARD_ITEM* aItem );
    void buildRTrees();

    void testSilkToMaskClearance();
    void testMaskBridges();

    void testItemAgainstItems( BOARD_ITEM* aItem, const BOX2I& aItemBBox,
                               PCB_LAYER_ID aRefLayer, PCB_LAYER_ID aTargetLayer );
    void testMaskItemAgainstZones( BOARD_ITEM* item, const BOX2I& itemBBox,
                                   PCB_LAYER_ID refLayer, PCB_LAYER_ID targetLayer );

    void recordMaskAperture( BOARD_ITEM* aMaskItem, BOARD_ITEM* aTestItem, PCB_LAYER_ID aTestLayer,
                             int aTestNet, const VECTOR2I& aPos );

    bool maskApertureBridgeExcluded( FOOTPRINT* aApertureFootprint,
                                     const std::map<wxString, int>& aNetTieGroups,
                                     BOARD_ITEM* aRefItem, BOARD_ITEM* aTestItem );

    void reportMaskApertureBridges();

    void collectBridge( BOARD_ITEM* aItemA, BOARD_ITEM* aItemB, BOARD_ITEM* aItemC,
                        const VECTOR2I& aPos, PCB_LAYER_ID aLayer );

    void flushBridgeViolations();

    bool checkItemMask( BOARD_ITEM* aItem, int aTestNet );

private:
    DRC_RULE m_bridgeRule;

    BOARD*   m_board;
    int      m_webWidth;
    int      m_maxError;
    int      m_largestClearance;

    std::unique_ptr<DRC_RTREE> m_fullSolderMaskRTree;
    std::unique_ptr<DRC_RTREE> m_itemTree;

    std::mutex                                  m_checkedPairsMutex;
    std::unordered_map<PTR_PTR_CACHE_KEY, LSET> m_checkedPairs;

    // Shapes used to define solder mask apertures don't have nets, so a bridge exists only when
    // an aperture exposes copper on two different nets.  Every (item, net) that collides with an
    // aperture is recorded during the parallel pass, then bridges are decided and reported
    // single-threaded afterwards so the reported set never depends on worker arrival order.
    struct MASK_APERTURE_ITEM
    {
        BOARD_ITEM* item;
        int         net;
        VECTOR2I    pos;
    };

    std::mutex                                                               m_apertureMutex;
    std::unordered_map<PTR_LAYER_CACHE_KEY, std::vector<MASK_APERTURE_ITEM>> m_maskApertureItems;

    // Bridges are collected during the tests and reported in a deterministic order afterwards.
    // Without a stable order the error-limit cap keeps a different subset of an over-limit board's
    // bridges on every run, so the report entries wobble even though the count is stable.  Order by
    // layer, then the sorted UUIDs of the participating items, then the position, none of which
    // depend on heap addresses or worker scheduling.
    struct PENDING_BRIDGE
    {
        BOARD_ITEM*         a;
        BOARD_ITEM*         b;
        BOARD_ITEM*         c;
        VECTOR2I            pos;
        PCB_LAYER_ID        layer;
        std::array<KIID, 3> ids;

        bool operator<( const PENDING_BRIDGE& aRhs ) const
        {
            if( layer != aRhs.layer )
                return layer < aRhs.layer;

            for( size_t ii = 0; ii < ids.size(); ++ii )
            {
                if( ids[ ii ] != aRhs.ids[ ii ] )
                    return ids[ ii ] < aRhs.ids[ ii ];
            }

            if( pos.x != aRhs.pos.x )
                return pos.x < aRhs.pos.x;

            return pos.y < aRhs.pos.y;
        }
    };

    // Only the first m_bridgeLimit bridges in that order can ever be reported, so the collection is
    // a bounded max-heap rather than the full list.  A board whose bridge count runs into the
    // millions would otherwise cost memory and a sort proportional to a count the report discards.
    std::mutex                  m_bridgeMutex;
    std::vector<PENDING_BRIDGE> m_bridgeViolations;
    int                         m_bridgeLimit;
};


void DRC_TEST_PROVIDER_SOLDER_MASK::addItemToRTrees( BOARD_ITEM* aItem )
{
    // Rule areas are purely logical: no copper, no mask, no silk.  Skip them entirely
    // so they cannot contribute to solder-mask bridge or silk-to-mask collisions.
    if( aItem->Type() == PCB_ZONE_T && static_cast<ZONE*>( aItem )->GetIsRuleArea() )
        return;

    for( PCB_LAYER_ID layer : { F_Mask, B_Mask } )
    {
        if( !aItem->IsOnLayer( layer ) )
            continue;

        SHAPE_POLY_SET* solderMask = m_board->m_SolderMaskBridges->GetFill( layer );

        if( aItem->Type() == PCB_ZONE_T )
        {
            ZONE* zone = static_cast<ZONE*>( aItem );

            solderMask->BooleanAdd( *zone->GetFilledPolysList( layer ) );
        }
        else
        {
            int clearance = m_webWidth / 2;

            if( aItem->Type() == PCB_PAD_T )
                clearance += static_cast<PAD*>( aItem )->GetSolderMaskExpansion( layer );
            else if( aItem->Type() == PCB_VIA_T )
                clearance += static_cast<PCB_VIA*>( aItem )->GetSolderMaskExpansion();
            else if( aItem->Type() == PCB_TRACE_T )
                clearance += static_cast<PCB_TRACK*>( aItem )->GetSolderMaskExpansion();
            else if( aItem->Type() == PCB_SHAPE_T )
                clearance += static_cast<PCB_SHAPE*>( aItem )->GetSolderMaskExpansion();

            if( aItem->Type() == PCB_FIELD_T || aItem->Type() == PCB_TEXT_T )
            {
                PCB_TEXT* text = static_cast<PCB_TEXT*>( aItem );

                text->TransformTextToPolySet( *solderMask, clearance, m_maxError, ERROR_OUTSIDE );
            }
            else
            {
                addItemPolysWithEndings( aItem, *solderMask, layer, clearance, m_maxError, ERROR_OUTSIDE );
            }

            m_itemTree->Insert( aItem, layer, NULL_CONSTRAINT, m_largestClearance );
        }
    }
}


void DRC_TEST_PROVIDER_SOLDER_MASK::buildRTrees()
{
    ZONE*  solderMask = m_board->m_SolderMaskBridges;
    LSET   layers( { F_Mask, B_Mask, F_Cu, B_Cu } );

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

    solderMask->GetFill( F_Mask )->Simplify();
    solderMask->GetFill( B_Mask )->Simplify();

    if( m_webWidth > 0 )
    {
        solderMask->GetFill( F_Mask )->Deflate( m_webWidth / 2, CORNER_STRATEGY::CHAMFER_ALL_CORNERS, m_maxError );
        solderMask->GetFill( B_Mask )->Deflate( m_webWidth / 2, CORNER_STRATEGY::CHAMFER_ALL_CORNERS, m_maxError );
    }

    solderMask->SetFillFlag( F_Mask, true );
    solderMask->SetFillFlag( B_Mask, true );
    solderMask->SetIsFilled( true );

    solderMask->CacheTriangulation();

    m_fullSolderMaskRTree->Insert( solderMask, F_Mask, NULL_CONSTRAINT );
    m_fullSolderMaskRTree->Insert( solderMask, B_Mask, NULL_CONSTRAINT );
    m_fullSolderMaskRTree->Build();

    m_itemTree->Build();

    m_checkedPairs.clear();
}


void DRC_TEST_PROVIDER_SOLDER_MASK::testSilkToMaskClearance()
{
    LSET   silkLayers( { F_SilkS, B_SilkS } );

    // If we have no minimum web width then we delegate to the silk checker which does object-to-object
    // testing (instead of object-to-solder-mask-zone-fill checking that we do here).
    if( m_webWidth <= 0 )
        return;

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
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_SILK_MASK_CLEARANCE ) )
                    return false;

                if( !reportProgress( ii++, count, progressDelta ) )
                    return false;

                if( isInvisibleText( item ) )
                    return true;

                for( PCB_LAYER_ID layer : silkLayers )
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
                        std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_SILK_MASK_CLEARANCE );

                        if( clearance > 0 )
                        {
                            drce->SetErrorDetail( formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                             constraint.GetName(),
                                                             clearance,
                                                             actual ) );
                        }

                        drce->SetItems( item );
                        drce->SetViolatingRule( constraint.GetParentRule() );

                        reportViolation( drce, pos, layer );
                    }
                }

                return true;
            } );
}


bool isNPTHPadWithNoCopper( BOARD_ITEM* aItem )
{
    if( aItem->Type() == PCB_PAD_T )
        return static_cast<PAD*>( aItem )->IsNPTHWithNoCopper();

    return false;
}


// Simple mask apertures aren't associated with copper items, so they only constitute a bridge
// when they expose other copper items having at least two distinct nets.
//
// Note that this algorithm is also used for free pads.

bool isMaskAperture( BOARD_ITEM* aItem )
{
    if( aItem->Type() == PCB_PAD_T && static_cast<PAD*>( aItem )->IsFreePad() )
        return true;

    static const LSET saved( { F_Mask, B_Mask } );

    LSET maskLayers = aItem->GetLayerSet() & saved;
    LSET copperLayers = ( aItem->GetLayerSet() & ~saved ) & LSET::AllCuMask();

    return maskLayers.count() > 0 && copperLayers.count() == 0;
}


void DRC_TEST_PROVIDER_SOLDER_MASK::recordMaskAperture( BOARD_ITEM* aMaskItem, BOARD_ITEM* aTestItem,
                                                        PCB_LAYER_ID aTestLayer, int aTestNet,
                                                        const VECTOR2I& aPos )
{
    // Only positive nets can bridge, and the pairing below is quadratic in what is recorded here.
    if( aTestNet <= 0 )
        return;

    if( aTestLayer == F_Mask && !aTestItem->IsOnLayer( F_Cu ) )
        return;

    if( aTestLayer == B_Mask && !aTestItem->IsOnLayer( B_Cu ) )
        return;

    // Mask apertures in footprints which allow soldermask bridges are ignored entirely.
    if( FOOTPRINT* fp = aMaskItem->GetParentFootprint(); fp && fp->AllowSolderMaskBridges() )
        return;

    PCB_LAYER_ID        maskLayer = IsFrontLayer( aTestLayer ) ? F_Mask : B_Mask;
    PTR_LAYER_CACHE_KEY key = { aMaskItem, maskLayer };

    std::lock_guard<std::mutex> lock( m_apertureMutex );
    m_maskApertureItems[ key ].push_back( { aTestItem, aTestNet, aPos } );
}


// Items belonging to the same net-tie group (or the same logical pad) may legitimately share a
// mask aperture, so a pairing between them is not a bridge.

bool DRC_TEST_PROVIDER_SOLDER_MASK::maskApertureBridgeExcluded(
        FOOTPRINT* aApertureFootprint, const std::map<wxString, int>& aNetTieGroups,
        BOARD_ITEM* aRefItem, BOARD_ITEM* aTestItem )
{
    if( !aApertureFootprint || aTestItem->GetParentFootprint() != aApertureFootprint )
        return false;

    PAD* padA = aRefItem->Type() == PCB_PAD_T ? static_cast<PAD*>( aRefItem ) : nullptr;
    PAD* padB = aTestItem->Type() == PCB_PAD_T ? static_cast<PAD*>( aTestItem ) : nullptr;

    if( padA && padB )
        return padA->SameLogicalPadAs( padB ) || padA->SharesNetTieGroup( padB );

    if( padA && aTestItem->Type() == PCB_SHAPE_T )
        return aNetTieGroups.contains( padA->GetNumber() );
    else if( padB && aRefItem->Type() == PCB_SHAPE_T )
        return aNetTieGroups.contains( padB->GetNumber() );

    return false;
}


bool DRC_TEST_PROVIDER_SOLDER_MASK::checkItemMask( BOARD_ITEM* aItem, int aTestNet )
{
    if( FOOTPRINT* fp = aItem->GetParentFootprint() )
    {
        // If we're allowing bridges then we're allowing bridges.  Nothing to check.
        if( fp->AllowSolderMaskBridges() )
            return false;

        // Items belonging to a net-tie may share the mask aperture of pads in the same group.
        if( aItem->Type() == PCB_PAD_T && fp->IsNetTie() )
        {
            PAD* pad = static_cast<PAD*>( aItem );
            std::map<wxString, int> padNumberToGroupIdxMap = fp->MapPadNumbersToNetTieGroups();
            int groupIdx = padNumberToGroupIdxMap[ pad->GetNumber() ];

            if( groupIdx >= 0 )
            {
                if( aTestNet < 0 )
                    return false;

                if( pad->GetNetCode() == aTestNet )
                    return false;

                for( PAD* other : fp->GetNetTiePads( pad ) )
                {
                    if( other->GetNetCode() == aTestNet )
                        return false;
                }
            }
        }
    }

    return true;
}


void DRC_TEST_PROVIDER_SOLDER_MASK::testItemAgainstItems( BOARD_ITEM* aItem, const BOX2I& aItemBBox,
                                                          PCB_LAYER_ID aRefLayer, PCB_LAYER_ID aTargetLayer )
{
    PAD*       pad = aItem->Type() == PCB_PAD_T ? static_cast<PAD*>( aItem ) : nullptr;
    PCB_VIA*   via = aItem->Type() == PCB_VIA_T ? static_cast<PCB_VIA*>( aItem ) : nullptr;
    PCB_SHAPE* shape = aItem->Type() == PCB_SHAPE_T ? static_cast<PCB_SHAPE*>( aItem ) : nullptr;
    int        itemNet = -1;

    std::optional<DRC_CONSTRAINT> itemConstraint;
    DRC_CONSTRAINT                otherConstraint;

    if( aItem->IsConnected() )
        itemNet = static_cast<BOARD_CONNECTED_ITEM*>( aItem )->GetNetCode();

    std::shared_ptr<SHAPE> itemShape = aItem->GetEffectiveShape( aRefLayer );

    m_itemTree->QueryColliding( aItem, aRefLayer, aTargetLayer,
            // Filter:
            [&]( BOARD_ITEM* other ) -> bool
            {
                FOOTPRINT* itemFP = aItem->GetParentFootprint();
                PAD*       otherPad = other->Type() == PCB_PAD_T ? static_cast<PAD*>( other ) : nullptr;
                int        otherNet = -1;

                if( other->IsConnected() )
                    otherNet = static_cast<BOARD_CONNECTED_ITEM*>( other )->GetNetCode();

                if( otherNet > 0 && otherNet == itemNet )
                    return false;

                if( isNPTHPadWithNoCopper( other ) )
                    return false;

                if( itemFP && itemFP == other->GetParentFootprint() )
                {
                    // Board-wide exclusion
                    if( BOARD* board = itemFP->GetBoard() )
                    {
                        if( board->GetDesignSettings().m_AllowSoldermaskBridgesInFPs )
                            return false;
                    }

                    // Footprint-specific exclusion
                    if( itemFP->AllowSolderMaskBridges() )
                        return false;
                }

                if( pad && otherPad && ( pad->SameLogicalPadAs( otherPad )
                                         || pad->SharesNetTieGroup( otherPad ) ) )
                {
                    return false;
                }

                if( itemFP && itemFP->IsNetTie() )
                {
                    const std::set<int>& nets = itemFP->GetNetTieCache( aItem );

                    if( otherNet < 0 || nets.count( otherNet ) )
                        return false;
                }

                if( FOOTPRINT* otherFP = other->GetParentFootprint(); otherFP && otherFP->IsNetTie() )
                {
                    const std::set<int>& nets = otherFP->GetNetTieCache( other );

                    if( itemNet < 0 || nets.count( itemNet ) )
                        return false;
                }

                BOARD_ITEM* a = aItem;
                BOARD_ITEM* b = other;

                // store canonical order so we don't collide in both directions (a:b and b:a)
                if( static_cast<void*>( a ) > static_cast<void*>( b ) )
                    std::swap( a, b );

                {
                    std::lock_guard<std::mutex> lock( m_checkedPairsMutex );
                    auto it = m_checkedPairs.find( { a, b } );

                    if( it != m_checkedPairs.end() && it->second.test( aTargetLayer ) )
                    {
                        return false;
                    }
                    else
                    {
                        m_checkedPairs[{ a, b }].set( aTargetLayer );
                        return true;
                    }
                }
            },
            // Visitor:
            [&]( BOARD_ITEM* other ) -> bool
            {
                PAD*       otherPad = other->Type() == PCB_PAD_T ? static_cast<PAD*>( other ) : nullptr;
                PCB_VIA*   otherVia = other->Type() == PCB_VIA_T ? static_cast<PCB_VIA*>( other ) : nullptr;
                PCB_SHAPE* otherShape = other->Type() == PCB_SHAPE_T ? static_cast<PCB_SHAPE*>( other ) : nullptr;
                auto       otherItemShape = other->GetEffectiveShape( aTargetLayer );
                int        otherNet = -1;

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
                    clearance += pad->GetSolderMaskExpansion( aRefLayer );
                else if( via && !via->IsTented( aRefLayer ) )
                    clearance += via->GetSolderMaskExpansion();
                else if( shape )
                    clearance += shape->GetSolderMaskExpansion();

                if( otherPad )
                    clearance += otherPad->GetSolderMaskExpansion( aTargetLayer );
                else if( otherVia && !otherVia->IsTented( aTargetLayer ) )
                    clearance += otherVia->GetSolderMaskExpansion();
                else if( otherShape )
                    clearance += otherShape->GetSolderMaskExpansion();

                if( itemShape->Collide( otherItemShape.get(), clearance, &actual, &pos ) )
                {
                    if( !itemConstraint.has_value() )
                        itemConstraint = m_drcEngine->EvalRules( BRIDGED_MASK_CONSTRAINT, aItem, nullptr, aRefLayer );

                    otherConstraint = m_drcEngine->EvalRules( BRIDGED_MASK_CONSTRAINT, other, nullptr, aTargetLayer );

                    bool itemConstraintIgnored = itemConstraint->GetSeverity() == RPT_SEVERITY_IGNORE;
                    bool otherConstraintIgnored = otherConstraint.GetSeverity() == RPT_SEVERITY_IGNORE;

                    // Mask apertures are ignored on their own; in other cases both participants must be ignored
                    if(    ( isMaskAperture( aItem ) && itemConstraintIgnored )
                        || ( isMaskAperture( other ) && otherConstraintIgnored )
                        || ( itemConstraintIgnored && otherConstraintIgnored ) )
                    {
                        return !m_drcEngine->IsCancelled();
                    }

                    // Simple mask apertures aren't associated with copper items, so they only
                    // constitute a bridge when they expose other copper items having at least
                    // two distinct nets.  Record the colliding item now and decide/report bridges
                    // deterministically once all threads have finished.
                    if( isMaskAperture( aItem ) )
                    {
                        recordMaskAperture( aItem, other, aRefLayer, otherNet, pos );
                    }
                    else if( isMaskAperture( other ) )
                    {
                        recordMaskAperture( other, aItem, aRefLayer, itemNet, pos );
                    }
                    else if( checkItemMask( other, itemNet ) )
                    {
                        collectBridge( aItem, other, nullptr, pos, aTargetLayer );
                    }
                }

                return !m_drcEngine->IsCancelled();
            },
            m_largestClearance );
}


void DRC_TEST_PROVIDER_SOLDER_MASK::testMaskItemAgainstZones( BOARD_ITEM* aItem, const BOX2I& aItemBBox,
                                                              PCB_LAYER_ID aMaskLayer, PCB_LAYER_ID aTargetLayer )
{
    PAD*       pad = aItem->Type() == PCB_PAD_T ? static_cast<PAD*>( aItem ) : nullptr;
    PCB_VIA*   via = aItem->Type() == PCB_VIA_T ? static_cast<PCB_VIA*>( aItem ) : nullptr;
    PCB_SHAPE* shape = aItem->Type() == PCB_SHAPE_T ? static_cast<PCB_SHAPE*>( aItem ) : nullptr;

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

        if( pad )
            clearance += pad->GetSolderMaskExpansion( aTargetLayer );
        else if( via && !via->IsTented( aTargetLayer ) )
            clearance += via->GetSolderMaskExpansion();
        else if( shape )
            clearance += shape->GetSolderMaskExpansion();

        inflatedBBox.Inflate( clearance );

        if( !inflatedBBox.Intersects( zone->GetBoundingBox() ) )
            continue;

        DRC_RTREE* zoneTree = m_board->m_CopperZoneRTreeCache[ zone ].get();
        int        actual;
        VECTOR2I   pos;

        std::shared_ptr<SHAPE> itemShape = aItem->GetEffectiveShape( aMaskLayer );

        if( zoneTree && zoneTree->QueryColliding( aItemBBox, itemShape.get(), aTargetLayer, clearance,
                                                  &actual, &pos ) )
        {
            // Simple mask apertures aren't associated with copper items, so they only constitute
            // a bridge when they expose other copper items having at least two distinct nets.
            if( isMaskAperture( aItem ) && zoneNet >= 0 )
            {
                recordMaskAperture( aItem, zone, aMaskLayer, zoneNet, pos );
            }
            else
            {
                collectBridge( aItem, zone, nullptr, pos, aTargetLayer );
            }
        }

        if( m_drcEngine->IsCancelled() )
            return;
    }
}


void DRC_TEST_PROVIDER_SOLDER_MASK::testMaskBridges()
{
    LSET                     copperAndMaskLayers( { F_Mask, B_Mask, F_Cu, B_Cu } );
    std::atomic<int>         count = 0;
    std::vector<BOARD_ITEM*> test_items;

    forEachGeometryItem( s_allBasicItemsButZones, copperAndMaskLayers,
            [&]( BOARD_ITEM* item ) -> bool
            {
                test_items.push_back( item );
                return true;
            } );

    thread_pool& tp = GetKiCadThreadPool();

    auto returns = tp.submit_loop( 0, test_items.size(),
            [&]( size_t i ) -> bool
            {
                BOARD_ITEM* item = test_items[ i ];

                if( m_drcEngine->IsCancelled() )
                    return false;

                BOX2I itemBBox = item->GetBoundingBox();

                if( item->IsOnLayer( F_Mask ) && !isNPTHPadWithNoCopper( item ) )
                {
                    // Test for aperture-to-aperture collisions
                    testItemAgainstItems( item, itemBBox, F_Mask, F_Mask );

                    // Test for aperture-to-zone collisions
                    testMaskItemAgainstZones( item, itemBBox, F_Mask, F_Cu );
                }
                else if( item->IsOnLayer( PADSTACK::ALL_LAYERS ) )
                {
                    // Test for copper-item-to-aperture collisions
                    testItemAgainstItems( item, itemBBox, F_Cu, F_Mask );
                }

                if( item->IsOnLayer( B_Mask ) && !isNPTHPadWithNoCopper( item ) )
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

                ++count;

                return true;
            } );

    for( auto& ret : returns )
    {
        if( !ret.valid() )
            continue;

        while( ret.wait_for( std::chrono::milliseconds( 100 ) ) == std::future_status::timeout )
            reportProgress( count, test_items.size() );
    }

    // Decide mask aperture bridges now that all threads have completed and the full set of items
    // exposed by each aperture is known, then emit every collected bridge in a deterministic order.
    reportMaskApertureBridges();
    flushBridgeViolations();
}


void DRC_TEST_PROVIDER_SOLDER_MASK::collectBridge( BOARD_ITEM* aItemA, BOARD_ITEM* aItemB,
                                                   BOARD_ITEM* aItemC, const VECTOR2I& aPos,
                                                   PCB_LAYER_ID aLayer )
{
    // Canonicalize the item order so the reported violation is identical no matter which worker
    // observed the collision.  The aperture (when present) stays first; the copper items are
    // ordered by UUID.
    if( !aItemC )
    {
        if( aItemB->m_Uuid < aItemA->m_Uuid )
            std::swap( aItemA, aItemB );
    }
    else if( aItemC->m_Uuid < aItemB->m_Uuid )
    {
        std::swap( aItemB, aItemC );
    }

    // Only the third item is ever absent, and the swaps above already ordered the copper items, so
    // the key needs at most the aperture inserted.  The unused slot stays trailing.
    PENDING_BRIDGE bridge = { aItemA, aItemB, aItemC, aPos, aLayer,
                              { aItemA->m_Uuid, aItemB->m_Uuid,
                                aItemC ? aItemC->m_Uuid : niluuid } };

    std::sort( bridge.ids.begin(), bridge.ids.end() - ( aItemC ? 0 : 1 ) );

    std::lock_guard<std::mutex> lock( m_bridgeMutex );

    if( static_cast<int>( m_bridgeViolations.size() ) == m_bridgeLimit )
    {
        // Anything sorting after the worst kept bridge can never reach the report.
        if( !( bridge < m_bridgeViolations.front() ) )
            return;

        std::pop_heap( m_bridgeViolations.begin(), m_bridgeViolations.end() );
        m_bridgeViolations.pop_back();
    }

    m_bridgeViolations.push_back( bridge );
    std::push_heap( m_bridgeViolations.begin(), m_bridgeViolations.end() );
}


void DRC_TEST_PROVIDER_SOLDER_MASK::flushBridgeViolations()
{
    // The collection is a max-heap holding at most the reportable count, so this puts it in
    // reporting order without ever having sorted the bridges the cap discards.
    std::sort_heap( m_bridgeViolations.begin(), m_bridgeViolations.end() );

    const wxString frontMsg = _( "Front solder mask aperture bridges items with different nets" );
    const wxString backMsg = _( "Rear solder mask aperture bridges items with different nets" );

    for( const PENDING_BRIDGE& bridge : m_bridgeViolations )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_SOLDERMASK_BRIDGE ) || m_drcEngine->IsCancelled() )
            break;

        std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_SOLDERMASK_BRIDGE );

        drce->SetErrorMessage( IsFrontLayer( bridge.layer ) ? frontMsg : backMsg );

        // SetItems skips null participants, so the two-item case needs no separate call.
        drce->SetItems( bridge.a, bridge.b, bridge.c );
        drce->SetViolatingRule( &m_bridgeRule );

        // Recompute the marker position from the two (UUID-ordered) copper items so it does not
        // depend on which worker observed the collision.  Query the copper layer, where every
        // participant (including zones) has a real shape.
        BOARD_ITEM*  itemX = bridge.c ? bridge.b : bridge.a;
        BOARD_ITEM*  itemY = bridge.c ? bridge.c : bridge.b;
        PCB_LAYER_ID copperLayer = IsFrontLayer( bridge.layer ) ? F_Cu : B_Cu;
        VECTOR2I     markerPos = itemX->GetPosition();

        std::shared_ptr<SHAPE> shapeX = itemX->GetEffectiveShape( copperLayer );
        std::shared_ptr<SHAPE> shapeY = itemY->GetEffectiveShape( copperLayer );
        VECTOR2I               ptX, ptY;

        if( shapeX->NearestPoints( shapeY.get(), ptX, ptY ) )
            markerPos = SEG( ptX, ptY ).Center();

        reportViolation( drce, markerPos, bridge.layer );
    }
}


void DRC_TEST_PROVIDER_SOLDER_MASK::reportMaskApertureBridges()
{
    // Visit apertures in a stable order (heap addresses and worker order are not reproducible).
    std::vector<PTR_LAYER_CACHE_KEY> apertureKeys;
    apertureKeys.reserve( m_maskApertureItems.size() );

    for( const auto& [key, items] : m_maskApertureItems )
        apertureKeys.push_back( key );

    std::sort( apertureKeys.begin(), apertureKeys.end(),
            []( const PTR_LAYER_CACHE_KEY& a, const PTR_LAYER_CACHE_KEY& b ) -> bool
            {
                if( a.A->m_Uuid != b.A->m_Uuid )
                    return a.A->m_Uuid < b.A->m_Uuid;

                return a.Layer < b.Layer;
            } );

    for( const PTR_LAYER_CACHE_KEY& key : apertureKeys )
    {
        // Pairing is quadratic in the items an aperture exposes, so stay interruptible.
        if( m_drcEngine->IsCancelled() )
            return;

        BOARD_ITEM*  aperture = key.A;
        PCB_LAYER_ID maskLayer = key.Layer;

        // Built once per aperture rather than per candidate pair; the map covers every pad in the
        // footprint and the pairing below is quadratic.
        FOOTPRINT*              apertureFootprint = aperture->GetParentFootprint();
        std::map<wxString, int> netTieGroups;

        if( apertureFootprint )
            netTieGroups = apertureFootprint->MapPadNumbersToNetTieGroups();

        std::vector<MASK_APERTURE_ITEM>& items = m_maskApertureItems.at( key );

        std::sort( items.begin(), items.end(),
                []( const MASK_APERTURE_ITEM& a, const MASK_APERTURE_ITEM& b ) -> bool
                {
                    if( a.item->m_Uuid != b.item->m_Uuid )
                        return a.item->m_Uuid < b.item->m_Uuid;

                    return a.net < b.net;
                } );

        // The first positive net exposed by the aperture is the reference net; any item on a
        // different positive net bridges it.
        BOARD_ITEM* refItem = nullptr;
        int         refNet = -1;

        for( const MASK_APERTURE_ITEM& entry : items )
        {
            if( entry.net > 0 )
            {
                refItem = entry.item;
                refNet = entry.net;
                break;
            }
        }

        if( !refItem )
            continue;

        const bool reportAllTracks = m_drcEngine->GetReportAllTrackErrors();

        std::unordered_set<PTR_PTR_CACHE_KEY> reportedPairs;

        for( const MASK_APERTURE_ITEM& collision : items )
        {
            if( m_drcEngine->IsCancelled() )
                return;

            if( collision.net == refNet )
                continue;

            // Footprint-local exclusions need the colliding item inside the aperture's footprint,
            // which is invariant across the inner loop.
            bool collisionInFootprint = apertureFootprint
                                        && collision.item->GetParentFootprint() == apertureFootprint;

            if( collisionInFootprint
                    && maskApertureBridgeExcluded( apertureFootprint, netTieGroups, refItem,
                                                   collision.item ) )
            {
                continue;
            }

            bool reportedAnyTrack = false;

            for( const MASK_APERTURE_ITEM& entry : items )
            {
                if( entry.net == collision.net )
                    continue;

                if( collisionInFootprint
                        && maskApertureBridgeExcluded( apertureFootprint, netTieGroups, entry.item,
                                                       collision.item ) )
                {
                    continue;
                }

                bool entryIsTrack = entry.item->Type() == PCB_TRACE_T || entry.item->Type() == PCB_ARC_T;

                // Track throttling is per-collision, so it must gate emission before the pair is
                // marked reported.  Marking a suppressed pair here would drop it for good under a
                // later collision ordering, making the total count depend on the reference net.
                if( entryIsTrack && reportedAnyTrack && !reportAllTracks )
                    continue;

                // Deduplicate unordered pairs so (B, C) and (C, B) are reported once.  std::less
                // gives a total order over unrelated pointers, which bare < does not.
                BOARD_ITEM* lo = entry.item;
                BOARD_ITEM* hi = collision.item;

                if( std::less<BOARD_ITEM*>{}( hi, lo ) )
                    std::swap( lo, hi );

                if( !reportedPairs.insert( { lo, hi } ).second )
                    continue;

                collectBridge( aperture, entry.item, collision.item, collision.pos, maskLayer );

                if( entryIsTrack )
                    reportedAnyTrack = true;
            }
        }
    }
}


bool DRC_TEST_PROVIDER_SOLDER_MASK::Run()
{
    if( m_drcEngine->IsErrorLimitExceeded( DRCE_SILK_MASK_CLEARANCE )
            && m_drcEngine->IsErrorLimitExceeded( DRCE_SOLDERMASK_BRIDGE ) )
    {
        REPORT_AUX( wxT( "Solder mask violations ignored. Tests not run." ) );
        return true;    // continue with other tests
    }

    m_board = m_drcEngine->GetBoard();
    m_webWidth = m_board->GetDesignSettings().m_SolderMaskMinWidth;
    m_maxError = m_board->GetDesignSettings().m_MaxError;
    m_largestClearance = 0;

    auto updateLargestClearance =
            [&]( int aClearance )
            {
                m_largestClearance = std::max( m_largestClearance, aClearance );
            };

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
            updateLargestClearance( pad->GetSolderMaskExpansion( PADSTACK::ALL_LAYERS ) );

        for( BOARD_ITEM* item : footprint->GraphicalItems() )
        {
            if( item->Type() == PCB_SHAPE_T )
                updateLargestClearance( static_cast<PCB_SHAPE*>( item )->GetSolderMaskExpansion() );
        }
    }

    for( PCB_TRACK* track : m_board->Tracks() )
        updateLargestClearance( track->GetSolderMaskExpansion() );

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        if( item->Type() == PCB_SHAPE_T )
            updateLargestClearance( static_cast<PCB_SHAPE*>( item )->GetSolderMaskExpansion() );
    }

    // Order is important here: m_webWidth must be added in before m_largestClearance is
    // maxed with the various clearance constraints.
    m_largestClearance += m_largestClearance + m_webWidth;

    // Include SolderMaskToCopperClearance so R-tree queries find copper items that are within
    // the required distance of mask apertures. Without this, tracks passing near pad apertures
    // from different nets would not be found if SolderMaskToCopperClearance > m_largestClearance.
    m_largestClearance = std::max( m_largestClearance,
                                   m_board->GetDesignSettings().m_SolderMaskToCopperClearance );

    DRC_CONSTRAINT worstClearanceConstraint;

    if( m_drcEngine->QueryWorstConstraint( SILK_CLEARANCE_CONSTRAINT, worstClearanceConstraint ) )
        m_largestClearance = std::max( m_largestClearance, worstClearanceConstraint.m_Value.Min() );

    if( !reportPhase( _( "Building solder mask..." ) ) )
        return false;   // DRC cancelled

    m_checkedPairs.clear();
    m_maskApertureItems.clear();
    m_bridgeViolations.clear();

    // Snapshotting the cap keeps the bridge collection bounded without contending on the engine's
    // error-limit mutex from the worker threads.  Nothing else reports DRCE_SOLDERMASK_BRIDGE, so
    // the budget cannot shrink under us.
    m_bridgeLimit = m_drcEngine->GetErrorLimit( DRCE_SOLDERMASK_BRIDGE );
    m_bridgeViolations.reserve( m_bridgeLimit );

    buildRTrees();

    if( !reportPhase( _( "Checking solder mask to silk clearance..." ) ) )
        return false;   // DRC cancelled

    testSilkToMaskClearance();

    if( m_bridgeLimit > 0 )
    {
        if( !reportPhase( _( "Checking solder mask web integrity..." ) ) )
            return false;   // DRC cancelled

        testMaskBridges();
    }

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_SOLDER_MASK> dummy;
}
