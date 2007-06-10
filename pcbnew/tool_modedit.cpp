	/*****************************************************************/
	/*	too_modeit.cpp: construction du menu de l'editeur de modules */
	/*****************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"

#define BITMAP wxBitmap

#include "bitmaps.h"

#include "id.h"

#include "Module_Check.xpm"
#include "Import_Module.xpm"
#include "Export_Module.xpm"
#include "New_FootPrint.xpm"
#include "module_options.xpm"
#include "Load_Module_Board.xpm"
#include "Repl_Module_Board.xpm"

#ifdef __UNIX__
#define LISTBOX_WIDTH 120
#else
#define LISTBOX_WIDTH 100
#endif

/***************************************************/
void WinEDA_ModuleEditFrame::ReCreateHToolbar(void)
/***************************************************/
{
	// Cecreate the toolbar
	if ( m_HToolBar  != NULL ) return;

	m_HToolBar = new WinEDA_Toolbar(TOOLBAR_MAIN, this, ID_H_TOOLBAR, TRUE);

	SetToolBar(m_HToolBar);

	// Set up toolbar
	m_HToolBar->AddTool(ID_LIBEDIT_SELECT_CURRENT_LIB, wxEmptyString,
					BITMAP(open_library_xpm),
					_("Select working library"));

	m_HToolBar->AddTool(ID_MODEDIT_SAVE_LIBMODULE, wxEmptyString, BITMAP(save_library_xpm),
					_("Save Module in working library"));

	m_HToolBar->AddTool(ID_LIBEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART, wxEmptyString,
					BITMAP(new_library_xpm),
					_("Create new library and save current module"));

	m_HToolBar->AddSeparator();
	m_HToolBar->AddTool(ID_LIBEDIT_DELETE_PART,  wxEmptyString,
					BITMAP(delete_xpm),
					_("Delete part in current library"));

	m_HToolBar->AddSeparator();
	m_HToolBar->AddTool(ID_MODEDIT_NEW_MODULE,  wxEmptyString,
				BITMAP(new_footprint_xpm),
				_("New Module"));

	m_HToolBar->AddTool(ID_MODEDIT_LOAD_MODULE,  wxEmptyString,
					BITMAP(module_xpm),
					_("Load module from lib"));

	m_HToolBar->AddSeparator();
	m_HToolBar->AddTool(ID_MODEDIT_LOAD_MODULE_FROM_BOARD, wxEmptyString,
					BITMAP(load_module_board_xpm),
					_("Load module from current board"));

	m_HToolBar->AddTool(ID_MODEDIT_SAVE_MODULE_IN_BOARD, wxEmptyString,
					BITMAP(repl_module_board_xpm),
					_("Update module in current board"));

	m_HToolBar->AddSeparator();
	m_HToolBar->AddTool(ID_LIBEDIT_IMPORT_PART,  wxEmptyString,
					BITMAP(import_module_xpm),
					_("import module"));

	m_HToolBar->AddTool(ID_LIBEDIT_EXPORT_PART, wxEmptyString,
					BITMAP(export_module_xpm),
					_("export module"));


	m_HToolBar->AddSeparator();
	m_HToolBar->AddTool(ID_MODEDIT_UNDO, wxEmptyString, BITMAP(undo_xpm),
				_("Undo last edition"));
	m_HToolBar->AddTool(ID_MODEDIT_REDO, wxEmptyString, BITMAP(redo_xpm),
				_("Redo the last undo command"));

	m_HToolBar->AddSeparator();
	m_HToolBar->AddTool(ID_MODEDIT_EDIT_MODULE_PROPERTIES,  wxEmptyString,
					BITMAP(module_options_xpm),
					_("Module Properties"));

	m_HToolBar->AddSeparator();
	m_HToolBar->AddTool(ID_GEN_PRINT,  wxEmptyString, BITMAP(print_button),
					_("Print Module"));

	m_HToolBar->AddSeparator();
	m_HToolBar->AddTool(ID_ZOOM_PLUS_BUTT,  wxEmptyString, BITMAP(zoom_in_xpm),
					_("zoom + (F1)"));

	m_HToolBar->AddTool(ID_ZOOM_MOINS_BUTT,  wxEmptyString,BITMAP(zoom_out_xpm),
					_("zoom - (F2)"));

	m_HToolBar->AddTool(ID_ZOOM_REDRAW_BUTT,  wxEmptyString,BITMAP(repaint_xpm),
					_("redraw (F3)"));

	m_HToolBar->AddTool(ID_ZOOM_PAGE_BUTT,  wxEmptyString,BITMAP(zoom_optimal_xpm),
					_("auto zoom"));

		m_HToolBar->AddSeparator();
	m_HToolBar->AddTool(ID_MODEDIT_PAD_SETTINGS, wxEmptyString,
				BITMAP(options_pad_xpm),
				_("Pad Settings"));

	m_HToolBar->AddSeparator();
	m_HToolBar->AddTool(ID_MODEDIT_CHECK,  wxEmptyString, BITMAP(module_check_xpm),
					_("Module Check"));

	// after adding the buttons to the toolbar, must call Realize() to reflect
	// the changes
	m_HToolBar->Realize();
	SetToolbars();
}

