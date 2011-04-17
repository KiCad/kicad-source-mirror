/**
 * @file eeschema/menubar_libedit.cpp
 * @brief (Re)Create the main menubar for the component editor frame (LibEdit)
 */
#include "fctsys.h"
#include "common.h"
#include "appl_wxstruct.h"
#include "bitmaps.h"

#include "general.h"
#include "libeditframe.h"
#include "eeschema_id.h"
#include "hotkeys.h"

#include "help_common_strings.h"

/**
 * @brief (Re)Create the menubar for the component editor frame
 */
void LIB_EDIT_FRAME::ReCreateMenuBar()
{
    // Create and try to get the current menubar
    wxString   text;
    wxMenuItem *item;
    wxMenuBar  *menuBar = GetMenuBar();

    if( ! menuBar )
        menuBar = new wxMenuBar();

    // Delete all existing menus so they can be rebuilt.
    // This allows language changes of the menu text on the fly.
    menuBar->Freeze();
    while( menuBar->GetMenuCount() )
        delete menuBar->Remove(0);

    // Recreate all menus:

    // Menu File:
    wxMenu* fileMenu = new wxMenu;

    // Save current library
    item = new wxMenuItem( fileMenu,
                           ID_LIBEDIT_SAVE_CURRENT_LIB,
                           _( "&Save Current Library\tCtrl+S" ),
                           _( "Save the current active library" ) );
    SET_BITMAP( save_xpm );
    fileMenu->Append( item );

    // Save current library as...
    item = new wxMenuItem( fileMenu,
                           ID_LIBEDIT_SAVE_CURRENT_LIB_AS,
                           _( "Save Current Library &as" ),
                           _( "Save current active library as..." ) );
    SET_BITMAP( save_as_xpm );
    fileMenu->Append( item );

    // Separator
    fileMenu->AppendSeparator();

    // Export as png file
    item = new wxMenuItem( fileMenu,
                           ID_LIBEDIT_GEN_PNG_FILE,
                           _( "&Create PNG File from Screen" ),
                           _( "Create a PNG file from the component displayed on screen" ) );
    SET_BITMAP( plot_xpm );
    fileMenu->Append( item );

    // Export as SVG file
    item = new wxMenuItem( fileMenu,
                           ID_LIBEDIT_GEN_SVG_FILE,
                           _( "&Create SVG File" ),
                           _( "Create a SVG file from the current loaded component" ) );
    SET_BITMAP( plot_xpm );
    fileMenu->Append( item );

    // Separator
    fileMenu->AppendSeparator();

    // Quit
    item = new wxMenuItem( fileMenu,
                           wxID_EXIT,
                           _( "&Quit" ),
                           _( "Quit Library Editor" ) );
    SET_BITMAP( exit_xpm );
    fileMenu->Append( item );

    // Edit menu
    wxMenu* editMenu = new wxMenu;

    // Undo
    text  = AddHotkeyName( _( "Undo" ), s_Libedit_Hokeys_Descr, HK_UNDO);

    item = new wxMenuItem( editMenu,
                           wxID_UNDO,
                           text,
                           _( "Undo last edition" ),
                           wxITEM_NORMAL );
    SET_BITMAP( undo_xpm );
    editMenu->Append( item );

    // Redo
    text  = AddHotkeyName( _( "Redo" ), s_Libedit_Hokeys_Descr, HK_REDO);

    item = new wxMenuItem( editMenu,
                           wxID_REDO,
                           text,
                           _( "Redo the last undo command" ),
                           wxITEM_NORMAL );
    SET_BITMAP( redo_xpm );
    editMenu->Append( item );

    // Separator
    editMenu->AppendSeparator();

    // Delete
    item = new wxMenuItem( editMenu,
                           ID_LIBEDIT_DELETE_ITEM_BUTT,
                           _( "Delete" ),
                           HELP_DELETE_ITEMS,
                           wxITEM_NORMAL );
    SET_BITMAP( delete_body_xpm );
    editMenu->Append( item );

    // Menu View:
    wxMenu* viewMenu = new wxMenu;

    /**
     * Important Note for ZOOM IN and ZOOM OUT commands from menubar:
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

    // Zoom in
    text  =_( "Zoom In" );
    item = new wxMenuItem( viewMenu, ID_ZOOM_IN, text, HELP_ZOOM_IN, wxITEM_NORMAL );
    SET_BITMAP( zoom_in_xpm );
    viewMenu->Append( item );

    // Zoom out
    text = _( "Zoom Out" );
    item = new wxMenuItem( viewMenu, ID_ZOOM_OUT, text, HELP_ZOOM_OUT, wxITEM_NORMAL );
    SET_BITMAP( zoom_out_xpm );
    viewMenu->Append( item );

    // Fit on screen
    text = AddHotkeyName( _( "Fit on Screen" ), s_Schematic_Hokeys_Descr, HK_ZOOM_AUTO );

    item = new wxMenuItem( viewMenu, ID_ZOOM_PAGE, text, HELP_ZOOM_FIT, wxITEM_NORMAL );
    SET_BITMAP( zoom_fit_in_page_xpm );
    viewMenu->Append( item );

    // Separator
    viewMenu->AppendSeparator();

    // Redraw
    text = AddHotkeyName( _( "Redraw" ), s_Schematic_Hokeys_Descr, HK_ZOOM_REDRAW );

    item = new wxMenuItem( viewMenu, ID_ZOOM_REDRAW, text, HELP_ZOOM_REDRAW, wxITEM_NORMAL );
    SET_BITMAP( zoom_redraw_xpm );
    viewMenu->Append( item );

    // Menu Place:
    wxMenu* placeMenu = new wxMenu;

    // Pin
    item = new wxMenuItem( placeMenu,
                           ID_LIBEDIT_PIN_BUTT,
                           _( "&Pin" ),
                           HELP_ADD_PIN,
                           wxITEM_NORMAL );
    SET_BITMAP( pin_xpm );
    placeMenu->Append( item );

    // Graphic text
    item = new wxMenuItem( placeMenu,
                           ID_LIBEDIT_BODY_TEXT_BUTT,
                           _( "Graphic text" ),
                           HELP_ADD_BODYTEXT,
                           wxITEM_NORMAL );
    SET_BITMAP( add_text_xpm );
    placeMenu->Append( item );

    // Graphic rectangle
    item = new wxMenuItem( placeMenu,
                           ID_LIBEDIT_BODY_RECT_BUTT,
                           _( "Rectangle" ),
                           HELP_ADD_BODYRECT,
                           wxITEM_NORMAL );
    SET_BITMAP( add_rectangle_xpm );
    placeMenu->Append( item );

    // Graphic Circle
    item = new wxMenuItem( placeMenu,
                           ID_LIBEDIT_BODY_CIRCLE_BUTT,
                           _( "Circle" ),
                           HELP_ADD_BODYCIRCLE,
                           wxITEM_NORMAL );
    SET_BITMAP( add_circle_xpm );
    placeMenu->Append( item );

    // Graphic Arc
    item = new wxMenuItem( placeMenu,
                           ID_LIBEDIT_BODY_ARC_BUTT,
                           _( "Arc" ),
                           HELP_ADD_BODYARC,
                           wxITEM_NORMAL );
    SET_BITMAP( add_arc_xpm );
    placeMenu->Append( item );

    // Graphic Line or Polygon
    item = new wxMenuItem( placeMenu,
                           ID_LIBEDIT_BODY_LINE_BUTT,
                           _( "Line or Polygon" ),
                           HELP_ADD_BODYPOLYGON,
                           wxITEM_NORMAL );
    SET_BITMAP( add_polygon_xpm );
    placeMenu->Append( item );

    // Menu Preferences:
    wxMenu* preferencesMenu = new wxMenu;

    // Library
    item = new wxMenuItem( preferencesMenu,
                           ID_CONFIG_REQ,
                           _( "&Library" ),
                           _( "Library preferences" ) );
    SET_BITMAP( library_xpm );
    preferencesMenu->Append( item );

    // Colors
    item = new wxMenuItem( preferencesMenu,
                           ID_COLORS_SETUP,
                           _( "&Colors" ),
                           _( "Color preferences" ) );
    SET_BITMAP( palette_xpm );
    preferencesMenu->Append( item );

    // Language submenu
    wxGetApp().AddMenuLanguageList( preferencesMenu );

    // Hotkey submenu
    AddHotkeyConfigMenu( preferencesMenu );

    // Separator
    preferencesMenu->AppendSeparator();

    // Save preferences
    item = new wxMenuItem( preferencesMenu,
                           ID_CONFIG_SAVE,
                           _( "&Save preferences" ),
                           _( "Save application preferences" ) );
    SET_BITMAP( save_setup_xpm );
    preferencesMenu->Append( item );

    // Read preferences
    item = new wxMenuItem( preferencesMenu,
                           ID_CONFIG_READ,
                           _( "&Read preferences" ),
                           _( "Read application preferences" ) );
    SET_BITMAP( read_setup_xpm );
    preferencesMenu->Append( item );

    // Menu Help:
    wxMenu* helpMenu = new wxMenu;

    // Version info
    AddHelpVersionInfoMenuEntry( helpMenu );

    // Contens
    item = new wxMenuItem( helpMenu,
                           wxID_HELP,
                           _( "&Contents" ),
                           _( "Open the eeschema manual" ) );
    SET_BITMAP( online_help_xpm );
    helpMenu->Append( item );

    // Create the menubar and append all submenus
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( preferencesMenu, _( "&Preferences" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    menuBar->Thaw();

    // Associate the menu bar with the frame, if no previous menubar
    if( GetMenuBar() == NULL )
        SetMenuBar( menuBar );
    else
        menuBar->Refresh();
}
