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
class UNIT_SELECTOR_ANGLE;
class UNIT_SELECTOR_FREQUENCY;
class UNIT_SELECTOR_LEN;
class UNIT_SELECTOR_RESISTOR;

#include "calculator_panels/calculator_panel.h"
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
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/statbox.h>
#include <wx/radiobut.h>
#include <wx/bmpbuttn.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_TRANSLINE_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_TRANSLINE_BASE : public CALCULATOR_PANEL
{
	private:

	protected:
		wxRadioBox* m_TranslineSelection;
		wxStaticBitmap* m_translineBitmap;
		wxStaticText* m_EpsilonR_label;
		wxTextCtrl* m_Value_EpsilonR;
		wxButton* m_button_EpsilonR;
		wxStaticText* m_TanD_label;
		wxTextCtrl* m_Value_TanD;
		wxButton* m_button_TanD;
		wxStaticText* m_Rho_label;
		wxTextCtrl* m_Value_Rho;
		wxButton* m_button_Rho;
		wxStaticText* m_substrate_prm3_labelUnit;
		wxStaticText* m_substrate_prm4_label;
		wxTextCtrl* m_Substrate_prm4_Value;
		UNIT_SELECTOR_LEN* m_SubsPrm4_choiceUnit;
		wxStaticText* m_substrate_prm5_label;
		wxTextCtrl* m_Substrate_prm5_Value;
		UNIT_SELECTOR_LEN* m_SubsPrm5_choiceUnit;
		wxStaticText* m_substrate_prm6_label;
		wxTextCtrl* m_Substrate_prm6_Value;
		UNIT_SELECTOR_LEN* m_SubsPrm6_choiceUnit;
		wxStaticText* m_substrate_prm7_label;
		wxTextCtrl* m_Substrate_prm7_Value;
		UNIT_SELECTOR_LEN* m_SubsPrm7_choiceUnit;
		wxStaticText* m_substrate_prm8_label;
		wxTextCtrl* m_Substrate_prm8_Value;
		UNIT_SELECTOR_LEN* m_SubsPrm8_choiceUnit;
		wxStaticText* m_substrate_prm9_label;
		wxTextCtrl* m_Substrate_prm9_Value;
		UNIT_SELECTOR_LEN* m_SubsPrm9_choiceUnit;
		wxStaticText* m_Frequency_label;
		wxTextCtrl* m_Value_Frequency_Ctrl;
		UNIT_SELECTOR_FREQUENCY* m_choiceUnit_Frequency;
		wxStaticBitmap* m_bmCMicrostripZoddZeven;
		wxStaticText* m_phys_prm1_label;
		wxTextCtrl* m_Phys_prm1_Value;
		UNIT_SELECTOR_LEN* m_choiceUnit_Param1;
		wxRadioButton* m_radioBtnPrm1;
		wxStaticText* m_phys_prm2_label;
		wxTextCtrl* m_Phys_prm2_Value;
		UNIT_SELECTOR_LEN* m_choiceUnit_Param2;
		wxRadioButton* m_radioBtnPrm2;
		wxStaticText* m_phys_prm3_label;
		wxTextCtrl* m_Phys_prm3_Value;
		UNIT_SELECTOR_LEN* m_choiceUnit_Param3;
		wxButton* m_AnalyseButton;
		STD_BITMAP_BUTTON* m_bpButtonAnalyze;
		wxButton* m_SynthetizeButton;
		STD_BITMAP_BUTTON* m_bpButtonSynthetize;
		wxStaticText* m_elec_prm1_label;
		wxTextCtrl* m_Elec_prm1_Value;
		UNIT_SELECTOR_RESISTOR* m_choiceUnit_ElecPrm1;
		wxStaticText* m_elec_prm2_label;
		wxTextCtrl* m_Elec_prm2_Value;
		UNIT_SELECTOR_RESISTOR* m_choiceUnit_ElecPrm2;
		wxStaticText* m_elec_prm3_label;
		wxTextCtrl* m_Elec_prm3_Value;
		UNIT_SELECTOR_ANGLE* m_choiceUnit_ElecPrm3;
		wxStaticText* m_left_message1;
		wxStaticText* m_Message1;
		wxStaticText* m_left_message2;
		wxStaticText* m_Message2;
		wxStaticText* m_left_message3;
		wxStaticText* m_Message3;
		wxStaticText* m_left_message4;
		wxStaticText* m_Message4;
		wxStaticText* m_left_message5;
		wxStaticText* m_Message5;
		wxStaticText* m_left_message6;
		wxStaticText* m_Message6;
		wxStaticText* m_left_message7;
		wxStaticText* m_Message7;
		wxStaticText* m_left_message8;
		wxStaticText* m_Message8;
		wxStaticText* m_left_message9;
		wxStaticText* m_Message9;
		wxStaticText* m_left_message10;
		wxStaticText* m_Message10;
		wxButton* m_buttonTransLineReset;

		// Virtual event handlers, override them in your derived class
		virtual void OnTranslineSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineEpsilonR_Button( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineTanD_Button( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineRho_Button( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineAnalyse( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTranslineSynthetize( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTransLineResetButtonClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_TRANSLINE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_TRANSLINE_BASE();

};

