/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013  CERN
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
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
 */

#ifndef __PNS_ROUTER_H
#define __PNS_ROUTER_H

#include <list>

#include <boost/optional.hpp>
#include <boost/unordered_set.hpp>

#include <geometry/shape_line_chain.h>
#include <class_undoredo_container.h>

#include "pns_routing_settings.h"
#include "pns_item.h"
#include "pns_itemset.h"

class BOARD;
class BOARD_ITEM;
class D_PAD;
class TRACK;
class SEGVIA;
class PNS_NODE;
class PNS_LINE_PLACER;
class PNS_ITEM;
class PNS_LINE;
class PNS_SOLID;
class PNS_SEGMENT;
class PNS_JOINT;
class PNS_VIA;
class PNS_CLEARANCE_FUNC;
class VIEW_GROUP;

namespace KIGFX {
class VIEW;
class VIEW_GROUP;
};


/**
 * Class PNS_ROUTER
 *
 * Main router class.
 */

class PNS_ROUTER
{
private:
    enum RouterState
    {
        IDLE,
        START_ROUTING,
        ROUTE_TRACK,
        FINISH_TRACK
    };

public:
    PNS_ROUTER();
    ~PNS_ROUTER();

    static PNS_ROUTER* GetInstance();

    void ClearWorld();
    void SetBoard( BOARD* aBoard );
    void SyncWorld();

    void SetView( KIGFX::VIEW* aView );

    bool RoutingInProgress() const;
    void StartRouting( const VECTOR2I& aP, PNS_ITEM* aItem );
    void Move( const VECTOR2I& aP, PNS_ITEM* aItem );
    bool FixRoute( const VECTOR2I& aP, PNS_ITEM* aItem );

    void StopRouting();

    const VECTOR2I GetCurrentEnd() const;

    int GetClearance( const PNS_ITEM* a, const PNS_ITEM* b ) const;

    PNS_NODE* GetWorld() const
    {
        return m_world;
    }

    void FlipPosture();

    void DisplayItem( const PNS_ITEM* aItem, bool aIsHead = false );
    void DisplayDebugLine( const SHAPE_LINE_CHAIN& aLine, int aType = 0, int aWidth = 0 );
    void DisplayDebugBox( const BOX2I& aBox, int aType = 0, int aWidth = 0 );

    void EraseView();
    void SwitchLayer( int layer );

    int GetCurrentLayer() const { return m_currentLayer; }
    void ToggleViaPlacement();

    void SetCurrentWidth( int w );

    void SetCurrentViaDiameter( int d ) { m_currentViaDiameter = d; }
    void SetCurrentViaDrill( int d ) { m_currentViaDrill = d; }
    int GetCurrentWidth() const { return m_currentWidth; }
    int GetCurrentViaDiameter() const { return m_currentViaDiameter; }
    int GetCurrentViaDrill() const { return m_currentViaDrill; }
    int GetCurrentNet() const { return m_currentNet; }

    PNS_CLEARANCE_FUNC* GetClearanceFunc() const
    {
        return m_clearanceFunc;
    }

    bool IsPlacingVia() const
    {
        return m_placingVia;
    }

    int NextCopperLayer( bool aUp );

    // typedef boost::optional<hoverItem> optHoverItem;

    const PNS_ITEMSET   QueryHoverItems( const VECTOR2I& aP );
    const VECTOR2I      SnapToItem( PNS_ITEM* item, VECTOR2I aP, bool& aSplitsSegment );

    /**
     * Returns the last changes introduced by the router (since the last time ClearLastChanges()
     * was called or a new track has been started).
     */
    const PICKED_ITEMS_LIST& GetLastChanges() const
    {
        return m_undoBuffer;
    }

    /**
     * Clears the list of recent changes, saved to be stored in the undo buffer.
     */
    void ClearLastChanges()
    {
        m_undoBuffer.ClearItemsList();
    }

private:
    void clearViewFlags();

    // optHoverItem queryHoverItemEx(const VECTOR2I& aP);

    PNS_ITEM* pickSingleItem( PNS_ITEMSET& aItems ) const;                               // std::vector<PNS_ITEM*> aItems) const;
    void splitAdjacentSegments( PNS_NODE* aNode, PNS_ITEM* aSeg, const VECTOR2I& aP );   // optHoverItem& aItem);
    void commitRouting( PNS_NODE* aNode );
    PNS_NODE* removeLoops( PNS_NODE* aNode, PNS_SEGMENT* aLatestSeg );
    PNS_NODE* removeLoops( PNS_NODE* aNode, PNS_LINE* aNewLine );
    PNS_VIA* checkLoneVia( PNS_JOINT* aJoint ) const;

    PNS_ITEM* syncPad( D_PAD* aPad );
    PNS_ITEM* syncTrack( TRACK* aTrack );
    PNS_ITEM* syncVia( SEGVIA* aVia );

    void commitPad( PNS_SOLID* aPad );
    void commitSegment( PNS_SEGMENT* aTrack );
    void commitVia( PNS_VIA* aVia );

    void highlightCurrent( bool enabled );

    int m_currentLayer;
    int m_currentNet;
    int m_currentWidth;
    int m_currentViaDiameter;
    int m_currentViaDrill;

    bool m_start_diagonal;

    RouterState m_state;

    BOARD* m_board;
    PNS_NODE* m_world;
    PNS_LINE_PLACER* m_placer;

    KIGFX::VIEW* m_view;
    KIGFX::VIEW_GROUP* m_previewItems;

    VECTOR2I m_currentEnd;
    VECTOR2I m_currentStart;
    VECTOR2I m_originalStart;
    bool m_placingVia;
    bool m_startsOnVia;

// optHoverItem m_startItem, m_endItem;

    PNS_ROUTING_SETTINGS m_settings;
    PNS_CLEARANCE_FUNC* m_clearanceFunc;

    boost::unordered_set<BOARD_ITEM*> m_hiddenItems;

    ///> Stores list of modified items in the current operation
    PICKED_ITEMS_LIST m_undoBuffer;
};

#endif
