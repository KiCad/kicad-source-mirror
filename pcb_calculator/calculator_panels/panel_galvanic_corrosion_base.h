///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-39-g3487c3cb)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "widgets/html_window.h"
#include "calculator_panels/calculator_panel.h"
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include <wx/html/htmlwin.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/statline.h>
#include <wx/radiobut.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_GALVANIC_CORROSION_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_GALVANIC_CORROSION_BASE : public CALCULATOR_PANEL
{
	private:

	protected:
		wxBoxSizer* bSizerMain;
		wxScrolledWindow* m_scrolledWindow1;
		wxGrid* m_table;
		HTML_WINDOW* m_helpText;
		wxStaticText* m_staticText2;
		wxTextCtrl* m_corFilterCtrl;
		wxStaticText* m_staticText3;
		wxStaticLine* m_staticline;
		wxStaticText* m_stOpts;
		wxRadioButton* m_radioBtnSymbol;
		wxRadioButton* m_radioBtnName;

		// Virtual event handlers, override them in your derived class
		virtual void OnCorFilterChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnNomenclatureChange( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_GALVANIC_CORROSION_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 509,245 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_GALVANIC_CORROSION_BASE();

};

