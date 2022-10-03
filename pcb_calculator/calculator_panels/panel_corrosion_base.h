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
#include "calculator_panels/calculator_panel.h"
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_CORROSION_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_CORROSION_BASE : public CALCULATOR_PANEL
{
	private:

	protected:
		wxScrolledWindow* m_scrolledWindow1;
		wxGrid* m_table;
		wxStaticText* m_staticText16;
		wxStaticText* m_staticText2;
		wxTextCtrl* m_corFilterCtrl;
		wxStaticText* m_staticText3;

		// Virtual event handlers, override them in your derived class
		virtual void OnCorFilterChange( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_CORROSION_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 677,453 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_CORROSION_BASE();

};

