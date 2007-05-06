	/***********************************************/
	/*	buildmnu.h: construction du menu principal */
	/***********************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"

#include "wx/spinctrl.h"

#include "kicad.h"
#include "macros.h"

#define BITMAP wxBitmap

// ----------------------------------------------------------------------------
// resources
// ----------------------------------------------------------------------------

// USE_XPM_BITMAPS
#include "bitmaps.h"		// Common bitmaps

#include "zip.xpm"
#include "unzip.xpm"
#include "Browse_Files.xpm"
#include "Editor.xpm"
#include "New_Project.xpm"
#include "Open_Project.xpm"

#include "id.h"

/* Fonctions locales */

/* Variables locales */


BEGIN_EVENT_TABLE(WinEDA_MainFrame, WinEDA_BasicFrame)
	EVT_SIZE(WinEDA_MainFrame::OnSize)
	EVT_CLOSE(WinEDA_MainFrame::OnCloseWindow)
	EVT_SASH_DRAGGED(ID_LEFT_FRAME, WinEDA_MainFrame::OnSashDrag)
	EVT_SASH_DRAGGED(ID_BOTTOM_FRAME, WinEDA_MainFrame::OnSashDrag)
	EVT_SASH_DRAGGED(ID_MAIN_COMMAND, WinEDA_MainFrame::OnSashDrag)

	EVT_MENU_RANGE(ID_LOAD_PROJECT,ID_LOAD_FILE_10,
		WinEDA_MainFrame::Process_Files)
	EVT_MENU(ID_SAVE_PROJECT, WinEDA_MainFrame::Process_Files)

	EVT_TOOL(ID_NEW_PROJECT, WinEDA_MainFrame::Process_Files)
	EVT_TOOL(ID_LOAD_PROJECT, WinEDA_MainFrame::Process_Files)
	EVT_TOOL(ID_SAVE_PROJECT, WinEDA_MainFrame::Process_Files)
	EVT_TOOL(ID_SAVE_AND_ZIP_FILES, WinEDA_MainFrame::Process_Files)

	EVT_MENU(ID_EXIT, WinEDA_MainFrame::Process_Special_Functions)

	EVT_MENU(ID_TO_EDITOR, WinEDA_MainFrame::Process_Fct)
	EVT_MENU(ID_BROWSE_AN_SELECT_FILE, WinEDA_MainFrame::Process_Fct)
	EVT_MENU(ID_SELECT_PREFERED_EDITOR, WinEDA_MainFrame::Process_Preferences)
	EVT_MENU(ID_SELECT_DEFAULT_PDF_BROWSER, WinEDA_MainFrame::Process_Preferences)
	EVT_MENU(ID_SELECT_PREFERED_PDF_BROWSER, WinEDA_MainFrame::Process_Preferences)
	EVT_MENU(ID_SELECT_PREFERED_PDF_BROWSER_NAME, WinEDA_MainFrame::Process_Preferences)
	EVT_MENU(ID_SAVE_AND_ZIP_FILES, WinEDA_MainFrame::Process_Files)
	EVT_MENU(ID_READ_ZIP_ARCHIVE, WinEDA_MainFrame::Process_Files)

	EVT_MENU(ID_PREFERENCES_FONT_INFOSCREEN, WinEDA_MainFrame::Process_Preferences)

	EVT_MENU_RANGE(ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END,
		WinEDA_MainFrame::SetLanguage)


	EVT_MENU(ID_GENERAL_HELP, WinEDA_MainFrame::GetKicadHelp)
	EVT_MENU(ID_KICAD_ABOUT, WinEDA_MainFrame::GetKicadAbout)

	EVT_BUTTON(ID_TO_PCB, WinEDA_MainFrame::Process_Fct)
	EVT_BUTTON(ID_TO_CVPCB, WinEDA_MainFrame::Process_Fct)
	EVT_BUTTON(ID_TO_EESCHEMA, WinEDA_MainFrame::Process_Fct)
	EVT_BUTTON(ID_TO_GERBVIEW, WinEDA_MainFrame::Process_Fct)

END_EVENT_TABLE()


