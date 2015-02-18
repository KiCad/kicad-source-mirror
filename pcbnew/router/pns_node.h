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

#ifndef __PNS_NODE_H
#define __PNS_NODE_H

#include <vector>
#include <list>

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/optional.hpp>

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


/**
 * Class PNS_CLEARANCE_FUNC
 *
 * An abstract function object, returning a required clearance between two items.
 **/
class PNS_CLEARANCE_FUNC
{
public:
    virtual ~PNS_CLEARANCE_FUNC() {}
    virtual int operator()( const PNS_ITEM* aA, const PNS_ITEM* aB ) = 0;
    virtual void OverrideClearance (bool aEnable, int aNetA = 0, int aNetB = 0, int aClearance = 0) = 0;
    
};

class PNS_PCBNEW_CLEARANCE_FUNC : public PNS_CLEARANCE_FUNC 
{
public:
    PNS_PCBNEW_CLEARANCE_FUNC( BOARD *aBoard );
    virtual ~PNS_PCBNEW_CLEARANCE_FUNC();

    virtual int operator()( const PNS_ITEM* aA, const PNS_ITEM* aB );
    virtual void OverrideClearance (bool aEnable, int aNetA = 0, int aNetB = 0, int aClearance = 0);

private:
    int localPadClearance( const PNS_ITEM* aItem ) const;
    std::vector<int> m_clearanceCache;
    int m_defaultClearance;
    bool m_overrideEnabled;
    int m_overrideNetA, m_overrideNetB;
    int m_overrideClearance;
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
    const PNS_ITEM* m_head;

    ///> Item found to be colliding with m_head
    PNS_ITEM* m_item;

    ///> Hull of the colliding m_item
    SHAPE_LINE_CHAIN m_hull;

    ///> First and last intersection point between the head item and the hull
    ///> of the colliding m_item
    VECTOR2I m_ipFirst, m_ipLast;

    ///> ... and the distance thereof
    int m_distFirst, m_distLast;
};

/**
 * Struct PNS_COLLISION_FILTER
 * Used to override the decision of the collision search algorithm whether two
 * items collide.
 **/
struct PNS_COLLISION_FILTER {
    virtual bool operator()( const PNS_ITEM *aItemA, const PNS_ITEM *aItemB ) const = 0;
};

/**
 * Class PNS_NODE
 *
 * Keeps the router "world" - i.e. all the tracks, vias, solids in a
 * hierarchical and indexed way.
 * Features:
 * - spatial-indexed container for PCB item shapes
 * - collision search & clearance checking
 * - assembly of lines connecting joints, finding loops and unique paths
 * - lightweight cloning/branching (for recursive optimization and shove
 * springback)
 **/
class PNS_NODE
{
public:
    typedef boost::optional<PNS_OBSTACLE>   OPT_OBSTACLE;
    typedef std::vector<PNS_ITEM*>          ITEM_VECTOR;
    typedef std::vector<PNS_OBSTACLE>       OBSTACLES;

    PNS_NODE ();
    ~PNS_NODE ();

    ///> Returns the expected clearance between items a and b.
    int GetClearance( const PNS_ITEM* aA, const PNS_ITEM* aB ) const;

    ///> Returns the pre-set worst case clearance between any pair of items
    int GetMaxClearance() const
    {
        return m_maxClearance;
    }

    ///> Sets the worst-case clerance between any pair of items
    void SetMaxClearance( int aClearance )
    {
        m_maxClearance = aClearance;
    }

    ///> Assigns a clerance resolution function object
    void SetClearanceFunctor( PNS_CLEARANCE_FUNC* aFunc )
    {
        m_clearanceFunctor = aFunc;
    }

    ///> Returns the number of joints
    int JointCount() const
    {
        return m_joints.size();
    }

    ///> Returns the number of nodes in the inheritance chain (wrs to the root node)
    int Depth() const
    {
        return m_depth;
    }

    /**
     * Function QueryColliding()
     *
     * Finds items collliding (closer than clearance) with the item aItem.
     * @param aItem item to check collisions against
     * @param aObstacles set of colliding objects found
     * @param aKindMask mask of obstacle types to take into account
     * @param aLimitCount stop looking for collisions after finding this number of colliding items
     * @return number of obstacles found
     */
    int QueryColliding( const PNS_ITEM* aItem,
                        OBSTACLES&      aObstacles,
                        int             aKindMask = PNS_ITEM::ANY,
                        int             aLimitCount = -1 );

