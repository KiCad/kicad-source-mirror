/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2013 KiCad Developers, see change_log.txt for contributors.
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
 * @file tool_sch.cpp
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <wxEeschemaStruct.h>
#include <kiface_i.h>

#include <general.h>
#include <hotkeys.h>
#include <eeschema_id.h>

#include <help_common_strings.h>


/* Create  the main Horizontal Toolbar for the schematic editor
 */
void SCH_EDIT_FRAME::ReCreateHToolbar()
{
    if( m_mainToolBar != NULL )
        return;

    wxString msg;
    m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                      wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // Set up toolbar
    m_mainToolBar->AddTool( ID_NEW_PROJECT, wxEmptyString, KiBitmap( new_xpm ),
                            _( "New schematic project" ) );

    m_mainToolBar->AddTool( ID_LOAD_PROJECT, wxEmptyString, KiBitmap( open_document_xpm ),
                            _( "Open schematic project" ) );

    m_mainToolBar->AddTool( ID_SAVE_PROJECT, wxEmptyString, KiBitmap( save_project_xpm ),
                            _( "Save schematic project" ) );


    m_mainToolBar->AddSeparator();


    m_mainToolBar->AddTool( ID_SHEET_SET, wxEmptyString, KiBitmap( sheetset_xpm ),
                            _( "Page settings" ) );


    m_mainToolBar->AddSeparator();


    m_mainToolBar->AddTool( wxID_PRINT, wxEmptyString, KiBitmap( print_button_xpm ),
                            _( "Print schematic" ) );


    m_mainToolBar->AddSeparator();


    m_mainToolBar->AddTool( wxID_CUT, wxEmptyString, KiBitmap( cut_button_xpm ),
                            _( "Cut selected item" ) );

    m_mainToolBar->AddTool( wxID_COPY, wxEmptyString, KiBitmap( copy_button_xpm ),
                            _( "Copy selected item" ) );

    m_mainToolBar->AddTool( wxID_PASTE, wxEmptyString, KiBitmap( paste_xpm ),
                            _( "Paste" ) );


    m_mainToolBar->AddSeparator();


    msg = AddHotkeyName( HELP_UNDO, g_Schematic_Hokeys_Descr, HK_UNDO, IS_COMMENT );
    m_mainToolBar->AddTool( wxID_UNDO, wxEmptyString, KiBitmap( undo_xpm ), msg );

    msg = AddHotkeyName( HELP_REDO, g_Schematic_Hokeys_Descr, HK_REDO, IS_COMMENT );
    m_mainToolBar->AddTool( wxID_REDO, wxEmptyString, KiBitmap( redo_xpm ), msg );


    m_mainToolBar->AddSeparator();


    msg = AddHotkeyName( HELP_FIND, g_Schematic_Hokeys_Descr, HK_FIND_ITEM, IS_COMMENT );
    m_mainToolBar->AddTool( ID_FIND_ITEMS, wxEmptyString, KiBitmap( find_xpm ), msg );

    m_mainToolBar->AddTool( wxID_REPLACE, wxEmptyString, KiBitmap( find_replace_xpm ),
                            wxNullBitmap, wxITEM_NORMAL, _( "Find and replace text" ),
                            HELP_REPLACE, NULL );

    m_mainToolBar->AddSeparator();


    msg = AddHotkeyName( HELP_ZOOM_IN, g_Schematic_Hokeys_Descr, HK_ZOOM_IN, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, KiBitmap( zoom_in_xpm ), msg );

    msg = AddHotkeyName( HELP_ZOOM_OUT, g_Schematic_Hokeys_Descr, HK_ZOOM_OUT, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, KiBitmap( zoom_out_xpm ), msg );

    msg = AddHotkeyName( HELP_ZOOM_REDRAW, g_Schematic_Hokeys_Descr, HK_ZOOM_REDRAW, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString, KiBitmap( zoom_redraw_xpm ), msg );

    msg = AddHotkeyName( HELP_ZOOM_FIT, g_Schematic_Hokeys_Descr, HK_ZOOM_AUTO, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString, KiBitmap( zoom_fit_in_page_xpm ), msg );


    m_mainToolBar->AddSeparator();


    m_mainToolBar->AddTool( ID_HIERARCHY, wxEmptyString, KiBitmap( hierarchy_nav_xpm ),
                            _( "Navigate schematic hierarchy" ) );


    m_mainToolBar->AddSeparator();


    m_mainToolBar->AddTool( ID_RUN_LIBRARY, wxEmptyString, KiBitmap( libedit_xpm ),
                            HELP_RUN_LIB_EDITOR );

    m_mainToolBar->AddTool( ID_TO_LIBVIEW, wxEmptyString, KiBitmap( library_browse_xpm ),
                            HELP_RUN_LIB_VIEWER );

    m_mainToolBar->AddSeparator();

    m_mainToolBar->AddTool( ID_GET_ANNOTATE, wxEmptyString, KiBitmap( annotate_xpm ),
                            HELP_ANNOTATE );

    m_mainToolBar->AddTool( ID_GET_ERC, wxEmptyString, KiBitmap( erc_xpm ),
                            _( "Perform electrical rule check" ) );

    m_mainToolBar->AddTool( ID_GET_NETLIST, wxEmptyString, KiBitmap( netlist_xpm ),
                            _( "Generate netlist" ) );

    m_mainToolBar->AddTool( ID_GET_TOOLS, wxEmptyString, KiBitmap( bom_xpm ),
                            HELP_GENERATE_BOM );


    m_mainToolBar->AddSeparator();

    // The user must HAVE footprints before he can assign them.  So put this before
    // the CVPCB.
    if( !Kiface().IsSingle() )  // if pcbnew is not a separate process
    {
        m_mainToolBar->AddTool( ID_RUN_PCB_MODULE_EDITOR, wxEmptyString, KiBitmap( module_editor_xpm ),
                                _( "Footprint Editor" ) );
    }

    m_mainToolBar->AddTool( ID_RUN_CVPCB, wxEmptyString, KiBitmap( cvpcb_xpm ),
                            _( "Run CvPcb to associate components and footprints" ) );

    m_mainToolBar->AddTool( ID_RUN_PCB, wxEmptyString, KiBitmap( pcbnew_xpm ),
                            _( "Run Pcbnew to layout printed circuit board" ) );

    m_mainToolBar->AddTool( ID_BACKANNO_ITEMS, wxEmptyString,
                            KiBitmap( import_footprint_names_xpm ),
                            HELP_IMPORT_FOOTPRINTS );

    // set icon paddings
    m_mainToolBar->SetToolBorderPadding(3); // padding
    m_mainToolBar->SetToolSeparation(0);
    //m_mainToolBar->SetMargins(0,1); // margins width and height

    // after adding the tools to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}


/* Create Vertical Right Toolbar
 */
void SCH_EDIT_FRAME::ReCreateVToolbar()
{
    if( m_drawToolBar )
        return;

    m_drawToolBar = new wxAuiToolBar( this, ID_V_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                      wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_VERTICAL );

    // Set up toolbar
    m_drawToolBar->AddTool( ID_NO_TOOL_SELECTED, wxEmptyString, KiBitmap( cursor_xpm ),
                            wxEmptyString, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_HIERARCHY_PUSH_POP_BUTT, wxEmptyString,
                            KiBitmap( hierarchy_cursor_xpm ),
                            _( "Ascend/descend hierarchy" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_SCH_PLACE_COMPONENT, wxEmptyString, KiBitmap( add_component_xpm ),
                            HELP_PLACE_COMPONENTS, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_PLACE_POWER_BUTT, wxEmptyString, KiBitmap( add_power_xpm ),
                            HELP_PLACE_POWERPORT, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_WIRE_BUTT, wxEmptyString, KiBitmap( add_line_xpm ),
                            HELP_PLACE_WIRE, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_BUS_BUTT, wxEmptyString, KiBitmap( add_bus_xpm ),
                            HELP_PLACE_BUS, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_WIRETOBUS_ENTRY_BUTT, wxEmptyString, KiBitmap( add_line2bus_xpm ),
                            HELP_PLACE_WIRE2BUS_ENTRY, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_BUSTOBUS_ENTRY_BUTT, wxEmptyString, KiBitmap( add_bus2bus_xpm ),
                            HELP_PLACE_BUS2BUS_ENTRY, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_NOCONN_BUTT, wxEmptyString, KiBitmap( noconn_xpm ),
                            HELP_PLACE_NC_FLAG, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_JUNCTION_BUTT, wxEmptyString, KiBitmap( add_junction_xpm ),
                            HELP_PLACE_JUNCTION, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_LABEL_BUTT, wxEmptyString, KiBitmap( add_line_label_xpm ),
                            HELP_PLACE_NETLABEL, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_GLABEL_BUTT, wxEmptyString, KiBitmap( add_glabel_xpm ),
                            HELP_PLACE_GLOBALLABEL, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_HIERLABEL_BUTT, wxEmptyString,
                            KiBitmap( add_hierarchical_label_xpm ),
                            HELP_PLACE_HIER_LABEL, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_SHEET_SYMBOL_BUTT, wxEmptyString,
                            KiBitmap( add_hierarchical_subsheet_xpm ),
                            HELP_PLACE_SHEET, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_IMPORT_HLABEL_BUTT, wxEmptyString,
                            KiBitmap( import_hierarchical_label_xpm ),
                            HELP_IMPORT_SHEETPIN, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_SHEET_PIN_BUTT, wxEmptyString,
                            KiBitmap( add_hierar_pin_xpm ),
                            HELP_PLACE_SHEETPIN, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_LINE_COMMENT_BUTT, wxEmptyString,
                            KiBitmap( add_dashed_line_xpm ),
                            HELP_PLACE_GRAPHICLINES, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_TEXT_COMMENT_BUTT, wxEmptyString, KiBitmap( add_text_xpm ),
                            HELP_PLACE_GRAPHICTEXTS, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_ADD_IMAGE_BUTT, wxEmptyString, KiBitmap( image_xpm ),
                            _("Add bitmap image"), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_SCHEMATIC_DELETE_ITEM_BUTT, wxEmptyString,
                            KiBitmap( delete_xpm ),
                            HELP_DELETE_ITEMS, wxITEM_CHECK );

    // set icon paddings
    m_drawToolBar->SetToolBorderPadding(2); // padding
    m_drawToolBar->SetToolSeparation(0);
    //m_drawToolBar->SetMargins(1,0); // margins width and height

    m_drawToolBar->Realize();
}


/* Create Vertical Left Toolbar (Option Toolbar)
 */
void SCH_EDIT_FRAME::ReCreateOptToolbar()
{
    if( m_optionsToolBar )
        return;

    m_optionsToolBar = new wxAuiToolBar( this, ID_OPT_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                         wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_VERTICAL );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString,
                               KiBitmap( grid_xpm ),
                               _( "Turn grid off" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               KiBitmap( unit_inch_xpm ),
                               _( "Set unit to inch" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               KiBitmap( unit_mm_xpm ),
                               _( "Set unit to mm" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               KiBitmap( cursor_shape_xpm ),
                               _( "Change cursor shape" ), wxITEM_CHECK );

    //m_optionsToolBar->AddSeparator();
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_HIDDEN_PINS, wxEmptyString,
                               KiBitmap( hidden_pin_xpm ),
                               _( "Show hidden pins" ), wxITEM_CHECK );

    //m_optionsToolBar->AddSeparator();
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_BUS_WIRES_ORIENT, wxEmptyString,
                               KiBitmap( lines90_xpm ),
                               _( "HV orientation for wires and bus" ),
                               wxITEM_CHECK );

    // set icon paddings
    m_optionsToolBar->SetToolBorderPadding(2); // padding
    m_optionsToolBar->SetToolSeparation(0);

    m_optionsToolBar->Realize();
}


void SCH_EDIT_FRAME::OnSelectOptionToolbar( wxCommandEvent& event )
{
    if( m_canvas == NULL )
        return;

    int id = event.GetId();

    switch( id )
    {
    case ID_TB_OPTIONS_HIDDEN_PINS:
        m_showAllPins = m_optionsToolBar->GetToolToggled( id );
        m_canvas->Refresh();
        break;

    case ID_TB_OPTIONS_BUS_WIRES_ORIENT:
        SetForceHVLines( m_optionsToolBar->GetToolToggled( id ) );
        break;

    default:
        wxFAIL_MSG( wxT( "Unexpected select option tool bar ID." ) );
        break;
    }
}
