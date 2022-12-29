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
class UNIT_SELECTOR_LEN;

#include "widgets/unit_selector.h"
#include "calculator_panels/calculator_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/statbmp.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_VIA_SIZE_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_VIA_SIZE_BASE : public CALCULATOR_PANEL
{
	private:

	protected:
		wxStaticText* m_staticTextHoleDia;
		wxTextCtrl* m_textCtrlHoleDia;
		UNIT_SELECTOR_LEN* m_choiceHoleDia;
		wxStaticText* m_staticTextPlatingThickness;
		wxTextCtrl* m_textCtrlPlatingThickness;
		UNIT_SELECTOR_LEN* m_choicePlatingThickness;
		wxStaticText* m_staticTextViaLength;
		wxTextCtrl* m_textCtrlViaLength;
		UNIT_SELECTOR_LEN* m_choiceViaLength;
		wxStaticText* m_staticTextViaPadDia;
		wxTextCtrl* m_textCtrlViaPadDia;
		UNIT_SELECTOR_LEN* m_choiceViaPadDia;
		wxStaticText* m_staticTextClearanceDia;
		wxTextCtrl* m_textCtrlClearanceDia;
		UNIT_SELECTOR_LEN* m_choiceClearanceDia;
		wxStaticText* m_staticTextImpedance;
		wxTextCtrl* m_textCtrlImpedance;
		UNIT_SELECTOR_RESISTOR* m_choiceImpedance;
		wxStaticText* m_staticAppliedCurrent;
		wxTextCtrl* m_textCtrlAppliedCurrent;
		wxStaticText* m_staticTextAppliedCurrentUnits;
		wxStaticText* m_staticTextResistivity;
		wxTextCtrl* m_textCtrlPlatingResistivity;
		wxButton* m_button_ResistivityVia;
		wxStaticText* m_viaResistivityUnits;
		wxStaticText* m_staticTextPermittivity;
		wxTextCtrl* m_textCtrlPlatingPermittivity;
		wxButton* m_button_Permittivity;
		wxStaticText* m_staticTextTemperatureDiff;
		wxTextCtrl* m_textCtrlTemperatureDiff;
		wxStaticText* m_viaTempUnits;
		wxStaticText* m_staticTextRiseTime;
		wxTextCtrl* m_textCtrlRiseTime;
		wxStaticText* m_staticTextRiseTimeUnits;
		wxStaticText* m_staticTextWarning;
		wxStaticText* m_staticTextArea11;
		wxStaticText* m_ViaResistance;
		wxStaticText* m_viaResUnits;
		wxStaticText* m_staticText65111;
		wxStaticText* m_ViaVoltageDrop;
		wxStaticText* m_staticText8411;
		wxStaticText* m_staticText66111;
		wxStaticText* m_ViaPowerLoss;
		wxStaticText* m_staticText8311;
		wxStaticText* m_staticText79211;
		wxStaticText* m_ViaThermalResistance;
		wxStaticText* m_viaThermalResUnits;
		wxStaticText* m_staticTextAmpacity;
		wxStaticText* m_ViaAmpacity;
		wxStaticText* m_staticTextAmpacityUnits;
		wxStaticText* m_staticTextCapacitance;
		wxStaticText* m_ViaCapacitance;
		wxStaticText* m_staticTextCapacitanceUnits;
		wxStaticText* m_staticTextRiseTimeOutput;
		wxStaticText* m_RiseTimeOutput;
		wxStaticText* m_staticTextRiseTimeOutputUnits;
		wxStaticText* m_staticTextInductance;
		wxStaticText* m_Inductance;
		wxStaticText* m_staticTextInductanceUnits;
		wxStaticText* m_staticTextReactance;
		wxStaticText* m_Reactance;
		wxStaticText* m_viaReactanceUnits;
		wxStaticBitmap* m_viaBitmap;
		wxButton* m_buttonViaReset;

		// Virtual event handlers, override them in your derived class
		virtual void OnViaCalculate( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnViaRho_Button( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnViaEpsilonR_Button( wxCommandEvent& event ) { event.Skip(); }
		virtual void onUpdateViaCalcErrorText( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnViaResetButtonClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_VIA_SIZE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_VIA_SIZE_BASE();

};

