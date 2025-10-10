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

#include <algorithm>
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

SCH_DRAG_NET_COLLISION_MONITOR::SCH_DRAG_NET_COLLISION_MONITOR( SCH_EDIT_FRAME* aFrame,
                                                                KIGFX::VIEW* aView ) :
        m_frame( aFrame ),
        m_view( aView ),
        m_overlay(),
        m_itemNetCodes(),
        m_sheetPath(),
        m_hasCollision( false )
{
}


SCH_DRAG_NET_COLLISION_MONITOR::~SCH_DRAG_NET_COLLISION_MONITOR()
{
    Reset();
}


void SCH_DRAG_NET_COLLISION_MONITOR::Initialize( const SCH_SELECTION& aSelection )
{
    wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "Initialize: Starting initialization" );

    m_itemNetCodes.clear();
    m_sheetPath = m_frame->GetCurrentSheet();
    m_hasCollision = false;

    EE_RTREE& items = m_frame->GetScreen()->Items();

    wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "Initialize: Recording nets for %zu screen items",
                items.size() );

    for( SCH_ITEM* item : items )
        recordItemNet( item );

    wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "Initialize: Recording nets for %d selected items",
                aSelection.GetSize() );

    for( EDA_ITEM* edaItem : aSelection )
        recordItemNet( static_cast<SCH_ITEM*>( edaItem ) );

    wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "Initialize: Complete. Tracked %zu items with net codes",
                m_itemNetCodes.size() );
}


bool SCH_DRAG_NET_COLLISION_MONITOR::Update( const std::vector<SCH_JUNCTION*>& aJunctions,
                                             const SCH_SELECTION& aSelection,
                                             std::span<const PREVIEW_NET_ASSIGNMENT> aPreviewAssignments )
{
    wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "Update: Called with %zu junctions, %d selected items, %zu preview assignments",
                aJunctions.size(), aSelection.GetSize(), aPreviewAssignments.size() );

    if( aJunctions.empty() )
    {
        wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "Update: No junctions to check, clearing overlay" );
        clearOverlay();
        m_hasCollision = false;
        return false;
    }

    ensureOverlay();
    m_overlay->Clear();

    std::unordered_map<const SCH_ITEM*, std::optional<int>> previewNetCodes;

    previewNetCodes.reserve( aPreviewAssignments.size() );

    for( const PREVIEW_NET_ASSIGNMENT& assignment : aPreviewAssignments )
    {
        if( !assignment.item )
            continue;

        wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "Update: Preview assignment - item %p, netCode %s",
                    assignment.item,
                    assignment.netCode.has_value() ? std::to_string( *assignment.netCode ).c_str() : "none" );

        previewNetCodes[ assignment.item ] = assignment.netCode;
    }

    std::vector<COLLISION_MARKER> markers;

    wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "Update: Analyzing %zu junctions", aJunctions.size() );

    for( SCH_JUNCTION* junction : aJunctions )
    {
        if( auto marker = analyzeJunction( junction, aSelection, previewNetCodes ) )
        {
            wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "Update: Junction at (%d, %d) has collision",
                        marker->position.x, marker->position.y );
            markers.push_back( *marker );
        }
    }

    if( markers.empty() )
    {
        wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "Update: No collision markers found" );
        m_hasCollision = false;
        m_view->Update( m_overlay.get() );
        return false;
    }

    wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "Update: Drawing %zu collision markers", markers.size() );

    m_overlay->SetIsFill( true );
    m_overlay->SetFillColor( COLOR4D( 1.0, 0.0, 0.0, 0.25 ) );
    m_overlay->SetIsStroke( true );
    m_overlay->SetStrokeColor( COLOR4D( 1.0, 0.0, 0.0, 0.7 ) );

    for( const COLLISION_MARKER& marker : markers )
    {
        double lineWidth = std::max( marker.radius * 0.3, 1.0 );
        m_overlay->SetLineWidth( lineWidth );
        m_overlay->Circle( marker.position, marker.radius );
    }

    m_view->Update( m_overlay.get() );
    m_hasCollision = true;
    return true;
}


