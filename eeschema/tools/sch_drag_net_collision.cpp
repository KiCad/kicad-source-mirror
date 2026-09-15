/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2025 VUT Brno, Faculty of Electrical Engineering and Communication
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "sch_drag_net_collision.h"

#include <trace_helpers.h>
#include <advanced_config.h>
#include <connectivity/conn_preview.h>
#include <schematic.h>
#include <sch_line.h>

#include <algorithm>
#include <limits>

#include <eda_item.h>
#include <sch_connection.h>
#include <sch_edit_frame.h>
#include <sch_item.h>
#include <sch_junction.h>
#include <sch_screen.h>
#include <sch_selection.h>
#include <sch_sheet_path.h>
#include <view/view.h>
#include <view/view_overlay.h>
#include <gal/color4d.h>
#include <layer_ids.h>
#include <settings/color_settings.h>
#include <eeschema_settings.h>

SCH_DRAG_NET_COLLISION_MONITOR::SCH_DRAG_NET_COLLISION_MONITOR( SCH_EDIT_FRAME* aFrame,
                                                                KIGFX::VIEW* aView ) :
        m_frame( aFrame ),
        m_view( aView ),
        m_overlay(),
        m_itemNetCodes(),
        m_sheetPath(),
        m_originalConnections(),
        m_hasCollision( false )
{
}


SCH_DRAG_NET_COLLISION_MONITOR::~SCH_DRAG_NET_COLLISION_MONITOR()
{
    Reset();
}


void SCH_DRAG_NET_COLLISION_MONITOR::Initialize( const SCH_SELECTION& aSelection )
{
    wxLogTrace( traceSchDragNetCollision, "Initialize: Starting initialization" );

    m_itemNetCodes.clear();
    m_originalConnections.clear();
    m_sheetPath = m_frame->GetCurrentSheet();
    m_hasCollision = false;

    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        m_itemNetCodes = SCH_CONNECTIVITY::CapturePreviewNetCodes( m_frame->Schematic().Connectivity(), m_sheetPath );
    }
    else
    {
        const auto record = [&]( SCH_ITEM* item )
        {
            recordItemNet( item );
            item->RunOnChildren( [&]( SCH_ITEM* child ) { recordItemNet( child ); }, RECURSE_MODE::NO_RECURSE );
        };

        for( SCH_ITEM* item : m_frame->GetScreen()->Items() )
            record( item );

        for( EDA_ITEM* item : aSelection )
            record( static_cast<SCH_ITEM*>( item ) );
    }

    recordOriginalConnections( aSelection );
}


bool SCH_DRAG_NET_COLLISION_MONITOR::Update( const std::vector<SCH_JUNCTION*>& aJunctions,
                                              const SCH_SELECTION& aSelection )
{
    std::vector<COLLISION_MARKER> markers;

    for( SCH_JUNCTION* junction : aJunctions )
    {
        if( auto marker = analyzeJunction( junction, aSelection ) )
            markers.push_back( *marker );
    }

    const auto disconnections = collectDisconnectedMarkers( aSelection );

    if( markers.empty() && disconnections.empty() )
    {
        clearOverlay();
        m_hasCollision = false;
        return false;
    }

    wxLogTrace( traceSchDragNetCollision, "Update: Drawing %zu collision markers and %zu disconnection markers",
                markers.size(), disconnections.size() );

    ensureOverlay();
    m_overlay->Clear();

    COLOR4D baseColor( 1.0, 0.0, 0.0, 0.8 );

    if( COLOR_SETTINGS* colorSettings = m_frame->GetColorSettings() )
    {
        COLOR4D themeColor = colorSettings->GetColor( LAYER_DRAG_NET_COLLISION );

        if( themeColor != COLOR4D::UNSPECIFIED )
            baseColor = themeColor;
    }

    double baseAlpha = baseColor.a;

    if( baseAlpha <= 0.0 )
        baseAlpha = 1.0;

    double fillAlpha = std::clamp( baseAlpha * 0.35, 0.05, 1.0 );
    double strokeAlpha = std::clamp( baseAlpha, 0.05, 1.0 );

    m_overlay->SetIsFill( true );
    m_overlay->SetFillColor( baseColor.WithAlpha( fillAlpha ) );
    m_overlay->SetIsStroke( true );
    m_overlay->SetStrokeColor( baseColor.WithAlpha( strokeAlpha ) );

    int lineWidthPixels = 4;

    if( EESCHEMA_SETTINGS* cfg = m_frame->eeconfig() )
        lineWidthPixels = std::max( cfg->m_Selection.drag_net_collision_width, 1 );

    double lineWidth = m_view->ToWorld( lineWidthPixels );

    if( lineWidth <= 0.0 )
        lineWidth = 1.0;

    m_overlay->SetLineWidth( lineWidth );

    for( const COLLISION_MARKER& marker : markers )
        m_overlay->Circle( marker.position, marker.radius );

    for( const DISCONNECTION_MARKER& marker : disconnections )
    {
        m_overlay->Circle( marker.pointA, marker.radius );
        m_overlay->Circle( marker.pointB, marker.radius );
        m_overlay->Line( VECTOR2D( marker.pointA ), VECTOR2D( marker.pointB ) );
    }

    m_view->Update( m_overlay.get() );
    m_hasCollision = true;
    return true;
}


