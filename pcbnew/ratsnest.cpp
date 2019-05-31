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

#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
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

