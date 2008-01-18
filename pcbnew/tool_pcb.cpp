/*********************************************/
/*  tool_pcb.cpp: construction des tool bars */
/*********************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"

#include "bitmaps.h"

#include "id.h"

#define BITMAP wxBitmap

#ifdef __UNIX__
#define LISTBOX_WIDTH 140
#else
#define LISTBOX_WIDTH 120
#endif

#include  "wx/ownerdrw.h"
#include  "wx/menuitem.h"

#include "hotkeys.h"

#define MUWAVE_ENBL

#include "mw_Add_Stub.xpm"
#include "mw_Add_stub_arc.xpm"
#include "mw_Add_Shape.xpm"
#include "mw_Add_Line.xpm"
#include "mw_Add_Gap.xpm"
#include "mw_toolbar.xpm"
#include "Add_Tracks.xpm"
#include "Show_Zone.xpm"
#include "net_hightlight.xpm"
#include "PcbOffset.xpm"
#include "Add_Mires.xpm"
#include "Mode_Module.xpm"
#include "Mode_Track.xpm"
#include "tool_ratsnet.xpm"
#include "local_ratsnet.xpm"
#include "add_cotation.xpm"

#define SEL_LAYER_HELP _( \
        "Show active layer selections\nand select layer pair for route and place via" )

/* Data to build the layer pair indicator button */
static wxBitmap*  LayerPairBitmap = NULL;

