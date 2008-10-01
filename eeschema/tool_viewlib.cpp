/****************************************************************/
/* tool_viewlib.cpp: Build the toolbars for the library browser */
/****************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "wx/spinctrl.h"

#include "protos.h"

#define BITMAP wxBitmap

#include "bitmaps.h"

#include "id.h"

/****************************************************/
void WinEDA_ViewlibFrame::ReCreateHToolbar()
/****************************************************/
{
int ii;
EDA_LibComponentStruct * RootLibEntry = NULL, * CurrentLibEntry = NULL;
bool asdeMorgan = FALSE, state;

	if ( (g_CurrentViewLibraryName != wxEmptyString) && (g_CurrentViewComponentName != wxEmptyString) )
	{
		RootLibEntry = FindLibPart(g_CurrentViewComponentName.GetData(),
					 g_CurrentViewLibraryName.GetData(), FIND_ROOT);
		if ( RootLibEntry && LookForConvertPart(RootLibEntry) > 1 )
			asdeMorgan = TRUE;
		CurrentLibEntry = FindLibPart(g_CurrentViewComponentName.GetData(),
				g_CurrentViewLibraryName.GetData(), FIND_ALIAS);
	}

	if ( m_HToolBar  == NULL )
	{
		m_HToolBar = new WinEDA_Toolbar(TOOLBAR_MAIN, this, ID_H_TOOLBAR, TRUE);
		SetToolBar(m_HToolBar);

		// Set up toolbar
		m_HToolBar->AddTool(ID_LIBVIEW_SELECT_LIB, wxEmptyString,
					BITMAP(library_xpm),
					_("Select library to browse"));

		m_HToolBar->AddTool(ID_LIBVIEW_SELECT_PART, wxEmptyString,
					BITMAP(add_component_xpm),
					_("Select part to browse"));

		m_HToolBar->AddSeparator();
		m_HToolBar->AddTool(ID_LIBVIEW_PREVIOUS, wxEmptyString,
					BITMAP(lib_previous_xpm),
					_("Display previous part"));

		m_HToolBar->AddTool(ID_LIBVIEW_NEXT, wxEmptyString,
					BITMAP(lib_next_xpm),
					_("Display next part"));

		m_HToolBar->AddSeparator();
		m_HToolBar->AddTool(ID_ZOOM_IN_BUTT, wxEmptyString,
					BITMAP(zoom_in_xpm),
					_("zoom + (F1)"));

		m_HToolBar->AddTool(ID_ZOOM_OUT_BUTT, wxEmptyString,
					BITMAP(zoom_out_xpm),
					_("zoom - (F2)"));

		m_HToolBar->AddTool(ID_ZOOM_REDRAW_BUTT, wxEmptyString,
					BITMAP(zoom_redraw_xpm),
					_("redraw (F3)"));

		m_HToolBar->AddTool(ID_ZOOM_PAGE_BUTT, wxEmptyString,
					BITMAP(zoom_auto_xpm),
					_("best zoom"));

		m_HToolBar->AddSeparator();
		m_HToolBar->AddTool(ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, wxEmptyString,
					BITMAP(morgan1_xpm),
					_("Show as \"De Morgan\" normal part"), wxITEM_CHECK);

		m_HToolBar->AddTool(ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, wxEmptyString,
					BITMAP(morgan2_xpm),
					_("Show as \"De Morgan\" convert part"), wxITEM_CHECK);

		m_HToolBar->AddSeparator();

		SelpartBox = new WinEDAChoiceBox(m_HToolBar, ID_LIBVIEW_SELECT_PART_NUMBER,
					wxDefaultPosition, wxSize(150,-1));
		m_HToolBar->AddControl(SelpartBox);

		m_HToolBar->AddSeparator();
		m_HToolBar->AddTool(ID_LIBVIEW_VIEWDOC, wxEmptyString, BITMAP(datasheet_xpm),
					_("View component documents") );
		m_HToolBar->EnableTool(ID_LIBVIEW_VIEWDOC, FALSE);

		if ( m_Semaphore )	// The lib browser is called from a "load component" command
		{
			m_HToolBar->AddSeparator();
			m_HToolBar->AddTool(ID_LIBVIEW_CMP_EXPORT_TO_SCHEMATIC, wxEmptyString,
						BITMAP(export_xpm),
						_("Insert component in schematic") );
		}

		// after adding the buttons to the toolbar, must call Realize() to reflect
		// the changes
		m_HToolBar->Realize();
	}

	// Must be AFTER Realize():
	m_HToolBar->ToggleTool(ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT,
						(g_ViewConvert <= 1) ? TRUE : FALSE);
	m_HToolBar->ToggleTool(ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT,
						(g_ViewConvert >= 2) ? TRUE : FALSE );
	m_HToolBar->EnableTool(ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, asdeMorgan);
	m_HToolBar->EnableTool(ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, asdeMorgan);

	int jj = 1;
	if( RootLibEntry ) jj = MAX(RootLibEntry->m_UnitCount, 1);
	SelpartBox->Clear();
	for ( ii = 0; ii < jj ; ii ++ )
	{
		wxString msg;
		msg.Printf( _("Part %c"), 'A' + ii);
		SelpartBox->Append(msg);
	}
	SelpartBox->SetSelection(0);
	state = FALSE;
	if ( CurrentLibEntry && jj > 1 ) state = TRUE;
	SelpartBox->Enable(state);

	state = FALSE;
	if( CurrentLibEntry && (CurrentLibEntry->m_DocFile != wxEmptyString) )
		state = TRUE;
	m_HToolBar->EnableTool(ID_LIBVIEW_VIEWDOC, state);
}



/****************************************************/
void WinEDA_ViewlibFrame::ReCreateVToolbar()
/****************************************************/
{
}

