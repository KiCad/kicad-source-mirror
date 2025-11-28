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
#include <board_design_settings.h>
#include <board_connected_item.h>
#include <footprint.h>
#include <pad.h>
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

/*
    Solder mask tests. Checks for silkscreen which is clipped by mask openings and for bridges
    between mask apertures with different nets.
    Errors generated:
    - DRCE_SILK_MASK_CLEARANCE
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

    bool checkMaskAperture( BOARD_ITEM* aMaskItem, BOARD_ITEM* aTestItem, PCB_LAYER_ID aTestLayer,
                            int aTestNet, BOARD_ITEM** aCollidingItem );

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

    // Shapes used to define solder mask apertures don't have nets, so we assign them the
    // first object+net that bridges their aperture (after which any other nets will generate
    // violations).
    std::mutex                                                           m_netMapMutex;
    std::unordered_map<PTR_LAYER_CACHE_KEY, std::pair<BOARD_ITEM*, int>> m_maskApertureNetMap;
};


void DRC_TEST_PROVIDER_SOLDER_MASK::addItemToRTrees( BOARD_ITEM* aItem )
{
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
            else if( aItem->Type() == PCB_SHAPE_T )
                clearance += static_cast<PCB_SHAPE*>( aItem )->GetSolderMaskExpansion();

            if( aItem->Type() == PCB_FIELD_T || aItem->Type() == PCB_TEXT_T )
            {
                PCB_TEXT* text = static_cast<PCB_TEXT*>( aItem );

                text->TransformTextToPolySet( *solderMask, clearance, m_maxError, ERROR_OUTSIDE );
            }
            else
            {
                aItem->TransformShapeToPolygon( *solderMask, layer, clearance, m_maxError, ERROR_OUTSIDE );
            }

            m_itemTree->Insert( aItem, layer, m_largestClearance );
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

    m_fullSolderMaskRTree->Insert( solderMask, F_Mask );
    m_fullSolderMaskRTree->Insert( solderMask, B_Mask );

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


bool isNullAperture( BOARD_ITEM* aItem )
{
    if( aItem->Type() == PCB_PAD_T )
    {
        PAD* pad = static_cast<PAD*>( aItem );

        // TODO(JE) padstacks
        if( pad->GetAttribute() == PAD_ATTRIB::NPTH
                && ( pad->GetShape( PADSTACK::ALL_LAYERS ) == PAD_SHAPE::CIRCLE
                     || pad->GetShape( PADSTACK::ALL_LAYERS ) == PAD_SHAPE::OVAL )
                && pad->GetSize( PADSTACK::ALL_LAYERS ).x <= pad->GetDrillSize().x
                && pad->GetSize( PADSTACK::ALL_LAYERS ).y <= pad->GetDrillSize().y )
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

    static const LSET saved( { F_Mask, B_Mask } );

    LSET maskLayers = aItem->GetLayerSet() & saved;
    LSET copperLayers = ( aItem->GetLayerSet() & ~saved ) & LSET::AllCuMask();

    return maskLayers.count() > 0 && copperLayers.count() == 0;
}


bool DRC_TEST_PROVIDER_SOLDER_MASK::checkMaskAperture( BOARD_ITEM* aMaskItem, BOARD_ITEM* aTestItem,
                                                       PCB_LAYER_ID aTestLayer, int aTestNet,
                                                       BOARD_ITEM** aCollidingItem )
{
    if( aTestLayer == F_Mask && !aTestItem->IsOnLayer( F_Cu ) )
        return false;

    if( aTestLayer == B_Mask && !aTestItem->IsOnLayer( B_Cu ) )
        return false;

    PCB_LAYER_ID maskLayer = IsFrontLayer( aTestLayer ) ? F_Mask : B_Mask;

    FOOTPRINT* fp = aMaskItem->GetParentFootprint();

    // Mask apertures in footprints which allow soldermask bridges are ignored entirely.
    if( fp && fp->AllowSolderMaskBridges() )
        return false;

    PTR_LAYER_CACHE_KEY key = { aMaskItem, maskLayer };
    BOARD_ITEM*         alreadyEncounteredItem = nullptr;
    int                 encounteredItemNet = -1;

    {
        std::lock_guard<std::mutex> lock( m_checkedPairsMutex );
        auto ii = m_maskApertureNetMap.find( key );

        if( ii == m_maskApertureNetMap.end() )
        {
            m_maskApertureNetMap[ key ] = { aTestItem, aTestNet };

            // First net; no bridge yet....
            return false;
        }

        alreadyEncounteredItem = ii->second.first;
        encounteredItemNet = ii->second.second;
    }

    if( encounteredItemNet == aTestNet && aTestNet >= 0 )
    {
        // Same net; still no bridge...
        return false;
    }

    if( fp && aTestItem->GetParentFootprint() == fp )
    {
        std::map<wxString, int> padToNetTieGroupMap = fp->MapPadNumbersToNetTieGroups();
        PAD*                    padA = nullptr;
        PAD*                    padB = nullptr;

        if( alreadyEncounteredItem->Type() == PCB_PAD_T )
            padA = static_cast<PAD*>( alreadyEncounteredItem );

        if( aTestItem->Type() == PCB_PAD_T )
            padB = static_cast<PAD*>( aTestItem );

        if( padA && padB && ( padA->SameLogicalPadAs( padB ) || padA->SharesNetTieGroup( padB ) ) )
        {
            return false;
        }
        else if( padA && aTestItem->Type() == PCB_SHAPE_T )
        {
            if( padToNetTieGroupMap.contains( padA->GetNumber() ) )
                return false;
        }
        else if( padB && alreadyEncounteredItem->Type() == PCB_SHAPE_T )
        {
            if( padToNetTieGroupMap.contains( padB->GetNumber() ) )
                return false;
        }
    }

    *aCollidingItem = alreadyEncounteredItem;
    return true;
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

                if( isNullAperture( other ) )
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
                            std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_SOLDERMASK_BRIDGE );

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
                            std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_SOLDERMASK_BRIDGE );

                            drce->SetErrorMessage( msg );
                            drce->SetItems( other, colliding, aItem );
                            drce->SetViolatingRule( &m_bridgeRule );
                            reportViolation( drce, pos, aTargetLayer );
                        }
                    }
                    else if( checkItemMask( other, itemNet ) )
                    {
                        std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_SOLDERMASK_BRIDGE );

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
                    std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_SOLDERMASK_BRIDGE );

                    drce->SetErrorMessage( msg );
                    drce->SetItems( aItem, colliding, zone );
                    drce->SetViolatingRule( &m_bridgeRule );
                    reportViolation( drce, pos, aTargetLayer );
                }
            }
            else
            {
                std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_SOLDERMASK_BRIDGE );

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

                if( m_drcEngine->IsErrorLimitExceeded( DRCE_SOLDERMASK_BRIDGE ) )
                    return false;

                BOX2I itemBBox = item->GetBoundingBox();

                if( item->IsOnLayer( F_Mask ) && !isNullAperture( item ) )
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

    // Order is important here: m_webWidth must be added in before m_largestCourtyardClearance is
    // maxed with the various SILK_CLEARANCE_CONSTRAINTS.
    m_largestClearance += m_largestClearance + m_webWidth;

    DRC_CONSTRAINT worstClearanceConstraint;

    if( m_drcEngine->QueryWorstConstraint( SILK_CLEARANCE_CONSTRAINT, worstClearanceConstraint ) )
        m_largestClearance = std::max( m_largestClearance, worstClearanceConstraint.m_Value.Min() );

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

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_SOLDER_MASK> dummy;
}
