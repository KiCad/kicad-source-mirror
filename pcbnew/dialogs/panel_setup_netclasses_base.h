///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 10 2019)
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
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/textctrl.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_NETCLASSES_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_NETCLASSES_BASE : public wxPanel
{
	private:

	protected:
		wxPanel* m_netclassesPane;
		WX_GRID* m_netclassGrid;
		wxBitmapButton* m_addButton;
		wxBitmapButton* m_removeButton;
		wxPanel* m_membershipPane;
		wxStaticText* m_ncfilterLabel;
		wxChoice* m_netClassFilter;
		wxStaticText* m_filterLabel;
		wxTextCtrl* m_netNameFilter;
		wxButton* m_showAllButton;
		wxButton* m_filterNetsButton;
		wxStaticText* m_assignLabel;
		wxChoice* m_assignNetClass;
		wxButton* m_assignAllButton;
		wxButton* m_assignSelectedButton;
		WX_GRID* m_membershipGrid;

		// Virtual event handlers, overide them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnSizeNetclassGrid( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddNetclassClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveNetclassClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnShowAll( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnApplyFilters( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAssignAll( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAssignSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSizeMembershipGrid( wxSizeEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_NETCLASSES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_SETUP_NETCLASSES_BASE();

};

