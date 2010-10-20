/***************************************/
/*  tool_pcb.cpp: PCB editor tool bars */
/***************************************/

#include "fctsys.h"
#include "wx/wupdlock.h"

#include "common.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "class_board_design_settings.h"
#include "colors_selection.h"

#include "bitmaps.h"

#include "pcbnew_id.h"

#ifdef __UNIX__
#define LISTBOX_WIDTH 150
#else
#define LISTBOX_WIDTH 130
#endif

#include  "wx/ownerdrw.h"
#include  "wx/menuitem.h"

#include "hotkeys.h"

#include "help_common_strings.h"

#define SEL_LAYER_HELP _( \
        "Show active layer selections\nand select layer pair for route and place via" )

/* Data to build the layer pair indicator button */
static wxBitmap*  LayerPairBitmap = NULL;

static const char s_BitmapLayerIcon[16][16] = {
    // 0 = draw pixel with active layer color
    // 1 = draw pixel with top layer color (top/bottom layer used in
    //     autoroute and place via)
    // 2 = draw pixel with bottom layer color
    // 3 = draw pixel with via color
    { 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 3, 3, 0, 1, 1, 3, 3, 0, 0, 0, 0 },
    { 2, 2, 2, 2, 3, 3, 0, 1, 1, 1, 1, 3, 3, 2, 2, 2 },
    { 2, 2, 2, 2, 3, 3, 1, 1, 1, 0, 0, 3, 3, 2, 2, 2 },
    { 2, 2, 2, 2, 3, 3, 1, 1, 1, 1, 0, 3, 3, 2, 2, 2 },
    { 0, 0, 0, 0, 0, 3, 3, 1, 1, 0, 3, 3, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 1, 3, 3, 3, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 }
};


/* Draw the icon for the "Select layer pair" bitmap tool
 */
void WinEDA_PcbFrame::PrepareLayerIndicator()
{
    int        ii, jj;
    int        active_layer_color, Route_Layer_TOP_color,
               Route_Layer_BOTTOM_color, via_color;
    bool       change = false;

    static int previous_active_layer_color, previous_Route_Layer_TOP_color,
               previous_Route_Layer_BOTTOM_color, previous_via_color;

    /* get colors, and redraw bitmap button only on changes */
    active_layer_color = GetBoard()->GetLayerColor(getActiveLayer());
    if( previous_active_layer_color != active_layer_color )
    {
        previous_active_layer_color = active_layer_color;
        change = true;
    }
    Route_Layer_TOP_color = g_ColorsSettings.GetLayerColor(((PCB_SCREEN*)GetScreen())->m_Route_Layer_TOP);
    if( previous_Route_Layer_TOP_color != Route_Layer_TOP_color )
    {
        previous_Route_Layer_TOP_color = Route_Layer_TOP_color;
        change = true;
    }
    Route_Layer_BOTTOM_color = g_ColorsSettings.GetLayerColor(((PCB_SCREEN*)GetScreen())->m_Route_Layer_BOTTOM);
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

    /* Create the bitmap too and its Memory DC, if not already made */
    if( LayerPairBitmap == NULL )
    {
        LayerPairBitmap = new wxBitmap( 16, 16 );
    }

    /* Draw the icon, with colors according to the active layer and layer
     * pairs for via command (change layer)
     */
    wxMemoryDC iconDC;
    iconDC.SelectObject( *LayerPairBitmap );
    int        buttcolor = -1;
    wxPen      pen;
    for( ii = 0; ii < 16; ii++ )
    {
        for( jj = 0; jj < 16; jj++ )
        {
            if( s_BitmapLayerIcon[ii][jj] != buttcolor )
            {
                buttcolor = s_BitmapLayerIcon[ii][jj];
                int color;

                switch( buttcolor )
                {
                default:
                case 0:
                    color = active_layer_color;
                    break;

                case 1:
                    color = Route_Layer_TOP_color;
                    break;

                case 2:
                    color = Route_Layer_BOTTOM_color;
                    break;

                case 3:
                    color = via_color;
                    break;
                }

                color &= MASKCOLOR;
                pen.SetColour(
                    ColorRefs[color].m_Red,
                    ColorRefs[color].m_Green,
                    ColorRefs[color].m_Blue
                    );
                iconDC.SetPen( pen );
            }
            iconDC.DrawPoint( jj, ii );
        }
    }

    /* Deselect the Tool Bitmap from DC,
     *  in order to delete the MemoryDC safely without deleting the bitmap */
    iconDC.SelectObject( wxNullBitmap );

    if( m_HToolBar )
    {
        m_HToolBar->SetToolBitmap( ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR,
                                         *LayerPairBitmap );
        m_HToolBar->Realize();
    }
}


