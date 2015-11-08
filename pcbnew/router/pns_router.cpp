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

#include <tools/grid_helper.h>

#include "trace.h"
#include "pns_node.h"
#include "pns_line_placer.h"
#include "pns_line.h"
#include "pns_solid.h"
#include "pns_utils.h"
#include "pns_router.h"
#include "pns_shove.h"
#include "pns_dragger.h"
#include "pns_topology.h"
#include "pns_diff_pair_placer.h"
#include "pns_meander_placer.h"
#include "pns_meander_skew_placer.h"
#include "pns_dp_meander_placer.h"

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


PNS_PCBNEW_CLEARANCE_FUNC::PNS_PCBNEW_CLEARANCE_FUNC( PNS_ROUTER* aRouter ) :
    m_router( aRouter )
{
    BOARD* brd = m_router->GetBoard();
    PNS_NODE* world = m_router->GetWorld();

    PNS_TOPOLOGY topo( world );
    m_clearanceCache.resize( brd->GetNetCount() );
    m_useDpGap = false;

    for( unsigned int i = 0; i < brd->GetNetCount(); i++ )
    {
        NETINFO_ITEM* ni = brd->FindNet( i );
        if( ni == NULL )
            continue;

        CLEARANCE_ENT ent;
        ent.coupledNet = topo.DpCoupledNet( i );

        wxString netClassName = ni->GetClassName();
        NETCLASSPTR nc = brd->GetDesignSettings().m_NetClasses.Find( netClassName );

        int clearance = nc->GetClearance();
        ent.clearance = clearance;
        m_clearanceCache[i] = ent;

        TRACE( 1, "Add net %d netclass %s clearance %d", i % netClassName.mb_str() %
            clearance );
    }

    m_overrideEnabled = false;
    m_defaultClearance = Millimeter2iu( 0.254 );    // aBoard->m_NetClasses.Find ("Default clearance")->GetClearance();
    m_overrideNetA = 0;
    m_overrideNetB = 0;
    m_overrideClearance = 0;
}


PNS_PCBNEW_CLEARANCE_FUNC::~PNS_PCBNEW_CLEARANCE_FUNC()
{
}


int PNS_PCBNEW_CLEARANCE_FUNC::localPadClearance( const PNS_ITEM* aItem ) const
{
    if( !aItem->Parent() || aItem->Parent()->Type() != PCB_PAD_T )
        return 0;

    const D_PAD* pad = static_cast<D_PAD*>( aItem->Parent() );
    return pad->GetLocalClearance();
}


int PNS_PCBNEW_CLEARANCE_FUNC::operator()( const PNS_ITEM* aA, const PNS_ITEM* aB )
{
    int net_a = aA->Net();
    int cl_a = ( net_a >= 0 ? m_clearanceCache[net_a].clearance : m_defaultClearance );
    int net_b = aB->Net();
    int cl_b = ( net_b >= 0 ? m_clearanceCache[net_b].clearance : m_defaultClearance );

    bool linesOnly = aA->OfKind( PNS_ITEM::SEGMENT | PNS_ITEM::LINE ) && aB->OfKind( PNS_ITEM::SEGMENT | PNS_ITEM::LINE );

    if( net_a == net_b )
        return 0;

    if( m_useDpGap && linesOnly && net_a >= 0 && net_b >= 0 && m_clearanceCache[net_a].coupledNet == net_b )
    {
        cl_a = cl_b = m_router->Sizes().DiffPairGap() - 2 * PNS_HULL_MARGIN;
    }

    int pad_a = localPadClearance( aA );
    int pad_b = localPadClearance( aB );

    cl_a = std::max( cl_a, pad_a );
    cl_b = std::max( cl_b, pad_b );

    return std::max( cl_a, cl_b );
}


// fixme: ugly hack to make the optimizer respect gap width for currently routed differential pair.
void PNS_PCBNEW_CLEARANCE_FUNC::OverrideClearance( bool aEnable, int aNetA, int aNetB , int aClearance )
{
    m_overrideEnabled = aEnable;
    m_overrideNetA = aNetA;
    m_overrideNetB = aNetB;
    m_overrideClearance = aClearance;
}


