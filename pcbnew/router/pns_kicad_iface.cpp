/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <board.h>
#include <board_connected_item.h>
#include <board_design_settings.h>
#include <netinfo.h>
#include <footprint.h>
#include <layer_range.h>
#include <length_delay_calculation/length_delay_calculation.h>
#include <pad.h>
#include <pcb_track.h>
#include <zone.h>
#include <pcb_shape.h>
#include <pcb_generator.h>
#include <pcb_text.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <board_commit.h>
#include <layer_ids.h>
#include <kidialog.h>
#include <tools/pcb_tool_base.h>
#include <tool/tool_manager.h>
#include <settings/app_settings.h>

#include <gal/graphics_abstraction_layer.h>
#include <pcb_painter.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_simple.h>

#include <drc/drc_rule.h>
#include <drc/drc_engine.h>

#include <connectivity/connectivity_data.h>

#include <wx/log.h>

#include <memory>

#include <advanced_config.h>
#include <pcbnew_settings.h>
#include <macros.h>

#include "pns_kicad_iface.h"
#include "pns_arc.h"
#include "pns_sizes_settings.h"
#include "pns_item.h"
#include "pns_layerset.h"
#include "pns_line.h"
#include "pns_solid.h"
#include "pns_segment.h"
#include "pns_node.h"
#include "pns_router.h"
#include "pns_debug_decorator.h"
#include "router_preview_item.h"

typedef VECTOR2I::extended_type ecoord;


struct CLEARANCE_CACHE_KEY
{
    const PNS::ITEM*  A;
    const PNS::ITEM*  B;
    bool              Flag;

    bool operator==(const CLEARANCE_CACHE_KEY& other) const
    {
        return A == other.A && B == other.B && Flag == other.Flag;
    }
};

namespace std
{
    template <>
    struct hash<CLEARANCE_CACHE_KEY>
    {
        std::size_t operator()( const CLEARANCE_CACHE_KEY& k ) const
        {
            size_t retval = 0xBADC0FFEE0DDF00D;
            hash_combine( retval, hash<const void*>()( k.A ), hash<const void*>()( k.B ), hash<int>()( k.Flag ) );
            return retval;
        }
    };
}


class PNS_PCBNEW_RULE_RESOLVER : public PNS::RULE_RESOLVER
{
public:
    PNS_PCBNEW_RULE_RESOLVER( BOARD* aBoard, PNS::ROUTER_IFACE* aRouterIface );
    virtual ~PNS_PCBNEW_RULE_RESOLVER();

    int Clearance( const PNS::ITEM* aA, const PNS::ITEM* aB,
                   bool aUseClearanceEpsilon = true ) override;

    PNS::NET_HANDLE DpCoupledNet( PNS::NET_HANDLE aNet ) override;
    int DpNetPolarity( PNS::NET_HANDLE aNet ) override;
    bool DpNetPair( const PNS::ITEM* aItem, PNS::NET_HANDLE& aNetP,
                    PNS::NET_HANDLE& aNetN ) override;

    int NetCode( PNS::NET_HANDLE aNet ) override;
    wxString NetName( PNS::NET_HANDLE aNet ) override;

    bool IsInNetTie( const PNS::ITEM* aA ) override;
    bool IsNetTieExclusion( const PNS::ITEM* aItem, const VECTOR2I& aCollisionPos,
                            const PNS::ITEM* aCollidingItem ) override;

    bool IsDrilledHole( const PNS::ITEM* aItem ) override;
    bool IsNonPlatedSlot( const PNS::ITEM* aItem ) override;

    /**
     * @return true if \a aObstacle is a keepout.  Set \a aEnforce if said keepout's rules
     *         exclude \a aItem.
     */
    bool IsKeepout( const PNS::ITEM* aObstacle, const PNS::ITEM* aItem, bool* aEnforce ) override;

    bool QueryConstraint( PNS::CONSTRAINT_TYPE aType, const PNS::ITEM* aItemA,
                          const PNS::ITEM* aItemB, int aLayer,
                          PNS::CONSTRAINT* aConstraint ) override;

    int ClearanceEpsilon() const override { return m_clearanceEpsilon; }

    void ClearCacheForItems( std::vector<const PNS::ITEM*>& aItems ) override;
    void ClearCaches() override;
    void ClearTemporaryCaches() override;

private:
    BOARD_ITEM* getBoardItem( const PNS::ITEM* aItem, PCB_LAYER_ID aBoardLayer, int aIdx = 0 );

private:
    PNS::ROUTER_IFACE* m_routerIface;
    BOARD*             m_board;
    PCB_TRACK          m_dummyTracks[2];
    PCB_ARC            m_dummyArcs[2];
    PCB_VIA            m_dummyVias[2];
    int                m_clearanceEpsilon;

    std::unordered_map<CLEARANCE_CACHE_KEY, int> m_clearanceCache;
    std::unordered_map<CLEARANCE_CACHE_KEY, int> m_tempClearanceCache;
};


PNS_PCBNEW_RULE_RESOLVER::PNS_PCBNEW_RULE_RESOLVER( BOARD* aBoard,
                                                    PNS::ROUTER_IFACE* aRouterIface ) :
    m_routerIface( aRouterIface ),
    m_board( aBoard ),
    m_dummyTracks{ { aBoard }, { aBoard } },
    m_dummyArcs{ { aBoard }, { aBoard } },
    m_dummyVias{ { aBoard }, { aBoard } }
{
    for( PCB_TRACK& track : m_dummyTracks )
        track.SetFlags( ROUTER_TRANSIENT );

    for( PCB_ARC& arc : m_dummyArcs )
        arc.SetFlags( ROUTER_TRANSIENT );

    for ( PCB_VIA& via : m_dummyVias )
        via.SetFlags( ROUTER_TRANSIENT );

    if( aBoard )
        m_clearanceEpsilon = aBoard->GetDesignSettings().GetDRCEpsilon();
    else
        m_clearanceEpsilon = 0;
}


PNS_PCBNEW_RULE_RESOLVER::~PNS_PCBNEW_RULE_RESOLVER()
{
}


bool PNS_PCBNEW_RULE_RESOLVER::IsInNetTie( const PNS::ITEM* aA )
{
    BOARD_ITEM* item = aA->BoardItem();

    return item && item->GetParentFootprint() && item->GetParentFootprint()->IsNetTie();
}


bool PNS_PCBNEW_RULE_RESOLVER::IsNetTieExclusion( const PNS::ITEM* aItem,
                                                  const VECTOR2I& aCollisionPos,
                                                  const PNS::ITEM* aCollidingItem )
{
    if( !aItem || !aCollidingItem )
        return false;

    std::shared_ptr<DRC_ENGINE> drcEngine = m_board->GetDesignSettings().m_DRCEngine;
    BOARD_ITEM*                 item = aItem->BoardItem();
    BOARD_ITEM*                 collidingItem = aCollidingItem->BoardItem();

    FOOTPRINT* collidingFp = collidingItem->GetParentFootprint();
    FOOTPRINT* itemFp      = item ? item->GetParentFootprint() : nullptr;

    if( collidingFp && itemFp && ( collidingFp == itemFp ) && itemFp->IsNetTie() )
    {
        // Two items colliding from the same net tie footprint are not checked
        return true;
    }

    if( drcEngine )
    {
        return drcEngine->IsNetTieExclusion( NetCode( aItem->Net() ),
                                             m_routerIface->GetBoardLayerFromPNSLayer( aItem->Layer() ),
                                             aCollisionPos, collidingItem );
    }

    return false;
}


bool PNS_PCBNEW_RULE_RESOLVER::IsKeepout( const PNS::ITEM* aObstacle, const PNS::ITEM* aItem,
                                          bool* aEnforce )
{
    auto checkKeepout =
            []( const ZONE* aKeepout, const BOARD_ITEM* aOther )
            {
                if( !aOther )
                    return false;

                if( aKeepout->GetDoNotAllowTracks() && aOther->IsType( { PCB_ARC_T, PCB_TRACE_T } ) )
                    return true;

                if( aKeepout->GetDoNotAllowVias() && aOther->Type() == PCB_VIA_T )
                    return true;

                if( aKeepout->GetDoNotAllowPads() && aOther->Type() == PCB_PAD_T )
                    return true;

                // Incomplete test, but better than nothing:
                if( aKeepout->GetDoNotAllowFootprints() && aOther->Type() == PCB_PAD_T )
                {
                    return !aKeepout->GetParentFootprint()
                            || aKeepout->GetParentFootprint() != aOther->GetParentFootprint();
                }

                return false;
            };

    if( aObstacle->Parent() && aObstacle->Parent()->Type() == PCB_ZONE_T )
    {
        const ZONE* zone = static_cast<ZONE*>( aObstacle->Parent() );

        if( zone->GetIsRuleArea() && zone->HasKeepoutParametersSet() )
        {
            *aEnforce = checkKeepout( zone,
                                      getBoardItem( aItem, m_routerIface->GetBoardLayerFromPNSLayer(
                                                                   aObstacle->Layer() ) ) );
            return true;
        }
    }

    return false;
}


static bool isCopper( const PNS::ITEM* aItem )
{
    if ( !aItem )
        return false;

    const BOARD_ITEM *parent = aItem->Parent();

    return !parent || parent->IsOnCopperLayer();
}


static bool isHole( const PNS::ITEM* aItem )
{
    if ( !aItem )
        return false;

    return aItem->OfKind( PNS::ITEM::HOLE_T );
}


static bool isEdge( const PNS::ITEM* aItem )
{
    if ( !aItem )
        return false;

    const PCB_SHAPE *parent = dynamic_cast<PCB_SHAPE*>( aItem->BoardItem() );

    return parent && ( parent->IsOnLayer( Edge_Cuts ) || parent->IsOnLayer( Margin ) );
}


bool PNS_PCBNEW_RULE_RESOLVER::IsDrilledHole( const PNS::ITEM* aItem )
{
    if( !isHole( aItem ) )
        return false;

    BOARD_ITEM* parent = aItem->Parent();

    if( !parent && aItem->ParentPadVia() )
        parent = aItem->ParentPadVia()->Parent();

    return parent && parent->HasDrilledHole();
}


bool PNS_PCBNEW_RULE_RESOLVER::IsNonPlatedSlot( const PNS::ITEM* aItem )
{
    if( !isHole( aItem ) )
        return false;

    BOARD_ITEM* parent = aItem->Parent();

    if( !parent && aItem->ParentPadVia() )
        parent = aItem->ParentPadVia()->Parent();

    if( parent )
    {
        if( parent->Type() == PCB_PAD_T )
        {
            PAD* pad = static_cast<PAD*>( parent );

            return pad->GetAttribute() == PAD_ATTRIB::NPTH
                        && pad->GetDrillSizeX() != pad->GetDrillSizeY();
        }

        // Via holes are (currently) always round, and always plated
    }

    return false;
}


