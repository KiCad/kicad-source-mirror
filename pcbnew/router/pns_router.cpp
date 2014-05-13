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

#include <cstdio>
#include <vector>

#include <boost/foreach.hpp>

#include <view/view.h>
#include <view/view_item.h>
#include <view/view_group.h>
#include <gal/graphics_abstraction_layer.h>

#include <pcb_painter.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_circle.h>

#include "trace.h"
#include "pns_node.h"
#include "pns_line_placer.h"
#include "pns_line.h"
#include "pns_solid.h"
#include "pns_utils.h"
#include "pns_router.h"

#include <router/router_preview_item.h>

#include <class_board.h>
#include <class_board_item.h>
#include <class_module.h>
#include <class_track.h>
#include <ratsnest_data.h>
#include <layers_id_colors_and_visibility.h>

// an ugly singleton for drawing debug items within the router context.
// To be fixed sometime in the future.
static PNS_ROUTER* theRouter;

class PCBNEW_CLEARANCE_FUNC : public PNS_CLEARANCE_FUNC
{
public:
    PCBNEW_CLEARANCE_FUNC( BOARD* aBoard )
    {
        m_clearanceCache.resize( aBoard->GetNetCount() );

        for( unsigned int i = 0; i < aBoard->GetNetCount(); i++ )
        {
            NETINFO_ITEM* ni = aBoard->FindNet( i );
            if( ni == NULL )
                continue;

            wxString netClassName = ni->GetClassName();
            NETCLASS* nc = aBoard->m_NetClasses.Find( netClassName );
            int clearance = nc->GetClearance();
            m_clearanceCache[i] = clearance;
            TRACE( 1, "Add net %d netclass %s clearance %d", i % netClassName.mb_str() %
                    clearance );
        }

        m_defaultClearance = 254000;    // aBoard->m_NetClasses.Find ("Default clearance")->GetClearance();
    }

    int operator()( const PNS_ITEM* a, const PNS_ITEM* b )
    {
        int net_a = a->GetNet();
        int cl_a = (net_a >= 0 ? m_clearanceCache[net_a] : m_defaultClearance);
        int net_b = b->GetNet();
        int cl_b = (net_b >= 0 ? m_clearanceCache[net_b] : m_defaultClearance);

        return std::max( cl_a, cl_b );
    }

private:
    std::vector<int> m_clearanceCache;
    int m_defaultClearance;
};

PNS_ITEM* PNS_ROUTER::syncPad( D_PAD* aPad )
{
    PNS_LAYERSET layers( 0, 15 );

    switch( aPad->GetAttribute() )
    {
    case PAD_STANDARD:
        break;

    case PAD_SMD:
    case PAD_CONN:
    {
        LAYER_MSK lmsk = aPad->GetLayerMask();
        int i;

        for( i = FIRST_COPPER_LAYER; i <= LAST_COPPER_LAYER; i++ )
		{
            if( lmsk & (1 << i) )
            {
                layers = PNS_LAYERSET( i );
                break;
            }
		}
        break;
    }

    default:
        TRACE( 0, "unsupported pad type 0x%x", aPad->GetAttribute() );
        return NULL;
    }

    PNS_SOLID* solid = new PNS_SOLID;

    solid->SetLayers( layers );
    solid->SetNet( aPad->GetNetCode() );
    wxPoint wx_c = aPad->GetPosition();
    wxSize  wx_sz = aPad->GetSize();

    VECTOR2I c( wx_c.x, wx_c.y );
    VECTOR2I sz( wx_sz.x, wx_sz.y );

    solid->SetCenter( c );

    double orient = aPad->GetOrientation() / 10.0;

    if( orient == 90.0 || orient == 270.0 )
        sz = VECTOR2I( sz.y, sz.x );
    else if( orient != 0.0 && orient != 180.0 )
    {
        TRACEn( 0, "non-orthogonal pad rotations not supported yet" );
        delete solid;
        return NULL;
    }

    switch( aPad->GetShape() )
    {
    case PAD_CIRCLE:
        solid->SetShape( new SHAPE_CIRCLE( c, sz.x / 2 ) );
        break;

    case PAD_OVAL:
        if( sz.x == sz.y )
            solid->SetShape( new SHAPE_CIRCLE( c, sz.x / 2 ) );
        else
            solid->SetShape( new SHAPE_RECT( c - sz / 2, sz.x, sz.y ) );
        break;

    case PAD_RECT:
        solid->SetShape( new SHAPE_RECT( c - sz / 2, sz.x, sz.y ) );
        break;

    default:
        TRACEn( 0, "unsupported pad shape" );
        delete solid;
        return NULL;
    }

    solid->SetParent( aPad );
    return solid;
}


