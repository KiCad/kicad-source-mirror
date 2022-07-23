///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/unit_selector.h"

#include "panel_cable_size_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_CABLE_SIZE_BASE::PANEL_CABLE_SIZE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Wire properties") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerLeft;
	fgSizerLeft = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerLeft->AddGrowableCol( 1 );
	fgSizerLeft->SetFlexibleDirection( wxBOTH );
	fgSizerLeft->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText162 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Standard Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText162->Wrap( -1 );
	fgSizerLeft->Add( m_staticText162, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	wxArrayString m_sizeChoiceChoices;
	m_sizeChoice = new wxChoice( sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_sizeChoiceChoices, 0 );
	m_sizeChoice->SetSelection( 0 );
	fgSizerLeft->Add( m_sizeChoice, 0, wxALL|wxEXPAND, 5 );


	fgSizerLeft->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText16 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Diameter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	fgSizerLeft->Add( m_staticText16, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_diameterCtrl = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerLeft->Add( m_diameterCtrl, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_diameterUnitChoices;
	m_diameterUnit = new UNIT_SELECTOR_LEN( sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_diameterUnitChoices, 0 );
	m_diameterUnit->SetSelection( 0 );
	fgSizerLeft->Add( m_diameterUnit, 0, wxALL, 5 );

	m_staticText161 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Area:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText161->Wrap( -1 );
	fgSizerLeft->Add( m_staticText161, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_areaCtrl = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerLeft->Add( m_areaCtrl, 0, wxALL|wxEXPAND, 5 );

	m_staticText1641 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("mm^2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1641->Wrap( -1 );
	fgSizerLeft->Add( m_staticText1641, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxALL, 5 );

	m_staticText18 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Conductor resistivity:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	fgSizerLeft->Add( m_staticText18, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	wxBoxSizer* bSizerResistivity;
	bSizerResistivity = new wxBoxSizer( wxHORIZONTAL );

	m_textCtrlConductorResistivity = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerResistivity->Add( m_textCtrlConductorResistivity, 1, wxEXPAND|wxLEFT, 5 );

	m_button_ResistivityConductor = new wxButton( sbSizer1->GetStaticBox(), wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizerResistivity->Add( m_button_ResistivityConductor, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizerLeft->Add( bSizerResistivity, 1, wxEXPAND, 5 );


	fgSizerLeft->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText16411 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Linear resistance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16411->Wrap( -1 );
	fgSizerLeft->Add( m_staticText16411, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_linResistanceCtrl = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerLeft->Add( m_linResistanceCtrl, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_linResistanceUnitChoices;
	m_linResistanceUnit = new UNIT_SELECTOR_LINEAR_RESISTANCE( sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_linResistanceUnitChoices, 0 );
	m_linResistanceUnit->SetSelection( 0 );
	fgSizerLeft->Add( m_linResistanceUnit, 0, wxALL, 5 );

	m_staticText164 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Frequency for 100% skin depth:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText164->Wrap( -1 );
	fgSizerLeft->Add( m_staticText164, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_frequencyCtrl = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerLeft->Add( m_frequencyCtrl, 0, wxALL|wxEXPAND, 5 );

	wxArrayString m_frequencyUnitChoices;
	m_frequencyUnit = new UNIT_SELECTOR_FREQUENCY( sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_frequencyUnitChoices, 0 );
	m_frequencyUnit->SetSelection( 0 );
	fgSizerLeft->Add( m_frequencyUnit, 0, wxALL, 5 );

	m_staticText1642 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Ampacity:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1642->Wrap( -1 );
	fgSizerLeft->Add( m_staticText1642, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_AmpacityCtrl = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerLeft->Add( m_AmpacityCtrl, 0, wxALL|wxEXPAND, 5 );

	m_staticText16421 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("A"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16421->Wrap( -1 );
	fgSizerLeft->Add( m_staticText16421, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	sbSizer1->Add( fgSizerLeft, 1, wxEXPAND, 5 );


	bSizer4->Add( sbSizer1, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer12;
	sbSizer12 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Application") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerRight;
	fgSizerRight = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerRight->AddGrowableCol( 1 );
	fgSizerRight->SetFlexibleDirection( wxBOTH );
	fgSizerRight->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText163 = new wxStaticText( sbSizer12->GetStaticBox(), wxID_ANY, _("Current:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText163->Wrap( -1 );
	fgSizerRight->Add( m_staticText163, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_currentCtrl = new wxTextCtrl( sbSizer12->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRight->Add( m_currentCtrl, 1, wxALL|wxEXPAND, 5 );

	m_staticText = new wxStaticText( sbSizer12->GetStaticBox(), wxID_ANY, _("A"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText->Wrap( -1 );
	fgSizerRight->Add( m_staticText, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxALL, 5 );

	m_staticText1612 = new wxStaticText( sbSizer12->GetStaticBox(), wxID_ANY, _("Length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1612->Wrap( -1 );
	fgSizerRight->Add( m_staticText1612, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_lengthCtrl = new wxTextCtrl( sbSizer12->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRight->Add( m_lengthCtrl, 1, wxALL|wxEXPAND, 5 );

	wxArrayString m_lengthUnitChoices;
	m_lengthUnit = new UNIT_SELECTOR_LEN_CABLE( sbSizer12->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_lengthUnitChoices, 0 );
	m_lengthUnit->SetSelection( 0 );
	fgSizerRight->Add( m_lengthUnit, 0, wxALL, 5 );

	m_staticText16121 = new wxStaticText( sbSizer12->GetStaticBox(), wxID_ANY, _("Resistance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16121->Wrap( -1 );
	fgSizerRight->Add( m_staticText16121, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_resistanceCtrl = new wxTextCtrl( sbSizer12->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRight->Add( m_resistanceCtrl, 1, wxALL|wxEXPAND, 5 );

	m_staticText161211 = new wxStaticText( sbSizer12->GetStaticBox(), wxID_ANY, _("ohm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText161211->Wrap( -1 );
	fgSizerRight->Add( m_staticText161211, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_staticText161212 = new wxStaticText( sbSizer12->GetStaticBox(), wxID_ANY, _("Voltage drop:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText161212->Wrap( -1 );
	fgSizerRight->Add( m_staticText161212, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_vDropCtrl = new wxTextCtrl( sbSizer12->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRight->Add( m_vDropCtrl, 0, wxALL|wxEXPAND, 5 );

	m_staticText1612121 = new wxStaticText( sbSizer12->GetStaticBox(), wxID_ANY, _("mV"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1612121->Wrap( -1 );
	fgSizerRight->Add( m_staticText1612121, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_staticText1612122 = new wxStaticText( sbSizer12->GetStaticBox(), wxID_ANY, _("Dissipated power:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1612122->Wrap( -1 );
	fgSizerRight->Add( m_staticText1612122, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_powerCtrl = new wxTextCtrl( sbSizer12->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRight->Add( m_powerCtrl, 0, wxALL|wxEXPAND, 5 );

	m_staticText16121211 = new wxStaticText( sbSizer12->GetStaticBox(), wxID_ANY, _("mW"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16121211->Wrap( -1 );
	fgSizerRight->Add( m_staticText16121211, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	sbSizer12->Add( fgSizerRight, 1, wxEXPAND, 5 );


	bSizer4->Add( sbSizer12, 0, wxALL|wxEXPAND, 5 );


	bSizer9->Add( bSizer4, 1, wxEXPAND, 5 );


	bSizer6->Add( bSizer9, 0, wxEXPAND, 5 );


	this->SetSizer( bSizer6 );
	this->Layout();

	// Connect Events
	m_sizeChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnCableSizeChange ), NULL, this );
	m_diameterCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnDiameterChange ), NULL, this );
	m_diameterUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUpdateUnit ), NULL, this );
	m_areaCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnAreaChange ), NULL, this );
	m_textCtrlConductorResistivity->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnConductorResistivityChange ), NULL, this );
	m_button_ResistivityConductor->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnConductorResistivity_Button ), NULL, this );
	m_linResistanceCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnLinResistanceChange ), NULL, this );
	m_linResistanceUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUpdateUnit ), NULL, this );
	m_frequencyCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnFrequencyChange ), NULL, this );
	m_frequencyUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUpdateUnit ), NULL, this );
	m_AmpacityCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnAmpacityChange ), NULL, this );
	m_currentCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnCurrentChange ), NULL, this );
	m_lengthCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnLengthChange ), NULL, this );
	m_lengthUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUpdateUnit ), NULL, this );
	m_resistanceCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnResistanceChange ), NULL, this );
	m_vDropCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnVDropChange ), NULL, this );
	m_powerCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnPowerChange ), NULL, this );
}

PANEL_CABLE_SIZE_BASE::~PANEL_CABLE_SIZE_BASE()
{
	// Disconnect Events
	m_sizeChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnCableSizeChange ), NULL, this );
	m_diameterCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnDiameterChange ), NULL, this );
	m_diameterUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUpdateUnit ), NULL, this );
	m_areaCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnAreaChange ), NULL, this );
	m_textCtrlConductorResistivity->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnConductorResistivityChange ), NULL, this );
	m_button_ResistivityConductor->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnConductorResistivity_Button ), NULL, this );
	m_linResistanceCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnLinResistanceChange ), NULL, this );
	m_linResistanceUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUpdateUnit ), NULL, this );
	m_frequencyCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnFrequencyChange ), NULL, this );
	m_frequencyUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUpdateUnit ), NULL, this );
	m_AmpacityCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnAmpacityChange ), NULL, this );
	m_currentCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnCurrentChange ), NULL, this );
	m_lengthCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnLengthChange ), NULL, this );
	m_lengthUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUpdateUnit ), NULL, this );
	m_resistanceCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnResistanceChange ), NULL, this );
	m_vDropCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnVDropChange ), NULL, this );
	m_powerCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnPowerChange ), NULL, this );

}
