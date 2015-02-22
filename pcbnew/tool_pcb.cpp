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
 * @file tool_pcb.cpp
 * @brief PCB editor tool bars
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <help_common_strings.h>
#include <dialog_helpers.h>
#include <class_layer_box_selector.h>
#include <colors_selection.h>
#include <wxPcbStruct.h>

#include <class_board.h>

#include <pcbnew.h>
#include <pcbnew_id.h>
#include <hotkeys.h>
#include <class_pcb_layer_box_selector.h>

#include <wx/wupdlock.h>

#define SEL_LAYER_HELP _( \
        "Show active layer selections\nand select layer pair for route and place via" )


/* Data to build the layer pair indicator button */
static wxBitmap*  LayerPairBitmap = NULL;

#define BM_LAYERICON_SIZE 24
static const char s_BitmapLayerIcon[BM_LAYERICON_SIZE][BM_LAYERICON_SIZE] =
{
    // 0 = draw pixel with active layer color
    // 1 = draw pixel with top layer color (top/bottom layer used inautoroute and place via)
    // 2 = draw pixel with bottom layer color
    // 3 = draw pixel with via color
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 1, 1, 1, 1, 3, 0, 0, 0, 0, 0, 0, 0 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 0, 1, 1, 1, 1, 3, 3, 2, 2, 2, 2, 2, 2, 2 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 1, 1, 1, 1, 0, 3, 3, 2, 2, 2, 2, 2, 2, 2 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 1, 1, 1, 1, 0, 3, 3, 2, 2, 2, 2, 2, 2, 2 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 1, 1, 1, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};


/* Draw the icon for the "Select layer pair" bitmap tool
 */
void PCB_EDIT_FRAME::PrepareLayerIndicator()
{
    int        ii, jj;
    EDA_COLOR_T active_layer_color, Route_Layer_TOP_color,
               Route_Layer_BOTTOM_color, via_color;
    bool       change = false;
    bool first_call = LayerPairBitmap == NULL;

    static int previous_active_layer_color, previous_Route_Layer_TOP_color,
               previous_Route_Layer_BOTTOM_color, previous_via_color;

    /* get colors, and redraw bitmap button only on changes */
    active_layer_color = GetBoard()->GetLayerColor(GetActiveLayer());

    if( previous_active_layer_color != active_layer_color )
    {
        previous_active_layer_color = active_layer_color;
        change = true;
    }

    Route_Layer_TOP_color =
        g_ColorsSettings.GetLayerColor( ( ( PCB_SCREEN* ) GetScreen() )->m_Route_Layer_TOP );

    if( previous_Route_Layer_TOP_color != Route_Layer_TOP_color )
    {
        previous_Route_Layer_TOP_color = Route_Layer_TOP_color;
        change = true;
    }

    Route_Layer_BOTTOM_color =
        g_ColorsSettings.GetLayerColor( ( (PCB_SCREEN*) GetScreen() )->m_Route_Layer_BOTTOM );

    if( previous_Route_Layer_BOTTOM_color != Route_Layer_BOTTOM_color )
    {
        previous_Route_Layer_BOTTOM_color = Route_Layer_BOTTOM_color;
        change = true;
    }

    int via_type = GetDesignSettings().m_CurrentViaType;
    via_color = GetBoard()->GetVisibleElementColor(VIAS_VISIBLE+via_type);

    if( previous_via_color != via_color )
    {
        previous_via_color = via_color;
        change = true;
    }

    if( !change && (LayerPairBitmap != NULL) )
        return;

    /* Create the bitmap and its Memory DC, if not already made */
    if( LayerPairBitmap == NULL )
    {
        LayerPairBitmap = new wxBitmap( 24, 24 );
    }

    /* Draw the icon, with colors according to the active layer and layer
     * pairs for via command (change layer)
     */
    wxMemoryDC iconDC;
    iconDC.SelectObject( *LayerPairBitmap );
    wxPen      pen;
    int buttonColor = -1;

    for( ii = 0; ii < BM_LAYERICON_SIZE; ii++ )
    {
        for( jj = 0; jj < BM_LAYERICON_SIZE; jj++ )
        {
            if( s_BitmapLayerIcon[ii][jj] != buttonColor )
            {
                switch( s_BitmapLayerIcon[ii][jj] )
                {
                default:
                case 0:
                    pen.SetColour( MakeColour( active_layer_color ) );
                    break;

                case 1:
                    pen.SetColour( MakeColour( Route_Layer_TOP_color) );
                    break;

                case 2:
                    pen.SetColour( MakeColour( Route_Layer_BOTTOM_color ) );
                    break;

                case 3:
                    pen.SetColour( MakeColour( via_color ) );
                    break;
                }

                buttonColor = s_BitmapLayerIcon[ii][jj];
                iconDC.SetPen( pen );
            }

            iconDC.DrawPoint( jj, ii );
        }
    }

    /* Deselect the Tool Bitmap from DC,
     *  in order to delete the MemoryDC safely without deleting the bitmap */
    iconDC.SelectObject( wxNullBitmap );

    if( m_mainToolBar && ! first_call )
    {
        m_mainToolBar->SetToolBitmap( ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR, *LayerPairBitmap );
        m_mainToolBar->Refresh();
    }
}