PNS_ITEM* PNS_ROUTER::syncTrack( TRACK* aTrack )
{
    PNS_SEGMENT* s =
        new PNS_SEGMENT( SEG( aTrack->GetStart(), aTrack->GetEnd() ), aTrack->GetNetCode() );

    s->SetWidth( aTrack->GetWidth() );
    s->SetLayers( PNS_LAYERSET( aTrack->GetLayer() ) );
    s->SetParent( aTrack );
    return s;
}


PNS_ITEM* PNS_ROUTER::syncVia( VIA* aVia )
{
    PNS_VIA* v = new PNS_VIA(
            aVia->GetPosition(),
            PNS_LAYERSET( 0, 15 ),
            aVia->GetWidth(),
            aVia->GetNetCode() );

    v->SetParent( aVia );
    return v;
}


void PNS_ROUTER::SetBoard( BOARD* aBoard )
{
    m_board = aBoard;
    TRACE( 1, "m_board = %p\n", m_board );
}


int PNS_ROUTER::NextCopperLayer( bool aUp )
{
    LAYER_MSK mask = m_board->GetEnabledLayers() & m_board->GetVisibleLayers();
    LAYER_NUM l = m_currentLayer;

    do {
        l += ( aUp ? 1 : -1 );

        if( l > LAST_COPPER_LAYER )
            l = FIRST_COPPER_LAYER;

        if( l < FIRST_COPPER_LAYER )
            l = LAST_COPPER_LAYER;

        if( mask & GetLayerMask( l ) )
            return l;
    } while( l != m_currentLayer );

    return l;
}


void PNS_ROUTER::SyncWorld()
{
    std::vector<D_PAD*> pads;

    if( !m_board )
    {
        TRACEn( 0, "No board attached, aborting sync." );
        return;
    }

    ClearWorld();

    m_clearanceFunc = new PCBNEW_CLEARANCE_FUNC( m_board );
    m_world = new PNS_NODE();
    m_world->SetClearanceFunctor( m_clearanceFunc );
    m_world->SetMaxClearance( 1000000 );    // m_board->GetBiggestClearanceValue());
    pads = m_board->GetPads();

    for( MODULE* module = m_board->m_Modules; module; module = module->Next() )
    {
        for( D_PAD* pad = module->Pads(); pad; pad = pad->Next() )
        {
            PNS_ITEM* solid = syncPad( pad );

            if( solid )
                m_world->Add( solid );
        }
    }

    for( TRACK* t = m_board->m_Track; t; t = t->Next() )
    {
        KICAD_T type = t->Type();
        PNS_ITEM* item = NULL;

        if( type == PCB_TRACE_T )
            item = syncTrack( t );
        else if( type == PCB_VIA_T )
            item = syncVia( static_cast<VIA*>( t ) );

        if( item )
            m_world->Add( item );
    }

    m_placer = new PNS_LINE_PLACER( m_world );
}


PNS_ROUTER::PNS_ROUTER()
{
    theRouter = this;

    m_clearanceFunc = NULL;

    m_currentLayer = 1;
    m_placingVia = false;
    m_currentNet = -1;
    m_state = IDLE;
    m_world = NULL;
    m_placer = NULL;
    m_previewItems = NULL;
    m_start_diagonal = false;
    m_board = NULL;

    TRACE( 1, "m_board = %p\n", m_board );
}


void PNS_ROUTER::SetView( KIGFX::VIEW* aView )
{
    if( m_previewItems )
    {
        m_previewItems->FreeItems();
        delete m_previewItems;
    }

    m_view = aView;
    m_previewItems = new KIGFX::VIEW_GROUP( m_view );
    m_previewItems->SetLayer( ITEM_GAL_LAYER( GP_OVERLAY ) );
    m_view->Add( m_previewItems );
    m_previewItems->ViewSetVisible( true );
}


PNS_ROUTER* PNS_ROUTER::GetInstance()
{
    return theRouter;
}


PNS_ROUTER::~PNS_ROUTER()
{
    ClearWorld();
    theRouter = NULL;

    if( m_previewItems )
        delete m_previewItems;
}


void PNS_ROUTER::ClearWorld()
{
    if( m_world )
        delete m_world;

    if( m_clearanceFunc )
        delete m_clearanceFunc;

    if( m_placer )
        delete m_placer;

    if( m_previewItems )
        delete m_previewItems;

    m_clearanceFunc = NULL;
    m_world = NULL;
    m_placer = NULL;
    m_previewItems = NULL;
}