/*******************************************/
void WinEDA_MainFrame::ReCreateMenuBar(void)
/*******************************************/
{
int ii, jj;
wxMenuBar * menuBar = GetMenuBar() ;

	if( menuBar == NULL )
	{
		m_MenuBar = menuBar = new wxMenuBar();

		m_FilesMenu = new wxMenu;
		wxMenuItem *item = new wxMenuItem(m_FilesMenu, ID_LOAD_PROJECT,
					 _("&Open Project Descr"),
					 _("Select an existing project descriptor") );
		item->SetBitmap(open_project_xpm);
		m_FilesMenu->Append(item);

		item = new wxMenuItem(m_FilesMenu, ID_NEW_PROJECT,
					 _("&New Project Descr"),
					 _("Create new project descriptor") );
	    item->SetBitmap(new_project_xpm);
		m_FilesMenu->Append(item);

		item = new wxMenuItem(m_FilesMenu, ID_SAVE_PROJECT,
					 _("&Save Project Descr"),
					 _("Save current project descriptor") );
	    item->SetBitmap(save_project_xpm);
		m_FilesMenu->Append(item);

		m_FilesMenu->AppendSeparator();
		item = new wxMenuItem(m_FilesMenu, ID_SAVE_AND_ZIP_FILES,
					 _("Save &Project Files"),
					 _("Save and Zip all project files") );
	    item->SetBitmap(zip_xpm);
		m_FilesMenu->Append(item);
		item = new wxMenuItem(m_FilesMenu, ID_READ_ZIP_ARCHIVE,
					 _("&Unzip Archive"),
					 _("UnZip archive file") );
	    item->SetBitmap(unzip_xpm);
		m_FilesMenu->Append(item);

		m_FilesMenu->AppendSeparator();
		item = new wxMenuItem(m_FilesMenu, ID_EXIT, _("E&xit"), _("Quit Kicad") );
	    item->SetBitmap(exit_xpm);
		m_FilesMenu->Append(item);

		// Creation des selections des anciens fichiers
		m_FilesMenu->AppendSeparator();
		for ( ii = 0; ii < 10; ii++ )
		{
			if ( GetLastProject(ii).IsEmpty() ) break;
			m_FilesMenu->Append(ID_LOAD_FILE_1 + ii, GetLastProject(ii) );
		}

		// Menu Browse
		wxMenu *browseMenu = new wxMenu();
		item = new wxMenuItem(browseMenu, ID_TO_EDITOR,
				_("&Editor"), _("Text editor") );
	    item->SetBitmap(editor_xpm);
		browseMenu->Append(item);
		item = new wxMenuItem(browseMenu, ID_BROWSE_AN_SELECT_FILE,
				_("&Browse Files"), _("Read or edit files") );
	    item->SetBitmap(browse_files_xpm);
		browseMenu->Append(item);
		browseMenu->AppendSeparator();
		item = new wxMenuItem(browseMenu, ID_SELECT_PREFERED_EDITOR,
				_("&Select Editor"), _("Select your prefered editor for file browsing") );
	    item->SetBitmap(editor_xpm);
		browseMenu->Append(item);

		// Preferences menu:
		wxMenu *PreferencesMenu = new wxMenu;
		item = new wxMenuItem(PreferencesMenu , ID_PREFERENCES_FONT_INFOSCREEN,
				_("Select Fonts"), _("Select Fonts and  Font sizes"));
	    item->SetBitmap(fonts_xpm);
		PreferencesMenu->Append(item);

		// Submenu Pdf Browser selection: system browser or user selected browser (and its name)
		wxMenu *SubMenuPdfBrowserChoice = new wxMenu;
		item = new wxMenuItem(SubMenuPdfBrowserChoice , ID_SELECT_DEFAULT_PDF_BROWSER,
				_("Default Pdf Viewer"), _("Use the default (system) PDF viewer used to browse datasheets"),
				wxITEM_CHECK);
	    SETBITMAPS(datasheet_xpm);
		SubMenuPdfBrowserChoice->Append(item);
		SubMenuPdfBrowserChoice->Check(ID_SELECT_DEFAULT_PDF_BROWSER,
			EDA_Appl->m_PdfBrowserIsDefault);
		item = new wxMenuItem(SubMenuPdfBrowserChoice , ID_SELECT_PREFERED_PDF_BROWSER,
				_("Favourite Pdf Viewer"), _("Use your favourite PDF viewer used to browse datasheets"),
				wxITEM_CHECK);
	    SETBITMAPS(preference_xpm);
		SubMenuPdfBrowserChoice->Append(item);
		SubMenuPdfBrowserChoice->AppendSeparator();
		SubMenuPdfBrowserChoice->Check(ID_SELECT_PREFERED_PDF_BROWSER,
			!EDA_Appl->m_PdfBrowserIsDefault);
		item = new wxMenuItem(SubMenuPdfBrowserChoice , ID_SELECT_PREFERED_PDF_BROWSER_NAME,
				_("Select Pdf Viewer"), _("Select your favourite PDF viewer used to browse datasheets"));
	    item->SetBitmap(datasheet_xpm);
		SubMenuPdfBrowserChoice->Append(item);
		ADD_MENUITEM_WITH_HELP_AND_SUBMENU(PreferencesMenu, SubMenuPdfBrowserChoice,
			-1,  _("Pdf Browser"),
			wxT("Pdf Browser choice: default or user selection"),
			datasheet_xpm);

		PreferencesMenu->AppendSeparator();
		m_Parent->SetLanguageList(PreferencesMenu);


		// Menu Help:
		wxMenu *helpMenu = new wxMenu;
		item = new wxMenuItem(helpMenu , ID_GENERAL_HELP,
				_("Kicad &Help"), _("On line doc"));
	    item->SetBitmap(help_xpm);
		helpMenu->Append(item);

		item = new wxMenuItem(helpMenu , ID_KICAD_ABOUT,
				_("Kicad &About"), _("Kicad Infos"));
	    item->SetBitmap(info_xpm);
		helpMenu->Append(item);


		menuBar->Append(m_FilesMenu, _("&Projects"));
		menuBar->Append(browseMenu, _("&Browse"));
		menuBar->Append(PreferencesMenu, _("&Preferences"));
		menuBar->Append(helpMenu, _("&Help"));

		// Associate the menu bar with the frame
		SetMenuBar(menuBar);
	}
	else		// simple mise a jour de la liste des fichiers anciens
	{
		wxMenuItem * item;
		int max_file = m_Parent->m_LastProjectMaxCount;
		for ( ii = max_file-1; ii >=0 ; ii-- )
		{
			if( m_FilesMenu->FindItem(ID_LOAD_FILE_1 + ii) )
			{
				item = m_FilesMenu->Remove(ID_LOAD_FILE_1 + ii);
				if ( item ) delete item;
			}
		}
		for ( jj = 0, ii = 0; ii < max_file; ii++ )
		{
			if (GetLastProject(ii).IsEmpty() ) break;
			m_FilesMenu->Append(ID_LOAD_FILE_1 + jj, GetLastProject(ii) );
			jj++;
		}
	}
}


