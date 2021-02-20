/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
#include <unordered_set>
#include <unordered_map>

#include <core/optional.h>
#include <core/minoptmax.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_index.h>

#include "pns_item.h"
#include "pns_joint.h"
#include "pns_itemset.h"

namespace PNS {

class ARC;
class SEGMENT;
class LINE;
class SOLID;
class VIA;
class INDEX;
class ROUTER;
class NODE;

/**
 * An abstract function object, returning a design rule (clearance, diff pair gap, etc) required
 * between two items.
 */

enum class CONSTRAINT_TYPE
{
    CT_CLEARANCE = 1,
    CT_DIFF_PAIR_GAP = 2,
    CT_LENGTH = 3,
    CT_WIDTH = 4,
    CT_VIA_DIAMETER = 5,
    CT_VIA_HOLE = 6,
    CT_HOLE_CLEARANCE = 7,
    CT_EDGE_CLEARANCE = 8,
    CT_HOLE_TO_HOLE = 9
};

struct CONSTRAINT
{
    CONSTRAINT_TYPE m_Type;
    MINOPTMAX<int>  m_Value;
    bool            m_Allowed;
    wxString        m_RuleName;
    wxString        m_FromName;
    wxString        m_ToName;
};

class RULE_RESOLVER
{
public:
    virtual ~RULE_RESOLVER() {}

    virtual int Clearance( const ITEM* aA, const ITEM* aB ) = 0;
    virtual int HoleClearance( const ITEM* aA, const ITEM* aB ) = 0;
    virtual int HoleToHoleClearance( const ITEM* aA, const ITEM* aB ) = 0;

    virtual int DpCoupledNet( int aNet ) = 0;
    virtual int DpNetPolarity( int aNet ) = 0;
    virtual bool DpNetPair( const ITEM* aItem, int& aNetP, int& aNetN ) = 0;

    virtual bool IsDiffPair( const ITEM* aA, const ITEM* aB ) = 0;

    virtual bool QueryConstraint( CONSTRAINT_TYPE aType, const PNS::ITEM* aItemA,
                                  const PNS::ITEM* aItemB, int aLayer,
                                  PNS::CONSTRAINT* aConstraint ) = 0;

    virtual wxString NetName( int aNet ) = 0;
};

/**
 * Hold an object colliding with another object, along with some useful data about the collision.
 */
struct OBSTACLE
{
    const ITEM*      m_head;        ///< Item we search collisions with

    ITEM*            m_item;        ///< Item found to be colliding with m_head
    SHAPE_LINE_CHAIN m_hull;        ///< Hull of the colliding m_item
    VECTOR2I         m_ipFirst;     ///< First intersection between m_head and m_hull
    int              m_distFirst;   ///< ... and the distance thereof
};

class OBSTACLE_VISITOR
{
public:
    OBSTACLE_VISITOR( const ITEM* aItem );

    virtual ~OBSTACLE_VISITOR()
    {
    }

    void SetWorld( const NODE* aNode, const NODE* aOverride = NULL );

    virtual bool operator()( ITEM* aCandidate ) = 0;

protected:
    bool visit( ITEM* aCandidate );

protected:
    const ITEM* m_item;             ///< the item we are looking for collisions with

    const NODE* m_node;             ///< node we are searching in (either root or a branch)
    const NODE* m_override;         ///< node that overrides root entries
    int         m_extraClearance;   ///< additional clearance
};

/**
 * Keep the router "world" - i.e. all the tracks, vias, solids in a hierarchical and indexed way.
 *
 * Features:
 * - spatial-indexed container for PCB item shapes.
 * - collision search & clearance checking.
 * - assembly of lines connecting joints, finding loops and unique paths.
 * - lightweight cloning/branching (for recursive optimization and shove springback).
 **/
class NODE
{
public:
    typedef OPT<OBSTACLE>         OPT_OBSTACLE;
    typedef std::vector<ITEM*>    ITEM_VECTOR;
    typedef std::vector<OBSTACLE> OBSTACLES;

    NODE();
    ~NODE();

    ///< Return the expected clearance between items a and b.
    int GetClearance( const ITEM* aA, const ITEM* aB ) const;
    int GetHoleClearance( const ITEM* aA, const ITEM* aB ) const;
    int GetHoleToHoleClearance( const ITEM* aA, const ITEM* aB ) const;

    ///< Return the pre-set worst case clearance between any pair of items.
    int GetMaxClearance() const
    {
        return m_maxClearance;
    }

    ///< Set the worst-case clearance between any pair of items.
    void SetMaxClearance( int aClearance )
    {
        m_maxClearance = aClearance;
    }

    ///< Assign a clearance resolution function object.
    void SetRuleResolver( RULE_RESOLVER* aFunc )
    {
        m_ruleResolver = aFunc;
    }

    RULE_RESOLVER* GetRuleResolver() const
    {
        return m_ruleResolver;
    }

    ///< Return the number of joints.
    int JointCount() const
    {
        return m_joints.size();
    }

