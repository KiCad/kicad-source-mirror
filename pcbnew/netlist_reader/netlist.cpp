/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013-2016 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <confirm.h>
#include <kiway.h>
#include <drc/drc_engine.h>
#include <pcb_edit_frame.h>
#include <netlist_reader/pcb_netlist.h>
#include <netlist_reader/netlist_reader.h>
#include <reporter.h>
#include <lib_id.h>
#include <footprint_library_adapter.h>
#include <board.h>
#include <footprint.h>
#include <spread_footprints.h>
#include <ratsnest/ratsnest_data.h>
#include <pcb_io/pcb_io_mgr.h>
#include "board_netlist_updater.h"
#include <tool/tool_manager.h>
#include <tools/drc_tool.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <project/project_file.h>  // LAST_PATH_TYPE
#include <project_pcb.h>


bool PCB_EDIT_FRAME::ReadNetlistFromFile( const wxString &aFilename, NETLIST& aNetlist,
                                          REPORTER& aReporter )
{
    wxString msg;

    try
    {
        std::unique_ptr<NETLIST_READER> netlistReader( NETLIST_READER::GetNetlistReader(
                &aNetlist, aFilename, wxEmptyString ) );

        if( !netlistReader.get() )
        {
            msg.Printf( _( "Cannot open netlist file '%s'." ), aFilename );
            DisplayErrorMessage( this, msg );
            return false;
        }

        SetLastPath( LAST_PATH_NETLIST, aFilename );
        netlistReader->LoadNetlist();
        LoadFootprints( aNetlist, aReporter );
    }
    catch( const IO_ERROR& ioe )
    {
        msg.Printf( _( "Error loading netlist.\n%s" ), ioe.What().GetData() );
        DisplayErrorMessage( this, msg );
        return false;
    }

    SetLastPath( LAST_PATH_NETLIST, aFilename );

    return true;
}


void PCB_EDIT_FRAME::OnNetlistChanged( BOARD_NETLIST_UPDATER& aUpdater, bool* aRunDragCommand )
{
    BOARD* board = GetBoard();

    SetMsgPanel( board );

    // Re-sync nets and  netclasses
    board->SynchronizeNetsAndNetClasses( false );

    // Recompute component classes
    board->GetComponentClassManager().InvalidateComponentClasses();
    board->GetComponentClassManager().RebuildRequiredCaches();

    // Resync DRC rules to account for new aggregate netclass / component class rules
    DRC_TOOL* drcTool = m_toolManager->GetTool<DRC_TOOL>();

    try
    {
        drcTool->GetDRCEngine()->InitEngine( GetDesignRulesPath() );
    }
    catch( PARSE_ERROR& )
    {
    }

    // Update rendered track/via/pad net labels, and any text items that might reference a
    // netName or netClass
    int netNamesCfg = GetPcbNewSettings()->m_Display.m_NetNames;

    GetCanvas()->GetView()->UpdateAllItemsConditionally(
            [&]( KIGFX::VIEW_ITEM* aItem ) -> int
            {
                if( dynamic_cast<PCB_TRACK*>( aItem ) )
                {
                    if( netNamesCfg == 2 || netNamesCfg == 3 )
                        return KIGFX::REPAINT;
                }
                else if( dynamic_cast<PAD*>( aItem ) )
                {
                    if( netNamesCfg == 1 || netNamesCfg == 3 )
                        return KIGFX::REPAINT;
                }

                EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem );

                if( text && text->HasTextVars() )
                {
                    text->ClearRenderCache();
                    text->ClearBoundingBoxCache();
                    return KIGFX::GEOMETRY | KIGFX::REPAINT;
                }

                return 0;
            } );

    // Spread new footprints.
    std::vector<FOOTPRINT*> newFootprints = aUpdater.GetAddedFootprints();

    GetToolManager()->RunAction( ACTIONS::selectionClear );

    SpreadFootprints( &newFootprints, { 0, 0 }, true );

    // Start drag command for new footprints
    if( !newFootprints.empty() )
    {
        EDA_ITEMS items;
        std::copy( newFootprints.begin(), newFootprints.end(), std::back_inserter( items ) );
        GetToolManager()->RunAction<EDA_ITEMS*>( ACTIONS::selectItems, &items );

        *aRunDragCommand = true;
    }

    Compile_Ratsnest( true );

    UpdateVariantSelectionCtrl();

    GetCanvas()->Refresh();
}


