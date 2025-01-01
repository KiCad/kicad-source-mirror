/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2020 CERN
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

#ifndef __PNS_DRAG_ALGO_H
#define __PNS_DRAG_ALGO_H

#include <memory>
#include <math/vector2d.h>

#include "pns_router.h"
#include "pns_algo_base.h"
#include "pns_itemset.h"
#include "pns_layerset.h"


namespace PNS {

class NODE;

/**
 * DRAG_ALGO
 *
 * Base class for item dragging algorithms.
 */
class DRAG_ALGO : public ALGO_BASE
{
public:
    DRAG_ALGO( ROUTER* aRouter ) :
        ALGO_BASE( aRouter ),
        m_world( nullptr )
    {
    }

    ~DRAG_ALGO()
    {
    }

    /**
     * Function SetWorld()
     *
     * Sets the board to work on.
     */
    virtual void SetWorld( NODE* aWorld )
    {
        m_world = aWorld;
    }

    /**
     * Function Start()
     *
     * Starts routing a single track at point aP, taking item aStartItem as anchor (unless NULL).
     * Returns true if a dragging operation has started.
     */
    virtual bool Start( const VECTOR2I& aP, ITEM_SET& aPrimitives ) = 0;

    /**
     * Function Drag()
     *
     * Drags the current segment/corner/via to the point aP.
     * @return true, if dragging finished with success.
     */
    virtual bool Drag( const VECTOR2I& aP ) = 0;

    /**
     * Function FixRoute()
     *
     * Checks if the result of current dragging operation is correct
     * and eventually commits it to the world.
     * @return true, if dragging finished with success.
     */
    virtual bool FixRoute( bool aForceCommit ) = 0;

    /**
     * Function CurrentNode()
     *
     * Returns the most recent world state, including all items changed by dragging operation.
     */
    virtual NODE* CurrentNode() const = 0;

    /**
     * Function CurrentNets()
     *
     * Returns the net(s) of currently dragged item(s).
     */
    virtual const std::vector<NET_HANDLE> CurrentNets() const = 0;

    /**
     * Function CurrentLayer()
     *
     * Returns the layer of currently dragged item(s).
     */
    virtual int CurrentLayer() const = 0;

    /**
     * Function Traces()
     *
     * Returns the set of dragged items.
     */
    virtual const ITEM_SET Traces() = 0;

    virtual void SetMode( PNS::DRAG_MODE aDragMode ){};

    virtual PNS::DRAG_MODE Mode() const = 0;

    virtual bool GetForceMarkObstaclesMode( bool* aDragStatus ) const = 0;

    virtual std::vector<PNS::ITEM*> GetLastCommittedLeaderSegments() { return std::vector<PNS::ITEM*>(); };

protected:
    NODE*   m_world;

};


} // namespace PNS

#endif