    /**
     * Function NearestObstacle()
     *
     * Follows the line in search of an obstacle that is nearest to the starting to the line's starting
     * point.
     * @param aItem the item to find collisions with
     * @param aKindMask mask of obstacle types to take into account
     * @return the obstacle, if found, otherwise empty.
     */
    OPT_OBSTACLE NearestObstacle( const PNS_LINE*   aItem,
                                  int               aKindMask = PNS_ITEM::ANY );

    /**
     * Function CheckColliding()
     *
     * Checks if the item collides with anything else in the world,
     * and if found, returns the obstacle.
     * @param aItem the item to find collisions with
     * @param aKindMask mask of obstacle types to take into account
     * @return the obstacle, if found, otherwise empty.
     */
    OPT_OBSTACLE CheckColliding( const PNS_ITEM*     aItem,
                                 int                 aKindMask = PNS_ITEM::ANY );


    /**
     * Function CheckColliding()
     *
     * Checks if any item in the set collides with anything else in the world,
     * and if found, returns the obstacle.
     * @param aSet set of items to find collisions with
     * @param aKindMask mask of obstacle types to take into account
     * @return the obstacle, if found, otherwise empty.
     */
    OPT_OBSTACLE CheckColliding( const PNS_ITEMSET&  aSet,
                                 int                 aKindMask = PNS_ITEM::ANY );


    /**
     * Function CheckColliding()
     *
     * Checks if 2 items collide.
     * and if found, returns the obstacle.
     * @param aItemA  first item to find collisions with
     * @param aItemB  second item to find collisions with
     * @param aKindMask mask of obstacle types to take into account
     * @return the obstacle, if found, otherwise empty.
     */
    bool CheckColliding( const PNS_ITEM*    aItemA,
                         const PNS_ITEM*    aItemB,
                         int                aKindMask = PNS_ITEM::ANY,
                         int                aForceClearance = -1 );

    /**
     * Function HitTest()
     *
     * Finds all items that contain the point aPoint.
     * @param aPoint the point
     * @return the items
     */
    const PNS_ITEMSET HitTest( const VECTOR2I& aPoint ) const;

    /**
     * Function Add()
     *
     * Adds an item to the current node.
     * @param aItem item to add
     * @param aAllowRedundant if true, duplicate items are allowed (e.g. a segment or via
     * at the same coordinates as an existing one)
     */
    void Add( PNS_ITEM* aItem, bool aAllowRedundant = false );

    /**
     * Function Remove()
     *
     * Just as the name says, removes an item from this branch.
     * @param aItem item to remove
     */
    void Remove( PNS_ITEM* aItem );

    /**
     * Function Remove()
     *
     * Just as the name says, removes a line from this branch.
     * @param aItem item to remove
     */
    void Remove( PNS_LINE& aLine );


    /**
     * Function Replace()
     *
     * Just as the name says, replaces an item with another one.
     * @param aOldItem item to be removed
     * @param aNewItem item add instead
     */
    void Replace( PNS_ITEM* aOldItem, PNS_ITEM* aNewItem );

    /**
     * Function Branch()
     *
     * Creates a lightweight copy (called branch) of self that tracks
     * the changes (added/removed items) wrs to the root. Note that if there are
     * any branches in use, their parents must NOT be deleted.
     * @return the new branch
     */
    PNS_NODE* Branch();

    /**
     * Function AssembleLine()
     *
     * Follows the joint map to assemble a line connecting two non-trivial
     * joints starting from segment aSeg.
     * @param aSeg the initial segment
     * @param aOriginSegmentIndex index of aSeg in the resulting line
     * @return the line
     */
    PNS_LINE* AssembleLine( PNS_SEGMENT* aSeg, int *aOriginSegmentIndex = NULL );

    ///> Prints the contents and joints structure
    void Dump( bool aLong = false );

    /**
     * Function GetUpdatedItems()
     *
     * Returns the lists of items removed and added in this branch, with
     * respect to the root branch.
     * @param aRemoved removed items
     * @param aAdded added items
     */
    void GetUpdatedItems( ITEM_VECTOR& aRemoved, ITEM_VECTOR& aAdded );

    /**
     * Function Commit()
     *
     * Applies the changes from a given branch (aNode) to the root branch. Called on
     * a non-root branch will fail. Calling commit also kills all children nodes of the root branch.
     * @param aNode node to commit changes from
     */
    void Commit( PNS_NODE* aNode );

    /**
     * Function FindJoint()
     *
     * Searches for a joint at a given position, layer and belonging to given net.
     * @return the joint, if found, otherwise empty
     */
    PNS_JOINT* FindJoint( const VECTOR2I& aPos, int aLayer, int aNet );