/* Creates or updates the main horizontal toolbar for the board editor
*/
void PCB_EDIT_FRAME::ReCreateHToolbar()
{
    wxString msg;

    if( m_mainToolBar )
        return;

    wxWindowUpdateLocker dummy( this );

    m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                      wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // Set up toolbar
    if( Kiface().IsSingle() )
    {
        m_mainToolBar->AddTool( ID_NEW_BOARD, wxEmptyString, KiBitmap( new_pcb_xpm ),
                                _( "New board" ) );
        m_mainToolBar->AddTool( ID_LOAD_FILE, wxEmptyString, KiBitmap( open_brd_file_xpm ),
                                _( "Open existing board" ) );
    }

    m_mainToolBar->AddTool( ID_SAVE_BOARD, wxEmptyString, KiBitmap( save_xpm ),
                            _( "Save board" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_SHEET_SET, wxEmptyString, KiBitmap( sheetset_xpm ),
                            _( "Page settings for paper size and texts" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_OPEN_MODULE_EDITOR, wxEmptyString,
                            KiBitmap( module_editor_xpm ),
                            _( "Open footprint editor" ) );

    m_mainToolBar->AddTool( ID_OPEN_MODULE_VIEWER, wxEmptyString,
                            KiBitmap( modview_icon_xpm ),
                            _( "Open footprint viewer" ) );

    m_mainToolBar->AddSeparator();
    msg = AddHotkeyName( HELP_UNDO, g_Board_Editor_Hokeys_Descr, HK_UNDO, IS_COMMENT );
    m_mainToolBar->AddTool( wxID_UNDO, wxEmptyString, KiBitmap( undo_xpm ), msg );
    msg = AddHotkeyName( HELP_REDO, g_Board_Editor_Hokeys_Descr, HK_REDO, IS_COMMENT );
    m_mainToolBar->AddTool( wxID_REDO, wxEmptyString, KiBitmap( redo_xpm ), msg );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( wxID_PRINT, wxEmptyString, KiBitmap( print_button_xpm ),
                            _( "Print board" ) );
    m_mainToolBar->AddTool( ID_GEN_PLOT, wxEmptyString, KiBitmap( plot_xpm ),
                            _( "Plot (HPGL, PostScript, or GERBER format)" ) );

    m_mainToolBar->AddSeparator();
    msg = AddHotkeyName( HELP_ZOOM_IN, g_Board_Editor_Hokeys_Descr, HK_ZOOM_IN, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, KiBitmap( zoom_in_xpm ), msg );

    msg = AddHotkeyName( HELP_ZOOM_OUT, g_Board_Editor_Hokeys_Descr, HK_ZOOM_OUT, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, KiBitmap( zoom_out_xpm ), msg );

    msg = AddHotkeyName( HELP_ZOOM_REDRAW, g_Board_Editor_Hokeys_Descr, HK_ZOOM_REDRAW,
                         IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString, KiBitmap( zoom_redraw_xpm ), msg );

    msg = AddHotkeyName( HELP_ZOOM_FIT, g_Board_Editor_Hokeys_Descr, HK_ZOOM_AUTO, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString, KiBitmap( zoom_fit_in_page_xpm ), msg );

    m_mainToolBar->AddSeparator();
    msg = AddHotkeyName( HELP_FIND, g_Board_Editor_Hokeys_Descr, HK_FIND_ITEM, IS_COMMENT );
    m_mainToolBar->AddTool( ID_FIND_ITEMS, wxEmptyString, KiBitmap( find_xpm ), msg );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_GET_NETLIST, wxEmptyString, KiBitmap( netlist_xpm ),
                            _( "Read netlist" ) );
    m_mainToolBar->AddTool( ID_DRC_CONTROL, wxEmptyString, KiBitmap( erc_xpm ),
                            _( "Perform design rules check" ) );

    m_mainToolBar->AddSeparator();

    if( m_SelLayerBox == NULL )
    {
        m_SelLayerBox = new PCB_LAYER_BOX_SELECTOR( m_mainToolBar, ID_TOOLBARH_PCB_SELECT_LAYER );
        m_SelLayerBox->SetBoardFrame( this );
    }

    ReCreateLayerBox( false );
    m_mainToolBar->AddControl( m_SelLayerBox );

    PrepareLayerIndicator();    // Initialize the bitmap with current
                                // active layer colors for the next tool
    m_mainToolBar->AddTool( ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR, wxEmptyString,
                            *LayerPairBitmap, SEL_LAYER_HELP );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_TOOLBARH_PCB_MODE_MODULE, wxEmptyString, KiBitmap( mode_module_xpm ),
                            _( "Mode footprint: manual and automatic movement and placement" ),
                            wxITEM_CHECK );
    m_mainToolBar->AddTool( ID_TOOLBARH_PCB_MODE_TRACKS, wxEmptyString, KiBitmap( mode_track_xpm ),
                            _( "Mode track: autorouting" ), wxITEM_CHECK );

    // Fast call to FreeROUTE Web Bases router
    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_TOOLBARH_PCB_FREEROUTE_ACCESS, wxEmptyString,
                            KiBitmap( web_support_xpm ),
                            _( "Fast access to the FreeROUTE external advanced router" ) );

    // Access to the scripting console
