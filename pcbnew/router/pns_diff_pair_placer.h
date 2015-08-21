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

#ifndef __PNS_DIFF_PLACER_H
#define __PNS_DIFF_PLACER_H

#include <math/vector2d.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "pns_sizes_settings.h"
#include "pns_node.h"
#include "pns_via.h"
#include "pns_line.h"
#include "pns_algo_base.h"
#include "pns_diff_pair.h"

#include "pns_placement_algo.h"

class PNS_ROUTER;
class PNS_SHOVE;
class PNS_OPTIMIZER;
class PNS_ROUTER_BASE;
class PNS_VIA;
class PNS_SIZES_SETTINGS;


/**
 * Class PNS_LINE_PLACER
 *
 * Single track placement algorithm. Interactively routes a track.
 * Applies shove and walkaround algorithms when needed.
 */

class PNS_DIFF_PAIR_PLACER : public PNS_PLACEMENT_ALGO
{
public:
    PNS_DIFF_PAIR_PLACER( PNS_ROUTER* aRouter );
    ~PNS_DIFF_PAIR_PLACER();

    /**
     * Function Start()
     *
     * Starts routing a single track at point aP, taking item aStartItem as anchor
     * (unless NULL).
     */
    bool Start( const VECTOR2I& aP, PNS_ITEM* aStartItem );

    /**
     * Function Move()
     *
     * Moves the end of the currently routed trace to the point aP, taking
     * aEndItem as anchor (if not NULL).
     * (unless NULL).
     */
    bool Move( const VECTOR2I& aP, PNS_ITEM* aEndItem );

    /**
     * Function FixRoute()
     *
     * Commits the currently routed track to the parent node, taking
     * aP as the final end point and aEndItem as the final anchor (if provided).
     * @return true, if route has been commited. May return false if the routing
     * result is violating design rules - in such case, the track is only committed
     * if Settings.CanViolateDRC() is on.
     */
    bool FixRoute( const VECTOR2I& aP, PNS_ITEM* aEndItem );

    /**
     * Function ToggleVia()
     *
     * Enables/disables a via at the end of currently routed trace.
     */
    bool ToggleVia( bool aEnabled );

    /**
     * Function SetLayer()
     *
     * Sets the current routing layer.
     */
    bool SetLayer( int aLayer );

    /**
     * Function Traces()
     *
     * Returns the complete routed line, as a single-member PNS_ITEMSET.
     */
    const PNS_ITEMSET Traces();

    /**
     * Function CurrentEnd()
     *
     * Returns the current end of the line being placed. It may not be equal
     * to the cursor position due to collisions.
     */
    const VECTOR2I& CurrentEnd() const
    {
        return m_currentEnd;
    }

    /**
     * Function CurrentNets()
     *
     * Returns the net code of currently routed track.
     */
    const std::vector<int> CurrentNets() const;

    /**
     * Function CurrentLayer()
     *
     * Returns the layer of currently routed track.
     */
    int CurrentLayer() const
    {
        return m_currentLayer;
    }

    /**
     * Function CurrentNode()
     *
     * Returns the most recent world state.
     */
    PNS_NODE* CurrentNode( bool aLoopsRemoved = false ) const;

    /**
     * Function FlipPosture()
     *
     * Toggles the current posture (straight/diagonal) of the trace head.
     */
    void FlipPosture();

    /**
     * Function UpdateSizes()
     *
     * Performs on-the-fly update of the width, via diameter & drill size from
     * a settings class. Used to dynamically change these parameters as
     * the track is routed.
     */
    void UpdateSizes( const PNS_SIZES_SETTINGS& aSizes );

    bool IsPlacingVia() const { return m_placingVia; }

    void SetOrthoMode( bool aOrthoMode );

    void GetModifiedNets( std::vector<int>& aNets ) const;

private:
    int viaGap() const;
    int gap() const;

    /**
     * Function route()
     *
     * Re-routes the current track to point aP. Returns true, when routing has
     * completed successfully (i.e. the trace end has reached point aP), and false
     * if the trace was stuck somewhere on the way. May call routeStep()
     * repetitively due to mouse smoothing.
     * @param aP ending point of current route.
     * @return true, if the routing is complete.
     */
    bool route( const VECTOR2I& aP );

    /**
     * Function updateLeadingRatLine()
     *
     * Draws the "leading" ratsnest line, which connects the end of currently
     * routed track and the nearest yet unrouted item. If the routing for
     * current net is complete, draws nothing.
     */
    void updateLeadingRatLine();

    /**
     * Function setWorld()
     *
     * Sets the board to route.
     */
    void setWorld( PNS_NODE* aWorld );

    /**
     * Function startPlacement()
     *
     * Initializes placement of a new line with given parameters.
     */
    void initPlacement( bool aSplitSeg = false );

    /**
     * Function setInitialDirection()
     *
     * Sets preferred direction of the very first track segment to be laid.
     * Used by posture switching mechanism.
     */
    void setInitialDirection( const DIRECTION_45& aDirection );


    bool routeHead( const VECTOR2I& aP );
    bool tryWalkDp( PNS_NODE* aNode, PNS_DIFF_PAIR& aPair, bool aSolidsOnly );

    ///> route step, walkaround mode
    bool rhWalkOnly( const VECTOR2I& aP );

    ///> route step, shove mode
    bool rhShoveOnly ( const VECTOR2I& aP );

    ///> route step, mark obstacles mode
    bool rhMarkObstacles( const VECTOR2I& aP );

    const PNS_VIA makeVia ( const VECTOR2I& aP, int aNet );

    bool findDpPrimitivePair( const VECTOR2I& aP, PNS_ITEM* aItem, PNS_DP_PRIMITIVE_PAIR& aPair );
    OPT_VECTOR2I getDanglingAnchor( PNS_NODE* aNode, PNS_ITEM* aItem );
    int matchDpSuffix( wxString aNetName, wxString& aComplementNet, wxString& aBaseDpName );
    bool attemptWalk( PNS_NODE* aNode, PNS_DIFF_PAIR* aCurrent, PNS_DIFF_PAIR& aWalk, bool aPFirst, bool aWindCw, bool aSolidsOnly );
    bool propagateDpHeadForces ( const VECTOR2I& aP, VECTOR2I& aNewP );

    enum State {
        RT_START = 0,
        RT_ROUTE = 1,
        RT_FINISH = 2
    };

    State m_state;

    bool m_chainedPlacement;
    bool m_initialDiagonal;
    bool m_startDiagonal;
    bool m_fitOk;

    int m_netP, m_netN;

    PNS_DP_PRIMITIVE_PAIR m_start;
    boost::optional<PNS_DP_PRIMITIVE_PAIR> m_prevPair;

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

    ///> Postprocessed world state (including marked collisions & removed loops)
    PNS_NODE* m_lastNode;

    PNS_SIZES_SETTINGS m_sizes;

    ///> Are we placing a via?
    bool m_placingVia;

    ///> current via diameter
    int m_viaDiameter;

    ///> current via drill
    int m_viaDrill;

    ///> current track width
    int m_currentWidth;

    int m_currentNet;
    int m_currentLayer;

    bool m_startsOnVia;
    bool m_orthoMode;
    bool m_snapOnTarget;

    VECTOR2I m_currentEnd, m_currentStart;
    PNS_DIFF_PAIR m_currentTrace;

    PNS_ITEM* m_currentEndItem;
    PNS_MODE m_currentMode;

    bool m_idle;
};

#endif    // __PNS_LINE_PLACER_H
