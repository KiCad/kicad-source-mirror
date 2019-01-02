/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
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
 * @file kicad/menubar.cpp
 * @brief (Re)Create the project manager menubar for KiCad
 */


#include <bitmaps.h>
#include <hotkeys_basic.h>
#include <menus_helpers.h>

#include "kicad.h"
#include "pgm_kicad.h"


// Menubar and toolbar event table
BEGIN_EVENT_TABLE( KICAD_MANAGER_FRAME, EDA_BASE_FRAME )
    // Window events
    EVT_SIZE( KICAD_MANAGER_FRAME::OnSize )
    EVT_CLOSE( KICAD_MANAGER_FRAME::OnCloseWindow )

    // Toolbar events
    EVT_TOOL( ID_NEW_PROJECT, KICAD_MANAGER_FRAME::OnNewProject )
    EVT_TOOL( ID_NEW_PROJECT_FROM_TEMPLATE, KICAD_MANAGER_FRAME::OnCreateProjectFromTemplate )
    EVT_TOOL( ID_LOAD_PROJECT, KICAD_MANAGER_FRAME::OnLoadProject )

    // Menu events
    EVT_MENU( ID_SAVE_PROJECT, KICAD_MANAGER_FRAME::OnSaveProject )
    EVT_MENU( wxID_EXIT, KICAD_MANAGER_FRAME::OnExit )
    EVT_MENU( ID_TO_TEXT_EDITOR, KICAD_MANAGER_FRAME::OnOpenTextEditor )
    EVT_MENU( ID_BROWSE_AN_SELECT_FILE, KICAD_MANAGER_FRAME::OnOpenFileInTextEditor )
    EVT_MENU( ID_BROWSE_IN_FILE_EXPLORER, KICAD_MANAGER_FRAME::OnBrowseInFileExplorer )
    EVT_MENU( ID_PREFERENCES_CONFIGURE_PATHS, KICAD_MANAGER_FRAME::OnConfigurePaths )
    EVT_MENU( ID_EDIT_SYMBOL_LIBRARY_TABLE, KICAD_MANAGER_FRAME::OnEditSymLibTable )
    EVT_MENU( ID_EDIT_FOOTPRINT_LIBRARY_TABLE, KICAD_MANAGER_FRAME::OnEditFpLibTable )
    EVT_MENU( wxID_PREFERENCES, KICAD_MANAGER_FRAME::OnPreferences )
    EVT_MENU( ID_SAVE_AND_ZIP_FILES, KICAD_MANAGER_FRAME::OnArchiveFiles )
    EVT_MENU( ID_READ_ZIP_ARCHIVE, KICAD_MANAGER_FRAME::OnUnarchiveFiles )
    EVT_MENU( ID_PROJECT_TREE_REFRESH, KICAD_MANAGER_FRAME::OnRefresh )
    EVT_MENU( wxID_HELP, KICAD_MANAGER_FRAME::GetKicadHelp )
    EVT_MENU( wxID_INDEX, KICAD_MANAGER_FRAME::GetKicadHelp )
    EVT_MENU( ID_HELP_GET_INVOLVED, KICAD_MANAGER_FRAME::GetKicadContribute )
    EVT_MENU( wxID_ABOUT, KICAD_MANAGER_FRAME::GetKicadAbout )
    EVT_MENU( ID_IMPORT_EAGLE_PROJECT, KICAD_MANAGER_FRAME::OnImportEagleFiles )

    // Range menu events
    EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END,
                    KICAD_MANAGER_FRAME::language_change )

    EVT_MENU_RANGE( ID_FILE1, ID_FILEMAX, KICAD_MANAGER_FRAME::OnFileHistory )

    // Show hotkeys
    EVT_MENU( ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST, KICAD_MANAGER_FRAME::OnShowHotkeys )


    // Special functions
    EVT_MENU( ID_INIT_WATCHED_PATHS, KICAD_MANAGER_FRAME::OnChangeWatchedPaths )

    // Button events (in command frame), and menu events equivalent to buttons
    EVT_BUTTON( ID_TO_SCH, KICAD_MANAGER_FRAME::OnRunEeschema )
    EVT_MENU( ID_TO_SCH, KICAD_MANAGER_FRAME::OnRunEeschema )

    EVT_BUTTON( ID_TO_SCH_LIB_EDITOR, KICAD_MANAGER_FRAME::OnRunSchLibEditor )
    EVT_MENU( ID_TO_SCH_LIB_EDITOR, KICAD_MANAGER_FRAME::OnRunSchLibEditor )

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

    EVT_UPDATE_UI_RANGE( ID_TO_SCH, ID_TO_PCB_FP_EDITOR,
                         KICAD_MANAGER_FRAME::OnUpdateRequiresProject )