#if defined(KICAD_SCRIPTING_WXPYTHON)
    m_mainToolBar->AddSeparator();

    m_mainToolBar->AddTool( ID_TOOLBARH_PCB_SCRIPTING_CONSOLE, wxEmptyString,
                            KiBitmap( py_script_xpm ),
                            _( "Show/Hide the Python Scripting console" ),
                            wxITEM_CHECK );
#endif

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}


void PCB_EDIT_FRAME::ReCreateOptToolbar()
{
    if( m_optionsToolBar )
        return;

    wxWindowUpdateLocker dummy( this );

    m_optionsToolBar = new wxAuiToolBar( this, ID_OPT_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                         wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_VERTICAL );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_DRC_OFF, wxEmptyString, KiBitmap( drc_off_xpm ),
                               _( "Enable design rule checking" ), wxITEM_CHECK );
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString, KiBitmap( grid_xpm ),
                               _( "Hide grid" ), wxITEM_CHECK );
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLAR_COORD, wxEmptyString,
                               KiBitmap( polar_coord_xpm ),
                               _( "Display polar coordinates" ), wxITEM_CHECK );
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               KiBitmap( unit_inch_xpm ),
                               _( "Units in inches" ), wxITEM_CHECK );
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               KiBitmap( unit_mm_xpm ),
                               _( "Units in millimeters" ), wxITEM_CHECK );
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               KiBitmap( cursor_shape_xpm ),
                               _( "Change cursor shape" ), wxITEM_CHECK );

    m_optionsToolBar->AddSeparator();
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_RATSNEST, wxEmptyString,
                               KiBitmap( general_ratsnest_xpm ),
                               _( "Show board ratsnest" ), wxITEM_CHECK );
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MODULE_RATSNEST, wxEmptyString,
                               KiBitmap( local_ratsnest_xpm ),
                               _( "Show footprint ratsnest when moving" ),
                               wxITEM_CHECK );

    m_optionsToolBar->AddSeparator();
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_AUTO_DEL_TRACK, wxEmptyString,
                               KiBitmap( auto_delete_track_xpm ),
                               _( "Enable automatic track deletion" ),
                               wxITEM_CHECK );

    m_optionsToolBar->AddSeparator();
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_ZONES, wxEmptyString, KiBitmap( show_zone_xpm ),
                               _( "Show filled areas in zones" ), wxITEM_CHECK );
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_ZONES_DISABLE, wxEmptyString,
                               KiBitmap( show_zone_disable_xpm ),
                               _( "Do not show filled areas in zones" ) , wxITEM_CHECK );
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_ZONES_OUTLINES_ONLY, wxEmptyString,
                               KiBitmap( show_zone_outline_only_xpm ),
                               _( "Show outlines of filled areas only in zones" ), wxITEM_CHECK );

    m_optionsToolBar->AddSeparator();
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH, wxEmptyString,
                               KiBitmap( pad_sketch_xpm ),
                               _( "Show pads in outline mode" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_VIAS_SKETCH, wxEmptyString,
                               KiBitmap( via_sketch_xpm ),
                               _( "Show vias in outline mode" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_TRACKS_SKETCH, wxEmptyString,
                               KiBitmap( showtrack_xpm ),
                               _( "Show tracks in outline mode" ),
                               wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE, wxEmptyString,
                               KiBitmap( contrast_mode_xpm ),
                               _( "Enable high contrast display mode" ),
                               wxITEM_CHECK );

    // Tools to show/hide toolbars:
    m_optionsToolBar->AddSeparator();
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MANAGE_LAYERS_VERTICAL_TOOLBAR,
                               wxEmptyString,
                               KiBitmap( layers_manager_xpm ),
                               HELP_SHOW_HIDE_LAYERMANAGER,
                               wxITEM_CHECK );
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_EXTRA_VERTICAL_TOOLBAR_MICROWAVE,
                               wxEmptyString,
                               KiBitmap( mw_toolbar_xpm ),
                               HELP_SHOW_HIDE_MICROWAVE_TOOLS,
                               wxITEM_CHECK );


    m_optionsToolBar->AddSeparator();
    m_optionsToolBar->Realize();
}


