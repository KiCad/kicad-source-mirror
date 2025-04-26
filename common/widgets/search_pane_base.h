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
class BITMAP_BUTTON;

#include <wx/string.h>
#include <wx/srchctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/notebook.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class SEARCH_PANE_BASE
///////////////////////////////////////////////////////////////////////////////
class SEARCH_PANE_BASE : public wxPanel
{
	private:

	protected:
		wxBoxSizer* m_sizerOuter;
		wxSearchCtrl* m_searchCtrl1;
		wxStaticLine* m_staticline1;
		BITMAP_BUTTON* m_menuButton;
		wxNotebook* m_notebook;

		// Virtual event handlers, override them in your derived class
		virtual void OnSetFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void OnSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnSearchTextEntry( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnNotebookPageChanged( wxNotebookEvent& event ) { event.Skip(); }


	public:

		SEARCH_PANE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 284,110 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~SEARCH_PANE_BASE();

};