void PNS_ROUTER::SetCurrentWidth( int w )
{
    // fixme: change width while routing
    m_currentWidth = w;
}


bool PNS_ROUTER::RoutingInProgress() const
{
    return m_state != IDLE;
}


const PNS_ITEMSET PNS_ROUTER::QueryHoverItems( const VECTOR2I& aP )
{
    if( m_state == IDLE )
        return m_world->HitTest( aP );
    else
        return m_placer->GetCurrentNode()->HitTest( aP );
}


const VECTOR2I PNS_ROUTER::SnapToItem( PNS_ITEM* item, VECTOR2I aP, bool& aSplitsSegment )
{
    VECTOR2I anchor;

    if( !item )
    {
        aSplitsSegment = false;
        return aP;
    }

    switch( item->GetKind() )
    {
    case PNS_ITEM::SOLID:
        anchor = static_cast<PNS_SOLID*>(item)->GetCenter();
        aSplitsSegment = false;
        break;

    case PNS_ITEM::VIA:
        anchor = static_cast<PNS_VIA*>(item)->GetPos();
        aSplitsSegment = false;
        break;

    case PNS_ITEM::SEGMENT:
        {
            PNS_SEGMENT* seg = static_cast<PNS_SEGMENT*>( item );
            const SEG& s = seg->GetSeg();
            int w = seg->GetWidth();

            aSplitsSegment = false;

            if( ( aP - s.A ).EuclideanNorm() < w / 2 )
                anchor = s.A;
            else if( ( aP - s.B ).EuclideanNorm() < w / 2 )
                anchor = s.B;
            else
            {
                anchor = s.NearestPoint( aP );
                aSplitsSegment = true;
            }

            break;
        }

    default:
        break;
    }

    return anchor;
}


void PNS_ROUTER::StartRouting( const VECTOR2I& aP, PNS_ITEM* aStartItem )
{
    VECTOR2I p;

    static int unknowNetIdx = 0;    // -10000;

    m_placingVia = false;
    m_startsOnVia = false;
    m_currentNet = -1;

    bool splitSeg = false;

    p = SnapToItem( aStartItem, aP, splitSeg );

    if( !aStartItem || aStartItem->GetNet() < 0 )
        m_currentNet = unknowNetIdx--;
    else
        m_currentNet = aStartItem->GetNet();

    m_currentStart  = p;
    m_originalStart = p;
    m_currentEnd = p;

    m_placer->SetInitialDirection( m_start_diagonal ? DIRECTION_45(
                    DIRECTION_45::NE ) : DIRECTION_45( DIRECTION_45::N ) );
    m_placer->StartPlacement( m_originalStart, m_currentNet, m_currentWidth, m_currentLayer );
    m_state = ROUTE_TRACK;

    if( splitSeg )
        splitAdjacentSegments( m_placer->GetCurrentNode(), aStartItem, p );
}


const VECTOR2I PNS_ROUTER::GetCurrentEnd() const
{
    return m_currentEnd;
}


void PNS_ROUTER::EraseView()
{
    BOOST_FOREACH( BOARD_ITEM* item, m_hiddenItems )
    {
        item->ViewSetVisible( true );
    }

    m_hiddenItems.clear();

    if( m_previewItems )
    {
        m_previewItems->FreeItems();
        m_previewItems->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
    }
}


void PNS_ROUTER::DisplayItem( const PNS_ITEM* aItem, bool aIsHead )
{
    ROUTER_PREVIEW_ITEM* pitem = new ROUTER_PREVIEW_ITEM( aItem, m_previewItems );

    m_previewItems->Add( pitem );

    if( aIsHead )
        pitem->MarkAsHead();

    pitem->ViewSetVisible( true );
    m_previewItems->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY | KIGFX::VIEW_ITEM::APPEARANCE );
}


void PNS_ROUTER::DisplayDebugLine( const SHAPE_LINE_CHAIN& aLine, int aType, int aWidth )
{
    ROUTER_PREVIEW_ITEM* pitem = new ROUTER_PREVIEW_ITEM( NULL, m_previewItems );

    pitem->DebugLine( aLine, aWidth, aType );
    m_previewItems->Add( pitem );
    pitem->ViewSetVisible( true );
    m_previewItems->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY | KIGFX::VIEW_ITEM::APPEARANCE );
}


void PNS_ROUTER::DisplayDebugBox( const BOX2I& aBox, int aType, int aWidth )
{
}