BOARD_ITEM* PNS_PCBNEW_RULE_RESOLVER::getBoardItem( const PNS::ITEM* aItem, PCB_LAYER_ID aBoardLayer, int aIdx )
{
    switch( aItem->Kind() )
    {
    case PNS::ITEM::ARC_T:
        m_dummyArcs[aIdx].SetLayer( aBoardLayer );
        m_dummyArcs[aIdx].SetNet( static_cast<NETINFO_ITEM*>( aItem->Net() ) );
        m_dummyArcs[aIdx].SetStart( aItem->Anchor( 0 ) );
        m_dummyArcs[aIdx].SetEnd( aItem->Anchor( 1 ) );
        return &m_dummyArcs[aIdx];

    case PNS::ITEM::VIA_T:
    case PNS::ITEM::HOLE_T:
        m_dummyVias[aIdx].SetLayer( aBoardLayer );
        m_dummyVias[aIdx].SetNet( static_cast<NETINFO_ITEM*>( aItem->Net() ) );
        m_dummyVias[aIdx].SetStart( aItem->Anchor( 0 ) );
        return &m_dummyVias[aIdx];

    case PNS::ITEM::SEGMENT_T:
    case PNS::ITEM::LINE_T:
        m_dummyTracks[aIdx].SetLayer( aBoardLayer );
        m_dummyTracks[aIdx].SetNet( static_cast<NETINFO_ITEM*>( aItem->Net() ) );
        m_dummyTracks[aIdx].SetStart( aItem->Anchor( 0 ) );
        m_dummyTracks[aIdx].SetEnd( aItem->Anchor( 1 ) );
        return &m_dummyTracks[aIdx];

    default:
        return nullptr;
    }
}


bool PNS_PCBNEW_RULE_RESOLVER::QueryConstraint( PNS::CONSTRAINT_TYPE aType,
                                                const PNS::ITEM* aItemA, const PNS::ITEM* aItemB,
                                                int aPNSLayer, PNS::CONSTRAINT* aConstraint )
{
    std::shared_ptr<DRC_ENGINE> drcEngine = m_board->GetDesignSettings().m_DRCEngine;

    if( !drcEngine )
        return false;

    DRC_CONSTRAINT_T hostType;

    switch ( aType )
    {
    case PNS::CONSTRAINT_TYPE::CT_CLEARANCE:          hostType = CLEARANCE_CONSTRAINT;          break;
    case PNS::CONSTRAINT_TYPE::CT_WIDTH:              hostType = TRACK_WIDTH_CONSTRAINT;        break;
    case PNS::CONSTRAINT_TYPE::CT_DIFF_PAIR_GAP:      hostType = DIFF_PAIR_GAP_CONSTRAINT;      break;
    case PNS::CONSTRAINT_TYPE::CT_LENGTH:             hostType = LENGTH_CONSTRAINT;             break;
    case PNS::CONSTRAINT_TYPE::CT_DIFF_PAIR_SKEW:     hostType = SKEW_CONSTRAINT;               break;
    case PNS::CONSTRAINT_TYPE::CT_MAX_UNCOUPLED:      hostType = MAX_UNCOUPLED_CONSTRAINT;      break;
    case PNS::CONSTRAINT_TYPE::CT_VIA_DIAMETER:       hostType = VIA_DIAMETER_CONSTRAINT;       break;
    case PNS::CONSTRAINT_TYPE::CT_VIA_HOLE:           hostType = HOLE_SIZE_CONSTRAINT;          break;
    case PNS::CONSTRAINT_TYPE::CT_HOLE_CLEARANCE:     hostType = HOLE_CLEARANCE_CONSTRAINT;     break;
    case PNS::CONSTRAINT_TYPE::CT_EDGE_CLEARANCE:     hostType = EDGE_CLEARANCE_CONSTRAINT;     break;
    case PNS::CONSTRAINT_TYPE::CT_HOLE_TO_HOLE:       hostType = HOLE_TO_HOLE_CONSTRAINT;       break;
    case PNS::CONSTRAINT_TYPE::CT_PHYSICAL_CLEARANCE: hostType = PHYSICAL_CLEARANCE_CONSTRAINT; break;
    default:                                          return false; // should not happen
    }

    BOARD_ITEM*    parentA = aItemA ? aItemA->BoardItem() : nullptr;
    BOARD_ITEM*    parentB = aItemB ? aItemB->BoardItem() : nullptr;
    PCB_LAYER_ID   board_layer = m_routerIface->GetBoardLayerFromPNSLayer( aPNSLayer );
    DRC_CONSTRAINT hostConstraint;

    // A track being routed may not have a BOARD_ITEM associated yet.
    if( aItemA && !parentA )
        parentA = getBoardItem( aItemA, board_layer, 0 );

    if( aItemB && !parentB )
        parentB = getBoardItem( aItemB, board_layer, 1 );

    if( parentA )
        hostConstraint = drcEngine->EvalRules( hostType, parentA, parentB, board_layer );

    if( hostConstraint.IsNull() )
        return false;

    if( hostConstraint.GetSeverity() == RPT_SEVERITY_IGNORE )
    {
        aConstraint->m_Value.SetMin( -1 );
        aConstraint->m_RuleName = hostConstraint.GetName();
        aConstraint->m_Type = aType;
        return true;
    }

    switch ( aType )
    {
        case PNS::CONSTRAINT_TYPE::CT_CLEARANCE:
        case PNS::CONSTRAINT_TYPE::CT_WIDTH:
        case PNS::CONSTRAINT_TYPE::CT_DIFF_PAIR_GAP:
        case PNS::CONSTRAINT_TYPE::CT_VIA_DIAMETER:
        case PNS::CONSTRAINT_TYPE::CT_VIA_HOLE:
        case PNS::CONSTRAINT_TYPE::CT_HOLE_CLEARANCE:
        case PNS::CONSTRAINT_TYPE::CT_EDGE_CLEARANCE:
        case PNS::CONSTRAINT_TYPE::CT_HOLE_TO_HOLE:
        case PNS::CONSTRAINT_TYPE::CT_LENGTH:
        case PNS::CONSTRAINT_TYPE::CT_DIFF_PAIR_SKEW:
        case PNS::CONSTRAINT_TYPE::CT_MAX_UNCOUPLED:
        case PNS::CONSTRAINT_TYPE::CT_PHYSICAL_CLEARANCE:
            aConstraint->m_Value = hostConstraint.GetValue();
            aConstraint->m_RuleName = hostConstraint.GetName();
            aConstraint->m_Type = aType;
            aConstraint->m_IsTimeDomain = hostConstraint.GetOption( DRC_CONSTRAINT::OPTIONS::TIME_DOMAIN );
            return true;

        default:
            return false;
    }
}


void PNS_PCBNEW_RULE_RESOLVER::ClearCacheForItems( std::vector<const PNS::ITEM*>& aItems )
{
    int n_pruned = 0;
    std::set<const PNS::ITEM*> remainingItems( aItems.begin(), aItems.end() );

/* We need to carefully check both A and B item pointers in the cache against dirty/invalidated
   items in the set, as the clearance relation is commutative ( CL[a,b] == CL[b,a] ). The code
   below is a bit ugly, but works in O(n*log(m)) and is run once or twice during ROUTER::Move() call
   - so I hope it still gets better performance than no cache at all */
    for( auto it = m_clearanceCache.begin(); it != m_clearanceCache.end(); )
    {
        bool dirty = remainingItems.find( it->first.A ) != remainingItems.end();
        dirty |= remainingItems.find( it->first.B) != remainingItems.end();

        if( dirty )
        {
            it = m_clearanceCache.erase( it );
            n_pruned++;
        } else
            it++;
    }
#if 0
    printf("ClearCache : n_pruned %d\n", n_pruned );
#endif
}


void PNS_PCBNEW_RULE_RESOLVER::ClearCaches()
{
    m_clearanceCache.clear();
    m_tempClearanceCache.clear();
}


void PNS_PCBNEW_RULE_RESOLVER::ClearTemporaryCaches()
{
    m_tempClearanceCache.clear();
}


int PNS_PCBNEW_RULE_RESOLVER::Clearance( const PNS::ITEM* aA, const PNS::ITEM* aB,
                                         bool aUseClearanceEpsilon )
{
    CLEARANCE_CACHE_KEY key = { aA, aB, aUseClearanceEpsilon };

    // Search cache (used for actual board items)
    auto it = m_clearanceCache.find( key );

    if( it != m_clearanceCache.end() )
        return it->second;

    // Search cache (used for temporary items within an algorithm)
    it = m_tempClearanceCache.find( key );

    if( it != m_tempClearanceCache.end() )
        return it->second;

    PNS::CONSTRAINT constraint;
    int             rv = 0;
    PNS_LAYER_RANGE     layers;

    if( !aB )
        layers = aA->Layers();
    else if( isEdge( aA ) )
        layers = aB->Layers();
    else if( isEdge( aB ) )
        layers = aA->Layers();
    else
        layers = aA->Layers().Intersection( aB->Layers() );

    // Normalize layer range (no -1 magic numbers)
    layers = layers.Intersection( PNS_LAYER_RANGE( PCBNEW_LAYER_ID_START, PCB_LAYER_ID_COUNT - 1 ) );

    for( int layer = layers.Start(); layer <= layers.End(); ++layer )
    {
        if( IsDrilledHole( aA ) && IsDrilledHole( aB ) )
        {
            if( QueryConstraint( PNS::CONSTRAINT_TYPE::CT_HOLE_TO_HOLE, aA, aB, layer, &constraint ) )
            {
                if( constraint.m_Value.Min() > rv )
                    rv = constraint.m_Value.Min();
            }
        }
        else if( isHole( aA ) || isHole( aB ) )
        {
            if( QueryConstraint( PNS::CONSTRAINT_TYPE::CT_HOLE_CLEARANCE, aA, aB, layer, &constraint ) )
            {
                if( constraint.m_Value.Min() > rv )
                    rv = constraint.m_Value.Min();
            }
        }

        // No 'else'; plated holes get both HOLE_CLEARANCE and CLEARANCE
        if( isCopper( aA ) && ( !aB || isCopper( aB ) ) )
        {
            if( QueryConstraint( PNS::CONSTRAINT_TYPE::CT_CLEARANCE, aA, aB, layer, &constraint ) )
            {
                if( constraint.m_Value.Min() > rv )
                    rv = constraint.m_Value.Min();
            }
        }

        // No 'else'; non-plated milled holes get both HOLE_CLEARANCE and EDGE_CLEARANCE
        if( isEdge( aA ) || IsNonPlatedSlot( aA ) || isEdge( aB ) || IsNonPlatedSlot( aB ) )
        {
            if( QueryConstraint( PNS::CONSTRAINT_TYPE::CT_EDGE_CLEARANCE, aA, aB, layer, &constraint ) )
            {
                if( constraint.m_Value.Min() > rv )
                    rv = constraint.m_Value.Min();
            }
        }

        if( QueryConstraint( PNS::CONSTRAINT_TYPE::CT_PHYSICAL_CLEARANCE, aA, aB, layer, &constraint ) )
        {
            if( constraint.m_Value.Min() > rv )
                rv = constraint.m_Value.Min();
        }
    }

    if( aUseClearanceEpsilon && rv > 0 )
        rv = std::max( 0, rv - m_clearanceEpsilon );

    /*
     * It makes no sense to put items that have no owning NODE in the cache - they can be
     * allocated on stack and we can't really invalidate them in the cache when they are
     * destroyed.  Probably a better idea would be to use a static unique counter in PNS::ITEM
     * constructor to generate the cache keys.
     *
     * However, algorithms DO greatly benefit from using the cache, so ownerless items need to be
     * cached.  In order to easily clear those only, a temporary cache is created. If this doesn't
     * seem nice, an alternative is clearing the full cache once it reaches a certain size. Also
     * not pretty, but VERY effective to keep things interactive.
     */
    if( aA && aB )
    {
        if ( aA->Owner() && aB->Owner() )
            m_clearanceCache[ key ] = rv;
        else
            m_tempClearanceCache[ key ] = rv;
    }

    return rv;
}