/********************************************************/
void WinEDA_ModuleEditFrame::ReCreateVToolbar(void)
/********************************************************/
{
	if( m_VToolBar ) return;

	m_VToolBar = new WinEDA_Toolbar(TOOLBAR_TOOL, this, ID_V_TOOLBAR, FALSE);

	// Set up toolbar
	m_VToolBar->AddTool(ID_NO_SELECT_BUTT, wxEmptyString,
				BITMAP(cursor_xpm), wxEmptyString, wxITEM_CHECK );
	m_VToolBar->ToggleTool(ID_NO_SELECT_BUTT, TRUE);

	m_VToolBar->AddSeparator();
	m_VToolBar->AddTool(ID_MODEDIT_ADD_PAD, wxEmptyString,
				BITMAP(pad_xpm),
				_("Add Pads"), wxITEM_CHECK);

	m_VToolBar->AddSeparator();
	m_VToolBar->AddTool(ID_LINE_COMMENT_BUTT, wxEmptyString,
				BITMAP(add_polygon_xpm),
					_("Add graphic line or polygon"), wxITEM_CHECK);

	m_VToolBar->AddTool(ID_PCB_CIRCLE_BUTT, wxEmptyString,
				BITMAP(add_circle_xpm),
					_("Add graphic circle"), wxITEM_CHECK);

	m_VToolBar->AddTool(ID_PCB_ARC_BUTT, wxEmptyString,
				BITMAP(add_arc_xpm),
					_("Add graphic arc"), wxITEM_CHECK);

	m_VToolBar->AddTool(ID_TEXT_COMMENT_BUTT, wxEmptyString,
				BITMAP(add_text_xpm),
					_("Add Text"), wxITEM_CHECK);

	m_VToolBar->AddSeparator();
	m_VToolBar->AddTool(ID_MODEDIT_PLACE_ANCHOR, wxEmptyString,
				BITMAP(anchor_xpm),
					_("Place anchor"), wxITEM_CHECK);

	m_VToolBar->AddSeparator();
	m_VToolBar->AddTool(ID_MODEDIT_DELETE_ITEM_BUTT, wxEmptyString,
				BITMAP(delete_body_xpm),
					_("Delete items"), wxITEM_CHECK);

	m_VToolBar->Realize();

	SetToolbars();
}

