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

#ifndef __PNS_DRAGGER_H
#define __PNS_DRAGGER_H

#include <math/vector2d.h>

#include "pns_node.h"
#include "pns_via.h"
#include "pns_line.h"
#include "pns_algo_base.h"
#include "pns_itemset.h"

class PNS_ROUTER;
class PNS_SHOVE;
class PNS_OPTIMIZER;
class PNS_ROUTER_BASE;

/**
 * Class PNS_DRAGGER
 *
 * Via, segment and corner dragging algorithm.
 */
class PNS_DRAGGER : public PNS_ALGO_BASE
{
public:
     PNS_DRAGGER( PNS_ROUTER* aRouter );
    ~PNS_DRAGGER();

    /**
     * Function SetWorld()
     *
     * Sets the board to work on.
     */
    void SetWorld( PNS_NODE* aWorld );

    /**
     * Function Start()
     *
     * Starts routing a single track at point aP, taking item aStartItem as anchor
     * (unless NULL). Returns true if a dragging operation has started.
     */
    bool Start( const VECTOR2I& aP, PNS_ITEM* aStartItem );

    /**
     * Function Drag()
     *
     * Drags the current segment/corner/via to the point aP.
     * @return true, if dragging finished with success.
     */
    bool Drag( const VECTOR2I& aP );

    /**
     * Function FixRoute()
     *
     * Checks if the result of current dragging operation is correct
     * and eventually commits it to the world.
     * @return true, if dragging finished with success.
     */
    bool FixRoute();

    /**
     * Function CurrentNode()
     *
     * Returns the most recent world state, including all
     * items changed due to dragging operation.
     */
    PNS_NODE* CurrentNode() const;

    /**
     * Function Traces()
     *
     * Returns the set of dragged items.
     */
    const PNS_ITEMSET Traces();

    /// @copydoc PNS_ALGO_BASE::Logger()
    virtual PNS_LOGGER* Logger();

private:
    typedef std::pair<PNS_LINE*, PNS_LINE*> LinePair;
    typedef std::vector<LinePair> LinePairVec;

    enum DragMode {
        CORNER = 0,
        SEGMENT,
        VIA
    };

    bool dragMarkObstacles( const VECTOR2I& aP );
    bool dragShove(const VECTOR2I& aP );
    bool startDragSegment( const VECTOR2D& aP, PNS_SEGMENT* aSeg );
    bool startDragVia( const VECTOR2D& aP, PNS_VIA* aVia );
    void dumbDragVia( PNS_VIA* aVia, PNS_NODE* aNode, const VECTOR2I& aP );

    PNS_NODE*   m_world;
    PNS_NODE*   m_lastNode;
    DragMode    m_mode;
    PNS_LINE    m_draggedLine;
    PNS_VIA*    m_draggedVia;
    PNS_LINE    m_lastValidDraggedLine;
    PNS_SHOVE*  m_shove;
    int         m_draggedSegmentIndex;
    bool        m_dragStatus;
    PNS_MODE    m_currentMode;
    PNS_ITEMSET m_origViaConnections;
    PNS_ITEMSET m_draggedViaConnections;
    PNS_VIA*                m_initialVia;
    PNS_ITEMSET             m_draggedItems;
};

#endif
