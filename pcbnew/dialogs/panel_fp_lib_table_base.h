///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class WX_GRID;

#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/notebook.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <widgets/split_button.h>
#include <wx/statbox.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_FP_LIB_TABLE_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_FP_LIB_TABLE_BASE : public wxPanel
{
	private:

	protected:
		wxNotebook* m_notebook;
		wxPanel* m_global_panel;
		WX_GRID* m_global_grid;
		wxPanel* m_project_panel;
		WX_GRID* m_project_grid;
		wxPanel* m_buttonsPanel;
		wxBitmapButton* m_append_button;
		SPLIT_BUTTON* m_browseButton;
		wxBitmapButton* m_move_up_button;
		wxBitmapButton* m_move_down_button;
		wxBitmapButton* m_delete_button;
		WX_GRID* m_path_subs_grid;

		// Virtual event handlers, overide them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void appendRowHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void moveUpHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void moveDownHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void deleteRowHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSizeGrid( wxSizeEvent& event ) { event.Skip(); }


	public:

		PANEL_FP_LIB_TABLE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_FP_LIB_TABLE_BASE();

};

