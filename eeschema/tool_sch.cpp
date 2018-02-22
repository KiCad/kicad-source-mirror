/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2017 Wayne Stambaugh <stambaughw@verizon.net>
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

/**
 * @file tool_sch.cpp
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <sch_edit_frame.h>
#include <kiface_i.h>
#include <bitmaps.h>

#include <general.h>
#include <hotkeys.h>
#include <eeschema_id.h>

#include <help_common_strings.h>


/* Create  the main Horizontal Toolbar for the schematic editor
 */
void SCH_EDIT_FRAME::ReCreateHToolbar()
{
    if( m_mainToolBar )
        m_mainToolBar->Clear();
    else
        m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                          KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    wxString msg;

    // Set up toolbar
    if( Kiface().IsSingle() )   // not when under a project mgr
    {
        // These 2 menus have meaning only outside a project, i.e. not under a project manager:
        m_mainToolBar->AddTool( ID_NEW_PROJECT, wxEmptyString,
                                KiScaledBitmap( new_document_xpm, this ),
                                _( "New schematic" ) );

        m_mainToolBar->AddTool( ID_LOAD_PROJECT, wxEmptyString,
                                KiScaledBitmap( open_document_xpm, this ),
                                _( "Open schematic" ) );
    }

    m_mainToolBar->AddTool( ID_SAVE_PROJECT, wxEmptyString,
                            KiScaledBitmap( save_xpm, this ),
                            _( "Save (all sheets)" ) );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_SHEET_SET, wxEmptyString, KiScaledBitmap( sheetset_xpm, this ),
                            _( "Edit Page settings" ) );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( wxID_PRINT, wxEmptyString, KiScaledBitmap( print_button_xpm, this ),
                            _( "Print schematic" ) );

    m_mainToolBar->AddTool( ID_GEN_PLOT_SCHEMATIC, wxEmptyString, KiScaledBitmap( plot_xpm, this ),
                            _( "Plot schematic" ) );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( wxID_PASTE, wxEmptyString, KiScaledBitmap( paste_xpm, this ),
                            _( "Paste" ) );


    KiScaledSeparator( m_mainToolBar, this );

    msg = AddHotkeyName( HELP_UNDO, g_Schematic_Hokeys_Descr, HK_UNDO, IS_COMMENT );
    m_mainToolBar->AddTool( wxID_UNDO, wxEmptyString, KiScaledBitmap( undo_xpm, this ), msg );

    msg = AddHotkeyName( HELP_REDO, g_Schematic_Hokeys_Descr, HK_REDO, IS_COMMENT );
    m_mainToolBar->AddTool( wxID_REDO, wxEmptyString, KiScaledBitmap( redo_xpm, this ), msg );


    KiScaledSeparator( m_mainToolBar, this );

    msg = AddHotkeyName( HELP_FIND, g_Schematic_Hokeys_Descr, HK_FIND_ITEM, IS_COMMENT );
    m_mainToolBar->AddTool( ID_FIND_ITEMS, wxEmptyString, KiScaledBitmap( find_xpm, this ), msg );

    m_mainToolBar->AddTool( wxID_REPLACE, wxEmptyString, KiScaledBitmap( find_replace_xpm, this ),
                            wxNullBitmap, wxITEM_NORMAL, _( "Find and replace text" ),
                            HELP_REPLACE, nullptr );


    KiScaledSeparator( m_mainToolBar, this );

    msg = AddHotkeyName( HELP_ZOOM_REDRAW, g_Schematic_Hokeys_Descr, HK_ZOOM_REDRAW, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                            KiScaledBitmap( zoom_redraw_xpm, this ), msg );

    msg = AddHotkeyName( HELP_ZOOM_IN, g_Schematic_Hokeys_Descr, HK_ZOOM_IN, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, KiScaledBitmap( zoom_in_xpm, this ), msg );

    msg = AddHotkeyName( HELP_ZOOM_OUT, g_Schematic_Hokeys_Descr, HK_ZOOM_OUT, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, KiScaledBitmap( zoom_out_xpm, this ), msg );

    msg = AddHotkeyName( HELP_ZOOM_FIT, g_Schematic_Hokeys_Descr, HK_ZOOM_AUTO, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString,
                            KiScaledBitmap( zoom_fit_in_page_xpm, this ), msg );

    m_mainToolBar->AddTool( ID_ZOOM_SELECTION, wxEmptyString, KiScaledBitmap( zoom_area_xpm, this ),
                            _( "Zoom to selection" ), wxITEM_CHECK );


    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_HIERARCHY, wxEmptyString, KiScaledBitmap( hierarchy_nav_xpm, this ),
                            _( "Navigate schematic hierarchy" ) );


    m_mainToolBar->AddTool( ID_POPUP_SCH_LEAVE_SHEET, wxEmptyString,
                            KiScaledBitmap( leave_sheet_xpm, this ), _( "Leave sheet" ) );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_RUN_LIBRARY, wxEmptyString, KiScaledBitmap( libedit_xpm, this ),
                            HELP_RUN_LIB_EDITOR );

    m_mainToolBar->AddTool( ID_TO_LIBVIEW, wxEmptyString,
                            KiScaledBitmap( library_browse_xpm, this ), HELP_RUN_LIB_VIEWER );

    // modedit is with libedit in a "library section" because the user must have footprints before
    // they can be assigned.
    m_mainToolBar->AddTool( ID_RUN_PCB_MODULE_EDITOR, wxEmptyString,
                            KiScaledBitmap( module_editor_xpm, this ),
                            _( "Footprint Editor - Create/edit footprints" ) );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_GET_ANNOTATE, wxEmptyString, KiScaledBitmap( annotate_xpm, this ),
                            HELP_ANNOTATE );

    m_mainToolBar->AddTool( ID_GET_ERC, wxEmptyString, KiScaledBitmap( erc_xpm, this ),
                            _( "Perform electrical rules check" ) );

    m_mainToolBar->AddTool( ID_RUN_CVPCB, wxEmptyString, KiScaledBitmap( cvpcb_xpm, this ),
                            _( "Run CvPcb to associate footprints to symbols" ) );

    m_mainToolBar->AddTool( ID_GET_NETLIST, wxEmptyString, KiScaledBitmap( netlist_xpm, this ),
                            _( "Generate netlist" ) );

    m_mainToolBar->AddTool( ID_OPEN_CMP_TABLE, wxEmptyString,
                            KiScaledBitmap( spreadsheet_xpm, this ), _( "Edit symbol fields"  ) );


    m_mainToolBar->AddTool( ID_GET_TOOLS, wxEmptyString, KiScaledBitmap( bom_xpm, this ),
                            HELP_GENERATE_BOM );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_RUN_PCB, wxEmptyString, KiScaledBitmap( pcbnew_xpm, this ),
                            _( "Run Pcbnew to layout printed circuit board" ) );

    m_mainToolBar->AddTool( ID_BACKANNO_ITEMS, wxEmptyString,
                            KiScaledBitmap( import_footprint_names_xpm, this ),
                            HELP_IMPORT_FOOTPRINTS );

    // after adding the tools to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}


