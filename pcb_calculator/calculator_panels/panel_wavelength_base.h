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
class UNIT_SELECTOR_LEN_CABLE;
class UNIT_SELECTOR_SPEED;
class UNIT_SELECTOR_TIME;

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
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_WAVELENGTH_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_WAVELENGTH_BASE : public CALCULATOR_PANEL
{
	private:

	protected:
		wxStaticText* m_staticText18;
		wxTextCtrl* m_frequencyCtrl;
		UNIT_SELECTOR_FREQUENCY* m_frequencyUnit;
		wxStaticText* m_staticText181;
		wxTextCtrl* m_periodCtrl;
		UNIT_SELECTOR_TIME* m_periodUnit;
		wxStaticText* m_staticText1811;
		wxTextCtrl* m_wavelengthVacuumCtrl;
		UNIT_SELECTOR_LEN_CABLE* m_wavelengthVacuumUnit;
		wxStaticText* m_staticText18111;
		wxTextCtrl* m_wavelengthMediumCtrl;
		UNIT_SELECTOR_LEN_CABLE* m_wavelengthMediumUnit;
		wxStaticText* m_staticText181112;
		wxTextCtrl* m_speedCtrl;
		UNIT_SELECTOR_SPEED* m_speedUnit;
		wxStaticText* m_staticText181111;
		wxTextCtrl* m_permittivityCtrl;
		wxButton* m_button1;
		wxStaticText* m_staticText42;
		wxTextCtrl* m_permeabilityCtrl;

		// Virtual event handlers, override them in your derived class
		virtual void OnFrequencyChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void updateUnits( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPeriodChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnWavelengthVacuumChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnWavelengthMediumChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPermittivityChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonPermittivity( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPermeabilityChange( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_WAVELENGTH_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 538,453 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_WAVELENGTH_BASE();

};

