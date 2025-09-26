/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __PNS_LINE_PLACER_H
#define __PNS_LINE_PLACER_H

#include <math/vector2d.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "pns_line.h"
#include "pns_mouse_trail_tracer.h"
#include "pns_node.h"
#include "pns_placement_algo.h"
#include "pns_sizes_settings.h"
#include "pns_via.h"
#include "pns_walkaround.h"

namespace PNS {

class ROUTER;
class SHOVE;
class OPTIMIZER;
class VIA;
class SIZES_SETTINGS;
class NODE;

class FIXED_TAIL
{
public:
    FIXED_TAIL( int aLineCount = 1);
    ~FIXED_TAIL();

    struct FIX_POINT
    {
        int          layer;
        bool         placingVias;
        VECTOR2I     p;
        DIRECTION_45 direction;
    };

    struct STAGE
    {
        STAGE() :
            commit( nullptr )
        {}

        STAGE( const STAGE& aOther )
        {
            *this = aOther;
        }

        // Copy operator
        STAGE& operator=( const STAGE& aOther )
        {
            commit = aOther.commit;
            pts = aOther.pts;
            return *this;
        }

        // Move assignment operator
        STAGE& operator=( STAGE&& aOther ) noexcept
        {
            if (this != &aOther)
            {
                commit = aOther.commit;
                pts = std::move( aOther.pts );
            }

            return *this;
        }

        NODE*                  commit;
        std::vector<FIX_POINT> pts;
    };

    void Clear();
    void AddStage( const VECTOR2I& aStart, int aLayer, bool placingVias, DIRECTION_45 direction,
                   NODE* aNode );
    bool PopStage( STAGE& aStage );
    int StageCount() const;

private:
    std::vector<STAGE> m_stages;
};


/**
 * Single track placement algorithm. Interactively routes a track.
 * Applies shove and walkaround algorithms when needed.
 */

class LINE_PLACER : public PLACEMENT_ALGO
{
public:
    LINE_PLACER( ROUTER* aRouter );
    ~LINE_PLACER();

    /**
     * Start routing a single track at point aP, taking item aStartItem as anchor (unless NULL).
     */
    bool Start( const VECTOR2I& aP, ITEM* aStartItem ) override;

    /**
     * Move the end of the currently routed trace to the point \a aP, taking \a aEndItem as
     * anchor (if not NULL).
     */
    bool Move( const VECTOR2I& aP, ITEM* aEndItem ) override;

    /**
     * Commit the currently routed track to the parent node taking \a aP as the final end point
     * and \a aEndItem as the final anchor (if provided).
     *
     * @return true if route has been committed. May return false if the routing result is
     *         violating design rules.  In such cases, the track is only committed if
     *         CanViolateDRC() is on.
     */
    bool FixRoute( const VECTOR2I& aP, ITEM* aEndItem, bool aForceFinish ) override;

    std::optional<VECTOR2I> UnfixRoute() override;

    bool CommitPlacement() override;

    bool AbortPlacement() override;

    bool HasPlacedAnything() const override;

    /**
     * Enable/disable a via at the end of currently routed trace.
     */
    bool ToggleVia( bool aEnabled ) override;

    /**
     * Set the current routing layer.
     */
    bool SetLayer( int aLayer ) override;

    /**
     * Return the "head" of the line being placed, that is the volatile part that has not been
     * "fixed" yet.
     */
    const LINE& Head() const { return m_head; }

    /**
     * Return the "tail" of the line being placed, the part which has already wrapped around
     * and shoved some obstacles.
     */
    const LINE& Tail() const { return m_tail; }

    /**
     * Return the complete routed line.
     */
    const LINE Trace() const;

    /**
     * Return the complete routed line, as a single-member ITEM_SET.
     */
    const ITEM_SET Traces() override;

    /**
     * Return the current start of the line being placed.
     */
    const VECTOR2I& CurrentStart() const override
    {
        return m_currentStart;
    }

