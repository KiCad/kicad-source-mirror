/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 *
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
 * @file move_and_route_event_functions.cpp
 * @brief Routines for automatic displacement and rotation of modules.
 */

#include <algorithm>

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <kicad_string.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <kicad_device_context.h>

#include <autorout.h>
#include <cell.h>
#include <pcbnew_id.h>
#include <class_board.h>
#include <class_module.h>


typedef enum {
    FIXE_MODULE,
    FREE_MODULE,
    FIXE_ALL_MODULES,
    FREE_ALL_MODULES
} SelectFixeFct;



wxString ModulesMaskSelection = wxT( "*" );


/* Called on events (popup menus) relative to automove and autoplace footprints
 */
void PCB_EDIT_FRAME::OnPlaceOrRouteFootprints( wxCommandEvent& event )
{
    int        id = event.GetId();

    if( m_mainToolBar == NULL )
        return;

    INSTALL_UNBUFFERED_DC( dc, m_canvas );

    switch( id )
    {
    case ID_POPUP_PCB_AUTOROUTE_SELECT_LAYERS:
        return;

    case ID_POPUP_PCB_AUTOPLACE_FIXE_MODULE:
        LockModule( (MODULE*) GetScreen()->GetCurItem(), true );
        return;

    case ID_POPUP_PCB_AUTOPLACE_FREE_MODULE:
        LockModule( (MODULE*) GetScreen()->GetCurItem(), false );
        return;

    case ID_POPUP_PCB_AUTOPLACE_FREE_ALL_MODULES:
        LockModule( NULL, false );
        return;

    case ID_POPUP_PCB_AUTOPLACE_FIXE_ALL_MODULES:
        LockModule( NULL, true );
        return;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        if( m_canvas->IsMouseCaptured() )
        {
            m_canvas->CallEndMouseCapture( &dc );
        }

        break;

    default:   // Abort a current command (if any)
        m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );
        break;
    }

    // Erase ratsnest if needed
    if( GetBoard()->IsElementVisible(RATSNEST_VISIBLE) )
        DrawGeneralRatsnest( &dc );

    GetBoard()->m_Status_Pcb |= DO_NOT_SHOW_GENERAL_RASTNEST;

    switch( id )
    {
    case ID_POPUP_PCB_AUTOPLACE_CURRENT_MODULE:
        AutoPlaceModule( (MODULE*) GetScreen()->GetCurItem(), PLACE_1_MODULE, &dc );
        break;

    case ID_POPUP_PCB_AUTOPLACE_ALL_MODULES:
        AutoPlaceModule( NULL, PLACE_ALL, &dc );
        break;

    case ID_POPUP_PCB_AUTOPLACE_NEW_MODULES:
        AutoPlaceModule( NULL, PLACE_OUT_OF_BOARD, &dc );
        break;

    case ID_POPUP_PCB_AUTOPLACE_NEXT_MODULE:
        AutoPlaceModule( NULL, PLACE_INCREMENTAL, &dc );
        break;

    case ID_POPUP_PCB_SPREAD_ALL_MODULES:
        if( !IsOK( this,
                   _("Not locked footprints inside the board will be moved. OK?") ) )
            break;
        // Fall through
    case ID_POPUP_PCB_SPREAD_NEW_MODULES:
        if( GetBoard()->m_Modules == NULL )
        {
            DisplayError( this, _( "No modules found!" ) );
            return;
        }

        SpreadFootprints( id == ID_POPUP_PCB_SPREAD_NEW_MODULES );
        break;

    case ID_POPUP_PCB_AUTOROUTE_ALL_MODULES:
        Autoroute( &dc, ROUTE_ALL );
        break;

    case ID_POPUP_PCB_AUTOROUTE_MODULE:
        Autoroute( &dc, ROUTE_MODULE );
        break;

    case ID_POPUP_PCB_AUTOROUTE_PAD:
        Autoroute( &dc, ROUTE_PAD );
        break;

    case ID_POPUP_PCB_AUTOROUTE_NET:
        Autoroute( &dc, ROUTE_NET );
        break;

    case ID_POPUP_PCB_AUTOROUTE_RESET_UNROUTED:
        Reset_Noroutable( &dc );
        break;

    default:
        wxMessageBox( wxT( "OnPlaceOrRouteFootprints command error" ) );
        break;
    }

    GetBoard()->m_Status_Pcb &= ~DO_NOT_SHOW_GENERAL_RASTNEST;
    Compile_Ratsnest( &dc, true );
}


/* Set or reset (true or false) Lock attribute of aModule or all modules if aModule == NULL
 */
void PCB_EDIT_FRAME::LockModule( MODULE* aModule, bool aLocked )
{
    if( aModule )
    {
        aModule->SetLocked( aLocked );
        SetMsgPanel( aModule );
        OnModify();
    }
    else
    {
        aModule = GetBoard()->m_Modules;

        for( ; aModule != NULL; aModule = aModule->Next() )
        {
            if( WildCompareString( ModulesMaskSelection, aModule->GetReference() ) )
            {
                aModule->SetLocked( aLocked );
                OnModify();
            }
        }
    }
}

