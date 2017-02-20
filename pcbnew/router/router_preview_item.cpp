/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <geometry/shape_convex.h>

#include "class_track.h"
#include <pcb_painter.h>

#include "router_preview_item.h"

#include "pns_line.h"
#include "pns_segment.h"
#include "pns_via.h"

using namespace KIGFX;

ROUTER_PREVIEW_ITEM::ROUTER_PREVIEW_ITEM( const PNS::ITEM* aItem, KIGFX::VIEW* aView ) :
    EDA_ITEM( NOT_USED )
{
    m_view = aView;

    m_shape = NULL;
    m_clearance = -1;
    m_originLayer = m_layer = ITEM_GAL_LAYER( GP_OVERLAY );

    m_showTrackClearance = false;
    m_showViaClearance = false;

    // initialize variables, overwritten by Update( aItem ), if aItem != NULL
    m_router = NULL;
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
}


void ROUTER_PREVIEW_ITEM::Update( const PNS::ITEM* aItem )
{
    m_originLayer = aItem->Layers().Start();

    if( aItem->OfKind( PNS::ITEM::LINE_T ) )
    {
        const PNS::LINE* l = static_cast<const PNS::LINE*>( aItem );

        if( !l->SegmentCount() )
            return;
    }

    assert( m_originLayer >= 0 );

    m_layer = m_originLayer;
    m_color = getLayerColor( m_originLayer );
    m_color.a = 0.8;
    m_depth = BaseOverlayDepth - aItem->Layers().Start();
    m_shape  = aItem->Shape()->Clone();

    switch( aItem->Kind() )
    {
    case PNS::ITEM::LINE_T:
        m_type  = PR_SHAPE;
        m_width = ( (PNS::LINE*) aItem )->Width();
        break;

    case PNS::ITEM::SEGMENT_T:
    {
        PNS::SEGMENT* seg = (PNS::SEGMENT*) aItem;
        m_type  = PR_SHAPE;
        m_width = seg->Width();
        break;
    }

    case PNS::ITEM::VIA_T:
        m_originLayer = m_layer = ITEM_GAL_LAYER( VIAS_VISIBLE );
        m_type = PR_SHAPE;
        m_width = 0;
        m_color = COLOR4D( 0.7, 0.7, 0.7, 0.8 );
        m_depth = ViaOverlayDepth;
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
        bbox = m_shape->BBox();
        bbox.Inflate( m_width / 2 );
        return bbox;

    case PR_POINT:
        bbox = BOX2I ( m_pos - VECTOR2I( 100000, 100000 ), VECTOR2I( 200000, 200000 ) );
        return bbox;

    default:
        break;
    }

    return bbox;
}


void ROUTER_PREVIEW_ITEM::drawLineChain( const SHAPE_LINE_CHAIN& aL, KIGFX::GAL* gal ) const
{
    for( int s = 0; s < aL.SegmentCount(); s++ )
        gal->DrawLine( aL.CSegment( s ).A, aL.CSegment( s ).B );

    if( aL.IsClosed() )
        gal->DrawLine( aL.CSegment( -1 ).B, aL.CSegment( 0 ).A );
}


