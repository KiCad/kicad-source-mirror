///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/unit_selector.h"

#include "panel_wavelength_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_WAVELENGTH_BASE::PANEL_WAVELENGTH_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer3->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText18 = new wxStaticText( this, wxID_ANY, _("Frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	fgSizer3->Add( m_staticText18, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );

	m_frequencyCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_frequencyCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxArrayString m_frequencyUnitChoices;
	m_frequencyUnit = new UNIT_SELECTOR_FREQUENCY( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_frequencyUnitChoices, 0 );
	m_frequencyUnit->SetSelection( 0 );
	fgSizer3->Add( m_frequencyUnit, 0, wxEXPAND|wxTOP, 5 );

	m_staticText181 = new wxStaticText( this, wxID_ANY, _("Period:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText181->Wrap( -1 );
	fgSizer3->Add( m_staticText181, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );

	m_periodCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_periodCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxArrayString m_periodUnitChoices;
	m_periodUnit = new UNIT_SELECTOR_TIME( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_periodUnitChoices, 0 );
	m_periodUnit->SetSelection( 0 );
	fgSizer3->Add( m_periodUnit, 0, wxEXPAND|wxTOP, 5 );

	m_staticText1811 = new wxStaticText( this, wxID_ANY, _("Wavelength in vacuum:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1811->Wrap( -1 );
	fgSizer3->Add( m_staticText1811, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );

	m_wavelengthVacuumCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_wavelengthVacuumCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxArrayString m_wavelengthVacuumUnitChoices;
	m_wavelengthVacuumUnit = new UNIT_SELECTOR_LEN_CABLE( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_wavelengthVacuumUnitChoices, 0 );
	m_wavelengthVacuumUnit->SetSelection( 0 );
	fgSizer3->Add( m_wavelengthVacuumUnit, 0, wxEXPAND|wxTOP, 5 );

	m_staticText18111 = new wxStaticText( this, wxID_ANY, _("Wavelength in medium:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18111->Wrap( -1 );
	fgSizer3->Add( m_staticText18111, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );

	m_wavelengthMediumCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_wavelengthMediumCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxArrayString m_wavelengthMediumUnitChoices;
	m_wavelengthMediumUnit = new UNIT_SELECTOR_LEN_CABLE( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_wavelengthMediumUnitChoices, 0 );
	m_wavelengthMediumUnit->SetSelection( 0 );
	fgSizer3->Add( m_wavelengthMediumUnit, 0, wxEXPAND|wxTOP, 5 );

	m_staticText181112 = new wxStaticText( this, wxID_ANY, _("Speed in medium:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText181112->Wrap( -1 );
	fgSizer3->Add( m_staticText181112, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );

	m_speedCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	fgSizer3->Add( m_speedCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxArrayString m_speedUnitChoices;
	m_speedUnit = new UNIT_SELECTOR_SPEED( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_speedUnitChoices, 0 );
	m_speedUnit->SetSelection( 0 );
	fgSizer3->Add( m_speedUnit, 0, wxEXPAND|wxTOP, 5 );

	m_staticText181111 = new wxStaticText( this, wxID_ANY, _("er:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText181111->Wrap( -1 );
	m_staticText181111->SetToolTip( _("relative permittivity (dielectric constant)") );

	fgSizer3->Add( m_staticText181111, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );

	m_permittivityCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_permittivityCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_button1 = new wxButton( this, wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_button1, 0, wxTOP, 5 );

	m_staticText42 = new wxStaticText( this, wxID_ANY, _("mur:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText42->Wrap( -1 );
	m_staticText42->SetToolTip( _("relative permeability") );

	fgSizer3->Add( m_staticText42, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );

	m_permeabilityCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_permeabilityCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


	bSizer4->Add( fgSizer3, 1, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bSizer6->Add( bSizer4, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer6 );
	this->Layout();

	// Connect Events
	m_frequencyCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::OnFrequencyChange ), NULL, this );
	m_frequencyUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::updateUnits ), NULL, this );
	m_periodCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::OnPeriodChange ), NULL, this );
	m_periodUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::updateUnits ), NULL, this );
	m_wavelengthVacuumCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::OnWavelengthVacuumChange ), NULL, this );
	m_wavelengthVacuumUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::updateUnits ), NULL, this );
	m_wavelengthMediumCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::OnWavelengthMediumChange ), NULL, this );
	m_wavelengthMediumUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::updateUnits ), NULL, this );
	m_speedUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::updateUnits ), NULL, this );
	m_permittivityCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::OnPermittivityChange ), NULL, this );
	m_button1->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::OnButtonPermittivity ), NULL, this );
	m_permeabilityCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::OnPermeabilityChange ), NULL, this );
}

PANEL_WAVELENGTH_BASE::~PANEL_WAVELENGTH_BASE()
{
	// Disconnect Events
	m_frequencyCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::OnFrequencyChange ), NULL, this );
	m_frequencyUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::updateUnits ), NULL, this );
	m_periodCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::OnPeriodChange ), NULL, this );
	m_periodUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::updateUnits ), NULL, this );
	m_wavelengthVacuumCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::OnWavelengthVacuumChange ), NULL, this );
	m_wavelengthVacuumUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::updateUnits ), NULL, this );
	m_wavelengthMediumCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::OnWavelengthMediumChange ), NULL, this );
	m_wavelengthMediumUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::updateUnits ), NULL, this );
	m_speedUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::updateUnits ), NULL, this );
	m_permittivityCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::OnPermittivityChange ), NULL, this );
	m_button1->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::OnButtonPermittivity ), NULL, this );
	m_permeabilityCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_WAVELENGTH_BASE::OnPermeabilityChange ), NULL, this );

}
