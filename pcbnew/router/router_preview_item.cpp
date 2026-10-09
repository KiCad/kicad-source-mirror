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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "router_preview_item.h"


#include <board_item.h>
#include <board_connected_item.h>
#include <gal/color4d.h>
#include <gal/graphics_abstraction_layer.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_simple.h>
#include <geometry/shape_ellipse.h>
#include <pcb_painter.h>
#include <trigo.h>

#include "pns_arc.h"
#include "pns_line.h"
#include "pns_segment.h"
#include "pns_via.h"
#include "pns_kicad_iface.h"

#include <pcb_painter.h>
#include <netinfo.h>
#include <layer_ids.h>

using namespace KIGFX;


ROUTER_PREVIEW_ITEM::ROUTER_PREVIEW_ITEM( const PNS::ITEM* aItem, PNS::ROUTER_IFACE* aIface,
                                          KIGFX::VIEW* aView, int aFlags, DISPLAY_MODE aMode ) :
        EDA_ITEM( NOT_USED ),
        m_view( aView ),
        m_iface( aIface ),
        m_shape( nullptr ),
        m_hole( nullptr ),
        m_flags( aFlags ),
        m_mode( aMode )
{
    BOARD_ITEM* boardItem = aItem ? aItem->BoardItem() : nullptr;

    // A PNS::SOLID for an edge-cut item must have 0 width for collision calculations, but when
    // highlighting an edge we want to show it with its true width
    if( boardItem && boardItem->IsOnLayer( Edge_Cuts ) )
    {
        m_shape = aItem->Shape( -1 )->Clone();

        switch( m_shape->Type() )
        {
        case SH_SEGMENT:    static_cast<SHAPE_SEGMENT*>( m_shape )->SetWidth( 0 );    break;
        case SH_ARC:        static_cast<SHAPE_ARC*>( m_shape )->SetWidth( 0 );        break;
        case SH_LINE_CHAIN: static_cast<SHAPE_LINE_CHAIN*>( m_shape )->SetWidth( 0 ); break;
        default:            /* remaining shapes don't have width */                   break;
        }
    }
    else if( aItem )
    {
        // TODO(JE) padstacks -- need to know the layer here
        m_shape = aItem->Shape( -1 )->Clone();

        if( aItem->HasHole() )
            m_hole = aItem->Hole()->Shape( -1 )->Clone();
    }

    m_clearance = -1;
    m_originLayer = LAYER_SELECT_OVERLAY;
    m_layer = chooseLayer();


    // initialize variables, overwritten by Update( aItem ), if aItem != NULL
    m_type = PR_SHAPE;
    m_width = ( aFlags & PNS_SEMI_SOLID ) ? 1 : 0;

    // Depth follows the drawing layer so a collision marker passes the OpenGL depth test over the overlay
    m_depth = m_originDepth = aView->GetLayerOrder( m_layer );

    if( aItem )
        Update( aItem );
}


ROUTER_PREVIEW_ITEM::ROUTER_PREVIEW_ITEM( const SHAPE& aShape, PNS::ROUTER_IFACE* aIface,
                                          KIGFX::VIEW* aView ) :
        EDA_ITEM( NOT_USED ),
        m_view( aView ),
        m_iface( aIface ),
        m_flags( 0 ),
        m_mode( DM_SHAPE )
{
    m_shape = aShape.Clone();
    m_hole = nullptr;

    m_clearance = -1;
    m_originLayer = m_layer = LAYER_SELECT_OVERLAY;


    // initialize variables, overwritten by Update( aItem ), if aItem != NULL
    m_type = PR_SHAPE;
    m_width = 0;
    m_depth = m_originDepth = aView->GetLayerOrder( m_originLayer );
}


ROUTER_PREVIEW_ITEM::~ROUTER_PREVIEW_ITEM()
{
    delete m_shape;
    delete m_hole;
}