void SCH_DRAG_NET_COLLISION_MONITOR::Reset()
{
    clearOverlay();
    m_itemNetCodes.clear();
    m_originalConnections.clear();
    m_hasCollision = false;
}


KICURSOR SCH_DRAG_NET_COLLISION_MONITOR::AdjustCursor( KICURSOR aBaseCursor ) const
{
    if( m_hasCollision )
        return KICURSOR::WARNING;

    return aBaseCursor;
}


std::optional<SCH_DRAG_NET_COLLISION_MONITOR::COLLISION_MARKER>
SCH_DRAG_NET_COLLISION_MONITOR::analyzeJunction( SCH_JUNCTION* aJunction,
                                               const SCH_SELECTION& aSelection ) const
{
    if( !aJunction )
        return std::nullopt;

    const VECTOR2I position = aJunction->GetPosition();
    std::optional<int> firstNet;
    bool differentNets = false;
    bool movedNet = false;
    const auto accumulate = [&]( SCH_ITEM* item )
    {
        const auto found = m_itemNetCodes.find( item );

        if( found == m_itemNetCodes.end() || !found->second )
            return;

        if( !item->IsConnected( position )
            && !( item->Type() == SCH_LINE_T && item->HitTest( position ) ) )
        {
            return;
        }

        if( firstNet && firstNet != found->second )
            differentNets = true;

        firstNet = found->second;
        movedNet |= item->IsSelected() || aSelection.Contains( item )
                    || aSelection.Contains( item->GetParent() );
    };
    const auto visit = [&]( SCH_ITEM* item )
    {
        accumulate( item );
        item->RunOnChildren( accumulate, RECURSE_MODE::NO_RECURSE );
    };

    for( SCH_ITEM* candidate : m_frame->GetScreen()->Items().Overlapping( position ) )
        visit( candidate );

    // Moved geometry may not yet be reflected in the screen's spatial index.
    for( EDA_ITEM* selected : aSelection )
        visit( static_cast<SCH_ITEM*>( selected ) );

    if( !movedNet || !differentNets )
        return std::nullopt;

    return COLLISION_MARKER{ position, std::max( aJunction->GetEffectiveDiameter() * 1.5, 800.0 ) };
}


void SCH_DRAG_NET_COLLISION_MONITOR::recordItemNet( SCH_ITEM* aItem )
{
    if( !aItem || !aItem->IsConnectable() || m_itemNetCodes.contains( aItem ) )
        return;

    std::optional<int> netCode;

    if( const SCH_CONNECTION* connection = aItem->Connection( &m_sheetPath ) )
    {
        if( connection->IsNet() && !connection->IsUnconnected() && connection->NetCode() > 0 )
            netCode = connection->NetCode();
    }

    m_itemNetCodes.emplace( aItem, netCode );
}


