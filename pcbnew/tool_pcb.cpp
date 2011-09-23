/**
 * @file tool_pcb.cpp
 * @brief PCB editor tool bars
 */

#include "fctsys.h"
#include "help_common_strings.h"
#include "dialog_helpers.h"
#include "class_layer_box_selector.h"
#include "colors_selection.h"
#include "wxPcbStruct.h"

#include "class_board.h"

#include "pcbnew.h"
#include "pcbnew_id.h"
#include "hotkeys.h"

#include "wx/wupdlock.h"


#ifdef __UNIX__
#define LISTBOX_WIDTH 150
#else
#define LISTBOX_WIDTH 130
#endif

#define SEL_LAYER_HELP _( \
        "Show active layer selections\nand select layer pair for route and place via" )


/* Data to build the layer pair indicator button */
static wxBitmap*  LayerPairBitmap = NULL;

static const char s_BitmapLayerIcon[24][24] = {
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
    int        active_layer_color, Route_Layer_TOP_color,
               Route_Layer_BOTTOM_color, via_color;
    bool       change = false;
    bool first_call = LayerPairBitmap == NULL;

    static int previous_active_layer_color, previous_Route_Layer_TOP_color,
               previous_Route_Layer_BOTTOM_color, previous_via_color;

    /* get colors, and redraw bitmap button only on changes */
    active_layer_color = GetBoard()->GetLayerColor(getActiveLayer());

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

    int via_type = GetBoard()->GetBoardDesignSettings()->m_CurrentViaType;
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

    for( ii = 0; ii < 24; ii++ )
    {
        for( jj = 0; jj < 24; jj++ )
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

    if( m_HToolBar && ! first_call )
    {
        m_HToolBar->SetToolBitmap( ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR, *LayerPairBitmap );
        m_HToolBar->Refresh();
    }
}


/* Creates or updates the main horizontal toolbar for the board editor
*/
void PCB_EDIT_FRAME::ReCreateHToolbar()
{
    wxString msg;

    if( m_HToolBar )
        return;

    wxWindowUpdateLocker dummy( this );

    m_HToolBar = new EDA_TOOLBAR( TOOLBAR_MAIN, this, ID_H_TOOLBAR, true );
    m_HToolBar->SetRows( 1 );

    // Set up toolbar
    m_HToolBar->AddTool( ID_NEW_BOARD, wxEmptyString, KiBitmap( new_xpm ),
                         _( "New board" ) );
    m_HToolBar->AddTool( ID_LOAD_FILE, wxEmptyString, KiBitmap( open_document_xpm ),
                         _( "Open existing board" ) );
    m_HToolBar->AddTool( ID_SAVE_BOARD, wxEmptyString, KiBitmap( save_xpm ),
                         _( "Save board" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_SHEET_SET, wxEmptyString, KiBitmap( sheetset_xpm ),
                         _( "Page settings for paper size and texts" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_OPEN_MODULE_EDITOR, wxEmptyString, KiBitmap( modedit_xpm ),
                         _( "Open module editor" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( wxID_CUT, wxEmptyString, KiBitmap( cut_button_xpm ),
                         _( "Cut selected item" ) );

#if 0
    m_HToolBar->AddTool( wxID_COPY, wxEmptyString, KiBitmap( copy_button_xpm ),
                         _( "Copy selected item" ) );

    m_HToolBar->AddTool( wxID_PASTE, wxEmptyString, KiBitmap( paste_xpm ),
                         _( "Paste" ) );
#endif

    m_HToolBar->AddSeparator();
    msg = AddHotkeyName( HELP_UNDO, g_Board_Editor_Hokeys_Descr, HK_UNDO, IS_COMMENT );
    m_HToolBar->AddTool( wxID_UNDO, wxEmptyString, KiBitmap( undo_xpm ), HELP_UNDO );
    msg = AddHotkeyName( HELP_REDO, g_Board_Editor_Hokeys_Descr, HK_REDO, IS_COMMENT );
    m_HToolBar->AddTool( wxID_REDO, wxEmptyString, KiBitmap( redo_xpm ), HELP_REDO );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( wxID_PRINT, wxEmptyString, KiBitmap( print_button_xpm ),
                         _( "Print board" ) );
    m_HToolBar->AddTool( ID_GEN_PLOT, wxEmptyString, KiBitmap( plot_xpm ),
                         _( "Plot (HPGL, PostScript, or GERBER format)" ) );

    m_HToolBar->AddSeparator();
    msg = AddHotkeyName( HELP_ZOOM_IN, g_Board_Editor_Hokeys_Descr, HK_ZOOM_IN, IS_COMMENT );
    m_HToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, KiBitmap( zoom_in_xpm ), msg );

    msg = AddHotkeyName( HELP_ZOOM_OUT, g_Board_Editor_Hokeys_Descr, HK_ZOOM_OUT, IS_COMMENT );
    m_HToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, KiBitmap( zoom_out_xpm ), msg );

    msg = AddHotkeyName( HELP_ZOOM_REDRAW, g_Board_Editor_Hokeys_Descr, HK_ZOOM_REDRAW, IS_COMMENT );
    m_HToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString, KiBitmap( zoom_redraw_xpm ), msg );

    msg = AddHotkeyName( HELP_ZOOM_FIT, g_Board_Editor_Hokeys_Descr, HK_ZOOM_AUTO, IS_COMMENT );
    m_HToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString, KiBitmap( zoom_fit_in_page_xpm ), msg );

    m_HToolBar->AddSeparator();
    msg = AddHotkeyName( HELP_FIND, g_Board_Editor_Hokeys_Descr, HK_FIND_ITEM, IS_COMMENT );
    m_HToolBar->AddTool( ID_FIND_ITEMS, wxEmptyString, KiBitmap( find_xpm ), msg );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_GET_NETLIST, wxEmptyString, KiBitmap( netlist_xpm ),
                         _( "Read netlist" ) );
    m_HToolBar->AddTool( ID_DRC_CONTROL, wxEmptyString, KiBitmap( erc_xpm ),
                         _( "Perform design rules check" ) );

    m_HToolBar->AddSeparator();

    if( m_SelLayerBox == NULL )
        m_SelLayerBox = new LAYER_BOX_SELECTOR( m_HToolBar, ID_TOOLBARH_PCB_SELECT_LAYER );

    ReCreateLayerBox( m_HToolBar );
    m_HToolBar->AddControl( m_SelLayerBox );

    PrepareLayerIndicator();    // Initialize the bitmap with current
                                // active layer colors for the next tool
    m_HToolBar->AddTool( ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR, wxEmptyString,
                         *LayerPairBitmap, SEL_LAYER_HELP );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_TOOLBARH_PCB_MODE_MODULE, wxEmptyString, KiBitmap( mode_module_xpm ),
                         _( "Mode footprint: manual and automatic move and place modules" ),
                         wxITEM_CHECK );
    m_HToolBar->AddTool( ID_TOOLBARH_PCB_MODE_TRACKS, wxEmptyString, KiBitmap( mode_track_xpm ),
                         _( "Mode track: autorouting" ), wxITEM_CHECK );

    // Fast call to FreeROUTE Web Bases router
    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_TOOLBARH_PCB_FREEROUTE_ACCESS, wxEmptyString,
                         KiBitmap( web_support_xpm ),
                         _( "Fast access to the Web Based FreeROUTE advanced router" ) );

    m_HToolBar->AddSeparator();

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_HToolBar->Realize();
}


