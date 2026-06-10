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
#include <drc/drc_engine.h>
#include <kiway.h>
#include <pcb_edit_frame.h>
#include <netlist_reader/pcb_netlist.h>
#include <netlist_reader/netlist_reader.h>
#include <netlist_reader/pcb_netlist_utils.h>
#include <reporter.h>
#include <board.h>
#include <component_classes/component_class_manager.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <spread_footprints.h>
#include "board_netlist_updater.h"
#include <tool/tool_manager.h>
#include <tools/drc_tool.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <project/project_file.h>  // LAST_PATH_TYPE


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
        LoadNetlistFootprints( GetBoard(), aNetlist, aReporter );
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
        drcTool->GetDRCEngine()->InitEngine( board->GetDesignRulesPath() );
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

    board->CompileRatsnest();
    SetMsgPanel( board );

    UpdateVariantSelectionCtrl();

    GetCanvas()->Refresh();
}
