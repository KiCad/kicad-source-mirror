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
class HTML_WINDOW;
class STD_BITMAP_BUTTON;

#include "widgets/html_window.h"
#include "calculator_panel.h"
#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/bmpbuttn.h>
#include <wx/html/htmlwin.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_RF_ATTENUATORS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_RF_ATTENUATORS_BASE : public CALCULATOR_PANEL
{
	private:

	protected:
		wxRadioBox* m_AttenuatorsSelection;
		wxStaticBitmap* m_attenuatorBitmap;
		wxStaticText* m_attenuationLabel;
		wxTextCtrl* m_AttValueCtrl;
		wxStaticText* m_attUnit;
		wxStaticText* m_attenuationZinLabel;
		wxTextCtrl* m_ZinValueCtrl;
		wxStaticText* m_attZinUnit;
		wxStaticText* m_ZoutLabel;
		wxTextCtrl* m_ZoutValueCtrl;
		wxStaticText* m_attZoutUnit;
		wxButton* m_buttonAlcAtt;
		STD_BITMAP_BUTTON* m_bpButtonCalcAtt;
		wxStaticText* m_attenuatorR1Label;
		wxTextCtrl* m_Att_R1_Value;
		wxStaticText* m_attR1Unit;
		wxStaticText* m_attenuatorR2Label;
		wxTextCtrl* m_Att_R2_Value;
		wxStaticText* m_attR2Unit;
		wxStaticText* m_attenuatorR3Label;
		wxTextCtrl* m_Att_R3_Value;
		wxStaticText* m_attR3Unit;
		wxStaticText* m_staticTextAttMsg;
		HTML_WINDOW* m_Attenuator_Messages;
		HTML_WINDOW* m_panelAttFormula;

		// Virtual event handlers, override them in your derived class
		virtual void OnAttenuatorSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCalculateAttenuator( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_RF_ATTENUATORS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_RF_ATTENUATORS_BASE();

};

