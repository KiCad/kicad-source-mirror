/*****************************************************************************/

/**
 * @file buildmnu.cpp
 * @brief TODO
 */
/*****************************************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "kicad.h"
#include "macros.h"
#include "bitmaps.h"        // Common bitmaps


/*****************************************************************************/
BEGIN_EVENT_TABLE( WinEDA_MainFrame, WinEDA_BasicFrame )
/* Window events */
    EVT_SIZE( WinEDA_MainFrame::OnSize )
    EVT_CLOSE( WinEDA_MainFrame::OnCloseWindow )
/* Sash drag events */
    EVT_SASH_DRAGGED( ID_LEFT_FRAME, WinEDA_MainFrame::OnSashDrag )
/* Toolbar events */
    EVT_TOOL( ID_NEW_PROJECT, WinEDA_MainFrame::OnLoadProject )
    EVT_TOOL( ID_LOAD_PROJECT, WinEDA_MainFrame::OnLoadProject )
    EVT_TOOL( ID_SAVE_PROJECT, WinEDA_MainFrame::OnSaveProject )
    EVT_TOOL( ID_SAVE_AND_ZIP_FILES, WinEDA_MainFrame::OnArchiveFiles )

/* Menu events */
    EVT_MENU( ID_SAVE_PROJECT, WinEDA_MainFrame::OnSaveProject )
    EVT_MENU( wxID_EXIT, WinEDA_MainFrame::OnExit )
    EVT_MENU( ID_TO_EDITOR, WinEDA_MainFrame::OnOpenTextEditor )
    EVT_MENU( ID_BROWSE_AN_SELECT_FILE,
              WinEDA_MainFrame::OnOpenFileInTextEditor )
    EVT_MENU( ID_SELECT_PREFERED_EDITOR,
              WinEDA_MainFrame::OnSelectPreferredEditor )
    EVT_MENU( ID_SELECT_DEFAULT_PDF_BROWSER,
              WinEDA_MainFrame::OnSelectDefaultPdfBrowser )
    EVT_MENU( ID_SELECT_PREFERED_PDF_BROWSER,
              WinEDA_MainFrame::OnSelectPreferredPdfBrowser )
    EVT_MENU( ID_SELECT_PREFERED_PDF_BROWSER_NAME,
              WinEDA_MainFrame::OnSelectPreferredPdfBrowser )
    EVT_MENU( ID_SAVE_AND_ZIP_FILES, WinEDA_MainFrame::OnArchiveFiles )
    EVT_MENU( ID_READ_ZIP_ARCHIVE, WinEDA_MainFrame::OnUnarchiveFiles )
    EVT_MENU( ID_PROJECT_TREE_REFRESH, WinEDA_MainFrame::OnRefresh )
    EVT_MENU( ID_GENERAL_HELP, WinEDA_MainFrame::GetKicadHelp )
    EVT_MENU( ID_KICAD_ABOUT, WinEDA_MainFrame::GetKicadAbout )

/* Range menu events */
    EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END,
                    WinEDA_MainFrame::SetLanguage )

    EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, WinEDA_MainFrame::OnFileHistory )

/* Button events */
    EVT_BUTTON( ID_TO_PCB, WinEDA_MainFrame::OnRunPcbNew )
    EVT_BUTTON( ID_TO_CVPCB, WinEDA_MainFrame::OnRunCvpcb )
    EVT_BUTTON( ID_TO_EESCHEMA, WinEDA_MainFrame::OnRunEeschema )
    EVT_BUTTON( ID_TO_GERBVIEW, WinEDA_MainFrame::OnRunGerbview )

    EVT_UPDATE_UI( ID_SELECT_DEFAULT_PDF_BROWSER,
                   WinEDA_MainFrame::OnUpdateDefaultPdfBrowser )
    EVT_UPDATE_UI( ID_SELECT_PREFERED_PDF_BROWSER,
                   WinEDA_MainFrame::OnUpdatePreferredPdfBrowser )

#ifdef KICAD_PYTHON
    EVT_BUTTON( ID_RUN_PYTHON, WinEDA_MainFrame::OnRunPythonScript )
#endif

END_EVENT_TABLE()


/**
 * @brief TODO
 */
