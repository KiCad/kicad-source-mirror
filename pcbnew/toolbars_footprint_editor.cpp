/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <tool/actions.h>
#include <pcbnew.h>
#include <footprint_edit_frame.h>
#include <dialog_helpers.h>
#include <pcbnew_id.h>
#include <hotkeys.h>
#include <bitmaps.h>


void FOOTPRINT_EDIT_FRAME::ReCreateHToolbar()
{
    // Note:
    // To rebuild the aui toolbar, the more easy way is to clear ( calling m_mainToolBar.Clear() )
    // all wxAuiToolBarItems.
    // However the wxAuiToolBarItems are not the owners of controls managed by
    // them ( m_zoomSelectBox and m_gridSelectBox ), and therefore do not delete them
    // So we do not recreate them after clearing the tools.

    if( m_mainToolBar )
        m_mainToolBar->Clear();
    else
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    wxString msg;

    // Set up toolbar
    m_mainToolBar->AddTool( ID_MODEDIT_NEW_MODULE, wxEmptyString,
                            KiScaledBitmap( new_footprint_xpm, this ),
                            _( "New footprint" ) );

#ifdef KICAD_SCRIPTING
    m_mainToolBar->AddTool( ID_MODEDIT_NEW_MODULE_FROM_WIZARD, wxEmptyString,
                            KiScaledBitmap( module_wizard_xpm, this ),
                            _( "New footprint using footprint wizard" ) );
#endif

    if( IsCurrentFPFromBoard() )
    {
        m_mainToolBar->AddTool( ID_MODEDIT_SAVE, wxEmptyString,
                                KiScaledBitmap( save_fp_to_board_xpm, this ),
                                _( "Update footprint on board" ) );
    }
    else
    {
        m_mainToolBar->AddTool( ID_MODEDIT_SAVE, wxEmptyString,
                                KiScaledBitmap( save_xpm, this ),
                                _( "Save changes to library" ) );
    }

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( wxID_PRINT, wxEmptyString,
                            KiScaledBitmap( print_button_xpm, this ),
                            _( "Print footprint" ) );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( wxID_UNDO, wxEmptyString, KiScaledBitmap( undo_xpm, this ),
                            _( "Undo last edit" ) );
    m_mainToolBar->AddTool( wxID_REDO, wxEmptyString, KiScaledBitmap( redo_xpm, this ),
                            _( "Redo last undo command" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->Add( ACTIONS::zoomRedraw );
    m_mainToolBar->Add( ACTIONS::zoomInCenter );
    m_mainToolBar->Add( ACTIONS::zoomOutCenter );
    m_mainToolBar->Add( ACTIONS::zoomFitScreen );
    m_mainToolBar->Add( ACTIONS::zoomTool, ACTION_TOOLBAR::TOGGLE );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_MODEDIT_EDIT_MODULE_PROPERTIES, wxEmptyString,
                            KiScaledBitmap( module_options_xpm, this ),
                            _( "Footprint properties" ) );

    m_mainToolBar->AddTool( ID_MODEDIT_PAD_SETTINGS, wxEmptyString,
                            KiScaledBitmap( options_pad_xpm, this ),
                            _( "Default pad properties" ) );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_MODEDIT_LOAD_MODULE_FROM_BOARD, wxEmptyString,
                            KiScaledBitmap( load_module_board_xpm, this ),
                            _( "Load footprint from current board" ) );

    m_mainToolBar->AddTool( ID_ADD_FOOTPRINT_TO_BOARD, wxEmptyString,
                            KiScaledBitmap( export_xpm, this ),
                            _( "Insert footprint into current board" ) );

#if 0       // Currently there is no check footprint function defined, so do not show this tool
    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_MODEDIT_CHECK, wxEmptyString,
                            KiScaledBitmap( module_check_xpm, this ),
                            _( "Check footprint" ) );
#endif

    KiScaledSeparator( m_mainToolBar, this );

    // Grid selection choice box.
    if( m_gridSelectBox == nullptr )
        m_gridSelectBox = new wxChoice( m_mainToolBar, ID_ON_GRID_SELECT,
                                    wxDefaultPosition, wxDefaultSize, 0, NULL );

    UpdateGridSelectBox();
    m_mainToolBar->AddControl( m_gridSelectBox );

    KiScaledSeparator( m_mainToolBar, this );

    // Zoom selection choice box.
    if( m_zoomSelectBox == nullptr )
        m_zoomSelectBox = new wxChoice( m_mainToolBar, ID_ON_ZOOM_SELECT,
                                    wxDefaultPosition, wxDefaultSize, 0, NULL );

    updateZoomSelectBox();
    m_mainToolBar->AddControl( m_zoomSelectBox );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}


void FOOTPRINT_EDIT_FRAME::ReCreateVToolbar()
{
    if( m_drawToolBar )
        m_drawToolBar->Clear();
    else
        m_drawToolBar = new ACTION_TOOLBAR( this, ID_V_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    // Set up toolbar
    m_drawToolBar->AddTool( ID_NO_TOOL_SELECTED, wxEmptyString, KiScaledBitmap( cursor_xpm, this ),
                            _( "Select item" ), wxITEM_CHECK );

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
        m_optionsToolBar = new ACTION_TOOLBAR( this, ID_OPT_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                               KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    m_optionsToolBar->Add( ACTIONS::toggleGrid,          ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLAR_COORD, wxEmptyString,
                               KiScaledBitmap( polar_coord_xpm, this ),
                               _( "Display Polar Coord ON" ), wxITEM_CHECK );

    m_optionsToolBar->Add( ACTIONS::imperialUnits,       ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::metricUnits,         ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::toggleCursorStyle,   ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->AddSeparator();
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

    KiScaledSeparator( m_optionsToolBar, this );
    m_optionsToolBar->AddTool( ID_MODEDIT_SHOW_HIDE_SEARCH_TREE, wxEmptyString,
                               KiScaledBitmap( search_tree_xpm, this ),
                               _( "Toggles the search tree" ), wxITEM_CHECK );

    m_optionsToolBar->Realize();
}


void FOOTPRINT_EDIT_FRAME::SyncMenusAndToolbars()
{
    PCB_DISPLAY_OPTIONS* opts = (PCB_DISPLAY_OPTIONS*) GetDisplayOptions();

    m_optionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH, !opts->m_DisplayPadFill );
    m_optionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH, !opts->m_DisplayModTextFill );
    m_optionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH, !opts->m_DisplayModEdgeFill );

    m_optionsToolBar->Toggle( ACTIONS::toggleGrid, IsGridVisible() );
    m_optionsToolBar->Toggle( ACTIONS::metricUnits, GetUserUnits() != INCHES );
    m_optionsToolBar->Toggle( ACTIONS::imperialUnits, GetUserUnits() == INCHES );

    m_optionsToolBar->Refresh();
}
