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
class UNIT_SELECTOR_LEN;

#include <wx/string.h>
#include <wx/choice.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/grid.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_BOARD_CLASS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_BOARD_CLASS_BASE : public wxPanel
{
	private:

	protected:
		UNIT_SELECTOR_LEN* m_BoardClassesUnitsSelector;
		wxStaticText* m_staticTextBrdClass;
		wxGrid* m_gridClassesValuesDisplay;
		wxPanel* m_panelShowClassPrms;

		// Virtual event handlers, overide them in your derived class
		virtual void OnBoardClassesUnitsSelection( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_BOARD_CLASS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 701,347 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_BOARD_CLASS_BASE();

};