/***************************************************/
void WinEDA_MainFrame::RecreateBaseHToolbar(void)
/***************************************************/
{
	if ( m_HToolBar != NULL ) return;

	m_HToolBar = new WinEDA_Toolbar(TOOLBAR_MAIN, this, ID_H_TOOLBAR, TRUE);
	SetToolBar(m_HToolBar);

	// Set up toolbar
	m_HToolBar->AddTool(ID_NEW_PROJECT, BITMAP(new_project_xpm),
				wxNullBitmap, FALSE,
				-1, -1, (wxObject *) NULL,
				_("Create new project descriptor"));

	m_HToolBar->AddTool(ID_LOAD_PROJECT, BITMAP(open_project_xpm),
					wxNullBitmap, FALSE,
					-1, -1, (wxObject *) NULL,
					_("Select an existing project descriptor"));

	m_HToolBar->AddTool(ID_SAVE_PROJECT, BITMAP(save_project_xpm),
					wxNullBitmap, FALSE,
					-1, -1, (wxObject *) NULL,
					_("Save current project descriptor"));

	m_HToolBar->AddSeparator();
	m_HToolBar->AddTool(ID_SAVE_AND_ZIP_FILES, BITMAP(zip_xpm),
					wxNullBitmap, FALSE,
					-1, -1, (wxObject *) NULL,
					_("Archive all project files"));


	// after adding the buttons to the toolbar, must call Realize() to reflect
	// the changes
	m_HToolBar->Realize();
}

/*************************************************/
void WinEDA_MainFrame::CreateCommandToolbar(void)
/*************************************************/
{
#define SEPAR 20
int sizex, sizey,width;
wxBitmapButton * Butt;
wxPoint pos;

	// delete and recreate the toolbar
	if( m_VToolBar ) return;

	m_CommandWin->GetClientSize(&sizex, &sizey);
	width = 300;

	// Set up toolbar
	width = 32;
	pos.x = 20; pos.y = 20;

	Butt = new wxBitmapButton(m_CommandWin, ID_TO_EESCHEMA,
				BITMAP(icon_eeschema_xpm), pos );
	Butt->SetToolTip(_("EeSchema (Schematic editor)"));

	pos.x += width + SEPAR;
	Butt = new wxBitmapButton(m_CommandWin,ID_TO_CVPCB,
				BITMAP(icon_cvpcb_xpm), pos );
	Butt->SetToolTip(_("Cvpcb (Componants to modules)"));

	pos.x += width + SEPAR;
	Butt = new wxBitmapButton(m_CommandWin, ID_TO_PCB,
				BITMAP(a_icon_pcbnew_xpm), pos );
	Butt->SetToolTip(_("Pcbnew ( board editor )"));

	pos.x += width + SEPAR;
	Butt = new wxBitmapButton(m_CommandWin, ID_TO_GERBVIEW,
				BITMAP(icon_gerbview_xpm), pos );
	Butt->SetToolTip(_("GerbView ( Gerber viewer )"));

}