void ROUTER_PREVIEW_ITEM::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    auto gal = aView->GetGAL();
    //col.Brighten(0.7);
    gal->SetLayerDepth( m_depth );

    if( m_type == PR_SHAPE )
    {
        if( !m_shape )
            return;

        gal->SetLineWidth( m_width );
        gal->SetStrokeColor( m_color );
        gal->SetFillColor( m_color );
        gal->SetIsStroke( m_width ? true : false );
        gal->SetIsFill( true );

        switch( m_shape->Type() )
        {
        case SH_LINE_CHAIN:
        {
            const SHAPE_LINE_CHAIN* l = (const SHAPE_LINE_CHAIN*) m_shape;
            drawLineChain( *l, gal );

            if( m_showTrackClearance && m_clearance > 0 )
            {
                gal->SetLayerDepth( ClearanceOverlayDepth );
                gal->SetStrokeColor( COLOR4D( DARKDARKGRAY ) );
                gal->SetFillColor( COLOR4D( DARKDARKGRAY ) );
                gal->SetLineWidth( m_width + 2 * m_clearance );
                drawLineChain( *l, gal );
            }
            break;
        }

        case SH_SEGMENT:
        {
            const SHAPE_SEGMENT* s = (const SHAPE_SEGMENT*) m_shape;
            gal->DrawSegment( s->GetSeg().A, s->GetSeg().B, s->GetWidth() );

            if( m_showTrackClearance && m_clearance > 0 )
            {
                gal->SetLayerDepth( ClearanceOverlayDepth );
                gal->SetStrokeColor( COLOR4D( DARKDARKGRAY ) );
                gal->SetFillColor( COLOR4D( DARKDARKGRAY ) );
                gal->DrawSegment( s->GetSeg().A, s->GetSeg().B, s->GetWidth() + 2 * m_clearance );
            }

            break;
        }

        case SH_CIRCLE:
        {
            const SHAPE_CIRCLE* c = (const SHAPE_CIRCLE*) m_shape;
            gal->DrawCircle( c->GetCenter(), c->GetRadius() );

            if( m_showViaClearance && m_clearance > 0 )
            {
                gal->SetLayerDepth( ClearanceOverlayDepth );
                gal->SetFillColor( COLOR4D( DARKDARKGRAY ) );
                gal->SetIsStroke( false );
                gal->DrawCircle( c->GetCenter(), c->GetRadius() + m_clearance );
            }

            break;
        }

        case SH_RECT:
        {
            const SHAPE_RECT* r = (const SHAPE_RECT*) m_shape;
            gal->DrawRectangle( r->GetPosition(), r->GetPosition() + r->GetSize() );

            if( m_clearance > 0 )
            {
                gal->SetLayerDepth( ClearanceOverlayDepth );
                VECTOR2I p0( r->GetPosition() ), s( r->GetSize() );
                gal->SetStrokeColor( COLOR4D( DARKDARKGRAY ) );
                gal->SetIsStroke( true );
                gal->SetLineWidth( 2 * m_clearance );
                gal->DrawLine( p0, VECTOR2I( p0.x + s.x, p0.y ) );
                gal->DrawLine( p0, VECTOR2I( p0.x, p0.y + s.y ) );
                gal->DrawLine( p0 + s , VECTOR2I( p0.x + s.x, p0.y ) );
                gal->DrawLine( p0 + s, VECTOR2I( p0.x, p0.y + s.y ) );
            }

            break;
        }

    case SH_CONVEX:
    {
        const SHAPE_CONVEX* c = (const SHAPE_CONVEX*) m_shape;
        std::deque<VECTOR2D> polygon = std::deque<VECTOR2D>();
        for( int i = 0; i < c->PointCount(); i++ )
        {
            polygon.push_back( c->CDPoint( i ) );
        }
        gal->DrawPolygon( polygon );

        if( m_clearance > 0 )
        {
            gal->SetLayerDepth( ClearanceOverlayDepth );
            gal->SetStrokeColor( COLOR4D( DARKDARKGRAY ) );
            gal->SetIsStroke( true );
            gal->SetLineWidth( 2 * m_clearance );
            // need the implicit last segment to be explicit for DrawPolyline
            polygon.push_back( c->CDPoint( 0 ) );
            gal->DrawPolyline( polygon );
        }
        break;
    }

    case SH_POLY_SET:
    case SH_COMPOUND:
        break;          // Not yet in use
        }
    }
}


void ROUTER_PREVIEW_ITEM::Line( const SHAPE_LINE_CHAIN& aLine, int aWidth, int aStyle )
{
    m_originLayer = m_layer = 0;
    m_width = aWidth;
    m_color = assignColor( aStyle );
    m_type = PR_SHAPE;
    m_depth = -1024;        // TODO gal->GetMinDepth()
    m_shape = aLine.Clone();
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
    case 0:
        color = COLOR4D( 0, 1, 0, 1 ); break;

    case 1:
        color = COLOR4D( 1, 0, 0, 1 ); break;

    case 2:
        color = COLOR4D( 1, 1, 0, 1 ); break;

    case 3:
        color = COLOR4D( 0, 0, 1, 1 ); break;

    case 4:
        color = COLOR4D( 1, 1, 1, 1 ); break;

    case 5:
        color = COLOR4D( 1, 1, 0, 1 ); break;

    case 6:
        color = COLOR4D( 0, 1, 1, 1 ); break;

    case 32:
        color = COLOR4D( 0, 0, 1, 1 ); break;

    default:
        color = COLOR4D( 0.4, 0.4, 0.4, 1 ); break;
    }

    return color;
}

const int ROUTER_PREVIEW_ITEM::ClearanceOverlayDepth = -300;
const int ROUTER_PREVIEW_ITEM::BaseOverlayDepth = -310;
const int ROUTER_PREVIEW_ITEM::ViaOverlayDepth = -346;
