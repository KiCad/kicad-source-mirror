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

#include <wxPcbStruct.h>
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

#include <tool/context_menu.h>
#include <tools/common_actions.h>
#include <tools/grid_helper.h>

#include <ratsnest_data.h>

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

TOOL_ACTION TOOL_BASE::ACT_RouterOptions( "pcbnew.InteractiveRouter.RouterOptions",
                                            AS_CONTEXT, 'E',
                                            _( "Routing Options..." ),
                                            _( "Shows a dialog containing router options." ), tools_xpm );


TOOL_BASE::TOOL_BASE( const std::string& aToolName ) :
    TOOL_INTERACTIVE( aToolName )
{
    m_gridHelper = nullptr;
    m_iface = nullptr;
    m_router = nullptr;

    m_startItem = nullptr;
    m_startLayer = 0;

    m_endItem = nullptr;

    m_frame = nullptr;
    m_ctls = nullptr;
    m_board = nullptr;
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

    m_frame = getEditFrame<PCB_EDIT_FRAME>();
    m_ctls = getViewControls();
    m_board = getModel<BOARD>();

    m_iface = new PNS_KICAD_IFACE;
    m_iface->SetBoard( m_board );
    m_iface->SetView( getView() );
    m_iface->SetHostFrame( m_frame );

    m_router = new ROUTER;
    m_router->SetInterface(m_iface);
    m_router->ClearWorld();
    m_router->SyncWorld();
    m_router->LoadSettings( m_savedSettings );
    m_router->UpdateSizes( m_savedSizes );

    m_gridHelper = new GRID_HELPER( m_frame );
}


ITEM* TOOL_BASE::pickSingleItem( const VECTOR2I& aWhere, int aNet, int aLayer )
{
    int tl = getView()->GetTopLayer();

    if( aLayer > 0 )
        tl = aLayer;

    ITEM* prioritized[4];

    for( int i = 0; i < 4; i++ )
        prioritized[i] = 0;

    ITEM_SET candidates = m_router->QueryHoverItems( aWhere );

    for( ITEM* item : candidates.Items() )
    {
        if( !IsCopperLayer( item->Layers().Start() ) )
            continue;

        // fixme: this causes flicker with live loop removal...
        //if( item->Parent() && !item->Parent()->ViewIsVisible() )
        //    continue;

        if( aNet < 0 || item->Net() == aNet )
        {
            if( item->OfKind( ITEM::VIA_T | ITEM::SOLID_T ) )
            {
                if( !prioritized[2] )
                    prioritized[2] = item;
                if( item->Layers().Overlaps( tl ) )
                    prioritized[0] = item;
            }
            else
            {
                if( !prioritized[3] )
                    prioritized[3] = item;
                if( item->Layers().Overlaps( tl ) )
                    prioritized[1] = item;
            }
        }
    }

    ITEM* rv = NULL;
    PCB_EDIT_FRAME* frame = getEditFrame<PCB_EDIT_FRAME>();
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)frame->GetDisplayOptions();

    for( int i = 0; i < 4; i++ )
    {
        ITEM* item = prioritized[i];

        if( displ_opts->m_ContrastModeDisplay )
            if( item && !item->Layers().Overlaps( tl ) )
                item = NULL;

        if( item )
        {
            rv = item;
            break;
        }
    }

    if( rv && aLayer >= 0 && !rv->Layers().Overlaps( aLayer ) )
        rv = NULL;

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
        rs->SetHighlight( true, aNetcode );
    else
        rs->SetHighlight( false );

    getView()->UpdateAllLayersColor();
}


void TOOL_BASE::updateStartItem( TOOL_EVENT& aEvent )
{
    int tl = getView()->GetTopLayer();
    VECTOR2I cp = m_ctls->GetCursorPosition();
    VECTOR2I p;

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

    m_startItem = pickSingleItem( p );

    if( !snapEnabled && m_startItem && !m_startItem->Layers().Overlaps( tl ) )
        m_startItem = nullptr;

    m_startSnapPoint = snapToItem( snapEnabled, m_startItem, p );
    m_ctls->ForceCursorPosition( true, m_startSnapPoint );
}


void TOOL_BASE::updateEndItem( const TOOL_EVENT& aEvent )
{
    VECTOR2I mp = m_ctls->GetMousePosition();
    VECTOR2I p = getView()->ToWorld( mp );
    int layer;
    bool snapEnabled = !aEvent.Modifier( MD_SHIFT );

    if( m_router->GetCurrentNets().empty() || m_router->GetCurrentNets().front() < 0 )
    {
        m_endSnapPoint = snapToItem( snapEnabled, nullptr, p );
        m_ctls->ForceCursorPosition( true, m_endSnapPoint );
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
        endItem = pickSingleItem( p, net, layer );

        if( endItem )
            break;
    }

    VECTOR2I cursorPos = snapToItem( snapEnabled, endItem, p );
    m_ctls->ForceCursorPosition( true, cursorPos );
    m_endItem = endItem;
    m_endSnapPoint = cursorPos;

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
