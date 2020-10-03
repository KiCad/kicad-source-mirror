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

#include "gerbview_id.h"
#include <kiface_i.h>
#include <menus_helpers.h>
#include <pgm_base.h>
#include <tool/action_menu.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <tools/gerbview_actions.h>
#include <tools/gerbview_selection_tool.h>
#include <widgets/wx_menubar.h>


void GERBVIEW_FRAME::ReCreateMenuBar()
{
    GERBVIEW_SELECTION_TOOL* selTool = m_toolManager->GetTool<GERBVIEW_SELECTION_TOOL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar*  oldMenuBar = GetMenuBar();
    WX_MENUBAR* menuBar    = new WX_MENUBAR();

    //-- File menu -------------------------------------------------------
    //
    ACTION_MENU*   fileMenu = new ACTION_MENU( false, selTool );
    static ACTION_MENU* openRecentGbrMenu;
    static ACTION_MENU* openRecentDrlMenu;
    static ACTION_MENU* openRecentJobMenu;
    static ACTION_MENU* openRecentZipMenu;

    FILE_HISTORY& recentGbrFiles = GetFileHistory();

#define FileHistoryCond( x ) ACTION_CONDITIONS().Enable( FILE_HISTORY::FileHistoryNotEmpty( x ) )


    // Create the gerber file menu if it does not exist. Adding a file to/from the history
    // will automatically refresh the menu.
    if( !openRecentGbrMenu )
    {
        openRecentGbrMenu = new ACTION_MENU( false );
        openRecentGbrMenu->SetTool( selTool );
        openRecentGbrMenu->SetTitle( _( "Open Recent Gerber File" ) );
        openRecentGbrMenu->SetIcon( recent_xpm );

        recentGbrFiles.UseMenu( openRecentGbrMenu );
        recentGbrFiles.SetClearText( _( "Clear Recent Gerber Files" ) );
        recentGbrFiles.AddFilesToMenu();
    }

    fileMenu->Add( GERBVIEW_ACTIONS::openGerber );
    wxMenuItem* gbrItem = fileMenu->Add( openRecentGbrMenu );
    RegisterUIUpdateHandler( gbrItem->GetId(), FileHistoryCond( recentGbrFiles) );


    // Create the drill file menu if it does not exist. Adding a file to/from the history
    // will automatically refresh the menu.
    if( !openRecentDrlMenu )
    {
        openRecentDrlMenu = new ACTION_MENU( false );
        openRecentDrlMenu->SetTool( selTool );
        openRecentDrlMenu->SetTitle( _( "Open Recent Drill File" ) );
        openRecentDrlMenu->SetIcon( recent_xpm );

        m_drillFileHistory.UseMenu( openRecentDrlMenu );
        m_drillFileHistory.SetClearText( _( "Clear Recent Drill Files" ) );
        m_drillFileHistory.AddFilesToMenu();
    }

    fileMenu->Add( GERBVIEW_ACTIONS::openDrillFile );
    wxMenuItem* drillItem = fileMenu->Add( openRecentDrlMenu );
    RegisterUIUpdateHandler( drillItem->GetId(), FileHistoryCond( m_drillFileHistory ) );


    // Create the job file menu if it does not exist. Adding a file to/from the history
    // will automatically refresh the menu.
    if( !openRecentJobMenu )
    {
        openRecentJobMenu = new ACTION_MENU( false );
        openRecentJobMenu->SetTool( selTool );
        openRecentJobMenu->SetTitle( _( "Open Recent Job File" ) );
        openRecentJobMenu->SetIcon( recent_xpm );

        m_jobFileHistory.UseMenu( openRecentJobMenu );
        m_jobFileHistory.SetClearText( _( "Clear Recent Job Files" ) );
        m_jobFileHistory.AddFilesToMenu();
    }

    fileMenu->Add( GERBVIEW_ACTIONS::openJobFile );
    wxMenuItem* jobItem = fileMenu->Add( openRecentJobMenu );
    RegisterUIUpdateHandler( jobItem->GetId(), FileHistoryCond( m_jobFileHistory ) );


    // Create the zip file menu if it does not exist. Adding a file to/from the history
    // will automatically refresh the menu.
    if( !openRecentZipMenu )
    {
        openRecentZipMenu = new ACTION_MENU( false );
        openRecentZipMenu->SetTool( selTool );
        openRecentZipMenu->SetTitle( _( "Open Recent Zip File" ) );
        openRecentZipMenu->SetIcon( recent_xpm );

        m_zipFileHistory.UseMenu( openRecentZipMenu );
        m_zipFileHistory.SetClearText( _( "Clear Recent Zip Files" ) );
        m_zipFileHistory.AddFilesToMenu();
    }

    fileMenu->Add( GERBVIEW_ACTIONS::openZipFile );
    wxMenuItem* zipItem = fileMenu->Add( openRecentZipMenu );
    RegisterUIUpdateHandler( zipItem->GetId(), FileHistoryCond( m_zipFileHistory ) );

#undef FileHistoryCond

    fileMenu->AppendSeparator();
    fileMenu->Add( _( "Clear &All Layers" ),
                   _( "Clear all layers. All data will be deleted" ),
                   ID_GERBVIEW_ERASE_ALL,
                   delete_gerber_xpm );

    fileMenu->Add( _( "Reload All Layers" ),
                   _( "Reload all layers. All data will be reloaded" ),
                   ID_GERBVIEW_RELOAD_ALL,
                   reload2_xpm );

    fileMenu->AppendSeparator();
    fileMenu->Add( _( "Export to Pcbnew..." ),
                   _( "Export data in Pcbnew format" ),
                   ID_GERBVIEW_EXPORT_TO_PCBNEW,
                   export_xpm );

    fileMenu->AppendSeparator();
    fileMenu->Add( ACTIONS::print );

    fileMenu->AppendSeparator();
    fileMenu->AddQuitOrClose( &Kiface(), _( "GerbView" ) );


    //-- View menu -------------------------------------------------------
    //
    ACTION_MENU* viewMenu = new ACTION_MENU( false, selTool );

    // Hide layer manager
    viewMenu->Add( _( "Show &Layers Manager" ),
                   _( "Show or hide the layer manager" ),
                   ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR,
                   layers_manager_xpm );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::zoomInCenter );
    viewMenu->Add( ACTIONS::zoomOutCenter );
    viewMenu->Add( ACTIONS::zoomFitScreen );
    viewMenu->Add( ACTIONS::zoomTool );
    viewMenu->Add( ACTIONS::zoomRedraw );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::toggleGrid,                       ACTION_MENU::CHECK );
    viewMenu->Add( ACTIONS::togglePolarCoords,                ACTION_MENU::CHECK );

