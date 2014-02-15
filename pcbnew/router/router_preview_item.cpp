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

#include <gal/color4d.h>

#include "class_track.h"
#include <pcb_painter.h>

#include "router_preview_item.h"

#include "pns_line.h"
#include "pns_segment.h"
#include "pns_via.h"

using namespace KIGFX;

ROUTER_PREVIEW_ITEM::ROUTER_PREVIEW_ITEM( const PNS_ITEM* aItem, VIEW_GROUP* aParent ) :
    EDA_ITEM( NOT_USED )
{
    m_Flags = 0;
    m_parent = aParent;
    m_layer = DRAW_N;

    if( aItem )
        Update( aItem );
}


ROUTER_PREVIEW_ITEM::~ROUTER_PREVIEW_ITEM()
{
}


void ROUTER_PREVIEW_ITEM::Update( const PNS_ITEM* aItem )
{
    m_layer = aItem->GetLayers().Start();

    m_color = getLayerColor( m_layer );
    m_color.a = 0.8;

    switch( aItem->GetKind() )
    {
    case PNS_ITEM::LINE:
        m_type  = PR_LINE;
        m_width = static_cast<const PNS_LINE*>(aItem)->GetWidth();
        m_line  = *static_cast<const SHAPE_LINE_CHAIN*>( aItem->GetShape() );
        break;

    case PNS_ITEM::SEGMENT:
        m_type  = PR_LINE;
        m_width = static_cast<const PNS_SEGMENT*>(aItem)->GetWidth();
        m_line  = *static_cast<const SHAPE_LINE_CHAIN*>( aItem->GetShape() );
        break;

    case PNS_ITEM::VIA:
        m_type  = PR_VIA;
        m_color = COLOR4D( 0.7, 0.7, 0.7, 0.8 );
        m_width = static_cast<const PNS_VIA*>(aItem)->GetDiameter();
        m_viaCenter = static_cast<const PNS_VIA*>(aItem)->GetPos();
        break;

    default:
        break;
    }

    ViewSetVisible( true );
    ViewUpdate( GEOMETRY | APPEARANCE );
}


void ROUTER_PREVIEW_ITEM::MarkAsHead()
{
    if( m_type != PR_VIA )
        m_color.Saturate( 1.0 );
}


const BOX2I ROUTER_PREVIEW_ITEM::ViewBBox() const
{
    BOX2I bbox;

    switch( m_type )
    {
    case PR_LINE:
        bbox = m_line.BBox();
        bbox.Inflate( m_width / 2 );
        return bbox;

    case PR_VIA:
        bbox = BOX2I( m_viaCenter, VECTOR2I( 0, 0 ) );
        bbox.Inflate( m_width / 2 );
        return bbox;

    default:
        break;
    }

    return bbox;
}


void ROUTER_PREVIEW_ITEM::ViewDraw( int aLayer, KIGFX::GAL* aGal ) const
{
    switch( m_type )
    {
    case PR_LINE:
        aGal->SetLayerDepth( -100.0 );
        aGal->SetLineWidth( m_width );
        aGal->SetStrokeColor( m_color );
        aGal->SetIsStroke( true );
        aGal->SetIsFill( false );

        for( int s = 0; s < m_line.SegmentCount(); s++ )
            aGal->DrawLine( m_line.CSegment( s ).A, m_line.CSegment( s ).B );

        if( m_line.IsClosed() )
            aGal->DrawLine( m_line.CSegment( -1 ).B, m_line.CSegment( 0 ).A );
        break;

    case PR_VIA:
        aGal->SetLayerDepth( -101.0 );
        aGal->SetIsStroke( false );
        aGal->SetIsFill( true );
        aGal->SetFillColor( m_color );
        aGal->DrawCircle( m_viaCenter, m_width / 2 );
        break;

    default:
        break;
    }
}


void ROUTER_PREVIEW_ITEM::DebugLine( const SHAPE_LINE_CHAIN& aLine, int aWidth, int aStyle  )
{
#if 0
    m_line  = aLine;
    m_width = aWidth;
    m_color = assignColor( aStyle );


    m_type = PR_LINE;
    ViewUpdate( GEOMETRY | APPEARANCE );
#endif
}


void ROUTER_PREVIEW_ITEM::DebugBox( const BOX2I& aBox, int aStyle )
{
#if 0
    assert( false );

    m_line.Clear();
    m_line.Append( aBox.GetX(), aBox.GetY() );
    m_line.Append( aBox.GetX() + aBox.GetWidth(), aBox.GetY() + aBox.GetHeight() );
    m_line.Append( aBox.GetX() + aBox.GetWidth(), aBox.GetY() + aBox.GetHeight() );
    m_line.Append( aBox.GetX(), aBox.GetY() + aBox.GetHeight() );
    m_line.SetClosed( true );
    m_width = 20000;
    m_color = assignColor( aStyle );
    m_type  = PR_LINE;
    ViewUpdate( GEOMETRY | APPEARANCE );
#endif
}


const COLOR4D ROUTER_PREVIEW_ITEM::getLayerColor( int aLayer ) const
{
    // assert (m_view != NULL);

    PCB_RENDER_SETTINGS* settings =
        static_cast <PCB_RENDER_SETTINGS*> ( m_parent->GetView()->GetPainter()->GetSettings() );

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
        color = COLOR4D( 1, 0, 0, 0.3 ); break;

    case 2:
        color = COLOR4D( 1, 0.5, 0.5, 1 ); break;

    case 3:
        color = COLOR4D( 0, 0, 1, 1 ); break;

    case 4:
        color = COLOR4D( 1, 1, 1, 1 ); break;

    case 5:
        color = COLOR4D( 1, 1, 0, 1 ); break;

    case 6:
        color = COLOR4D( 0, 1, 1, 1 ); break;

    case 32:
        color = COLOR4D( 0, 0, 1, 0.5 ); break;

    default:
        break;
    }

    return color;
}