END_EVENT_TABLE()

enum hotkey_id_commnand
{
    HK_RUN_EESCHEMA = HK_COMMON_END,
    HK_NEW_PRJ_TEMPLATE,
    HK_REFRESH,
    HK_RUN_LIBEDIT,
    HK_RUN_PCBNEW,
    HK_RUN_FPEDITOR,
    HK_RUN_GERBVIEW,
    HK_RUN_BM2COMPONENT,
    HK_RUN_PCBCALCULATOR,
    HK_RUN_PLEDITOR
};

/////////////  Hotkeys management   ///////////////////////////////////////

// Remark: the hotkey message info is used as keyword in hotkey config files and
// as comments in help windows, therefore translated only when displayed
// they are marked _HKI to be extracted by translation tools
// See hotkeys_basic.h for more info

// hotkeys command:
static EDA_HOTKEY HkNewProjectFromTemplate( _HKI( "New Project From Template" ),
                                        HK_NEW_PRJ_TEMPLATE, 'T' + GR_KB_CTRL );
static EDA_HOTKEY HkRefresh( _HKI( "Refresh Project Tree" ), HK_REFRESH, GR_KB_CTRL + 'R' );
static EDA_HOTKEY HkRunEeschema( _HKI( "Run Eeschema" ), HK_RUN_EESCHEMA, 'E' + GR_KB_CTRL, 0 );
static EDA_HOTKEY HkRunLibedit( _HKI( "Run LibEdit" ), HK_RUN_LIBEDIT, 'L' + GR_KB_CTRL, 0 );
static EDA_HOTKEY HkRunPcbnew( _HKI( "Run Pcbnew" ), HK_RUN_PCBNEW, 'P' + GR_KB_CTRL, 0 );
static EDA_HOTKEY HkRunModedit( _HKI( "Run FpEditor" ), HK_RUN_FPEDITOR, 'F' + GR_KB_CTRL, 0 );
static EDA_HOTKEY HkRunGerbview( _HKI( "Run Gerbview" ), HK_RUN_GERBVIEW, 'G' + GR_KB_CTRL, 0 );
static EDA_HOTKEY HkRunBm2Cmp( _HKI( "Run Bitmap2Component" ),
                               HK_RUN_BM2COMPONENT, 'B' + GR_KB_CTRL, 0 );
static EDA_HOTKEY HkRunPcbCalc( _HKI( "Run PcbCalculator" ),
                                HK_RUN_PCBCALCULATOR, 'A' + GR_KB_CTRL, 0 );
static EDA_HOTKEY HkRunPleditor( _HKI( "Run PlEditor" ), HK_RUN_PLEDITOR, 'Y' + GR_KB_CTRL, 0 );

// Common: hotkeys_basic.h
static EDA_HOTKEY HkNewProject( _HKI( "New Project" ), HK_NEW, GR_KB_CTRL + 'N' );
static EDA_HOTKEY HkOpenProject( _HKI( "Open Project" ), HK_OPEN, GR_KB_CTRL + 'O' );
static EDA_HOTKEY HkSaveProject( _HKI( "Save Project" ), HK_SAVE, GR_KB_CTRL + 'S' );
static EDA_HOTKEY HkHelp( _HKI( "List Hotkeys" ), HK_HELP, GR_KB_CTRL + WXK_F1 );

// List of hotkey descriptors
EDA_HOTKEY* common_Hotkey_List[] =
{
    &HkNewProject,  &HkNewProjectFromTemplate, &HkOpenProject,
    // Currently there is nothing to save
    // (Kicad manager does not save any info in .pro file)
#if 0
    &HkSaveProject,
#endif
    &HkRefresh,     &HkHelp,
    &HkRunEeschema, &HkRunLibedit,
    &HkRunPcbnew,   &HkRunModedit,  &HkRunGerbview,
    &HkRunBm2Cmp,   &HkRunPcbCalc,  &HkRunPleditor,
    NULL
};

