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
#include "pns_shove.h"
#include "pns_dragger.h"

#include <router/router_preview_item.h>

#include <class_board.h>
#include <class_board_connected_item.h>
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
            NETCLASSPTR nc = aBoard->GetDesignSettings().m_NetClasses.Find( netClassName );
            int clearance = nc->GetClearance();
            m_clearanceCache[i] = clearance;
            TRACE( 1, "Add net %d netclass %s clearance %d", i % netClassName.mb_str() %
                    clearance );
        }

        m_defaultClearance = 254000;    // aBoard->m_NetClasses.Find ("Default clearance")->GetClearance();
    }

    int localPadClearance( const PNS_ITEM* aItem ) const
    {
        if( !aItem->Parent() || aItem->Parent()->Type() != PCB_PAD_T )
            return 0;

        const D_PAD* pad = static_cast<D_PAD*>( aItem->Parent() );

        return pad->GetLocalClearance();
    }

    int operator()( const PNS_ITEM* aA, const PNS_ITEM* aB )
    {
        int net_a = aA->Net();
        int cl_a = ( net_a >= 0 ? m_clearanceCache[net_a] : m_defaultClearance );
        int net_b = aB->Net();
        int cl_b = ( net_b >= 0 ? m_clearanceCache[net_b] : m_defaultClearance );

        int pad_a = localPadClearance( aA );
        int pad_b = localPadClearance( aB );

        cl_a = std::max( cl_a, pad_a );
        cl_b = std::max( cl_b, pad_b );

        return std::max( cl_a, cl_b );
    }

private:
    std::vector<int> m_clearanceCache;
    int m_defaultClearance;
};


PNS_ITEM* PNS_ROUTER::syncPad( D_PAD* aPad )
{
    PNS_LAYERSET layers( 0, MAX_CU_LAYERS - 1 );

    switch( aPad->GetAttribute() )
    {
    case PAD_STANDARD:
        break;

    case PAD_SMD:
    case PAD_CONN:
        {
            LSET lmsk = aPad->GetLayerSet();
            bool is_copper = false;

            for( int i = 0; i < MAX_CU_LAYERS; i++ )
            {
                if( lmsk[i] )
                {
                    is_copper = true;
                    layers = PNS_LAYERSET( i );
                    break;
                }
            }

            if( !is_copper )
                return NULL;
        }
        break;

    default:
        TRACE( 0, "unsupported pad type 0x%x", aPad->GetAttribute() );
        return NULL;
    }

    PNS_SOLID* solid = new PNS_SOLID;

    solid->SetLayers( layers );
    solid->SetNet( aPad->GetNetCode() );
    solid->SetParent( aPad );

    wxPoint wx_c = aPad->GetPosition();
    wxSize  wx_sz = aPad->GetSize();

    VECTOR2I c( wx_c.x, wx_c.y );
    VECTOR2I sz( wx_sz.x, wx_sz.y );

    solid->SetPos( c );

    double orient = aPad->GetOrientation() / 10.0;
    bool nonOrtho = false;

    if( orient == 90.0 || orient == 270.0 )
        sz = VECTOR2I( sz.y, sz.x );
    else if( orient != 0.0 && orient != 180.0 )
    {
        // rotated pads are replaced by for the moment by circles due to my laziness ;)
        solid->SetShape( new SHAPE_CIRCLE( c, std::min( sz.x, sz.y ) / 2 ) );
        nonOrtho = true;
    }

    if( !nonOrtho )
    {
        switch( aPad->GetShape() )
        {
        case PAD_CIRCLE:
            solid->SetShape( new SHAPE_CIRCLE( c, sz.x / 2 ) );
            break;

        case PAD_OVAL:
            if( sz.x == sz.y )
                solid->SetShape( new SHAPE_CIRCLE( c, sz.x / 2 ) );
            else
            {
                VECTOR2I delta;

                if( sz.x > sz.y )
                    delta = VECTOR2I( ( sz.x - sz.y ) / 2, 0 );
                else
                    delta = VECTOR2I( 0, ( sz.y - sz.x ) / 2 );

                SHAPE_SEGMENT* shape = new SHAPE_SEGMENT( c - delta, c + delta,
                                                          std::min( sz.x, sz.y ) );
                solid->SetShape( shape );
            }
            break;

        case PAD_RECT:
            solid->SetShape( new SHAPE_RECT( c - sz / 2, sz.x, sz.y ) );
            break;

        default:
            TRACEn( 0, "unsupported pad shape" );
            delete solid;

            return NULL;
        }
    }

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
    LAYER_ID top, bottom;
    aVia->LayerPair( &top, &bottom );
    PNS_VIA* v = new PNS_VIA(
            aVia->GetPosition(),
            PNS_LAYERSET( top, bottom ),
            aVia->GetWidth(),
            aVia->GetDrillValue(),
            aVia->GetNetCode(),
            aVia->GetViaType() );

    v->SetParent( aVia );

    return v;
}


