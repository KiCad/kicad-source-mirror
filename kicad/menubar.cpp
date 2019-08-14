/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <hotkeys_basic.h>
#include <menus_helpers.h>
#include <tool/tool_manager.h>
#include <tool/action_toolbar.h>
#include <tools/kicad_manager_control.h>
#include <tools/kicad_manager_actions.h>
#include "kicad_manager_frame.h"
#include "pgm_kicad.h"
#include "kicad_id.h"


void KICAD_MANAGER_FRAME::ReCreateMenuBar()
{
    KICAD_MANAGER_CONTROL* controlTool = m_toolManager->GetTool<KICAD_MANAGER_CONTROL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar*  oldMenuBar = GetMenuBar();
    wxMenuBar*  menuBar = new wxMenuBar();

    //-- File menu -----------------------------------------------------------
    //
    CONDITIONAL_MENU*   fileMenu = new CONDITIONAL_MENU( false, controlTool );
    static ACTION_MENU* openRecentMenu;

    // Before deleting, remove the menus managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history)
    if( openRecentMenu )
        PgmTop().GetFileHistory().RemoveMenu( openRecentMenu );

    openRecentMenu = new ACTION_MENU( false );
    openRecentMenu->SetTool( controlTool );
    openRecentMenu->SetTitle( _( "Open Recent" ) );
    openRecentMenu->SetIcon( recent_xpm );

    PgmTop().GetFileHistory().UseMenu( openRecentMenu );
    PgmTop().GetFileHistory().AddFilesToMenu( openRecentMenu );

    fileMenu->AddItem( KICAD_MANAGER_ACTIONS::newProject,      SELECTION_CONDITIONS::ShowAlways );
    fileMenu->AddItem( KICAD_MANAGER_ACTIONS::newFromTemplate, SELECTION_CONDITIONS::ShowAlways );
    fileMenu->AddItem( KICAD_MANAGER_ACTIONS::openProject,     SELECTION_CONDITIONS::ShowAlways );
    fileMenu->AddMenu( openRecentMenu,                         SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddItem( ID_IMPORT_EAGLE_PROJECT,
                       _( "Import EAGLE Project..." ),
                       _( "Import EAGLE CAD XML schematic and board" ),
                       import_project_xpm,                     SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddItem( ID_SAVE_AND_ZIP_FILES,
                       _( "&Archive Project..." ),
                       _( "Archive all needed project files into zip archive" ),
                       zip_xpm,                                SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddItem( ID_READ_ZIP_ARCHIVE,
                       _( "&Unarchive Project..." ),
                       _( "Unarchive project files from zip archive" ),
                       unzip_xpm,                              SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddQuitOrClose( nullptr, _( "KiCad" ) );

    fileMenu->Resolve();

    //-- View menu -----------------------------------------------------------
    //
    CONDITIONAL_MENU* viewMenu = new CONDITIONAL_MENU( false, controlTool );

    viewMenu->AddItem( ACTIONS::zoomRedraw,                    SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddItem( KICAD_MANAGER_ACTIONS::openTextEditor,  SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ID_BROWSE_IN_FILE_EXPLORER,
                       _( "Browse Project Files" ), _( "Open project directory in file browser" ),
                       directory_browser_xpm,                  SELECTION_CONDITIONS::ShowAlways );

#ifdef __APPLE__
    viewMenu->AddSeparator();
#endif

    viewMenu->Resolve();

    //-- Tools menu -----------------------------------------------
    //
    CONDITIONAL_MENU* toolsMenu = new CONDITIONAL_MENU( false, controlTool );

    toolsMenu->AddItem( KICAD_MANAGER_ACTIONS::editSchematic,  SELECTION_CONDITIONS::ShowAlways );
    toolsMenu->AddItem( KICAD_MANAGER_ACTIONS::editSymbols,    SELECTION_CONDITIONS::ShowAlways );
    toolsMenu->AddItem( KICAD_MANAGER_ACTIONS::editPCB,        SELECTION_CONDITIONS::ShowAlways );
    toolsMenu->AddItem( KICAD_MANAGER_ACTIONS::editFootprints, SELECTION_CONDITIONS::ShowAlways );

    toolsMenu->AddSeparator();
    toolsMenu->AddItem( KICAD_MANAGER_ACTIONS::viewGerbers,    SELECTION_CONDITIONS::ShowAlways );
    toolsMenu->AddItem( KICAD_MANAGER_ACTIONS::convertImage,   SELECTION_CONDITIONS::ShowAlways );
    toolsMenu->AddItem( KICAD_MANAGER_ACTIONS::showCalculator, SELECTION_CONDITIONS::ShowAlways );
    toolsMenu->AddItem( KICAD_MANAGER_ACTIONS::editWorksheet,  SELECTION_CONDITIONS::ShowAlways );

    toolsMenu->AddSeparator();
    toolsMenu->AddItem( ID_EDIT_LOCAL_FILE_IN_TEXT_EDITOR,
                       _( "Edit Local File..." ), _( "Edit local file in text editor" ),
                       browse_files_xpm,                       SELECTION_CONDITIONS::ShowAlways );

    toolsMenu->Resolve();

    //-- Preferences menu -----------------------------------------------
    //
    CONDITIONAL_MENU* prefsMenu = new CONDITIONAL_MENU( false, controlTool );

    prefsMenu->AddItem( ACTIONS::configurePaths,        SELECTION_CONDITIONS::ShowAlways );
    prefsMenu->AddItem( ACTIONS::showSymbolLibTable,    SELECTION_CONDITIONS::ShowAlways );
    prefsMenu->AddItem( ACTIONS::showFootprintLibTable, SELECTION_CONDITIONS::ShowAlways );
    prefsMenu->AddItem( wxID_PREFERENCES,
                        _( "Preferences...\tCTRL+," ),
                        _( "Show preferences for all open tools" ),
                        preference_xpm,                 SELECTION_CONDITIONS::ShowAlways );

    prefsMenu->AddSeparator();
    AddMenuLanguageList( prefsMenu, controlTool );

    prefsMenu->Resolve();

    //-- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
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
        m_mainToolBar->Clear();
    else
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // New
    m_mainToolBar->Add( KICAD_MANAGER_ACTIONS::newProject );
    m_mainToolBar->Add( KICAD_MANAGER_ACTIONS::newFromTemplate );
    m_mainToolBar->Add( KICAD_MANAGER_ACTIONS::openProject );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_SAVE_AND_ZIP_FILES, wxEmptyString,
                            KiScaledBitmap( zip_xpm, this ),
                            _( "Archive all project files" ) );

    m_mainToolBar->AddTool( ID_READ_ZIP_ARCHIVE, wxEmptyString,
                            KiScaledBitmap( unzip_xpm, this ),
                            _( "Unarchive project files from zip archive" ) );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->Add( ACTIONS::zoomRedraw );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_BROWSE_IN_FILE_EXPLORER, wxEmptyString,
                            KiScaledBitmap( directory_browser_xpm, this ),
#ifdef __APPLE__
                            _( "Reveal project directory in Finder" ) );
#else
                            _( "Open project directory in file explorer" ) );
#endif

    // Create m_mainToolBar
    m_mainToolBar->Realize();
}


void KICAD_MANAGER_FRAME::RecreateLauncher()
{
    if( m_launcher )
        m_launcher->Clear();
    else
        m_launcher = new ACTION_TOOLBAR( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                         KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // Add tools. Note these KICAD_MANAGER_ACTIONS are defined with a bitmap
    // suitable for menus. The icans will be changed later.
    m_launcher->Add( KICAD_MANAGER_ACTIONS::editSchematic );
    m_launcher->Add( KICAD_MANAGER_ACTIONS::editSymbols );

    KiScaledSeparator( m_launcher, this );
    m_launcher->Add( KICAD_MANAGER_ACTIONS::editPCB );
    m_launcher->Add( KICAD_MANAGER_ACTIONS::editFootprints );

    KiScaledSeparator( m_launcher, this );
    m_launcher->Add( KICAD_MANAGER_ACTIONS::viewGerbers );
    m_launcher->Add( KICAD_MANAGER_ACTIONS::convertImage );
    m_launcher->Add( KICAD_MANAGER_ACTIONS::showCalculator );
    m_launcher->Add( KICAD_MANAGER_ACTIONS::editWorksheet );

    // Now set big icons for these tools:
    m_launcher->SetToolBitmap( KICAD_MANAGER_ACTIONS::editSchematic,
                               KiScaledBitmap( icon_eeschema_xpm, this ) );
    m_launcher->SetToolBitmap( KICAD_MANAGER_ACTIONS::editSymbols,
                               KiScaledBitmap( icon_libedit_xpm, this ) );
    m_launcher->SetToolBitmap( KICAD_MANAGER_ACTIONS::editPCB,
                               KiScaledBitmap( icon_pcbnew_xpm, this ) );
    m_launcher->SetToolBitmap( KICAD_MANAGER_ACTIONS::editFootprints,
                               KiScaledBitmap( icon_modedit_xpm, this ) );
    m_launcher->SetToolBitmap( KICAD_MANAGER_ACTIONS::viewGerbers,
                               KiScaledBitmap( icon_gerbview_xpm, this ) );
    m_launcher->SetToolBitmap( KICAD_MANAGER_ACTIONS::convertImage,
                               KiScaledBitmap( icon_bitmap2component_xpm, this ) );
    m_launcher->SetToolBitmap( KICAD_MANAGER_ACTIONS::showCalculator,
                               KiScaledBitmap( icon_pcbcalculator_xpm, this ) );
    m_launcher->SetToolBitmap( KICAD_MANAGER_ACTIONS::editWorksheet,
                               KiScaledBitmap( icon_pagelayout_editor_xpm, this ) );

    // Create mlauncher
    m_launcher->Realize();

    // And update the visual tools state:
    SyncToolbars();
}


void KICAD_MANAGER_FRAME::SyncToolbars()
{
    m_launcher->Toggle( KICAD_MANAGER_ACTIONS::editSchematic,  m_active_project );
    m_launcher->Toggle( KICAD_MANAGER_ACTIONS::editSymbols,    m_active_project );
    m_launcher->Toggle( KICAD_MANAGER_ACTIONS::editPCB,        m_active_project );
    m_launcher->Toggle( KICAD_MANAGER_ACTIONS::editFootprints, m_active_project );
    m_launcher->Refresh();
}


