/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <deque>
#include <gal/color4d.h>

#include <geometry/shape_rect.h>
#include <geometry/shape_simple.h>
#include <pcb_painter.h>
#include <trigo.h>

#include "router_preview_item.h"

#include "pns_arc.h"
#include "pns_line.h"
#include "pns_segment.h"
#include "pns_via.h"

using namespace KIGFX;


ROUTER_PREVIEW_ITEM::ROUTER_PREVIEW_ITEM( const PNS::ITEM* aItem, KIGFX::VIEW* aView ) :
    EDA_ITEM( NOT_USED )
{
    m_view = aView;

    m_shape = aItem ? aItem->Shape()->Clone() : nullptr;
    m_hole = aItem && aItem->Hole() ? aItem->Hole()->Clone() : nullptr;

    m_clearance = -1;
    m_originLayer = m_layer = LAYER_SELECT_OVERLAY ;

    m_showClearance = false;

    // initialize variables, overwritten by Update( aItem ), if aItem != NULL
    m_router = nullptr;
    m_type = PR_SHAPE;
    m_style = 0;
    m_width = 0;
    m_depth = 0;

    if( aItem )
        Update( aItem );
}


ROUTER_PREVIEW_ITEM::~ROUTER_PREVIEW_ITEM()
{
    delete m_shape;
    delete m_hole;
}


void ROUTER_PREVIEW_ITEM::Update( const PNS::ITEM* aItem )
{
    m_originLayer = aItem->Layers().Start();

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

    assert( m_originLayer >= 0 );

    m_layer = m_originLayer;
    m_color = getLayerColor( m_originLayer );
    m_color.a = 0.8;
    m_depth = BaseOverlayDepth - aItem->Layers().Start();

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
        m_originLayer = m_layer = LAYER_VIAS;
        m_type = PR_SHAPE;
        m_width = 0;
        m_color = COLOR4D( 0.7, 0.7, 0.7, 0.8 );
        m_depth = ViaOverlayDepth;

        delete m_shape;
        m_shape = nullptr;

        if( aItem->Shape() )
            m_shape = aItem->Shape()->Clone();

        delete m_hole;
        m_hole = nullptr;

        if( aItem->Hole() )
            m_hole = aItem->Hole()->Clone();

        break;

    case PNS::ITEM::SOLID_T:
        m_type = PR_SHAPE;
        m_width = 0;
        break;

    default:
        break;
    }

    if( aItem->Marker() & PNS::MK_VIOLATION )
        m_color = COLOR4D( 0, 1, 0, 1 );
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
    gal->SetIsFill( false );

    for( int s = 0; s < aL->GetSegmentCount(); s++ )
        gal->DrawLine( aL->GetSegment( s ).A, aL->GetSegment( s ).B );

    const SHAPE_LINE_CHAIN* lineChain = dynamic_cast<const SHAPE_LINE_CHAIN*>( aL );

    for( size_t s = 0; lineChain && s < lineChain->ArcCount(); s++ )
    {
        const SHAPE_ARC& arc = lineChain->CArcs()[s];

        double start_angle = DEG2RAD( arc.GetStartAngle() );
        double angle = DEG2RAD( arc.GetCentralAngle() );

        gal->DrawArc( arc.GetCenter(), arc.GetRadius(), start_angle, start_angle + angle);
    }

    if( aL->IsClosed() )
        gal->DrawLine( aL->GetSegment( -1 ).B, aL->GetSegment( 0 ).A );
}


