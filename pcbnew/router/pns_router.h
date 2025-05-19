/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
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

#ifndef __PNS_ROUTER_H
#define __PNS_ROUTER_H

#include <list>
#include <memory>
#include <optional>
#include <math/box2.h>

#include "pns_routing_settings.h"
#include "pns_sizes_settings.h"
#include "pns_node.h"

namespace KIGFX
{

class VIEW;
class VIEW_GROUP;

}

namespace PNS {

class DEBUG_DECORATOR;
class NODE;
class DIFF_PAIR_PLACER;
class PLACEMENT_ALGO;
class LINE_PLACER;
class ITEM;
class ARC;
class LINE;
class SOLID;
class SEGMENT;
class JOINT;
class VIA;
class RULE_RESOLVER;
class SHOVE;
class DRAGGER;
class DRAG_ALGO;
class LOGGER;

enum ROUTER_MODE {
    PNS_MODE_ROUTE_SINGLE = 1,
    PNS_MODE_ROUTE_DIFF_PAIR,
    PNS_MODE_TUNE_SINGLE,
    PNS_MODE_TUNE_DIFF_PAIR,
    PNS_MODE_TUNE_DIFF_PAIR_SKEW
};

enum DRAG_MODE
{
    DM_CORNER = 0x1,
    DM_SEGMENT = 0x2,
    DM_VIA = 0x4,
    DM_FREE_ANGLE = 0x8,
    DM_ARC = 0x10,
    DM_ANY = 0x17,
    DM_COMPONENT = 0x20
};
/**
 * ROUTER
 *
 * Main router class.
 */

 class ROUTER_IFACE
 {
 public:
    ROUTER_IFACE() {};
    virtual ~ROUTER_IFACE() {};

    virtual void SyncWorld( NODE* aNode ) = 0;
    virtual void AddItem( ITEM* aItem ) = 0;
    virtual void UpdateItem( ITEM* aItem ) = 0;
    virtual void RemoveItem( ITEM* aItem ) = 0;
    virtual bool IsAnyLayerVisible( const PNS_LAYER_RANGE& aLayer ) const = 0;
    virtual bool IsItemVisible( const PNS::ITEM* aItem ) const = 0;
    virtual bool IsFlashedOnLayer( const PNS::ITEM* aItem, int aLayer ) const = 0;
    virtual bool IsFlashedOnLayer( const PNS::ITEM* aItem, const PNS_LAYER_RANGE& aLayer ) const = 0;
    virtual bool IsPNSCopperLayer( int aPNSLayer ) const = 0;
    virtual void DisplayItem( const ITEM* aItem, int aClearance, bool aEdit = false,
                                    int aFlags = 0 ) = 0;
    virtual void DisplayPathLine( const SHAPE_LINE_CHAIN& aLine, int aImportance ) = 0;
    virtual void DisplayRatline( const SHAPE_LINE_CHAIN& aRatline, NET_HANDLE aNetCode ) = 0;
    virtual void HideItem( ITEM* aItem ) = 0;
    virtual void Commit() = 0;
    virtual bool       ImportSizes( SIZES_SETTINGS& aSizes, ITEM* aStartItem, NET_HANDLE aNet,
                                    VECTOR2D aStartPosition ) = 0;
    virtual int  StackupHeight( int aFirstLayer, int aSecondLayer ) const = 0;
    virtual void EraseView() = 0;
    virtual int GetNetCode( NET_HANDLE aNet ) const = 0;
    virtual wxString GetNetName( PNS::NET_HANDLE aNet ) const = 0;
    virtual void UpdateNet( NET_HANDLE aNet ) = 0;
    virtual NET_HANDLE GetOrphanedNetHandle() = 0;
    virtual PNS::NODE* GetWorld() const = 0;

    virtual RULE_RESOLVER* GetRuleResolver() = 0;
    virtual DEBUG_DECORATOR* GetDebugDecorator() = 0;

    virtual long long int CalculateRoutedPathLength( const ITEM_SET& aLine, const SOLID* aStartPad,
                                                     const SOLID* aEndPad, const NETCLASS* aNetClass ) = 0;
    virtual int64_t       CalculateRoutedPathDelay( const ITEM_SET& aLine, const SOLID* aStartPad, const SOLID* aEndPad,
                                                    const NETCLASS* aNetClass ) = 0;
    virtual int64_t       CalculateLengthForDelay( int64_t aDesiredDelay, int aWidth, bool aIsDiffPairCoupled,
                                                   int aDiffPairCouplingGap, int aPNSLayer, const NETCLASS* aNetClass ) = 0;
    virtual int64_t       CalculateDelayForShapeLineChain( const SHAPE_LINE_CHAIN& aShape, int aWidth,
                                                           bool aIsDiffPairCoupled, int aDiffPairCouplingGap, int aPNSLayer,
                                                           const NETCLASS* aNetClass ) = 0;
    virtual PCB_LAYER_ID GetBoardLayerFromPNSLayer( int aLayer ) const = 0;
    virtual int GetPNSLayerFromBoardLayer( PCB_LAYER_ID aLayer ) const = 0;
};

class ROUTER
{
public:
    enum RouterState
    {
        IDLE,
        DRAG_SEGMENT,
        DRAG_COMPONENT,
        ROUTE_TRACK
    };

public:
    ROUTER();
    ~ROUTER();