void PNS_ROUTER::Move( const VECTOR2I& aP, PNS_ITEM* endItem )
{
    PNS_NODE::ItemVector removed, added;
    VECTOR2I p = aP;

    if( m_state == IDLE )
        return;

    // TODO is something missing here?
    if( m_state == START_ROUTING )
    {
    }

    EraseView();

    m_currentEnd = p;
    m_placer->Route( p );

    PNS_LINE current = m_placer->GetTrace();

    DisplayItem( &current, true );

    if( current.EndsWithVia() )
        DisplayItem( &current.GetVia(), true );

    m_placer->GetCurrentNode()->GetUpdatedItems( removed, added );

    BOOST_FOREACH( PNS_ITEM* item, added )
	{
        DisplayItem( item );
    }

    BOOST_FOREACH( PNS_ITEM* item, removed )
    {
        BOARD_ITEM* parent = item->GetParent();

        if( parent )
        {
            if( parent->ViewIsVisible() )
                m_hiddenItems.insert( parent );

            parent->ViewSetVisible( false );
            parent->ViewUpdate( KIGFX::VIEW_ITEM::APPEARANCE );
        }
    }
}


void PNS_ROUTER::splitAdjacentSegments( PNS_NODE* aNode, PNS_ITEM* aSeg, const VECTOR2I& aP )
{
    if( aSeg && aSeg->OfKind( PNS_ITEM::SEGMENT ) )
    {
        PNS_NODE::OptJoint jt = aNode->FindJoint( aP, aSeg->GetLayers().Start(), aSeg->GetNet() );

        if( jt && jt->LinkCount() >= 1 )
            return;

        PNS_SEGMENT* s_old = static_cast<PNS_SEGMENT*>( aSeg );
        PNS_SEGMENT* s_new[2];

        s_new[0] = s_old->Clone();
        s_new[1] = s_old->Clone();

        s_new[0]->SetEnds( s_old->GetSeg().A, aP );
        s_new[1]->SetEnds( aP, s_old->GetSeg().B );

        aNode->Remove( s_old );
        aNode->Add( s_new[0] );
        aNode->Add( s_new[1] );
    }
}


