///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class WX_GRID;
class WX_HTML_REPORT_BOX;

#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/html/htmlwin.h>
#include <wx/splitter.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_NETCLASSES_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_NETCLASSES_BASE : public wxPanel
{
	private:

	protected:
		wxSplitterWindow* m_splitter;
		wxPanel* m_netclassesPane;
		WX_GRID* m_netclassGrid;
		wxBitmapButton* m_addButton;
		wxBitmapButton* m_removeButton;
		wxStaticText* m_colorDefaultHelpText;
		wxPanel* m_membershipPane;
		wxStaticText* m_staticText5;
		WX_GRID* m_assignmentGrid;
		WX_HTML_REPORT_BOX* m_matchingNets;
		wxBitmapButton* m_addAssignmentButton;
		wxBitmapButton* m_removeAssignmentButton;

		// Virtual event handlers, override them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnSizeNetclassGrid( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddNetclassClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveNetclassClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onmembershipPanelSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnSizeAssignmentGrid( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddAssignmentClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveAssignmentClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_NETCLASSES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_NETCLASSES_BASE();

};

