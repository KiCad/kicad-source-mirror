/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright (C) 2016-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <zone.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <board_commit.h>
#include <layer_ids.h>
#include <geometry/convex_hull.h>
#include <confirm.h>
#include <tools/pcb_tool_base.h>
#include <tool/tool_manager.h>
#include <settings/app_settings.h>

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
#include "pns_routing_settings.h"
#include "pns_sizes_settings.h"
#include "pns_item.h"
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
            return hash<const void*>()( k.A ) ^ hash<const void*>()( k.B ) ^ hash<int>()( k.Flag );
        }
    };
}


class PNS_PCBNEW_RULE_RESOLVER : public PNS::RULE_RESOLVER
{
public:
    PNS_PCBNEW_RULE_RESOLVER( BOARD* aBoard, PNS::ROUTER_IFACE* aRouterIface );
    virtual ~PNS_PCBNEW_RULE_RESOLVER();

    virtual int Clearance( const PNS::ITEM* aA, const PNS::ITEM* aB,
                           bool aUseClearanceEpsilon = true ) override;

    virtual int DpCoupledNet( int aNet ) override;
    virtual int DpNetPolarity( int aNet ) override;
    virtual bool DpNetPair( const PNS::ITEM* aItem, int& aNetP, int& aNetN ) override;

    virtual bool IsInNetTie( const PNS::ITEM* aA ) override;
    virtual bool IsNetTieExclusion( const PNS::ITEM* aItem, const VECTOR2I& aCollisionPos,
                                    const PNS::ITEM* aCollidingItem ) override;

    virtual bool IsKeepout( const PNS::ITEM* aA, const PNS::ITEM* aB ) override;

    virtual bool QueryConstraint( PNS::CONSTRAINT_TYPE aType, const PNS::ITEM* aItemA,
                                  const PNS::ITEM* aItemB, int aLayer,
                                  PNS::CONSTRAINT* aConstraint ) override;
    virtual wxString NetName( int aNet ) override;

    int ClearanceEpsilon() const override { return m_clearanceEpsilon; }

    void ClearCacheForItem( const PNS::ITEM* aItem ) override;
    void ClearCaches() override;

private:
    /**
     * Checks for netnamed differential pairs.
     * This accepts nets named suffixed by 'P', 'N', '+', '-', as well as additional
     * numbers and underscores following the suffix.  So NET_P_123 is a valid positive net
     * name matched to NET_N_123.
     * @param aNetName Input net name to check for DP naming
     * @param aComplementNet Generated net name for the pair
     * @return -1 if found the negative pair, +1 if found the positive pair, 0 otherwise
     */
    int matchDpSuffix( const wxString& aNetName, wxString& aComplementNet );

private:
    PNS::ROUTER_IFACE* m_routerIface;
    BOARD*             m_board;
    PCB_TRACK          m_dummyTracks[2];
    PCB_ARC            m_dummyArcs[2];
    PCB_VIA            m_dummyVias[2];
    int                m_clearanceEpsilon;

    std::unordered_map<CLEARANCE_CACHE_KEY, int> m_clearanceCache;
};


PNS_PCBNEW_RULE_RESOLVER::PNS_PCBNEW_RULE_RESOLVER( BOARD* aBoard,
                                                    PNS::ROUTER_IFACE* aRouterIface ) :
    m_routerIface( aRouterIface ),
    m_board( aBoard ),
    m_dummyTracks{ { aBoard }, { aBoard } },
    m_dummyArcs{ { aBoard }, { aBoard } },
    m_dummyVias{ { aBoard }, { aBoard } }
{
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
    wxCHECK( aItem && aCollidingItem, false );

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
        return drcEngine->IsNetTieExclusion( aItem->Net(), ToLAYER_ID( aItem->Layer() ),
                                             aCollisionPos, collidingItem );
    }

    return false;
}


