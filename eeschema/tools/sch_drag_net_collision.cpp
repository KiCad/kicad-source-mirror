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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "sch_drag_net_collision.h"

#include <trace_helpers.h>

#include <algorithm>
#include <limits>
#include <unordered_set>

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

    EE_RTREE& items = m_frame->GetScreen()->Items();

    wxLogTrace( traceSchDragNetCollision, "Initialize: Recording nets for %zu screen items",
                items.size() );

    for( SCH_ITEM* item : items )
        recordItemNet( item );

    wxLogTrace( traceSchDragNetCollision, "Initialize: Recording nets for %d selected items",
                aSelection.GetSize() );

    for( EDA_ITEM* edaItem : aSelection )
        recordItemNet( static_cast<SCH_ITEM*>( edaItem ) );

    recordOriginalConnections( aSelection );

    wxLogTrace( traceSchDragNetCollision, "Initialize: Complete. Tracked %zu items with net codes",
                m_itemNetCodes.size() );
}


bool SCH_DRAG_NET_COLLISION_MONITOR::Update( const std::vector<SCH_JUNCTION*>& aJunctions,
                                             const SCH_SELECTION& aSelection,
                                             std::span<const PREVIEW_NET_ASSIGNMENT> aPreviewAssignments )
{
    wxLogTrace( traceSchDragNetCollision, "Update: Called with %zu junctions, %d selected items, %zu preview assignments",
                aJunctions.size(), aSelection.GetSize(), aPreviewAssignments.size() );

    std::unordered_map<const SCH_ITEM*, std::optional<int>> previewNetCodes;

    previewNetCodes.reserve( aPreviewAssignments.size() );

    for( const PREVIEW_NET_ASSIGNMENT& assignment : aPreviewAssignments )
    {
        if( !assignment.item )
            continue;

    wxLogTrace( traceSchDragNetCollision, "Update: Preview assignment - item %p, netCode %s",
            assignment.item,
            assignment.netCode.has_value() ? std::to_string( *assignment.netCode ).c_str() : "none" );

        previewNetCodes[ assignment.item ] = assignment.netCode;
    }

    std::vector<COLLISION_MARKER> markers;

    if( aJunctions.empty() )
    {
    wxLogTrace( traceSchDragNetCollision, "Update: No junctions to analyze" );
    }
    else
    {
    wxLogTrace( traceSchDragNetCollision, "Update: Analyzing %zu junctions", aJunctions.size() );

        for( SCH_JUNCTION* junction : aJunctions )
        {
            if( auto marker = analyzeJunction( junction, aSelection, previewNetCodes ) )
            {
                wxLogTrace( traceSchDragNetCollision, "Update: Junction at (%d, %d) has collision",
                            marker->position.x, marker->position.y );
                markers.push_back( *marker );
            }
        }
    }

    std::vector<DISCONNECTION_MARKER> disconnections = collectDisconnectedMarkers( aSelection );

    if( markers.empty() && disconnections.empty() )
    {
    wxLogTrace( traceSchDragNetCollision, "Update: No collisions or disconnections detected" );
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


std::optional<int> SCH_DRAG_NET_COLLISION_MONITOR::GetNetCode( const SCH_ITEM* aItem ) const
{
    if( !aItem )
        return std::nullopt;

    auto it = m_itemNetCodes.find( aItem );

    if( it != m_itemNetCodes.end() )
        return it->second;

    if( SCH_CONNECTION* connection = aItem->Connection( &m_sheetPath ) )
    {
        if( connection->IsNet() && !connection->IsUnconnected() )
        {
            int netCode = connection->NetCode();

            if( netCode > 0 )
                return netCode;
        }
    }

    return std::nullopt;
}


std::optional<SCH_DRAG_NET_COLLISION_MONITOR::COLLISION_MARKER> SCH_DRAG_NET_COLLISION_MONITOR::analyzeJunction(
        SCH_JUNCTION* aJunction, const SCH_SELECTION& aSelection,
        const std::unordered_map<const SCH_ITEM*, std::optional<int>>& aPreviewNetCodes ) const
{
    if( !aJunction )
        return std::nullopt;

    VECTOR2I position = aJunction->GetPosition();
    EE_RTREE& items = m_frame->GetScreen()->Items();

    wxLogTrace( traceSchDragNetCollision, "analyzeJunction: Checking junction at (%d, %d)",
                position.x, position.y );

    std::unordered_set<int> allNetCodes;
    std::unordered_set<int> movedNetCodes;
    std::unordered_set<int> originalNetCodes;
    std::unordered_set<int> movedOriginalNetCodes;
    std::unordered_set<int> stationaryOriginalNetCodes;

    auto accumulateNet = [&]( SCH_ITEM* item )
    {
        if( !item )
        {
            wxLogTrace( traceSchDragNetCollision, "  accumulateNet: null item" );
            return;
        }

        if( !item->IsConnectable() )
        {
            wxLogTrace( traceSchDragNetCollision, "  accumulateNet: item %p (%s) not connectable",
                        item, item->GetClass().c_str() );
            return;
        }

        if( !item->IsConnected( position ) && !( item->IsType( { SCH_LINE_T } ) && item->HitTest( position ) ) )
        {
            wxLogTrace( traceSchDragNetCollision, "  accumulateNet: item %p (%s) not connected at (%d, %d)",
                        item, item->GetClass().c_str(), position.x, position.y );
            return;
        }

        auto previewIt = aPreviewNetCodes.find( item );
        auto originalIt = m_itemNetCodes.find( item );
        std::optional<int> netCodeOpt;
        std::optional<int> originalNetOpt;

        if( originalIt != m_itemNetCodes.end() )
            originalNetOpt = originalIt->second;

        if( previewIt != aPreviewNetCodes.end() )
        {
            netCodeOpt = previewIt->second;
            wxLogTrace( traceSchDragNetCollision, "  accumulateNet: item %p (%s) using preview net %s",
                        item, item->GetClass().c_str(),
                        netCodeOpt.has_value() ? std::to_string( *netCodeOpt ).c_str() : "none" );
        }
        else if( originalIt != m_itemNetCodes.end() )
        {
            netCodeOpt = originalIt->second;
            wxLogTrace( traceSchDragNetCollision, "  accumulateNet: item %p (%s) using cached net %s",
                        item, item->GetClass().c_str(),
                        netCodeOpt.has_value() ? std::to_string( *netCodeOpt ).c_str() : "none" );
        }
        else
        {
            wxLogTrace( traceSchDragNetCollision, "  accumulateNet: item %p (%s) has no net code",
                        item, item->GetClass().c_str() );
        }

        bool isSelectionItem = item->IsSelected() || aSelection.Contains( item );
        bool isMoved = ( previewIt != aPreviewNetCodes.end() ) || isSelectionItem;

        if( !netCodeOpt )
        {
            if( originalNetOpt )
            {
                originalNetCodes.insert( *originalNetOpt );

                if( isSelectionItem )
                    movedOriginalNetCodes.insert( *originalNetOpt );
                else
                    stationaryOriginalNetCodes.insert( *originalNetOpt );
            }

            wxLogTrace( traceSchDragNetCollision, "  accumulateNet: item %p (%s) netCode is nullopt",
                        item, item->GetClass().c_str() );
            return;
        }

        int netCode = *netCodeOpt;
        allNetCodes.insert( netCode );

    wxLogTrace( traceSchDragNetCollision, "  accumulateNet: item %p (%s) net %d, moved=%s",
            item, item->GetClass().c_str(), netCode, isMoved ? "yes" : "no" );

        if( isMoved )
            movedNetCodes.insert( netCode );

        if( originalNetOpt )
        {
            originalNetCodes.insert( *originalNetOpt );

            if( isSelectionItem )
                movedOriginalNetCodes.insert( *originalNetOpt );
            else
                stationaryOriginalNetCodes.insert( *originalNetOpt );
        }
    };

    wxLogTrace( traceSchDragNetCollision, "analyzeJunction: Checking items overlapping position" );

    int candidateCount = 0;

    for( SCH_ITEM* candidate : items.Overlapping( position ) )
    {
        candidateCount++;
        accumulateNet( candidate );
    }

    wxLogTrace( traceSchDragNetCollision, "analyzeJunction: Checked %d overlapping items", candidateCount );
    wxLogTrace( traceSchDragNetCollision, "analyzeJunction: Checking %d selected items", aSelection.GetSize() );

    for( EDA_ITEM* selected : aSelection )
        accumulateNet( static_cast<SCH_ITEM*>( selected ) );

    wxLogTrace( traceSchDragNetCollision, "analyzeJunction: Found %zu unique nets, %zu moved nets",
                allNetCodes.size(), movedNetCodes.size() );

    if( !movedNetCodes.empty() )
    {
    wxLogTrace( traceSchDragNetCollision, "analyzeJunction: Moved nets:" );

        for( int netCode : movedNetCodes )
            wxLogTrace( traceSchDragNetCollision, "  - Net %d", netCode );
    }

    wxLogTrace( traceSchDragNetCollision,
                "analyzeJunction: Original nets=%zu, moved originals=%zu, stationary originals=%zu",
                originalNetCodes.size(), movedOriginalNetCodes.size(), stationaryOriginalNetCodes.size() );

    if( !movedOriginalNetCodes.empty() )
    {
    wxLogTrace( traceSchDragNetCollision, "analyzeJunction: Moved original nets:" );

        for( int netCode : movedOriginalNetCodes )
            wxLogTrace( traceSchDragNetCollision, "  - Net %d", netCode );
    }

    if( !stationaryOriginalNetCodes.empty() )
    {
    wxLogTrace( traceSchDragNetCollision, "analyzeJunction: Stationary original nets:" );

        for( int netCode : stationaryOriginalNetCodes )
            wxLogTrace( traceSchDragNetCollision, "  - Net %d", netCode );
    }

    if( allNetCodes.size() >= 2 )
    {
    wxLogTrace( traceSchDragNetCollision, "analyzeJunction: All nets at junction:" );

        for( int netCode : allNetCodes )
            wxLogTrace( traceSchDragNetCollision, "  - Net %d", netCode );
    }

    bool previewCollision = !movedNetCodes.empty() && allNetCodes.size() >= 2;

    bool originalCollision = false;

    if( !movedOriginalNetCodes.empty() && !stationaryOriginalNetCodes.empty() )
    {
        for( int movedNet : movedOriginalNetCodes )
        {
            for( int stationaryNet : stationaryOriginalNetCodes )
            {
                if( movedNet != stationaryNet )
                {
                    originalCollision = true;
                    break;
                }
            }

            if( originalCollision )
                break;
        }
    }

    if( !previewCollision && !originalCollision )
    {
        wxLogTrace( traceSchDragNetCollision, "analyzeJunction: No collision (movedNets=%zu, allNets=%zu)",
                    movedNetCodes.size(), allNetCodes.size() );
        return std::nullopt;
    }

    if( originalCollision && !previewCollision )
    {
        wxLogTrace( traceSchDragNetCollision,
                        "analyzeJunction: Original net mismatch detected under moved endpoints" );
    }

    COLLISION_MARKER marker;
    marker.position = position;
    double base = static_cast<double>( aJunction->GetEffectiveDiameter() );
    marker.radius = std::max( base * 1.5, 800.0 );

    wxLogTrace( traceSchDragNetCollision, "analyzeJunction: COLLISION DETECTED at (%d, %d) with radius %.1f",
                position.x, position.y, marker.radius );

    return marker;
}


void SCH_DRAG_NET_COLLISION_MONITOR::recordItemNet( SCH_ITEM* aItem )
{
    if( !aItem )
        return;

    if( !aItem->IsConnectable() )
        return;

    if( m_itemNetCodes.find( aItem ) != m_itemNetCodes.end() )
        return;

    if( SCH_CONNECTION* connection = aItem->Connection( &m_sheetPath ) )
    {
        if( connection->IsNet() && !connection->IsUnconnected() )
        {
            int netCode = connection->NetCode();

            if( netCode > 0 )
            {
                wxLogTrace( traceSchDragNetCollision, "recordItemNet: Item %p (%s) at (%d, %d) -> net %d (%s)",
                            aItem, aItem->GetClass().c_str(),
                            aItem->GetPosition().x, aItem->GetPosition().y,
                            netCode, connection->Name().c_str() );
                m_itemNetCodes.emplace( aItem, netCode );
            }
            else
            {
                wxLogTrace( traceSchDragNetCollision, "recordItemNet: Item %p (%s) has invalid netCode %d",
                            aItem, aItem->GetClass().c_str(), netCode );
                m_itemNetCodes.emplace( aItem, std::nullopt );
            }
        }
        else
        {
            wxLogTrace( traceSchDragNetCollision, "recordItemNet: Item %p (%s) connection not a net or unconnected",
                        aItem, aItem->GetClass().c_str() );
            m_itemNetCodes.emplace( aItem, std::nullopt );
        }
    }
    else
    {
    wxLogTrace( traceSchDragNetCollision, "recordItemNet: Item %p (%s) has no connection",
            aItem, aItem->GetClass().c_str() );
        m_itemNetCodes.emplace( aItem, std::nullopt );
    }
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