void ROUTER_PREVIEW_ITEM::Update( const PNS::ITEM* aItem )
{
    m_originLayer = m_iface->GetBoardLayerFromPNSLayer( aItem->Layers().Start() );

    if( const PNS::LINE* l = dyn_cast<const PNS::LINE*>( aItem ) )
    {
        if( !l->SegmentCount() )
            return;
    }
    else if( const PNS::VIA* v = dyn_cast<const PNS::VIA*>( aItem ) )
    {
        if( v->IsVirtual() )
            return;
    }

    if( m_originLayer < 0 )
        m_originLayer = 0;

    m_layer = m_originLayer;
    m_color = getLayerColor( m_originLayer, aItem );
    m_color.a = 0.8;
    m_depth = m_originDepth - ( ( aItem->Layers().Start() + 1 ) * LayerDepthFactor );

    switch( aItem->Kind() )
    {
    case PNS::ITEM::LINE_T:
        m_type  = PR_SHAPE;
        m_width = static_cast<const PNS::LINE*>( aItem )->Width();
        break;

    case PNS::ITEM::ARC_T:
        m_type = PR_SHAPE;
        m_width = static_cast<const PNS::ARC*>( aItem )->Width();
        break;

    case PNS::ITEM::SEGMENT_T:
        m_type  = PR_SHAPE;
        m_width = static_cast<const PNS::SEGMENT*>( aItem )->Width();
        break;

    case PNS::ITEM::VIA_T:
    {
        m_originLayer = LAYER_VIAS;
        m_type = PR_SHAPE;
        m_width = 0;
        m_color = COLOR4D( 0.7, 0.7, 0.7, 0.8 );
        m_depth = m_originDepth - ( static_cast<double>( PCB_LAYER_ID_COUNT ) * LayerDepthFactor );

        delete m_shape;
        m_shape = nullptr;

        auto via = static_cast<const PNS::VIA*>( aItem );
        int shapeLayer = -1;
        int largestDiameter = 0;

        for( int layer : via->UniqueShapeLayers() )
        {
            if( via->Diameter( layer ) > largestDiameter )
            {
                largestDiameter = via->Diameter( layer );
                shapeLayer = layer;
            }
        }

        if( aItem->Shape( shapeLayer ) )
            m_shape = aItem->Shape( shapeLayer )->Clone();

        delete m_hole;
        m_hole = nullptr;

        if( aItem->HasHole() )
            m_hole = aItem->Hole()->Shape( -1 )->Clone();

        break;
    }

    case PNS::ITEM::SOLID_T:
        m_type = PR_SHAPE;
        break;

    default:
        break;
    }

    if( aItem->Marker() & PNS::MK_VIOLATION )
        m_flags |= PNS_COLLISION;

    if( m_flags & PNS_COLLISION )
        m_color = COLOR4D( 0, 1, 0, 1 );

    if( m_flags & PNS_HOVER_ITEM )
        m_color = m_color.WithAlpha( 1.0 );

    m_layer = chooseLayer();
}


int ROUTER_PREVIEW_ITEM::chooseLayer() const
{
    // Halos go behind copper and collision markers go on top
    if( m_mode == DM_SHADOW )
        return LAYER_ROUTER_PREVIEW_SHADOW;

    if( m_flags & PNS_COLLISION )
        return LAYER_ROUTER_PREVIEW_COLLISION;

    return LAYER_SELECT_OVERLAY;
}


const BOX2I ROUTER_PREVIEW_ITEM::ViewBBox() const
{
    BOX2I bbox;

    switch( m_type )
    {
    case PR_SHAPE:
        if( m_shape )
        {
            bbox = m_shape->BBox();
            bbox.Inflate( m_width / 2 );

            if( m_mode == DM_SHADOW && m_clearance > 0 )
                bbox.Inflate( m_clearance );
        }

        if( m_hole )
            bbox.Merge( m_hole->BBox() );

        return bbox;

    case PR_POINT:
        bbox = BOX2I ( m_pos - VECTOR2I( 100000, 100000 ), VECTOR2I( 200000, 200000 ) );
        return bbox;

    default:
        break;
    }

    return bbox;
}


