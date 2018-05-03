/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @brief (Re)Create the main menubar for the part editor frame (LibEdit)
 */

#include <menus_helpers.h>
#include <pgm_base.h>

#include "eeschema_id.h"
#include "general.h"
#include "help_common_strings.h"
#include "hotkeys.h"
#include "lib_edit_frame.h"


/**
 * @brief (Re)Create the menubar for the part editor frame
 */
void LIB_EDIT_FRAME::ReCreateMenuBar()
{
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar* oldMenuBar = GetMenuBar();
    wxMenuBar* menuBar = new wxMenuBar();
    wxString   text;

    // Recreate all menus:

    // Menu File:
    wxMenu* fileMenu = new wxMenu;

    // Creating/loading libraries
    AddMenuItem( fileMenu,
                 ID_LIBEDIT_NEW_LIBRARY,
                 _( "&New Library..." ),
                 _( "Creates an empty library" ),
                 KiBitmap( new_library_xpm ) );

    AddMenuItem( fileMenu,
                 ID_LIBEDIT_ADD_LIBRARY,
                 _( "&Add Library..." ),
                 _( "Adds a previously created library" ),
                 KiBitmap( add_library_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Save library variants
    AddMenuItem( fileMenu,
                 ID_LIBEDIT_SAVE_LIBRARY,
                 _( "&Save Library" ),
                 _( "Save the current library" ),
                 KiBitmap( save_library_xpm ) );

    AddMenuItem( fileMenu,
                 ID_LIBEDIT_SAVE_LIBRARY_AS,
                 _( "Save Library As..." ),
                 _( "Save the current library to a new file" ),
                 KiBitmap( save_as_xpm ) );

    text = AddHotkeyName( _( "Save All &Libraries" ), g_Libedit_Hokeys_Descr, HK_SAVE_ALL_LIBS );
    AddMenuItem( fileMenu, ID_LIBEDIT_SAVE_ALL_LIBS, text, _( "Save all library changes" ),
                 KiBitmap( save_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Export as png file
    AddMenuItem( fileMenu,
                 ID_LIBEDIT_GEN_PNG_FILE,
                 _( "Export Current View as &PNG..." ),
                 _( "Create a PNG file from the current view" ),
                 KiBitmap( plot_xpm ) );

    // Export as SVG file
    AddMenuItem( fileMenu,
                 ID_LIBEDIT_GEN_SVG_FILE,
                 _( "Create S&VG File..." ),
                 _( "Create a SVG file from the current symbol" ),
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
    text = AddHotkeyName( _( "&Zoom to Fit" ), g_Libedit_Hokeys_Descr, HK_ZOOM_AUTO );
    AddMenuItem( viewMenu, ID_ZOOM_PAGE, text, _( "Zoom to fit symbol" ),
                 KiBitmap( zoom_fit_in_page_xpm ) );

    text = AddHotkeyName( _( "Zoom to Selection" ),
                          g_Schematic_Hokeys_Descr, HK_ZOOM_SELECTION );
    AddMenuItem( viewMenu, ID_MENU_ZOOM_SELECTION, text, KiBitmap( zoom_area_xpm ) );

    // Redraw
    text = AddHotkeyName( _( "&Redraw" ), g_Libedit_Hokeys_Descr, HK_ZOOM_REDRAW );
    AddMenuItem( viewMenu, ID_ZOOM_REDRAW, text, HELP_ZOOM_REDRAW, KiBitmap( zoom_redraw_xpm ) );

    // Separator
    viewMenu->AppendSeparator();

    AddMenuItem( viewMenu,
                 ID_LIBEDIT_SHOW_HIDE_SEARCH_TREE,
                 _( "&Search Tree" ),
                 _( "Toggles the search tree visibility" ),
                 KiBitmap( search_tree_xpm ) );

    // Menu Part:
    wxMenu* partMenu = new wxMenu;

    AddMenuItem( partMenu,
                 ID_LIBEDIT_NEW_PART,
                 _( "&New Symbol..." ),
                 _( "Create a new empty symbol" ),
                 KiBitmap( new_component_xpm ) );

    AddMenuItem( partMenu,
                 ID_LIBEDIT_SAVE_PART,
                 _( "&Save Symbol" ),
                 _( "Saves the current symbol to the library" ),
                 KiBitmap( save_part_xpm ) );

    partMenu->AppendSeparator();

    AddMenuItem( partMenu,
                 ID_LIBEDIT_IMPORT_PART,
                 _( "&Import Symbol..." ),
                 _( "Import a symbol to the current library" ),
                 KiBitmap( import_part_xpm ) );

    AddMenuItem( partMenu,
                 ID_LIBEDIT_EXPORT_PART,
                 _( "&Export Symbol..." ),
                 _( "Export the current symbol" ),
                 KiBitmap( export_part_xpm ) );

    partMenu->AppendSeparator();

    AddMenuItem( partMenu,
                 ID_LIBEDIT_GET_FRAME_EDIT_PART,
                 _( "&Properties..." ),
                 _( "Edit symbol properties" ),
                 KiBitmap( part_properties_xpm ) );

    AddMenuItem( partMenu,
                 ID_LIBEDIT_GET_FRAME_EDIT_FIELDS,
                 _( "&Fields..." ),
                 _( "Edit field properties" ),
                 KiBitmap( edit_text_xpm ) );

    partMenu->AppendSeparator();

    AddMenuItem( partMenu,
                 ID_LIBEDIT_EDIT_PIN_BY_TABLE,
                 _( "Pi&n Table..." ),
                 _( "Show pin table" ),
                 KiBitmap( pin_table_xpm ) );

    AddMenuItem( partMenu,
                 ID_LIBEDIT_CHECK_PART,
                 _( "Electrical Rules Checker" ),
                 _( "Check duplicate and off grid pins" ),
                 KiBitmap( erc_xpm ) );

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
                 KiBitmap( text_xpm ) );

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

    // Environment varialbes
    AddMenuItem( preferencesMenu,
                 ID_PREFERENCES_CONFIGURE_PATHS,
                 _( "Configure Pa&ths..." ),
                 _( "Edit path configuration environment variables" ),
                 KiBitmap( path_xpm ) );

    // Library list
    AddMenuItem( preferencesMenu,
                 ID_EDIT_SYM_LIB_TABLE,
                 _( "Manage Symbol Libraries..." ),
                 _( "Edit the global and project symbol library tables." ),
                 KiBitmap( library_table_xpm ) );

    preferencesMenu->AppendSeparator();

    // Default values and options
    AddMenuItem( preferencesMenu,
                 wxID_PREFERENCES,
                 _( "General &Options..." ),
                 _( "Set Symbol Editor default values and options" ),
                 KiBitmap( preference_xpm ) );

    // Language submenu
    Pgm().AddMenuLanguageList( preferencesMenu );

    // Hotkey submenu
    AddHotkeyConfigMenu( preferencesMenu );

    // Menu Help:
    wxMenu* helpMenu = new wxMenu;

    // Contents
    AddMenuItem( helpMenu,
                 wxID_HELP,
                 _( "Eeschema &Manual" ),
                 _( "Open the Eeschema Manual" ),
                 KiBitmap( online_help_xpm ) );

    AddMenuItem( helpMenu,
                 wxID_INDEX,
                 _( "&Getting Started in KiCad" ),
                 _( "Open the \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

    text = AddHotkeyName( _( "&List Hotkeys..." ), g_Eeschema_Hokeys_Descr, HK_HELP );
    AddMenuItem( helpMenu,
                 ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST,
                 text,
                 _( "Displays the current hotkeys list and corresponding commands" ),
                 KiBitmap( hotkeys_xpm ) );

    helpMenu->AppendSeparator();
    AddMenuItem( helpMenu, ID_HELP_GET_INVOLVED,
                 _( "Get &Involved" ),
                 _( "Contribute to KiCad (opens a web browser)" ),
                 KiBitmap( info_xpm ) );

    // About Eeschema
    helpMenu->AppendSeparator();

    AddMenuItem( helpMenu,
                 wxID_ABOUT,
                 _( "&About KiCad" ),
                 _( "About KiCad" ),
                 KiBitmap( about_xpm ) );

    // Create the menubar and append all submenus
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( partMenu, _( "&Symbol" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( preferencesMenu, _( "P&references" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}
