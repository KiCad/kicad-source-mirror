/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
#include "gerbview_frame.h"

#include <advanced_config.h>
#include <kiface_i.h>
#include <pgm_base.h>
#include "gerbview_id.h"
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

    //-- File menu -------------------------------------------------------
    //
    CONDITIONAL_MENU*   fileMenu = new CONDITIONAL_MENU( false, selTool );
    static ACTION_MENU* openRecentGbrMenu;
    static ACTION_MENU* openRecentDrlMenu;
    static ACTION_MENU* openRecentJobMenu;
    static ACTION_MENU* openRecentZipMenu;

    // Add this menu to list menu managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history)
    if( openRecentGbrMenu )
        Kiface().GetFileHistory().RemoveMenu( openRecentGbrMenu );

    openRecentGbrMenu = new ACTION_MENU( false );
    openRecentGbrMenu->SetTool( selTool );
    openRecentGbrMenu->SetTitle( _( "Open Recent Gerber" ) );
    openRecentGbrMenu->SetIcon( recent_xpm );

    Kiface().GetFileHistory().UseMenu( openRecentGbrMenu );
    Kiface().GetFileHistory().AddFilesToMenu();

    // Add drill file menu and the drill file history
    if( openRecentDrlMenu )
        m_drillFileHistory.RemoveMenu( openRecentDrlMenu );

    openRecentDrlMenu = new ACTION_MENU( false );
    openRecentDrlMenu->SetTool( selTool );
    openRecentDrlMenu->SetTitle( _( "Open Recent Drill File" ) );
    openRecentDrlMenu->SetIcon( recent_xpm );

    m_drillFileHistory.UseMenu( openRecentDrlMenu );
    m_drillFileHistory.AddFilesToMenu( );

    // Add job file menu and the job file history
    if( openRecentJobMenu )
        m_jobFileHistory.RemoveMenu( openRecentJobMenu );

    openRecentJobMenu = new ACTION_MENU( false );
    openRecentJobMenu->SetTool( selTool );
    openRecentJobMenu->SetTitle( _( "Open Recent Job" ) );

    openRecentJobMenu->SetIcon( recent_xpm );
    m_jobFileHistory.UseMenu( openRecentJobMenu );
    m_jobFileHistory.AddFilesToMenu( );

    // Add zip file menu and the zip file history
    if( openRecentZipMenu )
        m_zipFileHistory.RemoveMenu( openRecentZipMenu );

    openRecentZipMenu = new ACTION_MENU( false );
    openRecentZipMenu->SetTool( selTool );
    openRecentZipMenu->SetTitle( _( "Open Recent Zip" ) );
    openRecentZipMenu->SetIcon( recent_xpm );

    m_zipFileHistory.UseMenu( openRecentZipMenu );
    m_zipFileHistory.AddFilesToMenu( );

    fileMenu->AddItem( wxID_FILE, _( "Open &Gerber File(s)..." ),
                       _( "Open Gerber file(s) on the current layer. Previous data will be deleted" ),
                       load_gerber_xpm,            SELECTION_CONDITIONS::ShowAlways );
    fileMenu->AddMenu( openRecentGbrMenu,          SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddItem( ID_GERBVIEW_LOAD_DRILL_FILE, _( "Open &Excellon Drill File(s)..." ),
                       _( "Open Excellon drill file(s) on the current layer. Previous data will be deleted" ),
                       gerbview_drill_file_xpm,    SELECTION_CONDITIONS::ShowAlways );
    fileMenu->AddMenu( openRecentDrlMenu,          SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddItem( ID_GERBVIEW_LOAD_JOB_FILE, _( "Open Gerber &Job File..." ),
                       _( "Open a Gerber job file and its associated gerber files" ),
                       gerber_job_file_xpm,        SELECTION_CONDITIONS::ShowAlways );
    fileMenu->AddMenu( openRecentJobMenu,          SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddItem( ID_GERBVIEW_LOAD_ZIP_ARCHIVE_FILE, _( "Open &Zip Archive File..." ),
                       _( "Open a zipped archive (Gerber and Drill) file" ),
                       zip_xpm,                    SELECTION_CONDITIONS::ShowAlways );
    fileMenu->AddMenu( openRecentZipMenu,          SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddItem( ID_GERBVIEW_ERASE_ALL,  _( "Clear &All Layers" ),
                       _( "Clear all layers. All data will be deleted" ),
                       delete_gerber_xpm,          SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddItem( ID_GERBVIEW_RELOAD_ALL, _( "Reload All Layers" ),
                       _( "Reload all layers. All data will be reloaded" ),
                       reload2_xpm,                SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddItem( ID_GERBVIEW_EXPORT_TO_PCBNEW, _( "Export to Pcbnew..." ),
                       _( "Export data in Pcbnew format" ),
                       export_xpm,                SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddItem( ACTIONS::print,            SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddQuitOrClose( &Kiface(), _( "GerbView" ) );

    fileMenu->Resolve();

    //-- View menu -------------------------------------------------------
    //
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

    viewMenu->AddSeparator();
    viewMenu->AddItem( ACTIONS::zoomInCenter,                 SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomOutCenter,                SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomFitScreen,                SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomTool,                     SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomRedraw,                   SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddCheckItem( ACTIONS::toggleGrid,              gridShownCondition );
    viewMenu->AddCheckItem( ACTIONS::togglePolarCoords,       polarCoordsCondition );

    // Units submenu
    CONDITIONAL_MENU* unitsSubMenu = new CONDITIONAL_MENU( false, selTool );
    unitsSubMenu->SetTitle( _( "&Units" ) );
    unitsSubMenu->SetIcon( unit_mm_xpm );
    unitsSubMenu->AddCheckItem( ACTIONS::imperialUnits,       imperialUnitsCondition );
    unitsSubMenu->AddCheckItem( ACTIONS::metricUnits,         metricUnitsCondition );
    viewMenu->AddMenu( unitsSubMenu );

    viewMenu->AddSeparator();
    viewMenu->AddCheckItem( GERBVIEW_ACTIONS::flashedDisplayOutlines,  sketchFlashedCondition );
    viewMenu->AddCheckItem( GERBVIEW_ACTIONS::linesDisplayOutlines,    sketchLinesCondition );
    viewMenu->AddCheckItem( GERBVIEW_ACTIONS::polygonsDisplayOutlines, sketchPolygonsCondition );
    viewMenu->AddCheckItem( GERBVIEW_ACTIONS::dcodeDisplay,            showDcodes );
    viewMenu->AddCheckItem( GERBVIEW_ACTIONS::negativeObjectDisplay,   showNegativeObjects );
    viewMenu->AddCheckItem( GERBVIEW_ACTIONS::toggleDiffMode,          diffModeCondition );
    viewMenu->AddCheckItem( ACTIONS::highContrastMode,                 contrastModeCondition );

    viewMenu->Resolve();

    //-- Tools menu -------------------------------------------------------
    //
    ACTION_MENU* toolsMenu = new ACTION_MENU( false );

    toolsMenu->Add( _( "&List DCodes..." ), _( "List D-codes defined in Gerber files" ),
                    ID_GERBVIEW_SHOW_LIST_DCODES, show_dcodenumber_xpm );

    toolsMenu->Add( _( "&Show Source..." ), _( "Show source file for the current layer" ),
                    ID_GERBVIEW_SHOW_SOURCE, tools_xpm );

    toolsMenu->Add( ACTIONS::measureTool );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( _( "Clear Current Layer..." ), _( "Clear the selected graphic layer" ),
                    ID_GERBVIEW_ERASE_CURR_LAYER, delete_sheet_xpm );

    //-- Preferences menu -----------------------------------------------
    //
    auto acceleratedGraphicsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetCanvas()->GetBackend() == EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
    };
    auto standardGraphicsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetCanvas()->GetBackend() == EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;
    };

    CONDITIONAL_MENU* preferencesMenu = new CONDITIONAL_MENU( false, selTool );

    preferencesMenu->AddItem( wxID_PREFERENCES,
                              _( "Preferences...\tCTRL+," ),
                              _( "Show preferences for all open tools" ),
                              preference_xpm,                    SELECTION_CONDITIONS::ShowAlways );

    preferencesMenu->AddSeparator();
    AddMenuLanguageList( preferencesMenu, selTool );

    preferencesMenu->AddSeparator();
    preferencesMenu->AddCheckItem( ACTIONS::acceleratedGraphics, acceleratedGraphicsCondition );
    preferencesMenu->AddCheckItem( ACTIONS::standardGraphics,    standardGraphicsCondition );

    preferencesMenu->Resolve();

    //-- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( toolsMenu, _( "&Tools" ) );
    menuBar->Append( preferencesMenu, _( "&Preferences" ) );
    AddStandardHelpMenu( menuBar );

    // Associate the menu bar with the frame, if no previous menubar
    SetMenuBar( menuBar );
    delete oldMenuBar;
}