    /**
     * Return the current end of the line being placed. It may not be equal to the cursor
     * position due to collisions.
     */
    const VECTOR2I& CurrentEnd() const override
    {
        return m_currentEnd;
    }

    /**
     * Return the net of currently routed track.
     */
    const std::vector<NET_HANDLE> CurrentNets() const override
    {
        return std::vector<NET_HANDLE>( 1, m_currentNet );
    }

    /**
     * Return the layer of currently routed track.
     */
    int CurrentLayer() const override
    {
        return m_currentLayer;
    }

    /**
     * Return the most recent world state.
     */
    NODE* CurrentNode( bool aLoopsRemoved = false ) const override;

    /**
     * Toggle the current posture (straight/diagonal) of the trace head.
     */
    void FlipPosture() override;

    /**
     * Perform on-the-fly update of the width, via diameter & drill size from a settings class.
     *
     * Performs on-the-fly update of the width, via diameter & drill size from a settings class.
     * Used to dynamically change these parameters as the track is routed.
     */
    void UpdateSizes( const SIZES_SETTINGS& aSizes ) override;

    void SetOrthoMode( bool aOrthoMode ) override;

    bool IsPlacingVia() const override { return m_placingVia; }

    void GetModifiedNets( std::vector<NET_HANDLE>& aNets ) const override;

    /**
     * Snaps the point \a aP to segment \a aSeg. Splits the segment in two, forming a
     * joint at \a aP and stores updated topology in node \a aNode.
     */
    bool SplitAdjacentSegments( NODE* aNode, ITEM* aSeg, const VECTOR2I& aP );

    /**
     * Snaps the point \a aP to arc \a aArc. Splits the arc in two, forming a
     * joint at \a aP and stores updated topology in node \a aNode.
     */
    bool SplitAdjacentArcs( NODE* aNode, ITEM* aArc, const VECTOR2I& aP );

private:
    /**
     * Re-route the current track to point aP. Returns true, when routing has completed
     * successfully (i.e. the trace end has reached point \a aP), and false if the trace was
     * stuck somewhere on the way. May call routeStep() repetitively due to mouse smoothing.
     *
     * @param aP ending point of current route.
     * @return true, if the routing is complete.
     */
    bool route( const VECTOR2I& aP );

    /**
     * Draw the "leading" rats nest line, which connects the end of currently routed track and
     * the nearest yet unrouted item. If the routing for current net is complete, draws nothing.
     */
    void updateLeadingRatLine();

    /**
     * Set the board to route.
     */
    void setWorld( NODE* aWorld );

    /**
     * Initialize placement of a new line with given parameters.
     */
    void initPlacement();

    /**
     * Set preferred direction of the very first track segment to be laid.
     * Used by posture switching mechanism.
     */
    void setInitialDirection( const DIRECTION_45& aDirection );

    /**
     * Searches aNode for traces concurrent to aLatest and removes them. Updated
     * topology is stored in aNode.
     */
    void removeLoops( NODE* aNode, LINE& aLatest );

    /**
     * Assemble a line starting from segment or arc aLatest, removes collinear segments
     * and redundant vertices.  If a simplification has been found, replaces the old line
     * with the simplified one in \a aNode.
     */
    void simplifyNewLine( NODE* aNode, LINKED_ITEM* aLatest );

    /**
     * Check if the head of the track intersects its tail. If so, cuts the tail up to the
     * intersecting segment and fixes the head direction to match the last segment before
     * the cut.
     *
     * @return true if the line has been changed.
     */
    bool handleSelfIntersections();

    /**
     * Deal with pull-back: reduces the tail if head trace is moved backwards wrs to the
     * current tail direction.
     *
     * @return true if the line has been changed.
     */
    bool handlePullback();

    /**
     * Moves "established" segments from the head to the tail if certain conditions are met.
     *
     * @return true, if the line has been changed.
     */
    bool mergeHead();

    /**
     * Attempt to reduce the number of segments in the tail by trying to replace a certain
     * number of latest tail segments with a direct trace leading to \a aEnd that does not
     * collide with anything.
     *
     * @param aEnd is the current routing destination point.
     * @return true if the line has been changed.
     */
    bool reduceTail( const VECTOR2I& aEnd );