void ROUTER_PREVIEW_ITEM::drawShape( const SHAPE* aShape, KIGFX::GAL* gal ) const
{
    bool holeDrawn = false;

    switch( aShape->Type() )
    {
    case SH_POLY_SET_TRIANGLE:
    {
        const SHAPE_LINE_CHAIN_BASE* l = (const SHAPE_LINE_CHAIN_BASE*) aShape;

        if( m_showClearance && m_clearance > 0 )
        {
            gal->SetLineWidth( m_width + 2 * m_clearance );
            drawLineChain( l, gal );
        }

        gal->SetLayerDepth( m_depth );
        gal->SetLineWidth( m_width );
        gal->SetStrokeColor( m_color );
        gal->SetFillColor( m_color );
        drawLineChain( l, gal );
        break;
    }

    case SH_LINE_CHAIN:
    {
        const SHAPE_LINE_CHAIN* l = (const SHAPE_LINE_CHAIN*) aShape;
        const int               w = m_width;

        if( m_showClearance && m_clearance > 0 )
        {
            gal->SetLineWidth( w + 2 * m_clearance );
            drawLineChain( l, gal );
        }

        gal->SetLayerDepth( m_depth );
        gal->SetLineWidth( w );
        gal->SetStrokeColor( m_color );
        gal->SetFillColor( m_color );
        drawLineChain( l, gal );
        break;
    }

    case SH_SEGMENT:
    {
        const SHAPE_SEGMENT* s = (const SHAPE_SEGMENT*) aShape;
        const int            w = s->GetWidth();

        gal->SetIsStroke( false );

        if( m_showClearance && m_clearance > 0 )
        {
            gal->SetLineWidth( w + 2 * m_clearance );
            gal->DrawSegment( s->GetSeg().A, s->GetSeg().B, s->GetWidth() + 2 * m_clearance );
        }

        gal->SetLayerDepth( m_depth );
        gal->SetLineWidth( w );
        gal->SetFillColor( m_color );
        gal->DrawSegment( s->GetSeg().A, s->GetSeg().B, s->GetWidth() );
        break;
    }

    case SH_CIRCLE:
    {
        const SHAPE_CIRCLE* c = static_cast<const SHAPE_CIRCLE*>( aShape );
        gal->SetStrokeColor( m_color );

        if( m_showClearance && m_clearance > 0 )
        {
            gal->SetIsStroke( false );
            gal->DrawCircle( c->GetCenter(), c->GetRadius() + m_clearance );
        }

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
        const SHAPE_RECT* r = (const SHAPE_RECT*) aShape;
        gal->SetFillColor( m_color );

        if( m_showClearance && m_clearance > 0 )
        {
            VECTOR2I p0( r->GetPosition() ), s( r->GetSize() );
            gal->SetIsStroke( true );
            gal->SetLineWidth( 2 * m_clearance );
            gal->DrawLine( p0, VECTOR2I( p0.x + s.x, p0.y ) );
            gal->DrawLine( p0, VECTOR2I( p0.x, p0.y + s.y ) );
            gal->DrawLine( p0 + s , VECTOR2I( p0.x + s.x, p0.y ) );
            gal->DrawLine( p0 + s, VECTOR2I( p0.x, p0.y + s.y ) );
        }

        gal->SetLayerDepth( m_depth );
        gal->SetIsStroke( m_width ? true : false );
        gal->SetLineWidth( m_width);
        gal->SetStrokeColor( m_color );
        gal->DrawRectangle( r->GetPosition(), r->GetPosition() + r->GetSize() );

        break;
    }

    case SH_SIMPLE:
    {
        const SHAPE_SIMPLE*  c = (const SHAPE_SIMPLE*) aShape;
        std::deque<VECTOR2D> polygon = std::deque<VECTOR2D>();

        for( int i = 0; i < c->PointCount(); i++ )
        {
            polygon.push_back( c->CDPoint( i ) );
        }

        gal->SetFillColor( m_color );

        if( m_showClearance && m_clearance > 0 )
        {
            gal->SetIsStroke( true );
            gal->SetLineWidth( 2 * m_clearance );

            // need the implicit last segment to be explicit for DrawPolyline
            polygon.push_back( c->CDPoint( 0 ) );
            gal->DrawPolyline( polygon );
        }

        gal->SetLayerDepth( m_depth );
        gal->SetIsStroke( m_width ? true : false );
        gal->SetLineWidth( m_width );
        gal->SetStrokeColor( m_color );
        gal->DrawPolygon( polygon );
        break;
    }

    case SH_ARC:
    {
        const SHAPE_ARC* arc = static_cast<const SHAPE_ARC*>( aShape );
        const int        w = arc->GetWidth();

        auto start_angle = DEG2RAD( arc->GetStartAngle() );
        auto angle = DEG2RAD( arc->GetCentralAngle() );

        gal->SetIsFill( false );
        gal->SetIsStroke( true );

        if( m_showClearance && m_clearance > 0 )
        {
            gal->SetLineWidth( w + 2 * m_clearance );
            gal->DrawArc( arc->GetCenter(), arc->GetRadius(), start_angle, start_angle + angle );
        }

        gal->SetLayerDepth( m_depth );
        gal->SetStrokeColor( m_color );
        gal->SetFillColor( m_color );
        gal->SetLineWidth( w );
        gal->DrawArc( arc->GetCenter(), arc->GetRadius(), start_angle, start_angle + angle );
        break;
    }

    case SH_COMPOUND:
        wxFAIL_MSG( "Router preview item: nested compound shapes not supported" );
        break;

    case SH_POLY_SET:
        wxFAIL_MSG( "Router preview item: SHAPE_POLY_SET not supported" );
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
    //col.Brighten(0.7);

    if( m_type == PR_SHAPE )
    {
        if( !m_shape )
            return;

        // N.B. The order of draw here is important
        // Cairo doesn't current support z-ordering, so we need
        // to draw the clearance first to ensure it is in the background
        gal->SetLayerDepth( ClearanceOverlayDepth );

        //TODO(snh) Add configuration option for the color/alpha here
        gal->SetStrokeColor( COLOR4D( DARKDARKGRAY ).WithAlpha( 0.9 ) );
        gal->SetFillColor( COLOR4D( DARKDARKGRAY ).WithAlpha( 0.7 ) );
        gal->SetIsStroke( m_width ? true : false );
        gal->SetIsFill( true );

        if( m_shape->HasIndexableSubshapes() )
        {
            std::vector<SHAPE*> subshapes;
            m_shape->GetIndexableSubshapes( subshapes );

            for( SHAPE* shape : subshapes )
                drawShape( shape, gal );
        }
        else
        {
            drawShape( m_shape, gal );
        }
    }
}


void ROUTER_PREVIEW_ITEM::Line( const SHAPE_LINE_CHAIN& aLine, int aWidth, int aStyle )
{
    m_width = aWidth;

    if( aStyle >= 0 )
        m_color = assignColor( aStyle );

    m_type = PR_SHAPE;
    m_depth = -1024;        // TODO gal->GetMinDepth()

    SHAPE_LINE_CHAIN *lc = static_cast<SHAPE_LINE_CHAIN*>( aLine.Clone() );
    lc->SetWidth( aWidth );
    m_shape = lc;
}


void ROUTER_PREVIEW_ITEM::Point( const VECTOR2I& aPos, int aStyle )
{
}


void ROUTER_PREVIEW_ITEM::Box( const BOX2I& aBox, int aStyle )
{
}


const COLOR4D ROUTER_PREVIEW_ITEM::getLayerColor( int aLayer ) const
{
    auto settings = static_cast<PCB_RENDER_SETTINGS*>( m_view->GetPainter()->GetSettings() );

    return settings->GetLayerColor( aLayer );
}


const COLOR4D ROUTER_PREVIEW_ITEM::assignColor( int aStyle ) const
{
    COLOR4D color;

    switch( aStyle )
    {
    case 0:  color = COLOR4D( 0, 1, 0, 1 );       break;
    case 1:  color = COLOR4D( 1, 0, 0, 1 );       break;
    case 2:  color = COLOR4D( 1, 1, 0, 1 );       break;
    case 3:  color = COLOR4D( 0, 0, 1, 1 );       break;
    case 4:  color = COLOR4D( 1, 1, 1, 1 );       break;
    case 5:  color = COLOR4D( 1, 1, 0, 1 );       break;
    case 6:  color = COLOR4D( 0, 1, 1, 1 );       break;
    case 32: color = COLOR4D( 0, 0, 1, 1 );       break;
    default: color = COLOR4D( 0.4, 0.4, 0.4, 1 ); break;
    }

    return color;
}

const int ROUTER_PREVIEW_ITEM::ClearanceOverlayDepth = -VIEW::VIEW_MAX_LAYERS - 10;
const int ROUTER_PREVIEW_ITEM::BaseOverlayDepth = -VIEW::VIEW_MAX_LAYERS - 20;
const int ROUTER_PREVIEW_ITEM::ViaOverlayDepth = -VIEW::VIEW_MAX_LAYERS - 50;
