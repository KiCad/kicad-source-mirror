///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c5)
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

#include "calculator_panels/calculator_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
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
		wxStaticText* m_staticText162;
		wxChoice* m_sizeChoice;
		wxStaticText* m_staticText16;
		wxTextCtrl* m_diameterCtrl;
		UNIT_SELECTOR_LEN* m_diameterUnit;
		wxStaticText* m_staticText161;
		wxTextCtrl* m_areaCtrl;
		wxStaticText* m_staticText1641;
		wxStaticText* m_staticText16411;
		wxTextCtrl* m_linResistanceCtrl;
		UNIT_SELECTOR_LINEAR_RESISTANCE* m_linResistanceUnit;
		wxStaticText* m_staticText164;
		wxTextCtrl* m_frequencyCtrl;
		UNIT_SELECTOR_FREQUENCY* m_frequencyUnit;
		wxStaticText* m_staticText1642;
		wxTextCtrl* m_AmpacityCtrl;
		wxStaticText* m_staticText16421;
		wxStaticText* m_staticText163;
		wxTextCtrl* m_currentCtrl;
		wxStaticText* m_staticText;
		wxStaticText* m_staticText1612;
		wxTextCtrl* m_lengthCtrl;
		UNIT_SELECTOR_LEN_CABLE* m_lengthUnit;
		wxStaticText* m_staticText16121;
		wxTextCtrl* m_resistanceCtrl;
		wxStaticText* m_staticText161211;
		wxStaticText* m_staticText161212;
		wxTextCtrl* m_vDropCtrl;
		wxStaticText* m_staticText1612121;
		wxStaticText* m_staticText1612122;
		wxTextCtrl* m_powerCtrl;
		wxStaticText* m_staticText16121211;

		// Virtual event handlers, override them in your derived class
		virtual void OnSizeChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDiameterChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateUnit( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAreaChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLinResistanceChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFrequencyChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAmpacityChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCurrentChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLengthChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnResistanceChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnVDropChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPowerChange( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_CABLE_SIZE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 677,453 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_CABLE_SIZE_BASE();

};