    ///< Return the number of nodes in the inheritance chain (wrs to the root node).
    int Depth() const
    {
        return m_depth;
    }

    /**
     * Find items colliding (closer than clearance) with the item \a aItem.
     *
     * @param aItem item to check collisions against
     * @param aObstacles set of colliding objects found
     * @param aKindMask mask of obstacle types to take into account
     * @param aLimitCount stop looking for collisions after finding this number of colliding items
     * @return number of obstacles found
     */
    int QueryColliding( const ITEM* aItem, OBSTACLES& aObstacles, int aKindMask = ITEM::ANY_T,
                        int aLimitCount = -1, bool aDifferentNetsOnly = true );

    int QueryJoints( const BOX2I& aBox, std::vector<JOINT*>& aJoints,
                     LAYER_RANGE aLayerMask = LAYER_RANGE::All(), int aKindMask = ITEM::ANY_T );

    /**
     * Follow the line in search of an obstacle that is nearest to the starting to the line's
     * starting point.
     *
     * @param aLine the item to find collisions with
     * @param aKindMask mask of obstacle types to take into account
     * @return the obstacle, if found, otherwise empty.
     */
    OPT_OBSTACLE NearestObstacle( const LINE* aLine, int aKindMask = ITEM::ANY_T,
                                  const std::set<ITEM*>* aRestrictedSet = NULL );

    /**
     * Check if the item collides with anything else in the world, and if found, returns the
     * obstacle.
     *
     * @param aItem the item to find collisions with
     * @param aKindMask mask of obstacle types to take into account
     * @return the obstacle, if found, otherwise empty.
     */
    OPT_OBSTACLE CheckColliding( const ITEM* aItem, int aKindMask = ITEM::ANY_T );


    /**
     * Check if any item in the set collides with anything else in the world, and if found,
     * returns the obstacle.
     *
     * @param aSet set of items to find collisions with.
     * @param aKindMask mask of obstacle types to take into account.
     * @return the obstacle, if found, otherwise empty.
     */
    OPT_OBSTACLE CheckColliding( const ITEM_SET&  aSet, int aKindMask = ITEM::ANY_T );

    /**
     * Find all items that contain the point \a aPoint.
     *
     * @param aPoint the point.
     * @return the items.
     */
    const ITEM_SET HitTest( const VECTOR2I& aPoint ) const;

    /**
     * Add an item to the current node.
     *
     * @param aSegment item to add.
     * @param aAllowRedundant if true, duplicate items are allowed (e.g. a segment or via
     *                        at the same coordinates as an existing one).
     * @return true if added
     */
    bool Add( std::unique_ptr< SEGMENT > aSegment, bool aAllowRedundant = false );
    void Add( std::unique_ptr< SOLID >   aSolid );
    void Add( std::unique_ptr< VIA >     aVia );
    void Add( std::unique_ptr< ARC >     aArc );

    void Add( LINE& aLine, bool aAllowRedundant = false );

    /**
     * Remove an item from this branch.
     */
    void Remove( ARC* aArc );
    void Remove( SOLID* aSolid );
    void Remove( VIA* aVia );
    void Remove( SEGMENT* aSegment );
    void Remove( ITEM* aItem );

    /**
     * Removes a line from this branch.
     *
     * @param aLine item to remove
     */
    void Remove( LINE& aLine );

    /**
     * Replace an item with another one.
     *
     * @param aOldItem item to be removed
     * @param aNewItem item add instead
     */
    void Replace( ITEM* aOldItem, std::unique_ptr< ITEM > aNewItem );
    void Replace( LINE& aOldLine, LINE& aNewLine );

    /**
     * Create a lightweight copy (called branch) of self that tracks the changes (added/removed
     * items) wrs to the root.
     *
     * @note If there are any branches in use, their parents must **not** be deleted.
     *
     * @return the new branch.
     */
    NODE* Branch();

    /**
     * Follow the joint map to assemble a line connecting two non-trivial joints starting from
     * segment \a aSeg.
     *
     * @param aSeg the initial segment.
     * @param aOriginSegmentIndex index of aSeg in the resulting line.
     * @return the line
     */
    const LINE AssembleLine( LINKED_ITEM* aSeg, int* aOriginSegmentIndex = NULL,
                             bool aStopAtLockedJoints = false );

    ///< Print the contents and joints structure.
    void Dump( bool aLong = false );

    /**
     * Return the list of items removed and added in this branch with respect to the root branch.
     *
     * @param aRemoved removed items.
     * @param aAdded added items.
     */
    void GetUpdatedItems( ITEM_VECTOR& aRemoved, ITEM_VECTOR& aAdded );

    /**
     * Apply the changes from a given branch (aNode) to the root branch.
     *
     * Calling on a non-root branch will fail.  Calling commit also kills all children nodes of
     *  the root branch.
     *
     * @param aNode node to commit changes from.
     */
    void Commit( NODE* aNode );

