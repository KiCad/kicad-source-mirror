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
class UNIT_SELECTOR_LEN;
class UNIT_SELECTOR_THICKNESS;

#include "widgets/html_window.h"
#include "calculator_panels/calculator_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/radiobut.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/html/htmlwin.h>
#include <wx/statbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_FUSING_CURRENT_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_FUSING_CURRENT_BASE : public CALCULATOR_PANEL
{
	private:

	protected:
		wxStaticText* m_dummy1;
		wxStaticText* m_ambientText;
		wxTextCtrl* m_ambientValue;
		wxStaticText* m_ambientUnit;
		wxStaticText* m_dummy2;
		wxStaticText* m_meltingText;
		wxTextCtrl* m_meltingValue;
		wxStaticText* m_meltingUnit;
		wxRadioButton* m_widthRadio;
		wxStaticText* m_widthText;
		wxTextCtrl* m_widthValue;
		UNIT_SELECTOR_LEN* m_widthUnit;
		wxRadioButton* m_thicknessRadio;
		wxStaticText* m_thicknessText;
		wxTextCtrl* m_thicknessValue;
		UNIT_SELECTOR_THICKNESS* m_thicknessUnit;
		wxRadioButton* m_currentRadio;
		wxStaticText* m_currentText;
		wxTextCtrl* m_currentValue;
		wxStaticText* m_currentUnit;
		wxRadioButton* m_timeRadio;
		wxStaticText* m_timeText;
		wxTextCtrl* m_timeValue;
		wxStaticText* m_timeUnit;
		wxButton* m_calculateButton;
		wxStaticText* m_comment;
		HTML_WINDOW* m_htmlHelp;

		// Virtual event handlers, override them in your derived class
		virtual void m_onCalculateClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_FUSING_CURRENT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 512,574 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_FUSING_CURRENT_BASE();

};

