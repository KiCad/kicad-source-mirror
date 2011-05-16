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
    wxMenuBar* menuBar = GetMenuBar();

    if( !menuBar )
        menuBar = new wxMenuBar();

    // Delete all existing menus so they can be rebuilt.
    // This allows language changes of the menu text on the fly.
    menuBar->Freeze();
    while( menuBar->GetMenuCount() )
        delete menuBar->Remove( 0 );

    // Recreate all menus:

    // Menu File:
    wxMenu* fileMenu = new wxMenu;

    // Save current library
    ADD_MENUITEM_WITH_HELP( fileMenu,
                            ID_LIBEDIT_SAVE_CURRENT_LIB,
                            _( "&Save Current Library\tCtrl+S" ),
                            _( "Save the current active library" ),
                            save_xpm );

    // Save current library as...
    ADD_MENUITEM_WITH_HELP( fileMenu,
                            ID_LIBEDIT_SAVE_CURRENT_LIB_AS,
                            _( "Save Current Library &as" ),
                            _( "Save current active library as..." ),
                            save_as_xpm );

    // Separator
    fileMenu->AppendSeparator();

    // Export as png file
    ADD_MENUITEM_WITH_HELP( fileMenu,
                            ID_LIBEDIT_GEN_PNG_FILE,
                            _( "&Create PNG File from Screen" ),
                            _( "Create a PNG file from the component displayed on screen" ),
                            plot_xpm );

    // Export as SVG file
    ADD_MENUITEM_WITH_HELP( fileMenu,
                            ID_LIBEDIT_GEN_SVG_FILE,
                            _( "&Create SVG File" ),
                            _( "Create a SVG file from the current loaded component" ),
                            plot_xpm );

    // Separator
    fileMenu->AppendSeparator();

    // Quit
    ADD_MENUITEM_WITH_HELP( fileMenu,
                            wxID_EXIT,
                            _( "&Quit" ),
                            _( "Quit Library Editor" ),
                            exit_xpm );

    // Edit menu
    wxMenu* editMenu = new wxMenu;

    // Undo
    text = AddHotkeyName( _( "Undo" ), s_Libedit_Hokeys_Descr, HK_UNDO );

    ADD_MENUITEM_WITH_HELP( editMenu,
                            wxID_UNDO,
                            text,
                            _( "Undo last edition" ),
                            undo_xpm );

    // Redo
    text = AddHotkeyName( _( "Redo" ), s_Libedit_Hokeys_Descr, HK_REDO );
    ADD_MENUITEM_WITH_HELP( editMenu,
                            wxID_REDO,
                            text,
                            _( "Redo the last undo command" ),
                            redo_xpm );

    // Separator
    editMenu->AppendSeparator();

    // Delete
    ADD_MENUITEM_WITH_HELP( editMenu,
                            ID_LIBEDIT_DELETE_ITEM_BUTT,
                            _( "Delete" ),
                            HELP_DELETE_ITEMS,
                            delete_body_xpm );

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
    text = _( "Zoom In" );
    ADD_MENUITEM_WITH_HELP( viewMenu, ID_ZOOM_IN, text, HELP_ZOOM_IN,
                            zoom_in_xpm );

    // Zoom out
    text = _( "Zoom Out" );
    ADD_MENUITEM_WITH_HELP( viewMenu, ID_ZOOM_OUT, text, HELP_ZOOM_OUT,
                            zoom_out_xpm );

    // Fit on screen
    text = AddHotkeyName( _( "Fit on Screen" ), s_Schematic_Hokeys_Descr, HK_ZOOM_AUTO );
    ADD_MENUITEM_WITH_HELP( viewMenu, ID_ZOOM_PAGE, text, HELP_ZOOM_FIT,
                            zoom_fit_in_page_xpm );

    // Separator
    viewMenu->AppendSeparator();

    // Redraw
    text = AddHotkeyName( _( "Redraw" ), s_Schematic_Hokeys_Descr, HK_ZOOM_REDRAW );
    ADD_MENUITEM_WITH_HELP( viewMenu, ID_ZOOM_REDRAW, text, HELP_ZOOM_REDRAW,
                            zoom_redraw_xpm );

    // Menu Place:
    wxMenu* placeMenu = new wxMenu;

    // Pin
    ADD_MENUITEM_WITH_HELP( placeMenu,
                            ID_LIBEDIT_PIN_BUTT,
                            _( "&Pin" ),
                            HELP_ADD_PIN,
                            pin_xpm );

    // Graphic text
    ADD_MENUITEM_WITH_HELP( placeMenu,
                            ID_LIBEDIT_BODY_TEXT_BUTT,
                            _( "Graphic text" ),
                            HELP_ADD_BODYTEXT,
                            add_text_xpm );

    // Graphic rectangle
    ADD_MENUITEM_WITH_HELP( placeMenu,
                            ID_LIBEDIT_BODY_RECT_BUTT,
                            _( "Rectangle" ),
                            HELP_ADD_BODYRECT,
                            add_rectangle_xpm );

    // Graphic Circle
    ADD_MENUITEM_WITH_HELP( placeMenu,
                            ID_LIBEDIT_BODY_CIRCLE_BUTT,
                            _( "Circle" ),
                            HELP_ADD_BODYCIRCLE,
                            add_circle_xpm );

    // Graphic Arc
    ADD_MENUITEM_WITH_HELP( placeMenu,
                            ID_LIBEDIT_BODY_ARC_BUTT,
                            _( "Arc" ),
                            HELP_ADD_BODYARC,
                            add_arc_xpm );

    // Graphic Line or Polygon
    ADD_MENUITEM_WITH_HELP( placeMenu,
                            ID_LIBEDIT_BODY_LINE_BUTT,
                            _( "Line or Polygon" ),
                            HELP_ADD_BODYPOLYGON,
                            add_polygon_xpm );

    // Menu Preferences:
    wxMenu* preferencesMenu = new wxMenu;

    // Library
    ADD_MENUITEM_WITH_HELP( preferencesMenu,
                            ID_CONFIG_REQ,
                            _( "&Library" ),
                            _( "Library preferences" ),
                            library_xpm );

    // Colors
    ADD_MENUITEM_WITH_HELP( preferencesMenu,
                            ID_COLORS_SETUP,
                            _( "&Colors" ),
                            _( "Color preferences" ),
                            palette_xpm );

    // Language submenu
    wxGetApp().AddMenuLanguageList( preferencesMenu );

    // Hotkey submenu
    AddHotkeyConfigMenu( preferencesMenu );

    // Separator
    preferencesMenu->AppendSeparator();

    // Save preferences
    ADD_MENUITEM_WITH_HELP( preferencesMenu,
                            ID_CONFIG_SAVE,
                            _( "&Save preferences" ),
                            _( "Save application preferences" ),
                            save_setup_xpm );

    // Read preferences
    ADD_MENUITEM_WITH_HELP( preferencesMenu,
                            ID_CONFIG_READ,
                            _( "&Read preferences" ),
                            _( "Read application preferences" ),
                            read_setup_xpm );

    // Menu Help:
    wxMenu* helpMenu = new wxMenu;

    // Version info
    AddHelpVersionInfoMenuEntry( helpMenu );

    // Contents
    ADD_MENUITEM_WITH_HELP( helpMenu,
                            wxID_HELP,
                            _( "&Contents" ),
                            _( "Open the eeschema manual" ),
                            online_help_xpm );

    // About EESchema
    ADD_MENUITEM_WITH_HELP( helpMenu,
                            wxID_ABOUT,
                            _( "&About EESchema" ),
                            _( "About EESchema schematic designer" ),
                            info_xpm );

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
