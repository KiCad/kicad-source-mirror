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
#ifndef __PNS_NODE_H
#define __PNS_NODE_H

#include <vector>
#include <list>

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/optional.hpp>
#include <boost/smart_ptr.hpp>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_index.h>

#include "pns_item.h"
#include "pns_joint.h"
#include "pns_itemset.h"

class PNS_SEGMENT;
class PNS_LINE;
class PNS_SOLID;
class PNS_VIA;
class PNS_RATSNEST;
class PNS_INDEX;

using boost::shared_ptr;

class PNS_CLEARANCE_FUNC
{
public:
    virtual int operator()( const PNS_ITEM* a, const PNS_ITEM* b ) = 0;
};

/**
 * Struct PNS_OBSTACLE
 *
 * Holds an object colliding with another object, along with
 * some useful data about the collision.
 **/
struct PNS_OBSTACLE
{
    ///> Item we search collisions with
    PNS_ITEM* head;

    ///> Item found to be colliding with head
    PNS_ITEM* item;

    ///> Hull of the colliding item
    SHAPE_LINE_CHAIN hull;

    ///> First and last intersection point between the head item and the hull
    ///> of the colliding item
    VECTOR2I ip_first, ip_last;

    ///> ... and the distance thereof
    int dist_first, dist_last;
};

/**
 * Class PNS_NODE
 *
 * Keeps the router "world" - i.e. all the tracks, vias, solids in a 
 * hierarchical and indexed way.
 * Features:
 * - spatial-indexed container for PCB item shapes
 * - collision search (with clearance checking)
 * - assembly of lines connecting joints, finding loops and unique paths
 * - lightweight cloning/branching (for recursive optimization and shove 
 * springback)
 **/

class PNS_NODE
{
public:
    typedef boost::optional<PNS_OBSTACLE>   OptObstacle;
    typedef std::vector<PNS_ITEM*>          ItemVector;
    typedef std::vector<PNS_OBSTACLE>       Obstacles;
    typedef boost::optional<PNS_JOINT>      OptJoint;

    PNS_NODE();
    ~PNS_NODE();

    ///> Returns the expected clearance between items a and b.
    int GetClearance( const PNS_ITEM* a, const PNS_ITEM* b ) const;

    ///> Returns the pre-set worst case clearance between any pair of items
    int GetMaxClearance() const
    {
        return m_maxClearance;
    }

    void SetMaxClearance( int aClearance )
    {
        m_maxClearance = aClearance;
    }

    void SetClearanceFunctor( PNS_CLEARANCE_FUNC* aFunc )
    {
        m_clearanceFunctor = aFunc;
    }

    ///> Finds items that collide with aItem and stores collision information
    ///> in aObstacles.
    int QueryColliding( const PNS_ITEM* aItem,
            Obstacles& aObstacles,
            int aKindMask = PNS_ITEM::ANY,
            int aLimitCount = -1 );

    ///> Finds the nearest item that collides with aItem.
    OptObstacle NearestObstacle( const PNS_LINE* aItem, int aKindMask = PNS_ITEM::ANY );

    ///> Checks if the item collides with anything else in the world,
    ///> and returns it if so.
    OptObstacle CheckColliding( const PNS_ITEM* aItem, int aKindMask = PNS_ITEM::ANY );

    ///> Checks if two items collide [deprecated].
    bool CheckColliding( const PNS_ITEM* aItemA,
            const PNS_ITEM* aItemB,
            int aKindMask = PNS_ITEM::ANY );

    ///> Hit detection
    const PNS_ITEMSET HitTest( const VECTOR2I& aPoint );

    void Add( PNS_ITEM* aItem );
    void Remove( PNS_ITEM* aItem );
    void Replace( PNS_ITEM* aOldItem, PNS_ITEM* aNewItem );

    ///> Creates a lightweight copy ("branch") of self. Note that if there are 
    ///> any branches in use, their parents must NOT be deleted.
    PNS_NODE* Branch();

    ///> Assembles a line connecting two non-trivial joints the 
    ///> segment aSeg belongs to.
    PNS_LINE* AssembleLine( PNS_SEGMENT* aSeg,
            const OptJoint& a = OptJoint(), const OptJoint& b = OptJoint() );

    ///> Dumps the contents and joints structure
    void Dump( bool aLong = false );