#ifdef __APPLE__
    // Add a separator only on macOS because the OS adds menu items to the view menu after ours
    viewMenu->AppendSeparator();
#endif

    // Units submenu
    ACTION_MENU* unitsSubMenu = new ACTION_MENU( false, selTool );

    unitsSubMenu->SetTitle( _( "&Units" ) );
    unitsSubMenu->SetIcon( unit_mm_xpm );
    unitsSubMenu->Add( ACTIONS::inchesUnits,                  ACTION_MENU::CHECK );
    unitsSubMenu->Add( ACTIONS::milsUnits,                    ACTION_MENU::CHECK );
    unitsSubMenu->Add( ACTIONS::millimetersUnits,             ACTION_MENU::CHECK );

    viewMenu->Add( unitsSubMenu );

    viewMenu->AppendSeparator();
    viewMenu->Add( GERBVIEW_ACTIONS::flashedDisplayOutlines,  ACTION_MENU::CHECK );
    viewMenu->Add( GERBVIEW_ACTIONS::linesDisplayOutlines,    ACTION_MENU::CHECK );
    viewMenu->Add( GERBVIEW_ACTIONS::polygonsDisplayOutlines, ACTION_MENU::CHECK );
    viewMenu->Add( GERBVIEW_ACTIONS::dcodeDisplay,            ACTION_MENU::CHECK );
    viewMenu->Add( GERBVIEW_ACTIONS::negativeObjectDisplay,   ACTION_MENU::CHECK );
    viewMenu->Add( GERBVIEW_ACTIONS::toggleDiffMode,          ACTION_MENU::CHECK );
    viewMenu->Add( ACTIONS::highContrastMode,                 ACTION_MENU::CHECK );
    viewMenu->Add( GERBVIEW_ACTIONS::flipGerberView,          ACTION_MENU::CHECK );


    //-- Tools menu -------------------------------------------------------
    //
    ACTION_MENU* toolsMenu = new ACTION_MENU( false );

    toolsMenu->Add( _( "&List DCodes..." ),
                    _( "List D-codes defined in Gerber files" ),
                    ID_GERBVIEW_SHOW_LIST_DCODES,
                    show_dcodenumber_xpm );

    toolsMenu->Add( _( "&Show Source..." ),
                    _( "Show source file for the current layer" ),
                    ID_GERBVIEW_SHOW_SOURCE,
                    tools_xpm );

    toolsMenu->Add( ACTIONS::measureTool );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( _( "Clear Current Layer..." ),
                    _( "Clear the selected graphic layer" ),
                    ID_GERBVIEW_ERASE_CURR_LAYER,
                    delete_sheet_xpm );

    //-- Preferences menu -----------------------------------------------
    //
    ACTION_MENU* preferencesMenu = new ACTION_MENU( false, selTool );

    preferencesMenu->Add( _( "Preferences...\tCTRL+," ),
                          _( "Show preferences for all open tools" ),
                          wxID_PREFERENCES,
                          preference_xpm );

    preferencesMenu->AppendSeparator();
    AddMenuLanguageList( preferencesMenu, selTool );

#ifndef __WXMAC__
    preferencesMenu->AppendSeparator();
    preferencesMenu->Add( ACTIONS::acceleratedGraphics, ACTION_MENU::CHECK );
    preferencesMenu->Add( ACTIONS::standardGraphics,    ACTION_MENU::CHECK );
#endif


    //-- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu,        _( "&File" ) );
    menuBar->Append( viewMenu,        _( "&View" ) );
    menuBar->Append( toolsMenu,       _( "&Tools" ) );
    menuBar->Append( preferencesMenu, _( "&Preferences" ) );
    AddStandardHelpMenu( menuBar );

    // Associate the menu bar with the frame, if no previous menubar
    SetMenuBar( menuBar );
    delete oldMenuBar;
}