/*********************************************************/
void WinEDA_ModuleEditFrame::ReCreateOptToolbar(void)
/*********************************************************/
{

	if ( m_OptionsToolBar ) return;

	// creation du tool bar options
	m_OptionsToolBar = new WinEDA_Toolbar(TOOLBAR_OPTION, this, ID_OPT_TOOLBAR, FALSE);

	m_OptionsToolBar->AddTool(ID_TB_OPTIONS_SHOW_GRID, BITMAP(grid_xpm),
					wxNullBitmap,
					TRUE,
					-1, -1, (wxObject *) NULL,
					_("Display Grid OFF"));

	m_OptionsToolBar->AddTool(ID_TB_OPTIONS_SHOW_POLAR_COORD, wxEmptyString,
					BITMAP(polar_coord_xpm),
					_("Display Polar Coord ON"), wxITEM_CHECK);

	m_OptionsToolBar->AddTool(ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
					BITMAP(unit_inch_xpm),
					_("Units = Inch"), wxITEM_CHECK );
	
	m_OptionsToolBar->AddTool(ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
					BITMAP(unit_mm_xpm),
					_("Units = mm"), wxITEM_CHECK );

	m_OptionsToolBar->AddTool(ID_TB_OPTIONS_SELECT_CURSOR, BITMAP(cursor_shape_xpm),
					wxNullBitmap,
					TRUE,
					-1, -1, (wxObject *) NULL,
					_("Change Cursor Shape"));

	m_OptionsToolBar->AddSeparator();
	m_OptionsToolBar->AddTool(ID_TB_OPTIONS_SHOW_PADS_SKETCH,
					BITMAP(pad_sketch_xpm),
					wxNullBitmap,
					TRUE,
					-1, -1, (wxObject *) NULL,
					_("Show Pads Sketch"));

	m_OptionsToolBar->AddTool(ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH,
					BITMAP(text_sketch_xpm),
					wxNullBitmap,
					TRUE,
					-1, -1, (wxObject *) NULL,
					_("Show Texts Sketch"));

	m_OptionsToolBar->AddTool(ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH,
					BITMAP(show_mod_edge_xpm),
					wxNullBitmap,
					TRUE,
					-1, -1, (wxObject *) NULL,
					_("Show Edges Sketch"));

	m_OptionsToolBar->Realize();

	SetToolbars();
}

/*********************************************************/
void WinEDA_ModuleEditFrame::ReCreateAuxiliaryToolbar(void)
/*********************************************************/
{
int ii;
wxString msg;

	if ( m_AuxiliaryToolBar == NULL )
	{
		m_AuxiliaryToolBar = new WinEDA_Toolbar(TOOLBAR_AUX, this, ID_AUX_TOOLBAR, TRUE);

		// Set up toolbar
		m_AuxiliaryToolBar->AddSeparator();

		// Boite de selection du pas de grille
		m_SelGridBox = new WinEDAChoiceBox(m_AuxiliaryToolBar,
					ID_ON_GRID_SELECT,
					wxPoint(-1,-1), wxSize(LISTBOX_WIDTH , -1));
		m_AuxiliaryToolBar->AddControl( m_SelGridBox);

		// Boite de selection du Zoom
		m_AuxiliaryToolBar->AddSeparator();
		m_SelZoomBox = new WinEDAChoiceBox(m_AuxiliaryToolBar,
					ID_ON_ZOOM_SELECT,
					wxPoint(-1,-1), wxSize(LISTBOX_WIDTH , -1));
		msg = _("Auto");
		m_SelZoomBox->Append(msg);
		for ( int jj = 0, ii = 1; ii <= m_ZoomMaxValue; ii <<= 1, jj++ )
		{
			msg.Printf(_("Zoom %d"), ii);
			m_SelZoomBox->Append(msg);
		}
		m_SelZoomBox->Append(wxT(""));

		m_AuxiliaryToolBar->AddControl( m_SelZoomBox);
		// after adding the buttons to the toolbar, must call Realize() to reflect
		// the changes
		m_AuxiliaryToolBar->Realize();
	}

	// mise a jour des affichages
	m_SelGridBox->Clear();
	for ( ii = 0; g_GridList[ii].x > 0; ii++ )
	{
		double value = To_User_Unit(g_UnitMetric, g_GridList[ii].x, PCB_INTERNAL_UNIT);
				if ( g_UnitMetric == INCHES )
					msg.Printf(_("Grid %.1f"), value * 1000);
				else
					msg.Printf(_("Grid %.3f"), value);
		m_SelGridBox->Append(msg);
	}
	m_SelGridBox->Append( _("User Grid") );

	SetToolbars();
}






