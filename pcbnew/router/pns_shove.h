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

#ifndef __PNS_SHOVE_H
#define __PNS_SHOVE_H

#include <vector>
#include <stack>

#include "pns_optimizer.h"

class PNS_LINE;
class PNS_NODE;
class PNS_ROUTER;

class PNS_SHOVE
{
public:
    PNS_SHOVE( PNS_NODE* aWorld );
    ~PNS_SHOVE();

    enum ShoveStatus
    {
        SH_OK = 0,
        SH_NULL,
        SH_INCOMPLETE
    };

    ShoveStatus ShoveLines( PNS_LINE* aCurrentHead );

    PNS_NODE* GetCurrentNode()
    {
        return m_nodeStack.empty() ? m_root : m_nodeStack.back().node;
    }

    const PNS_COST_ESTIMATOR TotalCost() const;

    void Reset();
    void KillChildNodes();

private:
    static const int ShoveTimeLimit = 3000;

    bool tryShove( PNS_NODE* aWorld, PNS_LINE* aTrack, PNS_LINE* aObstacle, 
            PNS_SEGMENT& aObstacleSeg, PNS_LINE* aResult, bool aInvertWinding );

    ShoveStatus shoveSingleLine( PNS_NODE* aNode, PNS_LINE* aCurrent, PNS_LINE* aObstacle,
            PNS_SEGMENT& aObstacleSeg, PNS_LINE* aResult );

    bool reduceSpringback( PNS_LINE* aHead );
    bool pushSpringback( PNS_NODE* aNode, PNS_LINE* aHead, const PNS_COST_ESTIMATOR& aCost );

    struct SpringbackTag
    {
        int64_t length;
        int segments;
        VECTOR2I p;
        PNS_NODE* node;
        PNS_COST_ESTIMATOR cost;
    };

    std::vector<SpringbackTag> m_nodeStack;
    PNS_NODE* m_root;
    PNS_NODE* m_currentNode;
    int m_iterLimit;
};

#endif