/* Creates or updates the main horizontal toolbar for the board editor
*/
void WinEDA_PcbFrame::ReCreateHToolbar()
{
    wxString msg;

    if( m_HToolBar != NULL )
    {
        SetToolbars();
        return;
    }

    wxWindowUpdateLocker dummy(this);

    m_HToolBar = new WinEDA_Toolbar( TOOLBAR_MAIN, this, ID_H_TOOLBAR, true );
    m_HToolBar->SetRows( 1 );

    // Set up toolbar
    m_HToolBar->AddTool( ID_NEW_BOARD, wxEmptyString, wxBitmap( new_xpm ),
                         _( "New board" ) );
    m_HToolBar->AddTool( ID_LOAD_FILE, wxEmptyString, wxBitmap( open_xpm ),
                         _( "Open existing board" ) );
    m_HToolBar->AddTool( ID_SAVE_BOARD, wxEmptyString, wxBitmap( save_xpm ),
                         _( "Save board" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_SHEET_SET, wxEmptyString, wxBitmap( sheetset_xpm ),
                         _( "Page settings (size, texts)" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_OPEN_MODULE_EDITOR, wxEmptyString,
                         wxBitmap( modedit_xpm ),
                         _( "Open module editor" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( wxID_CUT, wxEmptyString, wxBitmap( cut_button ),
                         _( "Cut selected item" ) );

#if 0
    m_HToolBar->AddTool( wxID_COPY, wxEmptyString, wxBitmap( copy_button ),
                         _( "Copy selected item" ) );

    m_HToolBar->AddTool( wxID_PASTE, wxEmptyString, wxBitmap( paste_xpm ),
                         _( "Paste" ) );
#endif

    m_HToolBar->AddSeparator();
    msg = AddHotkeyName( HELP_UNDO, s_Board_Editor_Hokeys_Descr,
                         HK_UNDO, false );
    m_HToolBar->AddTool( wxID_UNDO, wxEmptyString, wxBitmap( undo_xpm ),
                         HELP_UNDO );
    msg = AddHotkeyName( HELP_REDO, s_Board_Editor_Hokeys_Descr,
                         HK_REDO, false );
    m_HToolBar->AddTool( wxID_REDO, wxEmptyString, wxBitmap( redo_xpm ),
                         HELP_REDO );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( wxID_PRINT, wxEmptyString, wxBitmap( print_button ),
                         _( "Print board" ) );
    m_HToolBar->AddTool( ID_GEN_PLOT, wxEmptyString, wxBitmap( plot_xpm ),
                         _( "Plot (HPGL, PostScript, or GERBER format)" ) );

    m_HToolBar->AddSeparator();
    msg = AddHotkeyName( HELP_ZOOM_IN, s_Board_Editor_Hokeys_Descr,
                         HK_ZOOM_IN, false );
    m_HToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, wxBitmap( zoom_in_xpm ),
                         msg );

    msg = AddHotkeyName( HELP_ZOOM_OUT, s_Board_Editor_Hokeys_Descr,
                         HK_ZOOM_OUT, false );
    m_HToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString,
                         wxBitmap( zoom_out_xpm ), msg );

    msg = AddHotkeyName( HELP_ZOOM_REDRAW, s_Board_Editor_Hokeys_Descr,
                         HK_ZOOM_REDRAW, false );
    m_HToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                         wxBitmap( zoom_redraw_xpm ), msg );

    msg = AddHotkeyName( HELP_ZOOM_FIT, s_Board_Editor_Hokeys_Descr,
                         HK_ZOOM_AUTO, false );
    m_HToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString,
                         wxBitmap( zoom_auto_xpm ), msg );

    m_HToolBar->AddSeparator();
    msg = AddHotkeyName( HELP_FIND, // Find components and texts
                         s_Board_Editor_Hokeys_Descr,
                         HK_FIND_ITEM, false );
    m_HToolBar->AddTool( ID_FIND_ITEMS, wxEmptyString, wxBitmap( find_xpm ),
                         msg );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_GET_NETLIST, wxEmptyString, wxBitmap( netlist_xpm ),
                         _( "Read netlist" ) );
    m_HToolBar->AddTool( ID_DRC_CONTROL, wxEmptyString, wxBitmap( erc_xpm ),
                         _( "Perform design rules check" ) );

    m_HToolBar->AddSeparator();

    ReCreateLayerBox( m_HToolBar );
    PrepareLayerIndicator();    // Initialize the bitmap with current
                                // active layer colors for the next tool
    m_HToolBar->AddTool( ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR, wxEmptyString,
                         *LayerPairBitmap, SEL_LAYER_HELP );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_TOOLBARH_PCB_MODE_MODULE, wxEmptyString,
                         wxBitmap( mode_module_xpm ),
                         _( "Mode footprint: manual and automatic move and place modules" ),
                         wxITEM_CHECK );
    m_HToolBar->AddTool( ID_TOOLBARH_PCB_MODE_TRACKS, wxEmptyString,
                         wxBitmap( mode_track_xpm ),
                         _( "Mode track: autorouting" ), wxITEM_CHECK );

    // Fast call to FreeROUTE Web Bases router
    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_TOOLBARH_PCB_FREEROUTE_ACCESS, wxEmptyString,
                         wxBitmap( web_support_xpm ),
                         _( "Fast access to the Web Based FreeROUTE advanced router" ) );

    m_HToolBar->AddSeparator();
    // after adding the buttons to the toolbar, must call Realize() to reflect
    // the changes

    m_HToolBar->Realize();
}


