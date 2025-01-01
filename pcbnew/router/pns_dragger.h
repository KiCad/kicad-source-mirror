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

#ifndef __PNS_DRAGGER_H
#define __PNS_DRAGGER_H

#include <memory>
#include <math/vector2d.h>

#include "pns_node.h"
#include "pns_via.h"
#include "pns_line.h"
#include "pns_drag_algo.h"
#include "pns_itemset.h"
#include "pns_layerset.h"
#include "pns_mouse_trail_tracer.h"

namespace PNS {

class ROUTER;
class SHOVE;
class OPTIMIZER;

/**
 * DRAGGER
 *
 * Via, segment and corner dragging algorithm.
 */
class DRAGGER : public DRAG_ALGO
{
public:
     DRAGGER( ROUTER* aRouter );
    ~DRAGGER();

    /**
     * Function Start()
     *
     * Starts routing a single track at point aP, taking item aStartItem as anchor
     * (unless NULL). Returns true if a dragging operation has started.
     */
    virtual bool Start( const VECTOR2I& aP, ITEM_SET& aPrimitives ) override;

    /**
     * Function Drag()
     *
     * Drags the current segment/corner/via to the point aP.
     * @return true, if dragging finished with success.
     */
    bool Drag( const VECTOR2I& aP ) override;

    /**
     * Function FixRoute()
     *
     * Checks if the result of current dragging operation is correct
     * and eventually commits it to the world.
     * @return true, if dragging finished with success.
     */
    bool FixRoute( bool aForceCommit ) override;

    /**
     * Function CurrentNode()
     *
     * Returns the most recent world state, including all
     * items changed due to dragging operation.
     */
    NODE* CurrentNode() const override;

    /**
     * Function CurrentNets()
     *
     * Returns the net(s) of currently routed track(s).
     */
    const std::vector<NET_HANDLE> CurrentNets() const override;

    /**
     * Function CurrentLayer()
     *
     * Returns the layer of currently routed track.
     */
    int CurrentLayer() const override
    {
        return m_draggedLine.Layer();
    }

    const LINE& GetOriginalLine()
    {
        return m_draggedLine;
    }

    const LINE& GetLastDragSolution()
    {
        return m_lastDragSolution;
    }

    /**
     * Function Traces()
     *
     * Returns the set of dragged items.
     */
    const ITEM_SET Traces() override;

    void SetMode( PNS::DRAG_MODE aDragMode ) override;

    PNS::DRAG_MODE Mode() const override;

    bool GetForceMarkObstaclesMode( bool* aDragStatus ) const override
    {
        *aDragStatus = m_dragStatus;
        return m_forceMarkObstaclesMode;
    }

private:
    const ITEM_SET findViaFanoutByHandle ( NODE *aNode, const VIA_HANDLE& handle );

    bool dragMarkObstacles( const VECTOR2I& aP );
    bool dragShove(const VECTOR2I& aP );
    bool dragWalkaround(const VECTOR2I& aP );
    bool startDragSegment( const VECTOR2D& aP, SEGMENT* aSeg );
    bool startDragArc( const VECTOR2D& aP, ARC* aArc );
    bool startDragVia( VIA* aVia );
    bool dragViaMarkObstacles( const VIA_HANDLE& aHandle, NODE* aNode, const VECTOR2I& aP );
    bool dragViaWalkaround( const VIA_HANDLE& aHandle, NODE* aNode, const VECTOR2I& aP );
    void optimizeAndUpdateDraggedLine( LINE& aDragged, const LINE& aOrig, const VECTOR2I& aP );
    bool propagateViaForces( NODE* node, std::set<VIA*>& vias );
    bool tryWalkaround( NODE* aNode, LINE& aOrig, LINE& aWalk );
    VVIA* checkVirtualVia( const VECTOR2D& aP, SEGMENT* aSeg );


    VIA_HANDLE             m_initialVia;
    VIA_HANDLE             m_draggedVia;

    NODE*                  m_lastNode;
    NODE*                  m_preDragNode;
    int                    m_mode;
    LINE                   m_draggedLine;
    LINE                   m_lastDragSolution;
    std::unique_ptr<SHOVE> m_shove;
    int                    m_draggedSegmentIndex;
    bool                   m_dragStatus;
    PNS_MODE               m_currentMode;
    ITEM_SET               m_origViaConnections;
    VECTOR2D               m_lastValidPoint;

    ///< Contains the list of items that are currently modified by the dragger
    ITEM_SET               m_draggedItems;

    ///< If true, moves the connection lines without maintaining 45 degrees corners
    bool                   m_freeAngleMode;
    bool                   m_forceMarkObstaclesMode;
    MOUSE_TRAIL_TRACER     m_mouseTrailTracer;
};

}

#endif
