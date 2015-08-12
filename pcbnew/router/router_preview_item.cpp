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

#include <deque>
#include <gal/color4d.h>

#include <geometry/shape_rect.h>
#include <geometry/shape_convex.h>

#include "class_track.h"
#include <pcb_painter.h>
#include <colors.h>

#include "router_preview_item.h"

#include "pns_line.h"
#include "pns_segment.h"
#include "pns_via.h"

using namespace KIGFX;

ROUTER_PREVIEW_ITEM::ROUTER_PREVIEW_ITEM( const PNS_ITEM* aItem, VIEW_GROUP* aParent ) :
    EDA_ITEM( NOT_USED )
{
    m_parent = aParent;

    m_shape = NULL;
    m_clearance = -1;
    m_originLayer = m_layer = ITEM_GAL_LAYER( GP_OVERLAY );

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


void ROUTER_PREVIEW_ITEM::Update( const PNS_ITEM* aItem )
{
    m_originLayer = aItem->Layers().Start();

    if( aItem->OfKind( PNS_ITEM::LINE ) )
    {
        const PNS_LINE* l = static_cast<const PNS_LINE*>( aItem );

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
    case PNS_ITEM::LINE:
        m_type  = PR_SHAPE;
        m_width = ( (PNS_LINE*) aItem )->Width();
        break;

    case PNS_ITEM::SEGMENT:
    {
        PNS_SEGMENT* seg = (PNS_SEGMENT*) aItem;
        m_type  = PR_SHAPE;
        m_width = seg->Width();
        break;
    }

    case PNS_ITEM::VIA:
        m_originLayer = m_layer = ITEM_GAL_LAYER( VIAS_VISIBLE );
        m_type = PR_SHAPE;
        m_width = 0;
        m_color = COLOR4D( 0.7, 0.7, 0.7, 0.8 );
        m_depth = ViaOverlayDepth;
        break;

    case PNS_ITEM::SOLID:
        m_type = PR_SHAPE;
        m_width = 0;
        break;

    default:
        break;
    }

    if( aItem->Marker() & MK_VIOLATION )
        m_color = COLOR4D( 0, 1, 0, 1 );

    if( aItem->Marker() & MK_HEAD )
        m_color.Brighten( 0.7 );

    ViewSetVisible( true );
    ViewUpdate( GEOMETRY | APPEARANCE );
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


void ROUTER_PREVIEW_ITEM::drawLineChain( const SHAPE_LINE_CHAIN& aL, KIGFX::GAL* aGal ) const
{
    for( int s = 0; s < aL.SegmentCount(); s++ )
        aGal->DrawLine( aL.CSegment( s ).A, aL.CSegment( s ).B );

    if( aL.IsClosed() )
        aGal->DrawLine( aL.CSegment( -1 ).B, aL.CSegment( 0 ).A );
}


void ROUTER_PREVIEW_ITEM::ViewDraw( int aLayer, KIGFX::GAL* aGal ) const
{
    //col.Brighten(0.7);
    aGal->SetLayerDepth( m_depth );

    if( m_type == PR_SHAPE )
    {
        if( !m_shape )
            return;

        aGal->SetLineWidth( m_width );
        aGal->SetStrokeColor( m_color );
        aGal->SetFillColor( m_color );
        aGal->SetIsStroke( m_width ? true : false );
        aGal->SetIsFill( true );

        switch( m_shape->Type() )
        {
            case SH_LINE_CHAIN:
            {
                const SHAPE_LINE_CHAIN* l = (const SHAPE_LINE_CHAIN*) m_shape;
                drawLineChain( *l, aGal );
                break;
            }

            case SH_SEGMENT:
            {
                const SHAPE_SEGMENT* s = (const SHAPE_SEGMENT*) m_shape;
                aGal->DrawSegment( s->GetSeg().A, s->GetSeg().B, s->GetWidth() );

                if( m_clearance > 0 )
                {
                    aGal->SetLayerDepth( ClearanceOverlayDepth );
                    aGal->SetStrokeColor( COLOR4D( DARKDARKGRAY ) );
                    aGal->SetFillColor( COLOR4D( DARKDARKGRAY ) );
                    aGal->DrawSegment( s->GetSeg().A, s->GetSeg().B, s->GetWidth() + 2 * m_clearance );
                }

                break;
            }

            case SH_CIRCLE:
            {
                const SHAPE_CIRCLE* c = (const SHAPE_CIRCLE*) m_shape;
                aGal->DrawCircle( c->GetCenter(), c->GetRadius() );

                if( m_clearance > 0 )
                {
                    aGal->SetLayerDepth( ClearanceOverlayDepth );
                    aGal->SetFillColor( COLOR4D( DARKDARKGRAY ) );
                    aGal->SetIsStroke( false );
                    aGal->DrawCircle( c->GetCenter(), c->GetRadius() + m_clearance );
                }

                break;
            }

            case SH_RECT:
            {
                const SHAPE_RECT* r = (const SHAPE_RECT*) m_shape;
                aGal->DrawRectangle( r->GetPosition(), r->GetPosition() + r->GetSize() );

                if( m_clearance > 0 )
                {
                    aGal->SetLayerDepth( ClearanceOverlayDepth );
                    VECTOR2I p0( r->GetPosition() ), s( r->GetSize() );
                    aGal->SetStrokeColor( COLOR4D( DARKDARKGRAY ) );
                    aGal->SetIsStroke( true );
                    aGal->SetLineWidth( 2 * m_clearance );
                    aGal->DrawLine( p0, VECTOR2I( p0.x + s.x, p0.y ) );
                    aGal->DrawLine( p0, VECTOR2I( p0.x, p0.y + s.y ) );
                    aGal->DrawLine( p0 + s , VECTOR2I( p0.x + s.x, p0.y ) );
                    aGal->DrawLine( p0 + s, VECTOR2I( p0.x, p0.y + s.y ) );
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
            aGal->DrawPolygon( polygon );

            if( m_clearance > 0 )
            {
                aGal->SetLayerDepth( ClearanceOverlayDepth );
                aGal->SetStrokeColor( COLOR4D( DARKDARKGRAY ) );
                aGal->SetIsStroke( true );
                aGal->SetLineWidth( 2 * m_clearance );
                // need the implicit last segment to be explicit for DrawPolyline
                polygon.push_back( c->CDPoint( 0 ) );
                aGal->DrawPolyline( polygon );
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

    ViewSetVisible( true );
    ViewUpdate( GEOMETRY | APPEARANCE );
}


void ROUTER_PREVIEW_ITEM::Point( const VECTOR2I& aPos, int aStyle )
{
}


void ROUTER_PREVIEW_ITEM::Box( const BOX2I& aBox, int aStyle )
{
}


const COLOR4D ROUTER_PREVIEW_ITEM::getLayerColor( int aLayer ) const
{
    PCB_RENDER_SETTINGS* settings =
        static_cast<PCB_RENDER_SETTINGS*>( m_parent->GetView()->GetPainter()->GetSettings() );

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

        break;
    }

    return color;
}

const int ROUTER_PREVIEW_ITEM::ClearanceOverlayDepth = -200;
const int ROUTER_PREVIEW_ITEM::BaseOverlayDepth = -210;
const int ROUTER_PREVIEW_ITEM::ViaOverlayDepth = -246;
