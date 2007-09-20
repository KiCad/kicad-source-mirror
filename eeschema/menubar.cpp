	/*************************************************************/
	/* menubar.cpp  - create the menubar for the schematic frame */
	/*************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "common.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "bitmaps.h"
#include "protos.h"
#include "id.h"
#include "hotkeys.h"


/************************************************/
void WinEDA_SchematicFrame::ReCreateMenuBar()
/************************************************/
/* create or update the menubar for the schematic frame
*/
{
int ii;
wxMenuBar * menuBar = GetMenuBar();
wxString msg;

	if( menuBar == NULL )
	{
		menuBar = new wxMenuBar();
			
		m_FilesMenu = new wxMenu;

		wxMenuItem *item = new wxMenuItem(m_FilesMenu, ID_LOAD_PROJECT,
						_("&Load Schematic Project"),
						_("Load a schematic project (Schematic, libraries...)") );
	    item->SetBitmap(open_xpm);
		m_FilesMenu->Append(item);

		item = new wxMenuItem(m_FilesMenu, ID_LOAD_ONE_SHEET,
						_("&Reload the current sheet"),
						_("Load or reload a schematic file from file into the current sheet") );
	    item->SetBitmap(import_xpm);
		m_FilesMenu->Append(item);

		m_FilesMenu->AppendSeparator();
		item = new wxMenuItem(m_FilesMenu,ID_SAVE_PROJECT,
					 _("&Save Schematic Project"),
					 _("Save all") );
	    item->SetBitmap(save_project_xpm);
		m_FilesMenu->Append(item);

		m_FilesMenu->AppendSeparator();
		item = new wxMenuItem(m_FilesMenu, ID_SAVE_ONE_SHEET,
					 _("Save &Current sheet"),
					 _("Save current sheet only") );
	    item->SetBitmap(save_xpm);
		m_FilesMenu->Append(item);

		item = new wxMenuItem(m_FilesMenu, ID_SAVE_ONE_SHEET_AS,
					 _("Save Current sheet &as.."),
					 _("Save current sheet as..") );
	    item->SetBitmap(save_as_xpm);
		m_FilesMenu->Append(item);

		/* Plot Menu */
		item = new wxMenuItem(m_FilesMenu, ID_GEN_PRINT,
					_("P&rint"), _("Print on current printer") );
	    item->SetBitmap(print_button);
		m_FilesMenu->Append(item);

		wxMenu *choice_plot_fmt = new wxMenu;
		item = new wxMenuItem(choice_plot_fmt, ID_GEN_PLOT_PS,
			_("Plot PostScript"), _("Plotting in PostScript format") );
	    item->SetBitmap(plot_PS_xpm);
		choice_plot_fmt->Append(item);

		item = new wxMenuItem(choice_plot_fmt, ID_GEN_PLOT_HPGL,
			_("Plot HPGL"), _("Plotting in HPGL format") );
	    item->SetBitmap(plot_HPG_xpm);
		choice_plot_fmt->Append(item);

		item = new wxMenuItem(choice_plot_fmt, ID_GEN_PLOT_SVG,
			_("Plot SVG"), _("Plotting in SVG format") );
	    item->SetBitmap(plot_xpm);
		choice_plot_fmt->Append(item);

#ifdef __WINDOWS__
		/* Under windows, one can draw to the clipboard */
		item = new wxMenuItem(choice_plot_fmt, ID_GEN_COPY_SHEET_TO_CLIPBOARD,
			_("Plot to Clipboard"), _("Export drawings to clipboard") );
	    item->SetBitmap(copy_button);
		choice_plot_fmt->Append(item);
#endif

		m_FilesMenu->AppendSeparator();
		ADD_MENUITEM_WITH_HELP_AND_SUBMENU(m_FilesMenu, choice_plot_fmt,
			ID_GEN_PLOT, _("&Plot"),  _("Plot HPGL, PostScript, SVG"), plot_xpm);

		m_FilesMenu->AppendSeparator();
		item = new wxMenuItem(m_FilesMenu, ID_EXIT, _("E&xit"), _("Quit Eeschema") );
	    item->SetBitmap(exit_xpm);
		m_FilesMenu->Append(item);

		// Create the list of last edited schematic files
		m_FilesMenu->AppendSeparator();
		int max_file = m_Parent->m_LastProjectMaxCount;
		for ( ii = 0; ii < max_file; ii++ )
		{
			if ( GetLastProject(ii).IsEmpty() ) break;
			item = new wxMenuItem(m_FilesMenu, ID_LOAD_FILE_1 + ii,
						GetLastProject(ii));
			m_FilesMenu->Append(item);
		}

		// Menu Edit:
		wxMenu * editMenu = new wxMenu;
		msg = AddHotkeyName( _( "&Undo\t" ), s_Schematic_Hokeys_Descr, HK_UNDO );
		item = new wxMenuItem(editMenu, ID_SCHEMATIC_UNDO,
			msg,
			_("Undo last edition") );
	    item->SetBitmap(undo_xpm);
		editMenu->Append(item);

		msg = AddHotkeyName( _( "&Redo\t" ), s_Schematic_Hokeys_Descr, HK_REDO );
		item = new wxMenuItem(editMenu, ID_SCHEMATIC_REDO,
			msg,
			_("Redo the last undo command") );
	    item->SetBitmap(redo_xpm);
		editMenu->Append(item);

		// Menu Configuration:
		wxMenu * configmenu = new wxMenu;
		item = new wxMenuItem(configmenu, ID_CONFIG_REQ,
			_("&Libs and Dir"),
			_("Setting Libraries, Directories and others...") );
	    item->SetBitmap(library_xpm);
		configmenu->Append(item);

		item = new wxMenuItem(configmenu, ID_COLORS_SETUP,
			_("&Colors"),
			_("Setting colors ...") );
	    item->SetBitmap(palette_xpm);
		configmenu->Append(item);

		ADD_MENUITEM(configmenu, ID_OPTIONS_SETUP, _("&Options"), preference_xpm);

		// Font selection and setup
		AddFontSelectionMenu(configmenu);
		
		m_Parent->SetLanguageList(configmenu);

		configmenu->AppendSeparator();
		item = new wxMenuItem(configmenu, ID_CONFIG_SAVE, _("&Save preferences"),
			_("Save application preferences") );
	    item->SetBitmap(save_setup_xpm);
		configmenu->Append(item);
		item = new wxMenuItem(configmenu, ID_CONFIG_READ, _("&Read preferences"),
				_("Read application preferences"));
	    item->SetBitmap(read_setup_xpm);
		configmenu->Append(item);

		configmenu->AppendSeparator();
		AddHotkeyConfigMenu( configmenu );

		// Menu Help:
		wxMenu *helpMenu = new wxMenu;
		item = new wxMenuItem(helpMenu , ID_GENERAL_HELP,
				_("&Contents"), _("Open the eeschema manual"));
	    item->SetBitmap(help_xpm);
		helpMenu->Append(item);

		item = new wxMenuItem(helpMenu , ID_KICAD_ABOUT,
				_("&About"), _("About this application"));
	    item->SetBitmap(info_xpm);
		helpMenu->Append(item);


		menuBar->Append(m_FilesMenu, _("&File") );
		menuBar->Append(editMenu, _("&Edit") );
		menuBar->Append(configmenu, _("&Preferences") );
		menuBar->Append(helpMenu, _("&Help") );

		// Associate the menu bar with the frame
		SetMenuBar(menuBar);
	}

	else		// Update the list of last edited schematic files
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
			item = new wxMenuItem(m_FilesMenu, ID_LOAD_FILE_1 + ii,
						GetLastProject(ii));
			m_FilesMenu->Append(item);
			}
		}
}


