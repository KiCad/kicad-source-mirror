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
#include <confirm.h>
#include <sch_edit_frame.h>
#include <menus_helpers.h>

#include <advanced_config.h>
#include <class_library.h>
#include <connection_graph.h>
#include <general.h>
#include <hotkeys.h>
#include <netlist_object.h>
#include <sch_bus_entry.h>
#include <sch_marker.h>
#include <sch_text.h>
#include <sch_junction.h>
#include <sch_component.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_bitmap.h>
#include <symbol_lib_table.h>
#include <sch_connection.h>
#include <sch_view.h>

#include <iostream>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>
#include <tools/sch_selection_tool.h>

static void AddMenusForWire( wxMenu* PopMenu, SCH_LINE* Wire, SCH_EDIT_FRAME* frame );
static void AddMenusForBus( wxMenu* PopMenu, SCH_LINE* Bus, SCH_EDIT_FRAME* frame );
static void AddMenusForHierchicalSheet( wxMenu* PopMenu, SCH_SHEET* Sheet );
static void AddMenusForComponent( wxMenu* PopMenu, SCH_COMPONENT* Component,
                                  SYMBOL_LIB_TABLE* aLibs );
static void AddMenusForMarkers( wxMenu* aPopMenu, SCH_MARKER* aMarker, SCH_EDIT_FRAME* aFrame );


bool SCH_EDIT_FRAME::OnRightClick( const wxPoint& aPosition, wxMenu* PopMenu )
{
    SCH_SELECTION_TOOL* selTool = GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    SCH_ITEM*           item = GetScreen()->GetCurItem();
    wxString            msg;

    // Ugly hack, clear any highligthed symbol, because the HIGHLIGHT flag create issues when creating menus
    // Will be fixed later
    GetCanvas()->GetView()->HighlightItem( nullptr, nullptr );

    // Do not start a block command on context menu.
    m_canvas->SetCanStartBlock( -1 );

    // Try to locate items at cursor position.
    if( item == NULL || item->GetEditFlags() == 0 )
    {
        bool actionCancelled = false;
        item = selTool->SelectPoint( aPosition, SCH_COLLECTOR::AllItemsButPins, &actionCancelled );

        // If the clarify item selection context menu is aborted, don't show the context menu.
        if( item == NULL && actionCancelled )
            return false;
    }

    // If a command is in progress: add "cancel" and "end tool" menu
    if( GetToolId() != ID_NO_TOOL_SELECTED )
    {
        switch( GetToolId() )
        {
        case ID_WIRE_BUTT:
            AddMenusForWire( PopMenu, NULL, this );
            break;

        case ID_BUS_BUTT:
            AddMenusForBus( PopMenu, NULL, this );
            break;

        default:
            break;
        }
    }

    if( item == NULL )
    {
        if( g_CurrentSheet->Last() != g_RootSheet )
        {
            msg = AddHotkeyName( _( "Leave Sheet" ), g_Schematic_Hotkeys_Descr, HK_LEAVE_SHEET );
            AddMenuItem( PopMenu, ID_POPUP_SCH_LEAVE_SHEET, msg, KiBitmap( leave_sheet_xpm ) );
        }

        return true;
    }

    switch( item->Type() )
    {
    case SCH_NO_CONNECT_T:
        AddMenuItem( PopMenu, ID_SCH_DELETE, _( "Delete No Connect" ),
                     KiBitmap( delete_xpm ) );
        break;

    case SCH_JUNCTION_T:
        addJunctionMenuEntries( PopMenu, (SCH_JUNCTION*) item );
        break;

    case SCH_MARKER_T:
        AddMenusForMarkers( PopMenu, (SCH_MARKER*) item, this );
        break;

    case SCH_COMPONENT_T:
        AddMenusForComponent( PopMenu, (SCH_COMPONENT*) item, Prj().SchSymbolLibTable() );
        break;

    case SCH_LINE_T:
        switch( item->GetLayer() )
        {
        case LAYER_WIRE:
            AddMenusForWire( PopMenu, (SCH_LINE*) item, this );
            break;

        case LAYER_BUS:
            AddMenusForBus( PopMenu, (SCH_LINE*) item, this );
            break;
        }
        break;

    case SCH_SHEET_T:
        AddMenusForHierchicalSheet( PopMenu, (SCH_SHEET*) item );
        break;
    }

    PopMenu->AppendSeparator();
    return true;
}


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

    if( part && part->HasConversion() )
        AddMenuItem( editmenu, ID_POPUP_SCH_EDIT_CONVERT_CMP, _( "Convert" ),
                     KiBitmap( component_select_alternate_shape_xpm ) );

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

    if( !Component->GetEditFlags() )
    {
        msg = AddHotkeyName( _( "Edit with Library Editor" ), g_Schematic_Hotkeys_Descr,
                             HK_EDIT_COMPONENT_WITH_LIBEDIT );
        AddMenuItem( editmenu, ID_POPUP_SCH_CALL_LIBEDIT_AND_LOAD_CMP,
                     msg, KiBitmap( libedit_xpm ) );
    }
}


