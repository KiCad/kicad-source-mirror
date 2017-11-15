/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file ratsnest_viewitem.cpp
 * @brief Class that draws missing connections on a PCB.
 */

#include <ratsnest_viewitem.h>
#include <ratsnest_data.h>
#include <connectivity_data.h>
#include <gal/graphics_abstraction_layer.h>
#include <pcb_painter.h>
#include <layers_id_colors_and_visibility.h>

#include <memory>

#include <view/view.h>

namespace KIGFX {

RATSNEST_VIEWITEM::RATSNEST_VIEWITEM(  std::shared_ptr<CONNECTIVITY_DATA> aData ) :
        EDA_ITEM( NOT_USED ), m_data( aData )
{
}


const BOX2I RATSNEST_VIEWITEM::ViewBBox() const
{
    // Make it always visible
    BOX2I bbox;
    bbox.SetMaximum();

    return bbox;
}

void RATSNEST_VIEWITEM::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr int CROSS_SIZE = 200000;

    auto gal = aView->GetGAL();
	gal->SetIsStroke( true );
    gal->SetIsFill( false );
    gal->SetLineWidth( 1.0 );
    auto rs = aView->GetPainter()->GetSettings();
    auto color = rs->GetColor( NULL, LAYER_RATSNEST );

    int highlightedNet = rs->GetHighlightNetCode();

    gal->SetStrokeColor( color.Brightened(0.8) );

    // Draw the "dynamic" ratsnest (i.e. for objects that may be currently being moved)
    for( const auto& l : m_data->GetDynamicRatsnest() )
    {
        if ( l.a == l.b )
        {
            gal->DrawLine( VECTOR2I( l.a.x - CROSS_SIZE, l.a.y - CROSS_SIZE ), VECTOR2I( l.b.x + CROSS_SIZE, l.b.y + CROSS_SIZE ) );
            gal->DrawLine( VECTOR2I( l.a.x - CROSS_SIZE, l.a.y + CROSS_SIZE ), VECTOR2I( l.b.x + CROSS_SIZE, l.b.y - CROSS_SIZE ) );
        } else {
            gal->DrawLine( l.a, l.b );
        }
    }

    for( int i = 1; i < m_data->GetNetCount(); ++i )
    {
        RN_NET* net = m_data->GetRatsnestForNet( i );

        if( !net )
            continue;

        // Draw the "static" ratsnest
        if( i != highlightedNet )
            gal->SetStrokeColor( color );  // using the default ratsnest color for not highlighted

        for( const auto& edge : net->GetUnconnected() )
        {
            //if ( !edge.IsVisible() )
            //    continue;

            const auto& sourceNode = edge.GetSourceNode();
            const auto& targetNode = edge.GetTargetNode();
            const VECTOR2I source( sourceNode->Pos() );
            const VECTOR2I target( targetNode->Pos() );

            bool enable =  !sourceNode->GetNoLine() && !targetNode->GetNoLine();
            bool show = sourceNode->Parent()->GetLocalRatsnestVisible() || targetNode->Parent()->GetLocalRatsnestVisible();

            if ( enable && show )
            {
                if ( source == target )
                {
                    gal->DrawLine( VECTOR2I( source.x - CROSS_SIZE, source.y - CROSS_SIZE ), VECTOR2I( source.x + CROSS_SIZE, source.y + CROSS_SIZE ) );
                    gal->DrawLine( VECTOR2I( source.x - CROSS_SIZE, source.y + CROSS_SIZE ), VECTOR2I( source.x + CROSS_SIZE, source.y - CROSS_SIZE ) );
                }
                else
                {
                    gal->DrawLine( source, target );
                }
            }
        }
    }
}


void RATSNEST_VIEWITEM::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 1;
    aLayers[0] = LAYER_RATSNEST;
}

}
