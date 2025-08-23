///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/gbsizer.h>
#include <wx/checkbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class GAL_OPTIONS_PANEL_BASE
///////////////////////////////////////////////////////////////////////////////
class GAL_OPTIONS_PANEL_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_staticText1;
		wxStaticLine* m_staticline1;
		wxStaticText* m_gridStyleLabel;
		wxRadioButton* m_rbDots;
		wxRadioButton* m_rbLines;
		wxRadioButton* m_rbCrosses;
		wxStaticText* l_gridLineWidth;
		wxChoice* m_gridLineWidth;
		wxStaticText* l_gridLineWidthUnits;
		wxStaticText* l_gridMinSpacing;
		wxSpinCtrl* m_gridMinSpacing;
		wxStaticText* l_gridMinSpacingUnits;
		wxStaticText* l_gridSnapOptions;
		wxChoice* m_gridSnapOptions;
		wxStaticText* m_stGridLabel;
		wxStaticLine* m_staticline2;
                wxRadioButton* m_rbSmallCrosshairs;
                wxRadioButton* m_rbFullWindowCrosshairs;
                wxRadioButton* m_rb45DegreeCrosshairs;
                wxCheckBox* m_forceCursorDisplay;

	public:

		GAL_OPTIONS_PANEL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~GAL_OPTIONS_PANEL_BASE();

};

