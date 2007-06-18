	/***************************************************/
	/*	tool_cvpcb.cpp: construction du menu principal */
	/***************************************************/

#include "fctsys.h"
#include "gr_basic.h"


#include "common.h"
#include "cvpcb.h"
#include "trigo.h"

#include "protos.h"

#define BITMAP wxBitmap

#include "bitmaps.h"
#include "delete_association.xpm"
#include "module_filtered_list.xpm"
#include "module_full_list.xpm"

#include "id.h"


/*********************************************/
void WinEDA_CvpcbFrame::ReCreateHToolbar(void)
/*********************************************/
{
	if ( m_HToolBar != NULL ) return;

 	m_HToolBar = new WinEDA_Toolbar(TOOLBAR_MAIN, this, ID_H_TOOLBAR, TRUE);
  	SetToolBar(m_HToolBar);

 	m_HToolBar->AddTool(ID_CVPCB_READ_INPUT_NETLIST, BITMAP(netlist_xpm),
					_("Open Netlist"));

 	m_HToolBar->AddTool(ID_CVPCB_SAVEQUITCVPCB, BITMAP(save_netlist_xpm),
					_("Save Nelist and Cmp list"));

 	m_HToolBar->AddSeparator();
 	m_HToolBar->AddTool(ID_CVPCB_CREATE_CONFIGWINDOW, BITMAP(config_xpm),
					_("Configuration"));

	m_HToolBar->AddSeparator();
	m_HToolBar->AddTool(ID_CVPCB_CREATE_SCREENCMP, BITMAP(module_xpm),
					_("View selected part"));

	m_HToolBar->AddTool(ID_CVPCB_AUTO_ASSOCIE, BITMAP(auto_associe_xpm),
					_("Automatic Association"));

 	m_HToolBar->AddSeparator();
 	m_HToolBar->AddTool(ID_CVPCB_GOTO_PREVIOUSNA, BITMAP(left_xpm),
					_("Select previous free component"));

	m_HToolBar->AddTool(ID_CVPCB_GOTO_FIRSTNA, BITMAP(right_xpm),
					_("Select next free component"));

	m_HToolBar->AddSeparator();
	m_HToolBar->AddTool(ID_CVPCB_DEL_ASSOCIATIONS, BITMAP(delete_association_xpm),
					_("Delete all associations"));

	m_HToolBar->AddSeparator();
 	m_HToolBar->AddTool(ID_CVPCB_CREATE_STUFF_FILE, BITMAP(save_cmpstuff_xpm),
					_("Create stuff file (component/module list)"));

	m_HToolBar->AddSeparator();
 	m_HToolBar->AddTool(ID_PCB_DISPLAY_FOOTPRINT_DOC, BITMAP(file_footprint_xpm),
					_("Display/print component documentation (footprint.pdf)"));

	m_HToolBar->AddSeparator();
	m_HToolBar->AddSeparator();
	m_HToolBar->AddRadioTool(ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST,
		wxEmptyString, BITMAP(module_filtered_list_xpm),
		wxNullBitmap,
		_("Display the filtered footprint list for the current component"));
	m_HToolBar->AddRadioTool(ID_CVPCB_FOOTPRINT_DISPLAY_FULL_LIST,
		wxEmptyString, BITMAP(module_full_list_xpm),
		wxNullBitmap,
		_("Display the full footprint list (without filtering)"));

	if( m_Parent->m_EDA_Config )
	{
		wxString key = wxT(FILTERFOOTPRINTKEY);
		int opt = m_Parent->m_EDA_Config->Read(key, (long)1);
		m_HToolBar->ToggleTool(ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST, opt);
		m_HToolBar->ToggleTool(ID_CVPCB_FOOTPRINT_DISPLAY_FULL_LIST, ! opt);
	}

	// after adding the buttons to the toolbar, must call Realize() to reflect
	// the changes
	m_HToolBar->Realize();
}


/*******************************************/
void WinEDA_CvpcbFrame::ReCreateMenuBar(void)
/*******************************************/
/* Creation des menus de la fenetre principale
*/
{
int ii;
wxMenuBar * menuBar = GetMenuBar();

	if( menuBar == NULL )
		{
		menuBar = new wxMenuBar();
		// Associate the menu bar with the frame
		SetMenuBar(menuBar);

		m_FilesMenu = new wxMenu;
		wxMenuItem *item = new wxMenuItem(m_FilesMenu, ID_LOAD_PROJECT,
					 _("&Load Netlist File"),
					 _("Load a Netlist") );
	    item->SetBitmap(open_xpm);
		m_FilesMenu->Append(item);

		m_FilesMenu->AppendSeparator();
		item = new wxMenuItem(m_FilesMenu, ID_SAVE_PROJECT,
					 _("&Save Netlist"),
					 _("Save New Netlist and Cmp file") );
	    item->SetBitmap(save_netlist_xpm);
		m_FilesMenu->Append(item);

		m_FilesMenu->AppendSeparator();
		item = new wxMenuItem(m_FilesMenu, ID_CVPCB_QUIT, _("E&xit"), _("Quit Cvpcb" ));
	    item->SetBitmap(exit_xpm);
		m_FilesMenu->Append(item);

// Creation des selections des anciens fichiers
		m_FilesMenu->AppendSeparator();
		for ( ii = 0; ii < 10; ii++ )
		{
			if ( GetLastProject(ii).IsEmpty() ) break;
			m_FilesMenu->Append(ID_LOAD_FILE_1 + ii, GetLastProject(ii) );
		}

		// Menu Configuration:
		wxMenu * configmenu = new wxMenu;
		item = new wxMenuItem(configmenu, ID_CONFIG_REQ, _("&Configuration"),
			_("Setting Libraries, Directories and others..."));
	    item->SetBitmap(config_xpm);
		configmenu->Append(item);

		// Font selection and setup
		AddFontSelectionMenu(configmenu);
		
		m_Parent->SetLanguageList(configmenu);

		configmenu->AppendSeparator();
		item = new wxMenuItem(configmenu, ID_CONFIG_SAVE,
                        _("&Save config"),
                        _("Save configuration in current dir"));
	    item->SetBitmap(save_setup_xpm);
		configmenu->Append(item);

		// Menu Help:
		wxMenu *helpMenu = new wxMenu;
		item = new wxMenuItem(helpMenu , ID_CVPCB_DISPLAY_HELP, _("&Contents"),
                        _("Open the cvpcb manual"));
	    item->SetBitmap(help_xpm);
		helpMenu->Append(item);
		item = new wxMenuItem(helpMenu , ID_CVPCB_DISPLAY_LICENCE, _("&About"),
                        _("About this application"));
	    item->SetBitmap(info_xpm);
		helpMenu->Append(item);

		menuBar->Append(m_FilesMenu, _("&File"));
		menuBar->Append(configmenu, _("&Preferences"));
		menuBar->Append(helpMenu, _("&Help"));
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
		for ( ii = 0; ii < max_file; ii++ )
			{
			if ( GetLastProject(ii).IsEmpty() ) break;
			m_FilesMenu->Append(ID_LOAD_FILE_1 + ii, GetLastProject(ii) );
			}
		}
}

