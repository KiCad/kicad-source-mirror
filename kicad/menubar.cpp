/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
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

#include <bitmaps.h>
#include <hotkeys_basic.h>
#include <menus_helpers.h>
#include <tool/tool_manager.h>
#include <tools/kicad_manager_control.h>
#include <tools/kicad_manager_actions.h>
#include "kicad_manager_frame.h"
#include "pgm_kicad.h"
#include "kicad_id.h"


// Menubar and toolbar event table
BEGIN_EVENT_TABLE( KICAD_MANAGER_FRAME, EDA_BASE_FRAME )
    // Window events
    EVT_SIZE( KICAD_MANAGER_FRAME::OnSize )
    EVT_CLOSE( KICAD_MANAGER_FRAME::OnCloseWindow )

    // Menu events
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

END_EVENT_TABLE()

enum hotkey_id_command
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
static EDA_HOTKEY HkPreferences( _HKI( "Preferences" ),
                                 HK_PREFERENCES, GR_KB_CTRL + ',', (int) wxID_PREFERENCES );

// List of hotkey descriptors
EDA_HOTKEY* common_Hotkey_List[] =
{
    &HkNewProject,  &HkNewProjectFromTemplate, &HkOpenProject,
    // Currently there is nothing to save
    // (Kicad manager does not save any info in .pro file)
#if 0
    &HkSaveProject,
#endif
    &HkRefresh,     &HkHelp,        &HkPreferences,
    &HkRunEeschema, &HkRunLibedit,
    &HkRunPcbnew,   &HkRunModedit,  &HkRunGerbview,
    &HkRunBm2Cmp,   &HkRunPcbCalc,  &HkRunPleditor,
    NULL
};

// list of sections and corresponding hotkey list for Kicad
// (used to create an hotkey config file, and edit hotkeys )
// here we have only one section.
static wxString sectionTitle( _HKI( "Kicad Manager Hotkeys" ) );

struct EDA_HOTKEY_CONFIG kicad_Manager_Hotkeys_Descr[] = {
    { &g_CommonSectionTag,      common_Hotkey_List,         &sectionTitle      },
    { NULL,                     NULL,                       NULL               }
};
/////////////  End hotkeys management   ///////////////////////////////////////


/**
 * @brief (Re)Create the menubar
 */
