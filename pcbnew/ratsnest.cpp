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
#include <pcb_edit_frame.h>
#include <macros.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>

#include <pcbnew.h>

#include <ratsnest_data.h>

/**
 * Function Compile_Ratsnest
 *  Create the entire board ratsnest.
 *  Must be called after a board change (changes for
 *  pads, footprints or a read netlist ).
 * @param aDC = the current device context (can be NULL)
 * @param aDisplayStatus : if true, display the computation results
 */
void PCB_BASE_FRAME::Compile_Ratsnest( bool aDisplayStatus )
{
    GetBoard()->GetConnectivity()->RecalculateRatsnest();
    GetBoard()->m_Status_Pcb = 0;   // we want a full ratsnest computation, from the scratch

    ClearMsgPanel();

    if( aDisplayStatus )
    {
        std::shared_ptr<CONNECTIVITY_DATA> conn = m_Pcb->GetConnectivity();
        wxString                           msg;

        msg.Printf( wxT( " %d" ), conn->GetPadCount() );
        AppendMsgPanel( _( "Pads" ), msg, RED );

        msg.Printf( wxT( " %d" ), conn->GetNetCount() - 1 /* Don't include "No Net" in count */ );
        AppendMsgPanel( _( "Nets" ), msg, CYAN );

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
    // JEY TODO: probalby obsolete (we don't really have DCs anymore)
    if( ( m_Pcb->m_Status_Pcb & DO_NOT_SHOW_GENERAL_RASTNEST ) )
    {
        return;
    }

    if( aDC == NULL )
        return;

    auto connectivity = m_Pcb->GetConnectivity();

    std::unique_lock<std::mutex> lock( connectivity->GetLock(), std::try_to_lock );

    if( !lock )
        return;

    COLOR4D color = Settings().Colors().GetItemColor( LAYER_RATSNEST );

    auto displ_opts = (PCB_DISPLAY_OPTIONS*) GetDisplayOptions();

    const bool curved_ratsnest = displ_opts->m_DisplayRatsnestLinesCurved;

    for( int i = 1 /* skip "No Net" at [0] */; i < connectivity->GetNetCount(); ++i )
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

                if( !sn->Valid() || !dn->Valid() )
                    continue;

                bool enable = !sn->GetNoLine() && !dn->GetNoLine();
                bool show = sn->Parent()->GetLocalRatsnestVisible()
                            || dn->Parent()->GetLocalRatsnestVisible();

                if( enable && show )
                {
                    if (curved_ratsnest) {
                        auto dx = d.x - s.x;
                        auto dy = d.y - s.y;
                        auto cx = s.x + 0.5 * dx + 1.2 * dy;
                        auto cy = s.y + 0.5 * dy - 1.2 * dx;
                        GRArc1( m_canvas->GetClipBox(), aDC, s.x, s.y, d.x, d.y, cx, cy, 0, color);
                    } else {
                        GRLine( m_canvas->GetClipBox(), aDC, s.x, s.y, d.x, d.y, 0, color );
                    }
                }
            }
        }
    }
}

