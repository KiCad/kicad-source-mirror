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

#ifndef __PNS_WALKAROUND_H
#define __PNS_WALKAROUND_H

#include "pns_line.h"
#include "pns_node.h"
#include "pns_router.h"
#include "pns_logger.h"
#include "pns_algo_base.h"

class PNS_WALKAROUND : public PNS_ALGO_BASE
{
    static const int DefaultIterationLimit = 50;

public:
    PNS_WALKAROUND( PNS_NODE* aWorld, PNS_ROUTER* aRouter ) :
        PNS_ALGO_BASE ( aRouter ),
        m_world( aWorld ), 
        m_iterationLimit( DefaultIterationLimit )
    {
        m_forceSingleDirection = false;
        m_forceLongerPath = false;
        m_cursorApproachMode = false;
        m_itemMask = PNS_ITEM::ANY;
    }

    ~PNS_WALKAROUND() {};

    enum WALKAROUND_STATUS
    {
        IN_PROGRESS = 0,
        DONE,
        STUCK
    };

    void SetWorld( PNS_NODE* aNode )
    {
        m_world = aNode;
    }

    void SetIterationLimit( const int aIterLimit )
    {
        m_iterationLimit = aIterLimit;
    }

    void SetSolidsOnly( bool aSolidsOnly )
    {
        if( aSolidsOnly )
            m_itemMask = PNS_ITEM::SOLID;
        else
            m_itemMask = PNS_ITEM::ANY;
    }

    void SetItemMask( int aMask )
    {
        m_itemMask = aMask;
    }

    void SetSingleDirection( bool aForceSingleDirection )
    {
        m_forceSingleDirection = aForceSingleDirection;
        m_forceLongerPath = aForceSingleDirection;
        //printf("FSD %d FPD %d\n", m_forceSingleDirection?1:0, m_forceLongerPath ? 1: 0);
    }

    void SetApproachCursor( bool aEnabled, const VECTOR2I& aPos )
    {
        m_cursorPos = aPos;
        m_cursorApproachMode = aEnabled;
    }

    WALKAROUND_STATUS Route( const PNS_LINE& aInitialPath, PNS_LINE& aWalkPath,
            bool aOptimize = true );

    virtual PNS_LOGGER* Logger()
    {
        return &m_logger;
    }

private:
    void start( const PNS_LINE& aInitialPath );

    WALKAROUND_STATUS singleStep( PNS_LINE& aPath, bool aWindingDirection );
    PNS_NODE::OPT_OBSTACLE nearestObstacle( const PNS_LINE& aPath );

    PNS_NODE* m_world;

    int m_recursiveBlockageCount;
    int m_iteration;
    int m_iterationLimit;
    int m_itemMask;
    bool m_forceSingleDirection, m_forceLongerPath;
    bool m_cursorApproachMode;
    VECTOR2I m_cursorPos;
    PNS_NODE::OPT_OBSTACLE m_currentObstacle[2];
    bool m_recursiveCollision[2];
    PNS_LOGGER m_logger;
};

#endif    // __PNS_WALKAROUND_H
