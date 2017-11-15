/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file ratsnest.cpp
 * @brief Ratsnets functions.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <class_drawpanel.h>
#include <wxBasePcbFrame.h>
#include <macros.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>

#include <pcbnew.h>

#include <connectivity_data.h>
#include <ratsnest_data.h>

#include <wxPcbStruct.h>

/**
 * Function Compile_Ratsnest
 *  Create the entire board ratsnest.
 *  Must be called after a board change (changes for
 *  pads, footprints or a read netlist ).
 * @param aDC = the current device context (can be NULL)
 * @param aDisplayStatus : if true, display the computation results
 */
void PCB_BASE_FRAME::Compile_Ratsnest( wxDC* aDC, bool aDisplayStatus )
{
    GetBoard()->GetConnectivity()->RecalculateRatsnest();

    GetBoard()->m_Status_Pcb = 0;   // we want a full ratsnest computation, from the scratch

    if( GetBoard()->IsElementVisible( LAYER_RATSNEST ) && aDC )
        DrawGeneralRatsnest( aDC, 0 );

    wxString msg;

    ClearMsgPanel();

    if( aDisplayStatus )
    {
        msg.Printf( wxT( " %d" ), m_Pcb->GetConnectivity()->GetPadCount() );
        AppendMsgPanel( wxT( "Pads" ), msg, RED );
        msg.Printf( wxT( " %d" ), m_Pcb->GetConnectivity()->GetNetCount() );
        AppendMsgPanel( wxT( "Nets" ), msg, CYAN );
        SetMsgPanel( m_Pcb );
    }
}


/**
 *  function DrawGeneralRatsnest
 *  Only ratsnest items with the status bit CH_VISIBLE set are displayed
 * @param aDC = the current device context (can be NULL)
 * @param aNetcode: if > 0, Display only the ratsnest relative to the
 * corresponding net_code
 */
void PCB_BASE_FRAME::DrawGeneralRatsnest( wxDC* aDC, int aNetcode )
{
    if( ( m_Pcb->m_Status_Pcb & DO_NOT_SHOW_GENERAL_RASTNEST ) )
    {
        return;
    }

    if( aDC == NULL )
        return;

    auto connectivity = m_Pcb->GetConnectivity();

    COLOR4D color = Settings().Colors().GetItemColor( LAYER_RATSNEST );

    for( int i = 1; i < connectivity->GetNetCount(); ++i )
    {
        RN_NET* net = connectivity->GetRatsnestForNet( i );

        if( !net )
            continue;

        if( ( aNetcode <= 0 ) || ( aNetcode == i ) )
        {
            for( const auto& edge : net->GetEdges() )
            {
                auto s = edge.GetSourcePos();
                auto d = edge.GetTargetPos();
                auto sn = edge.GetSourceNode();
                auto dn = edge.GetTargetNode();

                bool enable = !sn->GetNoLine() && !dn->GetNoLine();
                bool show = sn->Parent()->GetLocalRatsnestVisible()
                            || dn->Parent()->GetLocalRatsnestVisible();

                if( enable && show )
                    GRLine( m_canvas->GetClipBox(), aDC, wxPoint( s.x, s.y ), wxPoint( d.x,
                                    d.y ), 0, color );
            }
        }
    }
}


void PCB_BASE_FRAME::TraceModuleRatsNest( wxDC* DC )
{
    if( DC == NULL )
        return;

    COLOR4D tmpcolor = Settings().Colors().GetItemColor( LAYER_RATSNEST );

    for( const auto& l : GetBoard()->GetConnectivity()->GetDynamicRatsnest() )
    {
        GRLine( m_canvas->GetClipBox(), DC, wxPoint( l.a.x, l.a.y ), wxPoint( l.b.x,
                        l.b.y ), 0, tmpcolor );
    }
}


/*
 * PCB_BASE_FRAME::BuildAirWiresTargetsList and
 * PCB_BASE_FRAME::TraceAirWiresToTargets
 * are 2 function to show the near connecting points when
 * a new track is created, by displaying g_MaxLinksShowed airwires
 * between the on grid mouse cursor and these connecting points
 * during the creation of a track
 */

