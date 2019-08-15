/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013  CERN
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

#include <wx/numdlg.h>

#include <functional>
using namespace std::placeholders;

#include "class_draw_panel_gal.h"
#include "class_board.h"

#include <pcb_edit_frame.h>
#include <id.h>
#include <macros.h>
#include <pcbnew_id.h>
#include <view/view_controls.h>
#include <pcb_painter.h>
#include <dialogs/dialog_pns_settings.h>
#include <dialogs/dialog_pns_diff_pair_dimensions.h>
#include <dialogs/dialog_pns_length_tuning_settings.h>
#include <dialogs/dialog_track_via_size.h>
#include <base_units.h>
#include <bitmaps.h>

#include <tool/action_menu.h>
#include <tools/pcb_actions.h>
#include <tools/grid_helper.h>

#include "pns_kicad_iface.h"
#include "pns_tool_base.h"
#include "pns_segment.h"
#include "pns_solid.h"
#include "pns_via.h"
#include "pns_router.h"
#include "pns_meander_placer.h" // fixme: move settings to separate header
#include "pns_tune_status_popup.h"
#include "pns_topology.h"

#include <view/view.h>

using namespace KIGFX;

namespace PNS {


TOOL_BASE::TOOL_BASE( const std::string& aToolName ) :
    PCB_TOOL_BASE( aToolName )
{
    m_gridHelper = nullptr;
    m_iface = nullptr;
    m_router = nullptr;

    m_startItem = nullptr;
    m_startLayer = 0;
    m_startHighlight = false;

    m_endItem = nullptr;
    m_gridHelper = nullptr;
}


TOOL_BASE::~TOOL_BASE()
{
    delete m_gridHelper;
    delete m_iface;
    delete m_router;
}



void TOOL_BASE::Reset( RESET_REASON aReason )
{
    delete m_gridHelper;
    delete m_iface;
    delete m_router;

    m_iface = new PNS_KICAD_IFACE;
    m_iface->SetBoard( board() );
    m_iface->SetView( getView() );
    m_iface->SetHostTool( this );
    m_iface->SetDisplayOptions( (PCB_DISPLAY_OPTIONS*) frame()->GetDisplayOptions() );

    m_router = new ROUTER;
    m_router->SetInterface( m_iface );
    m_router->ClearWorld();
    m_router->SyncWorld();
    m_router->LoadSettings( m_savedSettings );
    m_router->UpdateSizes( m_savedSizes );

    m_gridHelper = new GRID_HELPER( frame() );
}


ITEM* TOOL_BASE::pickSingleItem( const VECTOR2I& aWhere, int aNet, int aLayer, bool aIgnorePads,
								 const std::vector<ITEM*> aAvoidItems)
{
    int tl = getView()->GetTopLayer();

    if( aLayer > 0 )
        tl = aLayer;

    static const int candidateCount = 5;
    ITEM* prioritized[candidateCount];
    int dist[candidateCount];

    for( int i = 0; i < candidateCount; i++ )
    {
        prioritized[i] = 0;
        dist[i] = std::numeric_limits<int>::max();
    }

    ITEM_SET candidates = m_router->QueryHoverItems( aWhere );

    for( ITEM* item : candidates.Items() )
    {
        if( !item->IsRoutable() )
            continue;

        if( !IsCopperLayer( item->Layers().Start() ) )
            continue;

        if( !m_iface->IsAnyLayerVisible( item->Layers() ) )
            continue;

        if( std::find( aAvoidItems.begin(), aAvoidItems.end(), item ) != aAvoidItems.end() )
            continue;

        // fixme: this causes flicker with live loop removal...
        //if( item->Parent() && !item->Parent()->ViewIsVisible() )
        //    continue;

        if( aNet <= 0 || item->Net() == aNet )
        {
            if( item->OfKind( ITEM::VIA_T | ITEM::SOLID_T ) )
            {
                if( item->OfKind( ITEM::SOLID_T ) && aIgnorePads )
                    continue;

                int itemDist = ( item->Shape()->Centre() - aWhere ).SquaredEuclideanNorm();

                if( !prioritized[2] || itemDist < dist[2] )
                {
                    prioritized[2] = item;
                    dist[2] = itemDist;
                }
                if( item->Layers().Overlaps( tl ) &&  itemDist < dist[0] )
                {
                    prioritized[0] = item;
                    dist[0] = itemDist;
                }
            }
            else
            {
                if( !prioritized[3] )
                    prioritized[3] = item;
                if( item->Layers().Overlaps( tl ) )
                    prioritized[1] = item;
            }
        }
        // Allow unconnected items as last resort in RM_MarkObstacles mode
        else if ( item->Net() == 0 && m_router->Settings().Mode() == RM_MarkObstacles )
        {
            if( item->OfKind( ITEM::SOLID_T ) && aIgnorePads )
                continue;

            if( item->Layers().Overlaps( tl ) )
                prioritized[4] = item;
        }
    }

    ITEM* rv = NULL;

    for( int i = 0; i < candidateCount; i++ )
    {
        ITEM* item = prioritized[i];

        if( displayOptions()->m_ContrastModeDisplay )
            if( item && !item->Layers().Overlaps( tl ) )
                item = NULL;

        if( item && ( aLayer < 0 || item->Layers().Overlaps( aLayer ) ) )
        {
            rv = item;
            break;
        }
    }

    if( rv )
    {
        wxLogTrace( "PNS", "%s, layer : %d, tl: %d", rv->KindStr().c_str(), rv->Layers().Start(), tl );
    }

    return rv;
}


void TOOL_BASE::highlightNet( bool aEnabled, int aNetcode )
{
    RENDER_SETTINGS* rs = getView()->GetPainter()->GetSettings();

    if( aNetcode >= 0 && aEnabled )
    {
        // If the user has previously set the current net to be highlighted,
        // we assume they want to keep it highlighted after routing
        m_startHighlight = ( rs->IsHighlightEnabled() && rs->GetHighlightNetCode() == aNetcode );

        rs->SetHighlight( true, aNetcode );
    }
    else
    {
        if( !m_startHighlight )
            rs->SetHighlight( false );

        m_startHighlight = false;
    }

    getView()->UpdateAllLayersColor();
}

bool TOOL_BASE::checkSnap( ITEM *aItem )
{
    // Sync PNS engine settings with the general PCB editor options.
    auto& pnss = m_router->Settings();

    pnss.SetSnapToPads( frame()->Settings().m_MagneticPads == CAPTURE_CURSOR_IN_TRACK_TOOL
                     || frame()->Settings().m_MagneticPads == CAPTURE_ALWAYS );

    pnss.SetSnapToTracks( frame()->Settings().m_MagneticTracks == CAPTURE_CURSOR_IN_TRACK_TOOL
                       || frame()->Settings().m_MagneticTracks == CAPTURE_ALWAYS );

    if( aItem )
    {
        if( aItem->OfKind( ITEM::VIA_T ) || aItem->OfKind( ITEM::SEGMENT_T )  )
            return pnss.GetSnapToTracks();
        else if( aItem->OfKind( ITEM::SOLID_T ) )
            return pnss.GetSnapToPads();
    }

    return false;
}

void TOOL_BASE::updateStartItem( const TOOL_EVENT& aEvent, bool aIgnorePads )
{
    int tl = getView()->GetTopLayer();
    VECTOR2I cp = controls()->GetCursorPosition( !aEvent.Modifier( MD_SHIFT ) );
    VECTOR2I p;

    controls()->ForceCursorPosition( false );
    m_gridHelper->SetUseGrid( !aEvent.Modifier( MD_ALT ) );
    m_gridHelper->SetSnap( !aEvent.Modifier( MD_SHIFT ) );

    bool snapEnabled = true;

    if( aEvent.IsMotion() || aEvent.IsClick() )
    {
        snapEnabled = !aEvent.Modifier( MD_SHIFT );
        p = aEvent.Position();
    }
    else
    {
        p = cp;
    }

    m_startItem = pickSingleItem( p, -1, -1, aIgnorePads );

    if( !snapEnabled && m_startItem && !m_startItem->Layers().Overlaps( tl ) )
        m_startItem = nullptr;

    m_startSnapPoint = snapToItem( snapEnabled, m_startItem, p );

    if( checkSnap( m_startItem ) )
    {
        controls()->ForceCursorPosition( true, m_startSnapPoint );
    }
}


void TOOL_BASE::updateEndItem( const TOOL_EVENT& aEvent )
{
    int layer;
    bool snapEnabled = !aEvent.Modifier( MD_SHIFT );
    m_gridHelper->SetUseGrid( !aEvent.Modifier( MD_ALT ) );
    m_gridHelper->SetSnap( snapEnabled );

    controls()->ForceCursorPosition( false );
    VECTOR2I mousePos = controls()->GetMousePosition();

    if( m_router->Settings().Mode() != RM_MarkObstacles &&
        ( m_router->GetCurrentNets().empty() || m_router->GetCurrentNets().front() < 0 ) )
    {
        m_endSnapPoint = snapToItem( snapEnabled, nullptr, mousePos );
        controls()->ForceCursorPosition( true, m_endSnapPoint );
        m_endItem = nullptr;

        return;
    }

    if( m_router->IsPlacingVia() )
        layer = -1;
    else
        layer = m_router->GetCurrentLayer();

    ITEM* endItem = nullptr;

    std::vector<int> nets = m_router->GetCurrentNets();

    for( int net : nets )
    {
        endItem = pickSingleItem( mousePos, net, layer, false, { m_startItem } );

        if( endItem )
            break;
    }

    if( checkSnap( endItem ) )
    {
        m_endItem = endItem;
        m_endSnapPoint = snapToItem( snapEnabled, endItem, mousePos );
    } else {
        m_endItem = nullptr;
        m_endSnapPoint = m_gridHelper->Align( mousePos );
    }

    controls()->ForceCursorPosition( true, m_endSnapPoint );

    if( m_endItem )
    {
        wxLogTrace( "PNS", "%s, layer : %d", m_endItem->KindStr().c_str(), m_endItem->Layers().Start() );
    }
}


void TOOL_BASE::deleteTraces( ITEM* aStartItem, bool aWholeTrack )
{
    NODE *node = m_router->GetWorld()->Branch();

    if( !aStartItem )
        return;

    if( !aWholeTrack )
    {
        node->Remove( aStartItem );
    }
    else
    {
        TOPOLOGY topo( node );
        ITEM_SET path = topo.AssembleTrivialPath( aStartItem );

        for( auto ent : path.Items() )
            node->Remove( ent.item );
    }

    m_router->CommitRouting( node );
}


ROUTER *TOOL_BASE::Router() const
{
    return m_router;
}


const VECTOR2I TOOL_BASE::snapToItem( bool aEnabled, ITEM* aItem, VECTOR2I aP)
{
    VECTOR2I anchor;

    if( !aItem || !aEnabled )
    {
        return m_gridHelper->Align( aP );
    }

    switch( aItem->Kind() )
    {
    case ITEM::SOLID_T:
        anchor = static_cast<SOLID*>( aItem )->Pos();
        break;

    case ITEM::VIA_T:
        anchor = static_cast<VIA*>( aItem )->Pos();
        break;

    case ITEM::SEGMENT_T:
    {
        SEGMENT* seg = static_cast<SEGMENT*>( aItem );
        const SEG& s = seg->Seg();
        int w = seg->Width();


        if( ( aP - s.A ).EuclideanNorm() < w / 2 )
            anchor = s.A;
        else if( ( aP - s.B ).EuclideanNorm() < w / 2 )
            anchor = s.B;
        else
            anchor = m_gridHelper->AlignToSegment( aP, s );

        break;
    }

    default:
        break;
    }

    return anchor;
}

}