    ///> Returns the number of joints
    int JointCount() const
    {
        return m_joints.size();
    }

    ///> Returns the lists of items removed and added in this branch, with
    ///> respect to the root.
    void GetUpdatedItems( ItemVector& aRemoved, ItemVector& aAdded );

    ///> Copies the changes from a given branch (aNode) to the root. Called on
    ///> a non-root branch will fail.
    void Commit( PNS_NODE* aNode );

    ///> finds a joint at a given position, layer and nets
    const OptJoint FindJoint( const VECTOR2I& aPos, int aLayer, int aNet );

    ///> finds all linest between a pair of joints. Used by the loop removal engine.
    int FindLinesBetweenJoints( PNS_JOINT& a, PNS_JOINT& b, 
                                std::vector<PNS_LINE*>& aLines );

    ///> finds the joints corresponding to the ends of line aLine
    void FindLineEnds( PNS_LINE* aLine, PNS_JOINT& a, PNS_JOINT& b );

    ///> finds all joints that have an (in)direct connection(s) 
    ///> (i.e. segments/vias) with the joint aJoint.
    void FindConnectedJoints( const PNS_JOINT& aJoint, 
                              std::vector<PNS_JOINT*>& aConnectedJoints );

    ///> Destroys all child nodes. Applicable only to the root node.
    void KillChildren();

    void AllItemsInNet( int aNet, std::list<PNS_ITEM*>& aItems );

private:
    struct obstacleVisitor;
    typedef boost::unordered_multimap<PNS_JOINT::HashTag, PNS_JOINT> JointMap;
    typedef JointMap::value_type TagJointPair;

    /// nodes are not copyable
    PNS_NODE( const PNS_NODE& b );
    PNS_NODE& operator=( const PNS_NODE& b );

    ///> tries to find matching joint and creates a new one if not found
    PNS_JOINT& touchJoint( const VECTOR2I& aPos, const PNS_LAYERSET& aLayers, 
                            int aNet );

    ///> touches a joint and links it to an item
    void linkJoint( const VECTOR2I& aPos, const PNS_LAYERSET& aLayers, 
                        int aNet, PNS_ITEM* aWhere );

    ///> unlinks an item from a joint
    void unlinkJoint( const VECTOR2I& aPos, const PNS_LAYERSET& aLayers, 
                        int aNet, PNS_ITEM* aWhere );

    ///> helpers for adding/removing items
    void addSolid( PNS_SOLID* aSeg );
    void addSegment( PNS_SEGMENT* aSeg );
    void addLine( PNS_LINE* aLine );
    void addVia( PNS_VIA* aVia );
    void removeSolid( PNS_SOLID* aSeg );
    void removeLine( PNS_LINE* aLine );
    void removeSegment( PNS_SEGMENT* aSeg );
    void removeVia( PNS_VIA* aVia );

    void doRemove( PNS_ITEM* aItem );
    void unlinkParent();
    void releaseChildren();

    bool isRoot() const
    {
        return m_parent == NULL;
    }

    ///> checks if this branch contains an updated version of the item 
    ///> from the root branch.
    bool overrides( PNS_ITEM* aItem ) const
    {
        return m_override.find( aItem ) != m_override.end();
    }

    ///> scans the joint map, forming a line starting from segment (current).
    void followLine( PNS_SEGMENT* current,
            bool scanDirection,
            int& pos,
            int limit,
            VECTOR2I* corners,
            PNS_SEGMENT** segments );

    ///> spatial index of all items
    // SHAPE_INDEX_LIST<PNS_ITEM *> m_items;

    ///> hash table with the joints, linking the items. Joints are hashed by 
    ///> their position, layer set and net.
    JointMap m_joints;

    ///> node this node was branched from
    PNS_NODE* m_parent;

    ///> root node of the whole hierarchy
    PNS_NODE* m_root;

    ///> list of nodes branched from this one
    std::vector<PNS_NODE*> m_children;

    ///> hash of root's items that are more recent in this node
    boost::unordered_set<PNS_ITEM*> m_override;

    ///> worst case item-item clearance
    int m_maxClearance;

    ///> Clearance resolution functor
    PNS_CLEARANCE_FUNC* m_clearanceFunctor;

    ///> Geometric/Net index of the items
    PNS_INDEX* m_index;

    ///> list of currently processed obstacles.
    Obstacles m_obstacleList;
};

#endif
