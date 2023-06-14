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
class UNIT_SELECTOR_FREQUENCY;
class UNIT_SELECTOR_LEN;
class UNIT_SELECTOR_LEN_CABLE;
class UNIT_SELECTOR_LINEAR_RESISTANCE;
class UNIT_SELECTOR_POWER;
class UNIT_SELECTOR_VOLTAGE;

#include "calculator_panels/calculator_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/statbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_CABLE_SIZE_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_CABLE_SIZE_BASE : public CALCULATOR_PANEL
{
	private:

	protected:
		wxStaticText* m_staticTextSize;
		wxChoice* m_sizeChoice;
		wxStaticText* m_staticTextDiameter;
		wxTextCtrl* m_diameterCtrl;
		UNIT_SELECTOR_LEN* m_diameterUnit;
		wxStaticText* m_staticTextArea;
		wxTextCtrl* m_areaCtrl;
		wxStaticText* m_stUnitmmSq;
		wxStaticText* m_staticTextResitivity;
		wxTextCtrl* m_textCtrlConductorResistivity;
		wxButton* m_button_ResistivityConductor;
		wxStaticText* m_stUnitOhmMeter;
		wxStaticText* m_staticTextTempCoeff;
		wxTextCtrl* m_textCtrlConductorThermCoef;
		wxButton* m_button_Temp_Coef_Conductor;
		wxStaticText* m_staticTextLinRes;
		wxTextCtrl* m_linResistanceCtrl;
		UNIT_SELECTOR_LINEAR_RESISTANCE* m_linResistanceUnit;
		wxStaticText* m_staticTextSkin;
		wxTextCtrl* m_frequencyCtrl;
		UNIT_SELECTOR_FREQUENCY* m_frequencyUnit;
		wxStaticText* m_staticTextAmpacity;
		wxTextCtrl* m_AmpacityCtrl;
		wxStaticText* m_staticText16421;
		wxStaticText* m_staticTextDensity;
		wxSlider* m_slCurrentDensity;
		wxStaticText* m_stUnitAmp_mmSq;
		wxStaticText* m_staticTextCableTemp;
		wxTextCtrl* m_conductorTempCtrl;
		wxStaticText* m_stUnitDegC;
		wxStaticText* m_staticTextCurrent;
		wxTextCtrl* m_currentCtrl;
		wxStaticText* m_staticText;
		wxStaticText* m_staticTextLen;
		wxTextCtrl* m_lengthCtrl;
		UNIT_SELECTOR_LEN_CABLE* m_lengthUnit;
		wxStaticText* m_staticTextResDC;
		wxTextCtrl* m_resistanceDcCtrl;
		wxStaticText* m_stUnitOhm;
		wxStaticText* m_staticTextDrop;
		wxTextCtrl* m_vDropCtrl;
		UNIT_SELECTOR_VOLTAGE* m_vDropUnit;
		wxStaticText* m_staticTextPower;
		wxTextCtrl* m_powerCtrl;
		UNIT_SELECTOR_POWER* m_powerUnit;

		// Virtual event handlers, override them in your derived class
		virtual void OnCableSizeChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDiameterChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateUnit( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAreaChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnConductorResistivityChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnConductorResistivity_Button( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnConductorThermCoefChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnConductorThermCoefChange_Button( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLinResistanceChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFrequencyChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAmpacityChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onUpdateCurrentDensity( wxScrollEvent& event ) { event.Skip(); }
		virtual void OnConductorTempChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCurrentChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLengthChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnResistanceDcChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnVDropChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPowerChange( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_CABLE_SIZE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_CABLE_SIZE_BASE();

};

