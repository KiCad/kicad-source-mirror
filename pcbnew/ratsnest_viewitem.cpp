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
#include <gal/graphics_abstraction_layer.h>
#include <pcb_painter.h>
#include <layers_id_colors_and_visibility.h>

#include <boost/foreach.hpp>

using namespace KIGFX;

RATSNEST_VIEWITEM::RATSNEST_VIEWITEM( RN_DATA* aData ) :
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


void RATSNEST_VIEWITEM::ViewDraw( int aLayer, GAL* aGal ) const
{
    aGal->SetIsStroke( true );
    aGal->SetIsFill( false );
    aGal->SetLineWidth( 1.0 );
    RENDER_SETTINGS* rs = m_view->GetPainter()->GetSettings();
    COLOR4D color = rs->GetColor( NULL, ITEM_GAL_LAYER( RATSNEST_VISIBLE ) );
    int highlightedNet = rs->GetHighlightNetCode();

    for( int i = 1; i < m_data->GetNetCount(); ++i )
    {
        RN_NET& net = m_data->GetNet( i );

        if( !net.IsVisible() )
            continue;

        // Set brighter color for the temporary ratsnest
        aGal->SetStrokeColor( color.Brightened( 0.8 ) );

        // Draw the "dynamic" ratsnest (i.e. for objects that may be currently being moved)
        BOOST_FOREACH( const RN_NODE_PTR& node, net.GetSimpleNodes() )
        {
            RN_NODE_PTR dest = net.GetClosestNode( node, WITHOUT_FLAG() );

            if( dest )
            {
                VECTOR2D origin( node->GetX(), node->GetY() );
                VECTOR2D end( dest->GetX(), dest->GetY() );

                aGal->DrawLine( origin, end );

                // Avoid duplicate destinations for ratsnest lines by storing already used nodes
                net.AddBlockedNode( dest );
            }
        }

        // Draw the "static" ratsnest
        if( i != highlightedNet )
            aGal->SetStrokeColor( color );  // using the default ratsnest color for not highlighted

        const std::vector<RN_EDGE_MST_PTR>* edges = net.GetUnconnected();

        if( edges == NULL )
            continue;

        BOOST_FOREACH( const RN_EDGE_MST_PTR& edge, *edges )
        {
            const RN_NODE_PTR& sourceNode = edge->GetSourceNode();
            const RN_NODE_PTR& targetNode = edge->GetTargetNode();
            VECTOR2D source( sourceNode->GetX(), sourceNode->GetY()  );
            VECTOR2D target( targetNode->GetX(), targetNode->GetY() );

            aGal->DrawLine( source, target );
        }
    }
}


void RATSNEST_VIEWITEM::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 1;
    aLayers[0] = ITEM_GAL_LAYER( RATSNEST_VISIBLE );
}
