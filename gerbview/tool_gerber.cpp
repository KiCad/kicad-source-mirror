	/***********************************************/
	/*	buildmnu.h: construction du menu principal */
	/***********************************************/

#include "fctsys.h"

#include "common.h"
#include "gerbview.h"
#include "pcbplot.h"

#include "protos.h"

#define BITMAP wxBitmap

#include "bitmaps.h"

#include "id.h"


/***********************************************/
void WinEDA_GerberFrame::ReCreateHToolbar(void)
/***********************************************/
{
int layer = 0;
GERBER_Descr * gerber_layer	= NULL;
int ii;

	// delete and recreate the toolbar
	if ( m_HToolBar  != NULL ) return;

	if ( GetScreen() )
		{
		layer = GetScreen()->m_Active_Layer;
		gerber_layer = g_GERBER_Descr_List[layer];
		}

	m_HToolBar = new WinEDA_Toolbar(TOOLBAR_MAIN, this, ID_H_TOOLBAR, TRUE);
	SetToolBar(m_HToolBar);

	// Set up toolbar
	m_HToolBar->AddTool(ID_NEW_BOARD, BITMAP(new_xpm),
				wxNullBitmap, FALSE,
				-1, -1, (wxObject *) NULL,
				_("New World") );

	m_HToolBar->AddTool(ID_LOAD_FILE, BITMAP(open_xpm),
					wxNullBitmap, FALSE,
					-1, -1, (wxObject *) NULL,
					_("Open existing Layer"));

#if 0
	m_HToolBar->AddTool(ID_SAVE_PROJECT, BITMAP(save_button),
					wxNullBitmap, FALSE,
					-1, -1, (wxObject *) NULL,
					_("Save World"));

	m_HToolBar->AddSeparator();
	m_HToolBar->AddTool(ID_SHEET_SET, BITMAP(sheetset_xpm),
					wxNullBitmap,
					FALSE,
					-1, -1, (wxObject *) NULL,
					_("page settings (size, texts)"));

#endif

	m_HToolBar->AddSeparator();

#if 0
	m_HToolBar->AddTool(wxID_CUT, BITMAP(cut_button),
					wxNullBitmap,
					FALSE,
					-1, -1, (wxObject *) NULL,
					_("Cut selected item"));

	m_HToolBar->AddTool(wxID_COPY, BITMAP(copy_button),
					wxNullBitmap, FALSE,
					-1, -1, (wxObject *) NULL,
					_("Copy selected item"));

	m_HToolBar->AddTool(wxID_PASTE, BITMAP(paste_xpm),
					wxNullBitmap,
					FALSE,
					-1, -1, (wxObject *) NULL,
					_("Paste"));
#endif

	m_HToolBar->AddTool(ID_UNDO_BUTT, BITMAP(undelete_xpm),
					wxNullBitmap,
					FALSE,
					-1, -1, (wxObject *) NULL,
					_("Undelete"));

	m_HToolBar->AddSeparator();
	m_HToolBar->AddTool(ID_GEN_PRINT, BITMAP(print_button),
					wxNullBitmap,
					FALSE,
					-1, -1, (wxObject *) NULL,
					_("Print World"));

	m_HToolBar->AddSeparator();
	m_HToolBar->AddTool(ID_ZOOM_PLUS_BUTT, BITMAP(zoom_in_xpm),
					wxNullBitmap,
					FALSE,
					-1, -1, (wxObject *) NULL,
					_("zoom + (F1)"));

	m_HToolBar->AddTool(ID_ZOOM_MOINS_BUTT, BITMAP(zoom_out_xpm),
					wxNullBitmap,
					FALSE,
					-1, -1, (wxObject *) NULL,
					_("zoom - (F2)"));

	m_HToolBar->AddTool(ID_ZOOM_REDRAW_BUTT, BITMAP(repaint_xpm),
					wxNullBitmap,
					FALSE,
					-1, -1, (wxObject *) NULL,
					_("redraw (F3)"));

	m_HToolBar->AddTool(ID_ZOOM_PAGE_BUTT, BITMAP(zoom_optimal_xpm),
					wxNullBitmap,
					FALSE,
					-1, -1, (wxObject *) NULL,
					_("auto zoom"));

	m_HToolBar->AddSeparator();
	m_HToolBar->AddTool(ID_FIND_ITEMS, BITMAP(find_xpm),
					wxNullBitmap,
					FALSE,
					-1, -1, (wxObject *) NULL,
					_("Find D Codes"));

wxArrayString choices;
	m_HToolBar->AddSeparator();
	for ( ii = 0; ii < 32 ; ii ++ )
	{
		wxString msg;
		msg = _("Layer "); msg << ii+1;
		choices.Add(msg);
	}
	m_SelLayerBox = new WinEDAChoiceBox(m_HToolBar, ID_TOOLBARH_PCB_SELECT_LAYER,
					wxDefaultPosition, wxSize(150, -1), choices);
	m_SelLayerBox->SetSelection( GetScreen()->m_Active_Layer );
	m_HToolBar->AddControl(m_SelLayerBox);

	m_HToolBar->AddSeparator();
	choices.Clear();
	choices.Add( _("No tool"));
	for ( ii = 0; ii < MAX_TOOLS ; ii ++ )
	{
		wxString msg;
		msg = _("Tool "); msg << ii + FIRST_DCODE;
		choices.Add(msg);
	}
	m_SelLayerTool = new WinEDAChoiceBox(m_HToolBar, ID_TOOLBARH_GERBER_SELECT_TOOL,
					wxDefaultPosition, wxSize(150, -1), choices);
	m_HToolBar->AddControl(m_SelLayerTool);


	// after adding the buttons to the toolbar, must call Realize() to reflect
	// the changes
	m_HToolBar->Realize();
	SetToolbars();
}


