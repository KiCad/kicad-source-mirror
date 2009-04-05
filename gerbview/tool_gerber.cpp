/***************************************************/
/*	tool_gerber.cpp: Build tool bars and main menu */
/***************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "macros.h"
#include "gerbview.h"
#include "pcbplot.h"
#include "protos.h"
#include "bitmaps.h"
#include "id.h"
#include "hotkeys.h"

/***********************************************/
void WinEDA_GerberFrame::ReCreateMenuBar( void )
/***********************************************/

/* Cree ou reinitialise le menu du haut d'ecran
 */
{
    wxMenuBar  *menuBar = GetMenuBar();

    /* Destroy the existing menu bar so it can be rebuilt.  This allows
     * language changes of the menu text on the fly. */
    if( menuBar )
        SetMenuBar( NULL );

    menuBar = new wxMenuBar();

    wxMenu* filesMenu = new wxMenu;
    filesMenu->Append( ID_MENU_LOAD_FILE, _( "Clear and Load Gerber File" ),
                       _( "Clear all layers and Load new Gerber file" ),
                       FALSE );

    filesMenu->Append( ID_MENU_APPEND_FILE, _( "Load Gerber File" ),
                       _( "Load new Gerber file on currrent layer" ),
                       FALSE );

    filesMenu->Append( ID_MENU_INC_LAYER_AND_APPEND_FILE,
                       _( "Inc Layer and load Gerber file" ),
                       _( "Increment layer number, and Load Gerber file" ),
                       FALSE );

    filesMenu->Append( ID_GERBVIEW_LOAD_DCODE_FILE, _( "Load DCodes" ),
                       _( "Load D-Codes File" ), FALSE );
#if 0
    filesMenu->Append( ID_GERBVIEW_LOAD_DRILL_FILE, _( "Load Drill" ),
                       _( "Load excellon drill file" ), FALSE );
#endif

    filesMenu->Append( ID_MENU_NEW_BOARD, _( "&New" ),
                       _( "Clear all layers" ), FALSE );

    filesMenu->AppendSeparator();
    filesMenu->Append( ID_GERBVIEW_EXPORT_TO_PCBNEW,  _( "&Export to Pcbnew" ),
                       _( "Export data in pcbnew format" ), FALSE );

#if 0
    filesMenu->AppendSeparator();
    filesMenu->Append( ID_MENU_SAVE_BOARD, _( "&Save Layers" ),
                       _( "Save current layers (GERBER format)" ), FALSE );

    filesMenu->Append( ID_MENU_SAVE_BOARD_AS, _( "Save Layers As..." ),
                       _( "Save current layers as.." ), FALSE );
#endif

    filesMenu->AppendSeparator();

    filesMenu->Append( ID_GEN_PRINT, _( "P&rint" ), _( "Print gerber" ) );
    filesMenu->Append( ID_GEN_PLOT, _( "Plot" ),
                       _( "Plotting in various formats" ) );

    filesMenu->AppendSeparator();
    filesMenu->Append( ID_EXIT, _( "E&xit" ), _( "Quit Gerbview" ) );

    wxGetApp().m_fileHistory.AddFilesToMenu( filesMenu );

    // Configuration:
    wxMenu* configmenu = new wxMenu;
    ADD_MENUITEM_WITH_HELP( configmenu, ID_CONFIG_REQ, _( "&File Ext" ),
                            _( "Setting Files extension" ), config_xpm );
    ADD_MENUITEM_WITH_HELP( configmenu, ID_COLORS_SETUP, _( "&Colors" ),
                            _( "Select Colors and Display for layers" ),
                            palette_xpm );
    ADD_MENUITEM_WITH_HELP( configmenu, ID_OPTIONS_SETUP, _( "&Options" ),
                            _( " Select general options" ), preference_xpm );

    ADD_MENUITEM_WITH_HELP( configmenu, ID_PCB_LOOK_SETUP, _( "Display" ),
                            _( " Select how items are displayed" ),
                            display_options_xpm );

    // Font selection and setup
    AddFontSelectionMenu( configmenu );

    wxGetApp().AddMenuLanguageList( configmenu );

    configmenu->AppendSeparator();
    ADD_MENUITEM_WITH_HELP( configmenu, ID_CONFIG_SAVE, _( "&Save Setup" ),
                            _( "Save application preferences" ),
                            save_setup_xpm );

    configmenu->AppendSeparator();
    AddHotkeyConfigMenu( configmenu );

    // Menu drill ( generation fichiers percage)

/*	wxMenu *drill_menu = new wxMenu;
 *  postprocess_menu->Append(ID_PCB_GEN_DRILL_FILE, "Create &Drill file",
 *                  "Gen Drill (EXCELLON] file and/or Drill sheet");
 */

    // Menu d'outils divers
    wxMenu* miscellaneous_menu = new wxMenu;
    ADD_MENUITEM_WITH_HELP( miscellaneous_menu, ID_GERBVIEW_SHOW_LIST_DCODES,
                            _( "&List DCodes" ),
                            _( "List and edit D-codes" ), show_dcodenumber_xpm );
    ADD_MENUITEM_WITH_HELP( miscellaneous_menu, ID_GERBVIEW_SHOW_SOURCE,
                            _( "&Show Source" ),
                            _( "Show source file for the current layer" ),
                            tools_xpm );
    miscellaneous_menu->AppendSeparator();
    ADD_MENUITEM_WITH_HELP( miscellaneous_menu, ID_PCB_GLOBAL_DELETE,
                            _( "&Delete Layer" ),
                            _( "Delete current layer" ), general_deletions_xpm );

    // Menu Help:
    wxMenu* helpMenu = new wxMenu;
    ADD_MENUITEM_WITH_HELP( helpMenu, ID_GENERAL_HELP, _( "&Contents" ),
                            _( "Open the gerbview manual" ), help_xpm );
    ADD_MENUITEM_WITH_HELP( helpMenu, ID_KICAD_ABOUT, _( "&About Gerbview" ),
                            _( "About gerbview gerber and drill viewer" ),
                            info_xpm );

    menuBar->Append( filesMenu, _( "&File" ) );
    menuBar->Append( configmenu, _( "&Preferences" ) );
    menuBar->Append( miscellaneous_menu, _( "&Miscellaneous" ) );

//		menuBar->Append(drill_menu, _("&Drill"));
    menuBar->Append( helpMenu, _( "&Help" ) );

    // Associate the menu bar with the frame
    SetMenuBar( menuBar );
}


