/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009-2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <hotkeys_basic.h>

// Menubar and toolbar event table
BEGIN_EVENT_TABLE( KICAD_MANAGER_FRAME, EDA_BASE_FRAME )
    // Window events
    EVT_SIZE( KICAD_MANAGER_FRAME::OnSize )
    EVT_CLOSE( KICAD_MANAGER_FRAME::OnCloseWindow )

    // Toolbar events
    EVT_TOOL( ID_NEW_PROJECT, KICAD_MANAGER_FRAME::OnLoadProject )
    EVT_TOOL( ID_NEW_PROJECT_FROM_TEMPLATE, KICAD_MANAGER_FRAME::OnCreateProjectFromTemplate )
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

    // Hotkey management (show list, edit ...) events
    EVT_MENU_RANGE( ID_PREFERENCES_HOTKEY_START, ID_PREFERENCES_HOTKEY_END,
                    KICAD_MANAGER_FRAME::Process_Config )


    // Special functions
#ifdef KICAD_USE_FILES_WATCHER
    EVT_MENU( ID_INIT_WATCHED_PATHS, KICAD_MANAGER_FRAME::OnChangeWatchedPaths )
#endif

    // Button events (in command frame), and menu events equivalent to buttons
    EVT_BUTTON( ID_TO_SCH, KICAD_MANAGER_FRAME::OnRunEeschema )
    EVT_MENU( ID_TO_SCH, KICAD_MANAGER_FRAME::OnRunEeschema )

    EVT_BUTTON( ID_TO_SCH_LIB_EDITOR, KICAD_MANAGER_FRAME::OnRunSchLibEditor )
    EVT_MENU( ID_TO_SCH_LIB_EDITOR, KICAD_MANAGER_FRAME::OnRunSchLibEditor )

    EVT_BUTTON( ID_TO_CVPCB, KICAD_MANAGER_FRAME::OnRunCvpcb )

    EVT_BUTTON( ID_TO_PCB, KICAD_MANAGER_FRAME::OnRunPcbNew )
    EVT_MENU( ID_TO_PCB, KICAD_MANAGER_FRAME::OnRunPcbNew )

    EVT_BUTTON( ID_TO_PCB_FP_EDITOR, KICAD_MANAGER_FRAME::OnRunPcbFpEditor )
    EVT_MENU( ID_TO_PCB_FP_EDITOR, KICAD_MANAGER_FRAME::OnRunPcbFpEditor )

    EVT_BUTTON( ID_TO_GERBVIEW, KICAD_MANAGER_FRAME::OnRunGerbview )
    EVT_MENU( ID_TO_GERBVIEW, KICAD_MANAGER_FRAME::OnRunGerbview )

    EVT_BUTTON( ID_TO_BITMAP_CONVERTER, KICAD_MANAGER_FRAME::OnRunBitmapConverter )
    EVT_MENU( ID_TO_BITMAP_CONVERTER, KICAD_MANAGER_FRAME::OnRunBitmapConverter )

    EVT_BUTTON( ID_TO_PCB_CALCULATOR, KICAD_MANAGER_FRAME::OnRunPcbCalculator )
    EVT_MENU( ID_TO_PCB_CALCULATOR, KICAD_MANAGER_FRAME::OnRunPcbCalculator )

    EVT_BUTTON( ID_TO_PL_EDITOR, KICAD_MANAGER_FRAME::OnRunPageLayoutEditor )
    EVT_MENU( ID_TO_PL_EDITOR, KICAD_MANAGER_FRAME::OnRunPageLayoutEditor )

    EVT_UPDATE_UI( ID_SELECT_DEFAULT_PDF_BROWSER, KICAD_MANAGER_FRAME::OnUpdateDefaultPdfBrowser )
    EVT_UPDATE_UI( ID_SELECT_PREFERED_PDF_BROWSER,
                   KICAD_MANAGER_FRAME::OnUpdatePreferredPdfBrowser )

END_EVENT_TABLE()

enum hotkey_id_commnand
{
    HK_RUN_EESCHEMA = HK_COMMON_END,
    HK_LOAD_PROJECT,
    HK_SAVE_PROJECT,
    HK_NEW_PRJ,
    HK_NEW_PRJ_TEMPLATE,
    HK_RUN_LIBEDIT,
    HK_RUN_PCBNEW,
    HK_RUN_FPEDITOR,
    HK_RUN_GERBVIEW,
    HK_RUN_BM2COMPONENT,
    HK_RUN_PCBCALCULATOR,
    HK_RUN_PLEDITOR
};