/**********************************************/
void WinEDA_GerberFrame::ReCreateVToolbar(void)
/**********************************************/
{
	if( m_VToolBar ) return;

	m_VToolBar = new WinEDA_Toolbar(TOOLBAR_TOOL, this, ID_V_TOOLBAR, FALSE);

	// Set up toolbar
	m_VToolBar->AddTool(ID_NO_SELECT_BUTT,
				BITMAP(cursor_xpm),
				wxNullBitmap, TRUE,
					-1, -1, (wxObject *) NULL );
	m_VToolBar->ToggleTool(ID_NO_SELECT_BUTT,TRUE);

#if 0
	m_VToolBar->AddSeparator();
	m_VToolBar->AddTool(ID_COMPONENT_BUTT,
				BITMAP(component_button),
				wxNullBitmap, TRUE,
					-1, -1, (wxObject *) NULL,
					_("Add Flashes"));

	m_VToolBar->AddTool(ID_BUS_BUTT,
				BITMAP(bus_button),
				wxNullBitmap, TRUE,
					-1, -1, (wxObject *) NULL,
					_("Add Lines"));

	m_VToolBar->AddTool(ID_JUNCTION_BUTT,
				BITMAP(junction_xpm),
				wxNullBitmap, TRUE,
					-1, -1, (wxObject *) NULL,
					_("Add Mires"));

	m_VToolBar->AddSeparator();
	m_VToolBar->AddTool(ID_TEXT_COMMENT_BUTT,
				BITMAP(tool_text_xpm),
				wxNullBitmap, TRUE,
					-1, -1, (wxObject *) NULL,
					_("Add Text"));

#endif
	m_VToolBar->AddSeparator();
	m_VToolBar->AddTool(ID_PCB_DELETE_ITEM_BUTT,
				BITMAP(delete_body_xpm),
				wxNullBitmap, TRUE,
					-1, -1, (wxObject *) NULL,
					_("Delete items"));

	m_VToolBar->Realize();
	SetToolbars();
}


/************************************************/
void WinEDA_GerberFrame::ReCreateOptToolbar(void)
/************************************************/
{
	if ( m_OptionsToolBar ) return;

	// creation du tool bar options
	m_OptionsToolBar = new WinEDA_Toolbar(TOOLBAR_OPTION, this, ID_OPT_TOOLBAR, FALSE);

	m_OptionsToolBar->AddTool(ID_TB_OPTIONS_SHOW_GRID, BITMAP(grid_xpm),
					wxNullBitmap,
					TRUE,
					-1, -1, (wxObject *) NULL,
					_("Display Grid OFF"));

	m_OptionsToolBar->AddTool(ID_TB_OPTIONS_SHOW_POLAR_COORD, BITMAP(polar_coord_xpm),
					wxNullBitmap,
					TRUE,
					-1, -1, (wxObject *) NULL,
					_("Display Polar Coord ON"));

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
					_("Show Spots Sketch"));

	m_OptionsToolBar->AddTool(ID_TB_OPTIONS_SHOW_TRACKS_SKETCH,
					BITMAP(showtrack_xpm),
					wxNullBitmap,
					TRUE,
					-1, -1, (wxObject *) NULL,
					_("Show Lines Sketch"));

	m_OptionsToolBar->AddTool(ID_TB_OPTIONS_SHOW_DCODES,
					BITMAP(show_dcodenumber_xpm),
					wxNullBitmap,
					TRUE,
					-1, -1, (wxObject *) NULL,
					_("Show dcode number"));

	m_OptionsToolBar->Realize();

	SetToolbars();
}




