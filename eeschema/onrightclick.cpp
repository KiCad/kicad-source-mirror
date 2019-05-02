/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <eeschema_id.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <menus_helpers.h>

#include <advanced_config.h>
#include <class_library.h>
#include <connection_graph.h>
#include <general.h>
#include <hotkeys.h>
#include <sch_bus_entry.h>
#include <sch_junction.h>
#include <sch_component.h>
#include <sch_line.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <symbol_lib_table.h>
#include <sch_view.h>

#include <tool/tool_manager.h>
#include <tools/sch_actions.h>
#include <tools/sch_selection_tool.h>

void AddMenusForComponent( wxMenu* PopMenu, SCH_COMPONENT* Component, SYMBOL_LIB_TABLE* aLibs )
{
    wxString    msg;
    LIB_PART*   part = NULL;
    LIB_ALIAS*  alias = NULL;

    try
    {
        alias = aLibs->LoadSymbol( Component->GetLibId() );
    }
    catch( ... )
    {
    }

    if( alias )
        part = alias->GetPart();

    wxMenu* editmenu = new wxMenu;

    if( part && part->GetUnitCount() >= 2 )
    {
        wxMenu* sel_unit_menu = new wxMenu; int ii;

        for( ii = 0; ii < part->GetUnitCount(); ii++ )
        {
            wxString num_unit;
            int unit = Component->GetUnit();
            num_unit.Printf( _( "Unit %s" ), GetChars( LIB_PART::SubReference(  ii + 1, false ) ) );
            wxMenuItem * item = sel_unit_menu->Append( ID_POPUP_SCH_SELECT_UNIT1 + ii,
                                                       num_unit, wxEmptyString,
                                                       wxITEM_CHECK );
            if( unit == ii + 1 )
                item->Check(true);

            // The ID max for these submenus is ID_POPUP_SCH_SELECT_UNIT_CMP_MAX
            // See eeschema_id to modify this value.
            if( ii >= (ID_POPUP_SCH_SELECT_UNIT_CMP_MAX - ID_POPUP_SCH_SELECT_UNIT1) )
                break;      // We have used all IDs for these submenus
        }

        AddMenuItem( editmenu, sel_unit_menu, ID_POPUP_SCH_SELECT_UNIT_CMP,
                     _( "Unit" ), KiBitmap( component_select_unit_xpm ) );
    }
}


void AddMenusForBus( wxMenu* PopMenu, SCH_LINE* Bus, SCH_EDIT_FRAME* frame )
{
    SCH_SELECTION_TOOL* selTool = frame->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();

    // TODO(JE) remove once real-time is enabled
    if( !ADVANCED_CFG::GetCfg().m_realTimeConnectivity || !CONNECTION_GRAPH::m_allowRealTime )
    {
        frame->RecalculateConnections();

        // Have to pick up the pointer again because it may have been changed by SchematicCleanUp
        KICAD_T    busType[] = { SCH_LINE_LOCATE_BUS_T, EOT };
        SELECTION& selection = selTool->RequestSelection( busType );
        Bus = (SCH_LINE*) selection.Front();
        wxASSERT( Bus );
    }

    // Bus unfolding menu (only available if bus is properly defined)
    wxMenu* bus_unfold_menu = frame->GetUnfoldBusMenu( Bus );

    if( bus_unfold_menu )
        PopMenu->AppendSubMenu( bus_unfold_menu, _( "Unfold Bus" ) );
}