void SCH_DRAG_NET_COLLISION_MONITOR::Reset()
{
    clearOverlay();
    m_itemNetCodes.clear();
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

    wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "analyzeJunction: Checking junction at (%d, %d)",
                position.x, position.y );

    std::unordered_set<int> allNetCodes;
    std::unordered_set<int> movedNetCodes;

    auto accumulateNet = [&]( SCH_ITEM* item )
    {
        if( !item )
        {
            wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "  accumulateNet: null item" );
            return;
        }

        if( !item->IsConnectable() )
        {
            wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "  accumulateNet: item %p (%s) not connectable",
                        item, item->GetClass().c_str() );
            return;
        }

        if( !item->IsConnected( position ) && !( item->IsType( { SCH_LINE_T } ) && item->HitTest( position ) ) )
        {
            wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "  accumulateNet: item %p (%s) not connected at (%d, %d)",
                        item, item->GetClass().c_str(), position.x, position.y );
            return;
        }

        auto previewIt = aPreviewNetCodes.find( item );
        std::optional<int> netCodeOpt;

        if( previewIt != aPreviewNetCodes.end() )
        {
            netCodeOpt = previewIt->second;
            wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "  accumulateNet: item %p (%s) using preview net %s",
                        item, item->GetClass().c_str(),
                        netCodeOpt.has_value() ? std::to_string( *netCodeOpt ).c_str() : "none" );
        }
        else
        {
            auto it = m_itemNetCodes.find( item );

            if( it != m_itemNetCodes.end() )
            {
                netCodeOpt = it->second;
                wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "  accumulateNet: item %p (%s) using cached net %s",
                            item, item->GetClass().c_str(),
                            netCodeOpt.has_value() ? std::to_string( *netCodeOpt ).c_str() : "none" );
            }
            else
            {
                wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "  accumulateNet: item %p (%s) has no net code",
                            item, item->GetClass().c_str() );
            }
        }

        if( !netCodeOpt )
        {
            wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "  accumulateNet: item %p (%s) netCode is nullopt",
                        item, item->GetClass().c_str() );
            return;
        }

        int netCode = *netCodeOpt;
        allNetCodes.insert( netCode );

        bool isMoved = ( previewIt != aPreviewNetCodes.end()
                         || item->IsSelected()
                         || aSelection.Contains( item ) );

        wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "  accumulateNet: item %p (%s) net %d, moved=%s",
                    item, item->GetClass().c_str(), netCode, isMoved ? "yes" : "no" );

        if( isMoved )
            movedNetCodes.insert( netCode );
    };

    wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "analyzeJunction: Checking items overlapping position" );

    int candidateCount = 0;

    for( SCH_ITEM* candidate : items.Overlapping( position ) )
    {
        candidateCount++;
        accumulateNet( candidate );
    }

    wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "analyzeJunction: Checked %d overlapping items", candidateCount );
    wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "analyzeJunction: Checking %d selected items", aSelection.GetSize() );

    for( EDA_ITEM* selected : aSelection )
        accumulateNet( static_cast<SCH_ITEM*>( selected ) );

    wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "analyzeJunction: Found %zu unique nets, %zu moved nets",
                allNetCodes.size(), movedNetCodes.size() );

    if( !movedNetCodes.empty() )
    {
        wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "analyzeJunction: Moved nets:" );

        for( int netCode : movedNetCodes )
            wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "  - Net %d", netCode );
    }

    if( allNetCodes.size() >= 2 )
    {
        wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "analyzeJunction: All nets at junction:" );

        for( int netCode : allNetCodes )
            wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "  - Net %d", netCode );
    }

    if( movedNetCodes.empty() || allNetCodes.size() < 2 )
    {
        wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "analyzeJunction: No collision (movedNets=%zu, allNets=%zu)",
                    movedNetCodes.size(), allNetCodes.size() );
        return std::nullopt;
    }

    COLLISION_MARKER marker;
    marker.position = position;
    double base = static_cast<double>( aJunction->GetEffectiveDiameter() );
    marker.radius = std::max( base * 1.5, 800.0 );

    wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "analyzeJunction: COLLISION DETECTED at (%d, %d) with radius %.1f",
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
                wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "recordItemNet: Item %p (%s) at (%d, %d) -> net %d (%s)",
                            aItem, aItem->GetClass().c_str(),
                            aItem->GetPosition().x, aItem->GetPosition().y,
                            netCode, connection->Name().c_str() );
                m_itemNetCodes.emplace( aItem, netCode );
            }
            else
            {
                wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "recordItemNet: Item %p (%s) has invalid netCode %d",
                            aItem, aItem->GetClass().c_str(), netCode );
                m_itemNetCodes.emplace( aItem, std::nullopt );
            }
        }
        else
        {
            wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "recordItemNet: Item %p (%s) connection not a net or unconnected",
                        aItem, aItem->GetClass().c_str() );
            m_itemNetCodes.emplace( aItem, std::nullopt );
        }
    }
    else
    {
        wxLogTrace( "KICAD_SCH_DRAG_NET_COLLISION", "recordItemNet: Item %p (%s) has no connection",
                    aItem, aItem->GetClass().c_str() );
        m_itemNetCodes.emplace( aItem, std::nullopt );
    }
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