void SCH_EDIT_FRAME::addJunctionMenuEntries( wxMenu* aMenu, SCH_JUNCTION* aJunction )
{
    wxString msg;
    SCH_SCREEN* screen = GetScreen();

    if( !aJunction->IsNew() )
    {
        if( screen->GetWire( aJunction->GetPosition(), EXCLUDE_END_POINTS_T ) )
            AddMenuItem( aMenu, ID_POPUP_SCH_BREAK_WIRE, _( "Break Wire" ),
                         KiBitmap( break_line_xpm ) );
    }

    if( screen->GetWireOrBus( aJunction->GetPosition() ) )
    {
        AddMenuItem( aMenu, ID_POPUP_SCH_DELETE_NODE, _( "Delete Node" ),
                     KiBitmap( delete_node_xpm ) );
        AddMenuItem( aMenu, ID_POPUP_SCH_DELETE_CONNECTION, _( "Delete Connection" ),
                     KiBitmap( delete_connection_xpm ) );
    }
}


void AddMenusForWire( wxMenu* PopMenu, SCH_LINE* Wire, SCH_EDIT_FRAME* frame )
{
    SCH_SCREEN* screen = frame->GetScreen();
    wxPoint     pos    = frame->GetCrossHairPosition();
    wxString    msg;

    AddMenuItem( PopMenu, ID_POPUP_SCH_DELETE_NODE, _( "Delete Node" ),
                 KiBitmap( delete_node_xpm ) );
    AddMenuItem( PopMenu, ID_POPUP_SCH_DELETE_CONNECTION, _( "Delete Connection" ),
                 KiBitmap( delete_connection_xpm ) );

    SCH_LINE* line = screen->GetWireOrBus( frame->GetCrossHairPosition() );

    if( line && !line->IsEndPoint( frame->GetCrossHairPosition() ) )
        AddMenuItem( PopMenu, ID_POPUP_SCH_BREAK_WIRE, _( "Break Wire" ),
                     KiBitmap( break_line_xpm ) );

    PopMenu->AppendSeparator();

    msg = AddHotkeyName( _( "Add Junction" ), g_Schematic_Hotkeys_Descr, HK_ADD_JUNCTION );
    AddMenuItem( PopMenu, ID_POPUP_SCH_ADD_JUNCTION, msg, KiBitmap( add_junction_xpm ) );
    msg = AddHotkeyName( _( "Add Label..." ), g_Schematic_Hotkeys_Descr, HK_ADD_LABEL );
    AddMenuItem( PopMenu, ID_POPUP_SCH_ADD_LABEL, msg, KiBitmap( add_line_label_xpm ) );

    // Add global label command only if the cursor is over one end of the wire.
    if( Wire->IsEndPoint( pos ) )
        AddMenuItem( PopMenu, ID_POPUP_SCH_ADD_GLABEL, _( "Add Global Label..." ),
                     KiBitmap( add_glabel_xpm ) );
}