void KICAD_MANAGER_FRAME::ReCreateMenuBar()
{
    KICAD_MANAGER_CONTROL* controlTool = m_toolManager->GetTool<KICAD_MANAGER_CONTROL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar*  oldMenuBar = GetMenuBar();
    wxMenuBar*  menuBar = new wxMenuBar();

    m_manager_Hotkeys_Descr = kicad_Manager_Hotkeys_Descr;

    //-- File menu -----------------------------------------------------------
    //
    CONDITIONAL_MENU*   fileMenu = new CONDITIONAL_MENU( false, controlTool );
    static ACTION_MENU* openRecentMenu;

    // Before deleting, remove the menus managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history)
    if( openRecentMenu )
        PgmTop().GetFileHistory().RemoveMenu( openRecentMenu );

    openRecentMenu = new ACTION_MENU();
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
    // Don't use ACTIONS::quit; wxWidgets moves this on OSX and expects to find it via wxID_EXIT
    fileMenu->AddItem( wxID_EXIT, _( "Quit" ), "", exit_xpm,   SELECTION_CONDITIONS::ShowAlways );

    fileMenu->Resolve();

    //-- View menu -----------------------------------------------------------
    //
    CONDITIONAL_MENU* viewMenu = new CONDITIONAL_MENU( false, controlTool );
    viewMenu->AddItem( ACTIONS::zoomRedraw,             SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddItem( ID_TO_TEXT_EDITOR,
                       _( "Open Text Editor" ), _( "Launch preferred text editor" ),
                       editor_xpm,                      SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddItem( ID_BROWSE_AN_SELECT_FILE,
                       _( "Open Local File..." ), _( "Edit local file" ),
                       browse_files_xpm,                SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddItem( ID_BROWSE_IN_FILE_EXPLORER,
                       _( "Browse Project Files" ), _( "Open project directory in file browser" ),
                       directory_browser_xpm,           SELECTION_CONDITIONS::ShowAlways );

#ifdef __APPLE__
    viewMenu->AddSeparator();
#endif

    viewMenu->Resolve();

    //-- Tools menu -----------------------------------------------
    //
    wxMenu* toolsMenu = new wxMenu;
    wxString msg;
    msg = AddHotkeyName( _( "Edit Schematic" ), kicad_Manager_Hotkeys_Descr, HK_RUN_EESCHEMA );
    AddMenuItem( toolsMenu, ID_TO_SCH, msg, KiBitmap( eeschema_xpm ) );

    msg = AddHotkeyName( _( "Edit Schematic Symbols" ),
                         kicad_Manager_Hotkeys_Descr, HK_RUN_LIBEDIT );
    AddMenuItem( toolsMenu, ID_TO_SCH_LIB_EDITOR, msg, KiBitmap( libedit_xpm ) );

    msg = AddHotkeyName( _( "Edit PCB" ),
                         kicad_Manager_Hotkeys_Descr, HK_RUN_PCBNEW );
    AddMenuItem( toolsMenu, ID_TO_PCB, msg, KiBitmap( pcbnew_xpm ) );

    msg = AddHotkeyName( _( "Edit PCB Footprints" ),
                         kicad_Manager_Hotkeys_Descr, HK_RUN_FPEDITOR );
    AddMenuItem( toolsMenu, ID_TO_PCB_FP_EDITOR, msg, KiBitmap( module_editor_xpm ) );

    msg = AddHotkeyName( _( "View Gerber Files" ),
                         kicad_Manager_Hotkeys_Descr, HK_RUN_GERBVIEW );
    AddMenuItem( toolsMenu, ID_TO_GERBVIEW, msg, KiBitmap( icon_gerbview_small_xpm ) );

    msg = AddHotkeyName( _( "Convert Image" ),
                         kicad_Manager_Hotkeys_Descr, HK_RUN_BM2COMPONENT );
    AddMenuItem( toolsMenu, ID_TO_BITMAP_CONVERTER, msg,
                 _( "Convert bitmap images to schematic or PCB components." ),
                 KiBitmap( bitmap2component_xpm ) );

    msg = AddHotkeyName( _( "Calculator Tools" ), kicad_Manager_Hotkeys_Descr, HK_RUN_PCBCALCULATOR );
    AddMenuItem( toolsMenu, ID_TO_PCB_CALCULATOR, msg,
                 _( "Run component calculations, track width calculations, etc." ),
                 KiBitmap( calculator_xpm ) );

    msg = AddHotkeyName( _( "Edit Worksheet" ), kicad_Manager_Hotkeys_Descr, HK_RUN_PLEDITOR );
    AddMenuItem( toolsMenu, ID_TO_PL_EDITOR, msg,
                 _( "Edit worksheet graphics and text" ),
                 KiBitmap( pagelayout_load_xpm ) );

    //-- Preferences menu -----------------------------------------------
    //
    CONDITIONAL_MENU* prefsMenu = new CONDITIONAL_MENU( false, controlTool );

    prefsMenu->AddItem( ACTIONS::configurePaths,        SELECTION_CONDITIONS::ShowAlways );
    prefsMenu->AddItem( ACTIONS::showSymbolLibTable,    SELECTION_CONDITIONS::ShowAlways );
    prefsMenu->AddItem( ACTIONS::showFootprintLibTable, SELECTION_CONDITIONS::ShowAlways );
    prefsMenu->AddItem( wxID_PREFERENCES,
                        AddHotkeyName( _( "Preferences..." ), kicad_Manager_Hotkeys_Descr, HK_PREFERENCES ),
                        _( "Show preferences for all open tools" ),
                        preference_xpm,                 SELECTION_CONDITIONS::ShowAlways );

    prefsMenu->AddSeparator();
    Pgm().AddMenuLanguageList( prefsMenu );

    //-- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( toolsMenu, _( "&Tools" ) );
    menuBar->Append( prefsMenu, _( "&Preferences" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
    delete oldMenuBar;

    // Add the hotkey to the "show hotkey list" menu, because we do not have
    // a management of the keyboard keys in Kicad.
    // So all hotheys should be added to the menubar
    // Note Use wxMenuBar::SetLabel only after the menubar
    // has been associated with a frame. (see wxWidgets doc)
    msg = AddHotkeyName( menuBar->GetLabel( ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST ),
                         kicad_Manager_Hotkeys_Descr, HK_HELP );
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
                            _( "Open project directory in file explorer" ) );

    // Create m_mainToolBar
    m_mainToolBar->Realize();
}


void KICAD_MANAGER_FRAME::SyncMenusAndToolbars()
{
    m_mainToolBar->ToggleTool( ID_TO_SCH,            m_active_project );
    m_mainToolBar->ToggleTool( ID_TO_SCH_LIB_EDITOR, m_active_project );
    m_mainToolBar->ToggleTool( ID_TO_PCB,            m_active_project );
    m_mainToolBar->ToggleTool( ID_TO_PCB_FP_EDITOR,  m_active_project );
    m_mainToolBar->Refresh();
}


