/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <set>
#include <core/minoptmax.h>

#include <geometry/shape_line_chain.h>
#include <geometry/shape_index.h>

#include "pns_item.h"
#include "pns_joint.h"
#include "pns_itemset.h"

class ZONE;
namespace PNS {

class ARC;
class SEGMENT;
class LINE;
class SOLID;
class VIA;
class INDEX;
class ROUTER;
class NODE;


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
    CT_HOLE_TO_HOLE = 9,
    CT_DIFF_PAIR_SKEW = 10,
    CT_MAX_UNCOUPLED = 11,
    CT_PHYSICAL_CLEARANCE = 12
};

/**
 * An abstract function object, returning a design rule (clearance, diff pair gap, etc) required
 * between two items.
 */

struct CONSTRAINT
{
    CONSTRAINT_TYPE m_Type;
    MINOPTMAX<int>  m_Value;
    bool            m_Allowed;
    wxString        m_RuleName;
    wxString        m_FromName;
    wxString        m_ToName;
    bool            m_IsTimeDomain;
};


/**
 * Hold an object colliding with another object, along with some useful data about the collision.
 */
struct OBSTACLE
{
    ITEM*            m_head = nullptr;           ///< Line we search collisions against
    ITEM*            m_item = nullptr;           ///< Item found to be colliding with m_head
    VECTOR2I         m_ipFirst;        ///< First intersection between m_head and m_hull
    int              m_clearance;
    VECTOR2I         m_pos;
    int              m_distFirst;      ///< ... and the distance thereof
    int              m_maxFanoutWidth; ///< worst case (largest) width of the tracks connected to the item

    bool operator==(const OBSTACLE& other) const
    {
        return m_head == other.m_head && m_item == other.m_item;
    }

    bool operator<(const OBSTACLE& other) const
    {
        if( (uintptr_t)m_head < (uintptr_t)other.m_head )
            return true;
        else if ( m_head == other.m_head )
            return (uintptr_t)m_item < (uintptr_t)other.m_item;
        return false;
    }
};


struct COLLISION_SEARCH_OPTIONS
{
    bool m_differentNetsOnly = true;
    int m_overrideClearance = -1;
    int m_limitCount = -1;
    int m_kindMask = -1;
    bool m_useClearanceEpsilon = true;
    std::function<bool(const ITEM*)> m_filter = nullptr;
    int m_layer = -1;
};


struct COLLISION_SEARCH_CONTEXT
{
    COLLISION_SEARCH_CONTEXT( std::set<OBSTACLE>& aObs, const COLLISION_SEARCH_OPTIONS aOpts = COLLISION_SEARCH_OPTIONS() ) :
        obstacles( aObs ),
        options( aOpts )
    {
    }

    std::set<OBSTACLE>& obstacles;
    const COLLISION_SEARCH_OPTIONS options;
};


class RULE_RESOLVER
{
public:
    virtual ~RULE_RESOLVER() {}

    virtual int Clearance( const ITEM* aA, const ITEM* aB, bool aUseClearanceEpsilon = true ) = 0;

    virtual NET_HANDLE DpCoupledNet( NET_HANDLE aNet ) = 0;
    virtual int DpNetPolarity( NET_HANDLE aNet ) = 0;
    virtual bool DpNetPair( const ITEM* aItem, NET_HANDLE& aNetP, NET_HANDLE& aNetN ) = 0;

    virtual int NetCode( NET_HANDLE aNet ) = 0;
    virtual wxString NetName( NET_HANDLE aNet ) = 0;

    virtual bool IsInNetTie( const ITEM* aA ) = 0;
    virtual bool IsNetTieExclusion( const ITEM* aItem, const VECTOR2I& aCollisionPos,
                                    const ITEM* aCollidingItem ) = 0;

    virtual bool IsDrilledHole( const PNS::ITEM* aItem ) = 0;
    virtual bool IsNonPlatedSlot( const PNS::ITEM* aItem ) = 0;

    /**
     * @return true if \a aObstacle is a keepout.  Set \a aEnforce if said keepout's rules
     *         exclude \a aItem.
     */
    virtual bool IsKeepout( const ITEM* aObstacle, const ITEM* aItem, bool* aEnforce ) = 0;

    virtual bool QueryConstraint( CONSTRAINT_TYPE aType, const ITEM* aItemA, const ITEM* aItemB,
                                  int aLayer, CONSTRAINT* aConstraint ) = 0;

    virtual void ClearCacheForItems( std::vector<const ITEM*>& aItems ) {}
    virtual void ClearCaches() {}
    virtual void ClearTemporaryCaches() {}

    virtual int ClearanceEpsilon() const { return 0; }

