/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __PNS_BLOCK_DRAGGER_H
#define __PNS_BLOCK_DRAGGER_H

#include <set>
#include <unordered_set>
#include <vector>

#include <math/vector2d.h>

#include "pns_drag_algo.h"

namespace PNS
{
class ROUTER;

/**
 * BLOCK_DRAGGER
 *
 * Drags a mixed selection of pads, vias, segments and arcs as one block.
 *
 * Start() sorts everything into four groups:
 *  - rigid: selected pads and vias, displaced as they are
 *  - bridge: copper whose both ends travel with the block, displaced as it is
 *  - connection: copper with one end on the block and the other held outside, rerouted
 *  - anchored: selected arcs held at both ends, which cannot move
 *
 * A joint is held when an unselected pad, via or locked item sits on it. Unselected
 * copper at a block joint is followed to its far end and sorted the same way.
 *
 * Drag() rebuilds a fresh branch each call. A block marks collisions, it never shoves.
 */
class BLOCK_DRAGGER : public DRAG_ALGO
{
public:
    BLOCK_DRAGGER( ROUTER* aRouter );
    ~BLOCK_DRAGGER();

    /**
     * Sort the selection and the unselected copper touching it.
     *
     * @param aP is the point the drag starts from.
     * @param aPrimitives is the selection: pads, vias, segments and arcs.
     * @return true if there is anything to drag or to report.
     */
    bool Start( const VECTOR2I& aP, ITEM_SET& aPrimitives ) override;

    /**
     * Rebuild the block at aP: displace rigid and bridge items, reroute connections.
     *
     * @return true always. Collisions are reported through GetForceMarkObstaclesMode().
     */
    bool Drag( const VECTOR2I& aP ) override;

    /**
     * Commit the current branch if it is collision free or aForceCommit is set.
     *
     * @return true if committed.
     */
    bool FixRoute( bool aForceCommit ) override;

    /// The current branch, or the world before the first Drag().
    NODE* CurrentNode() const override;

    /// Unused for block dragging.
    const std::vector<NET_HANDLE> CurrentNets() const override { return std::vector<NET_HANDLE>(); }

    /// Unused for block dragging.
    virtual int CurrentLayer() const override { return UNDEFINED_LAYER; }

    /// Everything displaced or rerouted by the last Drag().
    const ITEM_SET Traces() override;

    /// Selected arcs held at both ends, which cannot move.
    const std::vector<LINE>& AnchoredLines() const { return m_anchoredLines; }

    void SetMode( PNS::DRAG_MODE aDragMode ) override { m_mode = aDragMode; }

    virtual PNS::DRAG_MODE Mode() const override { return m_mode; }

    bool GetForceMarkObstaclesMode( bool* aDragStatus ) const override
    {
        *aDragStatus = m_dragStatus;
        return true;
    }

private:
    struct DRAGGED_CONNECTION
    {
        LINE     origLine;
        VECTOR2I p_orig;           ///< The corner that follows the block, or a body point
        bool     dragBody = false; ///< Both ends held, the body moves and the ends reroute
    };

    /// Unselected and locked, so it neither moves nor is rerouted.
    bool isFixedByLock( ITEM* aItem ) const;

    /// Held in place by something outside the block. Links of aOwnLine do not count.
    bool isPinned( const JOINT* aJoint, const LINE& aOwnLine ) const;

    /// A selected pad or via sits on this joint.
    bool isHeldByBlock( const JOINT* aJoint ) const;

    /// A pad covers more than its anchor, so ask what really sits on the joint. aSelected
    /// picks between a solid the block carries and one it does not.
    bool coveredBySolid( const JOINT* aJoint, bool aSelected ) const;

    /// Copper at this joint is already sorted, so a walk stops here.
    bool isBlockBoundary( const JOINT* aJoint ) const;

    /// A one link line, so no unselected copper is assembled into a selected segment.
    static LINE wrapLink( LINKED_ITEM* aLink );

    /// Sort a selected segment or arc by what holds its ends.
    void classifySelectedLink( LINKED_ITEM* aLink );

    /// Cut an assembled line back to the stretch between aEntry and the first block joint.
    void clipToBlock( LINE& aLine, const VECTOR2I& aEntry, LINKED_ITEM* aLink ) const;

    /// Sort unselected copper that touches the block at aJoint.
    void classifyNeighbour( const JOINT* aJoint, LINKED_ITEM* aLink );

    /// Sort every unselected segment and arc on a joint.
    void scanJoint( const JOINT* aJoint );

    /// Sort the copper touching a selected pad or via.
    void scanRigidItem( ITEM* aItem );

    /// Replace aItem in the current branch with a copy moved by aDelta.
    void displaceItem( ITEM* aItem, const VECTOR2I& aDelta );

    std::set<ITEM*>                 m_rigidItems;    ///< Selected pads and vias
    std::set<ITEM*>                 m_bridgeItems;   ///< Copper displaced whole with the block
    std::vector<DRAGGED_CONNECTION> m_conns;         ///< Copper rerouted to follow the block
    std::vector<LINE>               m_anchoredLines; ///< Selected arcs that cannot move

    // Working state of Start()
    std::unordered_set<ITEM*>        m_selectedLinks; ///< Selected segments and arcs
    std::unordered_set<LINKED_ITEM*> m_seenLinks;     ///< Already sorted
    std::set<const JOINT*>           m_carriedJoints; ///< Ends of selected copper that travel

    bool           m_dragStatus;
    ITEM_SET       m_draggedItems;
    NODE*          m_currentNode;
    VECTOR2I       m_p0;
    PNS::DRAG_MODE m_mode;
};

}; // namespace PNS

#endif