/* Create the main vertical right toolbar, showing usual tools
 */
void PCB_EDIT_FRAME::ReCreateVToolbar()
{
    if( m_drawToolBar )
        return;

    wxWindowUpdateLocker dummy( this );

    m_drawToolBar = new wxAuiToolBar( this, ID_V_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                      wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_VERTICAL );

    // Set up toolbar
    m_drawToolBar->AddTool( ID_NO_TOOL_SELECTED, wxEmptyString, KiBitmap( cursor_xpm ),
                            wxEmptyString, wxITEM_CHECK );
    m_drawToolBar->AddSeparator();

    m_drawToolBar->AddTool( ID_PCB_HIGHLIGHT_BUTT, wxEmptyString, KiBitmap( net_highlight_xpm ),
                            _( "Highlight net" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_PCB_SHOW_1_RATSNEST_BUTT, wxEmptyString,
                            KiBitmap( tool_ratsnest_xpm ),
                            _( "Display local ratsnest" ), wxITEM_CHECK );

    m_drawToolBar->AddSeparator();
    m_drawToolBar->AddTool( ID_PCB_MODULE_BUTT, wxEmptyString, KiBitmap( module_xpm ),
                            _( "Add footprints" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_TRACK_BUTT, wxEmptyString, KiBitmap( add_tracks_xpm ),
                            _( "Add tracks and vias" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_PCB_ZONES_BUTT, wxEmptyString, KiBitmap( add_zone_xpm ),
                            _( "Add filled zones" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_PCB_KEEPOUT_AREA_BUTT, wxEmptyString,
                            KiBitmap( add_keepout_area_xpm ),
                            _( "Add keepout areas" ), wxITEM_CHECK );

    m_drawToolBar->AddSeparator();
    m_drawToolBar->AddTool( ID_PCB_ADD_LINE_BUTT, wxEmptyString, KiBitmap( add_dashed_line_xpm ),
                            _( "Add graphic line or polygon" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_PCB_CIRCLE_BUTT, wxEmptyString, KiBitmap( add_circle_xpm ),
                            _( "Add graphic circle" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_PCB_ARC_BUTT, wxEmptyString, KiBitmap( add_arc_xpm ),
                            _( "Add graphic arc" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_PCB_ADD_TEXT_BUTT, wxEmptyString, KiBitmap( add_text_xpm ),
                            _( "Add text on copper layers or graphic text" ), wxITEM_CHECK );

    m_drawToolBar->AddSeparator();
    m_drawToolBar->AddTool( ID_PCB_DIMENSION_BUTT, wxEmptyString, KiBitmap( add_dimension_xpm ),
                            _( "Add dimension" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_PCB_MIRE_BUTT, wxEmptyString, KiBitmap( add_mires_xpm ),
                            _( "Add layer alignment target" ), wxITEM_CHECK );

    m_drawToolBar->AddSeparator();
    m_drawToolBar->AddTool( ID_PCB_DELETE_ITEM_BUTT, wxEmptyString, KiBitmap( delete_xpm ),
                            _( "Delete items" ), wxITEM_CHECK );

    m_drawToolBar->AddSeparator();
    m_drawToolBar->AddTool( ID_PCB_PLACE_OFFSET_COORD_BUTT, wxEmptyString,
                            KiBitmap( pcb_offset_xpm ),
                            _( "Place the origin point for drill and place files" ),
                            wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_PCB_PLACE_GRID_COORD_BUTT, wxEmptyString,
                            KiBitmap( grid_select_axis_xpm ),
                            _( "Set the origin point for the grid" ),
                            wxITEM_CHECK );

    m_drawToolBar->Realize();
}


/* Create the auxiliary vertical right toolbar, showing tools for microwave applications
 */
void PCB_EDIT_FRAME::ReCreateMicrowaveVToolbar()
{
    if( m_microWaveToolBar )
        return;

    wxWindowUpdateLocker dummy(this);

    m_microWaveToolBar = new wxAuiToolBar( this, ID_MICROWAVE_V_TOOLBAR, wxDefaultPosition,
                                           wxDefaultSize,
                                           wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_VERTICAL );

    // Set up toolbar
    m_microWaveToolBar->AddTool( ID_PCB_MUWAVE_TOOL_SELF_CMD, wxEmptyString,
                                 KiBitmap( mw_add_line_xpm ),
                                 _( "Create line of specified length for microwave applications" ),
                                 wxITEM_CHECK );

    m_microWaveToolBar->AddTool( ID_PCB_MUWAVE_TOOL_GAP_CMD, wxEmptyString,
                                 KiBitmap( mw_add_gap_xpm ),
                                 _( "Create gap of specified length for microwave applications" ),
                                 wxITEM_CHECK );

    m_microWaveToolBar->AddSeparator();

    m_microWaveToolBar->AddTool( ID_PCB_MUWAVE_TOOL_STUB_CMD, wxEmptyString,
                                 KiBitmap( mw_add_stub_xpm ),
                                 _( "Create stub of specified length for microwave applications" ),
                                 wxITEM_CHECK );

    m_microWaveToolBar->AddTool( ID_PCB_MUWAVE_TOOL_STUB_ARC_CMD, wxEmptyString,
                                 KiBitmap( mw_add_stub_arc_xpm ),
                                 _( "Create stub (arc) of specified length for microwave applications" ),
                                 wxITEM_CHECK );

    m_microWaveToolBar->AddTool( ID_PCB_MUWAVE_TOOL_FUNCTION_SHAPE_CMD, wxEmptyString,
                                 KiBitmap( mw_add_shape_xpm ),
                                 _( "Create a polynomial shape for microwave applications" ),
                                 wxITEM_CHECK );

    m_microWaveToolBar->Realize();
}


/* Creates auxiliary horizontal toolbar
 * displays:
 * existing track width choice
 * selection for auto track width
 * existing via size choice
 * Current strategy (to choose the track and via sizes)
 * grid size choice
 * zoom level choice
 */
void PCB_EDIT_FRAME::ReCreateAuxiliaryToolbar()
{
    wxString msg;

    wxWindowUpdateLocker dummy( this );

    if( m_auxiliaryToolBar )
    {
        updateTraceWidthSelectBox();
        updateViaSizeSelectBox();

        // combobox sizes can have changed: apply new best sizes
        wxAuiToolBarItem* item = m_auxiliaryToolBar->FindTool( ID_AUX_TOOLBAR_PCB_TRACK_WIDTH );
        item->SetMinSize( m_SelTrackWidthBox->GetBestSize() );
        item = m_auxiliaryToolBar->FindTool( ID_AUX_TOOLBAR_PCB_VIA_SIZE );
        item->SetMinSize( m_SelViaSizeBox->GetBestSize() );

        m_auxiliaryToolBar->Realize();
        m_auimgr.Update();
        return;
    }

    m_auxiliaryToolBar = new wxAuiToolBar( this, ID_AUX_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                           wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );

    /* Set up toolbar items */

    // Creates box to display and choose tracks widths:
    m_SelTrackWidthBox = new wxComboBox( m_auxiliaryToolBar,
                                         ID_AUX_TOOLBAR_PCB_TRACK_WIDTH,
                                         wxEmptyString,
                                         wxDefaultPosition, wxDefaultSize,
                                         0, NULL, wxCB_READONLY );
    updateTraceWidthSelectBox();
    m_auxiliaryToolBar->AddControl( m_SelTrackWidthBox );
//    m_auxiliaryToolBar->AddSeparator();

    // Creates box to display and choose vias diameters:
    m_SelViaSizeBox = new wxComboBox( m_auxiliaryToolBar,
                                      ID_AUX_TOOLBAR_PCB_VIA_SIZE,
                                      wxEmptyString,
                                      wxDefaultPosition, wxDefaultSize,
                                      0, NULL, wxCB_READONLY );
    updateViaSizeSelectBox();
    m_auxiliaryToolBar->AddControl( m_SelViaSizeBox );
    m_auxiliaryToolBar->AddSeparator();

    // Creates box to display and choose strategy to handle tracks an vias sizes:
    m_auxiliaryToolBar->AddTool( ID_AUX_TOOLBAR_PCB_SELECT_AUTO_WIDTH,
                                 wxEmptyString,
                                 KiBitmap( auto_track_width_xpm ),
                                 _( "Auto track width: when starting on \
an existing track use its width\notherwise, use current width setting" ),
                                 wxITEM_CHECK );

    // Add the box to display and select the current grid size:
    m_auxiliaryToolBar->AddSeparator();
    m_gridSelectBox = new wxComboBox( m_auxiliaryToolBar,
                                      ID_ON_GRID_SELECT,
                                      wxEmptyString,
                                      wxDefaultPosition, wxDefaultSize,
                                      0, NULL, wxCB_READONLY );
    updateGridSelectBox();
    m_auxiliaryToolBar->AddControl( m_gridSelectBox );

    //  Add the box to display and select the current Zoom
    m_auxiliaryToolBar->AddSeparator();
    m_zoomSelectBox = new wxComboBox( m_auxiliaryToolBar,
                                      ID_ON_ZOOM_SELECT,
                                      wxEmptyString,
                                      wxDefaultPosition, wxDefaultSize,
                                      0, NULL, wxCB_READONLY );
    updateZoomSelectBox();
    m_auxiliaryToolBar->AddControl( m_zoomSelectBox );

    // after adding the buttons to the toolbar, must call Realize()
    m_auxiliaryToolBar->Realize();
//    m_auxiliaryToolBar->AddSeparator();
}


void PCB_EDIT_FRAME::updateTraceWidthSelectBox()
{
    if( m_SelTrackWidthBox == NULL )
        return;

    wxString msg;
    bool mmFirst = g_UserUnit != INCHES;

    m_SelTrackWidthBox->Clear();

    for( unsigned ii = 0; ii < GetDesignSettings().m_TrackWidthList.size(); ii++ )
    {
        int size = GetDesignSettings().m_TrackWidthList[ii];

        double valueMils = To_User_Unit( INCHES, size ) * 1000;
        double value_mm = To_User_Unit( MILLIMETRES, size );

        if( mmFirst )
            msg.Printf( _( "Track: %.3f mm (%.2f mils)" ),
                        value_mm, valueMils );
        else
            msg.Printf( _( "Track: %.2f mils (%.3f mm)" ),
                        valueMils, value_mm );

        // Mark the netclass track width value (the first in list)
        if( ii == 0 )
            msg << wxT( " *" );

        m_SelTrackWidthBox->Append( msg );
    }

    if( GetDesignSettings().GetTrackWidthIndex() >= GetDesignSettings().m_TrackWidthList.size() )
        GetDesignSettings().SetTrackWidthIndex( 0 );

    m_SelTrackWidthBox->SetSelection( GetDesignSettings().GetTrackWidthIndex() );
}


void PCB_EDIT_FRAME::updateViaSizeSelectBox()
{
    if( m_SelViaSizeBox == NULL )
        return;

    wxString msg;

    m_SelViaSizeBox->Clear();
    bool mmFirst = g_UserUnit != INCHES;

    for( unsigned ii = 0; ii < GetDesignSettings().m_ViasDimensionsList.size(); ii++ )
    {
        int diam = GetDesignSettings().m_ViasDimensionsList[ii].m_Diameter;

        double valueMils = To_User_Unit( INCHES, diam ) * 1000;
        double value_mm = To_User_Unit( MILLIMETRES, diam );

        if( mmFirst )
            msg.Printf( _( "Via: %.2f mm (%.1f mils)" ),
                        value_mm, valueMils );
        else
            msg.Printf( _( "Via: %.1f mils (%.2f mm)" ),
                        valueMils, value_mm );

        int hole = GetDesignSettings().m_ViasDimensionsList[ii].m_Drill;

        if( hole )
        {
            msg  << wxT("/ ");
            wxString hole_str;
            double valueMils = To_User_Unit( INCHES, hole ) * 1000;
            double value_mm = To_User_Unit( MILLIMETRES, hole );

            if( mmFirst )
                hole_str.Printf( _( "%.2f mm (%.1f mils)" ),
                            value_mm, valueMils );
            else
                hole_str.Printf( _( "%.1f mils (%.2f mm)" ),
                            valueMils, value_mm );

            msg += hole_str;
        }

        // Mark the netclass via size value (the first in list)
        if( ii == 0 )
            msg << wxT( " *" );

        m_SelViaSizeBox->Append( msg );
    }

    if( GetDesignSettings().GetViaSizeIndex() >= GetDesignSettings().m_ViasDimensionsList.size() )
        GetDesignSettings().SetViaSizeIndex( 0 );

    m_SelViaSizeBox->SetSelection( GetDesignSettings().GetViaSizeIndex() );
}


void PCB_EDIT_FRAME::ReCreateLayerBox( bool aForceResizeToolbar )
{
    if( m_SelLayerBox == NULL || m_mainToolBar == NULL )
        return;

    m_SelLayerBox->SetToolTip( _( "+/- to switch" ) );
    m_SelLayerBox->m_hotkeys = g_Board_Editor_Hokeys_Descr;
    m_SelLayerBox->Resync();

    if( aForceResizeToolbar )
    {
        // the layer box can have its size changed
        // Update the aui manager, to take in account the new size
        m_auimgr.Update();
    }
}
