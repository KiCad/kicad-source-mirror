/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2016 CERN
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

#include <class_undoredo_container.h>
#include <class_board.h>
#include <class_board_connected_item.h>
#include <class_module.h>
#include <class_track.h>
#include <ratsnest_data.h>
#include <layers_id_colors_and_visibility.h>
#include <geometry/convex_hull.h>
#include <wxPcbStruct.h>

#include <unordered_set>

#include <view/view.h>
#include <view/view_item.h>
#include <view/view_group.h>
#include <gal/graphics_abstraction_layer.h>

#include <pcb_painter.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_convex.h>
#include <geometry/convex_hull.h>


#include "pns_kicad_iface.h"
#include "pns_routing_settings.h"
#include "pns_sizes_settings.h"
#include "pns_item.h"
#include "pns_solid.h"
#include "pns_segment.h"
#include "pns_solid.h"
#include "pns_itemset.h"
#include "pns_node.h"
#include "pns_topology.h"
#include "pns_router.h"
#include "pns_debug_decorator.h"
#include "router_preview_item.h"

class PNS_PCBNEW_RULE_RESOLVER : public PNS_RULE_RESOLVER
{
public:
    PNS_PCBNEW_RULE_RESOLVER( BOARD *aBoard, PNS_ROUTER *aRouter );
    virtual ~PNS_PCBNEW_RULE_RESOLVER();

    virtual int Clearance( const PNS_ITEM* aA, const PNS_ITEM* aB );
    virtual void OverrideClearance (bool aEnable, int aNetA = 0, int aNetB = 0, int aClearance = 0);
    virtual void UseDpGap( bool aUseDpGap ) { m_useDpGap = aUseDpGap; }
    virtual int DpCoupledNet( int aNet );
    virtual int DpNetPolarity( int aNet );
    virtual bool DpNetPair( PNS_ITEM* aItem, int& aNetP, int& aNetN );

private:

    struct CLEARANCE_ENT {
        int coupledNet;
        int clearance;
    };

    int localPadClearance( const PNS_ITEM* aItem ) const;
    int matchDpSuffix( wxString aNetName, wxString& aComplementNet, wxString& aBaseDpName );

    PNS_ROUTER *m_router;
    BOARD *m_board;

    std::vector<CLEARANCE_ENT> m_clearanceCache;
    int m_defaultClearance;
    bool m_overrideEnabled;
    int m_overrideNetA, m_overrideNetB;
    int m_overrideClearance;
    bool m_useDpGap;

};


PNS_PCBNEW_RULE_RESOLVER::PNS_PCBNEW_RULE_RESOLVER( BOARD *aBoard, PNS_ROUTER* aRouter ) :
    m_router( aRouter ),
    m_board( aBoard )
{
    PNS_NODE* world = m_router->GetWorld();

    PNS_TOPOLOGY topo( world );
    m_clearanceCache.resize( m_board->GetNetCount() );

    for( unsigned int i = 0; i < m_board->GetNetCount(); i++ )
    {
        NETINFO_ITEM* ni = m_board->FindNet( i );
        if( ni == NULL )
            continue;

        CLEARANCE_ENT ent;
        ent.coupledNet = DpCoupledNet( i );

        wxString netClassName = ni->GetClassName();
        NETCLASSPTR nc = m_board->GetDesignSettings().m_NetClasses.Find( netClassName );

        int clearance = nc->GetClearance();
        ent.clearance = clearance;
        m_clearanceCache[i] = ent;

        TRACE( 1, "Add net %d netclass %s clearance %d", i % netClassName.mb_str() %
            clearance );
    }

    m_overrideEnabled = false;
    m_defaultClearance = Millimeter2iu( 0.254 );    // m_board->m_NetClasses.Find ("Default clearance")->GetClearance();
    m_overrideNetA = 0;
    m_overrideNetB = 0;
    m_overrideClearance = 0;
}

PNS_PCBNEW_RULE_RESOLVER::~PNS_PCBNEW_RULE_RESOLVER()
{
}