void ROUTER_PREVIEW_ITEM::drawLineChain( const SHAPE_LINE_CHAIN_BASE* aL, KIGFX::GAL* gal ) const
{
    wxCHECK( aL, /* void */ );

    gal->SetIsFill( false );

    for( int s = 0; s < (int) aL->GetSegmentCount(); s++ )
    {
        SEG seg = aL->GetSegment( s );

        if( seg.A == seg.B )
        {
            gal->SetIsFill( true );
            gal->SetIsStroke( false );
            gal->DrawCircle( seg.A, gal->GetLineWidth() / 2 );
            gal->SetIsFill( false );
            gal->SetIsStroke( true );
        }
        else
        {
            gal->DrawLine( seg.A, seg.B );
        }
    }

    const SHAPE_LINE_CHAIN* lineChain = dynamic_cast<const SHAPE_LINE_CHAIN*>( aL );

    for( size_t s = 0; lineChain && s < lineChain->ArcCount(); s++ )
    {
        const SHAPE_ARC& arc = lineChain->CArcs()[s];
        EDA_ANGLE        start_angle = arc.GetStartAngle();
        EDA_ANGLE        angle = arc.GetCentralAngle();

        gal->DrawArc( arc.GetCenter(), arc.GetRadius(), start_angle, angle);
    }

    if( aL->IsClosed() )
        gal->DrawLine( aL->GetSegment( -1 ).B, aL->GetSegment( 0 ).A );
}


void ROUTER_PREVIEW_ITEM::drawClearanceHalo( const SHAPE* aShape, KIGFX::GAL* gal ) const
{
    if( m_clearance <= 0 )
        return;

    switch( aShape->Type() )
    {
    case SH_POLY_SET_TRIANGLE:
    case SH_LINE_CHAIN:
    {
        const SHAPE_LINE_CHAIN_BASE* l = static_cast<const SHAPE_LINE_CHAIN_BASE*>( aShape );

        gal->SetLineWidth( m_width + 2 * m_clearance );
        drawLineChain( l, gal );
        break;
    }

    case SH_SEGMENT:
    {
        const SHAPE_SEGMENT* s = static_cast<const SHAPE_SEGMENT*>( aShape );

        gal->SetIsStroke( false );
        gal->DrawSegment( s->GetSeg().A, s->GetSeg().B, s->GetWidth() + 2 * m_clearance );
        break;
    }

    case SH_CIRCLE:
    {
        const SHAPE_CIRCLE* c = static_cast<const SHAPE_CIRCLE*>( aShape );

        gal->SetIsStroke( false );
        gal->DrawCircle( c->GetCenter(), c->GetRadius() + m_clearance );
        break;
    }

    case SH_RECT:
    {
        gal->SetIsStroke( true );
        gal->SetLineWidth( 2 * m_clearance );
        gal->DrawPolyline( static_cast<const SHAPE_RECT*>( aShape )->Outline() );
        break;
    }

    case SH_SIMPLE:
    {
        gal->SetIsStroke( true );
        gal->SetLineWidth( 2 * m_clearance );
        gal->DrawPolyline( static_cast<const SHAPE_SIMPLE*>( aShape )->Vertices() );
        break;
    }

    case SH_ARC:
    {
        const SHAPE_ARC* arc = static_cast<const SHAPE_ARC*>( aShape );

        gal->SetIsFill( false );
        gal->SetIsStroke( true );
        gal->SetLineWidth( arc->GetWidth() + 2 * m_clearance );
        gal->DrawArc( arc->GetCenter(), arc->GetRadius(), arc->GetStartAngle(),
                      arc->GetCentralAngle() );
        break;
    }

    case SH_ELLIPSE:
    {
        const SHAPE_ELLIPSE* ellipse = static_cast<const SHAPE_ELLIPSE*>( aShape );

        gal->SetIsFill( false );
        gal->SetIsStroke( true );
        gal->SetLineWidth( std::max( 1, ellipse->GetWidth() ) + 2 * m_clearance );

        if( ellipse->IsArc() )
        {
            gal->DrawEllipseArc( ellipse->GetCenter(), ellipse->GetMajorRadius(), ellipse->GetMinorRadius(),
                                 ellipse->GetRotation(), ellipse->GetStartAngle(), ellipse->GetEndAngle() );
        }
        else
        {
            gal->DrawEllipse( ellipse->GetCenter(), ellipse->GetMajorRadius(), ellipse->GetMinorRadius(),
                              ellipse->GetRotation() );
        }

        break;
    }

    default:
        break;
    }
}