bool PNS_PCBNEW_RULE_RESOLVER::IsKeepout( const PNS::ITEM* aA, const PNS::ITEM* aB )
{
    auto checkKeepout =
            []( const ZONE* aKeepout, const BOARD_ITEM* aOther )
            {
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

    if( const ZONE* zoneA = dynamic_cast<ZONE*>( aA->Parent() ) )
    {
        if( zoneA->GetIsRuleArea() && aB->Parent() )
            return checkKeepout( zoneA, aB->Parent() );
    }

    if( const ZONE* zoneB = dynamic_cast<ZONE*>( aB->Parent() ) )
    {
        if( zoneB->GetIsRuleArea() && aA->Parent() )
            return checkKeepout( zoneB, aA->Parent() );
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

    const BOARD_ITEM *parent = aItem->BoardItem();

    return parent && ( parent->IsOnLayer( Edge_Cuts ) || parent->IsOnLayer( Margin ) );
}


bool PNS_PCBNEW_RULE_RESOLVER::QueryConstraint( PNS::CONSTRAINT_TYPE aType,
                                                const PNS::ITEM* aItemA, const PNS::ITEM* aItemB,
                                                int aLayer, PNS::CONSTRAINT* aConstraint )
{
    std::shared_ptr<DRC_ENGINE> drcEngine = m_board->GetDesignSettings().m_DRCEngine;

    if( !drcEngine )
        return false;

    DRC_CONSTRAINT_T hostType;

    switch ( aType )
    {
    case PNS::CONSTRAINT_TYPE::CT_CLEARANCE:      hostType = CLEARANCE_CONSTRAINT;      break;
    case PNS::CONSTRAINT_TYPE::CT_WIDTH:          hostType = TRACK_WIDTH_CONSTRAINT;    break;
    case PNS::CONSTRAINT_TYPE::CT_DIFF_PAIR_GAP:  hostType = DIFF_PAIR_GAP_CONSTRAINT;  break;
    case PNS::CONSTRAINT_TYPE::CT_LENGTH:         hostType = LENGTH_CONSTRAINT;         break;
    case PNS::CONSTRAINT_TYPE::CT_VIA_DIAMETER:   hostType = VIA_DIAMETER_CONSTRAINT;   break;
    case PNS::CONSTRAINT_TYPE::CT_VIA_HOLE:       hostType = HOLE_SIZE_CONSTRAINT;      break;
    case PNS::CONSTRAINT_TYPE::CT_HOLE_CLEARANCE: hostType = HOLE_CLEARANCE_CONSTRAINT; break;
    case PNS::CONSTRAINT_TYPE::CT_EDGE_CLEARANCE: hostType = EDGE_CLEARANCE_CONSTRAINT; break;
    case PNS::CONSTRAINT_TYPE::CT_HOLE_TO_HOLE:   hostType = HOLE_TO_HOLE_CONSTRAINT;   break;
    default:                                      return false; // should not happen
    }

    BOARD_ITEM*    parentA = aItemA ? aItemA->BoardItem() : nullptr;
    BOARD_ITEM*    parentB = aItemB ? aItemB->BoardItem() : nullptr;
    DRC_CONSTRAINT hostConstraint;

    // A track being routed may not have a BOARD_ITEM associated yet.
    if( aItemA && !parentA )
    {
        switch( aItemA->Kind() )
        {
        case PNS::ITEM::ARC_T:     parentA = &m_dummyArcs[0];   break;
        case PNS::ITEM::VIA_T:     parentA = &m_dummyVias[0];   break;
        case PNS::ITEM::SEGMENT_T: parentA = &m_dummyTracks[0]; break;
        case PNS::ITEM::LINE_T:    parentA = &m_dummyTracks[0]; break;
        case PNS::ITEM::HOLE_T:    parentA = &m_dummyVias[0];   break;
        default: break;
        }

        if( parentA )
        {
            parentA->SetLayer( ToLAYER_ID( aLayer ) );
            static_cast<BOARD_CONNECTED_ITEM*>( parentA )->SetNetCode( aItemA->Net(), true );
        }
    }

    if( aItemB && !parentB )
    {
        switch( aItemB->Kind() )
        {
        case PNS::ITEM::ARC_T:     parentB = &m_dummyArcs[1];   break;
        case PNS::ITEM::VIA_T:     parentB = &m_dummyVias[1];   break;
        case PNS::ITEM::SEGMENT_T: parentB = &m_dummyTracks[1]; break;
        case PNS::ITEM::LINE_T:    parentB = &m_dummyTracks[1]; break;
        case PNS::ITEM::HOLE_T:    parentB = &m_dummyVias[1];   break;
        default: break;
        }

        if( parentB )
        {
            parentB->SetLayer( ToLAYER_ID( aLayer ) );
            static_cast<BOARD_CONNECTED_ITEM*>( parentB )->SetNetCode( aItemB->Net(), true );
        }
    }

    if( parentA )
        hostConstraint = drcEngine->EvalRules( hostType, parentA, parentB, ToLAYER_ID( aLayer ) );

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
            aConstraint->m_Value = hostConstraint.GetValue();
            aConstraint->m_RuleName = hostConstraint.GetName();
            aConstraint->m_Type = aType;
            return true;

        default:
            return false;
    }
}


void PNS_PCBNEW_RULE_RESOLVER::ClearCacheForItem( const PNS::ITEM* aItem )
{
    CLEARANCE_CACHE_KEY key = { aItem, nullptr, false };
    m_clearanceCache.erase( key );

    key.Flag = true;
    m_clearanceCache.erase( key );
}


void PNS_PCBNEW_RULE_RESOLVER::ClearCaches()
{
    m_clearanceCache.clear();
}


int PNS_PCBNEW_RULE_RESOLVER::Clearance( const PNS::ITEM* aA, const PNS::ITEM* aB,
                                         bool aUseClearanceEpsilon )
{
    CLEARANCE_CACHE_KEY key = { aA, aB, aUseClearanceEpsilon };
    auto it = m_clearanceCache.find( key );

    if( it != m_clearanceCache.end() )
        return it->second;

    PNS::CONSTRAINT constraint;
    int             rv = 0;
    LAYER_RANGE     layers;

    if( !aB )
        layers = aA->Layers();
    else if( isEdge( aA ) )
        layers = aB->Layers();
    else if( isEdge( aB ) )
        layers = aA->Layers();
    else
        layers = aA->Layers().Intersection( aB->Layers() );

    // Normalize layer range (no -1 magic numbers)
    layers = layers.Intersection( LAYER_RANGE( PCBNEW_LAYER_ID_START, PCB_LAYER_ID_COUNT - 1 ) );

    for( int layer = layers.Start(); layer <= layers.End(); ++layer )
    {
        if( isHole( aA ) && isHole( aB) )
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
        else if( isCopper( aA ) && ( !aB || isCopper( aB ) ) )
        {
            if( QueryConstraint( PNS::CONSTRAINT_TYPE::CT_CLEARANCE, aA, aB, layer, &constraint ) )
            {
                if( constraint.m_Value.Min() > rv )
                    rv = constraint.m_Value.Min();
            }
        }
        else if( isEdge( aA ) || ( aB && isEdge( aB ) ) )
        {
            if( QueryConstraint( PNS::CONSTRAINT_TYPE::CT_EDGE_CLEARANCE, aA, aB, layer, &constraint ) )
            {
                if( constraint.m_Value.Min() > rv )
                    rv = constraint.m_Value.Min();
            }
        }
    }

    if( aUseClearanceEpsilon && rv > 0 )
        rv = std::max( 0, rv - m_clearanceEpsilon );

    m_clearanceCache[ key ] = rv;
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
            case PNS::ITEM::ARC_T: return static_cast<PNS::ARC*>( aPnsItem )->Width();
            default: return -1;
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
    case PNS::ITEM::VIA_T:
        p = static_cast<PNS::VIA*>( aItem )->Pos();
        break;

    case PNS::ITEM::SOLID_T:
        p = static_cast<PNS::SOLID*>( aItem )->Pos();
        break;

    default:
        return false;
    }

    const PNS::JOINT* jt = static_cast<const PNS::NODE*>( aItem->Owner() )->FindJoint( p, aItem );

    assert( jt != nullptr );

    int mval = INT_MAX;

    PNS::ITEM_SET linkedSegs( jt->CLinks() );
    linkedSegs.ExcludeItem( aItem ).FilterKinds( PNS::ITEM::SEGMENT_T | PNS::ITEM::ARC_T );

    for( PNS::ITEM* item : linkedSegs.Items() )
    {
        int w = tryGetTrackWidth( item );
        assert( w > 0 );
        mval = std::min( w, mval );
    }

    if( mval == INT_MAX )
        return false;

    *aInheritedWidth = mval;
    return true;
}


