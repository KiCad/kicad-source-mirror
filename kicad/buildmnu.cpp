/*****************************************************************************/
/**
 * @file buildmnu.cpp
 * @brief TODO
 */
/*****************************************************************************/

#include "wx/spinctrl.h"

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"


#include "kicad.h"
#include "macros.h"

#define BITMAP wxBitmap

#include "bitmaps.h"        // Common bitmaps
#include "id.h"


/*****************************************************************************/
BEGIN_EVENT_TABLE(WinEDA_MainFrame, WinEDA_BasicFrame)
/*****************************************************************************/
  /* Window events */
  EVT_SIZE(WinEDA_MainFrame::OnSize)
  EVT_CLOSE(WinEDA_MainFrame::OnCloseWindow)

  /* Sash drag events */
  EVT_SASH_DRAGGED(ID_LEFT_FRAME,
                   WinEDA_MainFrame::OnSashDrag)
  EVT_SASH_DRAGGED(ID_BOTTOM_FRAME,
                   WinEDA_MainFrame::OnSashDrag)
  EVT_SASH_DRAGGED(ID_MAIN_COMMAND,
                   WinEDA_MainFrame::OnSashDrag)

  /* Toolbar events */
  EVT_TOOL(ID_NEW_PROJECT,
           WinEDA_MainFrame::Process_Files)
  EVT_TOOL(ID_LOAD_PROJECT,
           WinEDA_MainFrame::Process_Files)
  EVT_TOOL(ID_SAVE_PROJECT,
           WinEDA_MainFrame::Process_Files)
  EVT_TOOL(ID_SAVE_AND_ZIP_FILES,
           WinEDA_MainFrame::Process_Files)

  /* Menu events */
  EVT_MENU(ID_SAVE_PROJECT,
           WinEDA_MainFrame::Process_Files)
  EVT_MENU(ID_EXIT,
           WinEDA_MainFrame::Process_Special_Functions)
  EVT_MENU(ID_TO_EDITOR,
           WinEDA_MainFrame::Process_Fct)
  EVT_MENU(ID_BROWSE_AN_SELECT_FILE,
           WinEDA_MainFrame::Process_Fct)
  EVT_MENU(ID_SELECT_PREFERED_EDITOR,
           WinEDA_MainFrame::Process_Preferences)
  EVT_MENU(ID_SELECT_DEFAULT_PDF_BROWSER,
           WinEDA_MainFrame::Process_Preferences)
  EVT_MENU(ID_SELECT_PREFERED_PDF_BROWSER,
           WinEDA_MainFrame::Process_Preferences)
  EVT_MENU(ID_SELECT_PREFERED_PDF_BROWSER_NAME,
           WinEDA_MainFrame::Process_Preferences)
  EVT_MENU(ID_SAVE_AND_ZIP_FILES,
           WinEDA_MainFrame::Process_Files)
  EVT_MENU(ID_READ_ZIP_ARCHIVE,
           WinEDA_MainFrame::Process_Files)
  EVT_MENU(ID_PROJECT_TREE_REFRESH,
           WinEDA_MainFrame::OnRefresh)
  EVT_MENU(ID_PREFERENCES_FONT_INFOSCREEN,
           WinEDA_MainFrame::Process_Preferences)
  EVT_MENU(ID_GENERAL_HELP,
           WinEDA_MainFrame::GetKicadHelp)
  EVT_MENU(ID_KICAD_ABOUT,
           WinEDA_MainFrame::GetKicadAbout)

  /* Range menu events */
  EVT_MENU_RANGE(ID_LANGUAGE_CHOICE,
                 ID_LANGUAGE_CHOICE_END,
                 WinEDA_MainFrame::SetLanguage)

  EVT_MENU_RANGE(ID_LOAD_PROJECT,
                 ID_LOAD_FILE_10,
                 WinEDA_MainFrame::Process_Files)

  /* Button events */
  EVT_BUTTON(ID_TO_PCB,
             WinEDA_MainFrame::Process_Fct)
  EVT_BUTTON(ID_TO_CVPCB,
             WinEDA_MainFrame::Process_Fct)
  EVT_BUTTON(ID_TO_EESCHEMA,
             WinEDA_MainFrame::Process_Fct)
  EVT_BUTTON(ID_TO_GERBVIEW,
             WinEDA_MainFrame::Process_Fct)


#ifdef KICAD_PYTHON
  EVT_BUTTON(ID_RUN_PYTHON, WinEDA_MainFrame::Process_Fct)
#endif


/*****************************************************************************/
END_EVENT_TABLE()
/*****************************************************************************/



/**
 * @brief TODO
 */
