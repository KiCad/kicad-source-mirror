/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
 * @file gerbview/menubar.cpp
 * @brief (Re)Create the main menubar for GerbView
 */

#include "gerbview_frame.h"

#include <advanced_config.h>
#include <kiface_i.h>
#include <pgm_base.h>
#include "gerbview_id.h"
#include "hotkeys.h"
#include <menus_helpers.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <tool/conditional_menu.h>
#include <tools/gerbview_selection_tool.h>
#include <tools/gerbview_actions.h>

void GERBVIEW_FRAME::ReCreateMenuBar()
{
    GERBVIEW_SELECTION_TOOL* selTool = m_toolManager->GetTool<GERBVIEW_SELECTION_TOOL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar* oldMenuBar = GetMenuBar();
    wxMenuBar* menuBar = new wxMenuBar();
    wxString   text;

    // Recreate all menus:

    // Menu File:
    wxMenu* fileMenu = new wxMenu;

    // Open Gerber file(s)
    AddMenuItem( fileMenu, wxID_FILE,
                 _( "Open &Gerber File(s)..." ),
                 _( "Open Gerber file(s) on the current layer. Previous data will be deleted" ),
                 KiBitmap( load_gerber_xpm ) );

    // Open Excellon drill file(s)
    AddMenuItem( fileMenu, ID_GERBVIEW_LOAD_DRILL_FILE,
                 _( "Open &Excellon Drill File(s)..." ),
                 _( "Open Excellon drill file(s) on the current layer. Previous data will be deleted" ),
                 KiBitmap( gerbview_drill_file_xpm ) );

    // Open Gerber job files
    AddMenuItem( fileMenu, ID_GERBVIEW_LOAD_JOB_FILE,
                 _( "Open Gerber &Job File..." ),
                 _( "Open a Gerber job file, and it's associated gerber files depending on the job" ),
                 KiBitmap( gerber_job_file_xpm ) );

    // Open Zip archive files
    AddMenuItem( fileMenu, ID_GERBVIEW_LOAD_ZIP_ARCHIVE_FILE,
                 _( "Open &Zip Archive File..." ),
                 _( "Open a zipped archive (Gerber and Drill) file" ),
                 KiBitmap( zip_xpm ) );

    // Recent gerber files
    static wxMenu* openRecentGbrMenu;

    // Add this menu to list menu managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history
    if( openRecentGbrMenu )
        Kiface().GetFileHistory().RemoveMenu( openRecentGbrMenu );

    openRecentGbrMenu = new wxMenu();

    Kiface().GetFileHistory().UseMenu( openRecentGbrMenu );
    Kiface().GetFileHistory().AddFilesToMenu();

    AddMenuItem( fileMenu, openRecentGbrMenu, wxID_ANY,
                 _( "Open &Recent Gerber File" ),
                 _( "Open a recently opened Gerber file" ),
                 KiBitmap( recent_xpm ) );

    // Recent drill files
    static wxMenu* openRecentDrlMenu;

    if( openRecentDrlMenu )
        m_drillFileHistory.RemoveMenu( openRecentDrlMenu );

    openRecentDrlMenu = new wxMenu();
    m_drillFileHistory.UseMenu( openRecentDrlMenu );
    m_drillFileHistory.AddFilesToMenu( );
    AddMenuItem( fileMenu, openRecentDrlMenu, wxID_ANY,
                 _( "Open Recent Excellon Dri&ll File" ),
                 _( "Open a recently opened Excellon drill file" ),
                 KiBitmap( recent_xpm ) );

    // Recent job files
    static wxMenu* openRecentJobFilesMenu;

    if( openRecentJobFilesMenu )
        m_jobFileHistory.RemoveMenu( openRecentJobFilesMenu );

    openRecentJobFilesMenu = new wxMenu();
    m_jobFileHistory.UseMenu( openRecentJobFilesMenu );
    m_jobFileHistory.AddFilesToMenu( );
    AddMenuItem( fileMenu, openRecentJobFilesMenu, wxID_ANY,
                 _( "Open Recent Gerber &Job File" ),
                 _( "Open a recently opened gerber job file" ),
                 KiBitmap( recent_xpm ) );

    // Recent Zip archive
    static wxMenu* openRecentZipArchiveMenu;

    if( openRecentZipArchiveMenu )
        m_zipFileHistory.RemoveMenu( openRecentZipArchiveMenu );

    openRecentZipArchiveMenu = new wxMenu();
    m_zipFileHistory.UseMenu( openRecentZipArchiveMenu );
    m_zipFileHistory.AddFilesToMenu( );
    AddMenuItem( fileMenu, openRecentZipArchiveMenu, wxID_ANY,
                 _( "Open Recent Zip &Archive File" ),
                 _( "Open a recently opened zip archive file" ),
                 KiBitmap( recent_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Clear all
    AddMenuItem( fileMenu,
                 ID_GERBVIEW_ERASE_ALL,
                 _( "Clear &All Layers" ),
                 _( "Clear all layers. All data will be deleted" ),
                 KiBitmap( delete_gerber_xpm ) );

    // Reload all
    AddMenuItem( fileMenu,
                 ID_GERBVIEW_RELOAD_ALL,
                 _( "Reload All Layers" ),
                 _( "Reload all layers. All data will be reloaded" ),
                 KiBitmap( reload2_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Export to Pcbnew
    AddMenuItem( fileMenu,
                 ID_GERBVIEW_EXPORT_TO_PCBNEW,
                 _( "E&xport to Pcbnew..." ),
                 _( "Export data in Pcbnew format" ),
                 KiBitmap( export_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Print
    AddMenuItem( fileMenu, wxID_PRINT,
                 _( "&Print..." ), _( "Print layers" ),
                 KiBitmap( print_button_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Exit
    AddMenuItem( fileMenu, wxID_EXIT,
                 _( "&Close" ), _( "Close GerbView" ),
                 KiBitmap( exit_xpm ) );

    //--------- View menu ----------------
    CONDITIONAL_MENU* viewMenu = new CONDITIONAL_MENU( false, selTool );

    auto gridShownCondition = [ this ] ( const SELECTION& aSel ) {
        return IsGridVisible();
    };
    auto polarCoordsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetShowPolarCoords();
    };
    auto layersManagerShownCondition = [ this ] ( const SELECTION& aSel ) {
        return m_show_layer_manager_tools;
    };
    auto imperialUnitsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetUserUnits() == INCHES;
    };
    auto metricUnitsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetUserUnits() == MILLIMETRES;
    };
    auto sketchFlashedCondition = [ this ] ( const SELECTION& aSel ) {
        return !m_DisplayOptions.m_DisplayFlashedItemsFill;
    };
    auto sketchLinesCondition = [ this ] ( const SELECTION& aSel ) {
        return !m_DisplayOptions.m_DisplayLinesFill;
    };
    auto sketchPolygonsCondition = [ this ] ( const SELECTION& aSel ) {
        return !m_DisplayOptions.m_DisplayPolygonsFill;
    };
    auto showDcodes = [ this ] ( const SELECTION& aSel ) {
        return IsElementVisible( LAYER_DCODES );
    };
    auto showNegativeObjects = [ this ] ( const SELECTION& aSel ) {
        return IsElementVisible( LAYER_NEGATIVE_OBJECTS );
    };
    auto diffModeCondition = [ this ] ( const SELECTION& aSel ) {
        return m_DisplayOptions.m_DiffMode;
    };
    auto contrastModeCondition = [ this ] ( const SELECTION& aSel ) {
        return m_DisplayOptions.m_HighContrastMode;
    };

    // Hide layer manager
    viewMenu->AddCheckItem( ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR,
                            _( "Show &Layers Manager" ), _( "Show or hide the layer manager" ),
                            layers_manager_xpm, layersManagerShownCondition );

    viewMenu->AppendSeparator();

    viewMenu->AddSeparator();
    viewMenu->AddItem( ACTIONS::zoomInCenter,    SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomOutCenter,   SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomFitScreen,   SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomTool,        SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomRedraw,      SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AppendSeparator();
    viewMenu->AddCheckItem( ACTIONS::toggleGrid,              gridShownCondition );
    viewMenu->AddCheckItem( ACTIONS::togglePolarCoords,       polarCoordsCondition );

    // Units submenu
    CONDITIONAL_MENU* unitsSubMenu = new CONDITIONAL_MENU( false, selTool );
    unitsSubMenu->SetTitle( _( "&Units" ) );
    unitsSubMenu->SetIcon( unit_mm_xpm );
    unitsSubMenu->AddCheckItem( ACTIONS::imperialUnits,       imperialUnitsCondition );
    unitsSubMenu->AddCheckItem( ACTIONS::metricUnits,         metricUnitsCondition );
    viewMenu->AddMenu( unitsSubMenu );

    viewMenu->AppendSeparator();
    viewMenu->AddCheckItem( GERBVIEW_ACTIONS::flashedDisplayOutlines,  sketchFlashedCondition );
    viewMenu->AddCheckItem( GERBVIEW_ACTIONS::linesDisplayOutlines,    sketchLinesCondition );
    viewMenu->AddCheckItem( GERBVIEW_ACTIONS::polygonsDisplayOutlines, sketchPolygonsCondition );
    viewMenu->AddCheckItem( GERBVIEW_ACTIONS::dcodeDisplay,            showDcodes );
    viewMenu->AddCheckItem( GERBVIEW_ACTIONS::negativeObjectDisplay,   showNegativeObjects );

    viewMenu->AddCheckItem( ID_TB_OPTIONS_DIFF_MODE,
                            _( "Show in Differential Mode" ),
                            _( "Show layers in differential mode" ),
                            gbr_select_mode2_xpm, diffModeCondition );

    text = AddHotkeyName( _( "Show in High Contrast" ), GerbviewHotkeysDescr, HK_SWITCH_HIGHCONTRAST_MODE );
    viewMenu->AddCheckItem( ID_TB_OPTIONS_HIGH_CONTRAST_MODE, text,
                            _( "Show in high contrast mode" ),
                            contrast_mode_xpm, contrastModeCondition );

    // Menu for configuration and preferences
    wxMenu* configMenu = new wxMenu;

    // Options (Preferences on WXMAC)
    text = AddHotkeyName( _( "&Preferences..." ), GerbviewHotkeysDescr, HK_PREFERENCES );
    AddMenuItem( configMenu, wxID_PREFERENCES, text,
                 _( "Show preferences for all open tools" ),
                 KiBitmap( preference_xpm ) );

    // Canvas selection
    configMenu->AppendSeparator();

    if( ADVANCED_CFG::GetCfg().AllowLegacyCanvas() )
    {
        text = AddHotkeyName( _( "Legacy Tool&set" ), GerbviewHotkeysDescr, HK_CANVAS_LEGACY );
        AddMenuItem( configMenu, ID_MENU_CANVAS_LEGACY, text,
                _( "Use Legacy Toolset (not all features will be available)" ),
                KiBitmap( tools_xpm ), wxITEM_RADIO );
    }

    text = AddHotkeyName( _( "Modern Toolset (&Accelerated)" ), GerbviewHotkeysDescr, HK_CANVAS_OPENGL );
    AddMenuItem( configMenu, ID_MENU_CANVAS_OPENGL, text,
                 _( "Use Modern Toolset with hardware-accelerated graphics (recommended)" ),
                 KiBitmap( tools_xpm ), wxITEM_RADIO );

    text = AddHotkeyName( _( "Modern Toolset (Fallba&ck)" ), GerbviewHotkeysDescr, HK_CANVAS_CAIRO );
    AddMenuItem( configMenu, ID_MENU_CANVAS_CAIRO, text,
                 _( "Use Modern Toolset with software graphics (fall-back)" ),
                 KiBitmap( tools_xpm ), wxITEM_RADIO );

    configMenu->AppendSeparator();

    // Language submenu
    Pgm().AddMenuLanguageList( configMenu );

    // Menu miscellaneous
    wxMenu* miscellaneousMenu = new wxMenu;

    // List dcodes
    AddMenuItem( miscellaneousMenu, ID_GERBVIEW_SHOW_LIST_DCODES, _( "&List DCodes..." ),
                 _( "List D-codes defined in Gerber files" ),
                 KiBitmap( show_dcodenumber_xpm ) );

    // Show source
    AddMenuItem( miscellaneousMenu, ID_GERBVIEW_SHOW_SOURCE, _( "&Show Source..." ),
                 _( "Show source file for the current layer" ),
                 KiBitmap( tools_xpm ) );

    miscellaneousMenu->AppendSeparator();

    // Erase graphic layer
    AddMenuItem( miscellaneousMenu, ID_GERBVIEW_ERASE_CURR_LAYER, _( "&Clear Current Layer..." ),
                 _( "Clear the graphic layer currently selected" ),
                 KiBitmap( delete_sheet_xpm ) );

    // Help menu
    wxMenu* helpMenu = new wxMenu;

    AddMenuItem( helpMenu, wxID_HELP, _( "Gerbview &Manual" ),
                 _( "Open the GerbView Manual" ),
                 KiBitmap( online_help_xpm ) );

    text = AddHotkeyName( _( "&List Hotkeys..." ), GerbviewHotkeysDescr, HK_HELP );
    AddMenuItem( helpMenu, ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST, text,
                 _( "Displays the current hotkeys list and corresponding commands" ),
                 KiBitmap( hotkeys_xpm ) );

    // Separator
    helpMenu->AppendSeparator();

    // Get involved with KiCad
    AddMenuItem( helpMenu, ID_HELP_GET_INVOLVED, _( "Get &Involved" ),
                 _( "Contribute to KiCad (opens a web browser)" ),
                 KiBitmap( info_xpm ) );

    helpMenu->AppendSeparator();

    // About Kicad
    AddMenuItem( helpMenu, wxID_ABOUT, _( "&About KiCad" ), KiBitmap( about_xpm ) );

    // Append menus to the menubar
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( miscellaneousMenu, _( "&Tools" ) );
    menuBar->Append( configMenu, _( "&Preferences" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    // Associate the menu bar with the frame, if no previous menubar
    SetMenuBar( menuBar );
    delete oldMenuBar;
}