/////////////  Hotkeys management   ///////////////////////////////////////
// hotkeys command:
static EDA_HOTKEY HkHelp( wxT( "Help (this window)" ), HK_HELP, '?' );
static EDA_HOTKEY HkLoadPrj( wxT( "Load project" ), HK_LOAD_PROJECT, 'O' + GR_KB_CTRL );
static EDA_HOTKEY HkSavePrj( wxT( "Save project" ), HK_SAVE_PROJECT, 'S' + GR_KB_CTRL );
static EDA_HOTKEY HkNewProject( wxT( "New Project" ), HK_NEW_PRJ, 'N' + GR_KB_CTRL );
static EDA_HOTKEY HkNewPrjFromTemplate( wxT( "New Prj From Template" ),
                                        HK_NEW_PRJ_TEMPLATE, 'T' + GR_KB_CTRL );

static EDA_HOTKEY HkRunEeschema( wxT( "Run Eeschema" ), HK_RUN_EESCHEMA, 'E', 0 );
static EDA_HOTKEY HkRunLibedit( wxT( "Run LibEdit" ), HK_RUN_LIBEDIT, 'L', 0 );
static EDA_HOTKEY HkRunPcbnew( wxT( "Run Pcbnew" ), HK_RUN_PCBNEW, 'P', 0 );
static EDA_HOTKEY HkRunModedit( wxT( "Run FpEditor" ), HK_RUN_FPEDITOR, 'F', 0 );
static EDA_HOTKEY HkRunGerbview( wxT( "Run Gerbview" ), HK_RUN_GERBVIEW, 'G', 0 );
static EDA_HOTKEY HkRunBm2Cmp( wxT( "Run Bitmap2Component" ), HK_RUN_BM2COMPONENT, 'B', 0 );
static EDA_HOTKEY HkRunPcbCalc( wxT( "Run PcbCalculator" ), HK_RUN_PCBCALCULATOR, 'C', 0 );
static EDA_HOTKEY HkRunPleditor( wxT( "Run PlEditor" ), HK_RUN_PLEDITOR, 'Y', 0 );

// List of hotkey descriptors
EDA_HOTKEY* common_Hotkey_List[] =
{
    &HkHelp,
    &HkLoadPrj,     &HkSavePrj,     &HkNewProject,  &HkNewPrjFromTemplate,
    &HkRunEeschema, &HkRunLibedit,
    &HkRunPcbnew,   &HkRunModedit,  &HkRunGerbview,
    &HkRunBm2Cmp,   &HkRunPcbCalc,  &HkRunPleditor,
    NULL
};

// list of sections and corresponding hotkey list for Kicad
// (used to create an hotkey config file, and edit hotkeys )
// here we have only one section.
struct EDA_HOTKEY_CONFIG kicad_Manager_Hokeys_Descr[] = {
    { &g_CommonSectionTag,      common_Hotkey_List,         &g_CommonSectionTitle      },
    { NULL,                     NULL,                       NULL                       }
};
/////////////  End hotkeys management   ///////////////////////////////////////


/**
 * @brief (Re)Create the menubar
 */
