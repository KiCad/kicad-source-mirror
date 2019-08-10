/**
 * @file pcbnew/netlist.cpp
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013-2016 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2016 KiCad Developers, see change_log.txt for contributors.
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

#include <functional>
using namespace std::placeholders;

#include <fctsys.h>
#include <pgm_base.h>
#include <class_draw_panel_gal.h>
#include <confirm.h>
#include <dialog_helpers.h>
#include <pcb_edit_frame.h>
#include <pcb_netlist.h>
#include <netlist_reader.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>
#include <lib_id.h>
#include <fp_lib_table.h>
#include <class_board.h>
#include <class_module.h>
#include <ratsnest_data.h>
#include <pcbnew.h>
#include <io_mgr.h>
#include <board_netlist_updater.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/selection_tool.h>
#include <view/view.h>

extern void SpreadFootprints( std::vector<MODULE*>* aFootprints,
                              wxPoint aSpreadAreaPosition );


bool PCB_EDIT_FRAME::ReadNetlistFromFile( const wxString &aFilename,
                                          NETLIST& aNetlist,
                                          REPORTER& aReporter )
{
    wxString msg;

    try
    {
        std::unique_ptr<NETLIST_READER> netlistReader( NETLIST_READER::GetNetlistReader(
                &aNetlist, aFilename, wxEmptyString ) );

        if( !netlistReader.get() )
        {
            msg.Printf( _( "Cannot open netlist file \"%s\"." ), GetChars( aFilename ) );
            wxMessageBox( msg, _( "Netlist Load Error." ), wxOK | wxICON_ERROR, this );
            return false;
        }

        SetLastPath( LAST_PATH_NETLIST, aFilename );
        netlistReader->LoadNetlist();
        LoadFootprints( aNetlist, aReporter );
    }
    catch( const IO_ERROR& ioe )
    {
        msg.Printf( _( "Error loading netlist.\n%s" ), ioe.What().GetData() );
        wxMessageBox( msg, _( "Netlist Load Error" ), wxOK | wxICON_ERROR );
        return false;
    }

    SetLastPath( LAST_PATH_NETLIST, aFilename );

    return true;
}


void PCB_EDIT_FRAME::OnNetlistChanged( BOARD_NETLIST_UPDATER& aUpdater, bool* aRunDragCommand )
{
    BOARD* board = GetBoard();

    SetMsgPanel( board );

    // Update rendered tracks and vias net labels
    // TODO is there a way to extract information about which nets were modified?
    for( auto track : board->Tracks() )
        GetCanvas()->GetView()->Update( track );

    std::vector<MODULE*> newFootprints = aUpdater.GetAddedComponents();

    // Spread new footprints.
    wxPoint areaPosition = (wxPoint) GetCanvas()->GetViewControls()->GetCursorPosition();
    EDA_RECT bbox = board->GetBoundingBox();

    GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

    SpreadFootprints( &newFootprints, areaPosition );

    // Start drag command for new modules
    if( !newFootprints.empty() )
    {
        for( MODULE* footprint : newFootprints )
            GetToolManager()->RunAction( PCB_ACTIONS::selectItem, true, footprint );

        *aRunDragCommand = true;

        // Now fix a reference point to move the footprints.
        // We use the first footprint in list as reference point
        // The graphic cursor will be on this fp when moving the footprints.
        SELECTION_TOOL*   selTool = GetToolManager()->GetTool<SELECTION_TOOL>();
        PCBNEW_SELECTION& selection = selTool->GetSelection();
        selection.SetReferencePoint( newFootprints[0]->GetPosition() );
    }

    GetCanvas()->Refresh();
}


#define ALLOW_PARTIAL_FPID      1

void PCB_EDIT_FRAME::LoadFootprints( NETLIST& aNetlist, REPORTER& aReporter )
{
    wxString   msg;
    LIB_ID     lastFPID;
    COMPONENT* component;
    MODULE*    module = 0;
    MODULE*    fpOnBoard;

    if( aNetlist.IsEmpty() || Prj().PcbFootprintLibs()->IsEmpty() )
        return;

    aNetlist.SortByFPID();

    for( unsigned ii = 0; ii < aNetlist.GetCount(); ii++ )
    {
        component = aNetlist.GetComponent( ii );

#if ALLOW_PARTIAL_FPID
        // The FPID is ok as long as there is a footprint portion coming
        // from eeschema.
        if( !component->GetFPID().GetLibItemName().size() )
#else
        if( component->GetFPID().empty() )
#endif
        {
            msg.Printf( _( "No footprint defined for symbol \"%s\".\n" ),
                        component->GetReference() );
            aReporter.Report( msg, REPORTER::RPT_ERROR );

            continue;
        }

        // Check if component footprint is already on BOARD and only load the footprint from
        // the library if it's needed.  Nickname can be blank.
        if( aNetlist.IsFindByTimeStamp() )
            fpOnBoard = m_Pcb->FindModule( aNetlist.GetComponent( ii )->GetTimeStamp(), true );
        else
            fpOnBoard = m_Pcb->FindModule( aNetlist.GetComponent( ii )->GetReference() );

        bool footprintMisMatch = fpOnBoard && fpOnBoard->GetFPID() != component->GetFPID();

        if( footprintMisMatch && !aNetlist.GetReplaceFootprints() )
        {
            msg.Printf( _( "Footprint of %s changed: board footprint \"%s\", netlist footprint \"%s\"." ),
                        component->GetReference(),
                        GetChars( fpOnBoard->GetFPID().Format() ),
                        GetChars( component->GetFPID().Format() ) );
            aReporter.Report( msg, REPORTER::RPT_WARNING );

            continue;
        }

        if( !aNetlist.GetReplaceFootprints() )
            footprintMisMatch = false;

        if( fpOnBoard && !footprintMisMatch )   // nothing else to do here
            continue;

        if( component->GetFPID() != lastFPID )
        {
            module = NULL;

#if ALLOW_PARTIAL_FPID
            // The LIB_ID is ok as long as there is a footprint portion coming
            // the library if it's needed.  Nickname can be blank.
            if( !component->GetFPID().GetLibItemName().size() )
#else
            if( !component->GetFPID().IsValid() )
#endif
            {
                msg.Printf( _( "%s footprint ID \"%s\" is not valid." ),
                            component->GetReference(),
                            GetChars( component->GetFPID().Format() ) );
                aReporter.Report( msg, REPORTER::RPT_ERROR );

                continue;
            }

            // loadFootprint() can find a footprint with an empty nickname in fpid.
            module = PCB_BASE_FRAME::loadFootprint( component->GetFPID() );

            if( module )
            {
                lastFPID = component->GetFPID();
            }
            else
            {
                msg.Printf( _( "%s footprint \"%s\" not found in any libraries in the footprint library table.\n" ),
                            component->GetReference(),
                            GetChars( component->GetFPID().GetLibItemName() ) );
                aReporter.Report( msg, REPORTER::RPT_ERROR );

                continue;
            }
        }
        else
        {
            // Footprint already loaded from a library, duplicate it (faster)
            if( module == NULL )
                continue;            // Module does not exist in any library.

            module = new MODULE( *module );
        }

        if( module )
            component->SetModule( module );
    }
}