static const char s_BitmapLayerIcon[16][16] = {
    // 0 = draw pixel with active layer color
    // 1 = draw pixel with top layer color (top/bottom layer used in autoroute and place via)
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


/************************************************************/
void WinEDA_PcbFrame::PrepareLayerIndicator()
/************************************************************/

/* Draw the icon for the "Select layet pair" bitmap tool
 */
{
    int        ii, jj;
    int        active_layer_color, Route_Layer_TOP_color,
               Route_Layer_BOTTOM_color, via_color;
    bool       change = false;

    static int previous_active_layer_color, previous_Route_Layer_TOP_color,
               previous_Route_Layer_BOTTOM_color, previous_via_color;

    /* get colors, and redraw bitmap button only on changes */
    active_layer_color = g_DesignSettings.m_LayerColor[GetScreen()->m_Active_Layer];
    if( previous_active_layer_color != active_layer_color )
    {
        previous_active_layer_color = active_layer_color;
        change = TRUE;
    }
    Route_Layer_TOP_color = g_DesignSettings.m_LayerColor[GetScreen()->m_Route_Layer_TOP];
    if( previous_Route_Layer_TOP_color != Route_Layer_TOP_color )
    {
        previous_Route_Layer_TOP_color = Route_Layer_TOP_color;
        change = TRUE;
    }
    Route_Layer_BOTTOM_color = g_DesignSettings.m_LayerColor[GetScreen()->m_Route_Layer_BOTTOM];
    if( previous_Route_Layer_BOTTOM_color != Route_Layer_BOTTOM_color )
    {
        previous_Route_Layer_BOTTOM_color = Route_Layer_BOTTOM_color;
        change = TRUE;
    }
    via_color = g_DesignSettings.m_ViaColor[g_DesignSettings.m_CurrentViaType ];
    if( previous_via_color != via_color )
    {
        previous_via_color = via_color;
        change = TRUE;
    }

    if( !change && (LayerPairBitmap != NULL) )
        return;

    /* Creat the bitmap too and its Memory DC, if not already made */
    if( LayerPairBitmap == NULL )
    {
        LayerPairBitmap = new wxBitmap( 16, 16 );
    }

    /* Draw the icon, with colors according to the active layer and layer pairs for
     * via command (change layer)
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
#if wxCHECK_VERSION( 2, 8, 3 )
        m_HToolBar->SetToolNormalBitmap( ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR, *LayerPairBitmap );
#else
        int pos = m_HToolBar->GetToolPos( ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR );
        if( pos != wxNOT_FOUND )
        {
            m_HToolBar->DeleteTool( ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR );
            m_HToolBar->InsertTool( pos, ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR, *LayerPairBitmap,
                                    wxNullBitmap, false, NULL, SEL_LAYER_HELP );
            m_HToolBar->Realize();
        }
#endif
    }
}


/******************************************/
void WinEDA_PcbFrame::ReCreateHToolbar()
/******************************************/

/* Create the main horizontal toolbar for the board editor */
{
    int      ii;
    wxString msg;

    if( m_HToolBar != NULL )
    {           // simple mise a jour de la liste des fichiers anciens
        wxMenuItem* item;
        for( ii = 9; ii >=0; ii-- )
        {
            if( m_FilesMenu->FindItem( ID_LOAD_FILE_1 + ii ) )
            {
                item = m_FilesMenu->Remove( ID_LOAD_FILE_1 + ii );
                if( item )
                    delete item;
            }
        }

        for( ii = 0; ii < 10; ii++ )
        {
            if( GetLastProject( ii ).IsEmpty() )
                break;
            m_FilesMenu->Append( ID_LOAD_FILE_1 + ii, GetLastProject( ii ) );
        }

        SetToolbars();
        return;
    }


    m_HToolBar = new WinEDA_Toolbar( TOOLBAR_MAIN, this, ID_H_TOOLBAR, TRUE );
    m_HToolBar->SetRows( 1 );
    SetToolBar( m_HToolBar );

    // Set up toolbar
    m_HToolBar->AddTool( ID_NEW_BOARD, wxEmptyString, BITMAP( new_xpm ), _( "New Board" ) );
    m_HToolBar->AddTool( ID_LOAD_FILE, wxEmptyString, BITMAP( open_xpm ),
                        _( "Open existing Board" ) );
    m_HToolBar->AddTool( ID_SAVE_BOARD, wxEmptyString, BITMAP( save_xpm ), _( "Save Board" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_SHEET_SET, wxEmptyString, BITMAP( sheetset_xpm ),
                        _( "page settings (size, texts)" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_OPEN_MODULE_EDITOR, wxEmptyString, BITMAP( modedit_xpm ),
                        _( "Open Module Editor" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( wxID_CUT, wxEmptyString, BITMAP( cut_button ), _( "Cut selected item" ) );

#if 0
    m_HToolBar->AddTool( wxID_COPY, wxEmptyString, BITMAP( copy_button ),
                        _( "Copy selected item" ) );

    m_HToolBar->AddTool( wxID_PASTE, wxEmptyString, BITMAP( paste_xpm ), _( "Paste" ) );
#endif

    m_HToolBar->AddTool( ID_UNDO_BUTT, wxEmptyString, BITMAP( undelete_xpm ), _( "Undelete" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_GEN_PRINT, wxEmptyString, BITMAP( print_button ), _( "Print Board" ) );
    m_HToolBar->AddTool( ID_GEN_PLOT, wxEmptyString, BITMAP( plot_xpm ),
                        _( "Plot (HPGL, PostScript, or GERBER format)" ) );

    m_HToolBar->AddSeparator();
    msg = AddHotkeyName( _( "zoom +" ), s_Board_Editor_Hokeys_Descr, HK_ZOOM_IN );
    m_HToolBar->AddTool( ID_ZOOM_PLUS_BUTT, wxEmptyString, BITMAP( zoom_in_xpm ),
                         msg );

    msg = AddHotkeyName( _( "zoom -" ), s_Board_Editor_Hokeys_Descr, HK_ZOOM_OUT );
    m_HToolBar->AddTool( ID_ZOOM_MOINS_BUTT, wxEmptyString, BITMAP( zoom_out_xpm ),
                         msg );

    msg = AddHotkeyName( _( "redraw" ), s_Board_Editor_Hokeys_Descr, HK_ZOOM_REDRAW );
    m_HToolBar->AddTool( ID_ZOOM_REDRAW_BUTT, wxEmptyString, BITMAP( repaint_xpm ),
                         msg );

    m_HToolBar->AddTool( ID_ZOOM_PAGE_BUTT, wxEmptyString, BITMAP( zoom_optimal_xpm ),
                        _( "auto zoom" ) );

    m_HToolBar->AddSeparator();
    msg = AddHotkeyName( _( "Find components and texts" ), s_Board_Editor_Hokeys_Descr, HK_FIND_ITEM );
    m_HToolBar->AddTool( ID_FIND_ITEMS, wxEmptyString, BITMAP( find_xpm ),
                         msg );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_GET_NETLIST, wxEmptyString, BITMAP( netlist_xpm ),
                        _( "Read Netlist" ) );
    m_HToolBar->AddTool( ID_DRC_CONTROL, wxEmptyString, BITMAP( erc_xpm ),
                        _( "Pcb Design Rules Check" ) );

    m_HToolBar->AddSeparator();
    ReCreateLayerBox( m_HToolBar );
    PrepareLayerIndicator();    // Initialise the bitmap with current active layer colors for the next tool
    m_HToolBar->AddTool( ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR, wxEmptyString, *LayerPairBitmap,
                         SEL_LAYER_HELP );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_TOOLBARH_PCB_AUTOPLACE, wxEmptyString, BITMAP( mode_module_xpm ),
                         _(
                             "Mode Module: Manual and Automatic Move or Place for modules" ),
                         wxITEM_CHECK );
    m_HToolBar->AddTool( ID_TOOLBARH_PCB_AUTOROUTE, wxEmptyString, BITMAP( mode_track_xpm ),
                         _( "Mode Track and Autorouting" ), wxITEM_CHECK );

    // after adding the buttons to the toolbar, must call Realize() to reflect
    // the changes

    m_HToolBar->Realize();

    SetToolbars();
}


/*********************************************/
void WinEDA_PcbFrame::ReCreateOptToolbar()
/*********************************************/

// Create the left vertical toolbar (option selections)
{
    if( m_OptionsToolBar )
        return;

    // creation du tool bar options
    m_OptionsToolBar = new WinEDA_Toolbar( TOOLBAR_OPTION, this, ID_OPT_TOOLBAR, FALSE );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_DRC_OFF, wxEmptyString, BITMAP( drc_off_xpm ),
                               _( "Drc OFF" ), wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString, BITMAP( grid_xpm ),
                               _( "Display Grid OFF" ), wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLAR_COORD, wxEmptyString,
                               BITMAP( polar_coord_xpm ), _(
                                   "Display Polar Coord ON" ), wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               BITMAP( unit_inch_xpm ), _( "Units = Inch" ), wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               BITMAP( unit_mm_xpm ), _( "Units = mm" ), wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               BITMAP( cursor_shape_xpm ),
                               _( "Change Cursor Shape" ), wxITEM_CHECK );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_RATSNEST, wxEmptyString,
                               BITMAP( general_ratsnet_xpm ),
                               _( "Show General Ratsnest" ), wxITEM_CHECK );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MODULE_RATSNEST, wxEmptyString,
                               BITMAP( local_ratsnet_xpm ),
                               _( "Show Module Ratsnest when moving" ), wxITEM_CHECK );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_AUTO_DEL_TRACK, wxEmptyString,
                               BITMAP( auto_delete_track_xpm ),
                               _( "Enable Auto Del Track" ), wxITEM_CHECK );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_ZONES, wxEmptyString, BITMAP( show_zone_xpm ),
                               _( "Show Zones" ), wxITEM_CHECK );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH, wxEmptyString,
                               BITMAP( pad_sketch_xpm ),
                               _( "Show Pads Sketch" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_TRACKS_SKETCH, wxEmptyString,
                               BITMAP( showtrack_xpm ),
                               _( "Show Tracks Sketch" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_HIGHT_CONTRAST_MODE, wxEmptyString,
                               BITMAP( palette_xpm ),
                               _( "Hight Contrast Mode Display" ), wxITEM_CHECK );
    m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_HIGHT_CONTRAST_MODE,
                                  DisplayOpt.ContrastModeDisplay );

#ifdef MUWAVE_ENBL
    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_EXTRA_VERTICAL_TOOLBAR1, wxEmptyString,
                               BITMAP( mw_toolbar_xpm ),
                               _(
                                   "Display auxiliary vertical toolbar (tools for micro wave applications)\n This is a very experimental feature (under development)" ),
                               wxITEM_CHECK );