/* Create Vertical Right Toolbar
 */
void SCH_EDIT_FRAME::ReCreateVToolbar()
{
    if( m_drawToolBar )
        m_drawToolBar->Clear();
    else
        m_drawToolBar = new wxAuiToolBar( this, ID_V_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                          KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    // Set up toolbar
    m_drawToolBar->AddTool( ID_NO_TOOL_SELECTED, wxEmptyString, KiScaledBitmap( cursor_xpm, this ),
                            wxEmptyString, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_HIGHLIGHT, wxEmptyString,
                            KiScaledBitmap( net_highlight_schematic_xpm, this ),
                            _( "Highlight net" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_SCH_PLACE_COMPONENT, wxEmptyString,
                            KiScaledBitmap( add_component_xpm, this ), HELP_PLACE_COMPONENTS,
                            wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_PLACE_POWER_BUTT, wxEmptyString,
                            KiScaledBitmap( add_power_xpm, this ),
                            HELP_PLACE_POWERPORT, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_WIRE_BUTT, wxEmptyString, KiScaledBitmap( add_line_xpm, this ),
                            HELP_PLACE_WIRE, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_BUS_BUTT, wxEmptyString, KiScaledBitmap( add_bus_xpm, this ),
                            HELP_PLACE_BUS, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_WIRETOBUS_ENTRY_BUTT, wxEmptyString,
                            KiScaledBitmap( add_line2bus_xpm, this ),
                            HELP_PLACE_WIRE2BUS_ENTRY, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_BUSTOBUS_ENTRY_BUTT, wxEmptyString,
                            KiScaledBitmap( add_bus2bus_xpm, this ),
                            HELP_PLACE_BUS2BUS_ENTRY, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_NOCONN_BUTT, wxEmptyString, KiScaledBitmap( noconn_xpm, this ),
                            HELP_PLACE_NC_FLAG, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_JUNCTION_BUTT, wxEmptyString,
                            KiScaledBitmap( add_junction_xpm, this ),
                            HELP_PLACE_JUNCTION, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_LABEL_BUTT, wxEmptyString,
                            KiScaledBitmap( add_line_label_xpm, this ),
                            HELP_PLACE_NETLABEL, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_GLABEL_BUTT, wxEmptyString, KiScaledBitmap( add_glabel_xpm, this ),
                            HELP_PLACE_GLOBALLABEL, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_HIERLABEL_BUTT, wxEmptyString,
                            KiScaledBitmap( add_hierarchical_label_xpm, this ),
                            HELP_PLACE_HIER_LABEL, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_SHEET_SYMBOL_BUTT, wxEmptyString,
                            KiScaledBitmap( add_hierarchical_subsheet_xpm, this ),
                            HELP_PLACE_SHEET, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_IMPORT_HLABEL_BUTT, wxEmptyString,
                            KiScaledBitmap( import_hierarchical_label_xpm, this ),
                            HELP_IMPORT_SHEETPIN, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_SHEET_PIN_BUTT, wxEmptyString,
                            KiScaledBitmap( add_hierar_pin_xpm, this ),
                            HELP_PLACE_SHEETPIN, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_LINE_COMMENT_BUTT, wxEmptyString,
                            KiScaledBitmap( add_dashed_line_xpm, this ),
                            HELP_PLACE_GRAPHICLINES, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_TEXT_COMMENT_BUTT, wxEmptyString, KiScaledBitmap( text_xpm, this ),
                            HELP_PLACE_GRAPHICTEXTS, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_ADD_IMAGE_BUTT, wxEmptyString, KiScaledBitmap( image_xpm, this ),
                            _("Add bitmap image"), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_SCHEMATIC_DELETE_ITEM_BUTT, wxEmptyString,
                            KiScaledBitmap( delete_xpm, this ),
                            HELP_DELETE_ITEMS, wxITEM_CHECK );

    m_drawToolBar->Realize();
}


/* Create Vertical Left Toolbar (Option Toolbar)
 */
void SCH_EDIT_FRAME::ReCreateOptToolbar()
{
    if( m_optionsToolBar )
        m_optionsToolBar->Clear();
    else
        m_optionsToolBar = new wxAuiToolBar( this, ID_OPT_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                             KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString,
                               KiScaledBitmap( grid_xpm, this ),
                               _( "Turn grid off" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               KiScaledBitmap( unit_inch_xpm, this ),
                               _( "Set unit to inch" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               KiScaledBitmap( unit_mm_xpm, this ),
                               _( "Set unit to mm" ), wxITEM_CHECK );

#ifndef __APPLE__
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               KiScaledBitmap( cursor_shape_xpm, this ),
                               _( "Change cursor shape" ), wxITEM_CHECK );
#else
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               KiScaledBitmap( cursor_shape_xpm, this ),
                               _( "Change cursor shape (not supported in Legacy Toolset)" ),
                               wxITEM_CHECK  );
#endif

    //KiScaledSeparator( m_optionsToolBar, this );
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_HIDDEN_PINS, wxEmptyString,
                               KiScaledBitmap( hidden_pin_xpm, this ),
                               _( "Show hidden pins" ), wxITEM_CHECK );

    //KiScaledSeparator( m_optionsToolBar, this );
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_BUS_WIRES_ORIENT, wxEmptyString,
                               KiScaledBitmap( lines90_xpm, this ),
                               _( "HV orientation for wires and bus" ),
                               wxITEM_CHECK );

    m_optionsToolBar->Realize();
}


void SCH_EDIT_FRAME::OnSelectOptionToolbar( wxCommandEvent& event )
{
    int id = event.GetId();

    switch( id )
    {
    case ID_TB_OPTIONS_HIDDEN_PINS:
        m_showAllPins = !m_showAllPins;

        if( m_canvas )
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
