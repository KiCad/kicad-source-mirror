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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/numdlg.h>

#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/bind.hpp>

#include "class_draw_panel_gal.h"
#include "class_board.h"

#include <wxPcbStruct.h>
#include <id.h>
#include <macros.h>
#include <pcbnew_id.h>
#include <view/view_controls.h>
#include <pcbcommon.h>
#include <pcb_painter.h>
#include <dialogs/dialog_pns_settings.h>
#include <dialogs/dialog_pns_diff_pair_dimensions.h>
#include <dialogs/dialog_pns_length_tuning_settings.h>
#include <dialogs/dialog_track_via_size.h>
#include <base_units.h>

#include <tool/context_menu.h>
#include <tools/common_actions.h>

#include <ratsnest_data.h>

#include "pns_tool_base.h"
#include "pns_segment.h"
#include "pns_router.h"
#include "pns_meander_placer.h" // fixme: move settings to separate header
#include "pns_tune_status_popup.h"
#include "trace.h"

using namespace KIGFX;
using boost::optional;

TOOL_ACTION PNS_TOOL_BASE::ACT_RouterOptions( "pcbnew.InteractiveRouter.RouterOptions",
                                            AS_CONTEXT, 'E',
                                            "Routing Options...", "Shows a dialog containing router options.");


PNS_TOOL_BASE::PNS_TOOL_BASE( const std::string& aToolName ) :
    TOOL_INTERACTIVE( aToolName )
{
    m_router = NULL;
}


PNS_TOOL_BASE::~PNS_TOOL_BASE()
{
    delete m_router;
}


void PNS_TOOL_BASE::Reset( RESET_REASON aReason )
{
    if( m_router )
        delete m_router;

    m_frame = getEditFrame<PCB_EDIT_FRAME>();
    m_ctls = getViewControls();
    m_board = getModel<BOARD>();

    m_router = new PNS_ROUTER;

    m_router->ClearWorld();
    m_router->SetBoard( m_board );
    m_router->SyncWorld();
    m_router->LoadSettings( m_savedSettings );
    m_router->UpdateSizes( m_savedSizes );
    m_needsSync = false;

    if( getView() )
        m_router->SetView( getView() );
}

PNS_ITEM* PNS_TOOL_BASE::pickSingleItem( const VECTOR2I& aWhere, int aNet, int aLayer )
{
    int tl = getView()->GetTopLayer();

    if( aLayer > 0 )
        tl = aLayer;

    PNS_ITEM* prioritized[4];

    for( int i = 0; i < 4; i++ )
        prioritized[i] = 0;

    PNS_ITEMSET candidates = m_router->QueryHoverItems( aWhere );

    BOOST_FOREACH( PNS_ITEM* item, candidates.Items() )
    {
        if( !IsCopperLayer( item->Layers().Start() ) )
            continue;

        // fixme: this causes flicker with live loop removal...
        //if( item->Parent() && !item->Parent()->ViewIsVisible() )
        //    continue;

        if( aNet < 0 || item->Net() == aNet )
        {
            if( item->OfKind( PNS_ITEM::VIA | PNS_ITEM::SOLID ) )
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

    PNS_ITEM* rv = NULL;
    PCB_EDIT_FRAME* frame = getEditFrame<PCB_EDIT_FRAME> ();
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)frame->GetDisplayOptions();

    for( int i = 0; i < 4; i++ )
    {
        PNS_ITEM* item = prioritized[i];

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
        TRACE( 0, "%s, layer : %d, tl: %d", rv->KindStr().c_str() % rv->Layers().Start() % tl );

    return rv;
}


void PNS_TOOL_BASE::highlightNet( bool aEnabled, int aNetcode )
{
    RENDER_SETTINGS* rs = getView()->GetPainter()->GetSettings();

    if( aNetcode >= 0 && aEnabled )
        rs->SetHighlight( true, aNetcode );
    else
        rs->SetHighlight( false );

    getView()->UpdateAllLayersColor();
}

void PNS_TOOL_BASE::updateStartItem( TOOL_EVENT& aEvent )
{
    int tl = getView()->GetTopLayer();
    VECTOR2I cp = m_ctls->GetCursorPosition();
    PNS_ITEM* startItem = NULL;

    if( aEvent.IsMotion() || aEvent.IsClick() )
    {
        bool snapEnabled = !aEvent.Modifier( MD_SHIFT );
        
        VECTOR2I p( aEvent.Position() );
        startItem = pickSingleItem( p );
        m_router->EnableSnapping ( snapEnabled );

        if( !snapEnabled && startItem && !startItem->Layers().Overlaps( tl ) )
            startItem = NULL;

        if( startItem && startItem->Net() >= 0 )
        {
            bool dummy;
            VECTOR2I psnap = m_router->SnapToItem( startItem, p, dummy );

            if( snapEnabled )
            {
                m_startSnapPoint = psnap;
                m_ctls->ForceCursorPosition( true, psnap );
            }
            else
            {
                m_startSnapPoint = cp;
                m_ctls->ForceCursorPosition( false );
            }

//            if( startItem->Layers().IsMultilayer() )
//                m_startLayer = tl;
//            else
//                m_startLayer = startItem->Layers().Start();

            m_startItem = startItem;
        }
        else
        {
            m_startItem = NULL;
            m_startSnapPoint = cp;
            m_ctls->ForceCursorPosition( false );
        }
    }
}

void PNS_TOOL_BASE::updateEndItem( TOOL_EVENT& aEvent )
{
    VECTOR2I mp = m_ctls->GetMousePosition();
    VECTOR2I p = getView()->ToWorld( mp );
    VECTOR2I cp = m_ctls->GetCursorPosition();
    int layer;
    bool snapEnabled = !aEvent.Modifier( MD_CTRL );

    m_router->EnableSnapping( snapEnabled );

    if( !snapEnabled || m_router->GetCurrentNet() < 0 || !m_startItem )
    {
        m_endItem = NULL;
        m_endSnapPoint = cp;
        return;
    }

    bool dummy;

    if( m_router->IsPlacingVia() )
        layer = -1;
    else
        layer = m_router->GetCurrentLayer();

    PNS_ITEM* endItem = pickSingleItem( p, m_startItem->Net(), layer );

    if( endItem )
    {
        VECTOR2I cursorPos = m_router->SnapToItem( endItem, p, dummy );
        m_ctls->ForceCursorPosition( true, cursorPos );
        m_endItem = endItem;
        m_endSnapPoint = cursorPos;
    }
    else
    {
        m_endItem = NULL;
        m_endSnapPoint = cp;
        m_ctls->ForceCursorPosition( false );
    }

    if( m_endItem )
        TRACE( 0, "%s, layer : %d", m_endItem->KindStr().c_str() % m_endItem->Layers().Start() );
}