    virtual const SHAPE_LINE_CHAIN& HullCache( const ITEM* aItem, int aClearance,
                                               int aWalkaroundThickness, int aLayer )
    {
        static SHAPE_LINE_CHAIN empty;
        empty = aItem->Hull( aClearance, aWalkaroundThickness, aLayer );
        return empty;
    }
};


class OBSTACLE_VISITOR
{
public:
    OBSTACLE_VISITOR( const ITEM* aItem );

    virtual ~OBSTACLE_VISITOR()
    {
    }

    void SetWorld( const NODE* aNode, const NODE* aOverride = nullptr );

    void SetLayerContext( int aLayer ) { m_layerContext = aLayer; }
    void ClearLayerContext() { m_layerContext = std::nullopt; }

    virtual bool operator()( ITEM* aCandidate ) = 0;

protected:
    bool visit( ITEM* aCandidate );

protected:
    const ITEM* m_item;             ///< the item we are looking for collisions with

    const NODE* m_node;             ///< node we are searching in (either root or a branch)
    const NODE* m_override;         ///< node that overrides root entries
    std::optional<int> m_layerContext;
};


class LAYER_CONTEXT_SETTER
{
public:
    LAYER_CONTEXT_SETTER( OBSTACLE_VISITOR& aVisitor, int aLayer ) :
            m_visitor( aVisitor )
    {
        m_visitor.SetLayerContext( aLayer );
    }

    ~LAYER_CONTEXT_SETTER()
    {
        m_visitor.ClearLayerContext();
    }

private:
    OBSTACLE_VISITOR& m_visitor;
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
class NODE : public ITEM_OWNER
{
public:

///< Supported item types
    enum COLLISION_QUERY_SCOPE
    {
        CQS_ALL_RULES               =    1, ///< check all rules
        CQS_IGNORE_HOLE_CLEARANCE   =    2  ///< check everything except hole2hole / hole2copper
    };

    typedef std::optional<OBSTACLE>         OPT_OBSTACLE;
    typedef std::vector<ITEM*>    ITEM_VECTOR;
    typedef std::set<OBSTACLE>    OBSTACLES;

    NODE();
    ~NODE();

    ///< Return the expected clearance between items a and b.
    int GetClearance( const ITEM* aA, const ITEM* aB, bool aUseClearanceEpsilon = true ) const;

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
    int QueryColliding( const ITEM* aItem, OBSTACLES& aObstacles,
                        const COLLISION_SEARCH_OPTIONS& aOpts = COLLISION_SEARCH_OPTIONS() ) const;

    int QueryJoints( const BOX2I& aBox, std::vector<JOINT*>& aJoints,
                     PNS_LAYER_RANGE aLayerMask = PNS_LAYER_RANGE::All(), int aKindMask = ITEM::ANY_T );

    /**
     * Follow the line in search of an obstacle that is nearest to the starting to the line's
     * starting point.
     *
     * @param aLine the item to find collisions with
     * @param aOpts options for the search
     * @return the obstacle, if found, otherwise empty.
     */
    OPT_OBSTACLE NearestObstacle( const LINE* aLine,
                                  const COLLISION_SEARCH_OPTIONS& aOpts = COLLISION_SEARCH_OPTIONS() );

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
     * Check if the item collides with anything else in the world, and if found, returns the
     * obstacle.
     *
     * @param aItem the item to find collisions with
     * @param aOpts options for the search
     * @return the obstacle, if found, otherwise empty.
     */
    OPT_OBSTACLE CheckColliding( const ITEM* aItem, const COLLISION_SEARCH_OPTIONS& aOpts );

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
    bool Add( std::unique_ptr<SEGMENT> aSegment, bool aAllowRedundant = false );
    void Add( std::unique_ptr<SOLID>   aSolid );
    void Add( std::unique_ptr<VIA>     aVia );
    bool Add( std::unique_ptr<ARC>     aArc, bool aAllowRedundant = false );

    void Add( LINE& aLine, bool aAllowRedundant = false );

    void AddEdgeExclusion( std::unique_ptr<SHAPE> aShape );
    bool QueryEdgeExclusions( const VECTOR2I& aPos ) const;

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
    void Replace( LINE& aOldLine, LINE& aNewLine, bool aAllowRedundantSegments = false );

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
     * @param aStopAtLockedJoints will terminate the line at the first locked joint encountered
     * @param aFollowLockedSegments will consider a joint between a locked segment and an unlocked
     *                              segment of the same width as a trivial joint.
     * @param aAllowSegmentSizeMismatch will allow segments of different widths to be connected
     * @return the line
     */
    const LINE AssembleLine( LINKED_ITEM* aSeg, int* aOriginSegmentIndex = nullptr,
                             bool aStopAtLockedJoints = false,
                             bool aFollowLockedSegments = false,
                             bool aAllowSegmentSizeMismatch = true );

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
    const JOINT* FindJoint( const VECTOR2I& aPos, int aLayer, NET_HANDLE aNet ) const;

