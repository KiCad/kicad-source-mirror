/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
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

#include "footprint_edit_frame.h"

#include <advanced_config.h>
#include <menus_helpers.h>
#include <pgm_base.h>
#include <tool/tool_manager.h>
#include <tool/conditional_menu.h>
#include <tool/actions.h>
#include <tools/selection_tool.h>
#include <tools/pcb_actions.h>
#include "help_common_strings.h"
#include "hotkeys.h"
#include "pcbnew.h"
#include "pcbnew_id.h"


void FOOTPRINT_EDIT_FRAME::ReCreateMenuBar()
{
    SELECTION_TOOL* selTool = m_toolManager->GetTool<SELECTION_TOOL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar* oldMenuBar = GetMenuBar();
    wxMenuBar* menuBar = new wxMenuBar();
    wxString   text;

    // Recreate all menus:

    // Menu File:
    wxMenu* fileMenu = new wxMenu;

    AddMenuItem( fileMenu,
                 ID_MODEDIT_CREATE_NEW_LIB,
                 _( "New Library..." ),
                 _( "Creates an empty library" ),
                 KiBitmap( new_library_xpm ) );

    AddMenuItem( fileMenu,
                 ID_MODEDIT_ADD_LIBRARY,
                 _( "Add Library..." ),
                 _( "Adds a previously created library" ),
                 KiBitmap( add_library_xpm ) );

    text = AddHotkeyName( _( "&New Footprint..." ), m_hotkeysDescrList, HK_NEW );
    AddMenuItem( fileMenu, ID_MODEDIT_NEW_MODULE,
                 text, _( "Create a new footprint" ),
                 KiBitmap( new_footprint_xpm ) );

#ifdef KICAD_SCRIPTING
    AddMenuItem( fileMenu, ID_MODEDIT_NEW_MODULE_FROM_WIZARD,
                 _( "&Create Footprint..." ),
                 _( "Create a new footprint using the footprint wizard" ),
                 KiBitmap( module_wizard_xpm ) );
#endif

    fileMenu->AppendSeparator();

    text = AddHotkeyName( _( "&Save" ), m_hotkeysDescrList, HK_SAVE );
    AddMenuItem( fileMenu, ID_MODEDIT_SAVE, text,
                 _( "Save changes" ),
                 KiBitmap( save_xpm ) );

    text = AddHotkeyName( _( "Save &As..." ), m_hotkeysDescrList, HK_SAVEAS );
    AddMenuItem( fileMenu, ID_MODEDIT_SAVE_AS, text,
                 _( "Save a copy to a new name and/or location" ),
                 KiBitmap( save_as_xpm ) );

    AddMenuItem( fileMenu, ID_MODEDIT_REVERT_PART,
                 _( "&Revert" ),
                 _( "Throw away changes" ),
                 KiBitmap( undo_xpm ) );

    fileMenu->AppendSeparator();

    wxMenu* submenuImport = new wxMenu();

    AddMenuItem( submenuImport, ID_MODEDIT_IMPORT_PART,
                 _( "&Footprint..." ),
                 _( "Import a footprint from file" ),
                 KiBitmap( import_module_xpm ) );

    AddMenuItem( submenuImport, ID_GEN_IMPORT_GRAPHICS_FILE,
                 _( "&Graphics..." ),
                 _( "Import 2D Drawing file to Footprint Editor on Drawings layer" ),
                 KiBitmap( import_vector_xpm ) );

    AddMenuItem( fileMenu, submenuImport,
                 ID_GEN_IMPORT_FILE, _( "&Import" ),
                 _( "Import files" ), KiBitmap( import_xpm ) );

    wxMenu* submenuExport = new wxMenu();

    AddMenuItem( submenuExport, ID_MODEDIT_EXPORT_PART,
                 _( "&Footprint..." ),
                 _( "Export current footprint to a file" ),
                 KiBitmap( export_module_xpm ) );

    AddMenuItem( submenuExport, ID_MODEDIT_SAVE_PNG,
                 _( "View as &PNG..." ),
                 _( "Create a PNG file from the current view" ),
                 KiBitmap( plot_xpm ) );

    AddMenuItem( fileMenu, submenuExport,
                 ID_GEN_EXPORT_FILE, _( "&Export" ),
                 _( "Export files" ), KiBitmap( export_xpm ) );

    fileMenu->AppendSeparator();

    // Print
    text = AddHotkeyName( _( "&Print..." ), m_hotkeysDescrList, HK_PRINT );
    AddMenuItem( fileMenu, wxID_PRINT, text,
                 _( "Print current footprint" ),
                 KiBitmap( print_button_xpm ) );

    fileMenu->AppendSeparator();

    // Close editor
    AddMenuItem( fileMenu, wxID_EXIT,
                 _( "&Exit" ),
                 _( "Close footprint editor" ),
                 KiBitmap( exit_xpm ) );

    //----- Edit menu ------------------
    CONDITIONAL_MENU* editMenu = new CONDITIONAL_MENU( false, selTool );

    auto enableUndoCondition = [ this ] ( const SELECTION& sel ) {
        return GetScreen() && GetScreen()->GetUndoCommandCount() > 0;
    };
    auto enableRedoCondition = [ this ] ( const SELECTION& sel ) {
        return GetScreen() && GetScreen()->GetRedoCommandCount() > 0;
    };
    auto noActiveToolCondition = [ this ] ( const SELECTION& aSelection ) {
        return GetToolId() == ID_NO_TOOL_SELECTED;
    };

    text  = AddHotkeyName( _( "&Undo" ), g_Board_Editor_Hotkeys_Descr, HK_UNDO );
    editMenu->AddItem( ACTIONS::undo,    enableUndoCondition );
    editMenu->AddItem( ACTIONS::redo,    enableRedoCondition );

    editMenu->AddSeparator();
    editMenu->AddItem( ACTIONS::cut,     SELECTION_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::copy,    SELECTION_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::paste,   noActiveToolCondition );

    // Properties
    AddMenuItem( editMenu, ID_MODEDIT_EDIT_MODULE_PROPERTIES,
                 _( "&Footprint Properties..." ),
                 _( "Edit footprint properties" ),
                 KiBitmap( module_options_xpm ) );

    AddMenuItem( editMenu, ID_MODEDIT_PAD_SETTINGS,
                 _( "Default Pad Properties..." ),
                 _( "Edit default pad properties" ),
                 KiBitmap( options_pad_xpm ) );

    editMenu->AppendSeparator();

    AddMenuItem( editMenu, ID_MODEDIT_DELETE_PART,
                 _( "&Delete Footprint from Library" ),
                 _( "Delete the current footprint" ),
                 KiBitmap( delete_xpm ) );

    //--------- View menu ----------------
    CONDITIONAL_MENU* viewMenu = new CONDITIONAL_MENU( false, selTool );

    auto gridShownCondition = [ this ] ( const SELECTION& aSel ) {
        return IsGridVisible();
    };
    auto polarCoordsCondition = [ this ] ( const SELECTION& aSel ) {
        return ( (PCB_DISPLAY_OPTIONS*) GetDisplayOptions() )->m_DisplayPolarCood;
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
    auto sketchPadsCondition = [ this ] ( const SELECTION& aSel ) {
        return !( (PCB_DISPLAY_OPTIONS*) GetDisplayOptions() )->m_DisplayPadFill;
    };
    auto sketchEdgesCondition = [ this ] ( const SELECTION& aSel ) {
        return !( (PCB_DISPLAY_OPTIONS*) GetDisplayOptions() )->m_DisplayModEdgeFill;
    };
    auto contrastModeCondition = [ this ] ( const SELECTION& aSel ) {
        return !( (PCB_DISPLAY_OPTIONS*) GetDisplayOptions() )->m_ContrastModeDisplay;
    };
    auto searchTreeShownCondition = [ this ] ( const SELECTION& aSel ) {
        return IsSearchTreeShown();
    };

    viewMenu->AddItem( ID_OPEN_MODULE_VIEWER,
                       _( "Footprint &Library Browser" ), _( "Browse footprint libraries" ),
                       modview_icon_xpm, SELECTION_CONDITIONS::ShowAlways );

    text = AddHotkeyName( _( "&3D Viewer" ), m_hotkeysDescrList, HK_3D_VIEWER );
    viewMenu->AddItem( ID_MENU_PCB_SHOW_3D_FRAME,
                       text, _( "Show footprint in 3D viewer" ),
                       three_d_xpm, SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddItem( ACTIONS::zoomInCenter,    SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomOutCenter,   SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomFitScreen,   SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomTool,        SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomRedraw,      SELECTION_CONDITIONS::ShowAlways );


    viewMenu->AppendSeparator();
    viewMenu->AddCheckItem( ACTIONS::toggleGrid, gridShownCondition );
    viewMenu->AddItem( ACTIONS::gridProperties,  SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddCheckItem( ID_TB_OPTIONS_SHOW_POLAR_COORD,
                            _( "Display &Polar Coordinates" ), wxEmptyString,
                            polar_coord_xpm, polarCoordsCondition );

    // Units submenu
    CONDITIONAL_MENU* unitsSubMenu = new CONDITIONAL_MENU( false, selTool );
    unitsSubMenu->SetTitle( _( "&Units" ) );
    unitsSubMenu->SetIcon( unit_mm_xpm );
    unitsSubMenu->AddCheckItem( ACTIONS::imperialUnits,   imperialUnitsCondition );
    unitsSubMenu->AddCheckItem( ACTIONS::metricUnits,     metricUnitsCondition );
    viewMenu->AddMenu( unitsSubMenu );

    viewMenu->AddCheckItem( ACTIONS::toggleCursorStyle,   fullCrosshairCondition );

    viewMenu->AppendSeparator();

    viewMenu->AddSeparator();

    // Drawing Mode Submenu
    CONDITIONAL_MENU* drawingModeSubMenu = new CONDITIONAL_MENU( false, selTool );
    drawingModeSubMenu->SetTitle( _( "&Drawing Mode" ) );
    drawingModeSubMenu->SetIcon( add_zone_xpm );

    drawingModeSubMenu->AddCheckItem( PCB_ACTIONS::padDisplayMode,     sketchPadsCondition );
    drawingModeSubMenu->AddCheckItem( PCB_ACTIONS::moduleEdgeOutlines, sketchEdgesCondition );

    viewMenu->AddMenu( drawingModeSubMenu );

    // Contrast Mode Submenu
    CONDITIONAL_MENU* contrastModeSubMenu = new CONDITIONAL_MENU( false, selTool );
    contrastModeSubMenu->SetTitle( _( "&Contrast Mode" ) );
    contrastModeSubMenu->SetIcon( contrast_mode_xpm );

    contrastModeSubMenu->AddCheckItem( PCB_ACTIONS::highContrastMode,   contrastModeCondition );

    contrastModeSubMenu->AddSeparator();
    contrastModeSubMenu->AddItem( PCB_ACTIONS::layerAlphaDec, SELECTION_CONDITIONS::ShowAlways );
    contrastModeSubMenu->AddItem( PCB_ACTIONS::layerAlphaInc, SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddMenu( contrastModeSubMenu );

    // Separator
    viewMenu->AppendSeparator();

    viewMenu->AddCheckItem( ID_MODEDIT_SHOW_HIDE_SEARCH_TREE,
                            _( "&Search Tree" ), _( "Toggles the search tree visibility" ),
                            search_tree_xpm, searchTreeShownCondition );


    //-------- Place menu --------------------
    CONDITIONAL_MENU* placeMenu = new CONDITIONAL_MENU( false, selTool );

    placeMenu->AddItem( PCB_ACTIONS::placePad,    SELECTION_CONDITIONS::ShowAlways );

    placeMenu->AppendSeparator();
    placeMenu->AddItem( PCB_ACTIONS::placeText,   SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawArc,     SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawCircle,  SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawLine,    SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawPolygon, SELECTION_CONDITIONS::ShowAlways );

    placeMenu->AppendSeparator();
    placeMenu->AddItem( PCB_ACTIONS::setAnchor,   SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( ACTIONS::gridSetOrigin,   SELECTION_CONDITIONS::ShowAlways );


    //----- Inspect menu ---------------------
    wxMenu* inspectMenu = new wxMenu;

    text = AddHotkeyName( _( "&Measure" ), m_hotkeysDescrList, HK_MEASURE_TOOL );
    AddMenuItem( inspectMenu, ID_MODEDIT_MEASUREMENT_TOOL,
                 text, _( "Measure distance" ),
                 KiBitmap( measurement_xpm ) );

    //----- Tools menu ---------------------
    wxMenu* toolsMenu = new wxMenu;

    AddMenuItem( toolsMenu, ID_MODEDIT_LOAD_MODULE_FROM_BOARD,
                 _( "&Load Footprint from PCB..." ),
                 _( "Load a footprint from the current board into the editor" ),
                 KiBitmap( load_module_board_xpm ) );

    AddMenuItem( toolsMenu, ID_ADD_FOOTPRINT_TO_BOARD,
                 _( "&Insert Footprint on PCB" ),
                 _( "Insert footprint onto current board" ),
                 KiBitmap( insert_module_board_xpm ) );


    //----- Preferences menu -----------------
    wxMenu* prefs_menu = new wxMenu;

    // Path configuration edit dialog.
    AddMenuItem( prefs_menu,
                 ID_PREFERENCES_CONFIGURE_PATHS,
                 _( "&Configure Paths..." ),
                 _( "Edit path configuration environment variables" ),
                 KiBitmap( path_xpm ) );

    AddMenuItem( prefs_menu, ID_PCB_LIB_TABLE_EDIT,
                _( "Manage &Footprint Libraries..." ), _( "Configure footprint library table" ),
                KiBitmap( library_table_xpm ) );

    // Settings
    text = AddHotkeyName( _( "&Preferences..." ), m_hotkeysDescrList, HK_PREFERENCES );
    AddMenuItem( prefs_menu, wxID_PREFERENCES, text,
                 _( "Show preferences for all open tools" ),
                 KiBitmap( preference_xpm ) );

    prefs_menu->AppendSeparator();

    if( ADVANCED_CFG::GetCfg().AllowLegacyCanvas() )
    {
        text = AddHotkeyName( _( "Legacy Tool&set" ), m_hotkeysDescrList, HK_CANVAS_LEGACY );
        AddMenuItem( prefs_menu, ID_MENU_CANVAS_LEGACY, text,
                _( "Use Legacy Toolset (not all features will be available)" ),
                KiBitmap( tools_xpm ), wxITEM_RADIO );
    }

    text = AddHotkeyName( _( "Modern Toolset (&Accelerated)" ), m_hotkeysDescrList, HK_CANVAS_OPENGL );
    AddMenuItem( prefs_menu, ID_MENU_CANVAS_OPENGL, text,
                 _( "Use Modern Toolset with hardware-accelerated graphics (recommended)" ),
                 KiBitmap( tools_xpm ), wxITEM_RADIO );

    text = AddHotkeyName( _( "Modern Toolset (&Fallback)" ), m_hotkeysDescrList, HK_CANVAS_CAIRO );
    AddMenuItem( prefs_menu, ID_MENU_CANVAS_CAIRO, text,
                 _( "Use Modern Toolset with software graphics (fall-back)" ),
                 KiBitmap( tools_xpm ), wxITEM_RADIO );

    prefs_menu->AppendSeparator();

    // Language submenu
    Pgm().AddMenuLanguageList( prefs_menu );

    //----- Help menu --------------------
    wxMenu* helpMenu = new wxMenu;

    // Contents
    AddMenuItem( helpMenu, wxID_HELP,
                 _( "Pcbnew &Manual" ),
                 _( "Open the Pcbnew Manual" ),
                 KiBitmap( online_help_xpm ) );

    AddMenuItem( helpMenu, wxID_INDEX,
                 _( "&Getting Started in KiCad" ),
                 _( "Open the \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

    text = AddHotkeyName( _( "&List Hotkeys..." ), m_hotkeysDescrList, HK_HELP );
    AddMenuItem( helpMenu, ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST,
                 text,
                 _( "Displays current hotkeys table and corresponding commands" ),
                 KiBitmap( hotkeys_xpm ) );

    helpMenu->AppendSeparator();

    AddMenuItem( helpMenu, ID_HELP_GET_INVOLVED,
                 _( "Get &Involved" ),
                 _( "Contribute to KiCad (opens a web browser)" ),
                 KiBitmap( info_xpm ) );

    // About Pcbnew
    helpMenu->AppendSeparator();
    AddMenuItem( helpMenu, wxID_ABOUT, _( "&About KiCad" ), KiBitmap( about_xpm ) );

    // Append menus to the menubar
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( inspectMenu, _( "&Inspect" ) );
    menuBar->Append( toolsMenu, _( "&Tools" ) );
    menuBar->Append( prefs_menu, _( "P&references" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}