void PCB_EDIT_FRAME::ReCreateOptToolbar()
{
    if( m_OptionsToolBar )
        return;

    wxWindowUpdateLocker dummy( this );

    m_OptionsToolBar = new EDA_TOOLBAR( TOOLBAR_OPTION, this, ID_OPT_TOOLBAR, false );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_DRC_OFF, wxEmptyString, KiBitmap( drc_off_xpm ),
                               _( "Enable design rule checking" ), wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString, KiBitmap( grid_xpm ),
                               _( "Hide grid" ), wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLAR_COORD, wxEmptyString,
                               KiBitmap( polar_coord_xpm ),
                               _( "Display polar coordinates" ), wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               KiBitmap( unit_inch_xpm ),
                               _( "Units in inches" ), wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               KiBitmap( unit_mm_xpm ),
                               _( "Units in millimeters" ), wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               KiBitmap( cursor_shape_xpm ),
                               _( "Change cursor shape" ), wxITEM_CHECK );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_RATSNEST, wxEmptyString,
                               KiBitmap( general_ratsnest_xpm ),
                               _( "Show board ratsnest" ), wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MODULE_RATSNEST, wxEmptyString,
                               KiBitmap( local_ratsnest_xpm ),
                               _( "Show module ratsnest when moving" ),
                               wxITEM_CHECK );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_AUTO_DEL_TRACK, wxEmptyString,
                               KiBitmap( auto_delete_track_xpm ),
                               _( "Enable automatic track deletion" ),
                               wxITEM_CHECK );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddRadioTool( ID_TB_OPTIONS_SHOW_ZONES, wxEmptyString,
                                    KiBitmap( show_zone_xpm ), wxNullBitmap,
                                    _( "Show filled areas in zones" ) );
    m_OptionsToolBar->AddRadioTool( ID_TB_OPTIONS_SHOW_ZONES_DISABLE, wxEmptyString,
                                    KiBitmap( show_zone_disable_xpm ),
                                    wxNullBitmap, _( "Do not show filled areas in zones" ));
    m_OptionsToolBar->AddRadioTool( ID_TB_OPTIONS_SHOW_ZONES_OUTLINES_ONLY, wxEmptyString,
                                    KiBitmap( show_zone_outline_only_xpm ), wxNullBitmap,
                                    _( "Show outlines of filled areas only in zones" ) );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH, wxEmptyString,
                               KiBitmap( pad_sketch_xpm ),
                               _( "Show pads in outline mode" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_VIAS_SKETCH, wxEmptyString,
                               KiBitmap( via_sketch_xpm ),
                               _( "Show vias in outline mode" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_TRACKS_SKETCH, wxEmptyString,
                               KiBitmap( showtrack_xpm ),
                               _( "Show tracks in outline mode" ),
                               wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE, wxEmptyString,
                               KiBitmap( palette_xpm ),
                               _( "Enable high contrast display mode" ),
                               wxITEM_CHECK );

    // Tools to show/hide toolbars:
    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MANAGE_LAYERS_VERTICAL_TOOLBAR,
                               wxEmptyString,
                               KiBitmap( layers_manager_xpm ),
                               HELP_SHOW_HIDE_LAYERMANAGER,
                               wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_EXTRA_VERTICAL_TOOLBAR_MICROWAVE,
                               wxEmptyString,
                               KiBitmap( mw_toolbar_xpm ),
 _( "Show/hide the toolbar for microwaves tools\n This is a experimental feature (under development)" ),
                               wxITEM_CHECK );


    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->Realize();
}