    void SetInterface( ROUTER_IFACE* aIface );
    void SetMode ( ROUTER_MODE aMode );
    ROUTER_MODE Mode() const { return m_mode; }

    RouterState GetState() const { return m_state; }

    DRAG_ALGO* GetDragger() { return m_dragger.get(); }

    static ROUTER* GetInstance();

    void ClearWorld();
    void SyncWorld();

    bool RoutingInProgress() const;
    bool StartRouting( const VECTOR2I& aP, ITEM* aItem, int aLayer );
    bool Move( const VECTOR2I& aP, ITEM* aItem );
    bool Finish();
    bool ContinueFromEnd( ITEM** aNewStartItem );
    bool FixRoute( const VECTOR2I& aP, ITEM* aItem, bool aForceFinish, bool aForceCommit );
    void BreakSegmentOrArc( ITEM *aItem, const VECTOR2I& aP );

    std::optional<VECTOR2I> UndoLastSegment();
    void CommitRouting();

    void GetUpdatedItems( std::vector<PNS::ITEM*>& aRemoved, std::vector<PNS::ITEM*>& aAdded,
                          std::vector<PNS::ITEM*>& aHeads );

    void StopRouting();
    void ClearViewDecorations();

    NODE* GetWorld() const { return m_world.get(); }

    void FlipPosture();

    bool SwitchLayer( int layer );

    void ToggleViaPlacement();
    void SetOrthoMode( bool aEnable );

    void ToggleCornerMode();

    int GetCurrentLayer() const;
    const std::vector<NET_HANDLE> GetCurrentNets() const;

    LOGGER* Logger();

    RULE_RESOLVER* GetRuleResolver() const { return m_iface->GetRuleResolver(); }

    bool IsPlacingVia() const;

    const ITEM_SET QueryHoverItems( const VECTOR2I& aP, int aSlopRadius = 0 );

    bool StartDragging( const VECTOR2I& aP, ITEM* aItem, int aDragMode = DM_ANY );
    bool StartDragging( const VECTOR2I& aP, ITEM_SET aItems, int aDragMode = DM_COMPONENT );

    void SetIterLimit( int aX ) { m_iterLimit = aX; }
    int GetIterLimit() const { return m_iterLimit; };

    ROUTING_SETTINGS& Settings() { return *m_settings; }

    void CommitRouting( NODE* aNode );

    /**
     * Applies stored settings.
     * @see Settings()
     */
    void UpdateSizes( const SIZES_SETTINGS& aSizes );

    /**
     * Changes routing settings to ones passed in the parameter.
     * @param aSettings are the new settings.
     */
    void LoadSettings( ROUTING_SETTINGS* aSettings )
    {
        m_settings = aSettings;
    }

    SIZES_SETTINGS& Sizes() { return m_sizes; }

    void SetFailureReason( const wxString& aReason ) { m_failureReason = aReason; }
    const wxString& FailureReason() const { return m_failureReason; }

    PLACEMENT_ALGO* Placer() { return m_placer.get(); }

    ROUTER_IFACE* GetInterface() const { return m_iface; }

    void SetVisibleViewArea( const BOX2I& aExtents ) { m_visibleViewArea = aExtents; }
    const BOX2I& VisibleViewArea() const { return m_visibleViewArea; }

    std::vector<PNS::ITEM*> GetLastCommittedLeaderSegments();

    bool GetNearestRatnestAnchor( VECTOR2I& aOtherEnd, PNS_LAYER_RANGE& aOtherEndLayers,
                                  ITEM*& aOtherEndItem );
private:
    bool movePlacing( const VECTOR2I& aP, ITEM* aItem );
    bool moveDragging( const VECTOR2I& aP, ITEM* aItem );

    void updateView( NODE* aNode, ITEM_SET& aCurrent, bool aDragging = false );

    // optHoverItem queryHoverItemEx(const VECTOR2I& aP);

    void markViolations( NODE* aNode, ITEM_SET& aCurrent, NODE::ITEM_VECTOR& aRemoved );
    bool isStartingPointRoutable( const VECTOR2I& aWhere, ITEM* aItem, int aLayer );


private:
    BOX2I                           m_visibleViewArea;
    RouterState                     m_state;

    std::unique_ptr<NODE>           m_world;
    NODE*                           m_lastNode;

    std::unique_ptr<PLACEMENT_ALGO> m_placer;
    std::unique_ptr<DRAG_ALGO>      m_dragger;
    std::unique_ptr<SHOVE>          m_shove;
    std::vector<PNS::ITEM*>         m_leaderSegments;

    ROUTER_IFACE*     m_iface;

    int               m_iterLimit;

    ROUTING_SETTINGS* m_settings;
    SIZES_SETTINGS    m_sizes;
    ROUTER_MODE       m_mode;
    LOGGER*           m_logger;

    wxString          m_toolStatusbarName;
    wxString          m_failureReason;
};

}

#endif
