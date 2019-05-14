/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <menus_helpers.h>
#include <pgm_base.h>
#include <tool/conditional_menu.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>

#include "eeschema_id.h"
#include "general.h"
#include "help_common_strings.h"
#include "ee_hotkeys.h"
#include "lib_edit_frame.h"


void LIB_EDIT_FRAME::ReCreateMenuBar()
{
    EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar* oldMenuBar = GetMenuBar();
    wxMenuBar* menuBar = new wxMenuBar();
    wxString   text;

    // Recreate all menus:

    // Menu File:
    wxMenu* fileMenu = new wxMenu;

    AddMenuItem( fileMenu,
                 ID_LIBEDIT_NEW_LIBRARY,
                 _( "New Library..." ),
                 _( "Creates an empty library" ),
                 KiBitmap( new_library_xpm ) );

    AddMenuItem( fileMenu,
                 ID_LIBEDIT_ADD_LIBRARY,
                 _( "Add Library..." ),
                 _( "Adds a previously created library" ),
                 KiBitmap( add_library_xpm ) );

    text = AddHotkeyName( _( "&New Symbol..." ), m_hotkeysDescrList, HK_NEW );
    AddMenuItem( fileMenu,
                 ID_LIBEDIT_NEW_PART,
                 text,
                 _( "Create a new symbol" ),
                 KiBitmap( new_component_xpm ) );

    fileMenu->AppendSeparator();

    text = AddHotkeyName( _( "&Save" ), g_Libedit_Hotkeys_Descr, HK_SAVE );
    AddMenuItem( fileMenu,
                 ID_LIBEDIT_SAVE,
                 text,
                 _( "Save changes" ),
                 KiBitmap( save_xpm ) );

    text = AddHotkeyName( _( "Save &As..." ), g_Libedit_Hotkeys_Descr, HK_SAVEAS );
    AddMenuItem( fileMenu,
                 ID_LIBEDIT_SAVE_AS,
                 text,
                 _( "Save a copy to a new name and/or location" ),
                 KiBitmap( save_as_xpm ) );

    AddMenuItem( fileMenu,
                 ID_LIBEDIT_SAVE_ALL,
                 _( "Save All" ),
                 _( "Save all library and symbol changes" ),
                 KiBitmap( save_xpm ) );

    AddMenuItem( fileMenu,
                 ID_LIBEDIT_REVERT,
                 _( "&Revert" ),
                 _( "Throw away changes" ),
                 KiBitmap( undo_xpm ) );

    fileMenu->AppendSeparator();

    AddMenuItem( fileMenu,
                 ID_LIBEDIT_IMPORT_PART,
                 _( "&Import Symbol..." ),
                 _( "Import a symbol to the current library" ),
                 KiBitmap( import_part_xpm ) );

    // Export submenu
    wxMenu* submenuExport = new wxMenu();

    AddMenuItem( submenuExport,
                 ID_LIBEDIT_EXPORT_PART,
                 _( "S&ymbol..." ),
                 _( "Create a library file containing only the current symbol" ),
                 KiBitmap( export_part_xpm ) );

    AddMenuItem( submenuExport,
                 ID_LIBEDIT_GEN_PNG_FILE,
                 _( "View as &PNG..." ),
                 _( "Create a PNG file from the current view" ),
                 KiBitmap( plot_xpm ) );

    AddMenuItem( submenuExport,
                 ID_LIBEDIT_GEN_SVG_FILE,
                 _( "Symbol as S&VG..." ),
                 _( "Create a SVG file from the current symbol" ),
                 KiBitmap( plot_svg_xpm ) );

    AddMenuItem( fileMenu, submenuExport, ID_GEN_EXPORT_FILE, _( "E&xport" ),
                 _( "Export files" ),
                 KiBitmap( export_xpm ) );

    fileMenu->AppendSeparator();

    AddMenuItem( fileMenu,
                 wxID_EXIT,
                 _( "&Quit" ),
                 _( "Quit Library Editor" ),
                 KiBitmap( exit_xpm ) );

    //
    // Edit menu
    //
    CONDITIONAL_MENU* editMenu = new CONDITIONAL_MENU( false, selTool );

    auto enableUndoCondition = [ this ] ( const SELECTION& sel ) {
        return GetCurPart() && GetScreen() && GetScreen()->GetUndoCommandCount() != 0
                      && EE_CONDITIONS::Idle( sel );
    };
    auto enableRedoCondition = [ this ] ( const SELECTION& sel ) {
        return GetCurPart() && GetScreen() && GetScreen()->GetRedoCommandCount() != 0
                      && EE_CONDITIONS::Idle( sel );
    };
    auto havePartCondition = [ this ] ( const SELECTION& sel ) {
        return GetCurPart();
    };

    editMenu->AddItem( ACTIONS::undo,                enableUndoCondition );
    editMenu->AddItem( ACTIONS::redo,                enableRedoCondition );

    editMenu->AddSeparator();
    editMenu->AddItem( EE_ACTIONS::symbolProperties, havePartCondition );
    editMenu->AddItem( EE_ACTIONS::pinTable,         havePartCondition );

    //
    // Menu View:
    //
    CONDITIONAL_MENU* viewMenu = new CONDITIONAL_MENU( false, selTool );

    auto gridShownCondition = [ this ] ( const SELECTION& aSel ) {
        return IsGridVisible();
    };
    auto imperialUnitsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetUserUnits() == INCHES;
    };
    auto metricUnitsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetUserUnits() == MILLIMETRES;
    };
    auto fullCrosshairCondition = [ this ] ( const SELECTION& aSel ) {
        return GetGalDisplayOptions().m_fullscreenCursor;
    };
    auto compTreeShownCondition = [ this ] ( const SELECTION& aSel ) {
        return IsSearchTreeShown();
    };

    viewMenu->AddItem( EE_ACTIONS::showLibraryBrowser,     EE_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddItem( ACTIONS::zoomInCenter,              EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomOutCenter,             EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomFitScreen,             EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomTool,                  EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomRedraw,                EE_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddCheckItem( ACTIONS::toggleGrid,           gridShownCondition );
    viewMenu->AddItem( ACTIONS::gridProperties,            EE_CONDITIONS::ShowAlways );

    // Units submenu
    CONDITIONAL_MENU* unitsSubMenu = new CONDITIONAL_MENU( false, selTool );
    unitsSubMenu->SetTitle( _( "&Units" ) );
    unitsSubMenu->SetIcon( unit_mm_xpm );
    unitsSubMenu->AddCheckItem( ACTIONS::imperialUnits,    imperialUnitsCondition );
    unitsSubMenu->AddCheckItem( ACTIONS::metricUnits,      metricUnitsCondition );
    viewMenu->AddMenu( unitsSubMenu );

    viewMenu->AddCheckItem( ACTIONS::toggleCursorStyle,    fullCrosshairCondition );

    viewMenu->AddSeparator();
    viewMenu->AddCheckItem( EE_ACTIONS::showComponentTree, compTreeShownCondition );

    //
    // Menu Place:
    //
    CONDITIONAL_MENU* placeMenu = new CONDITIONAL_MENU( false, selTool );

    placeMenu->AddItem( EE_ACTIONS::placeSymbolPin,        EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeSymbolText,       EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::drawSymbolRectangle,   EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::drawSymbolCircle,      EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::drawSymbolArc,         EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::drawSymbolLines,       EE_CONDITIONS::ShowAlways );

    //
    // Menu Inspect:
    //
    wxMenu* inspectMenu = new wxMenu;


    text = AddHotkeyName( _( "Show Datasheet" ), g_Libedit_Hotkeys_Descr, HK_LIBEDIT_VIEW_DOC );
    AddMenuItem( inspectMenu,
                 ID_LIBEDIT_VIEW_DOC,
                 text,
                 _( "Open associated datasheet in web browser" ),
                 KiBitmap( datasheet_xpm ) );

    AddMenuItem( inspectMenu,
                 ID_LIBEDIT_CHECK_PART,
                 _( "Electrical Rules &Checker" ),
                 _( "Check duplicate and off grid pins" ),
                 KiBitmap( erc_xpm ) );

    // Menu Preferences:
    wxMenu* preferencesMenu = new wxMenu;

    // Environment varialbes
    AddMenuItem( preferencesMenu,
                 ID_PREFERENCES_CONFIGURE_PATHS,
                 _( "&Configure Paths..." ),
                 _( "Edit path configuration environment variables" ),
                 KiBitmap( path_xpm ) );

    // Library list
    AddMenuItem( preferencesMenu,
                 ID_EDIT_SYM_LIB_TABLE,
                 _( "Manage &Symbol Libraries..." ),
                 _( "Edit the global and project symbol library tables." ),
                 KiBitmap( library_table_xpm ) );

    preferencesMenu->AppendSeparator();

    // Default values and options
    text = AddHotkeyName( _( "&Preferences..." ), g_Libedit_Hotkeys_Descr, HK_PREFERENCES );
    AddMenuItem( preferencesMenu, wxID_PREFERENCES, text,
                 _( "Show preferences for all open tools" ),
                 KiBitmap( preference_xpm ) );

    // Language submenu
    Pgm().AddMenuLanguageList( preferencesMenu );

    preferencesMenu->AppendSeparator();

    text = AddHotkeyName( _( "Modern Toolset (&Accelerated)" ), g_Libedit_Hotkeys_Descr,
                          HK_CANVAS_OPENGL );
    AddMenuItem( preferencesMenu, ID_MENU_CANVAS_OPENGL, text,
                 _( "Use Modern Toolset with hardware-accelerated graphics (recommended)" ),
                 KiBitmap( tools_xpm ), wxITEM_RADIO );

    text = AddHotkeyName( _( "Modern Toolset (Fallba&ck)" ), g_Libedit_Hotkeys_Descr,
                          HK_CANVAS_CAIRO );
    AddMenuItem( preferencesMenu, ID_MENU_CANVAS_CAIRO, text,
                 _( "Use Modern Toolset with software graphics (fall-back)" ),
                 KiBitmap( tools_xpm ), wxITEM_RADIO );



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

    text = AddHotkeyName( _( "&List Hotkeys..." ), g_Libedit_Hotkeys_Descr, HK_HELP );
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

    AddMenuItem( helpMenu, wxID_ABOUT, _( "&About KiCad" ), KiBitmap( about_xpm ) );

    // Create the menubar and append all submenus
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( inspectMenu, _( "&Inspect" ) );
    menuBar->Append( preferencesMenu, _( "P&references" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}
