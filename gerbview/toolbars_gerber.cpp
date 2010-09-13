/***************************************************/
/*  tool_gerber.cpp: Build tool bars and main menu */
/***************************************************/

#include "fctsys.h"
#include "wx/wupdlock.h"

#include "appl_wxstruct.h"
#include "common.h"
#include "macros.h"
#include "gerbview.h"
#include "pcbplot.h"
#include "protos.h"
#include "bitmaps.h"
#include "gerbview_id.h"
#include "hotkeys.h"


void WinEDA_GerberFrame::ReCreateHToolbar( void )
{
    int           layer = 0;
    GERBER*       gerber = NULL;
    int           ii;
    wxString      msg;

    // delete and recreate the toolbar
    if( m_HToolBar != NULL )
        return;

    // we create m_SelLayerTool that have a lot of items,
    // so create a wxWindowUpdateLocker is a good idea
    wxWindowUpdateLocker dummy(this);

    if( GetScreen() )
    {
        layer = GetScreen()->m_Active_Layer;
        gerber = g_GERBER_List[layer];
    }

    m_HToolBar = new WinEDA_Toolbar( TOOLBAR_MAIN, this, ID_H_TOOLBAR, TRUE );

    // Set up toolbar
    m_HToolBar->AddTool( ID_NEW_BOARD, wxEmptyString,
                         wxBitmap( new_xpm ),
                         _( "New world" ) );

    m_HToolBar->AddTool( wxID_FILE, wxEmptyString,
                         wxBitmap( open_xpm ),
                         _( "Open existing Layer" ) );


    m_HToolBar->AddSeparator();


    m_HToolBar->AddTool( wxID_UNDO, wxEmptyString,
                         wxBitmap( undelete_xpm ),
                         _( "Undelete" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( wxID_PRINT, wxEmptyString,
                         wxBitmap( print_button ),
                         _( "Print world" ) );

    m_HToolBar->AddSeparator();
    msg = AddHotkeyName( _( "Zoom in" ), s_Gerbview_Hokeys_Descr, HK_ZOOM_IN );
    m_HToolBar->AddTool( ID_ZOOM_IN, wxEmptyString,
                         wxBitmap( zoom_in_xpm ),
                         msg );

    msg = AddHotkeyName( _( "Zoom out" ), s_Gerbview_Hokeys_Descr, HK_ZOOM_OUT );
    m_HToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString,
                         wxBitmap( zoom_out_xpm ),
                         msg );

    msg = AddHotkeyName( _( "Redraw view" ), s_Gerbview_Hokeys_Descr,
                         HK_ZOOM_REDRAW );
    m_HToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                         wxBitmap( zoom_redraw_xpm ),
                         msg );

    msg = AddHotkeyName( _( "Zoom auto" ), s_Gerbview_Hokeys_Descr,
                         HK_ZOOM_AUTO );
    m_HToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString,
                         wxBitmap( zoom_auto_xpm ),
                         msg );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_FIND_ITEMS, wxEmptyString,
                         wxBitmap( find_xpm ),
                         _( "Find D-codes" ) );

    wxArrayString choices;
    m_HToolBar->AddSeparator();
    for( ii = 0; ii < 32; ii++ )
    {
        wxString msg;
        msg = _( "Layer " ); msg << ii + 1;
        choices.Add( msg );
    }

    m_SelLayerBox = new WinEDAChoiceBox( m_HToolBar,
                                         ID_TOOLBARH_GERBVIEW_SELECT_LAYER,
                                         wxDefaultPosition, wxSize( 150, -1 ),
                                         choices );
    m_HToolBar->AddControl( m_SelLayerBox );

    m_HToolBar->AddSeparator();
    choices.Clear();

    choices.Alloc(MAX_TOOLS+1);
    choices.Add( _( "No tool" ) );

    for( ii = 0; ii < MAX_TOOLS; ii++ )
    {
        wxString msg;
        msg = _( "Tool " ); msg << ii + FIRST_DCODE;
        choices.Add( msg );
    }
    m_SelLayerTool = new WinEDAChoiceBox( m_HToolBar,
                                          ID_TOOLBARH_GERBER_SELECT_TOOL,
                                          wxDefaultPosition, wxSize( 150, -1 ),
                                          choices );
    m_HToolBar->AddControl( m_SelLayerTool );


    // after adding the buttons to the toolbar, must call Realize() to reflect
    // the changes
    m_HToolBar->Realize();
}


/**
 * Create or update the right vertical toolbar
 */
void WinEDA_GerberFrame::ReCreateVToolbar( void )
{
    if( m_VToolBar )
        return;

    wxWindowUpdateLocker dummy(this);

    m_VToolBar = new WinEDA_Toolbar( TOOLBAR_TOOL, this, ID_V_TOOLBAR, FALSE );

    // Set up toolbar
    m_VToolBar->AddTool( ID_NO_SELECT_BUTT, wxEmptyString, wxBitmap( cursor_xpm ) );
    m_VToolBar->ToggleTool( ID_NO_SELECT_BUTT, TRUE );
    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_GERBVIEW_DELETE_ITEM_BUTT, wxEmptyString,
                         wxBitmap( delete_body_xpm ),
                         _( "Delete items" ) );

    m_VToolBar->Realize();
}


/**
 * Create or update the left vertical toolbar (option toolbar
 */
void WinEDA_GerberFrame::ReCreateOptToolbar( void )
{
    if( m_OptionsToolBar )
        return;

    wxWindowUpdateLocker dummy(this);

    // creation of tool bar options
    m_OptionsToolBar = new WinEDA_Toolbar( TOOLBAR_OPTION, this, ID_OPT_TOOLBAR, FALSE );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString,
                               wxBitmap( grid_xpm ),
                               _( "Turn grid off" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLAR_COORD, wxEmptyString,
                               wxBitmap( polar_coord_xpm ),
                               _( "Turn polar coordinate on" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               wxBitmap( unit_inch_xpm ),
                               _( "Set units to inches" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               wxBitmap( unit_mm_xpm ),
                               _( "Set units to millimeters" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               wxBitmap( cursor_shape_xpm ),
                               _( "Change cursor shape" ), wxITEM_CHECK );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH, wxEmptyString,
                               wxBitmap( pad_sketch_xpm ),
                               _( "Show spots in sketch mode" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_TRACKS_SKETCH, wxEmptyString,
                               wxBitmap( showtrack_xpm ),
                               _( "Show lines in sketch mode" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLYGONS_SKETCH, wxEmptyString,
                               wxBitmap( opt_show_polygon_xpm ),
                               _( "Show polygons in sketch mode" ),
                               wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_DCODES, wxEmptyString,
                               wxBitmap( show_dcodenumber_xpm ),
                               _( "Show dcode number" ), wxITEM_CHECK );

    // Tools to show/hide toolbars:
    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR,
                               wxEmptyString,
                               wxBitmap( layers_manager_xpm ),
                               _( "Show/hide the layers manager toolbar" ),
                               wxITEM_CHECK );


    m_OptionsToolBar->Realize();
}