int PNS_PCBNEW_RULE_RESOLVER::localPadClearance( const PNS_ITEM* aItem ) const
{
    if( !aItem->Parent() || aItem->Parent()->Type() != PCB_PAD_T )
        return 0;

    const D_PAD* pad = static_cast<D_PAD*>( aItem->Parent() );
    return pad->GetLocalClearance();
}


int PNS_PCBNEW_RULE_RESOLVER::Clearance( const PNS_ITEM* aA, const PNS_ITEM* aB )
{
    int net_a = aA->Net();
    int cl_a = ( net_a >= 0 ? m_clearanceCache[net_a].clearance : m_defaultClearance );
    int net_b = aB->Net();
    int cl_b = ( net_b >= 0 ? m_clearanceCache[net_b].clearance : m_defaultClearance );

    bool linesOnly = aA->OfKind( PNS_ITEM::SEGMENT | PNS_ITEM::LINE ) && aB->OfKind( PNS_ITEM::SEGMENT | PNS_ITEM::LINE );

    if( linesOnly && net_a >= 0 && net_b >= 0 && m_clearanceCache[net_a].coupledNet == net_b )
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
void PNS_PCBNEW_RULE_RESOLVER::OverrideClearance( bool aEnable, int aNetA, int aNetB , int aClearance )
{
    m_overrideEnabled = aEnable;
    m_overrideNetA = aNetA;
    m_overrideNetB = aNetB;
    m_overrideClearance = aClearance;
}

int PNS_PCBNEW_RULE_RESOLVER::matchDpSuffix( wxString aNetName, wxString& aComplementNet, wxString& aBaseDpName )
{
    int rv = 0;

    if( aNetName.EndsWith( "+" ) )
    {
        aComplementNet = "-";
        rv = 1;
    }
    else if( aNetName.EndsWith( "_P" ) )
    {
        aComplementNet = "_N";
        rv = 1;
    }
    else if( aNetName.EndsWith( "-" ) )
    {
        aComplementNet = "+";
        rv = -1;
    }
    else if( aNetName.EndsWith( "_N" ) )
    {
        aComplementNet = "_P";
        rv = -1;
    }

    if( rv != 0 )
    {
        aBaseDpName = aNetName.Left( aNetName.Length() - aComplementNet.Length() );
        aComplementNet = aBaseDpName + aComplementNet;
    }

    return rv;
}


int PNS_PCBNEW_RULE_RESOLVER::DpCoupledNet( int aNet )
{
    wxString refName = m_board->FindNet( aNet )->GetNetname();
    wxString dummy, coupledNetName;

    if( matchDpSuffix( refName, coupledNetName, dummy ) )
    {
        NETINFO_ITEM* net = m_board->FindNet( coupledNetName );

        if( !net )
            return -1;

        return net->GetNet();

    }

    return -1;
}


int PNS_PCBNEW_RULE_RESOLVER::DpNetPolarity( int aNet )
{
    wxString refName = m_board->FindNet( aNet )->GetNetname();
    wxString dummy1, dummy2;

    return matchDpSuffix( refName, dummy1, dummy2 );
}

bool PNS_PCBNEW_RULE_RESOLVER::DpNetPair( PNS_ITEM* aItem, int& aNetP, int& aNetN )
{
    if( !aItem || !aItem->Parent() || !aItem->Parent()->GetNet() )
        return false;

    wxString netNameP = aItem->Parent()->GetNet()->GetNetname();
    wxString netNameN, netNameCoupled, netNameBase;

    int r = matchDpSuffix ( netNameP, netNameCoupled, netNameBase );

    if( r == 0 )
        return false;
    else if( r == 1 )
    {
        netNameN = netNameCoupled;
    }
    else
    {
        netNameN = netNameP;
        netNameP = netNameCoupled;
    }

//    printf("p %s n %s base %s\n", (const char *)netNameP.c_str(), (const char *)netNameN.c_str(), (const char *)netNameBase.c_str() );

    NETINFO_ITEM* netInfoP = m_board->FindNet( netNameP );
    NETINFO_ITEM* netInfoN = m_board->FindNet( netNameN );

    //printf("ip %p in %p\n", netInfoP, netInfoN);

    if( !netInfoP || !netInfoN )
        return false;

    aNetP = netInfoP->GetNet();
    aNetN = netInfoN->GetNet();

    return true;
}

class PNS_PCBNEW_DEBUG_DECORATOR: public PNS_DEBUG_DECORATOR
{
public:
    PNS_PCBNEW_DEBUG_DECORATOR( KIGFX::VIEW *aView = NULL ): PNS_DEBUG_DECORATOR(),
        m_view( NULL ), m_items( NULL )
    {
        SetView ( aView );
    }

    ~PNS_PCBNEW_DEBUG_DECORATOR()
    {
        Clear();
    }

    void SetView( KIGFX::VIEW *aView )
    {
        Clear();
        delete m_items;
        m_items = NULL;
        m_view = aView;
        if ( m_view == NULL )
            return;
        m_items = new KIGFX::VIEW_GROUP( m_view );
        m_items->SetLayer( ITEM_GAL_LAYER( GP_OVERLAY ) );
        m_view->Add( m_items );
        m_items->ViewSetVisible( true );
    }

    void AddPoint( VECTOR2I aP, int aColor ) override
    {
        SHAPE_LINE_CHAIN l;

        l.Append( aP - VECTOR2I( -50000, -50000 ) );
        l.Append( aP + VECTOR2I( -50000, -50000 ) );

        AddLine ( l, aColor, 10000 );

        l.Clear();
        l.Append( aP - VECTOR2I( 50000, -50000 ) );
        l.Append( aP + VECTOR2I( 50000, -50000 ) );

        AddLine( l, aColor, 10000 );
    }

    void AddBox( BOX2I aB, int aColor ) override
    {
        SHAPE_LINE_CHAIN l;

        VECTOR2I o = aB.GetOrigin();
        VECTOR2I s = aB.GetSize();

        l.Append( o );
        l.Append( o.x + s.x, o.y );
        l.Append( o.x + s.x, o.y + s.y );
        l.Append( o.x, o.y + s.y );
        l.Append( o );

        AddLine( l, aColor, 10000 );
    }

    void AddSegment( SEG aS, int aColor ) override
    {
        SHAPE_LINE_CHAIN l;

        l.Append( aS.A );
        l.Append( aS.B );

        AddLine( l, aColor, 10000 );
    }

    void AddDirections( VECTOR2D aP, int aMask, int aColor ) override
    {
        BOX2I b( aP - VECTOR2I( 10000, 10000 ), VECTOR2I( 20000, 20000 ) );

        AddBox( b, aColor );
        for( int i = 0; i < 8; i++ )
        {
            if( ( 1 << i ) & aMask )
            {
                VECTOR2I v = DIRECTION_45( ( DIRECTION_45::Directions ) i ).ToVector() * 100000;
                AddSegment( SEG( aP, aP + v ), aColor );
            }
        }
    }

    void AddLine( const SHAPE_LINE_CHAIN& aLine, int aType, int aWidth ) override
    {
        ROUTER_PREVIEW_ITEM* pitem = new ROUTER_PREVIEW_ITEM( NULL, m_items );

        pitem->Line( aLine, aWidth, aType );
        m_items->Add( pitem ); // Should not be needed, as m_items has been passed as a parent group in alloc;
        pitem->ViewSetVisible( true );
        m_items->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY | KIGFX::VIEW_ITEM::APPEARANCE );
    }

    void Clear() override
    {
        if (m_view && m_items)
        {
            m_items->FreeItems();
            m_items->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

private:
    KIGFX::VIEW* m_view;
    KIGFX::VIEW_GROUP* m_items;
};

PNS_DEBUG_DECORATOR* PNS_KICAD_IFACE::GetDebugDecorator()
{
    return m_debugDecorator;
}


PNS_KICAD_IFACE::PNS_KICAD_IFACE ()
{
    m_ruleResolver = nullptr;
    m_board = nullptr;
    m_frame = nullptr;
    m_view = nullptr;
    m_previewItems = nullptr;
    m_world = nullptr;
    m_router = nullptr;
    m_debugDecorator = nullptr;
}

PNS_KICAD_IFACE::~PNS_KICAD_IFACE ()
{
    if( m_ruleResolver )
        delete m_ruleResolver;

    if( m_debugDecorator )
        delete m_debugDecorator;
}


PNS_ITEM* PNS_KICAD_IFACE::syncPad( D_PAD* aPad )
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

            case PAD_SHAPE_ROUNDRECT:
            {
                SHAPE_POLY_SET outline;
                const int segmentToCircleCount = 64;

                aPad->BuildPadShapePolygon( outline, wxSize( 0, 0 ),
                                            segmentToCircleCount, 1.0 );

                // TransformRoundRectToPolygon creates only one convex polygon
                SHAPE_LINE_CHAIN& poly = outline.Outline( 0 );
                SHAPE_CONVEX* shape = new SHAPE_CONVEX();

                for( int ii = 0; ii < poly.PointCount(); ++ii )
                {
                    shape->Append( wxPoint( poly.Point( ii ).x, poly.Point( ii ).y ) );
                }

                solid->SetShape( shape );
            }
                break;

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

            case PAD_SHAPE_ROUNDRECT:
            {
                SHAPE_POLY_SET outline;
                const int segmentToCircleCount = 32;
                aPad->BuildPadShapePolygon( outline, wxSize( 0, 0 ),
                                            segmentToCircleCount, 1.0 );

                // TransformRoundRectToPolygon creates only one convex polygon
                SHAPE_LINE_CHAIN& poly = outline.Outline( 0 );
                SHAPE_CONVEX* shape = new SHAPE_CONVEX();

                for( int ii = 0; ii < poly.PointCount(); ++ii )
                {
                    shape->Append( wxPoint( poly.Point( ii ).x, poly.Point( ii ).y ) );
                }

                solid->SetShape( shape );
            }
                break;

            default:
                TRACEn( 0, "unsupported pad shape" );
                delete solid;

                return NULL;
            }
        }
    }
    return solid;
}


