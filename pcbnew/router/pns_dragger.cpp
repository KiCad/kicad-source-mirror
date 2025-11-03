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

#include "pns_arc.h"

#include "pns_dragger.h"
#include "pns_shove.h"
#include "pns_router.h"
#include "pns_debug_decorator.h"
#include "pns_walkaround.h"

namespace PNS {

DRAGGER::DRAGGER( ROUTER* aRouter ) :
    DRAG_ALGO( aRouter ),
    m_initialVia( {} ),
    m_draggedVia( {} )
{
    m_world = nullptr;
    m_lastNode = nullptr;
    m_mode = DM_SEGMENT;
    m_draggedSegmentIndex = 0;
    m_dragStatus = false;
    m_currentMode = RM_MarkObstacles;
    m_freeAngleMode = false;
    m_forceMarkObstaclesMode = false;
}


DRAGGER::~DRAGGER()
{
}


bool DRAGGER::propagateViaForces( NODE* node, std::set<VIA*>& vias )
{
    VIA* via = *vias.begin();

    VECTOR2I force;
    VECTOR2I lead = -m_mouseTrailTracer.GetTrailLeadVector();

    const int iterLimit = Settings().ViaForcePropIterationLimit();

    if( via->PushoutForce( node, lead, force, ITEM::ANY_T, iterLimit ) )
    {
        via->SetPos( via->Pos() + force );
        return true;
    }

    return false;
}


VVIA* DRAGGER::checkVirtualVia( const VECTOR2D& aP, SEGMENT* aSeg )
{
    int w2 = aSeg->Width() / 2;

    auto distA = ( aP - aSeg->Seg().A ).EuclideanNorm();
    auto distB = ( aP - aSeg->Seg().B ).EuclideanNorm();

    VECTOR2I psnap;

    if( distA <= w2 )
    {
        psnap = aSeg->Seg().A;
    }
    else if( distB <= w2 )
    {
        psnap = aSeg->Seg().B;
    }
    else
    {
        return nullptr;
    }

    const JOINT *jt = m_world->FindJoint( psnap, aSeg );

    if ( !jt )
        return nullptr;

    for( ITEM* item : jt->LinkList() )
    {
        if( item->IsVirtual() && item->OfKind( ITEM::VIA_T ))
            return static_cast<VVIA*>( item );
    }

    return nullptr;
}


bool DRAGGER::startDragSegment( const VECTOR2D& aP, SEGMENT* aSeg )
{
    int w2 = aSeg->Width() / 2;

    m_draggedLine      = m_world->AssembleLine( aSeg, &m_draggedSegmentIndex );
    m_lastDragSolution = m_draggedLine;

    auto distA = ( aP - aSeg->Seg().A ).EuclideanNorm();
    auto distB = ( aP - aSeg->Seg().B ).EuclideanNorm();

    if( distA < w2 || distB < w2 )
    {
        m_mode = DM_CORNER;

        if( distB <= distA )
            m_draggedSegmentIndex++;
    }
    else if( m_freeAngleMode )
    {
        if( distB < distA &&
            ( m_draggedSegmentIndex < m_draggedLine.PointCount() - 2 ) &&
            ( !m_draggedLine.CLine().IsPtOnArc( static_cast<size_t>(m_draggedSegmentIndex) + 1 ) ) )
        {
            m_draggedSegmentIndex++;
        }

        m_mode = DM_CORNER;
    }
    else
    {
        m_mode = DM_SEGMENT;
    }

    return true;
}


bool DRAGGER::startDragArc( const VECTOR2D& aP, ARC* aArc )
{
    m_draggedLine = m_world->AssembleLine( aArc, &m_draggedSegmentIndex );
    m_mode = DM_ARC;

    return true;
}


bool DRAGGER::startDragVia( VIA* aVia )
{
    m_initialVia = aVia->MakeHandle();
    m_draggedVia = m_initialVia;

    m_mode = DM_VIA;

    return true;
}

const ITEM_SET DRAGGER::findViaFanoutByHandle ( NODE *aNode, const VIA_HANDLE& handle )
{
    ITEM_SET rv;

    const JOINT* jt = aNode->FindJoint( handle.pos, handle.layers.Start(), handle.net );

    if( !jt )
        return rv;

    bool foundVia = false;

    for( ITEM* item : jt->LinkList() )
    {
        if( item->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
        {
            int segIndex;
            LINKED_ITEM* seg = ( LINKED_ITEM*) item;
            LINE l = aNode->AssembleLine( seg, &segIndex );

            if( segIndex != 0 )
                l.Reverse();

            rv.Add( l );
        }
        else if( item->OfKind( ITEM::VIA_T ) )
        {
            if( !foundVia )
            {
                rv.Add( item );
                foundVia = true;
            }
        }
    }

    return rv;
}

bool DRAGGER::Start( const VECTOR2I& aP, ITEM_SET& aPrimitives )
{
    if( aPrimitives.Empty() )
        return false;

    ITEM* startItem = aPrimitives[0];

    m_lastNode = nullptr;
    m_draggedItems.Clear();
    m_currentMode = Settings().Mode();
    m_freeAngleMode = (m_mode & DM_FREE_ANGLE);
    m_forceMarkObstaclesMode = false;
    m_lastValidPoint = aP;

    m_mouseTrailTracer.Clear();
    m_mouseTrailTracer.AddTrailPoint( aP );

    m_preDragNode = m_world->Branch();

    if( m_currentMode == RM_Shove && !m_freeAngleMode )
    {
        m_shove = std::make_unique<SHOVE>( m_preDragNode, Router() );
        m_shove->SetLogger( Logger() );
        m_shove->SetDebugDecorator( Dbg() );
        m_shove->SetDefaultShovePolicy( SHOVE::SHP_SHOVE );
    }

    startItem->Unmark( MK_LOCKED );

    PNS_DBG( Dbg(), Message, wxString::Format( "StartDragging: item %p [kind %d]",
                                               startItem, (int) startItem->Kind() ) );

    switch( startItem->Kind() )
    {
    case ITEM::SEGMENT_T:
    {
        SEGMENT* seg = static_cast<SEGMENT*>( startItem );
        VVIA* vvia = checkVirtualVia( aP, seg );

        if( vvia )
            return startDragVia( vvia );
        else
            return startDragSegment( aP, seg );
    }
    case ITEM::VIA_T:
        return startDragVia( static_cast<VIA*>( startItem ) );

    case ITEM::ARC_T:
        return startDragArc( aP, static_cast<ARC*>( startItem ) );

    default:
        return false;
    }
}


void DRAGGER::SetMode( PNS::DRAG_MODE aMode )
{
    m_mode = static_cast<int>( aMode );
}


PNS::DRAG_MODE DRAGGER::Mode() const
{
    return static_cast<PNS::DRAG_MODE>( m_mode );
}


const std::vector<NET_HANDLE> DRAGGER::CurrentNets() const
{
    if( m_mode == PNS::DM_VIA )
        return std::vector<NET_HANDLE>( 1, m_draggedVia.net );
    else
        return std::vector<NET_HANDLE>( 1, m_draggedLine.Net() );
}


bool DRAGGER::dragMarkObstacles( const VECTOR2I& aP )
{
    // fixme: rewrite using shared_ptr...
    if( m_lastNode )
    {
        delete m_lastNode;
        m_lastNode = nullptr;
    }

    m_lastNode = m_world->Branch();

    switch( m_mode )
    {
    case DM_SEGMENT:
    case DM_CORNER:
    {
        //TODO: Make threshold configurable
        int  thresh = Settings().SmoothDraggedSegments() ? m_draggedLine.Width() / 4 : 0;
        LINE origLine( m_draggedLine );
        LINE dragged( m_draggedLine );
        dragged.SetSnapThreshhold( thresh );
        dragged.ClearLinks();

        if( m_mode == DM_SEGMENT )
            dragged.DragSegment( aP, m_draggedSegmentIndex );
        else
            dragged.DragCorner( aP, m_draggedSegmentIndex, m_freeAngleMode );

        m_lastNode->Remove( origLine );
        m_lastNode->Add( dragged );

        m_draggedItems.Clear();
        m_draggedItems.Add( dragged );

        break;
    }

    case DM_VIA: // fixme...
        dragViaMarkObstacles( m_initialVia, m_lastNode, aP );

        break;
    }

    if( Settings().AllowDRCViolations() )
        m_dragStatus = true;
    else
        m_dragStatus = !m_lastNode->CheckColliding( m_draggedItems );

    return true;
}


bool DRAGGER::dragViaMarkObstacles( const VIA_HANDLE& aHandle, NODE* aNode, const VECTOR2I& aP )
{
    m_draggedItems.Clear();

    ITEM_SET fanout = findViaFanoutByHandle( aNode, aHandle );

    if( fanout.Empty() )
        return true;

    for( ITEM* item : fanout.Items() )
    {
        if( const LINE* l = dyn_cast<const LINE*>( item ) )
        {
            LINE origLine( *l );
            LINE draggedLine( *l );

            draggedLine.DragCorner( aP, origLine.CLine().Find( aHandle.pos ), m_freeAngleMode );
            draggedLine.ClearLinks();

            m_draggedItems.Add( draggedLine );

            m_lastNode->Remove( origLine );
            m_lastNode->Add( draggedLine );
        }
        else if ( VIA *via = dyn_cast<VIA*>( item ) )
        {
            auto nvia = Clone( *via );

            nvia->SetPos( aP );
            m_draggedItems.Add( nvia.get() );

            m_lastNode->Remove( via );
            m_lastNode->Add( std::move( nvia ) );
        }
    }

    return true;
}


bool DRAGGER::dragViaWalkaround( const VIA_HANDLE& aHandle, NODE* aNode, const VECTOR2I& aP )
{
    m_draggedItems.Clear();

    ITEM_SET fanout = findViaFanoutByHandle( aNode, aHandle );

    if( fanout.Empty() )
        return true;

    bool viaPropOk = false;
    VECTOR2I viaTargetPos;

    for( ITEM* item : fanout.Items() )
    {
        if( VIA *via = dyn_cast<VIA*>( item ) )
        {
            std::unique_ptr<VIA> draggedVia = Clone( *via );

            draggedVia->SetPos( aP );
            m_draggedItems.Add( draggedVia.get() );

            std::set<VIA*> vias;

            vias.insert( draggedVia.get() );

            m_lastNode->Remove( via );

            bool ok = propagateViaForces( m_lastNode, vias );

            if( ok )
            {
                viaTargetPos = draggedVia->Pos();
                viaPropOk = true;
                m_lastNode->Add( std::move(draggedVia) );
            }
        }
    }

    if( !viaPropOk ) // can't force-propagate the via? bummer...
        return false;

    for( ITEM* item : fanout.Items() )
    {
        if( const LINE* l = dyn_cast<const LINE*>( item ) )
        {
            LINE origLine( *l );
            LINE draggedLine( *l );
            LINE walkLine( *l );

            draggedLine.DragCorner( viaTargetPos, origLine.CLine().Find( aHandle.pos ),
                                    m_freeAngleMode );
            draggedLine.ClearLinks();

            if ( m_world->CheckColliding( &draggedLine ) )
            {
                bool ok = tryWalkaround( m_lastNode, draggedLine, walkLine );

                if( !ok )
                    return false;

                m_lastNode->Remove( origLine );
                optimizeAndUpdateDraggedLine( walkLine, origLine, aP );
            }
            else
            {
                m_draggedItems.Add( draggedLine );

                m_lastNode->Remove( origLine );
                m_lastNode->Add( draggedLine );
            }
        }
    }

    return true;
}


void DRAGGER::optimizeAndUpdateDraggedLine( LINE& aDragged, const LINE& aOrig, const VECTOR2I& aP )
{
    LINE draggedPostOpt, origLine( aOrig );

    aDragged.ClearLinks();
    aDragged.Unmark();

    if( Settings().GetOptimizeEntireDraggedTrack() )
    {
        OPTIMIZER optimizer( m_lastNode );

        int effort =
                OPTIMIZER::MERGE_SEGMENTS | OPTIMIZER::KEEP_TOPOLOGY | OPTIMIZER::RESTRICT_AREA;

        if( Settings().SmoothDraggedSegments() )
            effort |= OPTIMIZER::MERGE_COLINEAR;

        optimizer.SetEffortLevel( effort );

        OPT_BOX2I affectedArea = aDragged.ChangedArea( &aOrig );
        VECTOR2I  anchor( aP );

        if( aDragged.CLine().Find( aP ) < 0 )
            anchor = aDragged.CLine().NearestPoint( aP );

        optimizer.SetPreserveVertex( anchor );

        if( !affectedArea )
            affectedArea = BOX2I( aP ); // No valid area yet? set to minimum to disable optimization

        PNS_DBG( Dbg(), AddPoint, anchor, YELLOW, 100000, wxT( "drag-anchor" ) );
        PNS_DBG( Dbg(), AddShape, *affectedArea, RED, 0, wxT( "drag-affected-area" ) );

        optimizer.SetRestrictArea( *affectedArea );

        PNS_DBG( Dbg(), AddItem, aDragged.Clone(), RED, 0, wxT( "drag-preopt" ) );
        aDragged.Line().Split( anchor );

        optimizer.Optimize( &aDragged, &draggedPostOpt, &origLine );
        PNS_DBG( Dbg(), AddItem, aDragged.Clone(), GREEN, 0, wxT( "drag-postopt" ) );
    }
    else
    {
        draggedPostOpt = aDragged;
    }

    m_lastNode->Add( draggedPostOpt );
    m_draggedItems.Clear();
    m_draggedItems.Add( draggedPostOpt );
}


bool DRAGGER::tryWalkaround( NODE* aNode, LINE& aOrig, LINE& aWalk )
{
        WALKAROUND walkaround( aNode, Router() );
    bool       ok = false;
    walkaround.SetSolidsOnly( false );
    walkaround.SetDebugDecorator( Dbg() );
    walkaround.SetLogger( Logger() );
    walkaround.SetIterationLimit( Settings().WalkaroundIterationLimit() );
    walkaround.SetLengthLimit( true, 30.0 );
    walkaround.SetAllowedPolicies( { WALKAROUND::WP_SHORTEST } );

    aWalk = aOrig;

    WALKAROUND::RESULT wr = walkaround.Route( aWalk );

    if( wr.status[ WALKAROUND::WP_SHORTEST ] == WALKAROUND::ST_DONE )
    {
        aWalk = wr.lines[ WALKAROUND::WP_SHORTEST ];
         return true;
    }

    return false;
}


bool DRAGGER::dragWalkaround( const VECTOR2I& aP )
{
    bool ok = false;

    // fixme: rewrite using shared_ptr...
    if( m_lastNode )
    {
        delete m_lastNode;
        m_lastNode = nullptr;
    }

    m_lastNode = m_world->Branch();

    switch( m_mode )
    {
    case DM_SEGMENT:
    case DM_CORNER:
    {
        int thresh = Settings().SmoothDraggedSegments() ? m_draggedLine.Width() / 4 : 0;
        LINE dragged( m_draggedLine );
        LINE draggedWalk( m_draggedLine );
        LINE origLine( m_draggedLine );

        dragged.SetSnapThreshhold( thresh );

        if( m_mode == DM_SEGMENT )
            dragged.DragSegment( aP, m_draggedSegmentIndex );
        else
            dragged.DragCorner( aP, m_draggedSegmentIndex );

        if ( m_world->CheckColliding( &dragged ) )
        {
            ok = tryWalkaround( m_lastNode, dragged, draggedWalk );
        }
        else
        {
            draggedWalk = dragged;
            ok = true;
        }

        if( draggedWalk.CLine().PointCount() < 2 )
            ok = false;

        if( ok )
        {
            PNS_DBG( Dbg(), AddShape, &origLine.CLine(), BLUE, 50000, wxT( "drag-orig-line" ) );
            PNS_DBG( Dbg(), AddShape, &draggedWalk.CLine(), CYAN, 75000, wxT( "drag-walk" ) );
            m_lastNode->Remove( origLine );
            optimizeAndUpdateDraggedLine( draggedWalk, origLine, aP );
        }

        break;
    }
    case DM_VIA: // fixme...
        ok = dragViaWalkaround( m_initialVia, m_lastNode, aP );
        break;
    }

    m_dragStatus = ok;

    return ok;
}


bool DRAGGER::dragShove( const VECTOR2I& aP )
{

    if( m_lastNode )
    {
        delete m_lastNode;
        m_lastNode = nullptr;
    }

    switch( m_mode )
    {
    case DM_SEGMENT:
    case DM_CORNER:
    {
        bool ok = false;
        //TODO: Make threshold configurable
        int  thresh = Settings().SmoothDraggedSegments() ? m_draggedLine.Width() / 2 : 0;
        LINE draggedPreShove( m_draggedLine );
        draggedPreShove.SetSnapThreshhold( thresh );

        if( m_mode == DM_SEGMENT )
            draggedPreShove.DragSegment( aP, m_draggedSegmentIndex );
        else
            draggedPreShove.DragCorner( aP, m_draggedSegmentIndex );

        auto preShoveNode = m_shove->CurrentNode();

        if( preShoveNode )
            preShoveNode->Remove( draggedPreShove );

        m_shove->ClearHeads();
        m_shove->AddHeads( draggedPreShove, SHOVE::SHP_SHOVE | SHOVE::SHP_DONT_LOCK_ENDPOINTS );
        ok = m_shove->Run() == SHOVE::SH_OK;

        LINE draggedPostShove( draggedPreShove );

        if( ok )
        {
            if( m_shove->HeadsModified() )
                draggedPostShove = m_shove->GetModifiedHead( 0 );
        }

        m_lastNode = m_shove->CurrentNode()->Branch();

        if( ok )
        {
            draggedPostShove.ClearLinks();
            draggedPostShove.Unmark();
            optimizeAndUpdateDraggedLine( draggedPostShove, m_draggedLine, aP );
            m_lastDragSolution = std::move( draggedPostShove );
        }

        m_dragStatus = ok;
        break;
    }

    case DM_VIA:
    {
        VIA_HANDLE newVia;

        // corner count limiter intended to avoid excessive optimization produces mediocre results for via shoving.
        // this is a hack that disables it, before I figure out a more reliable solution
        m_shove->DisablePostShoveOptimizations( OPTIMIZER::LIMIT_CORNER_COUNT );

        m_shove->ClearHeads();
        m_shove->AddHeads( m_draggedVia, aP, SHOVE::SHP_SHOVE );

        SHOVE::SHOVE_STATUS st = m_shove->Run(); //ShoveDraggingVia( m_draggedVia, aP, newVia );

            PNS_DBG( Dbg(), Message, wxString::Format("head-mod %d",
                m_shove->HeadsModified() ? 1:  0 ) );

            if( m_shove->HeadsModified() )
            {
                newVia = m_shove->GetModifiedHeadVia( 0 );

                PNS_DBG( Dbg(), Message, wxString::Format("newvia %d %d %d %d",
                        newVia.pos.x,
                        newVia.pos.y,
                        newVia.layers.Start(),
                        newVia.layers.End()
                    ) );

                m_draggedVia = newVia;
            }


        m_lastNode = m_shove->CurrentNode()->Branch();

        m_draggedItems.Clear();

         // If drag didn't work (i.e. dragged onto a collision) try walkaround instead
        if( st != SHOVE::SH_OK )
            m_dragStatus = dragViaWalkaround( m_draggedVia, m_lastNode, aP );
        else
            m_dragStatus = true;

        break;
    }
    }

    return m_dragStatus;
}


bool DRAGGER::FixRoute( bool aForceCommit )
{
    NODE* node = CurrentNode();

    if( node )
    {
        if( m_dragStatus )
        {
            Router()->CommitRouting( node );
            return true;
        }
        else if( m_forceMarkObstaclesMode )
        {
            if( aForceCommit )
            {
                Router()->CommitRouting( node );
                return true;
            }

            return false;
        }
        else
        {
            // If collisions exist, we can fix in shove/smart mode because all tracks to be
            // committed will be in valid positions (even if the current routing solution to
            // the mouse cursor is invalid).
            Drag( m_lastValidPoint );
            node = CurrentNode();

            if( node && m_dragStatus )
            {
                Router()->CommitRouting( node );
                return true;
            }
        }
    }

    return false;
}


bool DRAGGER::Drag( const VECTOR2I& aP )
{
    m_mouseTrailTracer.AddTrailPoint( aP );

    bool firstDrag = m_lastNode == nullptr;
    bool ret = false;

    if( m_freeAngleMode || m_forceMarkObstaclesMode )
    {
        ret = dragMarkObstacles( aP );
    }
    else
    {
        switch( m_currentMode )
        {
        case RM_MarkObstacles:   ret = dragMarkObstacles( aP );   break;
        case RM_Shove:           ret = dragShove( aP );           break;
        case RM_Walkaround:      ret = dragWalkaround( aP );      break;
        default:                                                  break;
        }
    }

    if( ret )
    {
        m_lastValidPoint = aP;
    }
    else
    {
        if( firstDrag )
        {
            // First collision resolution failed, switch to highlight mode
            m_forceMarkObstaclesMode = true;

            ret = dragMarkObstacles( aP );

            if( ret )
                m_lastValidPoint = aP;
        }
        else if( m_lastNode )
        {
            // Restore last solution
            NODE* parent = m_lastNode->GetParent()->Branch();
            delete m_lastNode;
            m_lastNode = parent;
            m_draggedItems.Clear();
            m_lastDragSolution.ClearLinks();
            m_lastNode->Add( m_lastDragSolution );
        }
    }

    return ret;
}


NODE* DRAGGER::CurrentNode() const
{
   return m_lastNode ? m_lastNode : m_world;
}


const ITEM_SET DRAGGER::Traces()
{
    return m_draggedItems;
}

}