void WinEDA_PcbFrame::ReCreateOptToolbar()
{
    if( m_OptionsToolBar )
        return;

    wxWindowUpdateLocker dummy(this);

    m_OptionsToolBar = new WinEDA_Toolbar( TOOLBAR_OPTION, this,
                                           ID_OPT_TOOLBAR, FALSE );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_DRC_OFF, wxEmptyString,
                               wxBitmap( drc_off_xpm ),
                               _( "Enable design rule checking" ),
                               wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString,
                               wxBitmap( grid_xpm ),
                               _( "Hide grid" ), wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLAR_COORD, wxEmptyString,
                               wxBitmap( polar_coord_xpm ),
                               _( "Display polar coordinates" ), wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               wxBitmap( unit_inch_xpm ),
                               _( "Units in inches" ), wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               wxBitmap( unit_mm_xpm ),
                               _( "Units in millimeters" ), wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               wxBitmap( cursor_shape_xpm ),
                               _( "Change cursor shape" ), wxITEM_CHECK );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_RATSNEST, wxEmptyString,
                               wxBitmap( general_ratsnet_xpm ),
                               _( "Show board ratsnest" ), wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MODULE_RATSNEST, wxEmptyString,
                               wxBitmap( local_ratsnet_xpm ),
                               _( "Show module ratsnest when moving" ),
                               wxITEM_CHECK );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_AUTO_DEL_TRACK, wxEmptyString,
                               wxBitmap( auto_delete_track_xpm ),
                               _( "Enable automatic track deletion" ),
                               wxITEM_CHECK );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddRadioTool( ID_TB_OPTIONS_SHOW_ZONES, wxEmptyString,
                                    wxBitmap( show_zone_xpm ), wxNullBitmap,
                                    _( "Show filled areas in zones" ) );
    m_OptionsToolBar->AddRadioTool( ID_TB_OPTIONS_SHOW_ZONES_DISABLE,
                                    wxEmptyString,
                                    wxBitmap( show_zone_disable_xpm ),
                                    wxNullBitmap,
                                    _( "Do not show filled areas in zones" ));
    m_OptionsToolBar->AddRadioTool( ID_TB_OPTIONS_SHOW_ZONES_OUTLINES_ONLY,
                                    wxEmptyString,
                                    wxBitmap( show_zone_outline_only_xpm ),
                                    wxNullBitmap,
                                    _( "Show outlines of filled areas only in zones" ) );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH, wxEmptyString,
                               wxBitmap( pad_sketch_xpm ),
                               _( "Show pads in outline mode" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_VIAS_SKETCH, wxEmptyString,
                               wxBitmap( via_sketch_xpm ),
                               _( "Show vias in outline mode" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_TRACKS_SKETCH, wxEmptyString,
                               wxBitmap( showtrack_xpm ),
                               _( "Show tracks in outline mode" ),
                               wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE,
                               wxEmptyString,
                               wxBitmap( palette_xpm ),
                               _( "Enable high contrast display mode" ),
                               wxITEM_CHECK );
    m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE,
                                  DisplayOpt.ContrastModeDisplay );

    // Tools to show/hide toolbars:
    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MANAGE_LAYERS_VERTICAL_TOOLBAR,
                                    wxEmptyString,
                                    wxBitmap( layers_manager_xpm ),
                                    HELP_SHOW_HIDE_LAYERMANAGER,
                                    wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_EXTRA_VERTICAL_TOOLBAR1,
                               wxEmptyString,
                               wxBitmap( mw_toolbar_xpm ),
 _( "Show/hide the toolbar for microwaves tools\n This is a experimental feature (under development)" ),
                               wxITEM_CHECK );


    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->Realize();
}


