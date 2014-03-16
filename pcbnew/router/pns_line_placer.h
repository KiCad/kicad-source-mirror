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

#ifndef __PNS_LINE_PLACER_H
#define __PNS_LINE_PLACER_H

#include <math/vector2d.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "pns_node.h"
#include "pns_via.h"
#include "pns_line.h"
#include "pns_routing_settings.h"

class PNS_ROUTER;
class PNS_SHOVE;
class PNS_OPTIMIZER;
class PNS_ROUTER_BASE;

/**
 * Class PNS_LINE_PLACER
 *
 * Interactively routes a single track. Runs shove and walkaround
 * algorithms when needed.
 */

class PNS_LINE_PLACER
{
public:
    PNS_LINE_PLACER( PNS_NODE* aWorld );
    ~PNS_LINE_PLACER();

    ///> Appends a via at the end of currently placed line.
    void AddVia( bool aEnabled, int aDiameter, int aDrill )
    {
        m_viaDiameter = aDiameter;
        m_viaDrill = aDrill;
        m_placingVia = aEnabled;
    }

    ///> Starts placement of a line at point aStart.
    void StartPlacement( const VECTOR2I& aStart, int aNet, int aWidth, int aLayer );

    /**
     * Function Route()
     *
     * Re-routes the current track to point aP. Returns true, when routing has
     * completed successfully (i.e. the trace end has reached point aP), and false
     * if the trace was stuck somewhere on the way. May call routeStep()
     * repetitively due to mouse smoothing.
     * @param aP ending point of current route.
     * @return true, if the routing is complete.
     */
    bool Route( const VECTOR2I& aP );

    ///> Sets initial routing direction/posture
    void SetInitialDirection( const DIRECTION_45& aDirection );

    void ApplySettings( const PNS_ROUTING_SETTINGS& aSettings );

    ///> Returns the "head" of the line being placed, that is the volatile part
    ///> that has not been settled yet
    const PNS_LINE& GetHead() const { return m_head; }
    ///> Returns the "tail" of the line being placed the part that has been
    ///> fixed already (follow mouse mode only)
    const PNS_LINE& GetTail() const { return m_tail; }

    ///> Returns the whole routed line
    const PNS_LINE GetTrace() const;

    ///> Returns the current end of the line being placed. It may not be equal
    ///> to the cursor position due to collisions.
    const VECTOR2I& CurrentEnd() const
    {
        if( m_head.GetCLine().PointCount() > 0 )
            return m_head.GetCLine().CPoint( -1 );
        else if( m_tail.GetCLine().PointCount() > 0 )
            return m_tail.GetCLine().CPoint( -1 );
        else
            return m_p_start;
    }

    ///> Returns all items in the world that have been affected by the routing
    ///> operation. Used to update data structures of the host application
    void GetUpdatedItems( PNS_NODE::ItemVector& aRemoved,
                          PNS_NODE::ItemVector& aAdded );

    ///> Toggles the current posture (straight/diagonal) of the trace head.
    void FlipPosture();

    ///> Returns the most recent world state
    PNS_NODE* GetCurrentNode() const;

private:
    static const double m_shoveLengthThreshold;

    bool handleViaPlacement( PNS_LINE& aHead );

    /**
     * Function checkObtusity()
     *
     * Helper that checks if segments a and b form an obtuse angle
     * (in 45-degree regime).
     * @return true, if angle (a, b) is obtuse
     */
    bool checkObtusity( const SEG& a, const SEG& b ) const;

    /**
     * Function handleSelfIntersections()
     *
     * Checks if the head of the track intersects its tail. If so, cuts the
     * tail up to the intersecting segment and fixes the head direction to match
     * the last segment before the cut.
     * @return true if the line has been changed.
     */
    bool handleSelfIntersections();

    /**
     * Function handlePullback()
     *
     * Deals with pull-back: reduces the tail if head trace is moved backwards
     * wrs to the current tail direction.
     * @return true if the line has been changed.
     */
    bool handlePullback();

    /**
     * Function mergeHead()
     *
     * Moves "estabished" segments from the head to the tail if certain
     * conditions are met.
     * @return true, if the line has been changed.
     */
    bool mergeHead();

    /**
     * Function reduceTail()
     *
     * Attempts to reduce the numer of segments in the tail by trying to replace a
     * certain number of latest tail segments with a direct trace leading to aEnd
     * that does not collide with anything.
     * @param aEnd: current routing destination point.
     * @return true if the line has been changed.
     */
    bool reduceTail( const VECTOR2I& aEnd );

    void fixHeadPosture();

    /**
     * Function optimizeTailHeadTransition()
     *
     * Tries to reduce the corner count of the most recent part of tail/head by
     * merging obtuse/collinear segments.
     * @return true, if the line has been changed.
     */
    bool optimizeTailHeadTransition();

    /**
     * Function routeHead()
     *
     * Computes the head trace between the current start point (m_p_start) and
     * point aP, starting with direction defined in m_direction. The trace walks
     * around all colliding solid or non-movable items. Movable segments are
     * ignored, as they'll be handled later by the shove algorithm.
     */
    bool routeHead( const VECTOR2I& aP, PNS_LINE& aNewHead,
                    bool aCwWalkaround = true );

    /**
     * Function routeStep()
     *
     * Performs a single routing alorithm step, for the end point aP.
     * @param aP ending point of current route
     * @return true, if the line has been changed.
     */
    void routeStep( const VECTOR2I& aP );

    ///> routing mode (walkaround, shove, etc.)
    PNS_MODE m_mode;

    ///> follow mouse trail by attaching new segments to the head
    ///> as the cursor moves
    bool m_follow_mouse;

    ///> mouse smoothing active
    bool m_smooth_mouse;

    ///> mouse smoothing step (in world units)
    int m_smoothing_step;

    ///> current routing direction
    DIRECTION_45 m_direction;

    ///> routing direction for new traces
    DIRECTION_45 m_initial_direction;

    ///> routing "head": volatile part of the track from the previously
    ///  analyzed point to the current routing destination
    PNS_LINE m_head;

    ///> routing "tail": part of the track that has been already fixed due to collisions with obstacles
    PNS_LINE m_tail;

    ///> current algorithm iteration
    int m_iteration;

    ///> pointer to world to search colliding items
    PNS_NODE* m_world;

    ///> current routing start point (end of tail, beginning of head)
    VECTOR2I m_p_start;

    ///> The shove engine
    PNS_SHOVE* m_shove;

    ///> Current world state
    PNS_NODE* m_currentNode;

    ///> Are we placing a via?
    bool m_placingVia;

    ///> current via diameter
    int m_viaDiameter;

    ///> current via drill
    int m_viaDrill;

    ///> walkaround algorithm iteration limit
    int m_walkaroundIterationLimit;

    ///> smart pads optimizer enabled.
    bool m_smartPads;
};

#endif    // __PNS_LINE_PLACER_H