/* Create the main vertical right toolbar, showing usual tools
 */
void PCB_EDIT_FRAME::ReCreateVToolbar()
{
    if( m_VToolBar )
        return;

    wxWindowUpdateLocker dummy( this );

    m_VToolBar = new EDA_TOOLBAR( TOOLBAR_TOOL, this, ID_V_TOOLBAR, false );

    // Set up toolbar
    m_VToolBar->AddTool( ID_NO_TOOL_SELECTED, wxEmptyString, KiBitmap( cursor_xpm ),
                         wxEmptyString, wxITEM_CHECK );
    m_VToolBar->AddSeparator();

    m_VToolBar->AddTool( ID_PCB_HIGHLIGHT_BUTT, wxEmptyString, KiBitmap( net_highlight_xpm ),
                         _( "Highlight net" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_SHOW_1_RATSNEST_BUTT, wxEmptyString, KiBitmap( tool_ratsnest_xpm ),
                         _( "Display local ratsnest" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_PCB_MODULE_BUTT, wxEmptyString, KiBitmap( module_xpm ),
                         _( "Add modules" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_TRACK_BUTT, wxEmptyString, KiBitmap( add_tracks_xpm ),
                         _( "Add tracks and vias" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_ZONES_BUTT, wxEmptyString, KiBitmap( add_zone_xpm ),
                         _( "Add filled zones" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_PCB_ADD_LINE_BUTT, wxEmptyString, KiBitmap( add_dashed_line_xpm ),
                         _( "Add graphic line or polygon" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_CIRCLE_BUTT, wxEmptyString, KiBitmap( add_circle_xpm ),
                         _( "Add graphic circle" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_ARC_BUTT, wxEmptyString, KiBitmap( add_arc_xpm ),
                         _( "Add graphic arc" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_ADD_TEXT_BUTT, wxEmptyString, KiBitmap( add_text_xpm ),
                         _( "Add text on copper layers or graphic text" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_PCB_DIMENSION_BUTT, wxEmptyString, KiBitmap( add_dimension_xpm ),
                         _( "Add dimension" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_MIRE_BUTT, wxEmptyString, KiBitmap( add_mires_xpm ),
                         _( "Add layer alignment target" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_PCB_DELETE_ITEM_BUTT, wxEmptyString, KiBitmap( delete_body_xpm ),
                         _( "Delete items" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_PCB_PLACE_OFFSET_COORD_BUTT, wxEmptyString, KiBitmap( pcb_offset_xpm ),
                         _( "Place the origin point for drill and place files" ),
                         wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_PLACE_GRID_COORD_BUTT, wxEmptyString,
                         KiBitmap( grid_select_axis_xpm ),
                         _( "Set the origin point for the grid" ),
                         wxITEM_CHECK );

    m_VToolBar->Realize();
}


/* Create the auxiliary vertical right toolbar, showing tools for microwave applications
 */
void PCB_EDIT_FRAME::ReCreateMicrowaveVToolbar()
{
    if( m_AuxVToolBar )
        return;

    wxWindowUpdateLocker dummy(this);

    m_AuxVToolBar = new EDA_TOOLBAR( TOOLBAR_TOOL, this, ID_MICROWAVE_V_TOOLBAR, false );

    // Set up toolbar
    m_AuxVToolBar->AddTool( ID_PCB_MUWAVE_TOOL_SELF_CMD, wxEmptyString,
                            KiBitmap( mw_add_line_xpm ),
                            _( "Create line of specified length for microwave applications" ) );

    m_AuxVToolBar->AddTool( ID_PCB_MUWAVE_TOOL_GAP_CMD, wxEmptyString,
                            KiBitmap( mw_add_gap_xpm ),
                            _( "Create gap of specified length for microwave applications" ) );

    m_AuxVToolBar->AddSeparator();

    m_AuxVToolBar->AddTool( ID_PCB_MUWAVE_TOOL_STUB_CMD, wxEmptyString,
                            KiBitmap( mw_add_stub_xpm ),
                            _( "Create stub of specified length for microwave applications" ) );

    m_AuxVToolBar->AddTool( ID_PCB_MUWAVE_TOOL_STUB_ARC_CMD, wxEmptyString,
                            KiBitmap( mw_add_stub_arc_xpm ),
                            _( "Create stub (arc) of specified length for microwave applications" )
                            );

    m_AuxVToolBar->AddTool( ID_PCB_MUWAVE_TOOL_FUNCTION_SHAPE_CMD, wxEmptyString,
                            KiBitmap( mw_add_shape_xpm ),
                            _( "Create a polynomial shape for microwave applications" ) );

    m_AuxVToolBar->Realize();
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

    if( m_AuxiliaryToolBar )
        return;

    m_AuxiliaryToolBar = new EDA_TOOLBAR( TOOLBAR_AUX, this, ID_AUX_TOOLBAR, true );

    /* Set up toolbar items */

    // Creates box to display and choose tracks widths:
    m_SelTrackWidthBox = new wxComboBox( m_AuxiliaryToolBar,
                                         ID_AUX_TOOLBAR_PCB_TRACK_WIDTH,
                                         wxEmptyString,
                                         wxPoint( -1, -1 ),
                                         wxSize( LISTBOX_WIDTH, -1 ),
                                         0, NULL, wxCB_READONLY );
    m_AuxiliaryToolBar->AddControl( m_SelTrackWidthBox );
    m_AuxiliaryToolBar->AddSeparator();

    // Creates box to display and choose vias diameters:
    m_SelViaSizeBox = new wxComboBox( m_AuxiliaryToolBar,
                                      ID_AUX_TOOLBAR_PCB_VIA_SIZE,
                                      wxEmptyString,
                                      wxPoint( -1, -1 ),
                                      wxSize( (LISTBOX_WIDTH*12)/10, -1 ),
                                      0, NULL, wxCB_READONLY );
    m_AuxiliaryToolBar->AddControl( m_SelViaSizeBox );
    m_AuxiliaryToolBar->AddSeparator();

    // Creates box to display and choose strategy to handle tracks an vias sizes:
    m_AuxiliaryToolBar->AddTool( ID_AUX_TOOLBAR_PCB_SELECT_AUTO_WIDTH,
                                 wxEmptyString,
                                 KiBitmap( auto_track_width_xpm ),
                                 _( "Auto track width: when starting on \
an existing track use its width\notherwise, use current width setting" ),
                                 wxITEM_CHECK );

    // Add the box to display and select the current grid size:
    m_AuxiliaryToolBar->AddSeparator();
    m_SelGridBox = new wxComboBox( m_AuxiliaryToolBar,
                                   ID_ON_GRID_SELECT,
                                   wxEmptyString,
                                   wxPoint( -1, -1 ),
                                   wxSize( LISTBOX_WIDTH, -1 ),
                                   0, NULL, wxCB_READONLY );
    m_AuxiliaryToolBar->AddControl( m_SelGridBox );

    //  Add the box to display and select the current Zoom
    m_AuxiliaryToolBar->AddSeparator();
    m_SelZoomBox = new wxComboBox( m_AuxiliaryToolBar,
                                   ID_ON_ZOOM_SELECT,
                                   wxEmptyString,
                                   wxPoint( -1, -1 ),
                                   wxSize( LISTBOX_WIDTH, -1 ),
                                   0, NULL, wxCB_READONLY );
    m_AuxiliaryToolBar->AddControl( m_SelZoomBox );

    updateZoomSelectBox();
    updateGridSelectBox();
    updateTraceWidthSelectBox();
    updateViaSizeSelectBox();

    // after adding the buttons to the toolbar, must call Realize()
    m_AuxiliaryToolBar->Realize();
    m_AuxiliaryToolBar->AddSeparator();
}


void PCB_EDIT_FRAME::updateTraceWidthSelectBox()
{
    if( m_SelTrackWidthBox == NULL )
        return;

    wxString msg;

    m_SelTrackWidthBox->Clear();

    for( unsigned ii = 0; ii < GetBoard()->m_TrackWidthList.size(); ii++ )
    {
        msg = _( "Track " ) + CoordinateToString( GetBoard()->m_TrackWidthList[ii], true );

        if( ii == 0 )
            msg << _( " *" );

        m_SelTrackWidthBox->Append( msg );
    }

    if( GetBoard()->m_TrackWidthSelector >= GetBoard()->m_TrackWidthList.size() )
        GetBoard()->m_TrackWidthSelector = 0;
}


void PCB_EDIT_FRAME::updateViaSizeSelectBox()
{
    if( m_SelViaSizeBox == NULL )
        return;

    wxString msg;

    m_SelViaSizeBox->Clear();

    for( unsigned ii = 0; ii < GetBoard()->m_ViasDimensionsList.size(); ii++ )
    {
        msg = _( "Via " );
        msg << CoordinateToString( GetBoard()->m_ViasDimensionsList[ii].m_Diameter, true );

        if( GetBoard()->m_ViasDimensionsList[ii].m_Drill )
            msg  << wxT("/ ")
                 << CoordinateToString( GetBoard()->m_ViasDimensionsList[ii].m_Drill, true );

        if( ii == 0 )
            msg << _( " *" );

        m_SelViaSizeBox->Append( msg );
    }

    if( GetBoard()->m_ViaSizeSelector >= GetBoard()->m_ViasDimensionsList.size() )
        GetBoard()->m_ViaSizeSelector = 0;
}


LAYER_BOX_SELECTOR* PCB_EDIT_FRAME::ReCreateLayerBox( EDA_TOOLBAR* parent )
{
    if( m_SelLayerBox == NULL )
        return NULL;

    m_SelLayerBox->m_hotkeys = g_Board_Editor_Hokeys_Descr;
    m_SelLayerBox->Resync();
    m_SelLayerBox->SetToolTip( _( "+/- to switch" ) );

    return m_SelLayerBox;
}