void ROUTER_PREVIEW_ITEM::drawShape( const SHAPE* aShape, KIGFX::GAL* gal ) const
{
    bool holeDrawn = false;

    switch( aShape->Type() )
    {
    case SH_POLY_SET_TRIANGLE:
    case SH_LINE_CHAIN:
    {
        const SHAPE_LINE_CHAIN_BASE* l = static_cast<const SHAPE_LINE_CHAIN_BASE*>( aShape );

        gal->SetLayerDepth( m_depth );
        gal->SetLineWidth( m_width );
        gal->SetStrokeColor( m_color );
        gal->SetFillColor( m_color );
        drawLineChain( l, gal );
        break;
    }

    case SH_SEGMENT:
    {
        const SHAPE_SEGMENT* s = static_cast<const SHAPE_SEGMENT*>( aShape );

        gal->SetIsStroke( false );
        gal->SetLayerDepth( m_depth );
        gal->SetLineWidth( s->GetWidth() );
        gal->SetFillColor( m_color );
        gal->DrawSegment( s->GetSeg().A, s->GetSeg().B, s->GetWidth() );
        break;
    }

    case SH_CIRCLE:
    {
        const SHAPE_CIRCLE* c = static_cast<const SHAPE_CIRCLE*>( aShape );

        gal->SetStrokeColor( m_color );
        gal->SetLayerDepth( m_depth );

        if( m_hole && dynamic_cast<SHAPE_CIRCLE*>( m_hole ) )
        {
            const SHAPE_CIRCLE* h = static_cast<const SHAPE_CIRCLE*>( m_hole );
            int                 halfWidth = m_width / 2;

            gal->SetIsStroke( true );
            gal->SetIsFill( false );
            gal->SetLineWidth( halfWidth + c->GetRadius() - h->GetRadius() );
            gal->DrawCircle( c->GetCenter(), ( halfWidth + c->GetRadius() + h->GetRadius() ) / 2 );

            holeDrawn = true;
        }
        else
        {
            gal->SetIsStroke( m_width ? true : false );
            gal->SetLineWidth( m_width );
            gal->SetFillColor( m_color );
            gal->DrawCircle( c->GetCenter(), c->GetRadius() );
        }

        break;
    }

    case SH_RECT:
    {
        const SHAPE_RECT* r = static_cast<const SHAPE_RECT*>( aShape );

        gal->SetFillColor( m_color );
        gal->SetLayerDepth( m_depth );
        gal->SetIsStroke( m_width ? true : false );
        gal->SetLineWidth( m_width );
        gal->SetStrokeColor( m_color );
        gal->DrawRectangle( r->GetPosition(), r->GetPosition() + r->GetSize() );
        break;
    }

    case SH_SIMPLE:
    {
        gal->SetFillColor( m_color );
        gal->SetLayerDepth( m_depth );
        gal->SetIsStroke( m_width ? true : false );
        gal->SetLineWidth( m_width );
        gal->SetStrokeColor( m_color );
        gal->DrawPolygon( static_cast<const SHAPE_SIMPLE*>( aShape )->Vertices() );
        break;
    }

    case SH_ARC:
    {
        const SHAPE_ARC* arc = static_cast<const SHAPE_ARC*>( aShape );

        gal->SetIsFill( false );
        gal->SetIsStroke( true );
        gal->SetLayerDepth( m_depth );
        gal->SetStrokeColor( m_color );
        gal->SetFillColor( m_color );
        gal->SetLineWidth( arc->GetWidth() );
        gal->DrawArc( arc->GetCenter(), arc->GetRadius(), arc->GetStartAngle(),
                      arc->GetCentralAngle() );
        break;
    }

    case SH_ELLIPSE:
    {
        const SHAPE_ELLIPSE* ellipse = static_cast<const SHAPE_ELLIPSE*>( aShape );
        const VECTOR2I       center = ellipse->GetCenter();
        const int            major = ellipse->GetMajorRadius();
        const int            minor = ellipse->GetMinorRadius();
        const EDA_ANGLE      rotation = ellipse->GetRotation();
        const bool           isArc = ellipse->IsArc();
        const int            w = std::max( 1, ellipse->GetWidth() );

        gal->SetIsFill( false );
        gal->SetIsStroke( true );
        gal->SetLayerDepth( m_depth );
        gal->SetStrokeColor( m_color );
        gal->SetFillColor( m_color );
        gal->SetLineWidth( w );

        if( isArc )
            gal->DrawEllipseArc( center, major, minor, rotation, ellipse->GetStartAngle(), ellipse->GetEndAngle() );
        else
            gal->DrawEllipse( center, major, minor, rotation );

        break;
    }

    case SH_COMPOUND:
        wxFAIL_MSG( wxT( "Router preview item: nested compound shapes not supported" ) );
        break;

    case SH_POLY_SET:
        wxFAIL_MSG( wxT( "Router preview item: SHAPE_POLY_SET not supported" ) );
        break;

    case SH_NULL:
        break;
    }

    if( m_hole && !holeDrawn )
    {
        gal->SetLayerDepth( m_depth );
        gal->SetIsStroke( true );
        gal->SetIsFill( false );
        gal->SetStrokeColor( m_color );
        gal->SetLineWidth( 1 );

        SHAPE_CIRCLE*  circle = dynamic_cast<SHAPE_CIRCLE*>( m_hole );
        SHAPE_SEGMENT* slot = dynamic_cast<SHAPE_SEGMENT*>( m_hole );

        if( circle )
            gal->DrawCircle( circle->GetCenter(), circle->GetRadius() );
        else if( slot )
            gal->DrawSegment( slot->GetSeg().A, slot->GetSeg().B, slot->GetWidth() );
    }
}


