/***********************************************************************/
/* menubarmodedit.cpp - creation du menu general de l'editeur de module*/
/***********************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "common.h"

#include "pcbnew.h"

#include "bitmaps.h"
#include "protos.h"
#include "id.h"



/***********************************************/
void WinEDA_ModuleEditFrame::ReCreateMenuBar(void)
/***********************************************/

/* Cree ou reinitialise le menu du haut d'ecran
*/
{
wxMenuBar * menuBar = GetMenuBar();
wxMenuItem * item;
	
	if( ! menuBar )
		{
		menuBar = new wxMenuBar();

		/////////////////////////////
		// Ajustage de dimensions: //
		/////////////////////////////
		wxMenu * sizes_menu = new wxMenu;

		item = new wxMenuItem(sizes_menu, ID_PCB_DRAWINGS_WIDTHS_SETUP,
			_("Sizes and Widths"),
			_("Adjust width for texts and drawings"));
	    item->SetBitmap(options_text_xpm);
		sizes_menu->Append(item);

		item = new wxMenuItem(sizes_menu, ID_PCB_PAD_SETUP, _("Pad Settings"),
			_("Adjust size,shape,layers... for Pads"));
	    item->SetBitmap(pad_xpm);
		sizes_menu->Append(item);

		item = new wxMenuItem(sizes_menu, ID_PCB_USER_GRID_SETUP, _("User Grid Size"),
			_("Adjust User Grid"));
	    item->SetBitmap(grid_xpm);
		sizes_menu->Append(item);

		
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

		menuBar->Append(sizes_menu, _("&Dimensions"));
		menuBar->Append(Display3DMenu, _("&3D Display"));
		menuBar->Append(helpMenu, _("&Help"));

		// Associate the menu bar with the frame
		SetMenuBar(menuBar);
		}
}


