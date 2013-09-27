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

#ifndef __PNS_WALKAROUND_H
#define __PNS_WALKAROUND_H

#include "pns_line.h"
#include "pns_node.h"

class PNS_WALKAROUND
{
    static const int DefaultIterationLimit = 50;

public:
    PNS_WALKAROUND( PNS_NODE* aWorld ) :
        m_world( aWorld ), m_iteration_limit( DefaultIterationLimit )
    {
        m_forceSingleDirection = false;
        m_forceLongerPath = false;
        m_cursorApproachMode = false;
    };
    ~PNS_WALKAROUND() {};

    enum WalkaroundStatus
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
        m_iteration_limit = aIterLimit;
    }

    void SetSolidsOnly( bool aSolidsOnly )
    {
        m_solids_only = aSolidsOnly;
    }

    void SetSingleDirection( bool aForceSingleDirection  )
    {
        m_forceSingleDirection = aForceSingleDirection;
        m_forceLongerPath = true;
    }

    void SetApproachCursor( bool aEnabled, const VECTOR2I& aPos )
    {
        m_cursorPos = aPos;
        m_cursorApproachMode = aEnabled;
    }

    WalkaroundStatus Route( const PNS_LINE& aInitialPath, PNS_LINE& aWalkPath, 
            bool aOptimize = true );

private:
    void start( const PNS_LINE& aInitialPath );

    WalkaroundStatus singleStep( PNS_LINE& aPath, bool aWindingDirection );
    PNS_NODE::OptObstacle nearestObstacle( const PNS_LINE& aPath );

    PNS_NODE* m_world;

    int m_recursiveBlockageCount;
    int m_iteration;
    int m_iteration_limit;
    bool m_solids_only;
    bool m_forceSingleDirection, m_forceLongerPath;
    bool m_cursorApproachMode;
    VECTOR2I m_cursorPos;
    PNS_NODE::OptObstacle m_currentObstacle[2];
    bool m_recursiveCollision[2];
};

#endif    // __PNS_WALKAROUND_H

