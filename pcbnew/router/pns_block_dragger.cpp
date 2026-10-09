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

#include <memory>

#include "pns_arc.h"
#include "pns_joint.h"
#include "pns_line.h"
#include "pns_node.h"
#include "pns_segment.h"
#include "pns_solid.h"
#include "pns_via.h"
#include "pns_router.h"

#include "pns_block_dragger.h"
#include "pns_debug_decorator.h"

namespace PNS
{

BLOCK_DRAGGER::BLOCK_DRAGGER( ROUTER* aRouter ) :
        DRAG_ALGO( aRouter ),
        m_dragStatus( false ),
        m_currentNode( nullptr ),
        m_mode( DM_BLOCK )
{
}


BLOCK_DRAGGER::~BLOCK_DRAGGER()
{
}


bool BLOCK_DRAGGER::Start( const VECTOR2I& aP, ITEM_SET& aPrimitives )
{
    assert( m_world );

    m_currentNode = nullptr;
    m_p0 = aP;
    m_rigidItems.clear();
    m_bridgeItems.clear();
    m_conns.clear();
    m_anchoredLines.clear();
    m_selectedLinks.clear();
    m_seenLinks.clear();
    m_carriedJoints.clear();

    for( ITEM* item : aPrimitives.Items() )
    {
        if( item->OfKind( ITEM::SOLID_T | ITEM::VIA_T ) )
            m_rigidItems.insert( item );
        else if( item->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
            m_selectedLinks.insert( item );
    }

    for( ITEM* item : aPrimitives.Items() )
    {
        if( m_selectedLinks.count( item ) )
            classifySelectedLink( static_cast<LINKED_ITEM*>( item ) );
    }

    for( ITEM* item : aPrimitives.Items() )
    {
        if( m_rigidItems.count( item ) )
            scanRigidItem( item );
    }

    for( const JOINT* joint : m_carriedJoints )
        scanJoint( joint );

    return !m_rigidItems.empty() || !m_bridgeItems.empty() || !m_conns.empty() || !m_anchoredLines.empty();
}


bool BLOCK_DRAGGER::isFixedByLock( ITEM* aItem ) const
{
    // Locked copper the user selected still moves
    return aItem->IsLocked() && !m_selectedLinks.count( aItem ) && !m_rigidItems.count( aItem );
}


bool BLOCK_DRAGGER::isPinned( const JOINT* aJoint, const LINE& aOwnLine ) const
{
    for( ITEM* link : aJoint->LinkList() )
    {
        // Virtual vias only mark width changes, T junctions and locked ends
        if( link->IsVirtual() || m_rigidItems.count( link ) )
            continue;

        if( link->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T )
            && aOwnLine.ContainsLink( static_cast<LINKED_ITEM*>( link ) ) )
        {
            continue;
        }

        if( link->OfKind( ITEM::VIA_T | ITEM::SOLID_T ) || isFixedByLock( link ) )
            return true;
    }

    return coveredBySolid( aJoint, false );
}


bool BLOCK_DRAGGER::coveredBySolid( const JOINT* aJoint, bool aSelected ) const
{
    const ITEM_SET hits = m_world->HitTest( aJoint->Pos() );

    for( const ITEM* item : hits.CItems() )
    {
        if( !item->OfKind( ITEM::SOLID_T | ITEM::VIA_T ) )
            continue;

        if( ( m_rigidItems.count( const_cast<ITEM*>( item ) ) != 0 ) != aSelected )
            continue;

        if( item->Net() == aJoint->Net() && item->Layers().Overlaps( aJoint->Layers() ) )
            return true;
    }

    return false;
}


bool BLOCK_DRAGGER::isHeldByBlock( const JOINT* aJoint ) const
{
    for( ITEM* link : aJoint->LinkList() )
    {
        if( !link->IsVirtual() && m_rigidItems.count( link ) )
            return true;
    }

    return coveredBySolid( aJoint, true );
}


bool BLOCK_DRAGGER::isBlockBoundary( const JOINT* aJoint ) const
{
    if( !aJoint )
        return false;

    if( m_carriedJoints.count( aJoint ) )
        return true;

    for( ITEM* link : aJoint->LinkList() )
    {
        if( !link->IsVirtual() && ( m_selectedLinks.count( link ) || m_rigidItems.count( link ) ) )
            return true;
    }

    return false;
}


LINE BLOCK_DRAGGER::wrapLink( LINKED_ITEM* aLink )
{
    LINE line;
    line.SetWidth( aLink->Width() );
    line.SetLayers( aLink->Layers() );
    line.SetNet( aLink->Net() );
    line.SetSourceItem( aLink->GetSourceItem() );

    if( aLink->Kind() == ITEM::ARC_T )
    {
        SHAPE_LINE_CHAIN chain;
        chain.Append( static_cast<ARC*>( aLink )->Arc() );
        line.SetShape( chain );
    }
    else
    {
        line.SetShape( SHAPE_LINE_CHAIN( { aLink->Anchor( 0 ), aLink->Anchor( 1 ) } ) );
    }

    line.Link( aLink );
    return line;
}


void BLOCK_DRAGGER::classifySelectedLink( LINKED_ITEM* aLink )
{
    if( m_seenLinks.count( aLink ) )
        return;

    m_seenLinks.insert( aLink );

    // Wrapped on its own, so no unselected copper is assembled into it
    LINE line = wrapLink( aLink );

    const VECTOR2I& start = line.CLine().CPoint( 0 );
    const VECTOR2I& end = line.CLine().CLastPoint();

    const JOINT* jointA = m_world->FindJoint( start, aLink->Layer(), aLink->Net() );
    const JOINT* jointB = m_world->FindJoint( end, aLink->Layer(), aLink->Net() );

    bool carriedA = !jointA || !isPinned( jointA, line );
    bool carriedB = !jointB || !isPinned( jointB, line );

    if( carriedA && jointA )
        m_carriedJoints.insert( jointA );

    if( carriedB && jointB )
        m_carriedJoints.insert( jointB );

    if( carriedA && carriedB )
        m_bridgeItems.insert( aLink );
    else if( carriedA || carriedB )
        m_conns.push_back( { line, carriedA ? start : end } );
    else if( aLink->Kind() == ITEM::ARC_T )
        m_anchoredLines.push_back( line ); // an arc has no body drag
    else
        m_conns.push_back( { line, ( start + end ) / 2, true } );
}


void BLOCK_DRAGGER::clipToBlock( LINE& aLine, const VECTOR2I& aEntry, LINKED_ITEM* aLink ) const
{
    const SHAPE_LINE_CHAIN& chain = aLine.CLine();
    const int               last = chain.PointCount() - 1;
    const int               entry = chain.Find( aEntry );
    const VECTOR2I&         otherEnd = aLink->Anchor( 0 ) == aEntry ? aLink->Anchor( 1 ) : aLink->Anchor( 0 );
    const int               other = chain.Find( otherEnd );

    if( entry < 0 || other < 0 || other == entry || last < 1 )
    {
        wxFAIL;
        return;
    }

    // Keep the side of the entry that aLink is on, up to the first block joint
    int lo = 0;
    int hi = last;

    if( other > entry )
    {
        lo = entry;

        for( hi = entry + 1; hi < last; hi++ )
        {
            if( isBlockBoundary( m_world->FindJoint( chain.CPoint( hi ), &aLine ) ) )
                break;
        }
    }
    else
    {
        hi = entry;

        for( lo = entry - 1; lo > 0; lo-- )
        {
            if( isBlockBoundary( m_world->FindJoint( chain.CPoint( lo ), &aLine ) ) )
                break;
        }
    }

    if( lo == 0 && hi == last )
        return;

    // ClipVertexRange() keeps the right links only when the kept range starts at 0
    if( hi < last )
        aLine.ClipVertexRange( 0, hi );

    if( lo > 0 )
    {
        aLine.Reverse();
        aLine.ClipVertexRange( 0, hi - lo );
        aLine.Reverse();
    }
}


void BLOCK_DRAGGER::classifyNeighbour( const JOINT* aJoint, LINKED_ITEM* aLink )
{
    if( m_seenLinks.count( aLink ) || isFixedByLock( aLink ) )
        return;

    m_seenLinks.insert( aLink );

    LINE line = m_world->AssembleLine( aLink );
    clipToBlock( line, aJoint->Pos(), aLink );

    for( LINKED_ITEM* lineLink : line.Links() )
        m_seenLinks.insert( lineLink );

    const VECTOR2I& start = line.CLine().CPoint( 0 );
    const VECTOR2I& end = line.CLine().CLastPoint();
    const JOINT*    farJoint = m_world->FindJoint( start == aJoint->Pos() ? end : start, aLink );

    bool farCarried = false;

    if( farJoint )
        farCarried = m_carriedJoints.count( farJoint ) || ( isHeldByBlock( farJoint ) && !isPinned( farJoint, line ) );

    if( farCarried )
    {
        for( LINKED_ITEM* lineLink : line.Links() )
            m_bridgeItems.insert( lineLink );
    }
    else
    {
        m_conns.push_back( { line, aJoint->Pos() } );
    }
}


void BLOCK_DRAGGER::scanJoint( const JOINT* aJoint )
{
    if( !aJoint )
        return;

    for( ITEM* link : aJoint->LinkList() )
    {
        if( link->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
            classifyNeighbour( aJoint, static_cast<LINKED_ITEM*>( link ) );
    }
}


void BLOCK_DRAGGER::scanRigidItem( ITEM* aItem )
{
    if( aItem->Kind() == ITEM::VIA_T )
    {
        scanJoint( m_world->FindJoint( static_cast<VIA*>( aItem )->Pos(), aItem ) );
        return;
    }

    SOLID* solid = static_cast<SOLID*>( aItem );

    if( !solid->IsRoutable() )
        return;

    scanJoint( m_world->FindJoint( solid->Pos(), solid ) );

    // A track ending on the pad away from its centre follows too
    std::vector<JOINT*> joints;
    m_world->QueryJoints( solid->Hull().BBox(), joints, solid->Layers(), ITEM::SEGMENT_T | ITEM::ARC_T );

    for( JOINT* joint : joints )
    {
        if( joint->Net() != solid->Net() || joint->LinkCount() != 1 )
            continue;

        LINKED_ITEM* link = static_cast<LINKED_ITEM*>( joint->LinkList().front() );

        if( const SHAPE* padShape = solid->Shape( solid->Layer() ) )
        {
            if( padShape->Collide( joint->Pos() ) )
                classifyNeighbour( joint, link );
        }
    }
}


void BLOCK_DRAGGER::displaceItem( ITEM* aItem, const VECTOR2I& aDelta )
{
    switch( aItem->Kind() )
    {
    case ITEM::SOLID_T:
    {
        SOLID*                 solid = static_cast<SOLID*>( aItem );
        std::unique_ptr<SOLID> clone( static_cast<SOLID*>( solid->Clone() ) );

        clone->SetPos( solid->Pos() + aDelta );
        m_currentNode->Remove( aItem );
        m_draggedItems.Add( clone.get() );
        m_currentNode->Add( std::move( clone ) );
        break;
    }

    case ITEM::VIA_T:
    {
        VIA*                 via = static_cast<VIA*>( aItem );
        std::unique_ptr<VIA> clone( static_cast<VIA*>( via->Clone() ) );

        clone->SetPos( via->Pos() + aDelta );
        m_currentNode->Remove( aItem );
        m_draggedItems.Add( clone.get() );
        m_currentNode->Add( std::move( clone ) );
        break;
    }

    // Add() destroys a clone it refuses, so keep the pointer only once it is in. Redundant
    // copper is allowed, or one of two coincident selected tracks vanishes on commit.
    case ITEM::SEGMENT_T:
    {
        SEGMENT*                 seg = static_cast<SEGMENT*>( aItem );
        std::unique_ptr<SEGMENT> clone( seg->Clone() );
        SEGMENT*                 raw = clone.get();

        SEG orig = seg->Seg();
        clone->SetEnds( orig.A + aDelta, orig.B + aDelta );
        m_currentNode->Remove( aItem );

        if( m_currentNode->Add( std::move( clone ), true ) )
            m_draggedItems.Add( raw );

        break;
    }

    case ITEM::ARC_T:
    {
        ARC*                 arc = static_cast<ARC*>( aItem );
        std::unique_ptr<ARC> clone( arc->Clone() );
        ARC*                 raw = clone.get();

        clone->Arc().Move( aDelta );
        m_currentNode->Remove( aItem );

        if( m_currentNode->Add( std::move( clone ), true ) )
            m_draggedItems.Add( raw );

        break;
    }

    default: wxFAIL; break;
    }
}


bool BLOCK_DRAGGER::Drag( const VECTOR2I& aP )
{
    assert( m_world );

    m_world->KillChildren();
    m_currentNode = m_world->Branch();
    m_draggedItems.Clear();

    const VECTOR2I delta = aP - m_p0;

    for( ITEM* rigid : m_rigidItems )
        displaceItem( rigid, delta );

    for( ITEM* bridge : m_bridgeItems )
        displaceItem( bridge, delta );

    bool freeAngle = ( m_mode & DM_FREE_ANGLE ) != 0;

    for( DRAGGED_CONNECTION& cn : m_conns )
    {
        LINE l_new( cn.origLine );
        l_new.Unmark();
        l_new.ClearLinks();

        if( cn.dragBody )
            l_new.DragSegment( cn.p_orig + delta, 0, false );
        else
            l_new.DragCorner( cn.p_orig + delta, l_new.CLine().Find( cn.p_orig ), freeAngle );

        PNS_DBG( Dbg(), AddItem, &l_new, BLUE, 0, wxT( "bdrag-new-fanout" ) );
        m_draggedItems.Add( l_new );

        LINE l_orig( cn.origLine );
        m_currentNode->Remove( l_orig );

        // Redundant copper is allowed here for the same reason as in displaceItem()
        m_currentNode->Add( l_new, true );
    }

    m_dragStatus = !m_currentNode->CheckColliding( m_draggedItems );

    return true;
}


bool BLOCK_DRAGGER::FixRoute( bool aForceCommit )
{
    NODE* node = CurrentNode();

    if( node )
    {
        if( Settings().AllowDRCViolations() || aForceCommit || !node->CheckColliding( m_draggedItems ) )
        {
            Router()->CommitRouting( node );
            return true;
        }
    }

    return false;
}


NODE* BLOCK_DRAGGER::CurrentNode() const
{
    return m_currentNode ? m_currentNode : m_world;
}


const ITEM_SET BLOCK_DRAGGER::Traces()
{
    return m_draggedItems;
}

}; // namespace PNS