    void LockJoint( const VECTOR2I& aPos, const ITEM* aItem, bool aLock );

    /**
     * Search for a joint at a given position, linked to given item.
     *
     * @return the joint, if found, otherwise empty.
     */
    const JOINT* FindJoint( const VECTOR2I& aPos, const ITEM* aItem ) const
    {
        return FindJoint( aPos, aItem->Layers().Start(), aItem->Net() );
    }

    ///< Find all lines between a pair of joints. Used by the loop removal procedure.
    int FindLinesBetweenJoints( const JOINT& aA, const JOINT& aB, std::vector<LINE>& aLines );

    ///< Find the joints corresponding to the ends of line \a aLine.
    void FindLineEnds( const LINE& aLine, JOINT& aA, JOINT& aB );

    ///< Destroy all child nodes. Applicable only to the root node.
    void KillChildren();

    void AllItemsInNet( NET_HANDLE aNet, std::set<ITEM*>& aItems, int aKindMask = -1 );

    void ClearRanks( int aMarkerMask = MK_HEAD | MK_VIOLATION );

    void RemoveByMarker( int aMarker );

    ITEM* FindItemByParent( const BOARD_ITEM* aParent );

    std::vector<ITEM*> FindItemsByParent( const BOARD_ITEM* aParent );

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

    void FixupVirtualVias();

    void AddRaw( ITEM* aItem, bool aAllowRedundant = false )
    {
        add( aItem, aAllowRedundant );
    }

    const std::unordered_set<ITEM*>& GetOverrides() const
    {
        return m_override;
    }

    VIA* FindViaByHandle ( const VIA_HANDLE& handle ) const;

private:
    void add( ITEM* aItem, bool aAllowRedundant = false );

    /// nodes are not copyable
    NODE( const NODE& aB );
    NODE& operator=( const NODE& aB );

    ///< Try to find matching joint and creates a new one if not found.
    JOINT& touchJoint( const VECTOR2I& aPos, const PNS_LAYER_RANGE& aLayers, NET_HANDLE aNet );

    ///< Touch a joint and links it to an m_item.
    void linkJoint( const VECTOR2I& aPos, const PNS_LAYER_RANGE& aLayers, NET_HANDLE aNet,
                    ITEM* aWhere );

    ///< Unlink an item from a joint.
    void unlinkJoint( const VECTOR2I& aPos, const PNS_LAYER_RANGE& aLayers, NET_HANDLE aNet,
                      ITEM* aWhere );

    ///< Helpers for adding/removing items.
    void addSolid( SOLID* aSeg );
    void addSegment( SEGMENT* aSeg );
    void addVia( VIA* aVia );
    void addArc( ARC* aVia );
    void addHole( HOLE* aHole );

    void removeSolidIndex( SOLID* aSeg );
    void removeSegmentIndex( SEGMENT* aSeg );
    void removeViaIndex( VIA* aVia );
    void removeArcIndex( ARC* aVia );

    void doRemove( ITEM* aItem );
    void unlinkParent();
    void releaseChildren();
    void releaseGarbage();
    void rebuildJoint( const JOINT* aJoint, const ITEM* aItem );

    bool isRoot() const
    {
        return m_parent == nullptr;
    }

    SEGMENT* findRedundantSegment( const VECTOR2I& A, const VECTOR2I& B, const PNS_LAYER_RANGE& lr,
                                   NET_HANDLE aNet );
    SEGMENT* findRedundantSegment( SEGMENT* aSeg );

    ARC* findRedundantArc( const VECTOR2I& A, const VECTOR2I& B, const PNS_LAYER_RANGE& lr,
                           NET_HANDLE aNet );
    ARC* findRedundantArc( ARC* aSeg );

    ///< Scan the joint map, forming a line starting from segment (current).
    void followLine( LINKED_ITEM* aCurrent, bool aScanDirection, int& aPos, int aLimit,
                     VECTOR2I* aCorners, LINKED_ITEM** aSegments, bool* aArcReversed,
                     bool& aGuardHit, bool aStopAtLockedJoints, bool aFollowLockedSegments );

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

    std::vector< std::unique_ptr<SHAPE> > m_edgeExclusions;

    std::unordered_set<ITEM*> m_garbageItems;
};

}

#endif
