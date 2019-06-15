/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright (C) 2018-2019 KiCad Developers, see AUTHORS.txt for contributors.
 *
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
#include <pcb_base_frame.h>

#include <memory>
#include <utility>

#include <view/view.h>

namespace KIGFX {

RATSNEST_VIEWITEM::RATSNEST_VIEWITEM(  std::shared_ptr<CONNECTIVITY_DATA> aData ) :
        EDA_ITEM( NOT_USED ), m_data( std::move(aData) )
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
    std::unique_lock<std::mutex> lock( m_data->GetLock(), std::try_to_lock );

    if( !lock )
        return;

    constexpr int CROSS_SIZE = 200000;

    auto gal = aView->GetGAL();
	gal->SetIsStroke( true );
    gal->SetIsFill( false );
    gal->SetLineWidth( 1.0 );
    auto rs = static_cast<PCB_RENDER_SETTINGS*>(aView->GetPainter()->GetSettings());
    auto color = rs->GetColor( NULL, LAYER_RATSNEST );

    int highlightedNet = rs->GetHighlightNetCode();

    gal->SetStrokeColor( color.Brightened(0.5) );

    const bool curved_ratsnest = rs->GetCurvedRatsnestLinesEnabled();

    // Draw the "dynamic" ratsnest (i.e. for objects that may be currently being moved)
    for( const auto& l : m_data->GetDynamicRatsnest() )
    {
        if ( l.a == l.b )
        {
            gal->DrawLine( VECTOR2I( l.a.x - CROSS_SIZE, l.a.y - CROSS_SIZE ),
                           VECTOR2I( l.b.x + CROSS_SIZE, l.b.y + CROSS_SIZE ) );
            gal->DrawLine( VECTOR2I( l.a.x - CROSS_SIZE, l.a.y + CROSS_SIZE ),
                           VECTOR2I( l.b.x + CROSS_SIZE, l.b.y - CROSS_SIZE ) );
        }
        else
        {
            if( curved_ratsnest )
            {
                auto dx = l.b.x - l.a.x;
                auto dy = l.b.y - l.a.y;
                const auto center = VECTOR2I( l.a.x + 0.5 * dx - 0.1 * dy,
                        l.a.y + 0.5 * dy + 0.1 * dx );
                gal->DrawCurve( l.a, center, center, l.b );
            }
            else
            {
                gal->DrawLine( l.a, l.b );
            }
        }
    }

    for( int i = 1 /* skip "No Net" at [0] */; i < m_data->GetNetCount(); ++i )
    {
        RN_NET* net = m_data->GetRatsnestForNet( i );

        if( !net )
            continue;

        // Draw the "static" ratsnest
        if( i != highlightedNet )
            gal->SetStrokeColor( color );  // using the default ratsnest color for not highlighted
        else
            gal->SetStrokeColor( color.Brightened(0.8) );

        for( const auto& edge : net->GetUnconnected() )
        {
            //if ( !edge.IsVisible() )
            //    continue;

            const auto& sourceNode = edge.GetSourceNode();
            const auto& targetNode = edge.GetTargetNode();
            const VECTOR2I source( sourceNode->Pos() );
            const VECTOR2I target( targetNode->Pos() );

            if( !sourceNode->Valid() || !targetNode->Valid() )
                continue;

            bool enable =  !sourceNode->GetNoLine() && !targetNode->GetNoLine();
            bool show;

            // If the global ratsnest is currently enabled, the local ratsnest
            // should be easy to turn off, so either element can disable it
            // If the global ratsnest is disabled, the local ratsnest should be easy to turn on
            // so either element can enable it.
            if( rs->GetGlobalRatsnestLinesEnabled() )
                show = sourceNode->Parent()->GetLocalRatsnestVisible() &&
                       targetNode->Parent()->GetLocalRatsnestVisible();
            else
                show = sourceNode->Parent()->GetLocalRatsnestVisible() ||
                       targetNode->Parent()->GetLocalRatsnestVisible();

            if ( enable && show )
            {
                if ( source == target )
                {
                    gal->DrawLine( VECTOR2I( source.x - CROSS_SIZE, source.y - CROSS_SIZE ),
                                   VECTOR2I( source.x + CROSS_SIZE, source.y + CROSS_SIZE ) );
                    gal->DrawLine( VECTOR2I( source.x - CROSS_SIZE, source.y + CROSS_SIZE ),
                                   VECTOR2I( source.x + CROSS_SIZE, source.y - CROSS_SIZE ) );
                }
                else
                {
                    if( curved_ratsnest )
                    {
                        auto dx = target.x - source.x;
                        auto dy = target.y - source.y;
                        const auto center = VECTOR2I( source.x + 0.5 * dx - 0.1 * dy,
                                source.y + 0.5 * dy + 0.1 * dx );
                        gal->DrawCurve( source, center, center, target );
                    }
                    else
                    {
                        gal->DrawLine( source, target );
                    }
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
