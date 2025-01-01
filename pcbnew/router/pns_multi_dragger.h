/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
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

#ifndef __PNS_MULTI_DRAGGER_H
#define __PNS_MULTI_DRAGGER_H

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
 * MULTI_DRAGGER
 *
 * Dragging algorithm for multiple segments. Very trival version for demonstration purposes.
 */
class MULTI_DRAGGER : public DRAG_ALGO
{
public:
     MULTI_DRAGGER( ROUTER* aRouter );
    ~MULTI_DRAGGER();

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
     * Returns the net code(s) of currently routed track(s).
     */
    const std::vector<NET_HANDLE> CurrentNets() const override;

    /**
     * Function CurrentLayer()
     *
     * Returns the layer of currently routed track.
     */
    int CurrentLayer() const override;

    /**
     * Function Traces()
     *
     * Returns the set of dragged items.
     */
    const ITEM_SET Traces() override;

    void SetMode( PNS::DRAG_MODE aDragMode ) override;

    PNS::DRAG_MODE Mode() const override;

// Use case: we are dragging multiple tracks. The router erases a few of them, adds a few new ones. For the ease of use, it would be good for the tracks the be still selected when
// the drag operation is completed. This method returns a set of the 'leader' (segments/arcs that have been selected for multi-fragging)
    virtual std::vector<PNS::ITEM*> GetLastCommittedLeaderSegments() override { return m_leaderSegments; };

    virtual bool GetForceMarkObstaclesMode( bool* aDragStatus ) const override
    {
        *aDragStatus = m_dragStatus;
        return false;
    }



private:




    struct MDRAG_LINE
    {

        ITEM* leaderItem = nullptr;
        std::vector<PNS::ITEM*> originalLeaders;

        bool isStrict = false;
        bool isMidSeg = false;
        bool isCorner = false;
        bool isDraggable = false;

        int leaderSegIndex = -1;
        bool cornerIsLast  = false;

        PNS::LINE originalLine; // complete line (in a bundle) to drag
        PNS::LINE preDragLine; // complete line (in a bundle) to drag
        PNS::LINE draggedLine; // result of the drag calculation
        PNS::LINE preShoveLine; // result of the drag calculation

        bool dragOK        = false;
        bool isPrimaryLine = false; // when true, it's the "leader"/"primary one" - the one the cursor is attached to
        bool clipDone      = false;
        int  offset        = 0; // distance between this line and the primary one (only applicable if the respective end segments are parallel)
        SEG midSeg;
//        VECTOR2I dragAnchor;
        int dragDist          = 0;
        int cornerDistance    = 0;
        int leaderSegDistance = 0;
    };

    bool multidragMarkObstacles ( std::vector<MDRAG_LINE>& aCompletedLines );
    bool multidragShove ( std::vector<MDRAG_LINE>& aCompletedLines );
    bool multidragWalkaround ( std::vector<MDRAG_LINE>& aCompletedLines );
    void restoreLeaderSegments( std::vector<MDRAG_LINE>& aCompletedLines );
    int findNewLeaderSegment( const MDRAG_LINE& aLine ) const;
    bool tryWalkaround( NODE* aNode, LINE& aOrig, LINE& aWalk );


    int                     m_mode;
    bool                    m_dragStatus;
    PNS_MODE                m_currentMode;
    DRAG_MODE               m_dragMode;
    std::vector<MDRAG_LINE> m_mdragLines;
    std::vector<PNS::ITEM*> m_leaderSegments;
    NODE* m_lastNode;
    NODE* m_preShoveNode;
    ITEM_SET                m_origDraggedItems;
    ITEM_SET                m_draggedItems;
    VECTOR2I m_dragStartPoint;
    SEG m_guide;
    std::unique_ptr<SHOVE> m_shove;

};

}

#endif