PNS_ITEM* PNS_ROUTER::syncPad( D_PAD* aPad )
{
    PNS_LAYERSET layers( 0, MAX_CU_LAYERS - 1 );

    // ignore non-copper pads
    if ( (aPad->GetLayerSet() & LSET::AllCuMask()).none() )
        return NULL;

    switch( aPad->GetAttribute() )
    {
    case PAD_ATTRIB_STANDARD:
        break;

    case PAD_ATTRIB_SMD:
    case PAD_ATTRIB_HOLE_NOT_PLATED:
    case PAD_ATTRIB_CONN:
        {
            LSET lmsk = aPad->GetLayerSet();
            bool is_copper = false;

            for( int i = 0; i < MAX_CU_LAYERS; i++ )
            {
                if( lmsk[i] )
                {
                    is_copper = true;
                    if( aPad->GetAttribute() != PAD_ATTRIB_HOLE_NOT_PLATED )
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

    wxPoint wx_c = aPad->ShapePos();
    wxSize  wx_sz = aPad->GetSize();
    wxPoint offset = aPad->GetOffset();

    VECTOR2I c( wx_c.x, wx_c.y );
    VECTOR2I sz( wx_sz.x, wx_sz.y );

    RotatePoint( &offset, aPad->GetOrientation() );

    solid->SetPos( VECTOR2I( c.x - offset.x, c.y - offset.y ) );
    solid->SetOffset ( VECTOR2I ( offset.x, offset.y ) );

    double orient = aPad->GetOrientation() / 10.0;

    if( aPad->GetShape() == PAD_SHAPE_CIRCLE )
    {
        solid->SetShape( new SHAPE_CIRCLE( c, sz.x / 2 ) );
    }
    else
    {
        if( orient == 0.0 || orient == 90.0 || orient == 180.0 || orient == 270.0 )
        {
            if( orient == 90.0 || orient == 270.0 )
                sz = VECTOR2I( sz.y, sz.x );

            switch( aPad->GetShape() )
            {
            case PAD_SHAPE_OVAL:
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

            case PAD_SHAPE_RECT:
                solid->SetShape( new SHAPE_RECT( c - sz / 2, sz.x, sz.y ) );
                break;

            case PAD_SHAPE_TRAPEZOID:
            {
                wxPoint coords[4];
                aPad->BuildPadPolygon( coords, wxSize( 0, 0 ), aPad->GetOrientation() );
                SHAPE_CONVEX* shape = new SHAPE_CONVEX();

                for( int ii = 0; ii < 4; ii++ )
                {
                    shape->Append( wx_c + coords[ii] );
                }

                solid->SetShape( shape );
                break;
            }

            default:
                TRACEn( 0, "unsupported pad shape" );
                delete solid;
                return NULL;
            }
        }
        else
        {
            switch( aPad->GetShape() )
            {
            // PAD_SHAPE_CIRCLE already handled above

            case PAD_SHAPE_OVAL:
                if( sz.x == sz.y )
                    solid->SetShape( new SHAPE_CIRCLE( c, sz.x / 2 ) );
                else
                {
                    wxPoint start;
                    wxPoint end;
                    wxPoint corner;

                    SHAPE_CONVEX* shape = new SHAPE_CONVEX();

                    int w = aPad->BuildSegmentFromOvalShape( start, end, 0.0, wxSize( 0, 0 ) );

                    if( start.y == 0 )
                        corner = wxPoint( start.x, -( w / 2 ) );
                    else
                        corner = wxPoint( w / 2, start.y );

                    RotatePoint( &start, aPad->GetOrientation() );
                    RotatePoint( &corner, aPad->GetOrientation() );
                    shape->Append( wx_c + corner );

                    for( int rot = 100; rot <= 1800; rot += 100 )
                    {
                        wxPoint p( corner );
                        RotatePoint( &p, start, rot );
                        shape->Append( wx_c + p );
                    }

                    if( end.y == 0 )
                        corner = wxPoint( end.x, w / 2 );
                    else
                        corner = wxPoint( -( w / 2 ), end.y );

                    RotatePoint( &end, aPad->GetOrientation() );
                    RotatePoint( &corner, aPad->GetOrientation() );
                    shape->Append( wx_c + corner );

                    for( int rot = 100; rot <= 1800; rot += 100 )
                    {
                        wxPoint p( corner );
                        RotatePoint( &p, end, rot );
                        shape->Append( wx_c + p );
                    }

                    solid->SetShape( shape );
                }
                break;

            case PAD_SHAPE_RECT:
            case PAD_SHAPE_TRAPEZOID:
            {
                wxPoint coords[4];
                aPad->BuildPadPolygon( coords, wxSize( 0, 0 ), aPad->GetOrientation() );

                SHAPE_CONVEX* shape = new SHAPE_CONVEX();
                for( int ii = 0; ii < 4; ii++ )
                {
                    shape->Append( wx_c + coords[ii] );
                }

                solid->SetShape( shape );
                break;
            }

            default:
                TRACEn( 0, "unsupported pad shape" );
                delete solid;

                return NULL;
            }
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

    m_world = new PNS_NODE();

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

    int worstClearance = m_board->GetDesignSettings().GetBiggestClearanceValue();
    m_clearanceFunc = new PNS_PCBNEW_CLEARANCE_FUNC( this );
    m_world->SetClearanceFunctor( m_clearanceFunc );
    m_world->SetMaxClearance( 4 * worstClearance );
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
    m_mode = PNS_MODE_ROUTE_SINGLE;

    // Initialize all other variables:
    m_lastNode = NULL;
    m_shove = NULL;
    m_iterLimit = 0;
    m_showInterSteps = false;
    m_snapshotIter = 0;
    m_view = NULL;
    m_currentEndItem = NULL;
    m_snappingEnabled  = false;
    m_violation = false;

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
            anchor = m_gridHelper->AlignToSegment ( aP, s );
            aSplitsSegment = (anchor != s.A && anchor != s.B );
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
    m_clearanceFunc->UseDpGap( false );

    switch( m_mode )
    {
    case PNS_MODE_ROUTE_SINGLE:
        m_placer = new PNS_LINE_PLACER( this );
        break;
    case PNS_MODE_ROUTE_DIFF_PAIR:
        m_placer = new PNS_DIFF_PAIR_PLACER( this );
        m_clearanceFunc->UseDpGap( true );
        break;
    case PNS_MODE_TUNE_SINGLE:
        m_placer = new PNS_MEANDER_PLACER( this );
        break;
    case PNS_MODE_TUNE_DIFF_PAIR:
        m_placer = new PNS_DP_MEANDER_PLACER( this );
        break;
    case PNS_MODE_TUNE_DIFF_PAIR_SKEW:
        m_placer = new PNS_MEANDER_SKEW_PLACER( this );
        break;

    default:
        return false;
    }

    m_placer->UpdateSizes ( m_sizes );
    m_placer->SetLayer( aLayer );

    bool rv = m_placer->Start( aP, aStartItem );

    if( !rv )
        return false;

    m_currentEnd = aP;
    m_currentEndItem = NULL;
    m_state = ROUTE_TRACK;
    return rv;
}


BOARD* PNS_ROUTER::GetBoard()
{
    return m_board;
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
        pitem->SetColor( KIGFX::COLOR4D( aColor ) );

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
    PNS_ITEMSET current = m_placer->Traces();

    BOOST_FOREACH( const PNS_ITEM* item, current.CItems() )
    {
        if( !item->OfKind( PNS_ITEM::LINE ) )
            continue;

        const PNS_LINE* l = static_cast<const PNS_LINE*>( item );
        DisplayItem( l );

        if( l->EndsWithVia() )
            DisplayItem( &l->Via() );
    }

    //PNS_ITEMSET tmp( &current );

    updateView( m_placer->CurrentNode( true ), current );
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

    m_board->GetRatsnest()->Recalculate();
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
        std::vector<int> nets;
        m_placer->GetModifiedNets( nets );

        BOOST_FOREACH ( int n, nets )
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


const std::vector<int> PNS_ROUTER::GetCurrentNets() const
{
    if( m_placer )
        return m_placer->CurrentNets();

    return std::vector<int>();
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

    case ROUTE_TRACK:
        logger = m_placer->Logger();
        break;

    default:
        break;
    }

    if( logger )
        logger->Save( "/tmp/shove.log" );
}


bool PNS_ROUTER::IsPlacingVia() const
{
    if( !m_placer )
        return false;

    return m_placer->IsPlacingVia();
}


void PNS_ROUTER::SetOrthoMode( bool aEnable )
{
    if( !m_placer )
        return;

    m_placer->SetOrthoMode( aEnable );
}


void PNS_ROUTER::SetMode( PNS_ROUTER_MODE aMode )
{
    m_mode = aMode;
}