void SCH_DRAG_NET_COLLISION_MONITOR::recordOriginalConnections( const SCH_SELECTION& aSelection )
{
    wxLogTrace( traceSchDragNetCollision, "recordOriginalConnections: Recording connections for %d items",
                aSelection.GetSize() );

    // Don't record original connections for new or pasted items (duplicates, pastes)
    // as they weren't previously connected to anything
    bool hasNewOrPastedItems = false;

    for( EDA_ITEM* edaItem : aSelection )
    {
        if( edaItem->IsNew() || ( edaItem->GetFlags() & IS_PASTED ) )
        {
            hasNewOrPastedItems = true;
            break;
        }
    }

    if( hasNewOrPastedItems )
    {
        wxLogTrace( traceSchDragNetCollision,
                    "recordOriginalConnections: Skipping - selection contains new or pasted items" );
        return;
    }

    EE_RTREE& items = m_frame->GetScreen()->Items();

    for( EDA_ITEM* edaItem : aSelection )
    {
        SCH_ITEM* item = static_cast<SCH_ITEM*>( edaItem );

        if( !item || !item->IsConnectable() )
            continue;

        std::vector<VECTOR2I> points = item->GetConnectionPoints();

        for( size_t index = 0; index < points.size(); ++index )
        {
            const VECTOR2I& point = points[index];

            for( SCH_ITEM* candidate : items.Overlapping( point ) )
            {
                if( candidate == item || !candidate->IsConnectable() )
                    continue;

                if( !candidate->CanConnect( item ) )
                    continue;

                if( !candidate->IsConnected( point )
                        && !( candidate->IsType( { SCH_LINE_T } ) && candidate->HitTest( point ) ) )
                {
                    continue;
                }

                std::vector<VECTOR2I> candidatePoints = candidate->GetConnectionPoints();
                size_t               candidateIndex = std::numeric_limits<size_t>::max();

                for( size_t candidatePos = 0; candidatePos < candidatePoints.size(); ++candidatePos )
                {
                    if( candidatePoints[candidatePos] == point )
                    {
                        candidateIndex = candidatePos;
                        break;
                    }
                }

                if( candidateIndex == std::numeric_limits<size_t>::max() )
                    continue;

                SCH_ITEM* firstItem = item;
                size_t    firstIndex = index;
                SCH_ITEM* secondItem = candidate;
                size_t    secondIndex = candidateIndex;

                if( secondItem < firstItem || ( secondItem == firstItem && secondIndex < firstIndex ) )
                {
                    std::swap( firstItem, secondItem );
                    std::swap( firstIndex, secondIndex );
                }

                if( firstItem == secondItem )
                    continue;

                bool firstSelected = firstItem->IsSelected() || aSelection.Contains( firstItem );
                bool secondSelected = secondItem->IsSelected() || aSelection.Contains( secondItem );

                if( !firstSelected && !secondSelected )
                    continue;

                auto existing = std::find_if( m_originalConnections.begin(), m_originalConnections.end(),
                        [&]( const ORIGINAL_CONNECTION& connection )
                        {
                            return connection.itemA == firstItem && connection.indexA == firstIndex
                                   && connection.itemB == secondItem && connection.indexB == secondIndex;
                        } );

                if( existing != m_originalConnections.end() )
                    continue;

                m_originalConnections.push_back( { firstItem, firstIndex, secondItem, secondIndex } );
            }
        }
    }

    wxLogTrace( traceSchDragNetCollision, "recordOriginalConnections: Tracked %zu connections",
                m_originalConnections.size() );
}


std::vector<SCH_DRAG_NET_COLLISION_MONITOR::DISCONNECTION_MARKER>
SCH_DRAG_NET_COLLISION_MONITOR::collectDisconnectedMarkers( const SCH_SELECTION& aSelection ) const
{
    std::vector<DISCONNECTION_MARKER> markers;

    for( const ORIGINAL_CONNECTION& connection : m_originalConnections )
    {
        SCH_ITEM* itemA = connection.itemA;
        SCH_ITEM* itemB = connection.itemB;

        if( !itemA || !itemB )
            continue;

        if( !itemA->IsConnectable() || !itemB->IsConnectable() )
            continue;

        std::vector<VECTOR2I> pointsA = itemA->GetConnectionPoints();
        std::vector<VECTOR2I> pointsB = itemB->GetConnectionPoints();

        if( connection.indexA >= pointsA.size() || connection.indexB >= pointsB.size() )
            continue;

        VECTOR2I pointA = pointsA[ connection.indexA ];
        VECTOR2I pointB = pointsB[ connection.indexB ];

        // Check if the connection is still valid. Points match exactly.
        bool stillConnected = ( pointA == pointB );

        // For lines, connection is valid if the point is anywhere on the line
        if( !stillConnected && itemB->IsType( { SCH_LINE_T } ) && itemB->HitTest( pointA, 0 ) )
            stillConnected = true;

        if( !stillConnected && itemA->IsType( { SCH_LINE_T } ) && itemA->HitTest( pointB, 0 ) )
            stillConnected = true;

        if( stillConnected )
            continue;

        bool relevant = itemA->IsSelected() || aSelection.Contains( itemA )
                        || itemB->IsSelected() || aSelection.Contains( itemB );

        if( !relevant )
            continue;

        double radius = std::max( { 800.0,
                                    static_cast<double>( itemA->GetPenWidth() ),
                                    static_cast<double>( itemB->GetPenWidth() ) } );

        DISCONNECTION_MARKER marker;
        marker.pointA = pointA;
        marker.pointB = pointB;
        marker.radius = radius;
        markers.push_back( marker );
    }

    if( !markers.empty() )
    {
    wxLogTrace( traceSchDragNetCollision,
            "collectDisconnectedMarkers: Identified %zu disconnections", markers.size() );
    }

    return markers;
}


void SCH_DRAG_NET_COLLISION_MONITOR::ensureOverlay()
{
    if( !m_overlay )
        m_overlay = m_view->MakeOverlay();
}


void SCH_DRAG_NET_COLLISION_MONITOR::clearOverlay() const
{
    if( m_overlay )
    {
        m_overlay->Clear();
        m_view->Update( m_overlay.get() );
    }
}