bool PNS_KICAD_IFACE_BASE::inheritTrackWidth( PNS::ITEM* aItem, int* aInheritedWidth )
{
    VECTOR2I p;

    assert( aItem->Owner() != nullptr );

    auto tryGetTrackWidth =
            []( PNS::ITEM* aPnsItem ) -> int
            {
                switch( aPnsItem->Kind() )
                {
                case PNS::ITEM::SEGMENT_T: return static_cast<PNS::SEGMENT*>( aPnsItem )->Width();
                case PNS::ITEM::ARC_T:     return static_cast<PNS::ARC*>( aPnsItem )->Width();
                default:                   return -1;
                }
            };

    int itemTrackWidth = tryGetTrackWidth( aItem );

    if( itemTrackWidth > 0 )
    {
        *aInheritedWidth = itemTrackWidth;
        return true;
    }

    switch( aItem->Kind() )
    {
    case PNS::ITEM::VIA_T:   p = static_cast<PNS::VIA*>( aItem )->Pos();   break;
    case PNS::ITEM::SOLID_T: p = static_cast<PNS::SOLID*>( aItem )->Pos(); break;
    default:                 return false;
    }

    const PNS::JOINT* jt = static_cast<const PNS::NODE*>( aItem->Owner() )->FindJoint( p, aItem );

    assert( jt != nullptr );

    int mval = INT_MAX;

    PNS::ITEM_SET linkedSegs( jt->CLinks() );
    linkedSegs.ExcludeItem( aItem ).FilterKinds( PNS::ITEM::SEGMENT_T | PNS::ITEM::ARC_T );

    for( PNS::ITEM* item : linkedSegs.Items() )
    {
        int w = tryGetTrackWidth( item );

        if( w > 0 )
            mval = std::min( w, mval );
    }

    if( mval == INT_MAX )
        return false;

    *aInheritedWidth = mval;
    return true;
}


bool PNS_KICAD_IFACE_BASE::ImportSizes( PNS::SIZES_SETTINGS& aSizes, PNS::ITEM* aStartItem,
                                        PNS::NET_HANDLE aNet, VECTOR2D aStartPosition )
{
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    PNS::CONSTRAINT        constraint;

    if( aStartItem && m_startLayer < 0 )
        m_startLayer = aStartItem->Layer();

    aSizes.SetClearance( bds.m_MinClearance );
    aSizes.SetMinClearance( bds.m_MinClearance );
    aSizes.SetClearanceSource( _( "board minimum clearance" ) );

    int      startAnchor = 0;
    VECTOR2I startPosInt( aStartPosition.x, aStartPosition.y );

    if( aStartItem && aStartItem->Kind() == PNS::ITEM::SEGMENT_T )
    {
        // Find the start anchor which is closest to the start mouse location
        double anchor0Distance = startPosInt.Distance( aStartItem->Anchor( 0 ) );
        double anchor1Distance = startPosInt.Distance( aStartItem->Anchor( 1 ) );

        if( anchor1Distance < anchor0Distance )
            startAnchor = 1;
    }

    if( aStartItem )
    {
        PNS::SEGMENT dummyTrack;
        dummyTrack.SetEnds( aStartItem->Anchor( startAnchor ), aStartItem->Anchor( startAnchor ) );
        dummyTrack.SetLayer( m_startLayer );
        dummyTrack.SetNet( static_cast<NETINFO_ITEM*>( aStartItem->Net() ) );

        if( m_ruleResolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_CLEARANCE, &dummyTrack,
                                             nullptr, m_startLayer, &constraint ) )
        {
            if( constraint.m_Value.Min() >= bds.m_MinClearance )
            {
                aSizes.SetClearance( constraint.m_Value.Min() );
                aSizes.SetClearanceSource( constraint.m_RuleName );
            }
        }
    }

    int  trackWidth = bds.m_TrackMinWidth;
    bool found = false;
    aSizes.SetWidthSource( _( "board minimum track width" ) );

    if( bds.m_UseConnectedTrackWidth && !bds.m_TempOverrideTrackWidth && aStartItem != nullptr )
    {
        found = inheritTrackWidth( aStartItem, &trackWidth );

        if( found )
            aSizes.SetWidthSource( _( "existing track" ) );
    }

    if( !found && bds.UseNetClassTrack() && aStartItem )
    {
        PNS::SEGMENT dummyTrack;
        dummyTrack.SetEnds( aStartItem->Anchor( startAnchor ), aStartItem->Anchor( startAnchor ) );
        dummyTrack.SetLayer( m_startLayer );
        dummyTrack.SetNet( static_cast<NETINFO_ITEM*>( aStartItem->Net() ) );

        if( m_ruleResolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_WIDTH, &dummyTrack, nullptr,
                                             m_startLayer, &constraint ) )
        {
            trackWidth = std::max( trackWidth, constraint.m_Value.Opt() );
            found = true;

            if( trackWidth == constraint.m_Value.Opt() )
                aSizes.SetWidthSource( constraint.m_RuleName );
        }
    }

    if( !found )
    {
        trackWidth = std::max( trackWidth, bds.GetCurrentTrackWidth() );

        if( bds.UseNetClassTrack() )
            aSizes.SetWidthSource( _( "netclass 'Default'" ) );
        else if( trackWidth == bds.GetCurrentTrackWidth() )
            aSizes.SetWidthSource( _( "user choice" ) );
    }

    aSizes.SetTrackWidth( trackWidth );
    aSizes.SetBoardMinTrackWidth( bds.m_TrackMinWidth );
    aSizes.SetTrackWidthIsExplicit( !bds.m_UseConnectedTrackWidth || bds.m_TempOverrideTrackWidth );

    int viaDiameter = bds.m_ViasMinSize;
    int viaDrill = bds.m_MinThroughDrill;

    PNS::VIA dummyVia, coupledVia;

    if( aStartItem )
    {
        dummyVia.SetNet( aStartItem->Net() );
        coupledVia.SetNet( m_ruleResolver->DpCoupledNet( aStartItem->Net() ) );
    }

    if( bds.UseNetClassVia() && aStartItem )   // netclass value
    {
        if( m_ruleResolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_VIA_DIAMETER, &dummyVia,
                                             nullptr, m_startLayer, &constraint ) )
        {
            viaDiameter = std::max( viaDiameter, constraint.m_Value.Opt() );
        }

        if( m_ruleResolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_VIA_HOLE, &dummyVia,
                                             nullptr, m_startLayer, &constraint ) )
        {
            viaDrill = std::max( viaDrill, constraint.m_Value.Opt() );
        }
    }
    else
    {
        viaDiameter = bds.GetCurrentViaSize();
        viaDrill    = bds.GetCurrentViaDrill();
    }

    aSizes.SetViaDiameter( viaDiameter );
    aSizes.SetViaDrill( viaDrill );

    int diffPairWidth = bds.m_TrackMinWidth;
    int diffPairGap = bds.m_MinClearance;
    int diffPairViaGap = bds.m_MinClearance;

    aSizes.SetDiffPairWidthSource( _( "board minimum track width" ) );
    aSizes.SetDiffPairGapSource( _( "board minimum clearance" ) );

    found = false;

    // First try to pick up diff pair width from starting track, if enabled
    if( bds.m_UseConnectedTrackWidth && aStartItem )
        found = inheritTrackWidth( aStartItem, &diffPairWidth );

    // Next, pick up gap from netclass, and width also if we didn't get a starting width above
    if( bds.UseNetClassDiffPair() && aStartItem )
    {
        PNS::NET_HANDLE coupledNet = m_ruleResolver->DpCoupledNet( aStartItem->Net() );

        PNS::SEGMENT dummyTrack;
        dummyTrack.SetEnds( aStartItem->Anchor( 0 ), aStartItem->Anchor( 0 ) );
        dummyTrack.SetLayer( m_startLayer );
        dummyTrack.SetNet( static_cast<NETINFO_ITEM*>( aStartItem->Net() ) );

        PNS::SEGMENT coupledTrack;
        dummyTrack.SetEnds( aStartItem->Anchor( 0 ), aStartItem->Anchor( 0 ) );
        dummyTrack.SetLayer( m_startLayer );
        dummyTrack.SetNet( static_cast<NETINFO_ITEM*>( coupledNet ) );

        if( !found
            && m_ruleResolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_WIDTH, &dummyTrack,
                                                &coupledTrack, m_startLayer, &constraint ) )
        {
            diffPairWidth = std::max( diffPairWidth, constraint.m_Value.Opt() );

            if( diffPairWidth == constraint.m_Value.Opt() )
                aSizes.SetDiffPairWidthSource( constraint.m_RuleName );
        }

        if( m_ruleResolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_DIFF_PAIR_GAP, &dummyTrack,
                                             &coupledTrack, m_startLayer, &constraint ) )
        {
            diffPairGap = std::max( diffPairGap, constraint.m_Value.Opt() );
            diffPairViaGap = std::max( diffPairViaGap, constraint.m_Value.Opt() );

            if( diffPairGap == constraint.m_Value.Opt() )
                aSizes.SetDiffPairGapSource( constraint.m_RuleName );
        }
    }
    else
    {
        diffPairWidth  = bds.GetCurrentDiffPairWidth();
        diffPairGap    = bds.GetCurrentDiffPairGap();
        diffPairViaGap = bds.GetCurrentDiffPairViaGap();

        aSizes.SetDiffPairWidthSource( _( "user choice" ) );
        aSizes.SetDiffPairGapSource( _( "user choice" ) );
    }

    aSizes.SetDiffPairWidth( diffPairWidth );
    aSizes.SetDiffPairGap( diffPairGap );
    aSizes.SetDiffPairViaGap( diffPairViaGap );
    aSizes.SetDiffPairViaGapSameAsTraceGap( false );

    int holeToHoleMin = bds.m_HoleToHoleMin;

    if( m_ruleResolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_HOLE_TO_HOLE, &dummyVia,
                                         &dummyVia, UNDEFINED_LAYER, &constraint ) )
    {
        holeToHoleMin = constraint.m_Value.Min();
    }

    aSizes.SetHoleToHole( holeToHoleMin );

    if( m_ruleResolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_HOLE_TO_HOLE, &dummyVia,
                                         &coupledVia, UNDEFINED_LAYER, &constraint ) )
    {
        holeToHoleMin = constraint.m_Value.Min();
    }

    aSizes.SetDiffPairHoleToHole( holeToHoleMin );

    return true;
}


int PNS_KICAD_IFACE_BASE::StackupHeight( int aFirstLayer, int aSecondLayer ) const
{
    if( !m_board || !m_board->GetDesignSettings().m_UseHeightForLengthCalcs )
        return 0;

    BOARD_STACKUP& stackup = m_board->GetDesignSettings().GetStackupDescriptor();

    return stackup.GetLayerDistance( GetBoardLayerFromPNSLayer( aFirstLayer ),
                                     GetBoardLayerFromPNSLayer( aSecondLayer ) );
}


PNS::NET_HANDLE PNS_PCBNEW_RULE_RESOLVER::DpCoupledNet( PNS::NET_HANDLE aNet )
{
    return m_board->DpCoupledNet( static_cast<NETINFO_ITEM*>( aNet ) );
}


