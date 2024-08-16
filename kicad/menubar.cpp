/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2019 CERN
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

#include <advanced_config.h>
#include <bitmaps.h>
#include <file_history.h>
#include <kiplatform/policy.h>
#include <paths.h>
#include <policy_keys.h>
#include <tool/action_manager.h>
#include <tool/action_toolbar.h>
#include <tool/tool_manager.h>
#include <tools/kicad_manager_control.h>
#include <tools/kicad_manager_actions.h>
#include "kicad_manager_frame.h"
#include "pgm_kicad.h"
#include "kicad_id.h"
#include <widgets/wx_menubar.h>
#include <wx/dir.h>


void KICAD_MANAGER_FRAME::doReCreateMenuBar()
{
    KICAD_MANAGER_CONTROL* controlTool = m_toolManager->GetTool<KICAD_MANAGER_CONTROL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar*  oldMenuBar = GetMenuBar();
    WX_MENUBAR* menuBar    = new WX_MENUBAR();

    //-- File menu -----------------------------------------------------------
    //
    ACTION_MENU*  fileMenu    = new ACTION_MENU( false, controlTool );
    FILE_HISTORY& fileHistory = GetFileHistory();

    fileHistory.SetClearText( _( "Clear Recent Projects" ) );

    static ACTION_MENU* openRecentMenu;

    // Create the menu if it does not exist. Adding a file to/from the history
    // will automatically refresh the menu.
    if( !openRecentMenu )
    {
        openRecentMenu = new ACTION_MENU( false, controlTool );
        openRecentMenu->SetIcon( BITMAPS::recent );

        fileHistory.UseMenu( openRecentMenu );
        fileHistory.AddFilesToMenu();
    }

    // Ensure the title is up to date after changing language
    openRecentMenu->SetTitle( _( "Open Recent" ) );

    fileMenu->Add( KICAD_MANAGER_ACTIONS::newProject );
    fileMenu->Add( KICAD_MANAGER_ACTIONS::newFromTemplate );

    if( ADVANCED_CFG::GetCfg().m_EnableGit )
    {
        fileMenu->Add( KICAD_MANAGER_ACTIONS::newFromRepository );
    }

    if( wxDir::Exists( PATHS::GetStockDemosPath() ) )
    {
        fileMenu->Add( KICAD_MANAGER_ACTIONS::openDemoProject );
    }

    fileMenu->Add( KICAD_MANAGER_ACTIONS::openProject );

    wxMenuItem* item = fileMenu->Add( openRecentMenu->Clone() );

    // Add the file menu condition here since it needs the item ID for the submenu
    ACTION_CONDITIONS cond;
    cond.Enable( FILE_HISTORY::FileHistoryNotEmpty( fileHistory ) );
    RegisterUIUpdateHandler( item->GetId(), cond );

    fileMenu->AppendSeparator();
    fileMenu->Add( KICAD_MANAGER_ACTIONS::closeProject );

    fileMenu->AppendSeparator();
    fileMenu->Add( ACTIONS::saveAs );

    fileMenu->AppendSeparator();

    //Import Sub-menu
    ACTION_MENU* importMenu = new ACTION_MENU( false, controlTool );
    importMenu->SetTitle( _( "Import Non-KiCad Project..." ) );
    importMenu->SetIcon( BITMAPS::import_project );

    importMenu->Add( _( "CADSTAR Project..." ),
                     _( "Import CADSTAR Archive Schematic and PCB (*.csa, *.cpa)" ),
                     ID_IMPORT_CADSTAR_ARCHIVE_PROJECT,
                     BITMAPS::import_project );

    importMenu->Add( _( "EAGLE Project..." ),
                     _( "Import EAGLE CAD XML schematic and board" ),
                     ID_IMPORT_EAGLE_PROJECT,
                     BITMAPS::import_project );

    importMenu->Add( _( "EasyEDA (JLCEDA) Std Backup..." ),
                     _( "Import EasyEDA (JLCEDA) Standard schematic and board" ),
                     ID_IMPORT_EASYEDA_PROJECT,
                     BITMAPS::import_project );

    importMenu->Add( _( "EasyEDA (JLCEDA) Pro Project..." ),
                     _( "Import EasyEDA (JLCEDA) Professional schematic and board" ),
                     ID_IMPORT_EASYEDAPRO_PROJECT, BITMAPS::import_project );

    fileMenu->Add( importMenu );

    fileMenu->AppendSeparator();
    fileMenu->Add( _( "&Archive Project..." ),
                   _( "Archive all needed project files into zip archive" ),
                   ID_SAVE_AND_ZIP_FILES,
                   BITMAPS::zip );

    fileMenu->Add( _( "&Unarchive Project..." ),
                   _( "Unarchive project files from zip archive" ),
                   ID_READ_ZIP_ARCHIVE,
                   BITMAPS::unzip );

    fileMenu->AppendSeparator();
    fileMenu->AddQuitOrClose( nullptr, wxS( "KiCad" ) );

    //-- Edit menu -----------------------------------------------------------
    //

    ACTION_MENU* editMenu = new ACTION_MENU( false, controlTool );

    /*
     * While we don't presently use these, they need to be here so that cut/copy/paste work
     * in things like search boxes in file open dialogs.
     */
    editMenu->Add( ACTIONS::cut );
    editMenu->Add( ACTIONS::copy );
    editMenu->Add( ACTIONS::paste );

    //-- View menu -----------------------------------------------------------
    //
    ACTION_MENU* viewMenu = new ACTION_MENU( false, controlTool );

    viewMenu->Add( ACTIONS::zoomRedraw );

    viewMenu->AppendSeparator();
    viewMenu->Add( KICAD_MANAGER_ACTIONS::openTextEditor );
    viewMenu->Add( _( "Browse Project Files" ),
                   _( "Open project directory in file browser" ),
                   ID_BROWSE_IN_FILE_EXPLORER,
                   BITMAPS::directory_browser );

#ifdef __APPLE__
    // Add a separator only on macOS because the OS adds menu items to the view menu after ours
    viewMenu->AppendSeparator();
#endif

    //-- Tools menu -----------------------------------------------
    //
    ACTION_MENU* toolsMenu = new ACTION_MENU( false, controlTool );

    toolsMenu->Add( KICAD_MANAGER_ACTIONS::editSchematic );
    toolsMenu->Add( KICAD_MANAGER_ACTIONS::editSymbols );
    toolsMenu->Add( KICAD_MANAGER_ACTIONS::editPCB );
    toolsMenu->Add( KICAD_MANAGER_ACTIONS::editFootprints );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( KICAD_MANAGER_ACTIONS::viewGerbers );
    toolsMenu->Add( KICAD_MANAGER_ACTIONS::convertImage );
    toolsMenu->Add( KICAD_MANAGER_ACTIONS::showCalculator );
    toolsMenu->Add( KICAD_MANAGER_ACTIONS::editDrawingSheet );

    wxMenuItem* pcmMenuItem = toolsMenu->Add( KICAD_MANAGER_ACTIONS::showPluginManager );

    if( KIPLATFORM::POLICY::GetPolicyBool( POLICY_KEY_PCM )
        == KIPLATFORM::POLICY::PBOOL::DISABLED )
    {
        pcmMenuItem->Enable( false );
    }

    toolsMenu->AppendSeparator();
    toolsMenu->Add( _( "Edit Local File..." ),
                    _( "Edit local file in text editor" ),
                    ID_EDIT_LOCAL_FILE_IN_TEXT_EDITOR,
                    BITMAPS::editor );

    //-- Preferences menu -----------------------------------------------
    //
    ACTION_MENU* prefsMenu = new ACTION_MENU( false, controlTool );

    prefsMenu->Add( ACTIONS::configurePaths );
    prefsMenu->Add( ACTIONS::showSymbolLibTable );
    prefsMenu->Add( ACTIONS::showFootprintLibTable );
    prefsMenu->Add( ACTIONS::openPreferences );

    prefsMenu->AppendSeparator();
    AddMenuLanguageList( prefsMenu, controlTool );


    //-- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu,  _( "&File" ) );
    menuBar->Append( editMenu,  _( "&Edit" ) );
    menuBar->Append( viewMenu,  _( "&View" ) );
    menuBar->Append( toolsMenu, _( "&Tools" ) );
    menuBar->Append( prefsMenu, _( "&Preferences" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}


/**
 * @brief (Re)Create the horizontal toolbar
 */
void KICAD_MANAGER_FRAME::RecreateBaseHToolbar()
{
    if( m_mainToolBar )
    {
        m_mainToolBar->ClearToolbar();
    }
    else
    {
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );
        m_mainToolBar->SetAuiManager( &m_auimgr );
    }

    // New
    m_mainToolBar->Add( KICAD_MANAGER_ACTIONS::newProject );
    m_mainToolBar->Add( KICAD_MANAGER_ACTIONS::openProject );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->AddTool( ID_SAVE_AND_ZIP_FILES, wxEmptyString,
                            KiBitmapBundle( BITMAPS::zip ),
                            KiDisabledBitmapBundle( BITMAPS::zip ),
                            wxITEM_NORMAL,
                            _( "Archive all project files" ),
                            wxEmptyString, nullptr );

    m_mainToolBar->AddTool( ID_READ_ZIP_ARCHIVE, wxEmptyString,
                            KiBitmapBundle( BITMAPS::unzip ),
                            KiDisabledBitmapBundle( BITMAPS::unzip ),
                            wxITEM_NORMAL,
                            _( "Unarchive project files from zip archive" ),
                            wxEmptyString, nullptr );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::zoomRedraw );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->AddTool( ID_BROWSE_IN_FILE_EXPLORER, wxEmptyString,
                            KiBitmapBundle( BITMAPS::directory_browser ),
#ifdef __APPLE__
                            _( "Reveal project folder in Finder" ) );
#else
                            _( "Open project directory in file explorer" ) );
#endif

    // Create m_mainToolBar
    m_mainToolBar->KiRealize();
}
