/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
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

#include <boost/optional.hpp>
#include <boost/unordered_set.hpp>

#include <geometry/shape_line_chain.h>
#include <class_undoredo_container.h>

#include "pns_routing_settings.h"
#include "pns_item.h"
#include "pns_itemset.h"
#include "pns_node.h"

class BOARD;
class BOARD_ITEM;
class D_PAD;
class TRACK;
class VIA;
class PNS_NODE;
class PNS_LINE_PLACER;
class PNS_ITEM;
class PNS_LINE;
class PNS_SOLID;
class PNS_SEGMENT;
class PNS_JOINT;
class PNS_VIA;
class PNS_CLEARANCE_FUNC;
class PNS_SHOVE;
class PNS_DRAGGER;

namespace KIGFX
{
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
        DRAG_SEGMENT,
        ROUTE_TRACK
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
    bool StartRouting( const VECTOR2I& aP, PNS_ITEM* aItem );
    void Move( const VECTOR2I& aP, PNS_ITEM* aItem );
    bool FixRoute( const VECTOR2I& aP, PNS_ITEM* aItem );

    void StopRouting();

    const VECTOR2I CurrentEnd() const;

    int GetClearance( const PNS_ITEM* aA, const PNS_ITEM* aB ) const;

    PNS_NODE* GetWorld() const
    {
        return m_world;
    }

    void FlipPosture();

    void DisplayItem( const PNS_ITEM* aItem, int aColor = -1, int aClearance = -1 );
    void DisplayItems( const PNS_ITEMSET& aItems );
    
    void DisplayDebugLine( const SHAPE_LINE_CHAIN& aLine, int aType = 0, int aWidth = 0 );
    void DisplayDebugPoint( const VECTOR2I aPos, int aType = 0 );
    void DisplayDebugBox( const BOX2I& aBox, int aType = 0, int aWidth = 0 );

    void SwitchLayer( int layer );

    void ToggleViaPlacement();

    int GetCurrentLayer() const;
    int GetCurrentNet() const;

    void DumpLog();
  
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
    const VECTOR2I      SnapToItem( PNS_ITEM* aItem, VECTOR2I aP, bool& aSplitsSegment );

    bool StartDragging( const VECTOR2I& aP, PNS_ITEM* aItem );

    void SetIterLimit( int aX ) { m_iterLimit = aX; }
    int GetIterLimit() const { return m_iterLimit; };

    void SetShowIntermediateSteps( bool aX, int aSnapshotIter = -1 )
    {
        m_showInterSteps = aX;
        m_snapshotIter = aSnapshotIter;
    }

    bool GetShowIntermediateSteps() const { return m_showInterSteps; }
    int GetShapshotIter() const { return m_snapshotIter; }

    PNS_ROUTING_SETTINGS& Settings() { return m_settings; }
    
    void CommitRouting( PNS_NODE* aNode );
    
    /**
     * Returns the last changes introduced by the router (since the last time ClearLastChanges()
     * was called or a new track has been started).
     */
    const PICKED_ITEMS_LIST& GetUndoBuffer() const
    {
        return m_undoBuffer;
    }

    /**
     * Clears the list of recent changes, saved to be stored in the undo buffer.
     */
    void ClearUndoBuffer()
    {
        m_undoBuffer.ClearItemsList();
    }

    /**
     * Applies stored settings.
     * @see Settings()
     */
    void ApplySettings();

    /**
     * Changes routing settings to ones passed in the parameter.
     * @param aSettings are the new settings.
     */
    void LoadSettings( const PNS_ROUTING_SETTINGS& aSettings )
    {
        m_settings = aSettings;

        ApplySettings();
    }

    void EnableSnapping( bool aEnable )
    {
        m_snappingEnabled = aEnable;
    }

    bool SnappingEnabled() const
    {
        return m_snappingEnabled;
    }

private:
    void movePlacing( const VECTOR2I& aP, PNS_ITEM* aItem );
    void moveDragging( const VECTOR2I& aP, PNS_ITEM* aItem );
    
    void eraseView();
    void updateView( PNS_NODE* aNode, PNS_ITEMSET& aCurrent );
    
    void clearViewFlags();

    // optHoverItem queryHoverItemEx(const VECTOR2I& aP);

    PNS_ITEM* pickSingleItem( PNS_ITEMSET& aItems ) const;
    void splitAdjacentSegments( PNS_NODE* aNode, PNS_ITEM* aSeg, const VECTOR2I& aP );
    PNS_VIA* checkLoneVia( PNS_JOINT* aJoint ) const;

    PNS_ITEM* syncPad( D_PAD* aPad );
    PNS_ITEM* syncTrack( TRACK* aTrack );
    PNS_ITEM* syncVia( VIA* aVia );

    void commitPad( PNS_SOLID* aPad );
    void commitSegment( PNS_SEGMENT* aTrack );
    void commitVia( PNS_VIA* aVia );

    void highlightCurrent( bool enabled );

    void markViolations( PNS_NODE* aNode, PNS_ITEMSET& aCurrent,  PNS_NODE::ITEM_VECTOR& aRemoved );
    
    int m_currentLayer;
    int m_currentNet;

    RouterState m_state;

    BOARD* m_board;
    PNS_NODE* m_world;
    PNS_NODE* m_lastNode;
    PNS_LINE_PLACER* m_placer;
    PNS_DRAGGER* m_dragger;
    PNS_LINE* m_draggedLine;
    PNS_SHOVE* m_shove;
    int m_draggedSegmentIndex;
    int m_iterLimit;
    bool m_showInterSteps;
    int m_snapshotIter;

    KIGFX::VIEW* m_view;
    KIGFX::VIEW_GROUP* m_previewItems;

    PNS_ITEM* m_currentEndItem;
    
    VECTOR2I m_currentEnd;
    VECTOR2I m_currentStart;
    VECTOR2I m_originalStart;
    bool m_placingVia;
    bool m_startsOnVia;
    bool m_snappingEnabled;
    bool m_violation;

    // optHoverItem m_startItem, m_endItem;

    PNS_ROUTING_SETTINGS m_settings;
    PNS_CLEARANCE_FUNC* m_clearanceFunc;

    boost::unordered_set<BOARD_CONNECTED_ITEM*> m_hiddenItems;

    ///> Stores list of modified items in the current operation
    PICKED_ITEMS_LIST m_undoBuffer;
};

#endif