bool PNS_KICAD_IFACE_BASE::ImportSizes( PNS::SIZES_SETTINGS& aSizes, PNS::ITEM* aStartItem,
                                        int aNet )
{
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    PNS::CONSTRAINT        constraint;

    if( aStartItem && m_startLayer < 0 )
        m_startLayer = aStartItem->Layer();

    aSizes.SetClearance( bds.m_MinClearance );
    aSizes.SetMinClearance( bds.m_MinClearance );
    aSizes.SetClearanceSource( _( "board minimum clearance" ) );

    if( m_ruleResolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_CLEARANCE, aStartItem, nullptr,
                                         m_startLayer, &constraint ) )
    {
        if( constraint.m_Value.Min() > bds.m_MinClearance )
        {
            aSizes.SetClearance( constraint.m_Value.Min() );
            aSizes.SetClearanceSource( constraint.m_RuleName );
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
        if( m_ruleResolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_WIDTH, aStartItem, nullptr,
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
    aSizes.SetTrackWidthIsExplicit( !bds.m_UseConnectedTrackWidth || bds.m_TempOverrideTrackWidth );

    int viaDiameter = bds.m_ViasMinSize;
    int viaDrill = bds.m_MinThroughDrill;

    if( bds.UseNetClassVia() && aStartItem )   // netclass value
    {
        if( m_ruleResolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_VIA_DIAMETER, aStartItem,
                                             nullptr, m_startLayer, &constraint ) )
        {
            viaDiameter = std::max( viaDiameter, constraint.m_Value.Opt() );
        }

        if( m_ruleResolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_VIA_HOLE, aStartItem,
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
        if( !found && m_ruleResolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_WIDTH, aStartItem,
                                                       nullptr, m_startLayer, &constraint ) )
        {
            diffPairWidth = std::max( diffPairWidth, constraint.m_Value.Opt() );

            if( diffPairWidth == constraint.m_Value.Opt() )
                aSizes.SetDiffPairWidthSource( constraint.m_RuleName );
        }

        if( m_ruleResolver->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_DIFF_PAIR_GAP, aStartItem,
                                             nullptr, m_startLayer, &constraint ) )
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

    int      holeToHoleMin = bds.m_HoleToHoleMin;
    PNS::VIA dummyVia, coupledVia;

    if( aStartItem )
    {
        dummyVia.SetNet( aStartItem->Net() );
        coupledVia.SetNet( m_ruleResolver->DpCoupledNet( aStartItem->Net() ) );
    }

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

    return stackup.GetLayerDistance( ToLAYER_ID( aFirstLayer ), ToLAYER_ID( aSecondLayer ) );
}


int PNS_PCBNEW_RULE_RESOLVER::matchDpSuffix( const wxString& aNetName, wxString& aComplementNet )
{
    int rv = 0;
    int count = 0;

    for( auto it = aNetName.rbegin(); it != aNetName.rend() && rv == 0; ++it, ++count )
    {
        int ch = *it;

        if( ( ch >= '0' && ch <= '9' ) || ch == '_' )
        {
            continue;
        }
        else if( ch == '+' )
        {
            aComplementNet = wxT( "-" );
            rv = 1;
        }
        else if( ch == '-' )
        {
            aComplementNet = wxT( "+" );
            rv = -1;
        }
        else if( ch == 'N' )
        {
            aComplementNet = wxT( "P" );
            rv = -1;
        }
        else if ( ch == 'P' )
        {
            aComplementNet = wxT( "N" );
            rv = 1;
        }
        else
        {
            break;
        }
    }

    if( rv != 0 && count >= 1 )
    {
        aComplementNet = aNetName.Left( aNetName.length() - count ) + aComplementNet
                + aNetName.Right( count - 1 );
    }

    return rv;
}


int PNS_PCBNEW_RULE_RESOLVER::DpCoupledNet( int aNet )
{
    wxString refName = m_board->FindNet( aNet )->GetNetname();
    wxString coupledNetName;

    if( matchDpSuffix( refName, coupledNetName ) )
    {
        NETINFO_ITEM* net = m_board->FindNet( coupledNetName );

        if( !net )
            return -1;

        return net->GetNetCode();
    }

    return -1;
}


wxString PNS_PCBNEW_RULE_RESOLVER::NetName( int aNet )
{
    return m_board->FindNet( aNet )->GetNetname();
}


int PNS_PCBNEW_RULE_RESOLVER::DpNetPolarity( int aNet )
{
    wxString refName = m_board->FindNet( aNet )->GetNetname();
    wxString dummy1;

    return matchDpSuffix( refName, dummy1 );
}


bool PNS_PCBNEW_RULE_RESOLVER::DpNetPair( const PNS::ITEM* aItem, int& aNetP, int& aNetN )
{
    if( !aItem )
        return false;

    NETINFO_ITEM* netInfo = nullptr;

    if( aItem->Parent() && aItem->Parent()->IsConnected() )
    {
        BOARD_CONNECTED_ITEM* cItem = static_cast<BOARD_CONNECTED_ITEM*>( aItem->Parent() );
        netInfo = cItem->GetNet();
    }
    else
        netInfo = m_board->FindNet( aItem->Net() );

    if( !netInfo )
        return false;

    wxString netNameP = netInfo->GetNetname();
    wxString netNameN, netNameCoupled;

    int r = matchDpSuffix( netNameP, netNameCoupled );

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

    NETINFO_ITEM* netInfoP = m_board->FindNet( netNameP );
    NETINFO_ITEM* netInfoN = m_board->FindNet( netNameN );

    if( !netInfoP || !netInfoN )
        return false;

    aNetP = netInfoP->GetNetCode();
    aNetN = netInfoN->GetNetCode();

    return true;
}


class PNS_PCBNEW_DEBUG_DECORATOR: public PNS::DEBUG_DECORATOR
{
public:
    PNS_PCBNEW_DEBUG_DECORATOR( KIGFX::VIEW* aView = nullptr ) :
            PNS::DEBUG_DECORATOR(),
            m_view( nullptr ),
            m_items( nullptr ),
            m_depth( 0 )
    {
        SetView( aView );
    }


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
                   const wxString&          aName = wxT( "" ),
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
                  const wxString&          aName = wxT( "" ),
                  const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override
    {
        if( !m_view || !aItem )
            return;

        ROUTER_PREVIEW_ITEM* pitem = new ROUTER_PREVIEW_ITEM( aItem, m_view );

        pitem->SetColor( aColor.WithAlpha( 0.5 ) );
        pitem->SetWidth( aOverrideWidth );
        pitem->SetDepth( nextDepth() );

        m_items->Add( pitem );
        m_view->Update( m_items );
    }


    void AddShape( const BOX2I& aBox, const KIGFX::COLOR4D& aColor, int aOverrideWidth = 0,
                   const wxString&          aName = wxT( "" ),
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
                   const wxString&          aName = wxT( "" ),
                   const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ) override
    {
        if( !m_view || !aShape )
            return;

        ROUTER_PREVIEW_ITEM* pitem = new ROUTER_PREVIEW_ITEM( *aShape, m_view );

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

private:
    double nextDepth()
    {
        // Use different depths so that the transculent shapes won't overwrite each other.

        m_depth++;

        if( m_depth >= 0 && m_view->GetGAL() )
            m_depth = m_view->GetGAL()->GetMinDepth();

        return m_depth;
    }

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
}


PNS_KICAD_IFACE::~PNS_KICAD_IFACE()
{
    delete m_ruleResolver;
    delete m_debugDecorator;

     if( m_previewItems )
    {
        m_previewItems->FreeItems();
        delete m_previewItems;
    }
}


std::unique_ptr<PNS::SOLID> PNS_KICAD_IFACE_BASE::syncPad( PAD* aPad )
{
    LAYER_RANGE layers( 0, MAX_CU_LAYERS - 1 );

    // ignore non-copper pads except for those with holes
    if( ( aPad->GetLayerSet() & LSET::AllCuMask() ).none() && aPad->GetDrillSize().x == 0 )
        return nullptr;

    switch( aPad->GetAttribute() )
    {
    case PAD_ATTRIB::PTH:
    case PAD_ATTRIB::NPTH:
        break;

    case PAD_ATTRIB::CONN:
    case PAD_ATTRIB::SMD:
    {
        LSET lmsk = aPad->GetLayerSet();
        bool is_copper = false;

        for( int i = 0; i < MAX_CU_LAYERS; i++ )
        {
            if( lmsk[i] )
            {
                is_copper = true;

                if( aPad->GetAttribute() != PAD_ATTRIB::NPTH )
                    layers = LAYER_RANGE( i );

                break;
            }
        }

        if( !is_copper )
            return nullptr;

        break;
    }

    default:
        wxLogTrace( wxT( "PNS" ), wxT( "unsupported pad type 0x%x" ), aPad->GetAttribute() );
        return nullptr;
    }

    std::unique_ptr<PNS::SOLID> solid = std::make_unique<PNS::SOLID>();

    if( aPad->GetAttribute() == PAD_ATTRIB::NPTH )
        solid->SetRoutable( false );

    solid->SetLayers( layers );
    solid->SetNet( aPad->GetNetCode() );
    solid->SetParent( aPad );
    solid->SetPadToDie( aPad->GetPadToDieLength() );
    solid->SetOrientation( aPad->GetOrientation() );

    if( aPad->IsFreePad() )
        solid->SetIsFreePad();

    VECTOR2I wx_c = aPad->ShapePos();
    VECTOR2I offset = aPad->GetOffset();

    VECTOR2I c( wx_c.x, wx_c.y );

    RotatePoint( offset, aPad->GetOrientation() );

    solid->SetPos( VECTOR2I( c.x - offset.x, c.y - offset.y ) );
    solid->SetOffset( VECTOR2I( offset.x, offset.y ) );

    if( aPad->GetDrillSize().x > 0 )
        solid->SetHole( new PNS::HOLE( aPad->GetEffectiveHoleShape()->Clone() ) );

    // We generate a single SOLID for a pad, so we have to treat it as ALWAYS_FLASHED and then
    // perform layer-specific flashing tests internally.
    std::shared_ptr<SHAPE> shape = aPad->GetEffectiveShape( UNDEFINED_LAYER,
                                                            FLASHING::ALWAYS_FLASHED );

    if( shape->HasIndexableSubshapes() && shape->GetIndexableSubshapeCount() == 1 )
    {
        std::vector<const SHAPE*> subshapes;
        shape->GetIndexableSubshapes( subshapes );

        solid->SetShape( subshapes[0]->Clone() );
    }
    else
    {
        solid->SetShape( shape->Clone() );
    }

    return solid;
}


std::unique_ptr<PNS::SEGMENT> PNS_KICAD_IFACE_BASE::syncTrack( PCB_TRACK* aTrack )
{
    auto segment = std::make_unique<PNS::SEGMENT>( SEG( aTrack->GetStart(), aTrack->GetEnd() ),
                                                   aTrack->GetNetCode() );

    segment->SetWidth( aTrack->GetWidth() );
    segment->SetLayers( LAYER_RANGE( aTrack->GetLayer() ) );
    segment->SetParent( aTrack );

    if( aTrack->IsLocked() )
        segment->Mark( PNS::MK_LOCKED );

    return segment;
}


std::unique_ptr<PNS::ARC> PNS_KICAD_IFACE_BASE::syncArc( PCB_ARC* aArc )
{
    auto arc = std::make_unique<PNS::ARC>(
            SHAPE_ARC( aArc->GetStart(), aArc->GetMid(), aArc->GetEnd(), aArc->GetWidth() ),
            aArc->GetNetCode() );

    arc->SetLayers( LAYER_RANGE( aArc->GetLayer() ) );
    arc->SetParent( aArc );

    if( aArc->IsLocked() )
        arc->Mark( PNS::MK_LOCKED );

    return arc;
}


std::unique_ptr<PNS::VIA> PNS_KICAD_IFACE_BASE::syncVia( PCB_VIA* aVia )
{
    PCB_LAYER_ID top, bottom;
    aVia->LayerPair( &top, &bottom );

    auto via = std::make_unique<PNS::VIA>( aVia->GetPosition(),
                                           LAYER_RANGE( aVia->TopLayer(), aVia->BottomLayer() ),
                                           aVia->GetWidth(),
                                           aVia->GetDrillValue(),
                                           aVia->GetNetCode(),
                                           aVia->GetViaType() );

    via->SetParent( aVia );

    if( aVia->IsLocked() )
        via->Mark( PNS::MK_LOCKED );

    via->SetIsFree( aVia->GetIsFree() );
    via->SetHole( PNS::HOLE::MakeCircularHole( aVia->GetPosition(), aVia->GetDrillValue() / 2 ) );

    return via;
}


bool PNS_KICAD_IFACE_BASE::syncZone( PNS::NODE* aWorld, ZONE* aZone, SHAPE_POLY_SET* aBoardOutline )
{
    SHAPE_POLY_SET* poly;

    if( !aZone->GetIsRuleArea() && aZone->GetZoneName().IsEmpty() )
        return false;

    // TODO handle aZone->GetDoNotAllowVias()
    // TODO handle rules which disallow tracks & vias
    if( !aZone->GetIsRuleArea() || !aZone->GetDoNotAllowTracks() )
        return false;

    LSET      layers = aZone->GetLayerSet();

    poly = aZone->Outline();
    poly->CacheTriangulation( false );

    if( !poly->IsTriangulationUpToDate() )
    {
        UNITS_PROVIDER unitsProvider( pcbIUScale, GetUnits() );
        KIDIALOG       dlg( nullptr, wxString::Format( _( "%s is malformed." ),
                                              aZone->GetItemDescription( &unitsProvider ) ),
                            KIDIALOG::KD_WARNING );
        dlg.ShowDetailedText( wxString::Format( _( "This zone cannot be handled by the router.\n"
                                                   "Please verify it is not a self-intersecting "
                                                   "polygon." ) ) );
        dlg.DoNotShowCheckbox( __FILE__, __LINE__ );
        dlg.ShowModal();

        return false;
    }

    for( int layer = F_Cu; layer <= B_Cu; layer++ )
    {
        if( !layers[ layer ] )
            continue;

        for( int outline = 0; outline < poly->OutlineCount(); outline++ )
        {
            const SHAPE_POLY_SET::TRIANGULATED_POLYGON* tri = poly->TriangulatedPolygon( outline );

            for( size_t i = 0; i < tri->GetTriangleCount(); i++)
            {
                VECTOR2I a, b, c;
                tri->GetTriangle( i, a, b, c );
                SHAPE_SIMPLE* triShape = new SHAPE_SIMPLE;

                triShape->Append( a );
                triShape->Append( b );
                triShape->Append( c );

                std::unique_ptr<PNS::SOLID> solid = std::make_unique<PNS::SOLID>();

                solid->SetLayer( layer );
                solid->SetNet( -1 );
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


bool PNS_KICAD_IFACE_BASE::syncTextItem( PNS::NODE* aWorld, PCB_TEXT* aText, PCB_LAYER_ID aLayer )
{
    if( !IsCopperLayer( aLayer ) )
        return false;

    std::unique_ptr<PNS::SOLID> solid = std::make_unique<PNS::SOLID>();
    SHAPE_SIMPLE*               shape = new SHAPE_SIMPLE;

    solid->SetLayer( aLayer );
    solid->SetNet( -1 );
    solid->SetParent( aText );
    solid->SetShape( shape );   // takes ownership
    solid->SetRoutable( false );

    TEXT_ATTRIBUTES attrs = aText->GetAttributes();
    int             margin = KiROUND( attrs.m_StrokeWidth / 2 );
    SHAPE_POLY_SET  cornerBuffer;

    if( aText->IsKnockout() )
        margin += attrs.m_StrokeWidth + GetKnockoutTextMargin( attrs.m_Size, attrs.m_StrokeWidth );

    aText->TransformBoundingBoxToPolygon( &cornerBuffer, margin );

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
            || IsCopperLayer( aItem->GetLayer() ) )
    {
        std::vector<SHAPE*> shapes = aItem->MakeEffectiveShapes();

        for( SHAPE* shape : shapes )
        {
            std::unique_ptr<PNS::SOLID> solid = std::make_unique<PNS::SOLID>();

            if( aItem->GetLayer() == Edge_Cuts || aItem->GetLayer() == Margin )
                solid->SetLayers( LAYER_RANGE( F_Cu, B_Cu ) );
            else
                solid->SetLayer( aItem->GetLayer() );

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

            solid->SetNet( -1 );
            solid->SetParent( aItem );
            solid->SetShape( shape );       // takes ownership

            if( shapes.size() > 1 )
                solid->SetIsCompoundShapePrimitive();

            solid->SetRoutable( false );

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


bool PNS_KICAD_IFACE::IsAnyLayerVisible( const LAYER_RANGE& aLayer ) const
{
    if( !m_view )
        return false;

    for( int i = aLayer.Start(); i <= aLayer.End(); i++ )
    {
        if( m_view->IsLayerVisible( i ) )
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

            return via->FlashLayer( ToLAYER_ID( aLayer ) );
        }

        case PCB_PAD_T:
        {
            const PAD* pad = static_cast<const PAD*>( aItem->Parent() );

            return pad->FlashLayer( ToLAYER_ID( aLayer ) );
        }

        default:
            break;
        }
    }

    return aItem->Layers().Overlaps( aLayer );
}


bool PNS_KICAD_IFACE_BASE::IsFlashedOnLayer( const PNS::ITEM* aItem,
                                             const LAYER_RANGE& aLayer ) const
{
    LAYER_RANGE test = aItem->Layers().Intersection( aLayer );

    if( aItem->Parent() )
    {
        switch( aItem->Parent()->Type() )
        {
        case PCB_VIA_T:
        {
            const PCB_VIA* via = static_cast<const PCB_VIA*>( aItem->Parent() );

            for( int layer = test.Start(); layer <= test.End(); ++layer )
            {
                if( via->FlashLayer( ToLAYER_ID( layer ) ) )
                    return true;
            }

            return false;
        }

        case PCB_PAD_T:
        {
            const PAD* pad = static_cast<const PAD*>( aItem->Parent() );

            for( int layer = test.Start(); layer <= test.End(); ++layer )
            {
                if( pad->FlashLayer( ToLAYER_ID( layer ) ) )
                    return true;
            }

            return false;
        }

        default:
            break;
        }
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
        for( PCB_LAYER_ID layer : item->GetLayerSet().Seq() )
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

    int worstClearance = m_board->GetDesignSettings().GetBiggestClearanceValue();

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
            if( std::unique_ptr<PNS::SOLID> solid = syncPad( pad ) )
                aWorld->Add( std::move( solid ) );

            worstClearance = std::max( worstClearance, pad->GetLocalClearance() );

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
        }
    }

    for( PCB_TRACK* t : m_board->Tracks() )
    {
        KICAD_T type = t->Type();

        if( type == PCB_TRACE_T )
        {
            if( std::unique_ptr<PNS::SEGMENT> segment = syncTrack( t ) )
                aWorld->Add( std::move( segment ) );
        }
        else if( type == PCB_ARC_T )
        {
            if( std::unique_ptr<PNS::ARC> arc = syncArc( static_cast<PCB_ARC*>( t ) ) )
                aWorld->Add( std::move( arc ) );
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
    for( auto item : m_hiddenItems )
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


void PNS_KICAD_IFACE::DisplayItem( const PNS::ITEM* aItem, int aClearance, bool aEdit, bool aIsHeadTrace )
{
    if( aItem->IsVirtual() )
        return;

    ROUTER_PREVIEW_ITEM* pitem = new ROUTER_PREVIEW_ITEM( aItem, m_view );

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

    if( aIsHeadTrace )
    {
        pitem->SetIsHeadTrace( true );
        pitem->Update( aItem );
    }

    m_previewItems->Add( pitem );
    m_view->Update( m_previewItems );
}


void PNS_KICAD_IFACE::DisplayPathLine( const SHAPE_LINE_CHAIN& aLine, int aImportance )
{
    ROUTER_PREVIEW_ITEM* pitem = new ROUTER_PREVIEW_ITEM( aLine, m_view );
    pitem->SetDepth( ROUTER_PREVIEW_ITEM::PathOverlayDepth );

    COLOR4D color;

    if( aImportance >= 1 )
        color = COLOR4D( 1.0, 1.0, 0.0, 0.6 );
    else if( aImportance == 0 )
        color = COLOR4D( 0.7, 0.7, 0.7, 0.6 );

    pitem->SetColor( color );

    m_previewItems->Add( pitem );
    m_view->Update( m_previewItems );
}


void PNS_KICAD_IFACE::DisplayRatline( const SHAPE_LINE_CHAIN& aRatline, int aNetCode )
{
    ROUTER_PREVIEW_ITEM* pitem = new ROUTER_PREVIEW_ITEM( aRatline, m_view );

    KIGFX::RENDER_SETTINGS*     renderSettings = m_view->GetPainter()->GetSettings();
    KIGFX::PCB_RENDER_SETTINGS* rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( renderSettings );
    bool                        colorByNet = rs->GetNetColorMode() != NET_COLOR_MODE::OFF;
    COLOR4D                     defaultColor = rs->GetColor( nullptr, LAYER_RATSNEST );
    COLOR4D                     color = defaultColor;

    std::shared_ptr<CONNECTIVITY_DATA>  connectivity = m_board->GetConnectivity();
    std::set<int>                       highlightedNets = rs->GetHighlightNetCodes();
    std::map<int, KIGFX::COLOR4D>&      netColors = rs->GetNetColorMap();
    std::map<wxString, KIGFX::COLOR4D>& ncColors = rs->GetNetclassColorMap();
    const std::map<int, wxString>&      ncMap = connectivity->GetNetclassMap();

    if( colorByNet && netColors.count( aNetCode ) )
        color = netColors.at( aNetCode );
    else if( colorByNet && ncMap.count( aNetCode ) && ncColors.count( ncMap.at( aNetCode ) ) )
        color = ncColors.at( ncMap.at( aNetCode ) );
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
    }
}


void PNS_KICAD_IFACE_BASE::RemoveItem( PNS::ITEM* aItem )
{
}


void PNS_KICAD_IFACE::RemoveItem( PNS::ITEM* aItem )
{
    BOARD_ITEM* parent = aItem->Parent();

    if( aItem->OfKind( PNS::ITEM::SOLID_T ) )
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


void PNS_KICAD_IFACE::UpdateItem( PNS::ITEM* aItem )
{
    BOARD_ITEM* board_item = aItem->Parent();

    m_commit->Modify( board_item );

    switch( aItem->Kind() )
    {
    case PNS::ITEM::ARC_T:
    {
        PNS::ARC*        arc = static_cast<PNS::ARC*>( aItem );
        PCB_ARC*         arc_board = static_cast<PCB_ARC*>( board_item );
        const SHAPE_ARC* arc_shape = static_cast<const SHAPE_ARC*>( arc->Shape() );
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
        track->SetStart( VECTOR2I( s.A.x, s.A.y ) );
        track->SetEnd( VECTOR2I( s.B.x, s.B.y ) );
        track->SetWidth( seg->Width() );
        break;
    }

    case PNS::ITEM::VIA_T:
    {
        PCB_VIA*  via_board = static_cast<PCB_VIA*>( board_item );
        PNS::VIA* via = static_cast<PNS::VIA*>( aItem );
        via_board->SetPosition( VECTOR2I( via->Pos().x, via->Pos().y ) );
        via_board->SetWidth( via->Diameter() );
        via_board->SetDrill( via->Drill() );
        via_board->SetNetCode( via->Net() > 0 ? via->Net() : 0 );
        via_board->SetViaType( via->ViaType() ); // MUST be before SetLayerPair()
        via_board->SetIsFree( via->IsFree() );
        via_board->SetLayerPair( ToLAYER_ID( via->Layers().Start() ),
                                 ToLAYER_ID( via->Layers().End() ) );
        break;
    }

    case PNS::ITEM::SOLID_T:
    {
        PAD*     pad = static_cast<PAD*>( aItem->Parent() );
        VECTOR2I pos = static_cast<PNS::SOLID*>( aItem )->Pos();

        m_fpOffsets[ pad ].p_old = pad->GetPosition();
        m_fpOffsets[ pad ].p_new = pos;
        break;
    }

    default:
        break;
    }
}


void PNS_KICAD_IFACE_BASE::AddItem( PNS::ITEM* aItem )
{

}


void PNS_KICAD_IFACE::AddItem( PNS::ITEM* aItem )
{
    BOARD_CONNECTED_ITEM* newBI = nullptr;

    switch( aItem->Kind() )
    {
    case PNS::ITEM::ARC_T:
    {
        PNS::ARC* arc = static_cast<PNS::ARC*>( aItem );
        PCB_ARC*  new_arc = new PCB_ARC( m_board, static_cast<const SHAPE_ARC*>( arc->Shape() ) );
        new_arc->SetWidth( arc->Width() );
        new_arc->SetLayer( ToLAYER_ID( arc->Layers().Start() ) );
        new_arc->SetNetCode( std::max<int>( 0, arc->Net() ) );
        newBI = new_arc;
        break;
    }

    case PNS::ITEM::SEGMENT_T:
    {
        PNS::SEGMENT* seg = static_cast<PNS::SEGMENT*>( aItem );
        PCB_TRACK*    track = new PCB_TRACK( m_board );
        const SEG& s = seg->Seg();
        track->SetStart( VECTOR2I( s.A.x, s.A.y ) );
        track->SetEnd( VECTOR2I( s.B.x, s.B.y ) );
        track->SetWidth( seg->Width() );
        track->SetLayer( ToLAYER_ID( seg->Layers().Start() ) );
        track->SetNetCode( seg->Net() > 0 ? seg->Net() : 0 );
        newBI = track;
        break;
    }

    case PNS::ITEM::VIA_T:
    {
        PCB_VIA*  via_board = new PCB_VIA( m_board );
        PNS::VIA* via = static_cast<PNS::VIA*>( aItem );
        via_board->SetPosition( VECTOR2I( via->Pos().x, via->Pos().y ) );
        via_board->SetWidth( via->Diameter() );
        via_board->SetDrill( via->Drill() );
        via_board->SetNetCode( via->Net() > 0 ? via->Net() : 0 );
        via_board->SetViaType( via->ViaType() ); // MUST be before SetLayerPair()
        via_board->SetIsFree( via->IsFree() );
        via_board->SetLayerPair( ToLAYER_ID( via->Layers().Start() ),
                                 ToLAYER_ID( via->Layers().End() ) );
        newBI = via_board;
        break;
    }

    case PNS::ITEM::SOLID_T:
    {
        PAD*   pad = static_cast<PAD*>( aItem->Parent() );
        VECTOR2I pos = static_cast<PNS::SOLID*>( aItem )->Pos();

        m_fpOffsets[ pad ].p_new = pos;
        return;
    }

    default:
        break;
    }

    if( newBI )
    {
        //newBI->SetLocalRatsnestVisible( m_dispOptions->m_ShowGlobalRatsnest );
        aItem->SetParent( newBI );
        newBI->ClearFlags();

        m_commit->Add( newBI );
    }
}


void PNS_KICAD_IFACE::Commit()
{
    std::set<FOOTPRINT*> processedFootprints;

    EraseView();

    for( const std::pair<const PAD*, OFFSET>& fpOffset : m_fpOffsets )
    {
        VECTOR2I   offset = fpOffset.second.p_new - fpOffset.second.p_old;
        FOOTPRINT* footprint = fpOffset.first->GetParent();
        VECTOR2I   p_orig = footprint->GetPosition();
        VECTOR2I   p_new = p_orig + offset;

        if( processedFootprints.find( footprint ) != processedFootprints.end() )
            continue;

        processedFootprints.insert( footprint );
        m_commit->Modify( footprint );
        footprint->SetPosition( p_new );
    }

    m_fpOffsets.clear();

    m_commit->Push( _( "Interactive Router" ), m_commitFlags );
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

    auto dec = new PNS_PCBNEW_DEBUG_DECORATOR();
    m_debugDecorator = dec;

    dec->SetDebugEnabled( ADVANCED_CFG::GetCfg().m_ShowRouterDebugGraphics );

    if( ADVANCED_CFG::GetCfg().m_ShowRouterDebugGraphics )
        dec->SetView( m_view );
}


void PNS_KICAD_IFACE::UpdateNet( int aNetCode )
{
    wxLogTrace( wxT( "PNS" ), wxT( "Update-net %d" ), aNetCode );
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