// list of sections and corresponding hotkey list for Kicad
// (used to create an hotkey config file, and edit hotkeys )
// here we have only one section.
static wxString sectionTitle( _HKI( "Kicad Manager Hotkeys" ) );

struct EDA_HOTKEY_CONFIG kicad_Manager_Hokeys_Descr[] = {
    { &g_CommonSectionTag,      common_Hotkey_List,         &sectionTitle      },
    { NULL,                     NULL,                       NULL               }
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

    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar*  oldMenuBar = GetMenuBar();
    wxMenuBar*  menuBar = new wxMenuBar();

    // Before deleting, remove the menus managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history)
    if( openRecentMenu )
        PgmTop().GetFileHistory().RemoveMenu( openRecentMenu );

    // Recreate all menus:

    // Menu File:
    wxMenu* fileMenu = new wxMenu;

    // New project creation
    wxMenu* newprjSubMenu = new wxMenu();
    msg = AddHotkeyName( _( "&Project..." ), kicad_Manager_Hokeys_Descr, HK_NEW );
    AddMenuItem( newprjSubMenu, ID_NEW_PROJECT, msg,
                 _( "Create new blank project" ),
                 KiBitmap( new_project_xpm ) );
    msg = AddHotkeyName( _( "Project from &Template..." ),
                         kicad_Manager_Hokeys_Descr, HK_NEW_PRJ_TEMPLATE );
    AddMenuItem( newprjSubMenu, ID_NEW_PROJECT_FROM_TEMPLATE, msg,
                 _( "Create new project from template" ),
                 KiBitmap( new_project_with_template_xpm ) );
    AddMenuItem( fileMenu, newprjSubMenu,
                 wxID_ANY,
                 _( "&New" ),
                 _( "Create new project" ),
                 KiBitmap( new_project_xpm ) );

    // Open
    msg = AddHotkeyName( _( "&Open Project..." ), kicad_Manager_Hokeys_Descr, HK_OPEN );
    AddMenuItem( fileMenu, ID_LOAD_PROJECT, msg,
                 _( "Open an existing project" ),
                 KiBitmap( open_project_xpm ) );

    // File history
    openRecentMenu = new wxMenu();
    PgmTop().GetFileHistory().UseMenu( openRecentMenu );
    PgmTop().GetFileHistory().AddFilesToMenu( );
    AddMenuItem( fileMenu, openRecentMenu,
                 wxID_ANY,
                 _( "Open &Recent" ),
                 _( "Open a recent project" ),
                 KiBitmap( recent_xpm ) );

    // Currently there is nothing to save
    // (Kicad manager does not save any info in .pro file)
#if 0
    // Save
    msg = AddHotkeyName( _( "&Save" ), kicad_Manager_Hokeys_Descr, HK_SAVE );
    AddMenuItem( fileMenu, ID_SAVE_PROJECT, msg,
                 _( "Save current project" ),
                 KiBitmap( save_project_xpm ) );
#endif

    fileMenu->AppendSeparator();
    wxMenu* importprjSubMenu = new wxMenu();

    AddMenuItem( importprjSubMenu, ID_IMPORT_EAGLE_PROJECT, _( "EAGLE CAD..." ),
            _( "Import EAGLE CAD XML schematic and board" ),
            KiBitmap( import_project_xpm ) );


    AddMenuItem( fileMenu, importprjSubMenu,
            wxID_ANY,
            _( "Import Project" ),
            _( "Import project files from other software" ),
            KiBitmap( import_project_xpm ) );


    fileMenu->AppendSeparator();

    // Archive
    AddMenuItem( fileMenu,
                 ID_SAVE_AND_ZIP_FILES,
                 _( "&Archive Project..." ),
                 _( "Archive all needed project files into zip archive" ),
                 KiBitmap( zip_xpm ) );

