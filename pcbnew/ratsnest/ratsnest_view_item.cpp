/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * @brief Class that draws missing connections on a PCB.
 */

#include <ratsnest/ratsnest_view_item.h>

#include <kiface_base.h>
#include <gal/graphics_abstraction_layer.h>
#include <pcbnew_settings.h>
#include <pcb_painter.h>
#include <ratsnest/ratsnest_data.h>
#include <layer_ids.h>
#include <pcb_base_frame.h>
#include <view/view.h>

#include <memory>
#include <utility>


RATSNEST_VIEW_ITEM::RATSNEST_VIEW_ITEM( std::shared_ptr<CONNECTIVITY_DATA> aData ) :
        EDA_ITEM( NOT_USED ),
        m_data( std::move( aData ) )
{
}


const BOX2I RATSNEST_VIEW_ITEM::ViewBBox() const
{
    // Make it always visible
    BOX2I bbox;
    bbox.SetMaximum();

    return bbox;
}


void RATSNEST_VIEW_ITEM::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    std::unique_lock<KISPINLOCK> lock( m_data->GetLock(), std::try_to_lock );

    if( !lock )
        return;

    constexpr int CROSS_SIZE = 200000;

    PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() );

    if( !cfg )
        return;

    auto rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( aView->GetPainter()->GetSettings() );
    KIGFX::GAL* gal = aView->GetGAL();
    gal->SetIsStroke( true );
    gal->SetIsFill( false );
    gal->SetLineWidth( cfg->m_Display.m_RatsnestThickness / gal->GetWorldScale() );

    std::set<int>        highlightedNets = rs->GetHighlightNetCodes();
    const std::set<int>& hiddenNets      = rs->GetHiddenNets();

    COLOR4D    defaultColor = rs->GetColor( nullptr, LAYER_RATSNEST );
    COLOR4D    color = defaultColor;
    const bool colorByNet = rs->GetNetColorMode() != NET_COLOR_MODE::OFF;
    const bool dimStatic = m_data->GetLocalRatsnest().size() > 0 || highlightedNets.size() > 0;

    std::map<int, KIGFX::COLOR4D>& netColors = rs->GetNetColorMap();

    const bool onlyVisibleLayers = cfg->m_Display.m_RatsnestMode == RATSNEST_MODE::VISIBLE;
    LSET       visibleLayers;

    // If we are in "other layers off" mode, the active layer is the only visible layer
    if( rs->m_ContrastModeDisplay == HIGH_CONTRAST_MODE::HIDDEN )
    {
        if( rs->GetPrimaryHighContrastLayer() > UNDEFINED_LAYER )
            visibleLayers.set( rs->GetPrimaryHighContrastLayer() );
    }
    else
    {
        LSET::AllCuMask().RunOnLayers(
                [&]( PCB_LAYER_ID layer )
                {
                    if( aView->IsLayerVisible( layer ) )
                        visibleLayers.set( layer );
                } );
    }

    auto adjustColor =
            [&]( COLOR4D& color_base, double brightnessDelta, double alpha )
            {
                if( rs->GetColor( nullptr, LAYER_PCB_BACKGROUND ).GetBrightness() < 0.5 )
                    return color_base.Brightened( brightnessDelta ).WithAlpha( std::min( alpha, 1.0 ) );
                else
                    return color_base.Darkened( brightnessDelta ).WithAlpha( std::min( alpha, 1.0 ) );
            };

    const bool curved_ratsnest = cfg->m_Display.m_DisplayRatsnestLinesCurved;

    // Draw the "dynamic" ratsnest (i.e. for objects that may be currently being moved)
    for( const RN_DYNAMIC_LINE& l : m_data->GetLocalRatsnest() )
    {
        if( hiddenNets.count( l.netCode ) )
            continue;

        const NETCLASS*     nc = nullptr;
        const NET_SETTINGS* netSettings = m_data->GetNetSettings();

        if( m_data->HasNetNameForNetCode( l.netCode ) )
        {
            const wxString& netName = m_data->GetNetNameForNetCode( l.netCode );

            if( netSettings && netSettings->HasEffectiveNetClass( netName ) )
                nc = netSettings->GetCachedEffectiveNetClass( netName ).get();
        }

        if( colorByNet && netColors.count( l.netCode ) )
            color = netColors.at( l.netCode );
        else if( colorByNet && nc && nc->HasPcbColor() )
            color = nc->GetPcbColor();
        else
            color = defaultColor;

        if( color == COLOR4D::UNSPECIFIED )
            color = defaultColor;

        gal->SetStrokeColor( adjustColor( color, 0.5, color.a + 0.3 ) );

        if( l.a == l.b )
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
                int dx = l.b.x - l.a.x;
                int dy = l.b.y - l.a.y;
                const VECTOR2I center = VECTOR2I( l.a.x + 0.5 * dx - 0.1 * dy,
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
        if( hiddenNets.count( i ) )
            continue;

        RN_NET* net = m_data->GetRatsnestForNet( i );

        if( !net || m_data->GetConnectivityAlgo()->IsNetDirty( i ) )
            continue;

        const NETCLASS*     nc = nullptr;
        const NET_SETTINGS* netSettings = m_data->GetNetSettings();

        if( m_data->HasNetNameForNetCode( i ) )
        {
            const wxString& netName = m_data->GetNetNameForNetCode( i );

            if( netSettings && netSettings->HasEffectiveNetClass( netName ) )
                nc = netSettings->GetCachedEffectiveNetClass( netName ).get();
        }

        if( colorByNet && netColors.count( i ) )
            color = netColors.at( i );
        else if( colorByNet && nc && nc->HasPcbColor() )
            color = nc->GetPcbColor();
        else
            color = defaultColor;

        if( color == COLOR4D::UNSPECIFIED )
            color = defaultColor;

        if( dimStatic )
            color = adjustColor( color, 0.0, color.a / 2 );

        // Draw the "static" ratsnest
        if( highlightedNets.count( i ) )
            gal->SetStrokeColor( adjustColor( color, 0.8, color.a + 0.4 ) );
        else
            gal->SetStrokeColor( color );  // using the default ratsnest color for not highlighted

        for( const CN_EDGE& edge : net->GetEdges() )
        {
            if( !edge.IsVisible() )
                continue;

            const std::shared_ptr<const CN_ANCHOR>& sourceNode = edge.GetSourceNode();
            const std::shared_ptr<const CN_ANCHOR>& targetNode = edge.GetTargetNode();

            if( !sourceNode || sourceNode->Dirty() || !targetNode || targetNode->Dirty() )
                continue;

            const VECTOR2I source( sourceNode->Pos() );
            const VECTOR2I target( targetNode->Pos() );

            bool enable =  !sourceNode->GetNoLine() && !targetNode->GetNoLine();
            bool show;

            // If the global ratsnest is currently enabled, the local ratsnest should be easy to
            // turn off, so either element can disable it.
            // If the global ratsnest is disabled, the local ratsnest should be easy to turn on
            // so either element can enable it.
            if( cfg->m_Display.m_ShowGlobalRatsnest )
            {
                show = sourceNode->Parent()->GetLocalRatsnestVisible() &&
                       targetNode->Parent()->GetLocalRatsnestVisible();
            }
            else
            {
                show = sourceNode->Parent()->GetLocalRatsnestVisible() ||
                       targetNode->Parent()->GetLocalRatsnestVisible();
            }

            if( onlyVisibleLayers && show )
            {
                LSET sourceLayers = sourceNode->Parent()->GetLayerSet();
                LSET targetLayers = targetNode->Parent()->GetLayerSet();

                if( !( sourceLayers & visibleLayers ).any() ||
                    !( targetLayers & visibleLayers ).any() )
                {
                    show = false;
                }
            }

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
                        int dx = target.x - source.x;
                        int dy = target.y - source.y;
                        const VECTOR2I center = VECTOR2I( source.x + 0.5 * dx - 0.1 * dy,
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


std::vector<int> RATSNEST_VIEW_ITEM::ViewGetLayers() const
{
    return { LAYER_RATSNEST };
}