/* Create the main vertical right toolbar, showing usual tools
 */
void WinEDA_PcbFrame::ReCreateVToolbar()
{
    if( m_VToolBar )
        return;

    wxWindowUpdateLocker dummy(this);

    m_VToolBar = new WinEDA_Toolbar( TOOLBAR_TOOL, this, ID_V_TOOLBAR, FALSE );

    // Set up toolbar
    m_VToolBar->AddTool( ID_NO_SELECT_BUTT, wxEmptyString,
                         wxBitmap( cursor_xpm ), wxEmptyString, wxITEM_CHECK );
    m_VToolBar->ToggleTool( ID_NO_SELECT_BUTT, true );
    m_VToolBar->AddSeparator();

    m_VToolBar->AddTool( ID_PCB_HIGHLIGHT_BUTT, wxEmptyString,
                         wxBitmap( net_highlight_xpm ), _( "Highlight net" ),
                         wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_SHOW_1_RATSNEST_BUTT, wxEmptyString,
                         wxBitmap( tool_ratsnet_xpm ),
                         _( "Display local ratsnest" ),
                         wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_COMPONENT_BUTT, wxEmptyString,
                         wxBitmap( module_xpm ),
                         _( "Add modules" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_TRACK_BUTT, wxEmptyString,
                         wxBitmap( add_tracks_xpm ),
                         _( "Add tracks and vias" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_ZONES_BUTT, wxEmptyString,
                         wxBitmap( add_zone_xpm ),
                         _( "Add zones" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_PCB_ADD_LINE_BUTT, wxEmptyString,
                         wxBitmap( add_dashed_line_xpm ),
                         _( "Add graphic line or polygon" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_CIRCLE_BUTT, wxEmptyString,
                         wxBitmap( add_circle_xpm ),
                         _( "Add graphic circle" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_ARC_BUTT, wxEmptyString,
                         wxBitmap( add_arc_xpm ),
                         _( "Add graphic arc" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_ADD_TEXT_BUTT, wxEmptyString,
                         wxBitmap( add_text_xpm ),
                         _( "Add text" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_PCB_DIMENSION_BUTT, wxEmptyString,
                         wxBitmap( add_dimension_xpm ),
                         _( "Add dimension" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_MIRE_BUTT, wxEmptyString,
                         wxBitmap( add_mires_xpm ),
                         _( "Add layer alignment target" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_PCB_DELETE_ITEM_BUTT, wxEmptyString,
                         wxBitmap( delete_body_xpm ),
                         _( "Delete items" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_PCB_PLACE_OFFSET_COORD_BUTT, wxEmptyString,
                         wxBitmap( pcb_offset_xpm ),
                         _( "Offset adjust for drill and place files" ),
                         wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_PLACE_GRID_COORD_BUTT, wxEmptyString,
                         wxBitmap( grid_select_axis_xpm ),
                         _( "Set the origin point for the grid" ),
                         wxITEM_CHECK );

    m_VToolBar->Realize();
}


/* Create the auxiliary vertical right toolbar, showing tools for
 * microwave applications
 */
void WinEDA_PcbFrame::ReCreateMicrowaveVToolbar()
{
    if( m_AuxVToolBar )
        return;

    wxWindowUpdateLocker dummy(this);

    m_AuxVToolBar = new WinEDA_Toolbar( TOOLBAR_TOOL, this,
                                        ID_MICROWAVE_V_TOOLBAR, FALSE );

    // Set up toolbar
    m_AuxVToolBar->AddTool( ID_PCB_MUWAVE_TOOL_SELF_CMD, wxEmptyString,
                            wxBitmap( mw_Add_Line_xpm ),
                            _( "Create line of specified length for microwave applications" ) );

    m_AuxVToolBar->AddTool( ID_PCB_MUWAVE_TOOL_GAP_CMD, wxEmptyString,
                            wxBitmap( mw_Add_Gap_xpm ),
                            _( "Create gap of specified length for microwave applications" ) );

    m_AuxVToolBar->AddSeparator();

    m_AuxVToolBar->AddTool( ID_PCB_MUWAVE_TOOL_STUB_CMD, wxEmptyString,
                            wxBitmap( mw_Add_Stub_xpm ),
                            _( "Create stub of specified length for microwave applications" ) );

    m_AuxVToolBar->AddTool( ID_PCB_MUWAVE_TOOL_STUB_ARC_CMD, wxEmptyString,
                            wxBitmap( mw_Add_stub_arc_xpm ),
                            _( "Create stub (arc) of specified length for microwave applications" )
                            );

    m_AuxVToolBar->AddTool( ID_PCB_MUWAVE_TOOL_FUNCTION_SHAPE_CMD, wxEmptyString,
                            wxBitmap( mw_Add_Shape_xpm ),
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
void WinEDA_PcbFrame::ReCreateAuxiliaryToolbar()
{
    size_t   i;
    wxString msg;

    wxWindowUpdateLocker dummy(this);

    if( m_AuxiliaryToolBar == NULL )
    {
        m_AuxiliaryToolBar = new WinEDA_Toolbar( TOOLBAR_AUX, this,
                                                 ID_AUX_TOOLBAR, true );

        m_TrackAndViasSizesList_Changed = true;

        /* Set up toolbar items */

        // Creates box to display and choose tracks widths:
        m_SelTrackWidthBox = new WinEDAChoiceBox( m_AuxiliaryToolBar,
                                                  ID_AUX_TOOLBAR_PCB_TRACK_WIDTH,
                                                  wxPoint( -1, -1 ),
                                                  wxSize( LISTBOX_WIDTH, -1 ) );
        m_AuxiliaryToolBar->AddControl( m_SelTrackWidthBox );
        m_AuxiliaryToolBar->AddSeparator();

        // Creates box to display and choose vias diameters:
        m_SelViaSizeBox = new WinEDAChoiceBox( m_AuxiliaryToolBar,
                                               ID_AUX_TOOLBAR_PCB_VIA_SIZE,
                                               wxPoint( -1, -1 ),
                                               wxSize( (LISTBOX_WIDTH*12)/10, -1 ) );
        m_AuxiliaryToolBar->AddControl( m_SelViaSizeBox );
        m_AuxiliaryToolBar->AddSeparator();

        // Creates box to display tracks and vias clearance:
        m_ClearanceBox = new wxTextCtrl( m_AuxiliaryToolBar, -1,
                                         wxEmptyString, wxPoint( -1, -1 ),
                                         wxSize( LISTBOX_WIDTH + 10, -1 ),
                                         wxTE_READONLY );
        m_ClearanceBox->SetToolTip(_("Current NetClass clearance value") );
        m_AuxiliaryToolBar->AddControl( m_ClearanceBox );
        m_AuxiliaryToolBar->AddSeparator();

        // Creates box to display the current NetClass:
        m_NetClassSelectedBox = new wxTextCtrl( m_AuxiliaryToolBar, -1,
                                                wxEmptyString, wxPoint( -1, -1 ),
                                                wxSize( LISTBOX_WIDTH, -1 ),
                                                wxTE_READONLY );
        m_NetClassSelectedBox->SetToolTip(_("Name of the current NetClass") );
        m_AuxiliaryToolBar->AddControl( m_NetClassSelectedBox );
        m_AuxiliaryToolBar->AddSeparator();

        // Creates box to display and choose strategy to handle tracks an
        // vias sizes:
        m_AuxiliaryToolBar->AddTool( ID_AUX_TOOLBAR_PCB_SELECT_AUTO_WIDTH,
                                     wxEmptyString,
                                     wxBitmap( auto_track_width_xpm ),
                                     _( "Auto track width: when starting on \
an existing track use its width\notherwise, use current width setting" ),
                                     wxITEM_CHECK );

        // Add the box to display and select the current grid size:
        m_AuxiliaryToolBar->AddSeparator();
        m_SelGridBox = new WinEDAChoiceBox( m_AuxiliaryToolBar,
                                            ID_ON_GRID_SELECT,
                                            wxPoint( -1, -1 ),
                                            wxSize( LISTBOX_WIDTH, -1 ) );
        m_AuxiliaryToolBar->AddControl( m_SelGridBox );

        //  Add the box to display and select the current Zoom
        m_AuxiliaryToolBar->AddSeparator();
        m_SelZoomBox = new WinEDAChoiceBox( m_AuxiliaryToolBar,
                                            ID_ON_ZOOM_SELECT,
                                            wxPoint( -1, -1 ),
                                            wxSize( LISTBOX_WIDTH, -1 ) );
        msg = _( "Auto" );
        m_SelZoomBox->Append( msg );
        for( int i = 0; i < (int)GetScreen()->m_ZoomList.GetCount(); i++ )
        {
            msg = _( "Zoom " );
            if ( (GetScreen()->m_ZoomList[i] % GetScreen()->m_ZoomScalar) == 0 )
                msg << GetScreen()->m_ZoomList[i] / GetScreen()->m_ZoomScalar;
            else
            {
                wxString value;
                value.Printf( wxT( "%.1f" ),
                              (float)GetScreen()->m_ZoomList[i] /
                              GetScreen()->m_ZoomScalar );
                msg += value;
            }
            m_SelZoomBox->Append( msg );
        }

        m_AuxiliaryToolBar->AddControl( m_SelZoomBox );

        // after adding the buttons to the toolbar, must call Realize()
        m_AuxiliaryToolBar->Realize();
    }

    // Update displayed values
    m_SelGridBox->Clear();
    wxString format = _( "Grid");
    switch( g_UserUnit )
    {
    case INCHES:
        format += wxT( " %.1f" );
        break;

    case MILLIMETRES:
        format += wxT( " %.3f" );
        break;

    case UNSCALED_UNITS:
        format += wxT( " %f" );
        break;
    }

    for( i = 0; i < GetScreen()->m_GridList.GetCount(); i++ )
    {
        GRID_TYPE grid = GetScreen()->m_GridList[i];
        double value = To_User_Unit( g_UserUnit, grid.m_Size.x,
                                     m_InternalUnits );
        if( grid.m_Id != ID_POPUP_GRID_USER )
        {
            switch( g_UserUnit )
            {
            case INCHES:
                msg.Printf( format.GetData(), value * 1000 );
                break;

            case MILLIMETRES:
            case UNSCALED_UNITS:
                msg.Printf( format.GetData(), value );
                break;
            }
        }
        else
            msg = _( "User Grid" );

        m_SelGridBox->Append( msg, (void*) &GetScreen()->m_GridList[i].m_Id );

        if( m_LastGridSizeId == GetScreen()->m_GridList[i].m_Id )
            m_SelGridBox->SetSelection( i );
    }

    m_TrackAndViasSizesList_Changed    = true;
    m_AuxiliaryToolBar->AddSeparator();
    ReCreateLayerBox( NULL );
}


void WinEDA_PcbFrame::syncLayerBox()
{
    wxASSERT( m_SelLayerBox );

    // Enable the display on the correct layer
    // To avoid reentrancy ( Bug wxGTK Linux version? ), the selection is
    // made only if it needs changing ( corrected on wxGTK 2.6.0 )
    int     count  = m_SelLayerBox->GetCount();
    int     choice = m_SelLayerBox->GetChoice();
    int     layer  = getActiveLayer();

    for( int listNdx=0;  listNdx<count;  ++listNdx )
    {
        int clientData = (int) (size_t) m_SelLayerBox->wxItemContainer::GetClientData( listNdx );

        if( clientData == layer )
        {
            if( listNdx != choice )
                m_SelLayerBox->SetSelection( listNdx );
            break;
        }
    }
}


WinEDAChoiceBox* WinEDA_PcbFrame::ReCreateLayerBox( WinEDA_Toolbar* parent )
{
    if( m_SelLayerBox == NULL )
    {
        if( parent == NULL )
            return NULL;

        m_SelLayerBox = new WinEDAChoiceBox( parent,
                                             ID_TOOLBARH_PCB_SELECT_LAYER,
                                             wxPoint( -1, -1 ),
#if defined (__UNIX__)

                                             // Width enough for the longest
                                             // string: "Component (Page Down)"
                                             // Maybe that string is too long?
                                             wxSize( 230, -1 )
#else
                                             wxSize( LISTBOX_WIDTH + 30, -1 )
#endif
                                             );

        parent->AddControl( m_SelLayerBox );
    }
    int      layer_mask = GetBoard()->GetEnabledLayers();
    unsigned length  = 0;

    m_SelLayerBox->Clear();

    static DECLARE_LAYERS_ORDER_LIST(layerOrder_for_display);

    for( int idx=0, listNdx=0;  idx <= EDGE_N;  idx++ )
    {
        int layer = layerOrder_for_display[idx];
        // List to append hotkeys in layer box selection
        static const int HK_SwitchLayer[EDGE_N + 1] = {
            HK_SWITCH_LAYER_TO_COPPER,
            HK_SWITCH_LAYER_TO_INNER1,
            HK_SWITCH_LAYER_TO_INNER2,
            HK_SWITCH_LAYER_TO_INNER3,
            HK_SWITCH_LAYER_TO_INNER4,
            HK_SWITCH_LAYER_TO_INNER5,
            HK_SWITCH_LAYER_TO_INNER6,
            HK_SWITCH_LAYER_TO_INNER7,
            HK_SWITCH_LAYER_TO_INNER8,
            HK_SWITCH_LAYER_TO_INNER9,
            HK_SWITCH_LAYER_TO_INNER10,
            HK_SWITCH_LAYER_TO_INNER11,
            HK_SWITCH_LAYER_TO_INNER12,
            HK_SWITCH_LAYER_TO_INNER13,
            HK_SWITCH_LAYER_TO_INNER14,
            HK_SWITCH_LAYER_TO_COMPONENT
        };

        if( g_TabOneLayerMask[layer] & layer_mask )
        {
            wxString msg = GetBoard()->GetLayerName( layer );
            msg << wxT("  ");
            msg = AddHotkeyName( msg, s_Board_Editor_Hokeys_Descr,
                                 HK_SwitchLayer[layer], false );

            m_SelLayerBox->Append( msg );

            //D(printf("appending layername=%s, ndx=%d, layer=%d\n", CONV_TO_UTF8(msg), listNdx, layer );)

            m_SelLayerBox->wxItemContainer::SetClientData( listNdx, (void*) layer );
            length = MAX( length, msg.Len() );
            listNdx++;
        }
    }

    m_SelLayerBox->SetToolTip( _( "+/- to switch" ) );

    syncLayerBox();

    return m_SelLayerBox;
}