void PNS_ROUTER::commitRouting( PNS_NODE* aNode )
{
    PNS_NODE::ItemVector removed, added;

    aNode->GetUpdatedItems( removed, added );

    for( unsigned int i = 0; i < removed.size(); i++ )
    {
        BOARD_CONNECTED_ITEM* parent = removed[i]->GetParent();

        if( parent )
        {
            m_undoBuffer.PushItem( ITEM_PICKER( parent, UR_DELETED ) );
            m_board->Remove( parent );
            m_view->Remove( parent );
        }
    }

    BOOST_FOREACH( PNS_ITEM* item, added )
    {
        BOARD_CONNECTED_ITEM* newBI = NULL;

        switch( item->GetKind() )
        {
        case PNS_ITEM::SEGMENT:
            {
                PNS_SEGMENT* seg = static_cast<PNS_SEGMENT*>( item );
                TRACK* track = new TRACK( m_board );
                const SEG& s = seg->GetSeg();

                track->SetStart( wxPoint( s.A.x, s.A.y ) );
                track->SetEnd( wxPoint( s.B.x, s.B.y ) );
                track->SetWidth( seg->GetWidth() );
                track->SetLayer( seg->GetLayers().Start() );
                track->SetNetCode( seg->GetNet() );
                newBI = track;
                break;
            }

        case PNS_ITEM::VIA:
            {
                VIA* via_board = new VIA( m_board );
                PNS_VIA* via = static_cast<PNS_VIA*>( item );
                via_board->SetPosition( wxPoint( via->GetPos().x, via->GetPos().y ) );
                via_board->SetWidth( via->GetDiameter() );
                via_board->SetNetCode( via->GetNet() );
                newBI = via_board;
                break;
            }

        default:
            break;
        }

        if( newBI )
        {
            item->SetParent( newBI );
            newBI->ClearFlags();
            m_view->Add( newBI );
            m_board->Add( newBI );
            m_undoBuffer.PushItem( ITEM_PICKER( newBI, UR_NEW ) );
            newBI->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

    m_world->Commit( aNode );
}


PNS_VIA* PNS_ROUTER::checkLoneVia( PNS_JOINT* aJoint ) const
{
    PNS_VIA* theVia = NULL;
    PNS_LAYERSET l;

    BOOST_FOREACH( PNS_ITEM* item, aJoint->GetLinkList() )
    {
        if( item->GetKind() == PNS_ITEM::VIA )
            theVia = static_cast<PNS_VIA*>( item );

        l.Merge( item->GetLayers() );
    }

    if( l.Start() == l.End() )
        return theVia;

    return NULL;
}


PNS_NODE* PNS_ROUTER::removeLoops( PNS_NODE* aNode, PNS_SEGMENT* aLatestSeg )
{
    PNS_LINE* ourLine = aNode->AssembleLine( aLatestSeg );
    PNS_NODE* cleaned = aNode->Branch();
    PNS_JOINT a, b;

    std::vector<PNS_LINE*> lines;

    cleaned->FindLineEnds( ourLine, a, b );
    cleaned->FindLinesBetweenJoints( a, b, lines );

    BOOST_FOREACH( PNS_LINE* line, lines )
    {
        if( !( line->ContainsSegment( aLatestSeg ) ) )
        {
            cleaned->Remove( line );
        }
    }

    return cleaned;
}


bool PNS_ROUTER::FixRoute( const VECTOR2I& aP, PNS_ITEM* aEndItem )
{
    bool real_end = false;

    PNS_LINE pl = m_placer->GetTrace();
    const SHAPE_LINE_CHAIN& l = pl.GetCLine();

    if( !l.SegmentCount() )
        return true;

    VECTOR2I p_pre_last = l.CPoint( -1 );
    const VECTOR2I p_last = l.CPoint( -1 );
    DIRECTION_45 d_last( l.CSegment( -1 ) );

    if( l.PointCount() > 2 )
        p_pre_last = l.CPoint( -2 );

    if( aEndItem && m_currentNet >= 0 && m_currentNet == aEndItem->GetNet() )
        real_end = true;

    int last = ( real_end || m_placingVia ) ? l.SegmentCount() : std::max( 1, l.SegmentCount() - 1 );

    PNS_NODE* latest = m_placer->GetCurrentNode();

    if( real_end )
        splitAdjacentSegments( latest, aEndItem, aP );

    PNS_SEGMENT* lastSeg = NULL;

    for( int i = 0; i < last; i++ )
    {
        const SEG& s = pl.GetCLine().CSegment( i );
        PNS_SEGMENT* seg = new PNS_SEGMENT( s, m_currentNet );
        seg->SetWidth( pl.GetWidth() );
        seg->SetLayer( m_currentLayer );
        latest->Add( seg );
        lastSeg = seg;
    }

    if( pl.EndsWithVia() )
        latest->Add( pl.GetVia().Clone() );

    if( real_end )
        latest = removeLoops( latest, lastSeg );

    commitRouting( latest );

    EraseView();

    if( real_end )
    {
        m_state = IDLE;
        // m_world->KillChildren();
    }
    else
    {
        m_state = ROUTE_TRACK;
        m_placer->SetInitialDirection( d_last );
        m_currentStart = m_placingVia ? p_last : p_pre_last;

        if( m_placingVia )
            m_currentLayer = NextCopperLayer( true );

        m_placer->StartPlacement( m_currentStart, m_currentNet, m_currentWidth, m_currentLayer );

        m_startsOnVia = m_placingVia;
        m_placingVia = false;
    }

    return real_end;
}


void PNS_ROUTER::StopRouting()
{
    // Update the ratsnest with new changes
    m_board->GetRatsnest()->Recalculate( m_currentNet );

    if( !RoutingInProgress() )
        return;

    EraseView();

    m_state = IDLE;
    m_world->KillChildren();
}


void PNS_ROUTER::FlipPosture()
{
    if( m_placer->GetTail().GetCLine().SegmentCount() == 0 )
    {
        m_start_diagonal = !m_start_diagonal;
        m_placer->SetInitialDirection( m_start_diagonal ?
			DIRECTION_45( DIRECTION_45::NE ) : DIRECTION_45( DIRECTION_45::N ) );
    }
    else
        m_placer->FlipPosture();

    Move( m_currentEnd, NULL );
}


void PNS_ROUTER::SwitchLayer( int layer )
{
    switch( m_state )
    {
    case IDLE:
        m_currentLayer = layer;
        break;

    case ROUTE_TRACK:
    if( m_startsOnVia )
    {
        m_currentLayer = layer;
        m_placer->StartPlacement( m_currentStart, m_currentNet, m_currentWidth,
                m_currentLayer );
    }

    default:
        break;
    }
}


void PNS_ROUTER::ToggleViaPlacement()
{
    if( m_state == ROUTE_TRACK )
    {
        m_placingVia = !m_placingVia;
        m_placer->AddVia( m_placingVia, m_currentViaDiameter, m_currentViaDrill );
    }
}
