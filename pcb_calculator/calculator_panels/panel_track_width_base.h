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
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/html/htmlwin.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_TRACK_WIDTH_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_TRACK_WIDTH_BASE : public CALCULATOR_PANEL
{
	private:

	protected:
		wxStaticText* m_staticTextCurrent;
		wxTextCtrl* m_TrackCurrentValue;
		wxStaticText* m_staticText62;
		wxStaticText* m_staticText63;
		wxTextCtrl* m_TrackDeltaTValue;
		wxStaticText* m_trackTempUnits;
		wxStaticText* m_staticText66;
		wxTextCtrl* m_TrackLengthValue;
		UNIT_SELECTOR_LEN* m_TW_CuLength_choiceUnit;
		wxStaticText* m_staticText103;
		wxTextCtrl* m_TWResistivity;
		wxStaticText* m_resistivityUnits;
		HTML_WINDOW* m_htmlWinFormulas;
		wxStaticText* m_staticTextExtWidth;
		wxTextCtrl* m_ExtTrackWidthValue;
		UNIT_SELECTOR_LEN* m_TW_ExtTrackWidth_choiceUnit;
		wxStaticText* m_staticText65;
		wxTextCtrl* m_ExtTrackThicknessValue;
		UNIT_SELECTOR_THICKNESS* m_ExtTrackThicknessUnit;
		wxStaticLine* m_staticline3;
		wxStaticLine* m_staticline4;
		wxStaticLine* m_staticline5;
		wxStaticText* m_staticTextArea;
		wxStaticText* m_ExtTrackAreaValue;
		wxStaticText* m_extTrackAreaUnitLabel;
		wxStaticText* m_staticText651;
		wxStaticText* m_ExtTrackResistValue;
		wxStaticText* m_extTrackResUnits;
		wxStaticText* m_staticText661;
		wxStaticText* m_ExtTrackVDropValue;
		wxStaticText* m_staticText83;
		wxStaticText* m_staticText79;
		wxStaticText* m_ExtTrackLossValue;
		wxStaticText* m_staticText791;
		wxStaticText* m_staticTextIntWidth;
		wxTextCtrl* m_IntTrackWidthValue;
		UNIT_SELECTOR_LEN* m_TW_IntTrackWidth_choiceUnit;
		wxStaticText* m_staticText652;
		wxTextCtrl* m_IntTrackThicknessValue;
		UNIT_SELECTOR_THICKNESS* m_IntTrackThicknessUnit;
		wxStaticLine* m_staticline8;
		wxStaticLine* m_staticline9;
		wxStaticLine* m_staticline10;
		wxStaticText* m_staticTextArea1;
		wxStaticText* m_IntTrackAreaValue;
		wxStaticText* m_intTrackAreaUnitLabel;
		wxStaticText* m_staticText6511;
		wxStaticText* m_IntTrackResistValue;
		wxStaticText* m_intTrackResUnits;
		wxStaticText* m_staticText6611;
		wxStaticText* m_IntTrackVDropValue;
		wxStaticText* m_staticText831;
		wxStaticText* m_staticText792;
		wxStaticText* m_IntTrackLossValue;
		wxStaticText* m_staticText7911;
		wxButton* m_buttonTrackWidthReset;

		// Virtual event handlers, override them in your derived class
		virtual void OnTWCalculateFromCurrent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTWParametersChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTWCalculateFromExtWidth( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTWCalculateFromIntWidth( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTWResetButtonClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_TRACK_WIDTH_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_TRACK_WIDTH_BASE();

};