void AddMenusForBus( wxMenu* PopMenu, SCH_LINE* Bus, SCH_EDIT_FRAME* frame )
{
    SCH_SELECTION_TOOL* selTool = frame->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    wxPoint             pos = frame->GetCrossHairPosition();
    wxString            msg;

    AddMenuItem( PopMenu, ID_POPUP_SCH_BREAK_WIRE, _( "Break Bus" ), KiBitmap( break_bus_xpm ) );

    // TODO(JE) remove once real-time is enabled
    if( !ADVANCED_CFG::GetCfg().m_realTimeConnectivity || !CONNECTION_GRAPH::m_allowRealTime )
    {
        frame->RecalculateConnections();

        // Have to pick up the pointer again because it may have been changed by SchematicCleanUp
        bool actionCancelled = false;
        Bus = dynamic_cast<SCH_LINE*>( selTool->SelectPoint( pos, SCH_COLLECTOR::AllItemsButPins,
                                                             &actionCancelled ) );
        wxASSERT( Bus );
    }

    // Bus unfolding menu (only available if bus is properly defined)
    wxMenu* bus_unfold_menu = frame->GetUnfoldBusMenu( Bus );

    if( bus_unfold_menu )
        PopMenu->AppendSubMenu( bus_unfold_menu, _( "Unfold Bus" ) );

    PopMenu->AppendSeparator();
    msg = AddHotkeyName( _( "Add Junction" ), g_Schematic_Hotkeys_Descr, HK_ADD_JUNCTION );
    AddMenuItem( PopMenu, ID_POPUP_SCH_ADD_JUNCTION, msg, KiBitmap( add_junction_xpm ) );
    msg = AddHotkeyName( _( "Add Label..." ), g_Schematic_Hotkeys_Descr, HK_ADD_LABEL );
    AddMenuItem( PopMenu, ID_POPUP_SCH_ADD_LABEL, msg, KiBitmap( add_line_label_xpm ) );

    // Add global label command only if the cursor is over one end of the bus.
    if( Bus->IsEndPoint( pos ) )
        AddMenuItem( PopMenu, ID_POPUP_SCH_ADD_GLABEL, _( "Add Global Label..." ),
                     KiBitmap( add_glabel_xpm ) );
}


void AddMenusForHierchicalSheet( wxMenu* PopMenu, SCH_SHEET* Sheet )
{
    wxString msg;

    if( !Sheet->GetEditFlags() )
    {
        AddMenuItem( PopMenu, ID_POPUP_SCH_ENTER_SHEET, _( "Enter Sheet" ),
                     KiBitmap( enter_sheet_xpm ) );
        PopMenu->AppendSeparator();
        msg = AddHotkeyName( _( "Select Items On PCB" ), g_Schematic_Hotkeys_Descr,
                             HK_SELECT_ITEMS_ON_PCB );
        AddMenuItem( PopMenu, ID_POPUP_SCH_SELECT_ON_PCB, msg, KiBitmap( select_same_sheet_xpm ) );
        PopMenu->AppendSeparator();
    }

    {
        AddMenuItem( PopMenu, ID_POPUP_SCH_RESIZE_SHEET, _( "Resize" ),
                     KiBitmap( resize_sheet_xpm ) );
        PopMenu->AppendSeparator();
        AddMenuItem( PopMenu, ID_POPUP_IMPORT_HLABEL_TO_SHEETPIN, _( "Import Sheet Pins" ),
                     KiBitmap( import_hierarchical_label_xpm ) );

        if( Sheet->HasUndefinedPins() )  // Sheet has pin labels, and can be cleaned
            AddMenuItem( PopMenu, ID_POPUP_SCH_CLEANUP_SHEET, _( "Cleanup Sheet Pins" ),
                         KiBitmap( options_pinsheet_xpm ) );

        PopMenu->AppendSeparator();
        msg = AddHotkeyName( _( "Delete" ), g_Schematic_Hotkeys_Descr, HK_DELETE );
        AddMenuItem( PopMenu, ID_SCH_DELETE, msg, KiBitmap( delete_sheet_xpm ) );
    }
}


void AddMenusForMarkers( wxMenu* aPopMenu, SCH_MARKER* aMarker, SCH_EDIT_FRAME* aFrame )
{
    AddMenuItem( aPopMenu, ID_POPUP_SCH_GETINFO_MARKER, _( "Marker Error Info" ),
                 KiBitmap( info_xpm ) );
}


