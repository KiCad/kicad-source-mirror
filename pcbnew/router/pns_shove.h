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

    enum SHOVE_POLICY
    {
        SHP_DEFAULT = 0,
        SHP_SHOVE = 0x1,
        SHP_WALK_FORWARD = 0x2,
        SHP_WALK_BACK = 0x4,
        SHP_IGNORE = 0x8,
        SHP_DONT_OPTIMIZE = 0x10,
        SHP_DONT_LOCK_ENDPOINTS = 0x20
    };


    void SetDefaultShovePolicy( int aPolicy );

    void SetShovePolicy( const LINKED_ITEM* aItem, int aPolicy );
    void SetShovePolicy( const LINE& aLine, int aPolicy );

    SHOVE( NODE* aWorld, ROUTER* aRouter );
    ~SHOVE();

    void ClearHeads();
    void AddHeads( const LINE& aHead,  int aPolicy = SHP_DEFAULT );
    void AddHeads( VIA_HANDLE aHead, VECTOR2I aNewPos, int aPolicy = SHP_DEFAULT );

    SHOVE_STATUS Run();

    SHOVE_STATUS ShoveDraggingVia( const VIA_HANDLE aOldVia, const VECTOR2I& aWhere,
                                   VIA_HANDLE& aNewVia );
    bool ShoveObstacleLine( const LINE& aCurLine, const LINE& aObstacleLine,
                                    LINE& aResultLine );

    void ForceClearance ( bool aEnabled, int aClearance )
    {
        if( aEnabled )
            m_forceClearance = aClearance;
        else
            m_forceClearance = -1;
    }

    NODE* CurrentNode();

    bool HeadsModified( int aIndex = -1 ) const;
    const PNS::LINE GetModifiedHead( int aIndex ) const;
    const VIA_HANDLE GetModifiedHeadVia( int aIndex ) const;

    bool AddLockedSpringbackNode( NODE* aNode );
    void UnlockSpringbackNode( NODE* aNode );
    bool RewindSpringbackTo( NODE* aNode );
    bool RewindToLastLockedNode();
    void DisablePostShoveOptimizations( int aMask );
    void SetSpringbackDoNotTouchNode( const NODE *aNode );