int PNS_PCBNEW_RULE_RESOLVER::NetCode( PNS::NET_HANDLE aNet )
{
    return m_routerIface->GetNetCode( aNet );
}


wxString PNS_PCBNEW_RULE_RESOLVER::NetName( PNS::NET_HANDLE aNet )
{
    return m_routerIface->GetNetName( aNet );
}


int PNS_PCBNEW_RULE_RESOLVER::DpNetPolarity( PNS::NET_HANDLE aNet )
{
    wxString refName;

    if( NETINFO_ITEM* net = static_cast<NETINFO_ITEM*>( aNet ) )
        refName = net->GetNetname();

    wxString dummy1;

    return m_board->MatchDpSuffix( refName, dummy1 );
}


bool PNS_PCBNEW_RULE_RESOLVER::DpNetPair( const PNS::ITEM* aItem, PNS::NET_HANDLE& aNetP,
                                          PNS::NET_HANDLE& aNetN )
{
    if( !aItem || !aItem->Net() )
        return false;

    wxString netNameP = static_cast<NETINFO_ITEM*>( aItem->Net() )->GetNetname();
    wxString netNameN, netNameCoupled;

    int r = m_board->MatchDpSuffix( netNameP, netNameCoupled );

    if( r == 0 )
    {
        return false;
    }
    else if( r == 1 )
    {
        netNameN = netNameCoupled;
    }
    else
    {
        netNameN = netNameP;
        netNameP = netNameCoupled;
    }

    PNS::NET_HANDLE netInfoP = m_board->FindNet( netNameP );
    PNS::NET_HANDLE netInfoN = m_board->FindNet( netNameN );

    if( !netInfoP || !netInfoN )
        return false;

    aNetP = netInfoP;
    aNetN = netInfoN;

    return true;
}


class PNS_PCBNEW_DEBUG_DECORATOR: public PNS::DEBUG_DECORATOR
{
public:
    PNS_PCBNEW_DEBUG_DECORATOR( PNS::ROUTER_IFACE* aIface  ) :
            PNS::DEBUG_DECORATOR(),
            m_iface( aIface ),
            m_view( nullptr ),
            m_items( nullptr ),
            m_depth( 0 )
    {}

    ~PNS_PCBNEW_DEBUG_DECORATOR()
    {
        PNS_PCBNEW_DEBUG_DECORATOR::Clear();
        delete m_items;
    }

    void SetView( KIGFX::VIEW* aView )
    {
        Clear();
        delete m_items;
        m_items = nullptr;
        m_view = aView;

        if( m_view == nullptr )
            return;

        if( m_view->GetGAL() )
            m_depth = m_view->GetGAL()->GetMinDepth();

        m_items = new KIGFX::VIEW_GROUP( m_view );
        m_items->SetLayer( LAYER_SELECT_OVERLAY ) ;
        m_view->Add( m_items );
    }