/***********************************************/
void WinEDA_GerberFrame::ReCreateHToolbar( void )
/***********************************************/
{
    int           layer = 0;
    GERBER*       gerber = NULL;
    int           ii;
    wxString      msg;

    // delete and recreate the toolbar
    if( m_HToolBar  != NULL )
        return;

    if( GetScreen() )
    {
        layer = GetScreen()->m_Active_Layer;
        gerber = g_GERBER_List[layer];
    }

    m_HToolBar = new WinEDA_Toolbar( TOOLBAR_MAIN, this, ID_H_TOOLBAR, TRUE );
    SetToolBar( m_HToolBar );

    // Set up toolbar
    m_HToolBar->AddTool( ID_NEW_BOARD, wxBitmap( new_xpm ),
                         wxNullBitmap, FALSE,
                         -1, -1, (wxObject*) NULL,
                         _( "New World" ) );

    m_HToolBar->AddTool( ID_LOAD_FILE, wxBitmap( open_xpm ),
                         wxNullBitmap, FALSE,
                         -1, -1, (wxObject*) NULL,
                         _( "Open existing Layer" ) );

#if 0
    m_HToolBar->AddTool( ID_SAVE_PROJECT, wxBitmap( save_button ),
                         wxNullBitmap, FALSE,
                         -1, -1, (wxObject*) NULL,
                         _( "Save World" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_SHEET_SET, wxBitmap( sheetset_xpm ),
                         wxNullBitmap,
                         FALSE,
                         -1, -1, (wxObject*) NULL,
                         _( "page settings (size, texts)" ) );

#endif

    m_HToolBar->AddSeparator();

#if 0
    m_HToolBar->AddTool( wxID_CUT, wxBitmap( cut_button ),
                         wxNullBitmap,
                         FALSE,
                         -1, -1, (wxObject*) NULL,
                         _( "Cut selected item" ) );

    m_HToolBar->AddTool( wxID_COPY, wxBitmap( copy_button ),
                         wxNullBitmap, FALSE,
                         -1, -1, (wxObject*) NULL,
                         _( "Copy selected item" ) );

    m_HToolBar->AddTool( wxID_PASTE, wxBitmap( paste_xpm ),
                         wxNullBitmap,
                         FALSE,
                         -1, -1, (wxObject*) NULL,
                         _( "Paste" ) );
#endif

    m_HToolBar->AddTool( ID_UNDO_BUTT, wxBitmap( undelete_xpm ),
                         wxNullBitmap,
                         FALSE,
                         -1, -1, (wxObject*) NULL,
                         _( "Undelete" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_GEN_PRINT, wxBitmap( print_button ),
                         wxNullBitmap,
                         FALSE,
                         -1, -1, (wxObject*) NULL,
                         _( "Print World" ) );

    m_HToolBar->AddSeparator();
    msg = AddHotkeyName( _( "Zoom in" ), s_Gerbview_Hokeys_Descr, HK_ZOOM_IN );
    m_HToolBar->AddTool( ID_ZOOM_IN, wxBitmap( zoom_in_xpm ),
                         wxNullBitmap,
                         FALSE,
                         -1, -1, (wxObject*) NULL,
                         msg );

    msg = AddHotkeyName( _( "Zoom out" ), s_Gerbview_Hokeys_Descr, HK_ZOOM_OUT );
    m_HToolBar->AddTool( ID_ZOOM_OUT, wxBitmap( zoom_out_xpm ),
                         wxNullBitmap,
                         FALSE,
                         -1, -1, (wxObject*) NULL,
                         msg );

    msg = AddHotkeyName( _( "Redraw view" ), s_Gerbview_Hokeys_Descr, HK_ZOOM_REDRAW );
    m_HToolBar->AddTool( ID_ZOOM_REDRAW, wxBitmap( zoom_redraw_xpm ),
                         wxNullBitmap,
                         FALSE,
                         -1, -1, (wxObject*) NULL,
                         msg );

    m_HToolBar->AddTool( ID_ZOOM_PAGE, wxBitmap( zoom_auto_xpm ),
                         wxNullBitmap,
                         FALSE,
                         -1, -1, (wxObject*) NULL,
                         _( "Zoom auto" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_FIND_ITEMS, wxBitmap( find_xpm ),
                         wxNullBitmap,
                         FALSE,
                         -1, -1, (wxObject*) NULL,
                         _( "Find D-codes" ) );

    wxArrayString choices;
    m_HToolBar->AddSeparator();
    for( ii = 0; ii < 32; ii++ )
    {
        wxString msg;
        msg = _( "Layer " ); msg << ii + 1;
        choices.Add( msg );
    }

    m_SelLayerBox = new WinEDAChoiceBox( m_HToolBar, ID_TOOLBARH_PCB_SELECT_LAYER,
                                         wxDefaultPosition, wxSize( 150, -1 ), choices );
    m_SelLayerBox->SetSelection( GetScreen()->m_Active_Layer );
    m_HToolBar->AddControl( m_SelLayerBox );

    m_HToolBar->AddSeparator();
    choices.Clear();
    choices.Add( _( "No tool" ) );
    for( ii = 0; ii < MAX_TOOLS; ii++ )
    {
        wxString msg;
        msg = _( "Tool " ); msg << ii + FIRST_DCODE;
        choices.Add( msg );
    }

    m_SelLayerTool = new WinEDAChoiceBox( m_HToolBar, ID_TOOLBARH_GERBER_SELECT_TOOL,
                                          wxDefaultPosition, wxSize( 150, -1 ), choices );
    m_HToolBar->AddControl( m_SelLayerTool );


    // after adding the buttons to the toolbar, must call Realize() to reflect
    // the changes
    m_HToolBar->Realize();
    SetToolbars();
}


/**********************************************/
void WinEDA_GerberFrame::ReCreateVToolbar( void )
/**********************************************/
/**
create or update the right vertical toolbar
*/
{
    if( m_VToolBar )
        return;

    m_VToolBar = new WinEDA_Toolbar( TOOLBAR_TOOL, this, ID_V_TOOLBAR, FALSE );

    // Set up toolbar
    m_VToolBar->AddTool( ID_NO_SELECT_BUTT,
                         wxBitmap( cursor_xpm ),
                         wxNullBitmap, TRUE,
                         -1, -1, (wxObject*) NULL );
    m_VToolBar->ToggleTool( ID_NO_SELECT_BUTT, TRUE );

#if 0
    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_COMPONENT_BUTT,
                         wxBitmap( component_button ),
                         wxNullBitmap, TRUE,
                         -1, -1, (wxObject*) NULL,
                         _( "Add Flashes" ) );

    m_VToolBar->AddTool( ID_BUS_BUTT,
                         wxBitmap( bus_button ),
                         wxNullBitmap, TRUE,
                         -1, -1, (wxObject*) NULL,
                         _( "Add Lines" ) );

    m_VToolBar->AddTool( ID_JUNCTION_BUTT,
                         wxBitmap( junction_xpm ),
                         wxNullBitmap, TRUE,
                         -1, -1, (wxObject*) NULL,
                         _( "Add layer alignment target" ) );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_TEXT_COMMENT_BUTT,
                         wxBitmap( tool_text_xpm ),
                         wxNullBitmap, TRUE,
                         -1, -1, (wxObject*) NULL,
                         _( "Add Text" ) );

#endif
    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_PCB_DELETE_ITEM_BUTT,
                         wxBitmap( delete_body_xpm ),
                         wxNullBitmap, TRUE,
                         -1, -1, (wxObject*) NULL,
                         _( "Delete items" ) );

    m_VToolBar->Realize();
    SetToolbars();
}