void WinEDA_MainFrame::ReCreateMenuBar()
{
    wxMenuItem *item;
    wxMenuBar  *menuBar = GetMenuBar();

    /* Destroy the existing menu bar so it can be rebuilt.  This allows
     * language changes of the menu text on the fly. */
    if( menuBar )
        SetMenuBar( NULL );

    menuBar = new wxMenuBar();

    // Check if menubar is empty
    wxMenu* filesMenu = new wxMenu;

    // Open project
    item = new wxMenuItem( filesMenu, ID_LOAD_PROJECT, _( "&Open\tCtrl+O" ),
                           _( "Open an existing project" ) );
    item->SetBitmap( open_project_xpm );
    filesMenu->Append( item );

    // New project
    item = new wxMenuItem( filesMenu, ID_NEW_PROJECT, _( "&New\tCtrl+N" ),
                           _( "Start a new project" ) );
    item->SetBitmap( new_project_xpm );
    filesMenu->Append( item );


    // Save project
    item = new wxMenuItem( filesMenu, ID_SAVE_PROJECT, _( "&Save\tCtrl+S" ),
                           _( "Save current project" ) );
    item->SetBitmap( save_project_xpm );
    filesMenu->Append( item );

    // Separator
    filesMenu->AppendSeparator();


    // Archive project
    item = new wxMenuItem( filesMenu, ID_SAVE_AND_ZIP_FILES, _( "&Archive" ),
                           _( "Archive project files in zip archive" ) );
    item->SetBitmap( zip_xpm );
    filesMenu->Append( item );

    // Unarchive project
    item = new wxMenuItem( filesMenu, ID_READ_ZIP_ARCHIVE, _( "&Unarchive" ),
                           _( "Unarchive project files from zip file" ) );
    item->SetBitmap( unzip_xpm );
    filesMenu->Append( item );

    // Separator
    filesMenu->AppendSeparator();

    /* Quit on all platforms except WXMAC */
#if !defined( __WXMAC__ )

    filesMenu->AppendSeparator();
    item = new wxMenuItem( filesMenu, wxID_EXIT, _( "&Quit" ),
                          _( "Quit KiCad" ) );
    item->SetBitmap( exit_xpm );
    filesMenu->Append( item );

#endif /* !defined( __WXMAC__ ) */

    /* Add the file history */
    wxGetApp().m_fileHistory.AddFilesToMenu( filesMenu );

    /**********************************************************************/
    wxMenu* browseMenu = new wxMenu();
    /**********************************************************************/

    // Editor
    item = new wxMenuItem( browseMenu, ID_TO_EDITOR, _( "Text E&ditor" ),
                           _( "Open prefered text editor" ) );
    item->SetBitmap( editor_xpm );
    browseMenu->Append( item );

    // Browse files
    item = new wxMenuItem( browseMenu, ID_BROWSE_AN_SELECT_FILE,
                           _( "&Browse Files" ),
                           _( "Read or edit files with text editor" ) );
    item->SetBitmap( browse_files_xpm );
    browseMenu->Append( item );

    /**********************************************************************/
    wxMenu* PreferencesMenu = new wxMenu;
    /**********************************************************************/

    // Prefered text editor
    item = new wxMenuItem( PreferencesMenu, ID_SELECT_PREFERED_EDITOR,
                           _( "&Text Editor" ),
                           _( "Select your prefered text editor" ) );
    item->SetBitmap( editor_xpm );
    PreferencesMenu->Append( item );

    // Submenu Pdf Browser selection: system browser or user
    // selected browser (and its name)
    /**********************************************************************/
    wxMenu* SubMenuPdfBrowserChoice = new wxMenu;
    /**********************************************************************/

    // Default PDF viewer
    item = new wxMenuItem( SubMenuPdfBrowserChoice,
                           ID_SELECT_DEFAULT_PDF_BROWSER,
                           _( "Default PDF Viewer" ),
                           _( "Use the default (system) PDF viewer used to browse datasheets" ),
                           wxITEM_CHECK );
    SETBITMAPS( datasheet_xpm );
    SubMenuPdfBrowserChoice->Append( item );
    SubMenuPdfBrowserChoice->Check( ID_SELECT_DEFAULT_PDF_BROWSER,
                                    wxGetApp().m_PdfBrowserIsDefault );


    // Favourite PDF viewer
    item = new wxMenuItem( SubMenuPdfBrowserChoice,
                           ID_SELECT_PREFERED_PDF_BROWSER,
                           _( "Favourite PDF Viewer" ),
                           _( "Use your favourite PDF viewer used to browse datasheets" ),
                           wxITEM_CHECK );
    SETBITMAPS( preference_xpm );
    SubMenuPdfBrowserChoice->Append( item );
    SubMenuPdfBrowserChoice->AppendSeparator();
    SubMenuPdfBrowserChoice->Check( ID_SELECT_PREFERED_PDF_BROWSER,
                                    !wxGetApp().m_PdfBrowserIsDefault );


    item = new wxMenuItem( SubMenuPdfBrowserChoice,
                           ID_SELECT_PREFERED_PDF_BROWSER_NAME,
                           _( "Select Pdf Viewer" ),
                           _( "Select your favourite PDF viewer used to browse datasheets" ) );
    item->SetBitmap( datasheet_xpm );
    SubMenuPdfBrowserChoice->Append( item );

    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( PreferencesMenu,
                                        SubMenuPdfBrowserChoice,
                                        -1, _( "Pdf Viewer" ),
                                        _( "Pdf viewer preferences" ),
                                        datasheet_xpm );

    PreferencesMenu->AppendSeparator();
    wxGetApp().AddMenuLanguageList( PreferencesMenu );


    /**********************************************************************/
    wxMenu* helpMenu = new wxMenu;
    /**********************************************************************/

    // Contents
    item = new wxMenuItem( helpMenu, ID_GENERAL_HELP, _( "&Contents" ),
                           _( "Open the kicad manual" ) );
    item->SetBitmap( help_xpm );
    helpMenu->Append( item );

    // About on all platforms except WXMAC */
#if !defined( __WXMAC__ )

    helpMenu->AppendSeparator();
    item = new wxMenuItem( helpMenu, ID_KICAD_ABOUT, _( "&About" ),
                           _( "About kicad project manager" ) );
    item->SetBitmap( info_xpm );
    helpMenu->Append( item );

#endif /* !defined( __WXMAC__ ) */

    // Append menus to menuBar
    menuBar->Append( filesMenu, _( "&File" ) );
    menuBar->Append( browseMenu, _( "&Browse" ) );
    menuBar->Append( PreferencesMenu, _( "&Preferences" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    SetMenuBar( menuBar );
}


/**
 * @brief TODO
 */
void WinEDA_MainFrame::RecreateBaseHToolbar()
{
    // Check if toolbar is not already set
    if( m_HToolBar != NULL )
        return;

    // Allocate memory for m_HToolBar
    m_HToolBar = new WinEDA_Toolbar( TOOLBAR_MAIN, this, ID_H_TOOLBAR, TRUE );
#if !defined(KICAD_AUIMANAGER)
    SetToolBar( (wxToolBar*)m_HToolBar );
#endif
    // Set up toolbar
    m_HToolBar->AddTool( ID_NEW_PROJECT, wxEmptyString,
                         wxBitmap( new_project_xpm ),
                         _( "Start a new project" ) );

    // Load project
    m_HToolBar->AddTool( ID_LOAD_PROJECT, wxEmptyString,
                         wxBitmap( open_project_xpm ),
                         _( "Load existing project" ) );

    // Save project
    m_HToolBar->AddTool( ID_SAVE_PROJECT, wxEmptyString,
                         wxBitmap( save_project_xpm ),
                         _( "Save current project" ) );

    // Separator
    m_HToolBar->AddSeparator();

    // Save and archive files
    m_HToolBar->AddTool( ID_SAVE_AND_ZIP_FILES, wxEmptyString,
                         wxBitmap( zip_xpm ),
                         _( "Archive all project files" ) ); // Tooltip

    // Separator
    m_HToolBar->AddSeparator();

    // Refresh project tree
    m_HToolBar->AddTool( ID_PROJECT_TREE_REFRESH, wxEmptyString,
                         wxBitmap( reload_xpm ),
                         _( "Refresh project tree" ) );

    m_HToolBar->Realize(); // Create m_HToolBar
}