void ROUTER_PREVIEW_ITEM::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    GAL* gal = aView->GetGAL();

    if( m_type != PR_SHAPE || !m_shape )
        return;

    std::vector<const SHAPE*> shapes;

    if( m_shape->HasIndexableSubshapes() )
        m_shape->GetIndexableSubshapes( shapes );
    else
        shapes.push_back( m_shape );

    if( m_mode == DM_SHADOW )
    {
        //TODO(snh) Add configuration option for the color/alpha here
        gal->SetStrokeColor( COLOR4D( DARKDARKGRAY ).WithAlpha( 0.9 ) );
        gal->SetFillColor( COLOR4D( DARKDARKGRAY ).WithAlpha( 0.7 ) );
        gal->SetIsStroke( m_width ? true : false );
        gal->SetIsFill( true );

        for( const SHAPE* shape : shapes )
            drawClearanceHalo( shape, gal );

        return;
    }

    gal->SetLayerDepth( m_depth );
    gal->SetStrokeColor( m_color );
    gal->SetFillColor( m_color );
    gal->SetIsStroke( m_width ? true : false );
    gal->SetIsFill( true );

    // Semi-solids (ie: rule areas) which are not in collision are sketched (ie: outline only)
    if( ( m_flags & PNS_SEMI_SOLID ) > 0 && ( m_flags & PNS_COLLISION ) == 0 )
        gal->SetIsFill( false );

    for( const SHAPE* shape : shapes )
        drawShape( shape, gal );
}


const COLOR4D ROUTER_PREVIEW_ITEM::getLayerColor( int aLayer, const PNS::ITEM* aItem ) const
{
    auto settings = static_cast<PCB_RENDER_SETTINGS*>( m_view->GetPainter()->GetSettings() );

    COLOR4D color = settings->GetLayerColor( aLayer );

    if( aItem && aItem->Net() && settings->GetNetColorMode() == NET_COLOR_MODE::ALL && IsCopperLayer( aLayer ) )
    {
        NETINFO_ITEM* ni = static_cast<NETINFO_ITEM*>( aItem->Net() );

        auto ii = settings->GetNetColorMap().find( ni->GetNetCode() );

        if( ii != settings->GetNetColorMap().end() && ii->second != COLOR4D::UNSPECIFIED )
        {
            color = ii->second;
        }
        else
        {
            NETCLASS* nc = ni->GetNetClass();

            if( nc && nc->HasPcbColor() )
                color = nc->GetPcbColor();
        }
    }

    if( m_flags & PNS_HEAD_TRACE )
        return color.Saturate( 1.0 );
    else if( m_flags & PNS_HOVER_ITEM )
        return color.Brightened( 0.7 );

    return color;
}