/* Buffer to store pads coordinates when creating a track.
 *  these pads are members of the net
 *  and when the mouse is moved, the g_MaxLinksShowed links to neighbors are
 * drawn
 */

static wxPoint s_CursorPos;     // Coordinate of the moving point (mouse cursor and
                                // end of current track segment)

/* Function BuildAirWiresTargetsList
 * Build a list of candidates that can be a coonection point
 * when a track is started.
 * This functions prepares data to show airwires to nearest connecting points (pads)
 * from the current new track to candidates during track creation
 */

static BOARD_CONNECTED_ITEM* s_ref = nullptr;
static int s_refNet = -1;

void PCB_BASE_FRAME::BuildAirWiresTargetsList( BOARD_CONNECTED_ITEM* aItemRef,
        const wxPoint& aPosition, int aNet )
{
    s_CursorPos = aPosition;    // needed for sort_by_distance
    s_ref = aItemRef;
    s_refNet = aNet;
}


static MODULE movedModule( nullptr );

void PCB_BASE_FRAME::build_ratsnest_module( MODULE* aModule, wxPoint aMoveVector )
{
    auto connectivity = GetBoard()->GetConnectivity();

    movedModule = *aModule;
    movedModule.Move( -aMoveVector );
    connectivity->ClearDynamicRatsnest();
    connectivity->BlockRatsnestItems( { aModule } );
    connectivity->ComputeDynamicRatsnest( { &movedModule } );
}


void PCB_BASE_FRAME::TraceAirWiresToTargets( wxDC* aDC )
{
    auto connectivity = GetBoard()->GetConnectivity();
    auto displ_opts = (PCB_DISPLAY_OPTIONS*) GetDisplayOptions();

    auto targets = connectivity->NearestUnconnectedTargets( s_ref, s_CursorPos, s_refNet );

    if( aDC == NULL )
        return;

    GRSetDrawMode( aDC, GR_XOR );

    for( int i = 0; i < std::min( (int) displ_opts->m_MaxLinksShowed, (int) targets.size() ); i++ )
    {
        auto p = targets[i];
        GRLine( m_canvas->GetClipBox(), aDC, s_CursorPos, wxPoint( p.x, p.y ), 0, YELLOW );
    }
}


// Redraw in XOR mode the outlines of the module.
void MODULE::DrawOutlinesWhenMoving( EDA_DRAW_PANEL* panel, wxDC* DC,
        const wxPoint& aMoveVector )
{
    int pad_fill_tmp;
    D_PAD* pt_pad;

    DrawEdgesOnly( panel, DC, aMoveVector, GR_XOR );
    auto displ_opts = (PCB_DISPLAY_OPTIONS*) ( panel->GetDisplayOptions() );

    // Show pads in sketch mode to speedu up drawings
    pad_fill_tmp = displ_opts->m_DisplayPadFill;
    displ_opts->m_DisplayPadFill = true;

    pt_pad = PadsList();

    for( ; pt_pad != NULL; pt_pad = pt_pad->Next() )
        pt_pad->Draw( panel, DC, GR_XOR, aMoveVector );

    displ_opts->m_DisplayPadFill = pad_fill_tmp;

    if( displ_opts->m_Show_Module_Ratsnest )
    {
        PCB_BASE_FRAME* frame = (PCB_BASE_FRAME*) panel->GetParent();
        frame->build_ratsnest_module( this, aMoveVector );
        frame->TraceModuleRatsNest( DC );
    }
}


void PCB_EDIT_FRAME::Show_1_Ratsnest( EDA_ITEM* item, wxDC* DC )
{
    if( item && item->Type() == PCB_MODULE_T )
    {
        auto mod = static_cast<MODULE*> (item);

        for( auto pad : mod->Pads() )
        {
            pad->SetLocalRatsnestVisible( true );
        }

        m_canvas->Refresh();
    }
}
