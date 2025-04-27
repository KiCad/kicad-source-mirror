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
class WX_HTML_REPORT_BOX;
class WX_PANEL;

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/grid.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
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
		WX_PANEL* m_netclassesPane;
		wxStaticText* m_staticText3;
		WX_GRID* m_netclassGrid;
		STD_BITMAP_BUTTON* m_addButton;
		STD_BITMAP_BUTTON* m_moveUpButton;
		STD_BITMAP_BUTTON* m_moveDownButton;
		STD_BITMAP_BUTTON* m_removeButton;
		wxStaticText* m_colorDefaultHelpText;
		wxButton* m_importColorsButton;
		WX_PANEL* m_membershipPane;
		wxStaticText* m_staticText5;
		WX_GRID* m_assignmentGrid;
		WX_HTML_REPORT_BOX* m_matchingNets;
		STD_BITMAP_BUTTON* m_addAssignmentButton;
		STD_BITMAP_BUTTON* m_removeAssignmentButton;

		// Virtual event handlers, override them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnSizeNetclassGrid( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddNetclassClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveNetclassUpClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveNetclassDownClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveNetclassClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnImportColorsClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onmembershipPanelSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnSizeAssignmentGrid( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddAssignmentClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveAssignmentClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_NETCLASSES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_NETCLASSES_BASE();

};

