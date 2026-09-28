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
#include "widgets/html_window.h"
#include "calculator_panels/calculator_panel.h"
#include <wx/string.h>
#include <wx/radiobut.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbox.h>
#include <wx/grid.h>
#include <wx/statbmp.h>
#include <wx/html/htmlwin.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_R_DIVIDER_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_R_DIVIDER_BASE : public CALCULATOR_PANEL
{
	private:

	protected:
		wxRadioButton* m_rbModeRatio;
		wxRadioButton* m_rbModeVoltage;
		wxStaticText* m_ratioLabel;
		wxTextCtrl* m_ratioValue;
		wxStaticText* m_vinLabel;
		wxTextCtrl* m_vinValue;
		wxStaticText* m_vinUnit;
		wxStaticText* m_voutLabel;
		wxTextCtrl* m_voutValue;
		wxStaticText* m_voutUnit;
		wxStaticText* m_impedanceLabel;
		wxRadioButton* m_impedanceSeries;
		wxRadioButton* m_impedanceParallel;
		wxStaticText* m_impedanceValueLabel;
		wxTextCtrl* m_impedanceValue;
		wxStaticText* m_impedanceUnit;
		wxStaticLine* m_staticline6;
		wxRadioButton* m_e3;
		wxRadioButton* m_e6;
		wxRadioButton* m_e12;
		wxRadioButton* m_e24;
		wxRadioButton* m_e48;
		wxRadioButton* m_e96;
		wxRadioButton* m_e192;
		wxStaticLine* m_staticline61;
		wxButton* m_btnCalculate;
		wxGrid* m_resultsGrid;
		wxStaticBitmap* m_bitmapDivider;
		HTML_WINDOW* m_panelHelp;

		// Virtual event handlers, override them in your derived class
		virtual void OnModeChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnESeriesSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCalculate( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_R_DIVIDER_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_R_DIVIDER_BASE();

};