    /**
     * Function FindJoint()
     *
     * Searches for a joint at a given position, linked to given item.
     * @return the joint, if found, otherwise empty
     */
    PNS_JOINT* FindJoint( const VECTOR2I& aPos, const PNS_ITEM* aItem )
    {
        return FindJoint( aPos, aItem->Layers().Start(), aItem->Net() );
    }

#if 0
    void MapConnectivity( PNS_JOINT* aStart, std::vector<PNS_JOINT*> & aFoundJoints );

    PNS_ITEM* NearestUnconnectedItem( PNS_JOINT* aStart, int *aAnchor = NULL,
                                      int aKindMask = PNS_ITEM::ANY);

#endif

    ///> finds all lines between a pair of joints. Used by the loop removal procedure.
    int FindLinesBetweenJoints( PNS_JOINT&                  aA,
                                PNS_JOINT&                  aB,
                                std::vector<PNS_LINE*>&     aLines );

    ///> finds the joints corresponding to the ends of line aLine
    void FindLineEnds( PNS_LINE* aLine, PNS_JOINT& aA, PNS_JOINT& aB );

    ///> Destroys all child nodes. Applicable only to the root node.
    void KillChildren();

    void AllItemsInNet( int aNet, std::set<PNS_ITEM*>& aItems );

    void ClearRanks( int aMarkerMask = MK_HEAD | MK_VIOLATION );

    int FindByMarker( int aMarker, PNS_ITEMSET& aItems );
    int RemoveByMarker( int aMarker );
    void SetCollisionFilter ( PNS_COLLISION_FILTER *aFilter );

private:
    struct OBSTACLE_VISITOR;
    typedef boost::unordered_multimap<PNS_JOINT::HASH_TAG, PNS_JOINT> JOINT_MAP;
    typedef JOINT_MAP::value_type TagJointPair;

    /// nodes are not copyable
    PNS_NODE( const PNS_NODE& aB );
    PNS_NODE& operator=( const PNS_NODE& aB );

    ///> tries to find matching joint and creates a new one if not found
    PNS_JOINT& touchJoint( const VECTOR2I&      aPos,
                           const PNS_LAYERSET&  aLayers,
                           int                  aNet );

    ///> touches a joint and links it to an m_item
    void linkJoint( const VECTOR2I& aPos, const PNS_LAYERSET& aLayers,
                    int aNet, PNS_ITEM* aWhere );

    ///> unlinks an item from a joint
    void unlinkJoint( const VECTOR2I& aPos, const PNS_LAYERSET& aLayers,
                        int aNet, PNS_ITEM* aWhere );

    ///> helpers for adding/removing items
    void addSolid( PNS_SOLID* aSeg );
    void addSegment( PNS_SEGMENT* aSeg, bool aAllowRedundant );
    void addLine( PNS_LINE* aLine, bool aAllowRedundant );
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

    ///> checks if this branch contains an updated version of the m_item
    ///> from the root branch.
    bool overrides( PNS_ITEM* aItem ) const
    {
        return m_override.find( aItem ) != m_override.end();
    }

    PNS_SEGMENT *findRedundantSegment ( PNS_SEGMENT* aSeg );

    ///> scans the joint map, forming a line starting from segment (current).
    void followLine( PNS_SEGMENT*    aCurrent,
                     bool            aScanDirection,
                     int&            aPos,
                     int             aLimit,
                     VECTOR2I*       aCorners,
                     PNS_SEGMENT**   aSegments,
                     bool&           aGuardHit );

    ///> hash table with the joints, linking the items. Joints are hashed by
    ///> their position, layer set and net.
    JOINT_MAP m_joints;

    ///> node this node was branched from
    PNS_NODE* m_parent;

    ///> root node of the whole hierarchy
    PNS_NODE* m_root;

    ///> list of nodes branched from this one
    std::vector<PNS_NODE*> m_children;

    ///> hash of root's items that have been changed in this node
    boost::unordered_set<PNS_ITEM*> m_override;

    ///> worst case item-item clearance
    int m_maxClearance;

    ///> Clearance resolution functor
    PNS_CLEARANCE_FUNC* m_clearanceFunctor;

    ///> Geometric/Net index of the items
    PNS_INDEX* m_index;

    ///> depth of the node (number of parent nodes in the inheritance chain)
    int m_depth;

    ///> optional collision filtering object
    PNS_COLLISION_FILTER *m_collisionFilter;
};

#endif
