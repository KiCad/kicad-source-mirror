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

#ifndef __PNS_WALKAROUND_H
#define __PNS_WALKAROUND_H

#include <set>

#include "pns_line.h"
#include "pns_node.h"
#include "pns_router.h"
#include "pns_logger.h"
#include "pns_algo_base.h"
#include "pns_topology.h"

namespace PNS {

class WALKAROUND : public ALGO_BASE
{
    static constexpr int MaxWalkPolicies = 3;

public:
    WALKAROUND( NODE* aWorld, ROUTER* aRouter ) :
        ALGO_BASE ( aRouter ),
        m_world( aWorld )
    {
        m_forceWinding = false;
        m_itemMask = ITEM::ANY_T;

        // Initialize other members, to avoid uninitialized variables.
        m_iteration = 0;
        m_initialLength = 0.0;
        m_forceCw = false;
        m_forceLongerPath = false;
        m_lengthLimitOn = true;
        m_useShortestPath = false;
        m_lengthExpansionFactor = 10.0;
        m_iterationLimit = Settings().WalkaroundIterationLimit();
    }

    ~WALKAROUND() {};

    enum STATUS
    {
        ST_IN_PROGRESS = 0,
        ST_ALMOST_DONE,
        ST_DONE,
        ST_STUCK,
        ST_NONE
    };

    enum WALK_POLICY
    {
        WP_CW = 0,
        WP_CCW = 1,
        WP_SHORTEST = 2
    };

    struct RESULT
    {
        RESULT()
        {
            for( int i = 0; i < MaxWalkPolicies; i++ )
                status [i] = ST_NONE;
        }

        STATUS status[ MaxWalkPolicies ];
        LINE lines[ MaxWalkPolicies ];
    };

    void SetWorld( NODE* aNode )
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
            m_itemMask = ITEM::SOLID_T;
        else
            m_itemMask = ITEM::ANY_T;
    }

    void SetItemMask( int aMask )
    {
        m_itemMask = aMask;
    }

    void SetForceWinding ( bool aEnabled, bool aCw )
    {
        m_forceCw = aCw;
        m_forceWinding = aEnabled;
    }

    void SetPickShortestPath( bool aEnabled )
    {
        m_useShortestPath = aEnabled;
    }

    void RestrictToCluster( bool aEnabled, const TOPOLOGY::CLUSTER& aCluster );

    STATUS Route( const LINE& aInitialPath, LINE& aWalkPath,
            bool aOptimize = true );

    const RESULT Route( const LINE& aInitialPath );

    void SetLengthLimit( bool aEnable, double aLengthExpansionFactor )
    {
        m_lengthLimitOn = aEnable;
        m_lengthExpansionFactor = aLengthExpansionFactor;
    }

    void SetAllowedPolicies( std::vector<WALK_POLICY> aPolicies);

private:
    void start( const LINE& aInitialPath );
    bool singleStep();

    NODE::OPT_OBSTACLE nearestObstacle( const LINE& aPath );
    NODE* m_world;

    int m_iteration;
    int m_iterationLimit;
    int m_itemMask;
    bool m_forceWinding;
    bool m_forceCw;
    VECTOR2I m_cursorPos;
    VECTOR2I m_lastP;
    std::set<const ITEM*> m_restrictedSet;
    std::vector<VECTOR2I> m_restrictedVertices;
    bool m_forceLongerPath;
    bool m_lengthLimitOn;
    bool m_useShortestPath;
    double m_lengthExpansionFactor;
    bool m_enabledPolicies[ MaxWalkPolicies ];
    NODE::OPT_OBSTACLE m_currentObstacle[ MaxWalkPolicies ];
    TOPOLOGY::CLUSTER m_currentCluster[ MaxWalkPolicies ];
    std::optional<TOPOLOGY::CLUSTER> m_lastShortestCluster;
    RESULT m_currentResult;
    double m_initialLength;
};

}

#endif    // __PNS_WALKAROUND_H
