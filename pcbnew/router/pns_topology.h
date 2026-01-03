/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2015 CERN
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

#ifndef __PNS_TOPOLOGY_H
#define __PNS_TOPOLOGY_H

#include <vector>
#include <set>

#include "pns_itemset.h"

namespace PNS {

class NODE;
class SEGMENT;
class JOINT;
class ITEM;
class SOLID;
class DIFF_PAIR;
class ROUTER_IFACE;
class LINKED_ITEM;

class TOPOLOGY
{
public:

    struct CLUSTER
    {
        const ITEM* m_key = nullptr;
        std::set<ITEM*> m_items;
    };

    typedef std::set<const JOINT*> JOINT_SET;

    TOPOLOGY( NODE* aNode ):
        m_world( aNode ) {};

    ~TOPOLOGY() {};

    bool SimplifyLine( LINE *aLine );
    ITEM* NearestUnconnectedItem( const JOINT* aStart, int* aAnchor = nullptr,
                                  int aKindMask = ITEM::ANY_T );

    bool NearestUnconnectedAnchorPoint( const LINE* aTrack, VECTOR2I& aPoint, PNS_LAYER_RANGE& aLayers,
                                        ITEM*& aItem );
    bool LeadingRatLine( const LINE* aTrack, SHAPE_LINE_CHAIN& aRatLine );

    const JOINT_SET ConnectedJoints( const JOINT* aStart );
    const ITEM_SET ConnectedItems( const JOINT* aStart, int aKindMask = ITEM::ANY_T );
    const ITEM_SET ConnectedItems( ITEM* aStart, int aKindMask = ITEM::ANY_T );
    int64_t ShortestConnectionLength( ITEM* aFrom, ITEM* aTo );

    /**
     * Assemble a trivial path between two joints given a starting item.
     *
     * @param aStart is the item to assemble from.
     * @param aTerminalJoints will be filled with the start and end points of the assembled path.
     * @param aFollowLockedSegments if true will assemble a path including locked segments
     * @return a set of items in the path.
     */
    const ITEM_SET AssembleTrivialPath( ITEM* aStart,
                                        std::pair<const JOINT*, const JOINT*>* aTerminalJoints = nullptr,
                                        bool aFollowLockedSegments = false );

    /**
     * Like AssembleTrivialPath, but follows the track length algorithm, which discards segments
     * that are fully inside pads, and truncates segments that cross into a pad (adding a straight-
     * line segment from the intersection to the pad anchor).
     *
     * @note When changing this, sync with BOARD::GetTrackLength()
     *
     * @param aStart is the item to assemble a path from.
     * @param aStartPad will be filled with the starting pad of the path, if found.
     * @param aEndPad will be filled with the ending pad of the path, if found.
     * @return an item set containing all the items in the path.
     */
    const ITEM_SET AssembleTuningPath( ROUTER_IFACE* aRouterIface, ITEM* aStart, SOLID** aStartPad = nullptr,
                                       SOLID** aEndPad = nullptr );

    const DIFF_PAIR AssembleDiffPair( SEGMENT* aStart );

    bool AssembleDiffPair( ITEM* aStart, DIFF_PAIR& aPair );

    const CLUSTER AssembleCluster( ITEM* aStart, int aLayer, double aAreaExpansionLimit = 0.0, NET_HANDLE aExcludedNet = nullptr );

private:
    const int DP_PARALLELITY_THRESHOLD = 5;

    struct PATH_RESULT
    {
        ITEM_SET    m_items;
        const JOINT* m_end;
        int         m_length;

        PATH_RESULT() : m_end( nullptr ), m_length( 0 ) {}
    };

    PATH_RESULT followBranch( const JOINT* aStartJoint, LINKED_ITEM* aPrev,
                              std::set<ITEM*>& aVisited, bool aFollowLockedSegments );

    ITEM_SET followTrivialPath( LINE* aLine, const JOINT** aTerminalJointA,
                                const JOINT** aTerminalJointB,
                                bool aFollowLockedSegments = false );

    NODE *m_world;
};

}

#endif
