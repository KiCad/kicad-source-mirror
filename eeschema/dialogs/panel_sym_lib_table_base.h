///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class STD_BITMAP_BUTTON;
class WX_GRID;

#include <wx/gdicmn.h>
#include <wx/aui/auibook.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/grid.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SYM_LIB_TABLE_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SYM_LIB_TABLE_BASE : public wxPanel
{
	private:

	protected:
		wxAuiNotebook* m_notebook;
		STD_BITMAP_BUTTON* m_append_button;
		STD_BITMAP_BUTTON* m_browse_button;
		STD_BITMAP_BUTTON* m_move_up_button;
		STD_BITMAP_BUTTON* m_move_down_button;
		STD_BITMAP_BUTTON* m_delete_button;
		wxButton* m_resetGlobal;
		wxButton* m_convertLegacy;
		WX_GRID* m_path_subs_grid;

		// Virtual event handlers, override them in your derived class
		virtual void onPageChange( wxAuiNotebookEvent& event ) { event.Skip(); }
		virtual void appendRowHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void browseLibrariesHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void moveUpHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void moveDownHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void deleteRowHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void onReset( wxCommandEvent& event ) { event.Skip(); }
		virtual void onConvertLegacyLibraries( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSizeGrid( wxSizeEvent& event ) { event.Skip(); }


	public:

		PANEL_SYM_LIB_TABLE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SYM_LIB_TABLE_BASE();

};

