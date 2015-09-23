/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2009-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file eeschema/menubar_libedit.cpp
 * @brief (Re)Create the main menubar for the component editor frame (LibEdit)
 */
#include <fctsys.h>
#include <pgm_base.h>

#include <general.h>
#include <libeditframe.h>
#include <eeschema_id.h>
#include <hotkeys.h>

#include <help_common_strings.h>
#include <menus_helpers.h>

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

    // Select current library
    AddMenuItem( fileMenu,
                 ID_LIBEDIT_SELECT_CURRENT_LIB,
                 _( "&Current Library" ),
                 _( "Select working library" ),
                 KiBitmap( library_xpm ) );
    fileMenu->AppendSeparator();

    // Save current library
    AddMenuItem( fileMenu,
                 ID_LIBEDIT_SAVE_CURRENT_LIB,
                 _( "&Save Current Library\tCtrl+S" ),
                 _( "Save the current active library" ),
                 KiBitmap( save_xpm ) );

    // Save current library as...
    AddMenuItem( fileMenu,
                 ID_LIBEDIT_SAVE_CURRENT_LIB_AS,
                 _( "Save Current Library &As" ),
                 _( "Save current active library as..." ),
                 KiBitmap( save_as_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Export as png file
    AddMenuItem( fileMenu,
                 ID_LIBEDIT_GEN_PNG_FILE,
                 _( "Create &PNG File from Screen" ),
                 _( "Create a PNG file from the component displayed on screen" ),
                 KiBitmap( plot_xpm ) );

    // Export as SVG file
    AddMenuItem( fileMenu,
                 ID_LIBEDIT_GEN_SVG_FILE,
                 _( "Create S&VG File" ),
                 _( "Create a SVG file from the current loaded component" ),
                 KiBitmap( plot_svg_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Quit
    AddMenuItem( fileMenu,
                 wxID_EXIT,
                 _( "&Quit" ),
                 _( "Quit Library Editor" ),
                 KiBitmap( exit_xpm ) );

    // Edit menu
    wxMenu* editMenu = new wxMenu;

    // Undo
    text = AddHotkeyName( _( "&Undo" ), g_Libedit_Hokeys_Descr, HK_UNDO );

    AddMenuItem( editMenu,
                 wxID_UNDO,
                 text,
                 _( "Undo last edit" ),
                 KiBitmap( undo_xpm ) );

    // Redo
    text = AddHotkeyName( _( "&Redo" ), g_Libedit_Hokeys_Descr, HK_REDO );
    AddMenuItem( editMenu,
                 wxID_REDO,
                 text,
                 _( "Redo the last undo command" ),
                 KiBitmap( redo_xpm ) );

    // Separator
    editMenu->AppendSeparator();

    // Delete
    AddMenuItem( editMenu,
                 ID_LIBEDIT_DELETE_ITEM_BUTT,
                 _( "&Delete" ),
                 HELP_DELETE_ITEMS,
                 KiBitmap( delete_xpm ) );

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
    text = _( "Zoom &In" );
    AddMenuItem( viewMenu, ID_ZOOM_IN, text, HELP_ZOOM_IN, KiBitmap( zoom_in_xpm ) );

    // Zoom out
    text = _( "Zoom &Out" );
    AddMenuItem( viewMenu, ID_ZOOM_OUT, text, HELP_ZOOM_OUT, KiBitmap( zoom_out_xpm ) );

    // Fit on screen
    text = AddHotkeyName( _( "&Fit on Screen" ), g_Libedit_Hokeys_Descr, HK_ZOOM_AUTO );
    AddMenuItem( viewMenu, ID_ZOOM_PAGE, text, HELP_ZOOM_FIT, KiBitmap( zoom_fit_in_page_xpm ) );

    // Separator
    viewMenu->AppendSeparator();

    // Redraw
    text = AddHotkeyName( _( "&Redraw" ), g_Libedit_Hokeys_Descr, HK_ZOOM_REDRAW );
    AddMenuItem( viewMenu, ID_ZOOM_REDRAW, text, HELP_ZOOM_REDRAW, KiBitmap( zoom_redraw_xpm ) );

    // Menu Place:
    wxMenu* placeMenu = new wxMenu;

    // Pin
    AddMenuItem( placeMenu,
                 ID_LIBEDIT_PIN_BUTT,
                 _( "&Pin" ),
                 HELP_ADD_PIN,
                 KiBitmap( pin_xpm ) );

    // Graphic text
    AddMenuItem( placeMenu,
                 ID_LIBEDIT_BODY_TEXT_BUTT,
                 _( "Graphic &Text" ),
                 HELP_ADD_BODYTEXT,
                 KiBitmap( add_text_xpm ) );

    // Graphic rectangle
    AddMenuItem( placeMenu,
                 ID_LIBEDIT_BODY_RECT_BUTT,
                 _( "&Rectangle" ),
                 HELP_ADD_BODYRECT,
                 KiBitmap( add_rectangle_xpm ) );

    // Graphic Circle
    AddMenuItem( placeMenu,
                 ID_LIBEDIT_BODY_CIRCLE_BUTT,
                 _( "&Circle" ),
                 HELP_ADD_BODYCIRCLE,
                 KiBitmap( add_circle_xpm ) );

    // Graphic Arc
    AddMenuItem( placeMenu,
                 ID_LIBEDIT_BODY_ARC_BUTT,
                 _( "&Arc" ),
                 HELP_ADD_BODYARC,
                 KiBitmap( add_arc_xpm ) );

    // Graphic Line or Polygon
    AddMenuItem( placeMenu,
                 ID_LIBEDIT_BODY_LINE_BUTT,
                 _( "&Line or Polygon" ),
                 HELP_ADD_BODYPOLYGON,
                 KiBitmap( add_polygon_xpm ) );

    // Menu Preferences:
    wxMenu* preferencesMenu = new wxMenu;

    // Library list
    AddMenuItem( preferencesMenu,
                 ID_CONFIG_REQ,
                 _( "Component &Libraries" ),
                 _( "Configure component libraries and paths" ),
                 KiBitmap( library_xpm ) );

    // Default values and options
     AddMenuItem( preferencesMenu,
                 wxID_PREFERENCES,
                 _( "Component Editor &Options" ),
                 _( "Set Component Editor default values and options" ),
                 KiBitmap( preference_xpm ) );

    // Colors
    AddMenuItem( preferencesMenu,
                 ID_COLORS_SETUP,
                 _( "Set &Colors Scheme" ),
                 _( "Set color preferences" ),
                 KiBitmap( palette_xpm ) );

    // Language submenu
    Pgm().AddMenuLanguageList( preferencesMenu );

    // Hotkey submenu
    AddHotkeyConfigMenu( preferencesMenu );

    // Menu Help:
    wxMenu* helpMenu = new wxMenu;

    // Version info
    AddHelpVersionInfoMenuEntry( helpMenu );

    // Contents
    AddMenuItem( helpMenu,
                 wxID_HELP,
                 _( "Eeschema &Manual" ),
                 _( "Open the Eeschema manual" ),
                 KiBitmap( online_help_xpm ) );

    AddMenuItem( helpMenu,
                 wxID_INDEX,
                 _( "&Getting Started in KiCad" ),
                 _( "Open the \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

    // About Eeschema
    helpMenu->AppendSeparator();

    AddMenuItem( helpMenu,
                 wxID_ABOUT,
                 _( "&About Eeschema" ),
                 _( "About Eeschema schematic designer" ),
                 KiBitmap( info_xpm ) );

    // Create the menubar and append all submenus
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( preferencesMenu, _( "P&references" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    menuBar->Thaw();

    // Associate the menu bar with the frame, if no previous menubar
    if( GetMenuBar() == NULL )
        SetMenuBar( menuBar );
    else
        menuBar->Refresh();
}