PNS_ITEM* PNS_KICAD_IFACE::syncTrack( TRACK* aTrack )
{
    PNS_SEGMENT* s =
        new PNS_SEGMENT( SEG( aTrack->GetStart(), aTrack->GetEnd() ), aTrack->GetNetCode() );

    s->SetWidth( aTrack->GetWidth() );
    s->SetLayers( PNS_LAYERSET( aTrack->GetLayer() ) );
    s->SetParent( aTrack );
    return s;
}


PNS_ITEM* PNS_KICAD_IFACE::syncVia( VIA* aVia )
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


void PNS_KICAD_IFACE::SetBoard( BOARD* aBoard )
{
    m_board = aBoard;
    TRACE( 1, "m_board = %p\n", m_board );
}


void PNS_KICAD_IFACE::SyncWorld( PNS_NODE *aWorld )
{
    if( !m_board )
    {
        TRACEn( 0, "No board attached, aborting sync." );
        return;
    }

    for( MODULE* module = m_board->m_Modules; module; module = module->Next() )
    {
        for( D_PAD* pad = module->Pads(); pad; pad = pad->Next() )
        {
            PNS_ITEM* solid = syncPad( pad );

            if( solid )
                aWorld->Add( solid );
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

        if( t->IsLocked() )
            item->Mark ( MK_LOCKED );

        if( item )
            aWorld->Add( item );
    }

    int worstClearance = m_board->GetDesignSettings().GetBiggestClearanceValue();

    if (m_ruleResolver)
        delete m_ruleResolver;

    m_ruleResolver = new PNS_PCBNEW_RULE_RESOLVER( m_board, m_router );

    aWorld->SetRuleResolver( m_ruleResolver );
    aWorld->SetMaxClearance( 4 * worstClearance );
}

void PNS_KICAD_IFACE::EraseView()
{
    for ( auto item : m_hiddenItems)
        item->ViewSetVisible( true );

    m_hiddenItems.clear();

    if( m_previewItems )
    {
        m_previewItems->FreeItems();
        m_previewItems->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
    }

    if(m_debugDecorator)
        m_debugDecorator->Clear();
}

void PNS_KICAD_IFACE::DisplayItem( const PNS_ITEM* aItem, int aColor, int aClearance )
{
    printf("DisplayItem %p\n", aItem);

    ROUTER_PREVIEW_ITEM* pitem = new ROUTER_PREVIEW_ITEM( aItem, m_previewItems );

    if( aColor >= 0 )
        pitem->SetColor( KIGFX::COLOR4D( aColor ) );

    if( aClearance >= 0 )
        pitem->SetClearance( aClearance );

    m_previewItems->Add( pitem );

    pitem->ViewSetVisible( true );
    m_previewItems->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY | KIGFX::VIEW_ITEM::APPEARANCE );
}

