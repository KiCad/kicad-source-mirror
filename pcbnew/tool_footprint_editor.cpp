/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file tool_footprint_editor.cpp
 * @brief Footprint editor tool bars
 */

#include <fctsys.h>

#include <pcbnew.h>
#include <footprint_edit_frame.h>
#include <dialog_helpers.h>
#include <pcbnew_id.h>
#include <hotkeys.h>
#include <bitmaps.h>


void FOOTPRINT_EDIT_FRAME::ReCreateHToolbar()
{
    if( m_mainToolBar )
        m_mainToolBar->Clear();
    else
        m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                          KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    wxString msg;

    // Set up toolbar
    m_mainToolBar->AddTool( ID_MODEDIT_SELECT_CURRENT_LIB, wxEmptyString,
                            KiScaledBitmap( open_library_xpm, this ),
                            _( "Select active library" ) );

    m_mainToolBar->AddTool( ID_MODEDIT_SAVE_LIBMODULE, wxEmptyString, KiScaledBitmap( save_library_xpm, this ),
                            _( "Save footprint in active library" ) );

    m_mainToolBar->AddTool( ID_MODEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART, wxEmptyString,
                            KiScaledBitmap( new_library_xpm, this ),
                            _( "Create new library and save current footprint" ) );

    m_mainToolBar->AddTool( ID_OPEN_MODULE_VIEWER, wxEmptyString, KiScaledBitmap( modview_icon_xpm, this ),
                            _( "Open footprint viewer" ) );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_MODEDIT_DELETE_PART, wxEmptyString, KiScaledBitmap( delete_xpm, this ),
                            _( "Delete part from active library" ) );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_MODEDIT_NEW_MODULE, wxEmptyString, KiScaledBitmap( new_footprint_xpm, this ),
                            _( "New footprint" ) );

#ifdef KICAD_SCRIPTING
    m_mainToolBar->AddTool( ID_MODEDIT_NEW_MODULE_FROM_WIZARD, wxEmptyString,
                            KiScaledBitmap( module_wizard_xpm, this ),
                            _( "New footprint using footprint wizard" ) );
#endif


    m_mainToolBar->AddTool( ID_MODEDIT_LOAD_MODULE, wxEmptyString,
                            KiScaledBitmap( load_module_lib_xpm, this ),
                            _( "Load footprint from library" ) );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_MODEDIT_LOAD_MODULE_FROM_BOARD, wxEmptyString,
                            KiScaledBitmap( load_module_board_xpm, this ),
                            _( "Load footprint from current board" ) );

    m_mainToolBar->AddTool( ID_MODEDIT_UPDATE_MODULE_IN_BOARD, wxEmptyString,
                            KiScaledBitmap( update_module_board_xpm, this ),
                            _( "Update footprint into current board" ) );

    m_mainToolBar->AddTool( ID_MODEDIT_INSERT_MODULE_IN_BOARD, wxEmptyString,
                            KiScaledBitmap( insert_module_board_xpm, this ),
                            _( "Insert footprint into current board" ) );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_MODEDIT_IMPORT_PART, wxEmptyString, KiScaledBitmap( import_module_xpm, this ),
                            _( "Import footprint" ) );

    m_mainToolBar->AddTool( ID_MODEDIT_EXPORT_PART, wxEmptyString, KiScaledBitmap( export_module_xpm, this ),
                            _( "Export footprint" ) );


    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( wxID_UNDO, wxEmptyString, KiScaledBitmap( undo_xpm, this ),
                            _( "Undo last edition" ) );
    m_mainToolBar->AddTool( wxID_REDO, wxEmptyString, KiScaledBitmap( redo_xpm, this ),
                            _( "Redo last undo command" ) );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_MODEDIT_EDIT_MODULE_PROPERTIES, wxEmptyString,
                            KiScaledBitmap( module_options_xpm, this ),
                            _( "Footprint properties" ) );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( wxID_PRINT, wxEmptyString, KiScaledBitmap( print_button_xpm, this ),
                            _( "Print footprint" ) );

    KiScaledSeparator( m_mainToolBar, this );
    msg = AddHotkeyName( _( "Redraw view" ), g_Module_Editor_Hotkeys_Descr, HK_ZOOM_REDRAW,
                         IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString, KiScaledBitmap( zoom_redraw_xpm, this ), msg );

    msg = AddHotkeyName( _( "Zoom in" ), g_Module_Editor_Hotkeys_Descr, HK_ZOOM_IN, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, KiScaledBitmap( zoom_in_xpm, this ), msg );

    msg = AddHotkeyName( _( "Zoom out" ), g_Module_Editor_Hotkeys_Descr, HK_ZOOM_OUT, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, KiScaledBitmap( zoom_out_xpm, this ), msg );

    msg = AddHotkeyName( _( "Zoom auto" ), g_Module_Editor_Hotkeys_Descr, HK_ZOOM_AUTO, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString, KiScaledBitmap( zoom_fit_in_page_xpm, this ), msg );

    m_mainToolBar->AddTool( ID_ZOOM_SELECTION, wxEmptyString, KiScaledBitmap( zoom_area_xpm, this ),
                            _( "Zoom to selection" ), wxITEM_CHECK );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_MODEDIT_PAD_SETTINGS, wxEmptyString, KiScaledBitmap( options_pad_xpm, this ),
                            _( "Pad properties" ) );

#if 0       // Currently there is no check footprint function defined, so do not show this tool
    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_MODEDIT_CHECK, wxEmptyString,
                            KiScaledBitmap( module_check_xpm, this ),
                            _( "Check footprint" ) );
#endif

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}