    /**
     * Try to reduce the corner count of the most recent part of tail/head by merging
     * obtuse/collinear segments.
     *
     * @return true if the line has been changed.
     */
    bool optimizeTailHeadTransition();

    /**
     * Compute the head trace between the current start point (m_p_start) and point \a aP,
     * starting with direction defined in m_direction.  The trace walks around all
     * colliding solid or non-movable items.  Movable segments are ignored, as they'll be
     * handled later by the shove algorithm.
     */
    bool routeHead( const VECTOR2I& aP, LINE& aNewHead, LINE& aNewTail );

    /**
     * Perform a single routing algorithm step, for the end point \a aP.
     *
     * @param aP is the  ending point of current route.
     * @return true if the line has been changed.
     */
    void routeStep( const VECTOR2I& aP );

    ///< Route step walk around mode.
    bool rhWalkOnly( const VECTOR2I& aP, LINE& aNewHead, LINE& aNewTail );
    bool rhWalkBase( const VECTOR2I& aP, LINE& aWalkLine, int aCollisionMask, PNS::PNS_MODE aMode, bool& aViaOk );
    bool splitHeadTail( const LINE& aNewLine, const LINE& aOldTail, LINE& aNewHead, LINE& aNewTail );
    bool cursorDistMinimum( const SHAPE_LINE_CHAIN& aL, const VECTOR2I& aCursor,  double lengthThreshold, SHAPE_LINE_CHAIN& aOut );
    bool clipAndCheckCollisions( const VECTOR2I& aP, const SHAPE_LINE_CHAIN& aL, SHAPE_LINE_CHAIN& aOut, int &thresholdDist );

    void updatePStart( const LINE& tail );

    //bool rhPostSplitHeadTail( )

    ///< Route step shove mode.
    bool rhShoveOnly( const VECTOR2I& aP, LINE& aNewHead, LINE& aNewTail );

    ///< Route step mark obstacles mode.
    bool rhMarkObstacles( const VECTOR2I& aP, LINE& aNewHead, LINE& aNewTail );

    const VIA makeVia( const VECTOR2I& aP );

    bool buildInitialLine( const VECTOR2I& aP, LINE& aHead, PNS::PNS_MODE aMode, bool aForceNoVia = false );


    DIRECTION_45   m_direction;         ///< current routing direction
    DIRECTION_45   m_initial_direction; ///< routing direction for new traces

    LINE           m_head;          ///< the volatile part of the track from the previously
                                    ///< analyzed point to the current routing destination

    LINE           m_tail;          ///< routing "tail": part of the track that has been already
                                    ///< fixed due to collisions with obstacles

    NODE*          m_world;         ///< pointer to world to search colliding items
    VECTOR2I       m_p_start;       ///< current routing start (end of tail, beginning of head)
    VECTOR2I       m_fixStart;       ///< start point of the last 'fix'

    std::optional<VECTOR2I>       m_last_p_end;

    std::unique_ptr<SHOVE> m_shove; ///< The shove engine

    NODE*          m_currentNode;   ///< Current world state
    NODE*          m_lastNode;      ///< Postprocessed world state (including marked collisions &
                                    ///< removed loops)

    SIZES_SETTINGS m_sizes;

    bool           m_placingVia;

    NET_HANDLE     m_currentNet;
    int            m_currentLayer;

    VECTOR2I       m_currentEnd;
    VECTOR2I       m_currentStart;
    LINE           m_currentTrace;

    ITEM*          m_startItem;
    ITEM*          m_endItem;

    bool           m_idle;
    bool           m_chainedPlacement;
    bool           m_orthoMode;
    bool           m_placementCorrect;

    FIXED_TAIL     m_fixedTail;
    MOUSE_TRAIL_TRACER m_mouseTrailTracer;
};

}

#endif    // __PNS_LINE_PLACER_H