void PCB_EDIT_FRAME::LoadFootprints( NETLIST& aNetlist, REPORTER& aReporter )
{
    wxString   msg;
    LIB_ID     lastFPID;
    COMPONENT* component;
    FOOTPRINT* footprint = nullptr;
    FOOTPRINT* fpOnBoard = nullptr;

    if( aNetlist.IsEmpty() || PROJECT_PCB::FootprintLibAdapter( &Prj() )->Rows().empty() )
        return;

    aNetlist.SortByFPID();

    for( unsigned ii = 0; ii < aNetlist.GetCount(); ii++ )
    {
        component = aNetlist.GetComponent( ii );

        // The FPID is ok as long as there is a footprint portion coming from eeschema.
        if( !component->GetFPID().GetLibItemName().size() )
        {
            msg.Printf( _( "No footprint defined for symbol %s." ),
                        component->GetReference() );
            aReporter.Report( msg, RPT_SEVERITY_ERROR );

            continue;
        }

        // Check if component footprint is already on BOARD and only load the footprint from
        // the library if it's needed.  Nickname can be blank.
        if( aNetlist.IsFindByTimeStamp() )
        {
            for( const KIID& uuid : component->GetKIIDs() )
            {
                KIID_PATH path = component->GetPath();
                path.push_back( uuid );

                if( ( fpOnBoard = m_pcb->FindFootprintByPath( path ) ) != nullptr )
                    break;
            }
        }
        else
            fpOnBoard = m_pcb->FindFootprintByReference( component->GetReference() );

        bool footprintMisMatch = fpOnBoard && fpOnBoard->GetFPID() != component->GetFPID();

        if( footprintMisMatch && !aNetlist.GetReplaceFootprints() )
        {
            msg.Printf( _( "Footprint of %s changed: board footprint '%s', netlist footprint '%s'." ),
                        component->GetReference(),
                        fpOnBoard->GetFPID().Format().wx_str(),
                        component->GetFPID().Format().wx_str() );
            aReporter.Report( msg, RPT_SEVERITY_WARNING );

            continue;
        }

        if( !aNetlist.GetReplaceFootprints() )
            footprintMisMatch = false;

        if( fpOnBoard && !footprintMisMatch )   // nothing else to do here
            continue;

        if( component->GetFPID() != lastFPID )
        {
            footprint = nullptr;

            // The LIB_ID is ok as long as there is a footprint portion coming the library if
            // it's needed.  Nickname can be blank.
            if( !component->GetFPID().GetLibItemName().size() )
            {
                msg.Printf( _( "%s footprint ID '%s' is not valid." ),
                            component->GetReference(),
                            component->GetFPID().Format().wx_str() );
                aReporter.Report( msg, RPT_SEVERITY_ERROR );

                continue;
            }

            // loadFootprint() can find a footprint with an empty nickname in fpid.
            footprint = PCB_BASE_FRAME::loadFootprint( component->GetFPID() );

            if( footprint )
            {
                lastFPID = component->GetFPID();
            }
            else
            {
                msg.Printf( _( "%s footprint '%s' not found in any libraries in the footprint "
                               "library table." ),
                            component->GetReference(),
                            component->GetFPID().GetLibItemName().wx_str() );
                aReporter.Report( msg, RPT_SEVERITY_ERROR );

                continue;
            }
        }
        else
        {
            // Footprint already loaded from a library, duplicate it (faster)
            if( !footprint )
                continue;            // Footprint does not exist in any library.

            footprint = new FOOTPRINT( *footprint );
            const_cast<KIID&>( footprint->m_Uuid ) = KIID();
        }

        if( footprint )
            component->SetFootprint( footprint );
    }
}