private:
    typedef std::vector<SHAPE_LINE_CHAIN> HULL_SET;
    typedef std::optional<LINE> OPT_LINE;
    typedef std::pair<LINE, LINE> LINE_PAIR;
    typedef std::vector<LINE_PAIR> LINE_PAIR_VEC;

    struct ROOT_LINE_ENTRY
    {
        ROOT_LINE_ENTRY( LINE* aLine = nullptr, int aPolicy = SHP_DEFAULT ) :
            rootLine( aLine ),
            policy( aPolicy )
        {}

        LINE *rootLine = nullptr;
        VIA* oldVia = nullptr;
        VIA* newVia = nullptr;
        std::optional<LINE> newLine;
        int policy = SHP_DEFAULT;
        bool isHead = false;
    };

    struct HEAD_LINE_ENTRY
    {
        HEAD_LINE_ENTRY( const LINE& aOrig, int aPolicy = SHP_DEFAULT ) :
            origHead( aOrig ),
            policy( aPolicy )
        {
            origHead->ClearLinks();
        };

        HEAD_LINE_ENTRY( VIA_HANDLE aVia, int aPolicy = SHP_DEFAULT ) :
            theVia( aVia ),
            policy( aPolicy )
        {};

        HEAD_LINE_ENTRY( const HEAD_LINE_ENTRY& aOther )
        {
            *this = aOther;
        }

        // Copy operator
        HEAD_LINE_ENTRY& operator=( const HEAD_LINE_ENTRY& aOther ) noexcept
        {
            geometryModified = aOther.geometryModified;
            prevVia = aOther.prevVia;
            theVia = aOther.theVia;
            draggedVia = aOther.draggedVia;
            viaNewPos = aOther.viaNewPos;
            origHead = aOther.origHead;
            newHead = aOther.newHead;
            policy = aOther.policy;
            return *this;
        }

        // Move assignment operator
        HEAD_LINE_ENTRY& operator=( HEAD_LINE_ENTRY&& aOther ) noexcept
        {
            if (this != &aOther)
            {
                geometryModified = aOther.geometryModified;
                prevVia = aOther.prevVia;
                theVia = aOther.theVia;
                draggedVia = aOther.draggedVia;
                viaNewPos = aOther.viaNewPos;
                origHead = std::move( aOther.origHead );
                newHead = std::move( aOther.newHead );
                policy = aOther.policy;
            }

            return *this;
        }

        bool geometryModified = false;
        std::optional<VIA_HANDLE> prevVia;
        std::optional<VIA_HANDLE> theVia;
        VIA* draggedVia = nullptr;
        VECTOR2I viaNewPos;
        std::optional<LINE> origHead;
        std::optional<LINE> newHead;
        int policy;
    };

    struct SPRINGBACK_TAG
    {
        SPRINGBACK_TAG() :
            m_length( 0 ),
            m_node( nullptr ),
            m_seq( 0 ),
            m_locked( false )
        {}

        int64_t m_length;
        std::vector<VIA_HANDLE> m_draggedVias;
        VECTOR2I m_p;
        NODE* m_node;
        OPT_BOX2I m_affectedArea;
        int m_seq;
        bool m_locked;
    };

    bool pruneLineFromOptimizerQueue( const LINE& aLine );

    bool shoveLineToHullSet( const LINE& aCurLine, const LINE& aObstacleLine, LINE& aResultLine,
                             const HULL_SET& aHulls, bool aPermitAdjustingStart = false,
                             bool aPermitAdjustingEnd = false );

    NODE* reduceSpringback( const ITEM_SET& aHeadSet );

    bool patchTadpoleVia( ITEM* nearest, LINE& current );

    bool pushSpringback( NODE* aNode, const OPT_BOX2I& aAffectedArea );

    bool shoveLineFromLoneVia( const LINE& aCurLine, const LINE& aObstacleLine,
                                       LINE& aResultLine );
    bool checkShoveDirection( const LINE& aCurLine, const LINE& aObstacleLine,
                              const LINE& aShovedLine ) const;

    SHOVE_STATUS onCollidingArc( LINE& aCurrent, ARC* aObstacleArc );
    SHOVE_STATUS onCollidingLine( LINE& aCurrent, LINE& aObstacle, int aNextRank );
    SHOVE_STATUS onCollidingSegment( LINE& aCurrent, SEGMENT* aObstacleSeg );
    SHOVE_STATUS onCollidingSolid( LINE& aCurrent, ITEM* aObstacle, OBSTACLE& aObstacleInfo );
    SHOVE_STATUS onCollidingVia( ITEM* aCurrent, VIA* aObstacleVia, OBSTACLE& aObstacleInfo, int aNextRank );
    SHOVE_STATUS onReverseCollidingVia( LINE& aCurrent, VIA* aObstacleVia, OBSTACLE& aObstacleInfo );
    SHOVE_STATUS pushOrShoveVia( VIA* aVia, const VECTOR2I& aForce, int aNextRank, bool aDontUnwindStack = false );

    OPT_BOX2I totalAffectedArea() const;

    void unwindLineStack( const LINKED_ITEM* aSeg );
    void unwindLineStack( const ITEM* aItem );

    void runOptimizer( NODE* aNode );

    bool pushLineStack( const LINE& aL, bool aKeepCurrentOnTop = false );
    void popLineStack();

    LINE assembleLine( const LINKED_ITEM* aSeg, int* aIndex = nullptr, bool aPreCleanup = false );

    void replaceItems( ITEM* aOld, std::unique_ptr< ITEM > aNew );
    ROOT_LINE_ENTRY* replaceLine( LINE& aOld, LINE& aNew,
                    bool aIncludeInChangedArea = true,
                    bool aAllowRedundantSegments = true,
                      NODE *aNode = nullptr );

    ROOT_LINE_ENTRY* findRootLine( const LINE& aLine );
    ROOT_LINE_ENTRY* findRootLine( const LINKED_ITEM *aItem );
    ROOT_LINE_ENTRY* touchRootLine( const LINE& aLine );
    ROOT_LINE_ENTRY* touchRootLine( const LINKED_ITEM *aItem );
    void pruneRootLines( NODE *aRemovedNode );


    SHOVE_STATUS shoveIteration( int aIter );
    SHOVE_STATUS shoveMainLoop();

    int getClearance( const ITEM* aA, const ITEM* aB ) const;
    bool fixupViaCollisions( const LINE* aCurrent, OBSTACLE& obs );
    void sanityCheck( LINE* aOld, LINE* aNew );
    void reconstructHeads( bool aShoveFailed );
    void removeHeads();
    bool preShoveCleanup( LINE* aOld, LINE* aNew );
    const wxString formatPolicy( int aPolicy );

    std::vector<SPRINGBACK_TAG> m_nodeStack;
    std::vector<LINE>           m_lineStack;
    std::vector<LINE>           m_optimizerQueue;
    std::deque<HEAD_LINE_ENTRY> m_headLines;

    std::unordered_map<LINKED_ITEM::UNIQ_ID, ROOT_LINE_ENTRY*> m_rootLineHistory;

    NODE*                       m_root;
    NODE*                       m_currentNode;
    const NODE*                 m_springbackDoNotTouchNode;
    int                         m_restrictSpringbackTagId;
    VIA*                        m_draggedVia;
    int                         m_iter;
    bool m_headsModified;
    int m_forceClearance;
    bool m_multiLineMode;

    int m_optFlagDisableMask;

    int m_defaultPolicy;
    OPT_BOX2I                   m_affectedArea;

};

}

#endif // __PNS_SHOVE_H