void KICAD_MANAGER_FRAME::ReCreateMenuBar()
{
    wxString msg;
    static wxMenu* openRecentMenu;  // Open Recent submenu,
                                    // static to remember this menu

    m_manager_Hokeys_Descr = kicad_Manager_Hokeys_Descr;

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
    msg = AddHotkeyName( _( "&Open Project" ), kicad_Manager_Hokeys_Descr, HK_LOAD_PROJECT );
    AddMenuItem( fileMenu, ID_LOAD_PROJECT, msg,
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

    // New project creation
    wxMenu* newprjSubMenu = new wxMenu();

    msg = AddHotkeyName( _( "&New Project" ), kicad_Manager_Hokeys_Descr, HK_NEW_PRJ );
    AddMenuItem( newprjSubMenu, ID_NEW_PROJECT, msg,
                 _( "Create new blank project" ),
                 KiBitmap( new_project_xpm ) );

    msg = AddHotkeyName( _( "New Project from &Template" ),
                         kicad_Manager_Hokeys_Descr, HK_NEW_PRJ_TEMPLATE );
    AddMenuItem( newprjSubMenu, ID_NEW_PROJECT_FROM_TEMPLATE, msg,
                 _( "Create a new project from a template" ),
                 KiBitmap( new_project_with_template_xpm ) );

    AddMenuItem( fileMenu, newprjSubMenu,
                 wxID_ANY,
                 _( "New Project" ),
                 _( "Create new project" ),
                 KiBitmap( new_project_xpm ) );

    // Currently there is nothing to save
    // (Kicad manager does not save any info in .pro file)
#if 0
    // Save
    msg = AddHotkeyName( _( "&Save" ), kicad_Manager_Hokeys_Descr, HK_SAVE_PROJECT );
    AddMenuItem( fileMenu, ID_SAVE_PROJECT, msg,
                 _( "Save current project" ),
                 KiBitmap( save_project_xpm ) );
#endif

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

    // Hotkey submenu
    preferencesMenu->AppendSeparator();
    AddHotkeyConfigMenu( preferencesMenu );

    // Language submenu
    preferencesMenu->AppendSeparator();
    Pgm().AddMenuLanguageList( preferencesMenu );

    // Hotkey submenu
    AddHotkeyConfigMenu( preferencesMenu );

    // Menu Tools:
    wxMenu* toolsMenu = new wxMenu;

    msg = AddHotkeyName( _( "Run Eeschema" ), kicad_Manager_Hokeys_Descr, HK_RUN_EESCHEMA );
    AddMenuItem( toolsMenu, ID_TO_SCH, msg,
                 KiBitmap( eeschema_xpm ) );

    msg = AddHotkeyName( _( "Run Library Editor" ),
                         kicad_Manager_Hokeys_Descr, HK_RUN_LIBEDIT );
    AddMenuItem( toolsMenu, ID_TO_SCH_LIB_EDITOR, msg,
                 KiBitmap( libedit_xpm ) );

    msg = AddHotkeyName( _( "Run Pcbnew" ),
                         kicad_Manager_Hokeys_Descr, HK_RUN_PCBNEW );
    AddMenuItem( toolsMenu, ID_TO_PCB, msg,
                 KiBitmap( pcbnew_xpm ) );

    msg = AddHotkeyName( _( "Run Footprint Editor" ),
                         kicad_Manager_Hokeys_Descr, HK_RUN_FPEDITOR );
    AddMenuItem( toolsMenu, ID_TO_PCB_FP_EDITOR, msg,
                 KiBitmap( module_editor_xpm ) );

    msg = AddHotkeyName( _( "Run Gerbview" ),
                         kicad_Manager_Hokeys_Descr, HK_RUN_GERBVIEW );
    AddMenuItem( toolsMenu, ID_TO_GERBVIEW, msg,
                 KiBitmap( icon_gerbview_small_xpm ) );

    msg = AddHotkeyName( _( "Run Bitmap2Component" ),
                         kicad_Manager_Hokeys_Descr, HK_RUN_BM2COMPONENT );
    AddMenuItem( toolsMenu, ID_TO_BITMAP_CONVERTER, msg,
                 _( "Bitmap2Component - Convert bitmap images to Eeschema\n"
                    "or Pcbnew elements" ),
                 KiBitmap( image_xpm ) );

    msg = AddHotkeyName( _( "Run Pcb Calculator" ), kicad_Manager_Hokeys_Descr, HK_RUN_PCBCALCULATOR );
    AddMenuItem( toolsMenu, ID_TO_PCB_CALCULATOR, msg,
                 _( "Pcb calculator - Calculator for components, track width, etc." ),
                 KiBitmap( options_module_xpm ) );

    msg = AddHotkeyName( _( "Run Page Layout Editor" ), kicad_Manager_Hokeys_Descr, HK_RUN_PLEDITOR );
    AddMenuItem( toolsMenu, ID_TO_PL_EDITOR, msg,
                 _( "Pl editor - Worksheet layout editor" ),
                 KiBitmap( pagelayout_load_xpm ) );

    // Menu Help:
    wxMenu* helpMenu = new wxMenu;

    // Version info
    AddHelpVersionInfoMenuEntry( helpMenu );

    // Contents
    AddMenuItem( helpMenu, wxID_HELP,
                 _( "KiCad &Manual" ),
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
    menuBar->Append( toolsMenu, _( "&Tools" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    menuBar->Thaw();

    // Associate the menu bar with the frame, if no previous menubar
    if( GetMenuBar() == NULL )
        SetMenuBar( menuBar );
    else
        menuBar->Refresh();

    // Add the hotkey to the "show hotkey list" menu, because we do not have
    // a management of the keyboard keys in Kicad.
    // So all hotheys should be added to the menubar
    // Note Use wxMenuBar::SetLabel only after the menubar
    // has been associated with a frame. (see wxWidgets doc)
    msg = AddHotkeyName( menuBar->GetLabel( ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST ),
                         kicad_Manager_Hokeys_Descr, HK_HELP );
    menuBar->SetLabel( ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST, msg );
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