/************************************************/
void WinEDA_GerberFrame::ReCreateOptToolbar( void )
/************************************************/
/**
create or update the left vertical toolbar (option toolbar
*/
{
    if( m_OptionsToolBar )
        return;

    // creation du tool bar options
    m_OptionsToolBar = new WinEDA_Toolbar( TOOLBAR_OPTION, this, ID_OPT_TOOLBAR, FALSE );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxBitmap( grid_xpm ),
                               wxNullBitmap,
                               TRUE,
                               -1, -1, (wxObject*) NULL,
                               _( "Display Grid OFF" ) );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLAR_COORD, wxBitmap( polar_coord_xpm ),
                               wxNullBitmap,
                               TRUE,
                               -1, -1, (wxObject*) NULL,
                               _( "Display Polar Coord ON" ) );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               wxBitmap( unit_inch_xpm ),
                               _( "Units in inches" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               wxBitmap( unit_mm_xpm ),
                               _( "Units in millimeters" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxBitmap( cursor_shape_xpm ),
                               wxNullBitmap,
                               TRUE,
                               -1, -1, (wxObject*) NULL,
                               _( "Change Cursor Shape" ) );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH,
                               wxBitmap( pad_sketch_xpm ),
                               wxNullBitmap,
                               TRUE,
                               -1, -1, (wxObject*) NULL,
                               _( "Show Spots in Sketch Mode" ) );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_TRACKS_SKETCH,
                               wxBitmap( showtrack_xpm ),
                               wxNullBitmap,
                               TRUE,
                               -1, -1, (wxObject*) NULL,
                               _( "Show Lines in Sketch Mode" ) );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLYGONS_SKETCH,
                               wxBitmap( opt_show_polygon_xpm ),
                               wxNullBitmap,
                               TRUE,
                               -1, -1, (wxObject*) NULL,
                               _( "Show Polygons in Sketch Mode" ) );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_DCODES,
                               wxBitmap( show_dcodenumber_xpm ),
                               wxNullBitmap,
                               TRUE,
                               -1, -1, (wxObject*) NULL,
                               _( "Show dcode number" ) );

    m_OptionsToolBar->Realize();

    SetToolbars();
}