void PNS_ROUTER::SetBoard( BOARD* aBoard )
{
    m_board = aBoard;
    TRACE( 1, "m_board = %p\n", m_board );
}

void PNS_ROUTER::SyncWorld()
{
    if( !m_board )
    {
        TRACEn( 0, "No board attached, aborting sync." );
        return;
    }

    ClearWorld();

    int worstClearance = m_board->GetDesignSettings().GetBiggestClearanceValue();

    m_clearanceFunc = new PCBNEW_CLEARANCE_FUNC( m_board );
    m_world = new PNS_NODE();
    m_world->SetClearanceFunctor( m_clearanceFunc );
    m_world->SetMaxClearance( 4 * worstClearance );

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
}


PNS_ROUTER::PNS_ROUTER()
{
    theRouter = this;

    m_clearanceFunc = NULL;

    m_state = IDLE;
    m_world = NULL;
    m_placer = NULL;
    m_previewItems = NULL;
    m_board = NULL;
    m_dragger = NULL;
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
    {
        m_world->KillChildren();
        delete m_world;
    }

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


bool PNS_ROUTER::RoutingInProgress() const
{
    return m_state != IDLE;
}


const PNS_ITEMSET PNS_ROUTER::QueryHoverItems( const VECTOR2I& aP )
{
    if( m_state == IDLE )
        return m_world->HitTest( aP );
    else
    {
        //assert ( m_placer->GetCurrentNode()->checkExists() );
        //TRACE(0,"query-hover [%p]", m_placer->GetCurrentNode());
        return m_placer->CurrentNode()->HitTest( aP );
    }
}


const VECTOR2I PNS_ROUTER::SnapToItem( PNS_ITEM* aItem, VECTOR2I aP, bool& aSplitsSegment )
{
    VECTOR2I anchor;

    if( !aItem )
    {
        aSplitsSegment = false;
        return aP;
    }

    switch( aItem->Kind() )
    {
    case PNS_ITEM::SOLID:
        anchor = static_cast<PNS_SOLID*>( aItem )->Pos();
        aSplitsSegment = false;
        break;

    case PNS_ITEM::VIA:
        anchor = static_cast<PNS_VIA*>( aItem )->Pos();
        aSplitsSegment = false;
        break;

    case PNS_ITEM::SEGMENT:
    {
        PNS_SEGMENT* seg = static_cast<PNS_SEGMENT*>( aItem );
        const SEG& s = seg->Seg();
        int w = seg->Width();

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


bool PNS_ROUTER::StartDragging( const VECTOR2I& aP, PNS_ITEM* aStartItem )
{
    if( !aStartItem || aStartItem->OfKind( PNS_ITEM::SOLID ) )
        return false;

    m_dragger = new PNS_DRAGGER( this );
    m_dragger->SetWorld( m_world );

    if( m_dragger->Start ( aP, aStartItem ) )
        m_state = DRAG_SEGMENT;
    else
    {
        delete m_dragger;
        m_state = IDLE;
        return false;
    }

    return true;
}

bool PNS_ROUTER::StartRouting( const VECTOR2I& aP, PNS_ITEM* aStartItem, int aLayer )
{
    m_placer = new PNS_LINE_PLACER( this );

    m_placer->UpdateSizes ( m_sizes );
    m_placer->SetLayer( aLayer );
    m_placer->Start( aP, aStartItem );

    m_currentEnd = aP;
    m_currentEndItem = NULL;
    m_state = ROUTE_TRACK;

    return true;
}

void PNS_ROUTER::eraseView()
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


void PNS_ROUTER::DisplayItem( const PNS_ITEM* aItem, int aColor, int aClearance )
{
    ROUTER_PREVIEW_ITEM* pitem = new ROUTER_PREVIEW_ITEM( aItem, m_previewItems );

    if( aColor >= 0 )
        pitem->SetColor( KIGFX::COLOR4D ( aColor ) );

    if( aClearance >= 0 )
        pitem->SetClearance( aClearance );

    m_previewItems->Add( pitem );

    pitem->ViewSetVisible( true );
    m_previewItems->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY | KIGFX::VIEW_ITEM::APPEARANCE );
}


void PNS_ROUTER::DisplayItems( const PNS_ITEMSET& aItems )
{
    BOOST_FOREACH( const PNS_ITEM* item, aItems.CItems() )
        DisplayItem( item );
}


void PNS_ROUTER::DisplayDebugLine( const SHAPE_LINE_CHAIN& aLine, int aType, int aWidth )
{
    ROUTER_PREVIEW_ITEM* pitem = new ROUTER_PREVIEW_ITEM( NULL, m_previewItems );

    pitem->Line( aLine, aWidth, aType );
    m_previewItems->Add( pitem );
    pitem->ViewSetVisible( true );
    m_previewItems->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY | KIGFX::VIEW_ITEM::APPEARANCE );
}


void PNS_ROUTER::DisplayDebugPoint( const VECTOR2I aPos, int aType )
{
    ROUTER_PREVIEW_ITEM* pitem = new ROUTER_PREVIEW_ITEM( NULL, m_previewItems );

    pitem->Point( aPos, aType );
    m_previewItems->Add( pitem );
    pitem->ViewSetVisible( true );
    m_previewItems->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY | KIGFX::VIEW_ITEM::APPEARANCE );
}


void PNS_ROUTER::Move( const VECTOR2I& aP, PNS_ITEM* endItem )
{
    m_currentEnd = aP;
    m_currentEndItem = endItem;

    switch( m_state )
    {
        case ROUTE_TRACK:
            movePlacing( aP, endItem );
            break;

        case DRAG_SEGMENT:
            moveDragging( aP, endItem );
            break;

        default:
            break;
    }
}


void PNS_ROUTER::moveDragging( const VECTOR2I& aP, PNS_ITEM* aEndItem )
{
    eraseView();

    m_dragger->Drag( aP );
    PNS_ITEMSET dragged = m_dragger->Traces();

    updateView( m_dragger->CurrentNode(), dragged );
}


void PNS_ROUTER::markViolations( PNS_NODE* aNode, PNS_ITEMSET& aCurrent,
                                 PNS_NODE::ITEM_VECTOR& aRemoved )
{
    BOOST_FOREACH( PNS_ITEM* item, aCurrent.Items() )
    {
        PNS_NODE::OBSTACLES obstacles;

        aNode->QueryColliding( item, obstacles, PNS_ITEM::ANY );

        if( item->OfKind( PNS_ITEM::LINE ) )
        {
            PNS_LINE* l = static_cast<PNS_LINE*>( item );

            if( l->EndsWithVia() )
            {
                PNS_VIA v( l->Via() );
                aNode->QueryColliding( &v, obstacles, PNS_ITEM::ANY );
            }
        }

        BOOST_FOREACH( PNS_OBSTACLE& obs, obstacles )
        {
            int clearance = aNode->GetClearance( item, obs.m_item );
            std::auto_ptr<PNS_ITEM> tmp( obs.m_item->Clone() );
            tmp->Mark( MK_VIOLATION );
            DisplayItem( tmp.get(), -1, clearance );
            aRemoved.push_back( obs.m_item );
        }
    }
}


void PNS_ROUTER::updateView( PNS_NODE* aNode, PNS_ITEMSET& aCurrent )
{
    PNS_NODE::ITEM_VECTOR removed, added;
    PNS_NODE::OBSTACLES obstacles;

    if( !aNode )
        return;

    if( Settings().Mode() == RM_MarkObstacles )
        markViolations( aNode, aCurrent, removed );

    aNode->GetUpdatedItems( removed, added );

    BOOST_FOREACH( PNS_ITEM* item, added )
    {
        DisplayItem( item );
    }

    BOOST_FOREACH( PNS_ITEM* item, removed )
    {
        BOARD_CONNECTED_ITEM* parent = item->Parent();

        if( parent )
        {
            if( parent->ViewIsVisible() )
                m_hiddenItems.insert( parent );

            parent->ViewSetVisible( false );
            parent->ViewUpdate( KIGFX::VIEW_ITEM::APPEARANCE );
        }
    }
}


void PNS_ROUTER::UpdateSizes ( const PNS_SIZES_SETTINGS& aSizes )
{
    m_sizes = aSizes;

    // Change track/via size settings
    if( m_state == ROUTE_TRACK)
    {
        m_placer->UpdateSizes( m_sizes );
        movePlacing( m_currentEnd, m_currentEndItem );
    }
}


void PNS_ROUTER::movePlacing( const VECTOR2I& aP, PNS_ITEM* aEndItem )
{
    eraseView();

    m_placer->Move( aP, aEndItem );
    PNS_LINE current = m_placer->Trace();

    DisplayItem( &current );

    if( current.EndsWithVia() )
        DisplayItem( &current.Via() );

    PNS_ITEMSET tmp( &current );
    updateView( m_placer->CurrentNode( true ), tmp );
}


void PNS_ROUTER::CommitRouting( PNS_NODE* aNode )
{
    PNS_NODE::ITEM_VECTOR removed, added;

    aNode->GetUpdatedItems( removed, added );

    for( unsigned int i = 0; i < removed.size(); i++ )
    {
        BOARD_CONNECTED_ITEM* parent = removed[i]->Parent();

        if( parent )
        {
            m_view->Remove( parent );
            m_board->Remove( parent );
            m_undoBuffer.PushItem( ITEM_PICKER( parent, UR_DELETED ) );
        }
    }

    BOOST_FOREACH( PNS_ITEM* item, added )
    {
        BOARD_CONNECTED_ITEM* newBI = NULL;

        switch( item->Kind() )
        {
        case PNS_ITEM::SEGMENT:
        {
            PNS_SEGMENT* seg = static_cast<PNS_SEGMENT*>( item );
            TRACK* track = new TRACK( m_board );
            const SEG& s = seg->Seg();

            track->SetStart( wxPoint( s.A.x, s.A.y ) );
            track->SetEnd( wxPoint( s.B.x, s.B.y ) );
            track->SetWidth( seg->Width() );
            track->SetLayer( ToLAYER_ID( seg->Layers().Start() ) );
            track->SetNetCode( seg->Net() > 0 ? seg->Net() : 0 );
            newBI = track;
            break;
        }

        case PNS_ITEM::VIA:
        {
            VIA* via_board = new VIA( m_board );
            PNS_VIA* via = static_cast<PNS_VIA*>( item );
            via_board->SetPosition( wxPoint( via->Pos().x, via->Pos().y ) );
            via_board->SetWidth( via->Diameter() );
            via_board->SetDrill( via->Drill() );
            via_board->SetNetCode( via->Net() > 0 ? via->Net() : 0 );
            via_board->SetViaType( via->ViaType() ); // MUST be before SetLayerPair()
            via_board->SetLayerPair( ToLAYER_ID( via->Layers().Start() ),
                                     ToLAYER_ID( via->Layers().End() ) );
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


bool PNS_ROUTER::FixRoute( const VECTOR2I& aP, PNS_ITEM* aEndItem )
{
    bool rv = false;

    switch( m_state )
    {
        case ROUTE_TRACK:
            rv = m_placer->FixRoute( aP, aEndItem );
            break;

        case DRAG_SEGMENT:
            rv = m_dragger->FixRoute();
            break;

        default:
            break;
    }

    if( rv )
       StopRouting();

    return rv;
}


void PNS_ROUTER::StopRouting()
{
    // Update the ratsnest with new changes
    if( m_placer )
    {
        int n = m_placer->CurrentNet();

        if( n >= 0)
        {
            // Update the ratsnest with new changes
            m_board->GetRatsnest()->Recalculate( n );
        }
    }


    if( !RoutingInProgress() )
        return;

    if( m_placer )
        delete m_placer;

    if( m_dragger )
        delete m_dragger;

    m_placer = NULL;
    m_dragger = NULL;

    eraseView();

    m_state = IDLE;
    m_world->KillChildren();
    m_world->ClearRanks();
}


void PNS_ROUTER::FlipPosture()
{
    if( m_state == ROUTE_TRACK )
    {
        m_placer->FlipPosture();
        movePlacing ( m_currentEnd, m_currentEndItem );
    }
}


void PNS_ROUTER::SwitchLayer( int aLayer )
{
    switch( m_state )
    {
        case ROUTE_TRACK:
            m_placer->SetLayer( aLayer );
            break;
        default:
            break;
    }
}


void PNS_ROUTER::ToggleViaPlacement()
{
	if( m_state == ROUTE_TRACK )
    {
		bool toggle = !m_placer->IsPlacingVia();
        m_placer->ToggleVia( toggle );
    }
}


int PNS_ROUTER::GetCurrentNet() const
{
    if( m_placer )
        return m_placer->CurrentNet();
    return -1;
}


int PNS_ROUTER::GetCurrentLayer() const
{
    if( m_placer )
        return m_placer->CurrentLayer();
    return -1;
}


void PNS_ROUTER::DumpLog()
{
    PNS_LOGGER* logger = NULL;

    switch( m_state )
    {
        case DRAG_SEGMENT:
            logger = m_dragger->Logger();
            break;

        default:
            break;
    }

    if( logger )
        logger->Save( "/tmp/shove.log" );
}

bool PNS_ROUTER::IsPlacingVia() const
{
    if(!m_placer)
        return NULL;
    return m_placer->IsPlacingVia();
}