/*****************************************************************************/
void WinEDA_MainFrame::ReCreateMenuBar()
/*****************************************************************************/
{
  int ii, jj;
  wxMenuBar *menuBar = GetMenuBar() ;

  // Check if menubar is empty
  if( menuBar == NULL )
  {
    m_MenuBar = menuBar = new wxMenuBar();
    m_FilesMenu = new wxMenu;


    // Open project
    wxMenuItem *item = new wxMenuItem(m_FilesMenu,
                                      ID_LOAD_PROJECT,
                                      _("&Open"),
                                      _("Open an existing project") );
    item->SetBitmap(open_project_xpm);
    m_FilesMenu->Append(item);


    // New project
    item = new wxMenuItem(m_FilesMenu,
                          ID_NEW_PROJECT,
                          _("&New"),
                          _("Start a new project") );
    item->SetBitmap(new_project_xpm);
    m_FilesMenu->Append(item);


    // Save project
    item = new wxMenuItem(m_FilesMenu,
                          ID_SAVE_PROJECT,
                          _("&Save"),
                          _("Save current project") );
    item->SetBitmap(save_project_xpm);
    m_FilesMenu->Append(item);


    // Separator
    m_FilesMenu->AppendSeparator();
 

    // Archive project
    item = new wxMenuItem(m_FilesMenu,
                          ID_SAVE_AND_ZIP_FILES,
                          _("&Archive"),
                          _("Archive project files in zip archive") );
    item->SetBitmap(zip_xpm);
    m_FilesMenu->Append(item);


    // Unarchive project
    item = new wxMenuItem(m_FilesMenu,
                          ID_READ_ZIP_ARCHIVE,
                          _("&Unarchive"),
                          _("Unarchive project files from zip file") );
    item->SetBitmap(unzip_xpm);
    m_FilesMenu->Append(item);


    // Separator
    m_FilesMenu->AppendSeparator();


    // Exit
    item = new wxMenuItem(m_FilesMenu,
                          ID_EXIT,
                          _("E&xit"),
                          _("Quit kicad") );
    item->SetBitmap(exit_xpm);
    m_FilesMenu->Append(item);


    // Create last 10 project entries
    m_FilesMenu->AppendSeparator();
    for ( ii = 0; ii < 10; ii++ )
    {
      m_MenuBar = menuBar = new wxMenuBar();

      if ( GetLastProject(ii).IsEmpty() )
        break;

      m_FilesMenu->Append(ID_LOAD_FILE_1 + ii, GetLastProject(ii) );
    }

        
    /*************************************************************************/
    wxMenu *browseMenu = new wxMenu();
    /*************************************************************************/

    // Editor
    item = new wxMenuItem(browseMenu,          // Entry in menu
                          ID_TO_EDITOR,        // Entry ID
                          _("&Editor"),        // Entry text
                          _("Text editor") );  // Status bar text
    item->SetBitmap(editor_xpm);               // Entry XPM Bitmap
    browseMenu->Append(item);                  // Append wxMenuItem to menu


    // Browse files
    item = new wxMenuItem(browseMenu,
                          ID_BROWSE_AN_SELECT_FILE,
                          _("&Browse Files"),
                          _("Read or edit files") );
    item->SetBitmap(browse_files_xpm);
    browseMenu->Append(item);


    // Separator 
    browseMenu->AppendSeparator();


    // Select editor
    item = new wxMenuItem(browseMenu,
                          ID_SELECT_PREFERED_EDITOR,
                          _("&Select Editor"),
                          _("Select your prefered editor for file browsing") );
    item->SetBitmap(editor_xpm);
    browseMenu->Append(item);


    /*************************************************************************/
    wxMenu *PreferencesMenu = new wxMenu;
    /*************************************************************************/

    // Fonts
    item = new wxMenuItem(PreferencesMenu,
                          ID_PREFERENCES_FONT_INFOSCREEN,
                          _("Fonts"),
                          _("Font preferences"));
    item->SetBitmap(fonts_xpm);
    PreferencesMenu->Append(item);


    // Submenu Pdf Browser selection: system browser or user
    // selected browser (and its name)
    /*************************************************************************/
    wxMenu *SubMenuPdfBrowserChoice = new wxMenu;
    /*************************************************************************/

    // Default PDF viewer      
    item = new wxMenuItem(SubMenuPdfBrowserChoice,
                          ID_SELECT_DEFAULT_PDF_BROWSER,
                          _("Default PDF Viewer"),
            _("Use the default (system) PDF viewer used to browse datasheets"),
                            wxITEM_CHECK);
    SETBITMAPS(datasheet_xpm);
    SubMenuPdfBrowserChoice->Append(item);
    SubMenuPdfBrowserChoice->Check(ID_SELECT_DEFAULT_PDF_BROWSER,
    g_EDA_Appl->m_PdfBrowserIsDefault);


    // Favourite PDF viewer
    item = new wxMenuItem(SubMenuPdfBrowserChoice,
                          ID_SELECT_PREFERED_PDF_BROWSER,
                          _("Favourite PDF Viewer"),
                  _("Use your favourite PDF viewer used to browse datasheets"),
                          wxITEM_CHECK);
    SETBITMAPS(preference_xpm);
    SubMenuPdfBrowserChoice->Append(item);
    SubMenuPdfBrowserChoice->AppendSeparator();
    SubMenuPdfBrowserChoice->Check(ID_SELECT_PREFERED_PDF_BROWSER,
                                   !g_EDA_Appl->m_PdfBrowserIsDefault);


    item = new wxMenuItem(SubMenuPdfBrowserChoice,
                          ID_SELECT_PREFERED_PDF_BROWSER_NAME,
                          _("Select Pdf Viewer"),
              _("Select your favourite PDF viewer used to browse datasheets"));
    item->SetBitmap(datasheet_xpm);
    SubMenuPdfBrowserChoice->Append(item);

    ADD_MENUITEM_WITH_HELP_AND_SUBMENU(PreferencesMenu,
                                       SubMenuPdfBrowserChoice,
                                       -1,  _("Pdf Browser"),
                          wxT("Pdf Browser choice: default or user selection"),
                                       datasheet_xpm);

    PreferencesMenu->AppendSeparator();
    m_Parent->SetLanguageList(PreferencesMenu);


    /*************************************************************************/
    wxMenu *helpMenu = new wxMenu;
    /*************************************************************************/

    // Contents
    item = new wxMenuItem(helpMenu ,
                          ID_GENERAL_HELP,
                          _("&Contents"),
                          _("Open the kicad manual"));
    item->SetBitmap(help_xpm);
    helpMenu->Append(item);


    // About Kicad
    item = new wxMenuItem(helpMenu ,
                          ID_KICAD_ABOUT,
                          _("&About"),
                          _("About kicad project manager"));
    item->SetBitmap(info_xpm);
    helpMenu->Append(item);


    // Append menus to menuBar
    menuBar->Append(m_FilesMenu, _("&File"));
    menuBar->Append(browseMenu, _("&Browse"));
    menuBar->Append(PreferencesMenu, _("&Preferences"));
    menuBar->Append(helpMenu, _("&Help"));

    // Associate the menu bar with the frame
    SetMenuBar(menuBar);

  }
  else  // TODO (ENGLISH!) simple mise a jour de la liste des fichiers anciens
  {
    wxMenuItem * item;
    int max_file = m_Parent->m_LastProjectMaxCount;

    for ( ii = max_file-1; ii >=0 ; ii-- )
    {
      if( m_FilesMenu->FindItem(ID_LOAD_FILE_1 + ii) )
      {
        item = m_FilesMenu->Remove(ID_LOAD_FILE_1 + ii);

        if ( item )
          delete item;
      }
    }

    for ( jj = 0, ii = 0; ii < max_file; ii++ )
    {
      if (GetLastProject(ii).IsEmpty() )
        break;
      
      m_FilesMenu->Append(ID_LOAD_FILE_1 + jj, GetLastProject(ii) );
      jj++;

    }
  }
}



