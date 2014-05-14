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

#ifndef __PNS_SHOVE_H
#define __PNS_SHOVE_H

#include <vector>
#include <stack>

#include "pns_optimizer.h"
#include "pns_routing_settings.h"
#include "pns_algo_base.h"
#include "pns_logger.h"
#include "range.h"

class PNS_LINE;
class PNS_NODE;
class PNS_ROUTER;

/**
 * Class PNS_SHOVE
 *
 * The actual Push and Shove algorithm. 
 */

class PNS_SHOVE : public PNS_ALGO_BASE
{
public:

    enum ShoveStatus
    {
        SH_OK = 0,
        SH_NULL,
        SH_INCOMPLETE,
        SH_HEAD_MODIFIED
    };

    PNS_SHOVE( PNS_NODE* aWorld, PNS_ROUTER *aRouter );
    ~PNS_SHOVE();

    virtual PNS_LOGGER *Logger()
    {
        return &m_logger;
    }

    ShoveStatus ShoveLines( const PNS_LINE& aCurrentHead );
    ShoveStatus ShoveDraggingVia ( PNS_VIA *aVia, const VECTOR2I& aWhere, PNS_VIA **aNewVia );

    PNS_NODE* CurrentNode();

    const PNS_LINE NewHead() const;

    void SetInitialLine ( PNS_LINE *aInitial );

private:
    typedef std::vector<SHAPE_LINE_CHAIN> HullSet;
    typedef boost::optional<PNS_LINE> OptLine;
    typedef std::pair <PNS_LINE *, PNS_LINE *> LinePair;
    typedef std::vector<LinePair> LinePairVec;

    struct SpringbackTag
    {
        int64_t length;
        int segments;
        VECTOR2I p;
        PNS_NODE *node;
        PNS_ITEMSET headItems;
        PNS_COST_ESTIMATOR cost;
    };

    ShoveStatus processSingleLine(PNS_LINE *aCurrent, PNS_LINE* aObstacle, PNS_LINE *aShoved );
    ShoveStatus processHullSet ( PNS_LINE *aCurrent, PNS_LINE *aObstacle, PNS_LINE *aShoved, const HullSet& hulls );
    
    bool reduceSpringback( const PNS_ITEMSET &aHeadItems );
    bool pushSpringback( PNS_NODE* aNode, const PNS_ITEMSET &aHeadItems, const PNS_COST_ESTIMATOR& aCost );
    
    ShoveStatus walkaroundLoneVia ( PNS_LINE *aCurrent, PNS_LINE *aObstacle, PNS_LINE *aShoved );
    bool checkBumpDirection ( PNS_LINE *aCurrent, PNS_LINE *aShoved ) const;

    ShoveStatus onCollidingLine( PNS_LINE *aCurrent, PNS_LINE *aObstacle );
    ShoveStatus onCollidingSegment( PNS_LINE *aCurrent, PNS_SEGMENT *aObstacleSeg );
    ShoveStatus onCollidingSolid( PNS_LINE *aCurrent, PNS_SOLID *aObstacleSolid );
    ShoveStatus onCollidingVia( PNS_ITEM *aCurrent, PNS_VIA *aObstacleVia );
    ShoveStatus onReverseCollidingVia( PNS_LINE *aCurrent, PNS_VIA *aObstacleVia );
    ShoveStatus pushVia ( PNS_VIA *aVia, const VECTOR2I& aForce, int aCurrentRank );

    void unwindStack ( PNS_SEGMENT *seg );
    void unwindStack ( PNS_ITEM *item );

    void runOptimizer ( PNS_NODE *node, PNS_LINE *head );

    void pushLine ( PNS_LINE *l );
    void popLine(); 

    const RANGE<int> findShovedVertexRange ( PNS_LINE *l );

    PNS_LINE *assembleLine ( const PNS_SEGMENT *aSeg, int *aIndex = NULL );
    PNS_LINE *cloneLine ( const PNS_LINE *aLine );

    ShoveStatus shoveIteration(int aIter); 
    ShoveStatus shoveMainLoop(); 

    std::vector<SpringbackTag>  m_nodeStack;
    std::vector<PNS_LINE*>      m_lineStack;
    std::vector<PNS_LINE*>      m_optimizerQueue;
    std::vector<PNS_ITEM*>      m_gcItems;

    PNS_NODE*                   m_root;
    PNS_NODE*                   m_currentNode;
    PNS_LINE*                   m_currentHead;
    PNS_LINE*                   m_collidingLine;
    
    OptLine                     m_newHead;

    PNS_LOGGER                  m_logger;
    PNS_VIA*                    m_draggedVia;

    int                         m_iter;
};

#endif // __PNS_SHOVE_H