#endif

    m_OptionsToolBar->Realize();

    SetToolbars();
}


/********************************************/
void WinEDA_PcbFrame::ReCreateVToolbar()
/********************************************/

/* Create the main vertical right toolbar, showing usual tools
 */
{
    if( m_VToolBar )
        return;

    m_VToolBar = new WinEDA_Toolbar( TOOLBAR_TOOL, this, ID_V_TOOLBAR, FALSE );

    // Set up toolbar
    m_VToolBar->AddTool( ID_NO_SELECT_BUTT, wxEmptyString,
                         BITMAP( cursor_xpm ), wxEmptyString, wxITEM_CHECK );
    m_VToolBar->ToggleTool( ID_NO_SELECT_BUTT, TRUE );
    m_VToolBar->AddSeparator();

    m_VToolBar->AddTool( ID_PCB_HIGHLIGHT_BUTT, wxEmptyString,
                         BITMAP( net_hightlight_xpm ), _( "Net highlight" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_SHOW_1_RATSNEST_BUTT, wxEmptyString,
                         BITMAP( tool_ratsnet_xpm ),
                         _( "Display local ratsnest (pad or module)" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_COMPONENT_BUTT, wxEmptyString,
                         BITMAP( module_xpm ),
                         _( "Add modules" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_TRACK_BUTT, wxEmptyString,
                         BITMAP( add_tracks_xpm ),
                         _( "Add tracks and vias" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_ZONES_BUTT, wxEmptyString,
                         BITMAP( add_zone_xpm ),
                         _( "Add zones" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_LINE_COMMENT_BUTT, wxEmptyString,
                         BITMAP( add_dashed_line_xpm ),
                         _( "Add graphic line or polygon" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_CIRCLE_BUTT, wxEmptyString,
                         BITMAP( add_circle_xpm ),
                         _( "Add graphic circle" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_ARC_BUTT, wxEmptyString,
                         BITMAP( add_arc_xpm ),
                         _( "Add graphic arc" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_TEXT_COMMENT_BUTT, wxEmptyString,
                         BITMAP( add_text_xpm ),
                         _( "Add text" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_PCB_COTATION_BUTT, wxEmptyString,
                         BITMAP( add_cotation_xpm ),
                         _( "Add dimension" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_MIRE_BUTT, wxEmptyString,
                         BITMAP( add_mires_xpm ),
                         _( "Add layer alignment target" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_PCB_DELETE_ITEM_BUTT, wxEmptyString,
                         BITMAP( delete_body_xpm ),
                         _( "Delete items" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_PCB_PLACE_OFFSET_COORD_BUTT, wxEmptyString,
                         BITMAP( pcb_offset_xpm ),
                         _( "Offset adjust for drill and place files" ), wxITEM_CHECK );

    m_VToolBar->Realize();

    SetToolbars();
}


/*********************************************/
void WinEDA_PcbFrame::ReCreateAuxVToolbar()
/*********************************************/

/* Create the auxiliary vertical right toolbar, showing tools fo microwave applications
 */
{
    if( m_AuxVToolBar )
        return;

    m_AuxVToolBar = new WinEDA_Toolbar( TOOLBAR_TOOL, this, ID_AUX_V_TOOLBAR, FALSE );

    // Set up toolbar
    m_AuxVToolBar->AddTool( ID_PCB_MUWAVE_TOOL_SELF_CMD,
                           BITMAP( mw_Add_Line_xpm ),
                           wxNullBitmap, TRUE,
                           -1, -1, (wxObject*) NULL,
                           _( "Create line of specified length for microwave applications" ) );

    m_AuxVToolBar->AddTool( ID_PCB_MUWAVE_TOOL_GAP_CMD,
                           BITMAP( mw_Add_Gap_xpm ),
                           wxNullBitmap, TRUE,
                           -1, -1, (wxObject*) NULL,
                           _( "Create gap of specified length for microwave applications" ) );

    m_AuxVToolBar->AddSeparator();

    m_AuxVToolBar->AddTool( ID_PCB_MUWAVE_TOOL_STUB_CMD,
                           BITMAP( mw_Add_Stub_xpm ),
                           wxNullBitmap, TRUE,
                           -1, -1, (wxObject*) NULL,
                           _( "Create stub of specified length for microwave applications" ) );

    m_AuxVToolBar->AddTool( ID_PCB_MUWAVE_TOOL_STUB_ARC_CMD,
                            BITMAP( mw_Add_stub_arc_xpm ),
                            wxNullBitmap, TRUE,
                            -1, -1, (wxObject*) NULL,
                            _( "Create stub (arc) of specified length for microwave applications" )
                            );

    m_AuxVToolBar->AddTool( ID_PCB_MUWAVE_TOOL_FUNCTION_SHAPE_CMD,
                           BITMAP( mw_Add_Shape_xpm ),
                           wxNullBitmap, TRUE,
                           -1, -1, (wxObject*) NULL,
                           _( "Create a polynomial shape for microwave applications" ) );

    m_AuxVToolBar->Realize();

    SetToolbars();
}


/****************************************************/
void WinEDA_PcbFrame::ReCreateAuxiliaryToolbar()
/****************************************************/

/* Create auxiliary horizontal toolbar
 */
{
    int      ii;
    wxString msg;

    if( m_AuxiliaryToolBar == NULL )
    {
        m_AuxiliaryToolBar = new WinEDA_Toolbar( TOOLBAR_AUX, this, ID_AUX_TOOLBAR, TRUE );

        // Set up toolbar
        m_AuxiliaryToolBar->AddSeparator();
        m_SelTrackWidthBox = new WinEDAChoiceBox( m_AuxiliaryToolBar,
                                                 ID_AUX_TOOLBAR_PCB_TRACK_WIDTH,
                                                 wxPoint( -1,
                                                          -1 ), wxSize( LISTBOX_WIDTH + 20, -1 ) );
        m_AuxiliaryToolBar->AddControl( m_SelTrackWidthBox );
        m_SelTrackWidthBox_Changed = TRUE;

        m_AuxiliaryToolBar->AddSeparator();
        m_SelViaSizeBox = new WinEDAChoiceBox( m_AuxiliaryToolBar,
                                              ID_AUX_TOOLBAR_PCB_VIA_SIZE,
                                              wxPoint( -1, -1 ), wxSize( LISTBOX_WIDTH + 10, -1 ) );
        m_AuxiliaryToolBar->AddControl( m_SelViaSizeBox );

        m_AuxiliaryToolBar->AddSeparator();

        // Boite de selection du pas de grille
        m_SelGridBox = new WinEDAChoiceBox( m_AuxiliaryToolBar,
                                           ID_ON_GRID_SELECT,
                                           wxPoint( -1, -1 ), wxSize( LISTBOX_WIDTH, -1 ) );
        m_AuxiliaryToolBar->AddControl( m_SelGridBox );

        // Boite de selection du Zoom
        m_AuxiliaryToolBar->AddSeparator();
        m_SelZoomBox = new WinEDAChoiceBox( m_AuxiliaryToolBar,
                                           ID_ON_ZOOM_SELECT,
                                           wxPoint( -1, -1 ), wxSize( LISTBOX_WIDTH, -1 ) );
        msg = _( "Auto" );
        m_SelZoomBox->Append( msg );
        for( int jj = 0, ii = 1; ii <= m_ZoomMaxValue; ii <<= 1, jj++ )
        {
            msg = _( "Zoom " ); msg << ii;
            m_SelZoomBox->Append( msg );
        }

        m_SelZoomBox->Append( wxT( "" ) );

        m_AuxiliaryToolBar->AddControl( m_SelZoomBox );

        // after adding the buttons to the toolbar, must call Realize()
        m_AuxiliaryToolBar->Realize();
    }

    // mise a jour des affichages
    m_SelGridBox->Clear();
    wxString format = _( "Grid" );
    if( g_UnitMetric == INCHES )
        format += wxT( " %.1f" );
    else
        format += wxT( " %.3f" );

    for( ii = 0; g_GridList[ii].x > 0; ii++ )
    {
        double value = To_User_Unit( g_UnitMetric, g_GridList[ii].x, PCB_INTERNAL_UNIT );
        if( g_UnitMetric == INCHES )
            msg.Printf( format.GetData(), value * 1000 );
        else
            msg.Printf( format.GetData(), value );
        m_SelGridBox->Append( msg );
    }

    m_SelGridBox->Append( _( "User Grid" ) );

    m_SelViaSizeBox_Changed    = TRUE;
    m_SelTrackWidthBox_Changed = TRUE;

    SetToolbars();
}


/**************************************************************************/
WinEDAChoiceBox* WinEDA_PcbFrame::ReCreateLayerBox( WinEDA_Toolbar* parent )
/**************************************************************************/
{
    int  ii, jj, ll;
	unsigned lenght = 0;
    bool rebuild = FALSE;
    long current_mask_layer;

    if( m_SelLayerBox == NULL )
    {
        if( parent == NULL )
            return NULL;
        
        m_SelLayerBox = new WinEDAChoiceBox( parent, ID_TOOLBARH_PCB_SELECT_LAYER,
                                            
                                wxPoint( -1, -1 ),
#if defined(__UNIX__)                  
                                // Width enough for the longest string: "Component (Page Down)"
                                // Maybe that string is too long?
                                wxSize( 230, -1 )     
#else
                                wxSize( LISTBOX_WIDTH+40, -1 )
#endif
                              );
        
        parent->AddControl( m_SelLayerBox );
    }

    // Test si reconstruction de la liste necessaire
    current_mask_layer = 0;
    int Masque_Layer = g_TabAllCopperLayerMask[g_DesignSettings.m_CopperLayerCount - 1];
    Masque_Layer |= ALL_NO_CU_LAYERS;
    for( ii = 0; ii < (int) m_SelLayerBox->GetCount(); ii++ )
    {
        jj = (int) ( (size_t) m_SelLayerBox->GetClientData( ii ) );
        current_mask_layer |= g_TabOneLayerMask[jj];
    }

    if( current_mask_layer != Masque_Layer )
        rebuild = TRUE;

    // Construction de la liste
    if( rebuild )
    {
        m_SelLayerBox->Clear();
        for( ii = 0, jj = 0; ii <= EDGE_N; ii++ )
        {
        // List to append hotkeys in layer box selection
        static int HK_SwitchLayer[EDGE_N+1] = {
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

            if( (g_TabOneLayerMask[ii] & Masque_Layer) )
            {
                wxString msg = ReturnPcbLayerName( ii, false );
                msg = AddHotkeyName( msg, s_Board_Editor_Hokeys_Descr, HK_SwitchLayer[ii] );
                m_SelLayerBox->Append( msg );
                m_SelLayerBox->SetClientData( jj, (void*) ii );
				lenght = max(lenght, msg.Len() );
                jj++;
            }
        }
// Test me:
//		int lchar = m_SelLayerBox->GetFont().GetPointSize();
//		m_SelLayerBox->SetSize(wxSize(lenght * lchar,-1));

        m_SelLayerBox->SetToolTip( _( "+/- to switch" ) );
    }

    // Activation de l'affichage sur la bonne couche
    // Pour eviter la reentrance (Bug wxGTK version Linux?), la selection n'est faite que si
    // elle est mauvaise (Pb corrige sur wxGTK 2.6.0)
    jj = m_SelLayerBox->GetCount();
    ll = m_SelLayerBox->GetChoice();
    for( ii = 0; ii < jj; ii++ )
    {
        if( (int) ( (size_t) m_SelLayerBox->GetClientData( ii ) ) == GetScreen()->m_Active_Layer )
        {
            if( ii != ll )
                m_SelLayerBox->SetSelection( ii );
            break;
        }
    }

    return m_SelLayerBox;
}
