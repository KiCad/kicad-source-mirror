///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
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
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/radiobut.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/html/htmlwin.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_R_CALCULATOR_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_R_CALCULATOR_BASE : public CALCULATOR_PANEL
{
	private:

	protected:
		wxStaticText* m_ESrequired;
		wxTextCtrl* m_ResRequired;
		wxStaticText* m_reqResUnits;
		wxStaticText* m_ESrequired1;
		wxTextCtrl* m_ResExclude1;
		wxStaticText* m_exclude1Units;
		wxStaticText* m_ESrequired11;
		wxTextCtrl* m_ResExclude2;
		wxStaticText* m_exclude2Units;
		wxStaticLine* m_staticline6;
		wxRadioButton* m_e1;
		wxRadioButton* m_e3;
		wxRadioButton* m_e6;
		wxRadioButton* m_e12;
		wxRadioButton* m_e24;
		wxStaticText* m_ESeriesSimpleSolution;
		wxTextCtrl* m_ESeries_Sol2R;
		wxStaticText* m_ESeriesSimpleErr;
		wxTextCtrl* m_ESeriesError2R;
		wxStaticText* m_ESeriesSimplePercent;
		wxStaticText* m_ESeries3RSolution1;
		wxTextCtrl* m_ESeries_Sol3R;
		wxStaticText* m_ESeriesAltErr;
		wxTextCtrl* m_ESeriesError3R;
		wxStaticText* m_ESeriesAltPercent;
		wxStaticText* m_ESeries4RSolution;
		wxTextCtrl* m_ESeries_Sol4R;
		wxStaticText* m_ESeriesAltErr1;
		wxTextCtrl* m_ESeriesError4R;
		wxStaticText* m_ESeriesAltPercent1;
		wxStaticLine* m_staticline7;
		wxButton* m_buttonEScalculate;
		HTML_WINDOW* m_panelESeriesHelp;

		// Virtual event handlers, override them in your derived class
		virtual void OnESeriesSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCalculateESeries( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_R_CALCULATOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_R_CALCULATOR_BASE();

};

