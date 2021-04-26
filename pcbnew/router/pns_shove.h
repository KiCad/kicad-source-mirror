/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <math/box2.h>

#include "pns_optimizer.h"
#include "pns_routing_settings.h"
#include "pns_algo_base.h"
#include "pns_logger.h"
#include "range.h"

namespace PNS {

class LINE;
class NODE;
class ROUTER;

/**
 * SHOVE
 *
 * The actual Push and Shove algorithm.
 */

class SHOVE : public ALGO_BASE
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

    SHOVE( NODE* aWorld, ROUTER* aRouter );
    ~SHOVE();

    virtual LOGGER* Logger() override
    {
        return &m_logger;
    }

    SHOVE_STATUS ShoveLines( const LINE& aCurrentHead );
    SHOVE_STATUS ShoveMultiLines( const ITEM_SET& aHeadSet );

    SHOVE_STATUS ShoveDraggingVia( const VIA_HANDLE aOldVia, const VECTOR2I& aWhere, VIA_HANDLE& aNewVia );
    SHOVE_STATUS ShoveObstacleLine( const LINE& aCurLine, const LINE& aObstacleLine, LINE& aResultLine );

    void ForceClearance ( bool aEnabled, int aClearance )
    {
        if( aEnabled )
            m_forceClearance = aClearance;
        else
            m_forceClearance = -1;
    }

    NODE* CurrentNode();

    const LINE NewHead() const;

    void SetInitialLine( LINE& aInitial );

    bool AddLockedSpringbackNode( NODE* aNode );
    void UnlockSpringbackNode( NODE* aNode );
    bool RewindSpringbackTo( NODE* aNode );

private:
    typedef std::vector<SHAPE_LINE_CHAIN> HULL_SET;
    typedef OPT<LINE> OPT_LINE;
    typedef std::pair<LINE, LINE> LINE_PAIR;
    typedef std::vector<LINE_PAIR> LINE_PAIR_VEC;

    struct SPRINGBACK_TAG
    {
        SPRINGBACK_TAG() :
            m_length( 0 ),
            m_node( nullptr ),
            m_seq( 0 ),
            m_locked( false )
        {}

        int64_t m_length;
        VIA_HANDLE m_draggedVia;
        VECTOR2I m_p;
        NODE* m_node;
        OPT_BOX2I m_affectedArea;
        int m_seq;
        bool m_locked;
    };

    SHOVE_STATUS shoveLineToHullSet( const LINE& aCurLine, const LINE& aObstacleLine,
                                     LINE& aResultLine, const HULL_SET& aHulls );

    NODE* reduceSpringback( const ITEM_SET& aHeadSet, VIA_HANDLE& aDraggedVia );

    bool pushSpringback( NODE* aNode, const OPT_BOX2I& aAffectedArea, VIA* aDraggedVia );

    SHOVE_STATUS shoveLineFromLoneVia( const LINE& aCurLine, const LINE& aObstacleLine,
                                       LINE& aResultLine );
    bool checkShoveDirection( const LINE& aCurLine, const LINE& aObstacleLine,
                              const LINE& aShovedLine ) const;

    SHOVE_STATUS onCollidingArc( LINE& aCurrent, ARC* aObstacleArc );
    SHOVE_STATUS onCollidingLine( LINE& aCurrent, LINE& aObstacle );
    SHOVE_STATUS onCollidingSegment( LINE& aCurrent, SEGMENT* aObstacleSeg );
    SHOVE_STATUS onCollidingSolid( LINE& aCurrent, ITEM* aObstacle );
    SHOVE_STATUS onCollidingVia( ITEM* aCurrent, VIA* aObstacleVia );
    SHOVE_STATUS onReverseCollidingVia( LINE& aCurrent, VIA* aObstacleVia );
    SHOVE_STATUS pushOrShoveVia( VIA* aVia, const VECTOR2I& aForce, int aCurrentRank );

    OPT_BOX2I totalAffectedArea() const;

    void unwindLineStack( LINKED_ITEM* aSeg );
    void unwindLineStack( ITEM* aItem );

    void runOptimizer( NODE* aNode );

    bool pushLineStack( const LINE& aL, bool aKeepCurrentOnTop = false );
    void popLineStack();

    LINE assembleLine( const LINKED_ITEM* aSeg, int* aIndex = NULL );

    void replaceItems( ITEM* aOld, std::unique_ptr< ITEM > aNew );
    void replaceLine( LINE& aOld, LINE& aNew, bool aIncludeInChangedArea = true, NODE *aNode = nullptr );

    LINE* findRootLine( LINE *aLine );

    OPT_BOX2I                   m_affectedArea;

    SHOVE_STATUS shoveIteration( int aIter );
    SHOVE_STATUS shoveMainLoop();

    int getClearance( const ITEM* aA, const ITEM* aB ) const;
    int getHoleClearance( const ITEM* aA, const ITEM* aB ) const;

    std::vector<SPRINGBACK_TAG> m_nodeStack;
    std::vector<LINE>           m_lineStack;
    std::vector<LINE>           m_optimizerQueue;
    std::unordered_map<const LINKED_ITEM*, LINE*> m_rootLineHistory;

    NODE*                       m_root;
    NODE*                       m_currentNode;
    int                         m_restrictSpringbackTagId;

    OPT_LINE                    m_newHead;

    LOGGER                      m_logger;
    VIA*                        m_draggedVia;

    int                         m_iter;
    int m_forceClearance;
    bool m_multiLineMode;
    void sanityCheck( LINE* aOld, LINE* aNew );
};

}

#endif // __PNS_SHOVE_H