    /**
     * Search for a joint at a given position, layer and belonging to given net.
     *
     * @return the joint, if found, otherwise empty.
     */
    JOINT* FindJoint( const VECTOR2I& aPos, int aLayer, int aNet );

    void LockJoint( const VECTOR2I& aPos, const ITEM* aItem, bool aLock );

    /**
     * Search for a joint at a given position, linked to given item.
     *
     * @return the joint, if found, otherwise empty.
     */
    JOINT* FindJoint( const VECTOR2I& aPos, const ITEM* aItem )
    {
        return FindJoint( aPos, aItem->Layers().Start(), aItem->Net() );
    }

    ///< Find all lines between a pair of joints. Used by the loop removal procedure.
    int FindLinesBetweenJoints( const JOINT& aA, const JOINT& aB, std::vector<LINE>& aLines );

    ///< Find the joints corresponding to the ends of line \a aLine.
    void FindLineEnds( const LINE& aLine, JOINT& aA, JOINT& aB );

    ///< Destroy all child nodes. Applicable only to the root node.
    void KillChildren();

    void AllItemsInNet( int aNet, std::set<ITEM*>& aItems, int aKindMask = -1 );

    void ClearRanks( int aMarkerMask = MK_HEAD | MK_VIOLATION | MK_HOLE );

    void RemoveByMarker( int aMarker );

    ITEM* FindItemByParent( const BOARD_ITEM* aParent );

    bool HasChildren() const
    {
        return !m_children.empty();
    }

    NODE* GetParent() const
    {
        return m_parent;
    }

    ///< Check if this branch contains an updated version of the m_item from the root branch.
    bool Overrides( ITEM* aItem ) const
    {
        return m_override.find( aItem ) != m_override.end();
    }

private:
    void Add( std::unique_ptr< ITEM > aItem, bool aAllowRedundant = false );

    /// nodes are not copyable
    NODE( const NODE& aB );
    NODE& operator=( const NODE& aB );

    ///< Try to find matching joint and creates a new one if not found.
    JOINT& touchJoint( const VECTOR2I& aPos, const LAYER_RANGE& aLayers, int aNet );

    ///< Touch a joint and links it to an m_item.
    void linkJoint( const VECTOR2I& aPos, const LAYER_RANGE& aLayers, int aNet, ITEM* aWhere );

    ///< Unlink an item from a joint.
    void unlinkJoint( const VECTOR2I& aPos, const LAYER_RANGE& aLayers, int aNet, ITEM* aWhere );

    ///< Helpers for adding/removing items.
    void addSolid( SOLID* aSeg );
    void addSegment( SEGMENT* aSeg );
    void addVia( VIA* aVia );
    void addArc( ARC* aVia );

    void removeSolidIndex( SOLID* aSeg );
    void removeSegmentIndex( SEGMENT* aSeg );
    void removeViaIndex( VIA* aVia );
    void removeArcIndex( ARC* aVia );

    void doRemove( ITEM* aItem );
    void unlinkParent();
    void releaseChildren();
    void releaseGarbage();
    void rebuildJoint( JOINT* aJoint, ITEM* aItem );

    bool isRoot() const
    {
        return m_parent == NULL;
    }

    SEGMENT* findRedundantSegment( const VECTOR2I& A, const VECTOR2I& B, const LAYER_RANGE& lr,
                                   int aNet );
    SEGMENT* findRedundantSegment( SEGMENT* aSeg );

    ARC* findRedundantArc( const VECTOR2I& A, const VECTOR2I& B, const LAYER_RANGE& lr, int aNet );
    ARC* findRedundantArc( ARC* aSeg );

    ///< Scan the joint map, forming a line starting from segment (current).
    void followLine( LINKED_ITEM* aCurrent, bool aScanDirection, int& aPos, int aLimit,
                     VECTOR2I* aCorners, LINKED_ITEM** aSegments, bool* aArcReversed,
                     bool& aGuardHit, bool aStopAtLockedJoints );

private:
    struct DEFAULT_OBSTACLE_VISITOR;
    typedef std::unordered_multimap<JOINT::HASH_TAG, JOINT, JOINT::JOINT_TAG_HASH> JOINT_MAP;
    typedef JOINT_MAP::value_type TagJointPair;

    JOINT_MAP       m_joints;           ///< hash table with the joints, linking the items. Joints
                                        ///< are hashed by their position, layer set and net.

    NODE*           m_parent;           ///< node this node was branched from
    NODE*           m_root;             ///< root node of the whole hierarchy
    std::set<NODE*> m_children;         ///< list of nodes branched from this one

    std::unordered_set<ITEM*> m_override;   ///< hash of root's items that have been changed
                                            ///< in this node

    int             m_maxClearance;     ///< worst case item-item clearance
    RULE_RESOLVER*  m_ruleResolver;     ///< Design rules resolver
    INDEX*          m_index;            ///< Geometric/Net index of the items
    int             m_depth;            ///< depth of the node (number of parent nodes in the
                                        ///< inheritance chain)

    std::unordered_set<ITEM*> m_garbageItems;
};

}

#endif