    // Unarchive
    AddMenuItem( fileMenu,
                 ID_READ_ZIP_ARCHIVE,
                 _( "&Unarchive Project..." ),
                 _( "Unarchive project files from zip archive" ),
                 KiBitmap( unzip_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Quit
    AddMenuItem( fileMenu,
                 wxID_EXIT,
                 _( "&Exit" ),
                 _( "Close KiCad" ),
                 KiBitmap( exit_xpm ) );

     // View Menu:
     wxMenu* viewMenu = new wxMenu();

     // Refresh project tree
     msg = AddHotkeyName( _( "&Refresh" ), kicad_Manager_Hokeys_Descr, HK_REFRESH );
     AddMenuItem( viewMenu, ID_PROJECT_TREE_REFRESH, msg,
                  _( "Refresh project tree" ),
                  KiBitmap( reload_xpm ) );

#ifdef __APPLE__
    viewMenu->AppendSeparator();
#endif

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
                 _( "&Open Local File..." ),
                 _( "Edit local file" ),
                 KiBitmap( browse_files_xpm ) );

    // Browse in file explorer
    browseMenu->AppendSeparator();
    AddMenuItem( browseMenu,
                ID_BROWSE_IN_FILE_EXPLORER,
                _( "&Browse Project Files" ),
                _( "Open project directory in file explorer" ),
                KiBitmap( directory_browser_xpm ) );

    // Menu Preferences:
    wxMenu* preferencesMenu = new wxMenu;

    // Path configuration edit dialog.
    AddMenuItem( preferencesMenu,
                 ID_PREFERENCES_CONFIGURE_PATHS,
                 _( "&Configure Paths..." ),
                 _( "Edit path configuration environment variables" ),
                 KiBitmap( path_xpm ) );

    AddMenuItem( preferencesMenu,
                 ID_EDIT_SYMBOL_LIBRARY_TABLE,
                 _( "Manage &Symbol Libraries..." ),
                 _( "Edit the global and project symbol library tables" ),
                 KiBitmap( library_table_xpm ) );

    AddMenuItem( preferencesMenu,
                 ID_EDIT_FOOTPRINT_LIBRARY_TABLE,
                 _( "Manage &Footprint Libraries..." ),
                 _( "Configure footprint library table" ),
                 KiBitmap( library_table_xpm ) );

    AddMenuItem( preferencesMenu,
                 wxID_PREFERENCES,
                 _( "&Preferences..." ),
                 _( "Show preferences for all open tools" ),
                 KiBitmap( preference_xpm ) );

    preferencesMenu->AppendSeparator();

    // Language submenu
    Pgm().AddMenuLanguageList( preferencesMenu );

    // Menu Tools:
    wxMenu* toolsMenu = new wxMenu;

    msg = AddHotkeyName( _( "Edit Schematic" ), kicad_Manager_Hokeys_Descr, HK_RUN_EESCHEMA );
    AddMenuItem( toolsMenu, ID_TO_SCH, msg, KiBitmap( eeschema_xpm ) );

    msg = AddHotkeyName( _( "Edit Schematic Symbols" ),
                         kicad_Manager_Hokeys_Descr, HK_RUN_LIBEDIT );
    AddMenuItem( toolsMenu, ID_TO_SCH_LIB_EDITOR, msg, KiBitmap( libedit_xpm ) );

    msg = AddHotkeyName( _( "Edit PCB" ),
                         kicad_Manager_Hokeys_Descr, HK_RUN_PCBNEW );
    AddMenuItem( toolsMenu, ID_TO_PCB, msg, KiBitmap( pcbnew_xpm ) );

    msg = AddHotkeyName( _( "Edit PCB Footprints" ),
                         kicad_Manager_Hokeys_Descr, HK_RUN_FPEDITOR );
    AddMenuItem( toolsMenu, ID_TO_PCB_FP_EDITOR, msg, KiBitmap( module_editor_xpm ) );

    msg = AddHotkeyName( _( "View Gerber Files" ),
                         kicad_Manager_Hokeys_Descr, HK_RUN_GERBVIEW );
    AddMenuItem( toolsMenu, ID_TO_GERBVIEW, msg, KiBitmap( icon_gerbview_small_xpm ) );

    msg = AddHotkeyName( _( "Convert Image" ),
                         kicad_Manager_Hokeys_Descr, HK_RUN_BM2COMPONENT );
    AddMenuItem( toolsMenu, ID_TO_BITMAP_CONVERTER, msg,
                 _( "Convert bitmap images to schematic or PCB components." ),
                 KiBitmap( bitmap2component_xpm ) );

    msg = AddHotkeyName( _( "Calculator Tools" ), kicad_Manager_Hokeys_Descr, HK_RUN_PCBCALCULATOR );
    AddMenuItem( toolsMenu, ID_TO_PCB_CALCULATOR, msg,
                 _( "Run component calculations, track width calculations, etc." ),
                 KiBitmap( calculator_xpm ) );

    msg = AddHotkeyName( _( "Edit Worksheet" ), kicad_Manager_Hokeys_Descr, HK_RUN_PLEDITOR );
    AddMenuItem( toolsMenu, ID_TO_PL_EDITOR, msg,
                 _( "Edit worksheet graphics and text" ),
                 KiBitmap( pagelayout_load_xpm ) );

    // Menu Help:
    wxMenu* helpMenu = new wxMenu;

    // Contents
    AddMenuItem( helpMenu, wxID_HELP,
                 _( "KiCad &Manual" ),
                 _( "Open KiCad user manual" ),
                 KiBitmap( online_help_xpm ) );

    AddMenuItem( helpMenu, wxID_INDEX,
                 _( "&Getting Started in KiCad" ),
                 _( "Open \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

    AddMenuItem( helpMenu,
                 ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST,
                 _( "&List Hotkeys" ),
                 _( "Displays the current hotkeys list and corresponding commands" ),
                 KiBitmap( hotkeys_xpm ) );

    // Separator
    helpMenu->AppendSeparator();

    // Get involved with KiCad
    AddMenuItem( helpMenu, ID_HELP_GET_INVOLVED,
                 _( "Get &Involved" ),
                 _( "Contribute to KiCad (opens a web browser)" ),
                 KiBitmap( info_xpm ) );

    helpMenu->AppendSeparator();

    // About
    AddMenuItem( helpMenu, wxID_ABOUT, _( "&About KiCad" ), KiBitmap( about_xpm ) );

    // Create the menubar and append all submenus
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( toolsMenu, _( "&Tools" ) );
    menuBar->Append( browseMenu, _( "&Browse" ) );
    menuBar->Append( preferencesMenu, _( "&Preferences" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    SetMenuBar( menuBar );
    delete oldMenuBar;

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
    if( m_mainToolBar )
        m_mainToolBar->Clear();
    else
        m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                          KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // New
    m_mainToolBar->AddTool( ID_NEW_PROJECT, wxEmptyString,
                            KiScaledBitmap( new_project_xpm, this ),
                            _( "Create new project" ) );

    m_mainToolBar->AddTool( ID_NEW_PROJECT_FROM_TEMPLATE, wxEmptyString,
                            KiScaledBitmap( new_project_with_template_xpm, this ),
                            _( "Create new project from template" ) );

    // Load
    m_mainToolBar->AddTool( ID_LOAD_PROJECT, wxEmptyString,
                            KiScaledBitmap( open_project_xpm, this ),
                            _( "Open existing project" ) );

    // Currently there is nothing to save
    // (Kicad manager does not save any info in .pro file)
#if 0
    // Save
    m_mainToolBar->AddTool( ID_SAVE_PROJECT, wxEmptyString,
                            KiScaledBitmap( save_project_xpm, this ),
                            _( "Save current project" ) );
#endif

    KiScaledSeparator( m_mainToolBar, this );

    // Archive
    m_mainToolBar->AddTool( ID_SAVE_AND_ZIP_FILES, wxEmptyString,
                            KiScaledBitmap( zip_xpm, this ),
                            _( "Archive all project files" ) );

    // Unarchive
    m_mainToolBar->AddTool( ID_READ_ZIP_ARCHIVE, wxEmptyString,
                            KiScaledBitmap( unzip_xpm, this ),
                            _( "Unarchive project files from zip archive" ) );

    KiScaledSeparator( m_mainToolBar, this );

    // Refresh project tree
    m_mainToolBar->AddTool( ID_PROJECT_TREE_REFRESH, wxEmptyString,
                            KiScaledBitmap( reload_xpm, this ),
                            _( "Refresh project tree" ) );

    // Acces to the system file manager
    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_BROWSE_IN_FILE_EXPLORER, wxEmptyString,
                            KiScaledBitmap( directory_browser_xpm, this ),
                            _( "Open project directory in file explorer" ) );

    // Create m_mainToolBar
    m_mainToolBar->Realize();
}