void FOOTPRINT_EDIT_FRAME::ReCreateVToolbar()
{
    if( m_drawToolBar )
        m_drawToolBar->Clear();
    else
        m_drawToolBar = new wxAuiToolBar( this, ID_V_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                          KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    // Set up toolbar
    m_drawToolBar->AddTool( ID_NO_TOOL_SELECTED, wxEmptyString, KiScaledBitmap( cursor_xpm, this ),
                            wxEmptyString, wxITEM_CHECK );

    KiScaledSeparator( m_drawToolBar, this );
    m_drawToolBar->AddTool( ID_MODEDIT_PAD_TOOL, wxEmptyString, KiScaledBitmap( pad_xpm, this ),
                            _( "Add pad" ), wxITEM_CHECK );

    KiScaledSeparator( m_drawToolBar, this );
    m_drawToolBar->AddTool( ID_MODEDIT_LINE_TOOL, wxEmptyString, KiScaledBitmap( add_graphical_segments_xpm, this ),
                            _( "Add graphic line" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_MODEDIT_CIRCLE_TOOL, wxEmptyString, KiScaledBitmap( add_circle_xpm, this ),
                            _( "Add graphic circle" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_MODEDIT_ARC_TOOL, wxEmptyString, KiScaledBitmap( add_arc_xpm, this ),
                            _( "Add graphic arc" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_MODEDIT_POLYGON_TOOL, wxEmptyString, KiScaledBitmap( add_graphical_polygon_xpm, this ),
                            _( "Add graphic polygon" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_MODEDIT_TEXT_TOOL, wxEmptyString, KiScaledBitmap( text_xpm, this ),
                            _( "Add Text" ), wxITEM_CHECK );

    KiScaledSeparator( m_drawToolBar, this );
    m_drawToolBar->AddTool( ID_MODEDIT_ANCHOR_TOOL, wxEmptyString, KiScaledBitmap( anchor_xpm, this ),
                            _( "Place footprint reference anchor" ),
                            wxITEM_CHECK );

    KiScaledSeparator( m_drawToolBar, this );
    m_drawToolBar->AddTool( ID_MODEDIT_DELETE_TOOL, wxEmptyString, KiScaledBitmap( delete_xpm, this ),
                            _( "Delete item" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_MODEDIT_PLACE_GRID_COORD, wxEmptyString,
                            KiScaledBitmap( grid_select_axis_xpm, this ),
                            _( "Set grid origin" ),
                            wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_MODEDIT_MEASUREMENT_TOOL, wxEmptyString,
                            KiScaledBitmap( measurement_xpm, this ),
                            _( "Measure distance" ),
                            wxITEM_CHECK );

    m_drawToolBar->Realize();
}


void FOOTPRINT_EDIT_FRAME::ReCreateOptToolbar()
{
    if( m_optionsToolBar )
        m_optionsToolBar->Clear();
    else
        m_optionsToolBar = new wxAuiToolBar( this, ID_OPT_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                             KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString, KiScaledBitmap( grid_xpm, this ),
                               _( "Hide grid" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLAR_COORD, wxEmptyString,
                               KiScaledBitmap( polar_coord_xpm, this ),
                               _( "Display Polar Coord ON" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               KiScaledBitmap( unit_inch_xpm, this ),
                               _( "Set units to inches" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               KiScaledBitmap( unit_mm_xpm, this ),
                               _( "Set units to millimeters" ), wxITEM_CHECK );

#ifndef __APPLE__
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               KiScaledBitmap( cursor_shape_xpm, this ),
                               _( "Change Cursor Shape" ), wxITEM_CHECK  );
#else
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               KiScaledBitmap( cursor_shape_xpm, this ),
                               _( "Change cursor shape (not supported in Legacy Toolset)" ),
                               wxITEM_CHECK  );
#endif

    KiScaledSeparator( m_optionsToolBar, this );
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH, wxEmptyString,
                               KiScaledBitmap( pad_sketch_xpm, this ),
                               _( "Show Pads Sketch" ), wxITEM_CHECK  );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH, wxEmptyString,
                               KiScaledBitmap( text_sketch_xpm, this ),
                               _( "Show Texts Sketch" ), wxITEM_CHECK  );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH, wxEmptyString,
                               KiScaledBitmap( show_mod_edge_xpm, this ),
                               _( "Show Edges Sketch" ), wxITEM_CHECK  );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE, wxEmptyString,
                               KiScaledBitmap( contrast_mode_xpm, this ),
                               _( "Enable high contrast display mode" ),
                               wxITEM_CHECK );

    m_optionsToolBar->Realize();
}


void FOOTPRINT_EDIT_FRAME::ReCreateAuxiliaryToolbar()
{
    if( m_auxiliaryToolBar )
        m_auxiliaryToolBar->Clear();
    else
        m_auxiliaryToolBar = new wxAuiToolBar( this, ID_AUX_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                               KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // Set up toolbar
    KiScaledSeparator( m_auxiliaryToolBar, this );

    // Grid selection choice box.
    m_gridSelectBox = new wxChoice( m_auxiliaryToolBar,
                                      ID_ON_GRID_SELECT,
                                      wxDefaultPosition, wxDefaultSize,
                                      0, NULL );
    // Update tool bar to reflect setting.
    updateGridSelectBox();
    m_auxiliaryToolBar->AddControl( m_gridSelectBox );

    // Zoom selection choice box.
    KiScaledSeparator( m_auxiliaryToolBar, this );
    m_zoomSelectBox = new wxChoice( m_auxiliaryToolBar,
                                      ID_ON_ZOOM_SELECT,
                                      wxDefaultPosition, wxDefaultSize,
                                      0, NULL );
    updateZoomSelectBox();
    m_auxiliaryToolBar->AddControl( m_zoomSelectBox );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_auxiliaryToolBar->Realize();
}
