/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009-2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file kicad/menubar.cpp
 * @brief (Re)Create the project manager menubar for KiCad
 */
#include <fctsys.h>
#include <pgm_kicad.h>
#include <kicad.h>
#include <menus_helpers.h>
#include <tree_project_frame.h>

// Menubar and toolbar event table
BEGIN_EVENT_TABLE( KICAD_MANAGER_FRAME, EDA_BASE_FRAME )
    // Window events
    EVT_SIZE( KICAD_MANAGER_FRAME::OnSize )
    EVT_CLOSE( KICAD_MANAGER_FRAME::OnCloseWindow )

    // Toolbar events
    EVT_TOOL( ID_NEW_PROJECT, KICAD_MANAGER_FRAME::OnLoadProject )
    EVT_TOOL( ID_NEW_PROJECT_FROM_TEMPLATE, KICAD_MANAGER_FRAME::OnLoadProject )
    EVT_TOOL( ID_LOAD_PROJECT, KICAD_MANAGER_FRAME::OnLoadProject )
    EVT_TOOL( ID_SAVE_PROJECT, KICAD_MANAGER_FRAME::OnSaveProject )
    EVT_TOOL( ID_SAVE_AND_ZIP_FILES, KICAD_MANAGER_FRAME::OnArchiveFiles )

    // Menu events
    EVT_MENU( ID_SAVE_PROJECT, KICAD_MANAGER_FRAME::OnSaveProject )
    EVT_MENU( wxID_EXIT, KICAD_MANAGER_FRAME::OnExit )
    EVT_MENU( ID_TO_TEXT_EDITOR, KICAD_MANAGER_FRAME::OnOpenTextEditor )
    EVT_MENU( ID_BROWSE_AN_SELECT_FILE, KICAD_MANAGER_FRAME::OnOpenFileInTextEditor )
    EVT_MENU( ID_SELECT_PREFERED_EDITOR, EDA_BASE_FRAME::OnSelectPreferredEditor )
    EVT_MENU( ID_SELECT_DEFAULT_PDF_BROWSER, KICAD_MANAGER_FRAME::OnSelectDefaultPdfBrowser )
    EVT_MENU( ID_SELECT_PREFERED_PDF_BROWSER, KICAD_MANAGER_FRAME::OnSelectPreferredPdfBrowser )
    EVT_MENU( ID_SELECT_PREFERED_PDF_BROWSER_NAME,
              KICAD_MANAGER_FRAME::OnSelectPreferredPdfBrowser )
    EVT_MENU( ID_SAVE_AND_ZIP_FILES, KICAD_MANAGER_FRAME::OnArchiveFiles )
    EVT_MENU( ID_READ_ZIP_ARCHIVE, KICAD_MANAGER_FRAME::OnUnarchiveFiles )
    EVT_MENU( ID_PROJECT_TREE_REFRESH, KICAD_MANAGER_FRAME::OnRefresh )
    EVT_MENU( wxID_HELP, KICAD_MANAGER_FRAME::GetKicadHelp )
    EVT_MENU( wxID_INDEX, KICAD_MANAGER_FRAME::GetKicadHelp )
    EVT_MENU( wxID_ABOUT, KICAD_MANAGER_FRAME::GetKicadAbout )

    // Range menu events
    EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END, KICAD_MANAGER_FRAME::language_change )

    EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, KICAD_MANAGER_FRAME::OnFileHistory )

    // Special functions
#ifdef KICAD_USE_FILES_WATCHER
    EVT_MENU( ID_INIT_WATCHED_PATHS, KICAD_MANAGER_FRAME::OnChangeWatchedPaths )
#endif

    // Button events
    EVT_BUTTON( ID_TO_SCH, KICAD_MANAGER_FRAME::OnRunEeschema )
    EVT_BUTTON( ID_TO_SCH_LIB_EDITOR, KICAD_MANAGER_FRAME::OnRunSchLibEditor )

    EVT_BUTTON( ID_TO_CVPCB, KICAD_MANAGER_FRAME::OnRunCvpcb )

    EVT_BUTTON( ID_TO_PCB, KICAD_MANAGER_FRAME::OnRunPcbNew )
    EVT_BUTTON( ID_TO_PCB_FP_EDITOR, KICAD_MANAGER_FRAME::OnRunPcbFpEditor )

    EVT_BUTTON( ID_TO_GERBVIEW, KICAD_MANAGER_FRAME::OnRunGerbview )
    EVT_BUTTON( ID_TO_BITMAP_CONVERTER, KICAD_MANAGER_FRAME::OnRunBitmapConverter )
    EVT_BUTTON( ID_TO_PCB_CALCULATOR, KICAD_MANAGER_FRAME::OnRunPcbCalculator )
    EVT_BUTTON( ID_TO_PL_EDITOR, KICAD_MANAGER_FRAME::OnRunPageLayoutEditor )

    EVT_UPDATE_UI( ID_SELECT_DEFAULT_PDF_BROWSER, KICAD_MANAGER_FRAME::OnUpdateDefaultPdfBrowser )
    EVT_UPDATE_UI( ID_SELECT_PREFERED_PDF_BROWSER,
                   KICAD_MANAGER_FRAME::OnUpdatePreferredPdfBrowser )

