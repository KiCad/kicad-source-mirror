/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
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
 * @file tool_modedit.cpp
 * @brief Footprint editor tool bars
 */

#include <fctsys.h>

#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <module_editor_frame.h>
#include <dialog_helpers.h>
#include <pcbnew_id.h>
#include <hotkeys.h>

#ifdef __UNIX__
#define LISTBOX_WIDTH 140
#else
#define LISTBOX_WIDTH 120
#endif


void FOOTPRINT_EDIT_FRAME::ReCreateHToolbar()
{
    if( m_mainToolBar  != NULL )
        return;

    wxString msg;

    m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                      wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // Set up toolbar
    m_mainToolBar->AddTool( ID_MODEDIT_SELECT_CURRENT_LIB, wxEmptyString,
                            KiBitmap( open_library_xpm ),
                            _( "Select active library" ) );

    m_mainToolBar->AddTool( ID_MODEDIT_SAVE_LIBMODULE, wxEmptyString, KiBitmap( save_library_xpm ),
                            _( "Save footprint in active library" ) );

    m_mainToolBar->AddTool( ID_MODEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART, wxEmptyString,
                            KiBitmap( new_library_xpm ),
                            _( "Create new library and save current footprint" ) );

    m_mainToolBar->AddTool( ID_OPEN_MODULE_VIEWER, wxEmptyString, KiBitmap( modview_icon_xpm ),
                            _( "Open footprint viewer" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_MODEDIT_DELETE_PART, wxEmptyString, KiBitmap( delete_xpm ),
                            _( "Delete part from active library" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_MODEDIT_NEW_MODULE, wxEmptyString, KiBitmap( new_footprint_xpm ),
                            _( "New footprint" ) );

#ifdef KICAD_SCRIPTING
    m_mainToolBar->AddTool( ID_MODEDIT_NEW_MODULE_FROM_WIZARD, wxEmptyString,
                            KiBitmap( module_wizard_xpm ),
                            _( "New footprint using wizard" ) );
#endif


    m_mainToolBar->AddTool( ID_MODEDIT_LOAD_MODULE, wxEmptyString,
                            KiBitmap( load_module_lib_xpm ),
                            _( "Load footprint from library" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_MODEDIT_LOAD_MODULE_FROM_BOARD, wxEmptyString,
                            KiBitmap( load_module_board_xpm ),
                            _( "Load footprint from current board" ) );

    m_mainToolBar->AddTool( ID_MODEDIT_UPDATE_MODULE_IN_BOARD, wxEmptyString,
                            KiBitmap( update_module_board_xpm ),
                            _( "Update footprint in current board" ) );

    m_mainToolBar->AddTool( ID_MODEDIT_INSERT_MODULE_IN_BOARD, wxEmptyString,
                            KiBitmap( insert_module_board_xpm ),
                            _( "Insert footprint into current board" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_MODEDIT_IMPORT_PART, wxEmptyString, KiBitmap( import_module_xpm ),
                            _( "Import footprint" ) );

    m_mainToolBar->AddTool( ID_MODEDIT_EXPORT_PART, wxEmptyString, KiBitmap( export_module_xpm ),
                            _( "Export footprint" ) );


    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( wxID_UNDO, wxEmptyString, KiBitmap( undo_xpm ),
                            _( "Undo last edition" ) );
    m_mainToolBar->AddTool( wxID_REDO, wxEmptyString, KiBitmap( redo_xpm ),
                            _( "Redo the last undo command" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_MODEDIT_EDIT_MODULE_PROPERTIES, wxEmptyString,
                            KiBitmap( module_options_xpm ),
                            _( "Footprint properties" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( wxID_PRINT, wxEmptyString, KiBitmap( print_button_xpm ),
                            _( "Print footprint" ) );

    m_mainToolBar->AddSeparator();
    msg = AddHotkeyName( _( "Zoom in" ), g_Module_Editor_Hokeys_Descr, HK_ZOOM_IN, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, KiBitmap( zoom_in_xpm ), msg );

    msg = AddHotkeyName( _( "Zoom out" ), g_Module_Editor_Hokeys_Descr, HK_ZOOM_OUT, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, KiBitmap( zoom_out_xpm ), msg );

    msg = AddHotkeyName( _( "Redraw view" ), g_Module_Editor_Hokeys_Descr, HK_ZOOM_REDRAW,
                         IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString, KiBitmap( zoom_redraw_xpm ), msg );

    msg = AddHotkeyName( _( "Zoom auto" ), g_Module_Editor_Hokeys_Descr, HK_ZOOM_AUTO, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString, KiBitmap( zoom_fit_in_page_xpm ), msg );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_MODEDIT_PAD_SETTINGS, wxEmptyString, KiBitmap( options_pad_xpm ),
                            _( "Pad settings" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_MODEDIT_CHECK, wxEmptyString,
                            KiBitmap( module_check_xpm ),
                            _( "Check footprint" ) );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}


void FOOTPRINT_EDIT_FRAME::ReCreateVToolbar()
{
    if( m_drawToolBar )
        return;

    m_drawToolBar = new wxAuiToolBar( this, ID_V_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                      wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_VERTICAL );

    // Set up toolbar
    m_drawToolBar->AddTool( ID_NO_TOOL_SELECTED, wxEmptyString, KiBitmap( cursor_xpm ),
                            wxEmptyString, wxITEM_CHECK );

    m_drawToolBar->AddSeparator();
    m_drawToolBar->AddTool( ID_MODEDIT_PAD_TOOL, wxEmptyString, KiBitmap( pad_xpm ),
                            _( "Add pads" ), wxITEM_CHECK );

    m_drawToolBar->AddSeparator();
    m_drawToolBar->AddTool( ID_MODEDIT_LINE_TOOL, wxEmptyString, KiBitmap( add_polygon_xpm ),
                            _( "Add graphic line or polygon" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_MODEDIT_CIRCLE_TOOL, wxEmptyString, KiBitmap( add_circle_xpm ),
                            _( "Add graphic circle" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_MODEDIT_ARC_TOOL, wxEmptyString, KiBitmap( add_arc_xpm ),
                            _( "Add graphic arc" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_MODEDIT_TEXT_TOOL, wxEmptyString, KiBitmap( add_text_xpm ),
                            _( "Add Text" ), wxITEM_CHECK );

    m_drawToolBar->AddSeparator();
    m_drawToolBar->AddTool( ID_MODEDIT_ANCHOR_TOOL, wxEmptyString, KiBitmap( anchor_xpm ),
                            _( "Place the footprint reference anchor" ),
                            wxITEM_CHECK );

    m_drawToolBar->AddSeparator();
    m_drawToolBar->AddTool( ID_MODEDIT_DELETE_TOOL, wxEmptyString, KiBitmap( delete_xpm ),
                            _( "Delete items" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_MODEDIT_PLACE_GRID_COORD, wxEmptyString,
                            KiBitmap( grid_select_axis_xpm ),
                            _( "Set the origin point for the grid" ),
                            wxITEM_CHECK );

    m_drawToolBar->Realize();
}


void FOOTPRINT_EDIT_FRAME::ReCreateOptToolbar()
{
    if( m_optionsToolBar )
        return;

    // Create options tool bar.
    m_optionsToolBar = new wxAuiToolBar( this, ID_OPT_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                         wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_VERTICAL );
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString, KiBitmap( grid_xpm ),
                               _( "Hide grid" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLAR_COORD, wxEmptyString,
                               KiBitmap( polar_coord_xpm ),
                               _( "Display Polar Coord ON" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               KiBitmap( unit_inch_xpm ),
                               _( "Units in inches" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               KiBitmap( unit_mm_xpm ),
                               _( "Units in millimeters" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               KiBitmap( cursor_shape_xpm ),
                               _( "Change Cursor Shape" ), wxITEM_CHECK  );

    m_optionsToolBar->AddSeparator();
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH, wxEmptyString,
                               KiBitmap( pad_sketch_xpm ),
                               _( "Show Pads Sketch" ), wxITEM_CHECK  );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH, wxEmptyString,
                               KiBitmap( text_sketch_xpm ),
                               _( "Show Texts Sketch" ), wxITEM_CHECK  );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH, wxEmptyString,
                               KiBitmap( show_mod_edge_xpm ),
                               _( "Show Edges Sketch" ), wxITEM_CHECK  );

    m_optionsToolBar->Realize();
}


void FOOTPRINT_EDIT_FRAME::ReCreateAuxiliaryToolbar()
{
    wxString msg;

    if( m_auxiliaryToolBar )
        return;

    m_auxiliaryToolBar = new wxAuiToolBar( this, ID_AUX_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                           wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // Set up toolbar
    m_auxiliaryToolBar->AddSeparator();

    // Grid selection choice box.
    m_gridSelectBox = new wxComboBox( m_auxiliaryToolBar,
                                      ID_ON_GRID_SELECT,
                                      wxEmptyString,
                                      wxPoint( -1, -1 ),
                                      wxSize( LISTBOX_WIDTH, -1 ),
                                      0, NULL, wxCB_READONLY );
    m_auxiliaryToolBar->AddControl( m_gridSelectBox );

    // Zoom selection choice box.
    m_auxiliaryToolBar->AddSeparator();
    m_zoomSelectBox = new wxComboBox( m_auxiliaryToolBar,
                                      ID_ON_ZOOM_SELECT,
                                      wxEmptyString,
                                      wxPoint( -1, -1 ),
                                      wxSize( LISTBOX_WIDTH, -1 ),
                                      0, NULL, wxCB_READONLY );
    m_auxiliaryToolBar->AddControl( m_zoomSelectBox );

    // Update tool bar to reflect setting.
    updateGridSelectBox();
    updateZoomSelectBox();

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_auxiliaryToolBar->Realize();
}
