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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __PNS_PLACEMENT_ALGO_H
#define __PNS_PLACEMENT_ALGO_H

#include <math/vector2d.h>

#include "pns_algo_base.h"
#include "pns_sizes_settings.h"
#include "pns_itemset.h"

namespace PNS {

class ROUTER;
class ITEM;
class NODE;

/**
 * Abstract class for a P&S placement/dragging algorithm.
 *
 * All subtools (drag, single/diff pair routing and meandering) are derived from it.
 */

class PLACEMENT_ALGO : public ALGO_BASE
{
public:
    PLACEMENT_ALGO( ROUTER* aRouter ) :
        ALGO_BASE( aRouter ) {};

    virtual ~PLACEMENT_ALGO () {};

    /**
     * Start placement/drag operation at point aP, taking item aStartItem as anchor
     * (unless NULL).
     */
    virtual bool Start( const VECTOR2I& aP, ITEM* aStartItem ) = 0;

    /**
     * Move the end of the currently routed primtive(s) to the point aP, taking
     * aEndItem as the anchor (if not NULL).
     * (unless NULL).
     */
    virtual bool Move( const VECTOR2I& aP, ITEM* aEndItem ) = 0;

    /**
     * Commit the currently routed items to the parent node, taking
     * aP as the final end point and aEndItem as the final anchor (if provided).
     * @return true, if route has been committed. May return false if the routing
     * result is violating design rules - in such case, the track is only committed
     * if ROUTING_SETTINGS::AllowDRCViolations() is on.
     */
    virtual bool FixRoute( const VECTOR2I& aP, ITEM* aEndItem, bool aForceFinish = false ) = 0;

    virtual std::optional<VECTOR2I> UnfixRoute() { return std::nullopt; };

    virtual bool CommitPlacement() { return false; };

    virtual bool AbortPlacement() { return false; };

    virtual bool HasPlacedAnything() const { return false; }

    /**
     * Enable/disable a via at the end of currently routed trace.
     */
    virtual bool ToggleVia( bool aEnabled )
    {
        return false;
    }

    /**
     * Return true if the placer is placing a via (or more vias).
     */
    virtual bool IsPlacingVia() const
    {
        return false;
    }

    /**
     * Set the current routing layer.
     */
    virtual bool SetLayer( int aLayer )
    {
        return false;
    }

    /**
     * Return all routed/tuned traces.
     */
    virtual const ITEM_SET Traces() = 0;

    /**
     * Return the current start of the line(s) being placed/tuned.
     */
    virtual const VECTOR2I& CurrentStart() const = 0;

    /**
     * Return the current end of the line(s) being placed/tuned. It may not be equal
     * to the cursor position due to collisions.
     */
    virtual const VECTOR2I& CurrentEnd() const = 0;

    /**
     * Returns the net(s) of currently routed track(s).
     */
    virtual const std::vector<NET_HANDLE> CurrentNets() const = 0;

    /**
     * Return the layer of currently routed track.
     */
    virtual int CurrentLayer() const = 0;

    /**
     * Return the most recent board state.
     */
    virtual NODE* CurrentNode( bool aLoopsRemoved = false ) const = 0;

    /**
     * Toggle the current posture (straight/diagonal) of the trace head.
     */
    virtual void FlipPosture()
    {
    }

    /**
     * Perform on-the-fly update of the width, via diameter & drill size from
     * a settings class. Used to dynamically change these parameters as
     * the track is routed.
     */
    virtual void UpdateSizes( const SIZES_SETTINGS& aSizes )
    {
    }

    /**
     * Force the router to place a straight 90/45 degree trace (with the end
     * as near to the cursor as possible) instead of a standard 135 degree
     * two-segment bend.
     */
    virtual void SetOrthoMode ( bool aOrthoMode )
    {
    }

    /**
     * Return the nets of all currently routed trace(s)
     */
    virtual void GetModifiedNets( std::vector<NET_HANDLE> &aNets ) const
    {
    }

protected:
    virtual bool removeLoops( NODE* aNode, LINE& aLatest );

    /**
     * Assemble a line starting from segment or arc aLatest, removes collinear segments
     * and redundant vertices.  If a simplification has been found, replaces the old line
     * with the simplified one in \a aNode.
     */
    virtual bool simplifyNewLine( NODE* aNode, LINKED_ITEM* aLatest );

};

}

#endif