/**
 * @brief TODO
 */
/*****************************************************************************/
void WinEDA_MainFrame::RecreateBaseHToolbar()
/*****************************************************************************/
{
  // Check if toolbar is not already set
  if ( m_HToolBar != NULL )
    return;

  // Allocate memory for m_HToolBar
  m_HToolBar = new WinEDA_Toolbar(TOOLBAR_MAIN, this, ID_H_TOOLBAR, TRUE);
  SetToolBar(m_HToolBar);


  // Set up toolbar
  m_HToolBar->AddTool(ID_NEW_PROJECT,                    // Entry ID
                      BITMAP(new_project_xpm),           // XPM Bitmap
                      wxNullBitmap,
                      FALSE,
                      -1, -1,
                      (wxObject *) NULL,
                      _("Start a new project"));          // Tooltip


  // Load project
  m_HToolBar->AddTool(ID_LOAD_PROJECT,                   // Entry ID
                      BITMAP(open_project_xpm),          // XPM Bitmap
                      wxNullBitmap,
                      FALSE,
                      -1, -1, (wxObject *) NULL,
                      _("Load existing project"));       // Tooltip


  // Save project
  m_HToolBar->AddTool(ID_SAVE_PROJECT,                   // Entry ID
                      BITMAP(save_project_xpm),          // XPM Bitmap
                      wxNullBitmap,
                      FALSE,
                      -1, -1,
                      (wxObject *) NULL,
                      _( "Save current project" ));      // Tooltip


  // Separator
  m_HToolBar->AddSeparator();


  // Save and archive files
  m_HToolBar->AddTool(ID_SAVE_AND_ZIP_FILES,             // Entry ID
                      BITMAP(zip_xpm),                   // XPM Bitmap
                      wxNullBitmap,
                      FALSE,
                      -1, -1, (wxObject *) NULL,
                      _("Archive all project files"));   // Tooltip


  // Separator
  m_HToolBar->AddSeparator();


  // Refresh project tree
  m_HToolBar->AddTool(ID_PROJECT_TREE_REFRESH,           // Entry ID
                      BITMAP(reload_xpm),                // XPM Bitmap
                      wxNullBitmap,
                      FALSE,
                      -1, -1, (wxObject *) NULL,
                      _("Refresh project tree"));        // Tooltip


  m_HToolBar->Realize();  // Create m_HToolBar
}
