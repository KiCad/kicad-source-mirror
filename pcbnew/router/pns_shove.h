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

    enum SHOVE_STATUS
    {
        SH_OK = 0,
        SH_NULL,
        SH_INCOMPLETE,
        SH_HEAD_MODIFIED,
        SH_TRY_WALK
    };

    PNS_SHOVE( PNS_NODE* aWorld, PNS_ROUTER* aRouter );
    ~PNS_SHOVE();

    virtual PNS_LOGGER* Logger()
    {
        return &m_logger;
    }

    SHOVE_STATUS ShoveLines( const PNS_LINE& aCurrentHead );
    SHOVE_STATUS ShoveMultiLines( const PNS_ITEMSET& aHeadSet );

    SHOVE_STATUS ShoveDraggingVia( PNS_VIA* aVia, const VECTOR2I& aWhere, PNS_VIA** aNewVia );
    SHOVE_STATUS ProcessSingleLine( PNS_LINE& aCurrent, PNS_LINE& aObstacle,
                                    PNS_LINE& aShoved );

    void ForceClearance ( bool aEnabled, int aClearance )
    {
        if( aEnabled )
            m_forceClearance = aClearance;
        else
            m_forceClearance = -1;
    }

    PNS_NODE* CurrentNode();

    const PNS_LINE NewHead() const;

    void SetInitialLine( PNS_LINE& aInitial );

private:
    typedef std::vector<SHAPE_LINE_CHAIN> HULL_SET;
    typedef boost::optional<PNS_LINE> OPT_LINE;
    typedef std::pair<PNS_LINE, PNS_LINE> LINE_PAIR;
    typedef std::vector<LINE_PAIR> LINE_PAIR_VEC;

    struct SPRINGBACK_TAG
    {
        int64_t m_length;
        int m_segments;
        VECTOR2I m_p;
        PNS_NODE* m_node;
        PNS_ITEMSET m_headItems;
        PNS_COST_ESTIMATOR m_cost;
        OPT_BOX2I m_affectedArea;
    };

    SHOVE_STATUS processHullSet( PNS_LINE& aCurrent, PNS_LINE& aObstacle,
                                 PNS_LINE& aShoved, const HULL_SET& hulls );

    bool reduceSpringback( const PNS_ITEMSET& aHeadItems );
    bool pushSpringback( PNS_NODE* aNode, const PNS_ITEMSET& aHeadItems,
                                const PNS_COST_ESTIMATOR& aCost, const OPT_BOX2I& aAffectedArea );

    SHOVE_STATUS walkaroundLoneVia( PNS_LINE& aCurrent, PNS_LINE& aObstacle, PNS_LINE& aShoved );
    bool checkBumpDirection( const PNS_LINE& aCurrent, const PNS_LINE& aShoved ) const;

    SHOVE_STATUS onCollidingLine( PNS_LINE& aCurrent, PNS_LINE& aObstacle );
    SHOVE_STATUS onCollidingSegment( PNS_LINE& aCurrent, PNS_SEGMENT* aObstacleSeg );
    SHOVE_STATUS onCollidingSolid( PNS_LINE& aCurrent, PNS_ITEM* aObstacle );
    SHOVE_STATUS onCollidingVia( PNS_ITEM* aCurrent, PNS_VIA* aObstacleVia );
    SHOVE_STATUS onReverseCollidingVia( PNS_LINE& aCurrent, PNS_VIA* aObstacleVia );
    SHOVE_STATUS pushVia( PNS_VIA* aVia, const VECTOR2I& aForce, int aCurrentRank, bool aDryRun = false );

    OPT_BOX2I totalAffectedArea() const;

    void unwindStack( PNS_SEGMENT* aSeg );
    void unwindStack( PNS_ITEM* aItem );

    void runOptimizer( PNS_NODE* aNode );

    bool pushLine( const PNS_LINE& aL, bool aKeepCurrentOnTop = false );
    void popLine();

    PNS_LINE assembleLine( const PNS_SEGMENT* aSeg, int* aIndex = NULL );

    void replaceItems( PNS_ITEM* aOld, PNS_ITEM* aNew );

    OPT_BOX2I                   m_affectedAreaSum;

    SHOVE_STATUS shoveIteration( int aIter );
    SHOVE_STATUS shoveMainLoop();

    int getClearance( const PNS_ITEM* aA, const PNS_ITEM* aB ) const;

    std::vector<SPRINGBACK_TAG> m_nodeStack;
    std::vector<PNS_LINE>       m_lineStack;
    std::vector<PNS_LINE>       m_optimizerQueue;

    PNS_NODE*                   m_root;
    PNS_NODE*                   m_currentNode;

    OPT_LINE                    m_newHead;

    PNS_LOGGER                  m_logger;
    PNS_VIA*                    m_draggedVia;
    PNS_ITEMSET                 m_draggedViaHeadSet;

    int                         m_iter;
    int m_forceClearance;
    bool m_multiLineMode;
    void sanityCheck( PNS_LINE* aOld, PNS_LINE* aNew );
};

#endif // __PNS_SHOVE_H
