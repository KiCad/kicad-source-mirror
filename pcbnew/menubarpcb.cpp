/******************************************************************/
/* menubarpcb.cpp - creation du menu general de l'editeur de board*/
/******************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "common.h"

#include "pcbnew.h"

#include "bitmaps.h"
#include "protos.h"
#include "id.h"

#include "Swap_Layer.xpm"
#include "Post_Drill.xpm"
#include "Post_Compo.xpm"

/***********************************************/
void WinEDA_PcbFrame::ReCreateMenuBar(void)
/***********************************************/

/* Cree ou reinitialise le menu du haut d'ecran
*/
{
int ii;
wxMenuBar * menuBar = GetMenuBar();

	if( menuBar == NULL )
	{
		menuBar = new wxMenuBar();

		//////////////////
		// Menu "Files" //
		//////////////////
		m_FilesMenu = new wxMenu;
		wxMenuItem *item = new wxMenuItem(m_FilesMenu, ID_MENU_LOAD_FILE,
					 _("Load Board"),
					_("Delete old Board and Load new Board"));
	    item->SetBitmap(open_xpm);
		m_FilesMenu->Append(item);

		item = new wxMenuItem(m_FilesMenu, ID_MENU_APPEND_FILE,
					 _("Append Board"),
					 _("Add Board to old Board"));
	    item->SetBitmap(import_xpm);
		m_FilesMenu->Append(item);

		item = new wxMenuItem(m_FilesMenu, ID_MENU_NEW_BOARD,
					 _("&New board"),
					 _("Clear old PCB and init a new one"));
	    item->SetBitmap(new_xpm);
		m_FilesMenu->Append(item);

		item = new wxMenuItem(m_FilesMenu, ID_MENU_RECOVER_BOARD,
					 _("&Rescue"),
					 _("Clear old board and get last rescue file"));
	    item->SetBitmap(hammer_xpm);
		m_FilesMenu->Append(item);

		item = new wxMenuItem(m_FilesMenu, ID_MENU_READ_LAST_SAVED_VERSION_BOARD,
					 _("&Previous version"),
					 _("Clear old board and get old version of board") );
	    item->SetBitmap(jigsaw_xpm);
		m_FilesMenu->Append(item);

		// Add save menu
		m_FilesMenu->AppendSeparator();
		item = new wxMenuItem(m_FilesMenu, ID_MENU_SAVE_BOARD,
					 _("&Save board"),
					 _("Save current board") );
	    item->SetBitmap(save_xpm);
		m_FilesMenu->Append(item);

		item = new wxMenuItem(m_FilesMenu, ID_MENU_SAVE_BOARD_AS,
					 _("Save Board as.."),
					 _("Save current board as..") );
	    item->SetBitmap(save_as_xpm);
		m_FilesMenu->Append(item);

		// Add print menu
		m_FilesMenu->AppendSeparator();
		item = new wxMenuItem(m_FilesMenu, ID_GEN_PRINT,
					_("P&rint"), _("Print on current printer"));
	    item->SetBitmap(print_button);
		m_FilesMenu->Append(item);

		// Add plot menu
		item = new wxMenuItem(m_FilesMenu, ID_GEN_PLOT, _("&Plot"),
					_("Plot (Hplg, Postscript, or Gerber format)"));
	    item->SetBitmap(plot_xpm);
		m_FilesMenu->Append(item);

		// Add Export menu
		m_FilesMenu->AppendSeparator();
		wxMenu * submenuexport = new wxMenu();
		item = new wxMenuItem(submenuexport, ID_GEN_EXPORT_FILE_GENCADFORMAT,
			_("&GenCAD"), _("Export GenCAD Format") );
	    item->SetBitmap(export_xpm);
		submenuexport->Append(item);
		item = new wxMenuItem(submenuexport, ID_GEN_EXPORT_FILE_MODULE_REPORT,
			_("&Module report"), _("Create a pcb report (footprint report)") );
	    item->SetBitmap(tools_xpm);
		submenuexport->Append(item);
		ADD_MENUITEM_WITH_HELP_AND_SUBMENU(m_FilesMenu, submenuexport,
			ID_GEN_EXPORT_FILE, _("E&xport"), _("Export board"), export_xpm);

		// Add archive footprints menu
		m_FilesMenu->AppendSeparator();
		wxMenu * submenuarchive = new wxMenu();
		item = new wxMenuItem(submenuarchive, ID_MENU_ARCHIVE_NEW_MODULES,
				_("Add new footprints"),
				_("Archive new footprints only in a library (keep other footprints in this lib)") );
	    item->SetBitmap(library_update_xpm);
		submenuarchive->Append(item);
		item = new wxMenuItem(submenuarchive, ID_MENU_ARCHIVE_ALL_MODULES,
				_("Create footprint archive"),
				_("Archive all footprints  in a library(old lib will be deleted)") );
	    item->SetBitmap(library_xpm);
		submenuarchive->Append(item);
		ADD_MENUITEM_WITH_HELP_AND_SUBMENU(m_FilesMenu, submenuarchive,
				ID_MENU_ARCHIVE_MODULES,
				_("Archive footprints"),
				_("Archive or Add footprints in a library file"), library_xpm);

		// Add exit menu
		m_FilesMenu->AppendSeparator();
		item = new wxMenuItem(m_FilesMenu, ID_EXIT, _("E&xit"), _("Quit pcbnew") );
	    item->SetBitmap(exit_xpm);
		m_FilesMenu->Append(item);


		// Creation des selections des anciens fichiers
		m_FilesMenu->AppendSeparator();
		int max_file = m_Parent->m_LastProjectMaxCount;
		for ( ii = 0; ii < max_file; ii++ )
		{
			if ( GetLastProject(ii).IsEmpty() ) break;
			m_FilesMenu->Append(ID_LOAD_FILE_1 + ii, GetLastProject(ii) );
		}

		///////////////////////////////////
		// Configuration et preferences: //
		///////////////////////////////////
		wxMenu * configmenu = new wxMenu;
		item = new wxMenuItem(configmenu, ID_CONFIG_REQ, _("&Libs and Dir"),
			_("Setting Libraries, Directories and others..."));
	    item->SetBitmap(library_xpm);
		configmenu->Append(item);

		item = new wxMenuItem(configmenu, ID_COLORS_SETUP, _("&Colors"),
			_("Select Colors and Display for PCB items"));
	    item->SetBitmap(palette_xpm);
		configmenu->Append(item);

		item = new wxMenuItem(configmenu, ID_OPTIONS_SETUP, _("&General Options"),
			_("Select general options for pcbnew"));
	    item->SetBitmap(preference_xpm);
		configmenu->Append(item);

		item = new wxMenuItem(configmenu, ID_PCB_LOOK_SETUP, _("&Display Options"),
			_("Select what items are displayed"));
	    item->SetBitmap(display_options_xpm);
		configmenu->Append(item);

		// Font selection and setup
		AddFontSelectionMenu(configmenu);

		m_Parent->SetLanguageList(configmenu);

		configmenu->AppendSeparator();
		item = new wxMenuItem(configmenu, ID_CONFIG_SAVE, _("&Save Setup"),
				_("Save options in current directory"));
	    item->SetBitmap(save_setup_xpm);
		configmenu->Append(item);

		item = new wxMenuItem(configmenu, ID_CONFIG_READ, _("&Read Setup"),
				_("Read options from a selected config file"));
	    item->SetBitmap(read_setup_xpm);
		configmenu->Append(item);

		/////////////////////////////
		// Ajustage de dimensions: //
		/////////////////////////////
		wxMenu * sizes_menu = new wxMenu;

		item = new wxMenuItem(sizes_menu, ID_PCB_TRACK_SIZE_SETUP, _("Tracks and Vias"),
			_("Adjust size and width for tracks, vias"));
	    item->SetBitmap(showtrack_xpm);
		sizes_menu->Append(item);

		item = new wxMenuItem(sizes_menu, ID_PCB_USER_GRID_SETUP, _("User Grid Size"),
			_("Adjust User Grid"));
	    item->SetBitmap(grid_xpm);
		sizes_menu->Append(item);

		item = new wxMenuItem(sizes_menu, ID_PCB_DRAWINGS_WIDTHS_SETUP, _("Texts and Drawings"),
			_("Adjust width for texts and drawings"));
	    item->SetBitmap(options_text_xpm);
		sizes_menu->Append(item);

		item = new wxMenuItem(sizes_menu, ID_PCB_PAD_SETUP, _("Pad Settings"),
			_("Adjust size,shape,layers... for Pads"));
	    item->SetBitmap(pad_xpm);
		sizes_menu->Append(item);

		sizes_menu->AppendSeparator();
		item = new wxMenuItem(sizes_menu, ID_CONFIG_SAVE, _("&Save Setup"),
				_("Save options in current directory"));
	    item->SetBitmap(save_xpm);
		sizes_menu->Append(item);

		//////////////////////////////////////////////////////////////////
		// Menu postprocess ( generation fichiers percage, placement... //
		//////////////////////////////////////////////////////////////////
		wxMenu *postprocess_menu = new wxMenu;
		item = new wxMenuItem(postprocess_menu, ID_PCB_GEN_POS_MODULES_FILE,
					_("Create &Modules Pos"),
					_("Gen Position modules file"));
	    item->SetBitmap(post_compo_xpm);
		postprocess_menu->Append(item);

		item = new wxMenuItem(postprocess_menu, ID_PCB_GEN_DRILL_FILE, _("Create &Drill file"),
					_("Gen Drill (EXCELLON] file and/or Drill sheet"));
	    item->SetBitmap(post_drill_xpm);
		postprocess_menu->Append(item);

		item = new wxMenuItem(postprocess_menu, ID_PCB_GEN_CMP_FILE, _("Create &Cmp file"),
					_("Recreate .cmp file for CvPcb"));
	    item->SetBitmap(save_cmpstuff_xpm);
		postprocess_menu->Append(item);

		//////////////////////////
		// Menu d'outils divers //
		//////////////////////////
		wxMenu *miscellaneous_menu = new wxMenu;
		item = new wxMenuItem(miscellaneous_menu, ID_PCB_GLOBAL_DELETE, _("Global &Deletions"),
				_("Delete Tracks, Modules, Texts... on Board"));
	    item->SetBitmap(general_deletions_xpm);
		miscellaneous_menu->Append(item);

		item = new wxMenuItem(miscellaneous_menu, ID_MENU_LIST_NETS, _("&List nets"),
				_("List nets (names and id)"));
	    item->SetBitmap(tools_xpm);
		miscellaneous_menu->Append(item);

		item = new wxMenuItem(miscellaneous_menu, ID_MENU_PCB_CLEAN, _("&Clean tracks"),
				_("Clean stubs, vias, delete break points"));
	    item->SetBitmap(delete_body_xpm);
		miscellaneous_menu->Append(item);

		item = new wxMenuItem(miscellaneous_menu, ID_MENU_PCB_SWAP_LAYERS, _("&Swap layers"),
				_("Swap tracks on copper layers or drawings on others layers"));
	    item->SetBitmap(swap_layer_xpm);
		miscellaneous_menu->Append(item);

		////////////////
		// Menu Help: //
		////////////////
		wxMenu *helpMenu = new wxMenu;
		item = new wxMenuItem(helpMenu , ID_GENERAL_HELP, _("Pcbnew &Help"), _("On line doc"));
	    item->SetBitmap(help_xpm);
		helpMenu->Append(item);

		item = new wxMenuItem(helpMenu , ID_KICAD_ABOUT, _("Pcbnew &About"), _("Pcbnew Infos"));
	    item->SetBitmap(info_xpm);
		helpMenu->Append(item);

		//////////////////////
		// Menu Display 3D: //
		//////////////////////
		wxMenu *Display3DMenu = new wxMenu;
		item = new wxMenuItem(Display3DMenu , ID_MENU_PCB_SHOW_3D_FRAME, _("3D Display"), _("Show Board in 3D Mode"));
	    item->SetBitmap(show_3d_xpm);
		Display3DMenu->Append(item);

		menuBar->Append(m_FilesMenu, _("&Files"));
		menuBar->Append(configmenu, _("&Preferences"));
		menuBar->Append(sizes_menu, _("&Dimensions"));
		menuBar->Append(miscellaneous_menu, _("&Miscellaneous"));
		menuBar->Append(postprocess_menu, _("P&ostprocess"));
		menuBar->Append(Display3DMenu, _("&3D Display"));
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
		for ( ii = 0; ii < max_file; ii++ )
		{
			if ( GetLastProject(ii).IsEmpty() ) break;
			m_FilesMenu->Append(ID_LOAD_FILE_1 + ii, GetLastProject(ii) );
		}
	}
}