END_EVENT_TABLE()


/**
 * @brief (Re)Create the menubar
 */
void KICAD_MANAGER_FRAME::ReCreateMenuBar()
{
    static wxMenu* openRecentMenu;  // Open Recent submenu,
                                    // static to remember this menu

    // Create and try to get the current  menubar
    wxMenuBar*  menuBar = GetMenuBar();

    if( !menuBar )
        menuBar = new wxMenuBar();

    // Delete all existing menus so they can be rebuilt.
    // This allows language changes of the menu text on the fly.
    menuBar->Freeze();

    // Before deleting, remove the menus managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history)
    if( openRecentMenu )
        Pgm().GetFileHistory().RemoveMenu( openRecentMenu );

    // Delete all existing menus
    while( menuBar->GetMenuCount() )
        delete menuBar->Remove( 0 );

    // Recreate all menus:

    // Menu File:
    wxMenu* fileMenu = new wxMenu;

    // Open
    AddMenuItem( fileMenu,
                 ID_LOAD_PROJECT,
                 _( "&Open Project\tCtrl+O" ),
                 _( "Open existing project" ),
                 KiBitmap( open_project_xpm ) );

    // File history
    openRecentMenu = new wxMenu();
    Pgm().GetFileHistory().UseMenu( openRecentMenu );
    Pgm().GetFileHistory().AddFilesToMenu( );
    AddMenuItem( fileMenu, openRecentMenu,
                 wxID_ANY,
                 _( "Open &Recent" ),
                 _( "Open recent schematic project" ),
                 KiBitmap( open_project_xpm ) );

    // New
    wxMenu* newMenu = new wxMenu();
    AddMenuItem( newMenu, ID_NEW_PROJECT,
                 _( "&Blank Project\tCtrl+N" ),
                 _( "Create blank project" ),
                 KiBitmap( new_project_xpm ) );

    AddMenuItem( newMenu, ID_NEW_PROJECT_FROM_TEMPLATE,
                 _( "Project from &Template\tCtrl+T" ),
                 _( "Create new project from template" ),
                 KiBitmap( new_project_with_template_xpm ) );

    AddMenuItem( fileMenu, newMenu,
                 wxID_ANY,
                 _( "New" ),
                 _( "Create new project" ),
                 KiBitmap( new_project_xpm ) );

    // Save
    AddMenuItem( fileMenu,
                 ID_SAVE_PROJECT,
                 _( "&Save\tCtrl+S" ),
                 _( "Save current project" ),
                 KiBitmap( save_project_xpm ) );

    // Archive
    fileMenu->AppendSeparator();
    AddMenuItem( fileMenu,
                 ID_SAVE_AND_ZIP_FILES,
                 _( "&Archive" ),
                 _( "Archive project files in zip archive" ),
                 KiBitmap( zip_xpm ) );

    // Unarchive
    AddMenuItem( fileMenu,
                 ID_READ_ZIP_ARCHIVE,
                 _( "&Unarchive" ),
                 _( "Unarchive project files from zip file" ),
                 KiBitmap( unzip_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Quit
    AddMenuItem( fileMenu,
                 wxID_EXIT,
                 _( "&Quit" ),
                 _( "Quit KiCad" ),
                 KiBitmap( exit_xpm ) );

    // Menu Browse:
    wxMenu* browseMenu = new wxMenu();

    // Text editor
    AddMenuItem( browseMenu,
                 ID_TO_TEXT_EDITOR,
                 _( "Open Text E&ditor" ),
                 _( "Launch preferred text editor" ),
                 KiBitmap( editor_xpm ) );

    // View file
    AddMenuItem( browseMenu,
                 ID_BROWSE_AN_SELECT_FILE,
                 _( "&Open Local File" ),
                 _( "Edit local file" ),
                 KiBitmap( browse_files_xpm ) );

    // Menu Preferences:
    wxMenu* preferencesMenu = new wxMenu;

    // Text editor
    AddMenuItem( preferencesMenu,
                 ID_SELECT_PREFERED_EDITOR,
                 _( "&Set Text Editor" ),
                 _( "Set your preferred text editor" ),
                 KiBitmap( editor_xpm ) );

    // PDF Viewer submenu:System browser or user defined checkbox
    wxMenu* SubMenuPdfBrowserChoice = new wxMenu;

    // Default
    AddMenuItem( SubMenuPdfBrowserChoice, ID_SELECT_DEFAULT_PDF_BROWSER,
                  _( "System &Default PDF Viewer" ),
                  _( "Use system default PDF viewer" ),
                   KiBitmap( datasheet_xpm ),
                  wxITEM_CHECK );
    SubMenuPdfBrowserChoice->Check( ID_SELECT_DEFAULT_PDF_BROWSER,
                                    Pgm().UseSystemPdfBrowser() );

    // Favourite
    AddMenuItem( SubMenuPdfBrowserChoice, ID_SELECT_PREFERED_PDF_BROWSER,
                  _( "&Favourite PDF Viewer" ),
                  _( "Use favourite PDF viewer" ),
                   KiBitmap( datasheet_xpm ),
                  wxITEM_CHECK );
    SubMenuPdfBrowserChoice->Check( ID_SELECT_PREFERED_PDF_BROWSER,
                                    !Pgm().UseSystemPdfBrowser() );

    SubMenuPdfBrowserChoice->AppendSeparator();
    // Append PDF Viewer submenu to preferences
    AddMenuItem( SubMenuPdfBrowserChoice,
                 ID_SELECT_PREFERED_PDF_BROWSER_NAME,
                 _( "Set &PDF Viewer" ),
                 _( "Set favourite PDF viewer" ),
                 KiBitmap( datasheet_xpm ) );

    // PDF viewer submenu
    AddMenuItem( preferencesMenu, SubMenuPdfBrowserChoice, -1,
                 _( "&PDF Viewer" ),
                 _( "PDF viewer preferences" ),
                 KiBitmap( datasheet_xpm ) );

    // Language submenu
    preferencesMenu->AppendSeparator();
    Pgm().AddMenuLanguageList( preferencesMenu );

    // Menu Help:
    wxMenu* helpMenu = new wxMenu;

    // Version info
    AddHelpVersionInfoMenuEntry( helpMenu );

    // Contents
    AddMenuItem( helpMenu, wxID_HELP,
                 _( "KiCad Manual" ),
                 _( "Open KiCad user manual" ),
                 KiBitmap( online_help_xpm ) );

    AddMenuItem( helpMenu, wxID_INDEX,
                 _( "&Getting Started in KiCad" ),
                 _( "Open \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

    // Separator
    helpMenu->AppendSeparator();

    // About
    AddMenuItem( helpMenu, wxID_ABOUT,
                 _( "&About KiCad" ),
                 _( "About KiCad project manager" ),
                 KiBitmap( info_xpm ) );

    // Create the menubar and append all submenus
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( browseMenu, _( "&Browse" ) );
    menuBar->Append( preferencesMenu, _( "&Preferences" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    menuBar->Thaw();

    // Associate the menu bar with the frame, if no previous menubar
    if( GetMenuBar() == NULL )
        SetMenuBar( menuBar );
    else
        menuBar->Refresh();
}


/**
 * @brief (Re)Create the horizontal toolbar
 */
void KICAD_MANAGER_FRAME::RecreateBaseHToolbar()
{
    // Check if toolbar is not already created
    if( m_mainToolBar != NULL )
        return;

    // Allocate memory for m_mainToolBar
    m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                      wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // New
    m_mainToolBar->AddTool( ID_NEW_PROJECT, wxEmptyString,
                            KiBitmap( new_project_xpm ),
                            _( "Create new project" ) );

    m_mainToolBar->AddTool( ID_NEW_PROJECT_FROM_TEMPLATE, wxEmptyString,
                            KiBitmap( new_project_with_template_xpm ),
                            _( "Create new project from template" ) );

    // Load
    m_mainToolBar->AddTool( ID_LOAD_PROJECT, wxEmptyString,
                            KiBitmap( open_project_xpm ),
                            _( "Open existing project" ) );

    // Save
    m_mainToolBar->AddTool( ID_SAVE_PROJECT, wxEmptyString,
                            KiBitmap( save_project_xpm ),
                            _( "Save current project" ) );

    // Separator
    m_mainToolBar->AddSeparator();

    // Archive
    m_mainToolBar->AddTool( ID_SAVE_AND_ZIP_FILES, wxEmptyString,
                            KiBitmap( zip_xpm ),
                            _( "Archive all project files" ) );

    // Separator
    m_mainToolBar->AddSeparator();

    // Refresh project tree
    m_mainToolBar->AddTool( ID_PROJECT_TREE_REFRESH, wxEmptyString,
                            KiBitmap( reload_xpm ),
                            _( "Refresh project tree" ) );

    // Create m_mainToolBar
    m_mainToolBar->Realize();
}