void PNS_KICAD_IFACE::HideItem( PNS_ITEM *aItem )
{
    BOARD_CONNECTED_ITEM* parent = aItem->Parent();

    if( parent )
    {
        if( parent->ViewIsVisible() )
            m_hiddenItems.insert( parent );

        parent->ViewSetVisible( false );
        parent->ViewUpdate( KIGFX::VIEW_ITEM::APPEARANCE );
    }
}

void PNS_KICAD_IFACE::RemoveItem ( PNS_ITEM *aItem )
{
    BOARD_CONNECTED_ITEM* parent = aItem->Parent();

    if( parent )
    {
        m_view->Remove( parent );
        m_board->Remove( parent );
        m_undoBuffer.PushItem( ITEM_PICKER( parent, UR_DELETED ) );
    }
}

void PNS_KICAD_IFACE::AddItem ( PNS_ITEM *aItem )
{
    BOARD_CONNECTED_ITEM* newBI = NULL;

    switch( aItem->Kind() )
    {
    case PNS_ITEM::SEGMENT:
    {
        PNS_SEGMENT* seg = static_cast<PNS_SEGMENT*>( aItem );
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
        PNS_VIA* via = static_cast<PNS_VIA*>( aItem );
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
        aItem->SetParent( newBI );
        newBI->ClearFlags();
        m_view->Add( newBI );
        m_board->Add( newBI );
        m_undoBuffer.PushItem( ITEM_PICKER( newBI, UR_NEW ) );
        newBI->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
    }
}

void PNS_KICAD_IFACE::Commit()
{
    m_board->GetRatsnest()->Recalculate();
    m_frame->SaveCopyInUndoList( m_undoBuffer, UR_UNSPECIFIED );
    m_undoBuffer.ClearItemsList();
    m_frame->OnModify();
}

void PNS_KICAD_IFACE::SetView ( KIGFX::VIEW *aView )
{
    printf("SetView %p\n", aView);
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

    if( m_debugDecorator )
        delete m_debugDecorator;

    m_debugDecorator = new PNS_PCBNEW_DEBUG_DECORATOR();
    m_debugDecorator->SetView ( m_view );
}

void PNS_KICAD_IFACE::UpdateNet ( int aNetCode )
{
    printf("Update-net %d\n", aNetCode);
}

PNS_RULE_RESOLVER* PNS_KICAD_IFACE::GetRuleResolver()
{
    return m_ruleResolver;
}

void PNS_KICAD_IFACE::SetRouter( PNS_ROUTER *aRouter )
{
    m_router = aRouter;
}

void PNS_KICAD_IFACE::SetHostFrame ( PCB_EDIT_FRAME *aFrame )
{
    m_frame = aFrame;
}
