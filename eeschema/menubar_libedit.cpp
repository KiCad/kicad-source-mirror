/**
 * @file menubar_libedit.cpp
 * @brief Create the main menubar for the component editor frame (LibEdit)
 */
#include "fctsys.h"
#include "common.h"
#include "appl_wxstruct.h"
#include "bitmaps.h"

#include "program.h"
#include "general.h"
//#include "protos.h"
#include "libeditframe.h"
#include "eeschema_id.h"
#include "hotkeys.h"

#include "help_common_strings.h"

/**
 * @brief Create or update the menubar for the Component Editor frame
 */
void WinEDA_LibeditFrame::ReCreateMenuBar()
{
    wxString   text;
    wxMenuItem *item;
    wxMenuBar  *menuBar = GetMenuBar();

    /**
     * Destroy the existing menu bar so it can be rebuilt.  This allows
     * language changes of the menu text on the fly.
     */
    if( menuBar )
        SetMenuBar( NULL );

    menuBar = new wxMenuBar();

    /**
     * File menu
     */
    wxMenu* filesMenu = new wxMenu;

    /* Save current lib */
    item = new wxMenuItem( filesMenu, ID_LIBEDIT_SAVE_CURRENT_LIB,
                           _( "&Save Current Library\tCtrl+S" ),
                           _( "Save the current active library" ) );
    item->SetBitmap( save_xpm );
    filesMenu->Append( item );

    /* Save as... */
    item = new wxMenuItem( filesMenu, ID_LIBEDIT_SAVE_CURRENT_LIB_AS,
                           _( "Save Current Library &as" ),
                           _( "Save current active library as..." ) );
    item->SetBitmap( save_as_xpm );
    filesMenu->Append( item );

    /* Separator */
    filesMenu->AppendSeparator();

    /* Export as png file */
    item = new wxMenuItem( filesMenu, ID_LIBEDIT_GEN_PNG_FILE, _( "&Create PNG File from Screen" ),
                           _( "Create a PNG file from the component displayed on screen" ) );
    item->SetBitmap( plot_xpm );
    filesMenu->Append( item );

    /* Export as SVG file */
    item = new wxMenuItem( filesMenu, ID_LIBEDIT_GEN_SVG_FILE, _( "&Create SVG File" ),
                           _( "Create a SVG file from the current loaded component" ) );
    item->SetBitmap( plot_xpm );
    filesMenu->Append( item );

    /*  Quit on all platforms except WXMAC, because else this "breaks" the mac
        UI compliance. The Quit item is in a different menu on a mac than
        windows or unix machine.
    */
#if !defined(__WXMAC__)
    filesMenu->AppendSeparator();
    item = new wxMenuItem( filesMenu, wxID_EXIT, _( "&Quit" ),
                           _( "Quit Library Editor" ) );
    item->SetBitmap( exit_xpm );
    filesMenu->Append( item );
#endif

    /**
     * Edit menu
     */
    wxMenu* editMenu = new wxMenu;

    /* Undo */
    text  = AddHotkeyName( _( "Undo" ), s_Libedit_Hokeys_Descr, HK_UNDO);

    item = new wxMenuItem( editMenu, wxID_UNDO, text,
                           _( "Undo last edition" ), wxITEM_NORMAL );
    item->SetBitmap( undo_xpm );
    editMenu->Append( item );

    /* Redo */
    text  = AddHotkeyName( _( "Redo" ), s_Libedit_Hokeys_Descr, HK_REDO);

    item = new wxMenuItem( editMenu, wxID_REDO, text,
                           _( "Redo the last undo command" ), wxITEM_NORMAL );
    item->SetBitmap( redo_xpm );
    editMenu->Append( item );

    /* Separator */
    editMenu->AppendSeparator();

    /* Delete */
    item = new wxMenuItem( editMenu, ID_LIBEDIT_DELETE_ITEM_BUTT,
                           _( "Delete" ), HELP_DELETE_ITEMS, wxITEM_NORMAL );
    item->SetBitmap( delete_body_xpm );
    editMenu->Append( item );

    /**
     * View menu
     */
    wxMenu* viewMenu = new wxMenu;

    /* Important Note for ZOOM IN and ZOOM OUT commands from menubar:
     * we cannot add hotkey info here, because the hotkey HK_ZOOM_IN and HK_ZOOM_OUT
     * events(default = WXK_F1 and WXK_F2) are *NOT* equivalent to this menu command:
     * zoom in and out from hotkeys are equivalent to the pop up menu zoom
     * From here, zooming is made around the screen center
     * From hotkeys, zooming is made around the mouse cursor position
     * (obviously not possible from the toolbar or menubar command)
     *
     * in others words HK_ZOOM_IN and HK_ZOOM_OUT *are NOT* accelerators
     * for Zoom in and Zoom out sub menus
     */
    /* Zoom in */
    text  =_( "Zoom In" );
    item = new wxMenuItem( viewMenu, ID_ZOOM_IN, text, HELP_ZOOM_IN,
                           wxITEM_NORMAL );
    item->SetBitmap( zoom_in_xpm );
    viewMenu->Append( item );

    /* Zoom out */
    text = _( "Zoom Out" );
    item = new wxMenuItem( viewMenu, ID_ZOOM_OUT, text, HELP_ZOOM_OUT,
                           wxITEM_NORMAL );
    item->SetBitmap( zoom_out_xpm );
    viewMenu->Append( item );

    /* Fit on screen */
    text = AddHotkeyName( _( "Fit on Screen" ), s_Schematic_Hokeys_Descr,
                          HK_ZOOM_AUTO );

    item = new wxMenuItem( viewMenu, ID_ZOOM_PAGE, text,
                           HELP_ZOOM_FIT, wxITEM_NORMAL );
    item->SetBitmap( zoom_auto_xpm );
    viewMenu->Append( item );

    viewMenu->AppendSeparator();

    /* Redraw view */
    text = AddHotkeyName( _( "Redraw" ), s_Schematic_Hokeys_Descr,
                          HK_ZOOM_REDRAW );

    item = new wxMenuItem( viewMenu, ID_ZOOM_REDRAW, text,
                           HELP_ZOOM_REDRAW, wxITEM_NORMAL );
    item->SetBitmap( zoom_redraw_xpm );
    viewMenu->Append( item );

    /**
     * Place menu
     * TODO: Unify the ID names!
     */
    wxMenu* placeMenu = new wxMenu;

    /* Pin */
    item = new wxMenuItem( placeMenu, ID_LIBEDIT_PIN_BUTT, _( "&Pin" ),
                           HELP_ADD_PIN, wxITEM_NORMAL );
    item->SetBitmap( pin_xpm );
    placeMenu->Append( item );

    /* Graphic text */
    item = new wxMenuItem( placeMenu, ID_LIBEDIT_BODY_TEXT_BUTT,
                           _( "Graphic text" ),
                           HELP_ADD_BODYTEXT, wxITEM_NORMAL );
    item->SetBitmap( add_text_xpm );
    placeMenu->Append( item );

    /* Graphic rectangle */
    item = new wxMenuItem( placeMenu, ID_LIBEDIT_BODY_RECT_BUTT,
                           _( "Rectangle" ),
                           HELP_ADD_BODYRECT, wxITEM_NORMAL );
    item->SetBitmap( add_rectangle_xpm );
    placeMenu->Append( item );

    /* Graphic Circle */
    item = new wxMenuItem( placeMenu, ID_LIBEDIT_BODY_CIRCLE_BUTT,
                           _( "Circle" ),
                           HELP_ADD_BODYCIRCLE,
                           wxITEM_NORMAL );
    item->SetBitmap( add_circle_xpm );
    placeMenu->Append( item );

    /* Graphic Arc */
    item = new wxMenuItem( placeMenu, ID_LIBEDIT_BODY_ARC_BUTT,
                           _( "Arc" ),
                           HELP_ADD_BODYARC, wxITEM_NORMAL );
    item->SetBitmap( add_arc_xpm );
    placeMenu->Append( item );

    /* Graphic line or polygon */
    item = new wxMenuItem( placeMenu, ID_LIBEDIT_BODY_LINE_BUTT,
                           _( "Line or Polygon" ),
                           HELP_ADD_BODYPOLYGON, wxITEM_NORMAL );
    item->SetBitmap( add_polygon_xpm );
    placeMenu->Append( item );

    /**
     * Preferences Menu
     */
    wxMenu* configmenu = new wxMenu;

    /* Library */
    item = new wxMenuItem( configmenu, ID_CONFIG_REQ, _( "&Library" ),
                           _( "Library preferences" ) );
    item->SetBitmap( library_xpm );
    configmenu->Append( item );

    /* Colors */
    item = new wxMenuItem( configmenu, ID_COLORS_SETUP, _( "&Colors" ),
                           _( "Color preferences" ) );
    item->SetBitmap( palette_xpm );
    configmenu->Append( item );

#if 0   // work in progress. activated when finished
/* Dimension */
    item = new wxMenuItem( configmenu, ID_LIBEDIT_DIMENSIONS, _( "&Dimensions" ),
                           _( "Thickness of graphic lines, texts sizes and others" ) );
    item->SetBitmap( add_dimension_xpm );
    configmenu->Append( item );
#endif
    /* Language submenu */
    wxGetApp().AddMenuLanguageList( configmenu );

    /* Hotkey submenu */
    AddHotkeyConfigMenu( configmenu );

    /* Separator */
    configmenu->AppendSeparator();

    /* Save preferences */
    item = new wxMenuItem( configmenu, ID_CONFIG_SAVE, _( "&Save preferences" ),
                           _( "Save application preferences" ) );
    item->SetBitmap( save_setup_xpm );
    configmenu->Append( item );

    /* Read preferences */
    item = new wxMenuItem( configmenu, ID_CONFIG_READ, _( "&Read preferences" ),
                           _( "Read application preferences" ) );
    item->SetBitmap( read_setup_xpm );
    configmenu->Append( item );

    /**
     * Help Menu
     */
    wxMenu* helpMenu = new wxMenu;

    AddHelpVersionInfoMenuEntry( helpMenu );

    item = new wxMenuItem( helpMenu, ID_GENERAL_HELP, _( "&Contents" ),
                           _( "Open the eeschema manual" ) );
    item->SetBitmap( online_help_xpm );
    helpMenu->Append( item );

    /**
     * Create the menubar and append all submenus
     */
    menuBar->Append( filesMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( configmenu, _( "&Preferences" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    /* Associate the menu bar with the frame */
    SetMenuBar( menuBar );
}