    void AddPoint( const VECTOR2I& aP, const KIGFX::COLOR4D& aColor, int aSize,
                   const wxString& aName = wxT( "" ),
                   const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override

    {
        SHAPE_LINE_CHAIN sh;

        sh.SetWidth( 10000 );

        sh.Append( aP.x - aSize, aP.y - aSize );
        sh.Append( aP.x + aSize, aP.y + aSize );
        sh.Append( aP.x, aP.y );
        sh.Append( aP.x - aSize, aP.y + aSize );
        sh.Append( aP.x + aSize, aP.y - aSize );

        AddShape( &sh, aColor, sh.Width(), aName, aSrcLoc );
    }

    void AddItem( const PNS::ITEM* aItem, const KIGFX::COLOR4D& aColor, int aOverrideWidth = 0,
                  const wxString& aName = wxT( "" ),
                  const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override
    {
        if( !m_view || !aItem )
            return;

        ROUTER_PREVIEW_ITEM* pitem = new ROUTER_PREVIEW_ITEM( aItem, m_iface, m_view );

        pitem->SetColor( aColor.WithAlpha( 0.5 ) );
        pitem->SetWidth( aOverrideWidth );
        pitem->SetDepth( nextDepth() );

        m_items->Add( pitem );
        m_view->Update( m_items );
    }

    void AddShape( const BOX2I& aBox, const KIGFX::COLOR4D& aColor, int aOverrideWidth = 0,
                   const wxString& aName = wxT( "" ),
                   const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override
    {
        SHAPE_LINE_CHAIN l;
        l.SetWidth( aOverrideWidth );

        VECTOR2I o = aBox.GetOrigin();
        VECTOR2I s = aBox.GetSize();

        l.Append( o );
        l.Append( o.x + s.x, o.y );
        l.Append( o.x + s.x, o.y + s.y );
        l.Append( o.x, o.y + s.y );
        l.Append( o );

        AddShape( &l, aColor, aOverrideWidth, aName, aSrcLoc );
    }

    void AddShape( const SHAPE* aShape, const KIGFX::COLOR4D& aColor, int aOverrideWidth = 0,
                   const wxString& aName = wxT( "" ),
                   const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override
    {
        if( !m_view || !aShape )
            return;

        ROUTER_PREVIEW_ITEM* pitem = new ROUTER_PREVIEW_ITEM( *aShape, m_iface, m_view );

        pitem->SetColor( aColor.WithAlpha( 0.5 ) );
        pitem->SetWidth( aOverrideWidth );
        pitem->SetDepth( nextDepth() );

        m_items->Add( pitem );
        m_view->Update( m_items );
    }

    void Clear() override
    {
        if( m_view && m_items )
        {
            m_items->FreeItems();
            m_view->Update( m_items );

            if( m_view->GetGAL() )
                m_depth = m_view->GetGAL()->GetMinDepth();
        }
    }

    virtual void Message( const wxString& msg,
                          const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override
                          {
                                printf("PNS: %s\n", msg.c_str().AsChar() );
                          }

private:
    double nextDepth()
    {
        // Use different depths so that the transculent shapes won't overwrite each other.

        m_depth++;

        if( m_depth >= 0 && m_view->GetGAL() )
            m_depth = m_view->GetGAL()->GetMinDepth();

        return m_depth;
    }

    PNS::ROUTER_IFACE* m_iface;
    KIGFX::VIEW* m_view;
    KIGFX::VIEW_GROUP* m_items;

    double m_depth;
};


PNS::DEBUG_DECORATOR* PNS_KICAD_IFACE_BASE::GetDebugDecorator()
{
    return m_debugDecorator;
}


PNS_KICAD_IFACE_BASE::PNS_KICAD_IFACE_BASE()
{
    m_ruleResolver = nullptr;
    m_board = nullptr;
    m_world = nullptr;
    m_debugDecorator = nullptr;
    m_startLayer = -1;
}


PNS_KICAD_IFACE::PNS_KICAD_IFACE()
{
    m_tool = nullptr;
    m_view = nullptr;
    m_previewItems = nullptr;
    m_commitFlags = 0;
}


PNS_KICAD_IFACE_BASE::~PNS_KICAD_IFACE_BASE()
{
    delete m_ruleResolver;
    delete m_debugDecorator;
}


PNS_KICAD_IFACE::~PNS_KICAD_IFACE()
{
    if( m_previewItems )
    {
        m_previewItems->FreeItems();
        delete m_previewItems;
    }
}


std::vector<std::unique_ptr<PNS::SOLID>> PNS_KICAD_IFACE_BASE::syncPad( PAD* aPad )
{
    std::vector<std::unique_ptr<PNS::SOLID>> solids;
    PNS_LAYER_RANGE layers( 0, aPad->BoardCopperLayerCount() - 1 );
    LSEQ lmsk = aPad->GetLayerSet().CuStack();

    // ignore non-copper pads except for those with holes
    if( lmsk.empty() && aPad->GetDrillSize().x == 0 )
        return solids;

    switch( aPad->GetAttribute() )
    {
    case PAD_ATTRIB::PTH:
    case PAD_ATTRIB::NPTH:
        break;

    case PAD_ATTRIB::CONN:
    case PAD_ATTRIB::SMD:
    {
        bool is_copper = false;

        if( !lmsk.empty() && aPad->GetAttribute() != PAD_ATTRIB::NPTH )
        {
            layers = SetLayersFromPCBNew( lmsk.front(), lmsk.front() );
            is_copper = true;
        }

        if( !is_copper )
            return solids;

        break;
    }

    default:
        wxLogTrace( wxT( "PNS" ), wxT( "unsupported pad type 0x%x" ), aPad->GetAttribute() );
        return solids;
    }

    auto makeSolidFromPadLayer =
        [&]( PCB_LAYER_ID aLayer )
        {
            std::unique_ptr<PNS::SOLID> solid = std::make_unique<PNS::SOLID>();

            if( aPad->GetAttribute() == PAD_ATTRIB::NPTH )
                solid->SetRoutable( false );

            if( aPad->Padstack().Mode() == PADSTACK::MODE::CUSTOM )
            {
                solid->SetLayer( GetPNSLayerFromBoardLayer( aLayer ) );
            }
            else if( aPad->Padstack().Mode() == PADSTACK::MODE::FRONT_INNER_BACK )
            {
                if( aLayer == F_Cu || aLayer == B_Cu )
                    solid->SetLayer( GetPNSLayerFromBoardLayer( aLayer ) );
                else
                    solid->SetLayers( PNS_LAYER_RANGE( 1, aPad->BoardCopperLayerCount() - 2 ) );
            }
            else
            {
                solid->SetLayers( layers );
            }

            solid->SetNet( aPad->GetNet() );
            solid->SetParent( aPad );
            solid->SetPadToDie( aPad->GetPadToDieLength() );
            solid->SetPadToDieDelay( aPad->GetPadToDieDelay() );
            solid->SetOrientation( aPad->GetOrientation() );

            if( aPad->IsFreePad() )
                solid->SetIsFreePad();

            VECTOR2I wx_c = aPad->ShapePos( aLayer );
            VECTOR2I offset = aPad->GetOffset( aLayer );

            VECTOR2I c( wx_c.x, wx_c.y );

            RotatePoint( offset, aPad->GetOrientation() );

            solid->SetPos( VECTOR2I( c.x - offset.x, c.y - offset.y ) );
            solid->SetOffset( VECTOR2I( offset.x, offset.y ) );

            if( aPad->GetDrillSize().x > 0 )
            {
                solid->SetHole( new PNS::HOLE( aPad->GetEffectiveHoleShape()->Clone() ) );
                solid->Hole()->SetLayers( PNS_LAYER_RANGE( 0, aPad->BoardCopperLayerCount() - 1 ) );
            }

            // We generate a single SOLID for a pad, so we have to treat it as ALWAYS_FLASHED and
            // then perform layer-specific flashing tests internally.
            const std::shared_ptr<SHAPE>& shape =
                    aPad->GetEffectiveShape( aLayer, FLASHING::ALWAYS_FLASHED );

            if( shape->HasIndexableSubshapes() && shape->GetIndexableSubshapeCount() == 1 )
            {
                std::vector<const SHAPE*> subshapes;
                shape->GetIndexableSubshapes( subshapes );

                solid->SetShape( subshapes[0]->Clone() );
            }
            // For anything that's not a single shape we use a polygon.  Multiple shapes have a tendency
            // to confuse the hull generator. https://gitlab.com/kicad/code/kicad/-/issues/15553
            else
            {
                const std::shared_ptr<SHAPE_POLY_SET>& poly =
                        aPad->GetEffectivePolygon( aLayer, ERROR_OUTSIDE );

                if( poly->OutlineCount() )
                    solid->SetShape( new SHAPE_SIMPLE( poly->Outline( 0 ) ) );
            }

            solids.emplace_back( std::move( solid ) );
        };

    aPad->Padstack().ForEachUniqueLayer( makeSolidFromPadLayer );

    return solids;
}


std::unique_ptr<PNS::SEGMENT> PNS_KICAD_IFACE_BASE::syncTrack( PCB_TRACK* aTrack )
{
    auto segment = std::make_unique<PNS::SEGMENT>( SEG( aTrack->GetStart(), aTrack->GetEnd() ),
                                                   aTrack->GetNet() );

    segment->SetWidth( aTrack->GetWidth() );
    segment->SetLayer( GetPNSLayerFromBoardLayer( aTrack->GetLayer() ) );
    segment->SetParent( aTrack );

    if( aTrack->IsLocked() )
        segment->Mark( PNS::MK_LOCKED );

    if( PCB_GENERATOR* generator = dynamic_cast<PCB_GENERATOR*>( aTrack->GetParentGroup() ) )
    {
        if( !generator->HasFlag( IN_EDIT ) )
            segment->Mark( PNS::MK_LOCKED );
    }

    return segment;
}


std::unique_ptr<PNS::ARC> PNS_KICAD_IFACE_BASE::syncArc( PCB_ARC* aArc )
{
    auto arc = std::make_unique<PNS::ARC>( SHAPE_ARC( aArc->GetStart(), aArc->GetMid(),
                                                      aArc->GetEnd(), aArc->GetWidth() ),
                                           aArc->GetNet() );

    arc->SetLayer( GetPNSLayerFromBoardLayer( aArc->GetLayer() ) );
    arc->SetParent( aArc );

    if( aArc->IsLocked() )
        arc->Mark( PNS::MK_LOCKED );

    if( PCB_GENERATOR* generator = dynamic_cast<PCB_GENERATOR*>( aArc->GetParentGroup() ) )
    {
        if( !generator->HasFlag( IN_EDIT ) )
            arc->Mark( PNS::MK_LOCKED );
    }

    return arc;
}


std::unique_ptr<PNS::VIA> PNS_KICAD_IFACE_BASE::syncVia( PCB_VIA* aVia )
{
    PCB_LAYER_ID top, bottom;
    aVia->LayerPair( &top, &bottom );

    /*
     * NOTE about PNS via padstacks:
     *
     * PNS::VIA has no knowledge about how many layers are in the board, and there is no fixed
     * reference to the "back layer" in the PNS.  That means that there is no way for a VIA to know
     * the difference between its bottom layer and the bottom layer of the overall board (i.e. if
     * the via is a blind/buried via).  For this reason, PNS::VIA::STACK_MODE::FRONT_INNER_BACK
     * cannot be used for blind/buried vias.  This mode will always assume that the via's top layer
     * is the "front" layer and the via's bottom layer is the "back" layer, but from KiCad's point
     * of view, at least at the moment, front/inner/back padstack mode is board-scoped, not
     * via-scoped, so a buried via would only use the inner layer size even if its padstack mode is
     * set to PADSTACK::MODE::FRONT_INNER_BACK and different sizes are defined for front or back.
     * For this kind of via, the PNS VIA stack mode will be set to NORMAL because effectively it has
     * the same size on every layer it exists on.
     */

    auto via = std::make_unique<PNS::VIA>( aVia->GetPosition(),
                                   SetLayersFromPCBNew( aVia->TopLayer(), aVia->BottomLayer() ),
                                   0,
                                   aVia->GetDrillValue(),
                                   aVia->GetNet(),
                                   aVia->GetViaType() );
    via->SetUnconnectedLayerMode( aVia->Padstack().UnconnectedLayerMode() );

    auto syncDiameter =
        [&]( PCB_LAYER_ID aLayer )
        {
            via->SetDiameter( GetPNSLayerFromBoardLayer( aLayer ), aVia->GetWidth( aLayer ) );
        };

    switch( aVia->Padstack().Mode() )
    {
    case PADSTACK::MODE::NORMAL:
        via->SetDiameter( 0, aVia->GetWidth( PADSTACK::ALL_LAYERS ) );
        break;

    case PADSTACK::MODE::FRONT_INNER_BACK:
        if( aVia->GetViaType() == VIATYPE::BLIND_BURIED )
        {
            via->SetDiameter( 0, aVia->GetWidth( PADSTACK::INNER_LAYERS ) );
        }
        else
        {
            via->SetStackMode( PNS::VIA::STACK_MODE::FRONT_INNER_BACK );
            aVia->Padstack().ForEachUniqueLayer( syncDiameter );
        }

        break;

    case PADSTACK::MODE::CUSTOM:
        via->SetStackMode( PNS::VIA::STACK_MODE::CUSTOM );
        aVia->Padstack().ForEachUniqueLayer( syncDiameter );
    }

    via->SetParent( aVia );

    if( aVia->IsLocked() )
        via->Mark( PNS::MK_LOCKED );

    if( PCB_GENERATOR* generator = dynamic_cast<PCB_GENERATOR*>( aVia->GetParentGroup() ) )
    {
        if( !generator->HasFlag( IN_EDIT ) )
            via->Mark( PNS::MK_LOCKED );
    }

    via->SetIsFree( aVia->GetIsFree() );
    via->SetHole( PNS::HOLE::MakeCircularHole( aVia->GetPosition(),
                                               aVia->GetDrillValue() / 2,
                                               SetLayersFromPCBNew( aVia->TopLayer(), aVia->BottomLayer() ) ) );

    return via;
}


bool PNS_KICAD_IFACE_BASE::syncZone( PNS::NODE* aWorld, ZONE* aZone, SHAPE_POLY_SET* aBoardOutline )
{
    static wxString msg;
    SHAPE_POLY_SET* poly;

    if( !aZone->GetIsRuleArea() || !aZone->HasKeepoutParametersSet() )
        return false;

    LSET layers = aZone->GetLayerSet();

    poly = aZone->Outline();
    poly->CacheTriangulation( false );

    if( !poly->IsTriangulationUpToDate() )
    {
        UNITS_PROVIDER unitsProvider( pcbIUScale, GetUnits() );
        msg.Printf( _( "%s is malformed." ), aZone->GetItemDescription( &unitsProvider, true ) );

        KIDIALOG dlg( nullptr, msg, KIDIALOG::KD_WARNING );
        dlg.ShowDetailedText( _( "This zone cannot be handled by the router.\n"
                                 "Please verify it is not a self-intersecting polygon." ) );
        dlg.DoNotShowCheckbox( __FILE__, __LINE__ );
        dlg.ShowModal();

        return false;
    }

    for( PCB_LAYER_ID layer : LAYER_RANGE( F_Cu, B_Cu, m_board->GetCopperLayerCount() ) )
    {
        if( !layers[ layer ] )
            continue;

        for( int polyId = 0; polyId < poly->TriangulatedPolyCount(); polyId++ )
        {
            const SHAPE_POLY_SET::TRIANGULATED_POLYGON* tri = poly->TriangulatedPolygon( polyId );

            for( size_t i = 0; i < tri->GetTriangleCount(); i++)
            {
                VECTOR2I a, b, c;
                tri->GetTriangle( i, a, b, c );
                SHAPE_SIMPLE* triShape = new SHAPE_SIMPLE;

                triShape->Append( a );
                triShape->Append( b );
                triShape->Append( c );

                std::unique_ptr<PNS::SOLID> solid = std::make_unique<PNS::SOLID>();

                solid->SetLayer( GetPNSLayerFromBoardLayer( layer ) );
                solid->SetNet( nullptr );
                solid->SetParent( aZone );
                solid->SetShape( triShape );
                solid->SetIsCompoundShapePrimitive();
                solid->SetRoutable( false );

                aWorld->Add( std::move( solid ) );
            }
        }
    }

    return true;
}


bool PNS_KICAD_IFACE_BASE::syncTextItem( PNS::NODE* aWorld, BOARD_ITEM* aItem, PCB_LAYER_ID aLayer )
{
    if( !IsKicadCopperLayer( aLayer ) )
        return false;

    if( aItem->Type() == PCB_FIELD_T && !static_cast<PCB_FIELD*>( aItem )->IsVisible() )
        return false;

    std::unique_ptr<PNS::SOLID> solid = std::make_unique<PNS::SOLID>();
    SHAPE_SIMPLE*               shape = new SHAPE_SIMPLE;

    solid->SetLayer( GetPNSLayerFromBoardLayer( aLayer ) );
    solid->SetNet( nullptr );
    solid->SetParent( aItem );
    solid->SetShape( shape );   // takes ownership
    solid->SetRoutable( false );

    SHAPE_POLY_SET cornerBuffer;

    aItem->TransformShapeToPolygon( cornerBuffer, aItem->GetLayer(), 0, aItem->GetMaxError(), ERROR_OUTSIDE );

    cornerBuffer.Simplify();

    if( !cornerBuffer.OutlineCount() )
        return false;

    for( const VECTOR2I& pt : cornerBuffer.Outline( 0 ).CPoints() )
        shape->Append( pt );

    aWorld->Add( std::move( solid ) );

    return true;
}


bool PNS_KICAD_IFACE_BASE::syncGraphicalItem( PNS::NODE* aWorld, PCB_SHAPE* aItem )
{
    if( aItem->GetLayer() == Edge_Cuts
            || aItem->GetLayer() == Margin
            || IsKicadCopperLayer( aItem->GetLayer() ) )
    {
        std::vector<SHAPE*> shapes = aItem->MakeEffectiveShapes();

        for( SHAPE* shape : shapes )
        {
            std::unique_ptr<PNS::SOLID> solid = std::make_unique<PNS::SOLID>();

            if( aItem->GetLayer() == Edge_Cuts || aItem->GetLayer() == Margin )
            {
                solid->SetLayers( PNS_LAYER_RANGE( 0, m_board->GetCopperLayerCount() - 1 ) );
                solid->SetRoutable( false );
            }
            else
            {
                solid->SetLayer( GetPNSLayerFromBoardLayer( aItem->GetLayer() ) );
                solid->SetRoutable( aItem->Type() != PCB_TABLECELL_T );
            }

            if( aItem->GetLayer() == Edge_Cuts )
            {
                switch( shape->Type() )
                {
                case SH_SEGMENT:    static_cast<SHAPE_SEGMENT*>( shape )->SetWidth( 0 );    break;
                case SH_ARC:        static_cast<SHAPE_ARC*>( shape )->SetWidth( 0 );        break;
                case SH_LINE_CHAIN: static_cast<SHAPE_LINE_CHAIN*>( shape )->SetWidth( 0 ); break;
                default:            /* remaining shapes don't have width */                 break;
                }
            }

            solid->SetAnchorPoints( aItem->GetConnectionPoints() );
            solid->SetNet( aItem->GetNet() );
            solid->SetParent( aItem );
            solid->SetShape( shape );       // takes ownership

            if( shapes.size() > 1 )
                solid->SetIsCompoundShapePrimitive();

            aWorld->Add( std::move( solid ) );
        }

        return true;
    }

    return false;
}


void PNS_KICAD_IFACE_BASE::SetBoard( BOARD* aBoard )
{
    m_board = aBoard;
    wxLogTrace( wxT( "PNS" ), wxT( "m_board = %p" ), m_board );
}


bool PNS_KICAD_IFACE_BASE::IsPNSCopperLayer( int aPNSLayer ) const
{
    return ::IsCopperLayer( GetBoardLayerFromPNSLayer( aPNSLayer ) );
}



bool PNS_KICAD_IFACE_BASE::IsKicadCopperLayer( PCB_LAYER_ID aKicadLayer ) const
{
    return ::IsCopperLayer( aKicadLayer );
}


bool PNS_KICAD_IFACE::IsAnyLayerVisible( const PNS_LAYER_RANGE& aLayer ) const
{
    if( !m_view )
        return false;

    for( int i = aLayer.Start(); i <= aLayer.End(); i++ )
    {
        if( m_view->IsLayerVisible( GetBoardLayerFromPNSLayer( i ) ) )
            return true;
    }

    return false;
}


bool PNS_KICAD_IFACE_BASE::IsFlashedOnLayer( const PNS::ITEM* aItem, int aLayer ) const
{
    /// Default is all layers
    if( aLayer < 0 )
        return true;

    if( aItem->Parent() )
    {
        switch( aItem->Parent()->Type() )
        {
        case PCB_VIA_T:
        {
            const PCB_VIA* via = static_cast<const PCB_VIA*>( aItem->Parent() );

            return via->FlashLayer( GetBoardLayerFromPNSLayer( aLayer ) );
        }

        case PCB_PAD_T:
        {
            const PAD* pad = static_cast<const PAD*>( aItem->Parent() );

            return pad->FlashLayer( GetBoardLayerFromPNSLayer( aLayer ) );
        }

        default:
            break;
        }
    }

    if( aItem->OfKind( PNS::ITEM::VIA_T ) )
        return static_cast<const PNS::VIA*>( aItem )->ConnectsLayer( aLayer );

    return aItem->Layers().Overlaps( aLayer );
}


bool PNS_KICAD_IFACE_BASE::IsFlashedOnLayer( const PNS::ITEM* aItem,
                                             const PNS_LAYER_RANGE& aLayer ) const
{
    PNS_LAYER_RANGE test = aItem->Layers().Intersection( aLayer );

    if( aItem->Parent() )
    {
        switch( aItem->Parent()->Type() )
        {
        case PCB_VIA_T:
        {
            const PCB_VIA* via = static_cast<const PCB_VIA*>( aItem->Parent() );

            for( int layer = test.Start(); layer <= test.End(); ++layer )
            {
                if( via->FlashLayer( GetBoardLayerFromPNSLayer( layer ) ) )
                    return true;
            }

            return false;
        }

        case PCB_PAD_T:
        {
            const PAD* pad = static_cast<const PAD*>( aItem->Parent() );

            for( int layer = test.Start(); layer <= test.End(); ++layer )
            {
                if( pad->FlashLayer( GetBoardLayerFromPNSLayer( layer ) ) )
                    return true;
            }

            return false;
        }

        default:
            break;
        }
    }

    if( aItem->OfKind( PNS::ITEM::VIA_T ) )
    {
        const PNS::VIA* via = static_cast<const PNS::VIA*>( aItem );

        for( int layer = test.Start(); layer <= test.End(); ++layer )
        {
            if( via->ConnectsLayer( layer ) )
                return true;
        }

        return false;
    }

    return test.Start() <= test.End();
}


bool PNS_KICAD_IFACE::IsItemVisible( const PNS::ITEM* aItem ) const
{
    // by default, all items are visible (new ones created by the router have parent == NULL
    // as they have not been committed yet to the BOARD)
    if( !m_view || !aItem->Parent() )
        return true;

    BOARD_ITEM*      item = aItem->Parent();
    bool             isOnVisibleLayer = true;
    RENDER_SETTINGS* settings = m_view->GetPainter()->GetSettings();

    if( settings->GetHighContrast() )
        isOnVisibleLayer = item->IsOnLayer( settings->GetPrimaryHighContrastLayer() );

    if( m_view->IsVisible( item ) && isOnVisibleLayer )
    {
        for( PCB_LAYER_ID layer : item->GetLayerSet() )
        {
            if( item->ViewGetLOD( layer, m_view ) < m_view->GetScale() )
                return true;
        }
    }

    // Items hidden in the router are not hidden on the board
    if( m_hiddenItems.find( item ) != m_hiddenItems.end() )
        return true;

    return false;
}


void PNS_KICAD_IFACE_BASE::SyncWorld( PNS::NODE *aWorld )
{
    if( !m_board )
    {
        wxLogTrace( wxT( "PNS" ), wxT( "No board attached, aborting sync." ) );
        return;
    }

    int worstClearance = m_board->GetMaxClearanceValue();

    m_world = aWorld;

    for( BOARD_ITEM* gitem : m_board->Drawings() )
    {
        if ( gitem->Type() == PCB_SHAPE_T || gitem->Type() == PCB_TEXTBOX_T )
        {
            syncGraphicalItem( aWorld, static_cast<PCB_SHAPE*>( gitem ) );
        }
        else if( gitem->Type() == PCB_TEXT_T )
        {
            syncTextItem( aWorld, static_cast<PCB_TEXT*>( gitem ), gitem->GetLayer() );
        }
        else if( gitem->Type() == PCB_TABLE_T )
        {
            syncTextItem( aWorld, static_cast<PCB_TABLE*>( gitem ), gitem->GetLayer() );
        }
    }

    SHAPE_POLY_SET  buffer;
    SHAPE_POLY_SET* boardOutline = nullptr;

    if( m_board->GetBoardPolygonOutlines( buffer ) )
        boardOutline = &buffer;

    for( ZONE* zone : m_board->Zones() )
    {
        syncZone( aWorld, zone, boardOutline );
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            std::vector<std::unique_ptr<PNS::SOLID>> solids = syncPad( pad );

            for( std::unique_ptr<PNS::SOLID>& solid : solids )
                aWorld->Add( std::move( solid ) );

            std::optional<int> clearanceOverride = pad->GetClearanceOverrides( nullptr );

            if( clearanceOverride.has_value() )
                worstClearance = std::max( worstClearance, clearanceOverride.value() );

            if( pad->GetProperty() == PAD_PROP::CASTELLATED )
            {
                std::unique_ptr<SHAPE> hole;
                hole.reset( pad->GetEffectiveHoleShape()->Clone() );
                aWorld->AddEdgeExclusion( std::move( hole ) );
            }
        }

        syncTextItem( aWorld, &footprint->Reference(), footprint->Reference().GetLayer() );
        syncTextItem( aWorld, &footprint->Value(), footprint->Value().GetLayer() );

        for( ZONE* zone : footprint->Zones() )
            syncZone( aWorld, zone, boardOutline );

        for( PCB_FIELD* field : footprint->GetFields() )
            syncTextItem( aWorld, static_cast<PCB_TEXT*>( field ), field->GetLayer() );

        for( BOARD_ITEM* item : footprint->GraphicalItems() )
        {
            if( item->Type() == PCB_SHAPE_T || item->Type() == PCB_TEXTBOX_T )
            {
                syncGraphicalItem( aWorld, static_cast<PCB_SHAPE*>( item ) );
            }
            else if( item->Type() == PCB_TEXT_T )
            {
                syncTextItem( aWorld, static_cast<PCB_TEXT*>( item ), item->GetLayer() );
            }
            else if( item->Type() == PCB_TABLE_T )
            {
                syncTextItem( aWorld, static_cast<PCB_TABLE*>( item ), item->GetLayer() );
            }
        }
    }

    for( PCB_TRACK* t : m_board->Tracks() )
    {
        KICAD_T type = t->Type();

        if( type == PCB_TRACE_T )
        {
            if( std::unique_ptr<PNS::SEGMENT> segment = syncTrack( t ) )
                aWorld->Add( std::move( segment ), true );
        }
        else if( type == PCB_ARC_T )
        {
            if( std::unique_ptr<PNS::ARC> arc = syncArc( static_cast<PCB_ARC*>( t ) ) )
                aWorld->Add( std::move( arc ), true );
        }
        else if( type == PCB_VIA_T )
        {
            if( std::unique_ptr<PNS::VIA> via = syncVia( static_cast<PCB_VIA*>( t ) ) )
                aWorld->Add( std::move( via ) );
        }
    }

    // NB: if this were ever to become a long-lived object we would need to dirty its
    // clearance cache here....
    delete m_ruleResolver;
    m_ruleResolver = new PNS_PCBNEW_RULE_RESOLVER( m_board, this );

    aWorld->SetRuleResolver( m_ruleResolver );
    aWorld->SetMaxClearance( worstClearance + m_ruleResolver->ClearanceEpsilon() );
}


void PNS_KICAD_IFACE::EraseView()
{
    for( BOARD_ITEM* item : m_hiddenItems )
        m_view->SetVisible( item, true );

    m_hiddenItems.clear();

    if( m_previewItems )
    {
        m_previewItems->FreeItems();
        m_view->Update( m_previewItems );
    }

    if( m_debugDecorator )
        m_debugDecorator->Clear();
}


void PNS_KICAD_IFACE_BASE::SetDebugDecorator( PNS::DEBUG_DECORATOR *aDec )
{
    m_debugDecorator = aDec;
}


void PNS_KICAD_IFACE::DisplayItem( const PNS::ITEM* aItem, int aClearance, bool aEdit, int aFlags )
{
    if( aItem->IsVirtual() )
        return;

    if( ZONE* zone = dynamic_cast<ZONE*>( aItem->Parent() ) )
    {
        if( zone->GetIsRuleArea() )
            aFlags |= PNS_SEMI_SOLID;
    }

    ROUTER_PREVIEW_ITEM* pitem = new ROUTER_PREVIEW_ITEM( aItem, this, m_view, aFlags );

    // Note: SEGMENT_T is used for placed tracks; LINE_T is used for the routing head
    static int tracks = PNS::ITEM::SEGMENT_T | PNS::ITEM::ARC_T | PNS::ITEM::LINE_T;
    static int tracksOrVias = tracks | PNS::ITEM::VIA_T;

    if( aClearance >= 0 )
    {
        pitem->SetClearance( aClearance );

        auto* settings = static_cast<PCBNEW_SETTINGS*>( m_tool->GetManager()->GetSettings() );

        switch( settings->m_Display.m_TrackClearance )
        {
        case SHOW_WITH_VIA_ALWAYS:
        case SHOW_WITH_VIA_WHILE_ROUTING_OR_DRAGGING:
            pitem->ShowClearance( aItem->OfKind( tracksOrVias ) );
            break;

        case SHOW_WITH_VIA_WHILE_ROUTING:
            pitem->ShowClearance( aItem->OfKind( tracksOrVias ) && !aEdit );
            break;

        case SHOW_WHILE_ROUTING:
            pitem->ShowClearance( aItem->OfKind( tracks ) && !aEdit );
            break;

        default:
            pitem->ShowClearance( false );
            break;
        }
    }

    m_previewItems->Add( pitem );
    m_view->Update( m_previewItems );
}


void PNS_KICAD_IFACE::DisplayPathLine( const SHAPE_LINE_CHAIN& aLine, int aImportance )
{
    ROUTER_PREVIEW_ITEM* pitem = new ROUTER_PREVIEW_ITEM( aLine, this, m_view );
    pitem->SetDepth( pitem->GetOriginDepth() - ROUTER_PREVIEW_ITEM::PathOverlayDepth );

    COLOR4D color;

    if( aImportance >= 1 )
        color = COLOR4D( 1.0, 1.0, 0.0, 0.6 );
    else if( aImportance == 0 )
        color = COLOR4D( 0.7, 0.7, 0.7, 0.6 );

    pitem->SetColor( color );

    m_previewItems->Add( pitem );
    m_view->Update( m_previewItems );
}


void PNS_KICAD_IFACE::DisplayRatline( const SHAPE_LINE_CHAIN& aRatline, PNS::NET_HANDLE aNet )
{
    ROUTER_PREVIEW_ITEM* pitem = new ROUTER_PREVIEW_ITEM( aRatline, this, m_view );

    KIGFX::RENDER_SETTINGS*     renderSettings = m_view->GetPainter()->GetSettings();
    KIGFX::PCB_RENDER_SETTINGS* rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( renderSettings );
    bool                        colorByNet = rs->GetNetColorMode() != NET_COLOR_MODE::OFF;
    COLOR4D                     defaultColor = rs->GetColor( nullptr, LAYER_RATSNEST );
    COLOR4D                     color = defaultColor;

    std::shared_ptr<CONNECTIVITY_DATA>  connectivity = m_board->GetConnectivity();
    std::set<int>                       highlightedNets = rs->GetHighlightNetCodes();
    std::map<int, KIGFX::COLOR4D>&      netColors = rs->GetNetColorMap();
    int                                 netCode = -1;

    if( NETINFO_ITEM* net = static_cast<NETINFO_ITEM*>( aNet ) )
        netCode = net->GetNetCode();

    const NETCLASS*     nc = nullptr;
    const NET_SETTINGS* netSettings = connectivity->GetNetSettings();

    if( connectivity->HasNetNameForNetCode( netCode ) )
    {
        const wxString& netName = connectivity->GetNetNameForNetCode( netCode );

        if( netSettings && netSettings->HasEffectiveNetClass( netName ) )
            nc = netSettings->GetCachedEffectiveNetClass( netName ).get();
    }

    if( colorByNet && netColors.count( netCode ) )
        color = netColors.at( netCode );
    else if( colorByNet && nc && nc->HasPcbColor() )
        color = nc->GetPcbColor();
    else
        color = defaultColor;

    if( color == COLOR4D::UNSPECIFIED )
        color = defaultColor;

    pitem->SetColor( color.Brightened( 0.5 ).WithAlpha( std::min( 1.0, color.a + 0.4 ) ) );

    m_previewItems->Add( pitem );
    m_view->Update( m_previewItems );
}


void PNS_KICAD_IFACE::HideItem( PNS::ITEM* aItem )
{
    BOARD_ITEM* parent = aItem->Parent();

    if( parent )
    {
        if( m_view->IsVisible( parent ) )
            m_hiddenItems.insert( parent );

        m_view->SetVisible( parent, false );
        m_view->Update( parent, KIGFX::APPEARANCE );

        for( ZONE* td : m_board->Zones() )
        {
            if( td->IsTeardropArea()
                && td->GetBoundingBox().Intersects( aItem->Parent()->GetBoundingBox() )
                && td->Outline()->Collide( aItem->Shape( td->GetLayer() ) ) )
            {
                m_view->SetVisible( td, false );
                m_view->Update( td, KIGFX::APPEARANCE );
            }
        }
    }
}


void PNS_KICAD_IFACE_BASE::RemoveItem( PNS::ITEM* aItem )
{
}


void PNS_KICAD_IFACE::RemoveItem( PNS::ITEM* aItem )
{
    BOARD_ITEM* parent = aItem->Parent();

    if( aItem->OfKind( PNS::ITEM::SOLID_T ) && parent->Type() == PCB_PAD_T )
    {
        PAD*   pad = static_cast<PAD*>( parent );
        VECTOR2I pos = static_cast<PNS::SOLID*>( aItem )->Pos();

        m_fpOffsets[ pad ].p_old = pos;
        return;
    }

    if( parent )
    {
        m_commit->Remove( parent );
    }
}


void PNS_KICAD_IFACE_BASE::UpdateItem( PNS::ITEM* aItem )
{
}


void PNS_KICAD_IFACE::modifyBoardItem( PNS::ITEM* aItem )
{
    BOARD_ITEM* board_item = aItem->Parent();

    switch( aItem->Kind() )
    {
    case PNS::ITEM::ARC_T:
    {
        PNS::ARC*        arc = static_cast<PNS::ARC*>( aItem );
        PCB_ARC*         arc_board = static_cast<PCB_ARC*>( board_item );
        const SHAPE_ARC* arc_shape = static_cast<const SHAPE_ARC*>( arc->Shape( -1 ) );

        m_commit->Modify( arc_board );

        arc_board->SetStart( VECTOR2I( arc_shape->GetP0() ) );
        arc_board->SetEnd( VECTOR2I( arc_shape->GetP1() ) );
        arc_board->SetMid( VECTOR2I( arc_shape->GetArcMid() ) );
        arc_board->SetWidth( arc->Width() );
        break;
    }

    case PNS::ITEM::SEGMENT_T:
    {
        PNS::SEGMENT* seg = static_cast<PNS::SEGMENT*>( aItem );
        PCB_TRACK*    track = static_cast<PCB_TRACK*>( board_item );
        const SEG&    s = seg->Seg();

        m_commit->Modify( track );

        track->SetStart( VECTOR2I( s.A.x, s.A.y ) );
        track->SetEnd( VECTOR2I( s.B.x, s.B.y ) );
        track->SetWidth( seg->Width() );
        break;
    }

    case PNS::ITEM::VIA_T:
    {
        PCB_VIA*  via_board = static_cast<PCB_VIA*>( board_item );
        PNS::VIA* via = static_cast<PNS::VIA*>( aItem );

        m_commit->Modify( via_board );

        via_board->SetPosition( VECTOR2I( via->Pos().x, via->Pos().y ) );
        via_board->SetWidth( PADSTACK::ALL_LAYERS, via->Diameter( 0 ) );
        via_board->SetDrill( via->Drill() );
        via_board->SetNet( static_cast<NETINFO_ITEM*>( via->Net() ) );
        via_board->SetViaType( via->ViaType() ); // MUST be before SetLayerPair()
        via_board->Padstack().SetUnconnectedLayerMode( via->UnconnectedLayerMode() );
        via_board->SetIsFree( via->IsFree() );
        via_board->SetLayerPair( GetBoardLayerFromPNSLayer( via->Layers().Start() ),
                                 GetBoardLayerFromPNSLayer( via->Layers().End() ) );
        break;
    }

    case PNS::ITEM::SOLID_T:
    {
        if( aItem->Parent()->Type() == PCB_PAD_T )
        {
            PAD*     pad = static_cast<PAD*>( aItem->Parent() );
            VECTOR2I pos = static_cast<PNS::SOLID*>( aItem )->Pos();

            // Don't add to commit; we'll add the parent footprints when processing the m_fpOffsets

            m_fpOffsets[pad].p_old = pad->GetPosition();
            m_fpOffsets[pad].p_new = pos;
        }
        break;
    }

    default:
        m_commit->Modify( aItem->Parent() );
        break;
    }
}


void PNS_KICAD_IFACE::UpdateItem( PNS::ITEM* aItem )
{
    modifyBoardItem( aItem );
}


void PNS_KICAD_IFACE_BASE::AddItem( PNS::ITEM* aItem )
{
}


BOARD_CONNECTED_ITEM* PNS_KICAD_IFACE::createBoardItem( PNS::ITEM* aItem )
{
    BOARD_CONNECTED_ITEM* newBoardItem = nullptr;
    NETINFO_ITEM* net = static_cast<NETINFO_ITEM*>( aItem->Net() );

    if( !net )
        net = NETINFO_LIST::OrphanedItem();

    switch( aItem->Kind() )
    {
    case PNS::ITEM::ARC_T:
    {
        PNS::ARC* arc = static_cast<PNS::ARC*>( aItem );
        PCB_ARC*  new_arc = new PCB_ARC( m_board, static_cast<const SHAPE_ARC*>( arc->Shape( -1 ) ) );
        new_arc->SetWidth( arc->Width() );
        new_arc->SetLayer( GetBoardLayerFromPNSLayer( arc->Layers().Start() ) );
        new_arc->SetNet( net );

        if( aItem->GetSourceItem() && aItem->GetSourceItem()->IsType( { PCB_TRACE_T, PCB_ARC_T } ) )
        {
            PCB_TRACK* sourceTrack = static_cast<PCB_TRACK*>( aItem->GetSourceItem() );
            new_arc->SetHasSolderMask( sourceTrack->HasSolderMask() );
            new_arc->SetLocalSolderMaskMargin( sourceTrack->GetLocalSolderMaskMargin() );
        }

        newBoardItem = new_arc;
        break;
    }

    case PNS::ITEM::SEGMENT_T:
    {
        PNS::SEGMENT* seg = static_cast<PNS::SEGMENT*>( aItem );
        PCB_TRACK*    track = new PCB_TRACK( m_board );
        const SEG&    s = seg->Seg();
        track->SetStart( VECTOR2I( s.A.x, s.A.y ) );
        track->SetEnd( VECTOR2I( s.B.x, s.B.y ) );
        track->SetWidth( seg->Width() );
        track->SetLayer( GetBoardLayerFromPNSLayer( seg->Layers().Start() ) );
        track->SetNet( net );

        if( aItem->GetSourceItem() && aItem->GetSourceItem()->IsType( { PCB_TRACE_T, PCB_ARC_T } ) )
        {
            PCB_TRACK* sourceTrack = static_cast<PCB_TRACK*>( aItem->GetSourceItem() );
            track->SetHasSolderMask( sourceTrack->HasSolderMask() );
            track->SetLocalSolderMaskMargin( sourceTrack->GetLocalSolderMaskMargin() );
        }

        newBoardItem = track;
        break;
    }

    case PNS::ITEM::VIA_T:
    {
        PCB_VIA*  via_board = new PCB_VIA( m_board );
        PNS::VIA* via = static_cast<PNS::VIA*>( aItem );
        via_board->SetPosition( VECTOR2I( via->Pos().x, via->Pos().y ) );
        via_board->SetWidth( PADSTACK::ALL_LAYERS, via->Diameter( 0 ) );
        via_board->SetDrill( via->Drill() );
        via_board->SetNet( net );
        via_board->SetViaType( via->ViaType() ); // MUST be before SetLayerPair()
        via_board->Padstack().SetUnconnectedLayerMode( via->UnconnectedLayerMode() );
        via_board->SetIsFree( via->IsFree() );
        via_board->SetLayerPair( GetBoardLayerFromPNSLayer( via->Layers().Start() ),
                                 GetBoardLayerFromPNSLayer( via->Layers().End() ) );

        if( aItem->GetSourceItem() && aItem->GetSourceItem()->Type() == PCB_VIA_T )
        {
            PCB_VIA* sourceVia = static_cast<PCB_VIA*>( aItem->GetSourceItem() );
            via_board->SetFrontTentingMode( sourceVia->GetFrontTentingMode() );
            via_board->SetBackTentingMode( sourceVia->GetBackTentingMode() );
        }

        newBoardItem = via_board;
        break;
    }

    case PNS::ITEM::SOLID_T:
    {
        PAD*     pad = static_cast<PAD*>( aItem->Parent() );
        VECTOR2I pos = static_cast<PNS::SOLID*>( aItem )->Pos();

        m_fpOffsets[pad].p_new = pos;
        return nullptr;
    }

    default:
        return nullptr;
    }

    if( net->GetNetCode() <= 0 )
    {
        NETINFO_ITEM* newNetInfo = newBoardItem->GetNet();

        newNetInfo->SetParent( m_board );
        newNetInfo->SetNetClass( m_board->GetDesignSettings().m_NetSettings->GetDefaultNetclass() );
    }

    return newBoardItem;
}


void PNS_KICAD_IFACE::AddItem( PNS::ITEM* aItem )
{
    BOARD_CONNECTED_ITEM* boardItem = createBoardItem( aItem );

    if( boardItem )
    {
        aItem->SetParent( boardItem );
        boardItem->ClearFlags();

        m_commit->Add( boardItem );
    }
}


void PNS_KICAD_IFACE::Commit()
{
    std::set<FOOTPRINT*> processedFootprints;

    EraseView();

    for( const auto& [ pad, fpOffset ] : m_fpOffsets )
    {
        VECTOR2I   offset = fpOffset.p_new - fpOffset.p_old;
        FOOTPRINT* footprint = pad->GetParentFootprint();
        VECTOR2I   p_orig = footprint->GetPosition();
        VECTOR2I   p_new = p_orig + offset;

        if( processedFootprints.find( footprint ) != processedFootprints.end() )
            continue;

        processedFootprints.insert( footprint );
        m_commit->Modify( footprint );
        footprint->SetPosition( p_new );
    }

    m_fpOffsets.clear();

    m_commit->Push( _( "Routing" ), m_commitFlags );
    m_commit = std::make_unique<BOARD_COMMIT>( m_tool );
}


EDA_UNITS PNS_KICAD_IFACE::GetUnits() const
{
    return static_cast<EDA_UNITS>( m_tool->GetManager()->GetSettings()->m_System.units );
}


void PNS_KICAD_IFACE::SetView( KIGFX::VIEW* aView )
{
    wxLogTrace( wxT( "PNS" ), wxT( "SetView %p" ), aView );

    if( m_previewItems )
    {
        m_previewItems->FreeItems();
        delete m_previewItems;
    }

    m_view = aView;
    m_previewItems = new KIGFX::VIEW_GROUP( m_view );
    m_previewItems->SetLayer( LAYER_SELECT_OVERLAY ) ;

    if(m_view)
        m_view->Add( m_previewItems );

    delete m_debugDecorator;

    auto dec = new PNS_PCBNEW_DEBUG_DECORATOR( this );
    m_debugDecorator = dec;

    dec->SetDebugEnabled( ADVANCED_CFG::GetCfg().m_ShowRouterDebugGraphics );

    if( ADVANCED_CFG::GetCfg().m_ShowRouterDebugGraphics )
        dec->SetView( m_view );
}


int PNS_KICAD_IFACE::GetNetCode( PNS::NET_HANDLE aNet ) const
{
    if( aNet )
        return static_cast<NETINFO_ITEM*>( aNet )->GetNetCode();
    else
        return -1;
}


wxString PNS_KICAD_IFACE::GetNetName( PNS::NET_HANDLE aNet ) const
{
    if( aNet )
        return static_cast<NETINFO_ITEM*>( aNet )->GetNetname();
    else
        return wxEmptyString;
}


void PNS_KICAD_IFACE::UpdateNet( PNS::NET_HANDLE aNet )
{
    wxLogTrace( wxT( "PNS" ), wxT( "Update-net %s" ), GetNetName( aNet ) );
}


PNS::NET_HANDLE PNS_KICAD_IFACE_BASE::GetOrphanedNetHandle()
{
    return NETINFO_LIST::OrphanedItem();
}


PNS::RULE_RESOLVER* PNS_KICAD_IFACE_BASE::GetRuleResolver()
{
    return m_ruleResolver;
}


void PNS_KICAD_IFACE::SetHostTool( PCB_TOOL_BASE* aTool )
{
    m_tool = aTool;
    m_commit = std::make_unique<BOARD_COMMIT>( m_tool );
}


PCB_LAYER_ID PNS_KICAD_IFACE_BASE::GetBoardLayerFromPNSLayer( int aLayer ) const
{
    if( aLayer < 0 )
        return PCB_LAYER_ID::UNDEFINED_LAYER;

    if( aLayer == 0 )
        return F_Cu;

    if( aLayer == m_board->GetCopperLayerCount() - 1 )
        return B_Cu;

    return static_cast<PCB_LAYER_ID>( ( aLayer + 1 ) * 2 );
}


int PNS_KICAD_IFACE_BASE::GetPNSLayerFromBoardLayer( PCB_LAYER_ID aLayer ) const
{
    if( aLayer < 0 )
        return -1;

    if( aLayer == F_Cu )
        return 0;

    if( aLayer == B_Cu )
        return m_board->GetCopperLayerCount() - 1;

    return ( aLayer / 2 ) - 1;
}


void PNS_KICAD_IFACE_BASE::SetStartLayerFromPCBNew( PCB_LAYER_ID aLayer )
{
    m_startLayer = GetPNSLayerFromBoardLayer( aLayer );
}


PNS_LAYER_RANGE PNS_KICAD_IFACE_BASE::SetLayersFromPCBNew( PCB_LAYER_ID aStartLayer, PCB_LAYER_ID aEndLayer )
{
    return PNS_LAYER_RANGE( GetPNSLayerFromBoardLayer( aStartLayer ),
                             GetPNSLayerFromBoardLayer( aEndLayer ) );
}


long long int PNS_KICAD_IFACE_BASE::CalculateRoutedPathLength( const PNS::ITEM_SET& aLine, const PNS::SOLID* aStartPad,
                                                               const PNS::SOLID* aEndPad, const NETCLASS* aNetClass )
{
    std::vector<LENGTH_DELAY_CALCULATION_ITEM> lengthItems = getLengthDelayCalculationItems( aLine, aNetClass );

    const PAD* startPad = nullptr;
    const PAD* endPad = nullptr;

    if( aStartPad )
        startPad = static_cast<PAD*>( aStartPad->Parent() );

    if( aEndPad )
        endPad = static_cast<PAD*>( aEndPad->Parent() );

    constexpr PATH_OPTIMISATIONS opts = {
        .OptimiseViaLayers = false, .MergeTracks = false, .OptimiseTracesInPads = false, .InferViaInPad = true
    };
    const BOARD* board = GetBoard();
    return board->GetLengthCalculation()->CalculateLength( lengthItems, opts, startPad, endPad );
}


int64_t PNS_KICAD_IFACE_BASE::CalculateRoutedPathDelay( const PNS::ITEM_SET& aLine, const PNS::SOLID* aStartPad,
                                                        const PNS::SOLID* aEndPad, const NETCLASS* aNetClass )
{
    std::vector<LENGTH_DELAY_CALCULATION_ITEM> lengthItems = getLengthDelayCalculationItems( aLine, aNetClass );

    const PAD* startPad = nullptr;
    const PAD* endPad = nullptr;

    if( aStartPad )
        startPad = static_cast<PAD*>( aStartPad->Parent() );

    if( aEndPad )
        endPad = static_cast<PAD*>( aEndPad->Parent() );

    constexpr PATH_OPTIMISATIONS opts = {
        .OptimiseViaLayers = false, .MergeTracks = false, .OptimiseTracesInPads = false, .InferViaInPad = true
    };
    const BOARD* board = GetBoard();
    return board->GetLengthCalculation()->CalculateDelay( lengthItems, opts, startPad, endPad );
}


int64_t PNS_KICAD_IFACE_BASE::CalculateLengthForDelay( int64_t aDesiredDelay, const int aWidth,
                                                       const bool aIsDiffPairCoupled, const int aDiffPairCouplingGap,
                                                       const int aPNSLayer, const NETCLASS* aNetClass )
{
    TIME_DOMAIN_GEOMETRY_CONTEXT ctx;
    ctx.NetClass = aNetClass;
    ctx.Width = aWidth;
    ctx.IsDiffPairCoupled = aIsDiffPairCoupled;
    ctx.DiffPairCouplingGap = aDiffPairCouplingGap;
    ctx.Layer = GetBoardLayerFromPNSLayer( aPNSLayer );

    const BOARD* board = GetBoard();
    return board->GetLengthCalculation()->CalculateLengthForDelay( aDesiredDelay, ctx );
}


int64_t PNS_KICAD_IFACE_BASE::CalculateDelayForShapeLineChain( const SHAPE_LINE_CHAIN& aShape, int aWidth,
                                                               bool aIsDiffPairCoupled, int aDiffPairCouplingGap,
                                                               int aPNSLayer, const NETCLASS* aNetClass )
{
    TIME_DOMAIN_GEOMETRY_CONTEXT ctx;
    ctx.NetClass = aNetClass;
    ctx.Width = aWidth;
    ctx.IsDiffPairCoupled = aIsDiffPairCoupled;
    ctx.DiffPairCouplingGap = aDiffPairCouplingGap;
    ctx.Layer = GetBoardLayerFromPNSLayer( aPNSLayer );

    const BOARD* board = GetBoard();
    return board->GetLengthCalculation()->CalculatePropagationDelayForShapeLineChain( aShape, ctx );
}


std::vector<LENGTH_DELAY_CALCULATION_ITEM>
PNS_KICAD_IFACE_BASE::getLengthDelayCalculationItems( const PNS::ITEM_SET& aLine, const NETCLASS* aNetClass ) const
{
    std::vector<LENGTH_DELAY_CALCULATION_ITEM> lengthItems;

    for( int idx = 0; idx < aLine.Size(); idx++ )
    {
        const PNS::ITEM* lineItem = aLine[idx];

        if( const PNS::LINE* l = dyn_cast<const PNS::LINE*>( lineItem ) )
        {
            LENGTH_DELAY_CALCULATION_ITEM item;
            item.SetLine( l->CLine() );

            const PCB_LAYER_ID layer = GetBoardLayerFromPNSLayer( lineItem->Layer() );
            item.SetLayers( layer );
            item.SetEffectiveNetClass( aNetClass );

            lengthItems.emplace_back( std::move( item ) );
        }
        else if( lineItem->OfKind( PNS::ITEM::VIA_T ) && idx > 0 && idx < aLine.Size() - 1 )
        {
            const int          layerPrev = aLine[idx - 1]->Layer();
            const int          layerNext = aLine[idx + 1]->Layer();
            const PCB_LAYER_ID pcbLayerPrev = GetBoardLayerFromPNSLayer( layerPrev );
            const PCB_LAYER_ID pcbLayerNext = GetBoardLayerFromPNSLayer( layerNext );

            if( layerPrev != layerNext )
            {
                LENGTH_DELAY_CALCULATION_ITEM item;
                item.SetVia( static_cast<PCB_VIA*>( lineItem->GetSourceItem() ) );
                item.SetLayers( pcbLayerPrev, pcbLayerNext ); // TODO: BUG IS HERE!!!
                item.SetEffectiveNetClass( aNetClass );
                lengthItems.emplace_back( std::move( item ) );
            }
        }
    }

    return lengthItems;
}
