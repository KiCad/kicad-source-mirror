///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/unit_selector.h"

#include "panel_cable_size_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_CABLE_SIZE_BASE::PANEL_CABLE_SIZE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizerLeft;
	sbSizerLeft = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Wire properties") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerLeft;
	fgSizerLeft = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerLeft->AddGrowableCol( 1 );
	fgSizerLeft->SetFlexibleDirection( wxBOTH );
	fgSizerLeft->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextSize = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Standard Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSize->Wrap( -1 );
	fgSizerLeft->Add( m_staticTextSize, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_sizeChoiceChoices;
	m_sizeChoice = new wxChoice( sbSizerLeft->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_sizeChoiceChoices, 0 );
	m_sizeChoice->SetSelection( 0 );
	fgSizerLeft->Add( m_sizeChoice, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizerLeft->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextDiameter = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Diameter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDiameter->Wrap( -1 );
	fgSizerLeft->Add( m_staticTextDiameter, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_diameterCtrl = new wxTextCtrl( sbSizerLeft->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerLeft->Add( m_diameterCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxArrayString m_diameterUnitChoices;
	m_diameterUnit = new UNIT_SELECTOR_LEN( sbSizerLeft->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_diameterUnitChoices, 0 );
	m_diameterUnit->SetSelection( 0 );
	fgSizerLeft->Add( m_diameterUnit, 0, wxEXPAND|wxTOP, 5 );

	m_staticTextArea = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Area:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextArea->Wrap( -1 );
	fgSizerLeft->Add( m_staticTextArea, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_areaCtrl = new wxTextCtrl( sbSizerLeft->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerLeft->Add( m_areaCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_stUnitmmSq = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("mm^2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stUnitmmSq->Wrap( -1 );
	fgSizerLeft->Add( m_stUnitmmSq, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxTOP, 5 );

	m_staticTextResitivity = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Conductor resistivity:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextResitivity->Wrap( -1 );
	m_staticTextResitivity->SetToolTip( _("Specific resistance in Ohm*m at 20 deg C") );

	fgSizerLeft->Add( m_staticTextResitivity, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	wxBoxSizer* bSizerResistivity;
	bSizerResistivity = new wxBoxSizer( wxHORIZONTAL );

	m_textCtrlConductorResistivity = new wxTextCtrl( sbSizerLeft->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlConductorResistivity->SetMinSize( wxSize( 100,-1 ) );

	bSizerResistivity->Add( m_textCtrlConductorResistivity, 1, wxEXPAND|wxLEFT|wxTOP, 5 );

	m_button_ResistivityConductor = new wxButton( sbSizerLeft->GetStaticBox(), wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizerResistivity->Add( m_button_ResistivityConductor, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


	fgSizerLeft->Add( bSizerResistivity, 0, wxEXPAND, 5 );

	m_stUnitOhmMeter = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("ohm-meter"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stUnitOhmMeter->Wrap( -1 );
	fgSizerLeft->Add( m_stUnitOhmMeter, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxTOP, 5 );

	m_staticTextTempCoeff = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Temperature Coefficient:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTempCoeff->Wrap( -1 );
	m_staticTextTempCoeff->SetToolTip( _("Thermal coefficient at 20 deg C") );

	fgSizerLeft->Add( m_staticTextTempCoeff, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	wxBoxSizer* bSizerTempCoeff;
	bSizerTempCoeff = new wxBoxSizer( wxHORIZONTAL );

	m_textCtrlConductorThermCoef = new wxTextCtrl( sbSizerLeft->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerTempCoeff->Add( m_textCtrlConductorThermCoef, 1, wxEXPAND|wxLEFT|wxTOP, 5 );

	m_button_Temp_Coef_Conductor = new wxButton( sbSizerLeft->GetStaticBox(), wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizerTempCoeff->Add( m_button_Temp_Coef_Conductor, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


	fgSizerLeft->Add( bSizerTempCoeff, 0, wxEXPAND, 5 );


	fgSizerLeft->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextLinRes = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Linear resistance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLinRes->Wrap( -1 );
	fgSizerLeft->Add( m_staticTextLinRes, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_linResistanceCtrl = new wxTextCtrl( sbSizerLeft->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerLeft->Add( m_linResistanceCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxArrayString m_linResistanceUnitChoices;
	m_linResistanceUnit = new UNIT_SELECTOR_LINEAR_RESISTANCE( sbSizerLeft->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_linResistanceUnitChoices, 0 );
	m_linResistanceUnit->SetSelection( 0 );
	fgSizerLeft->Add( m_linResistanceUnit, 0, wxEXPAND|wxTOP, 5 );

	m_staticTextSkin = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Frequency for 100% skin depth:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSkin->Wrap( -1 );
	fgSizerLeft->Add( m_staticTextSkin, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_frequencyCtrl = new wxTextCtrl( sbSizerLeft->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerLeft->Add( m_frequencyCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxArrayString m_frequencyUnitChoices;
	m_frequencyUnit = new UNIT_SELECTOR_FREQUENCY( sbSizerLeft->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_frequencyUnitChoices, 0 );
	m_frequencyUnit->SetSelection( 0 );
	fgSizerLeft->Add( m_frequencyUnit, 0, wxEXPAND|wxTOP, 5 );

	m_staticTextAmpacity = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Ampacity:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAmpacity->Wrap( -1 );
	fgSizerLeft->Add( m_staticTextAmpacity, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_AmpacityCtrl = new wxTextCtrl( sbSizerLeft->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerLeft->Add( m_AmpacityCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticText16421 = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("A"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16421->Wrap( -1 );
	fgSizerLeft->Add( m_staticText16421, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_staticTextDensity = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("Current density"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDensity->Wrap( -1 );
	fgSizerLeft->Add( m_staticTextDensity, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_slCurrentDensity = new wxSlider( sbSizerLeft->GetStaticBox(), wxID_ANY, 3, 3, 12, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS|wxSL_VALUE_LABEL );
	fgSizerLeft->Add( m_slCurrentDensity, 0, wxEXPAND|wxRIGHT|wxTOP, 5 );

	m_stUnitAmp_mmSq = new wxStaticText( sbSizerLeft->GetStaticBox(), wxID_ANY, _("A/mm^2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stUnitAmp_mmSq->Wrap( -1 );
	fgSizerLeft->Add( m_stUnitAmp_mmSq, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );


	sbSizerLeft->Add( fgSizerLeft, 0, wxALL|wxEXPAND, 5 );


	bSizer4->Add( sbSizerLeft, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerRight;
	sbSizerRight = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Application") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerRight;
	fgSizerRight = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerRight->AddGrowableCol( 1 );
	fgSizerRight->SetFlexibleDirection( wxBOTH );
	fgSizerRight->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextCableTemp = new wxStaticText( sbSizerRight->GetStaticBox(), wxID_ANY, _("Cable temperature:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCableTemp->Wrap( -1 );
	m_staticTextCableTemp->SetToolTip( _("Off-Load max conductor temp. Reference: 20 deg C") );

	fgSizerRight->Add( m_staticTextCableTemp, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_conductorTempCtrl = new wxTextCtrl( sbSizerRight->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_conductorTempCtrl->SetMinSize( wxSize( 120,-1 ) );

	fgSizerRight->Add( m_conductorTempCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_stUnitDegC = new wxStaticText( sbSizerRight->GetStaticBox(), wxID_ANY, _("deg C"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stUnitDegC->Wrap( -1 );
	fgSizerRight->Add( m_stUnitDegC, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );

	m_staticTextCurrent = new wxStaticText( sbSizerRight->GetStaticBox(), wxID_ANY, _("Current:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCurrent->Wrap( -1 );
	fgSizerRight->Add( m_staticTextCurrent, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_currentCtrl = new wxTextCtrl( sbSizerRight->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRight->Add( m_currentCtrl, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticText = new wxStaticText( sbSizerRight->GetStaticBox(), wxID_ANY, _("A"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText->Wrap( -1 );
	fgSizerRight->Add( m_staticText, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxTOP, 5 );

	m_staticTextLen = new wxStaticText( sbSizerRight->GetStaticBox(), wxID_ANY, _("Length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLen->Wrap( -1 );
	m_staticTextLen->SetToolTip( _("Length includes the return path") );

	fgSizerRight->Add( m_staticTextLen, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_lengthCtrl = new wxTextCtrl( sbSizerRight->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRight->Add( m_lengthCtrl, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxArrayString m_lengthUnitChoices;
	m_lengthUnit = new UNIT_SELECTOR_LEN_CABLE( sbSizerRight->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_lengthUnitChoices, 0 );
	m_lengthUnit->SetSelection( 0 );
	fgSizerRight->Add( m_lengthUnit, 0, wxTOP|wxEXPAND, 5 );

	m_staticTextResDC = new wxStaticText( sbSizerRight->GetStaticBox(), wxID_ANY, _("Resistance DC:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextResDC->Wrap( -1 );
	m_staticTextResDC->SetToolTip( _("DC Resistance of the conductor") );

	fgSizerRight->Add( m_staticTextResDC, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_resistanceDcCtrl = new wxTextCtrl( sbSizerRight->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRight->Add( m_resistanceDcCtrl, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_stUnitOhm = new wxStaticText( sbSizerRight->GetStaticBox(), wxID_ANY, _("ohm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stUnitOhm->Wrap( -1 );
	fgSizerRight->Add( m_stUnitOhm, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_staticTextDrop = new wxStaticText( sbSizerRight->GetStaticBox(), wxID_ANY, _("Voltage drop:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDrop->Wrap( -1 );
	fgSizerRight->Add( m_staticTextDrop, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_vDropCtrl = new wxTextCtrl( sbSizerRight->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRight->Add( m_vDropCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxArrayString m_vDropUnitChoices;
	m_vDropUnit = new UNIT_SELECTOR_VOLTAGE( sbSizerRight->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_vDropUnitChoices, 0 );
	m_vDropUnit->SetSelection( 0 );
	fgSizerRight->Add( m_vDropUnit, 0, wxEXPAND|wxTOP, 5 );

	m_staticTextPower = new wxStaticText( sbSizerRight->GetStaticBox(), wxID_ANY, _("Dissipated power:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPower->Wrap( -1 );
	fgSizerRight->Add( m_staticTextPower, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_powerCtrl = new wxTextCtrl( sbSizerRight->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRight->Add( m_powerCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxArrayString m_powerUnitChoices;
	m_powerUnit = new UNIT_SELECTOR_POWER( sbSizerRight->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_powerUnitChoices, 0 );
	m_powerUnit->SetSelection( 0 );
	fgSizerRight->Add( m_powerUnit, 0, wxEXPAND|wxTOP, 5 );


	sbSizerRight->Add( fgSizerRight, 1, wxALL, 5 );


	bSizer4->Add( sbSizerRight, 1, wxALL|wxEXPAND, 5 );


	bSizer9->Add( bSizer4, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bSizerMain->Add( bSizer9, 0, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	// Connect Events
	m_sizeChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnCableSizeChange ), NULL, this );
	m_diameterCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnDiameterChange ), NULL, this );
	m_diameterUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUpdateUnit ), NULL, this );
	m_areaCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnAreaChange ), NULL, this );
	m_textCtrlConductorResistivity->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnConductorResistivityChange ), NULL, this );
	m_button_ResistivityConductor->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnConductorResistivity_Button ), NULL, this );
	m_textCtrlConductorThermCoef->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnConductorThermCoefChange ), NULL, this );
	m_button_Temp_Coef_Conductor->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnConductorThermCoefChange_Button ), NULL, this );
	m_linResistanceCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnLinResistanceChange ), NULL, this );
	m_linResistanceUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUpdateUnit ), NULL, this );
	m_frequencyCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnFrequencyChange ), NULL, this );
	m_frequencyUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUpdateUnit ), NULL, this );
	m_AmpacityCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnAmpacityChange ), NULL, this );
	m_slCurrentDensity->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( PANEL_CABLE_SIZE_BASE::onUpdateCurrentDensity ), NULL, this );
	m_slCurrentDensity->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( PANEL_CABLE_SIZE_BASE::onUpdateCurrentDensity ), NULL, this );
	m_slCurrentDensity->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( PANEL_CABLE_SIZE_BASE::onUpdateCurrentDensity ), NULL, this );
	m_slCurrentDensity->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( PANEL_CABLE_SIZE_BASE::onUpdateCurrentDensity ), NULL, this );
	m_slCurrentDensity->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( PANEL_CABLE_SIZE_BASE::onUpdateCurrentDensity ), NULL, this );
	m_slCurrentDensity->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( PANEL_CABLE_SIZE_BASE::onUpdateCurrentDensity ), NULL, this );
	m_slCurrentDensity->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( PANEL_CABLE_SIZE_BASE::onUpdateCurrentDensity ), NULL, this );
	m_slCurrentDensity->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( PANEL_CABLE_SIZE_BASE::onUpdateCurrentDensity ), NULL, this );
	m_slCurrentDensity->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( PANEL_CABLE_SIZE_BASE::onUpdateCurrentDensity ), NULL, this );
	m_conductorTempCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnConductorTempChange ), NULL, this );
	m_currentCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnCurrentChange ), NULL, this );
	m_lengthCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnLengthChange ), NULL, this );
	m_lengthUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUpdateUnit ), NULL, this );
	m_resistanceDcCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnResistanceDcChange ), NULL, this );
	m_vDropCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnVDropChange ), NULL, this );
	m_vDropUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUpdateUnit ), NULL, this );
	m_powerCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnPowerChange ), NULL, this );
	m_powerUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUpdateUnit ), NULL, this );
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
	m_textCtrlConductorThermCoef->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnConductorThermCoefChange ), NULL, this );
	m_button_Temp_Coef_Conductor->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnConductorThermCoefChange_Button ), NULL, this );
	m_linResistanceCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnLinResistanceChange ), NULL, this );
	m_linResistanceUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUpdateUnit ), NULL, this );
	m_frequencyCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnFrequencyChange ), NULL, this );
	m_frequencyUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUpdateUnit ), NULL, this );
	m_AmpacityCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnAmpacityChange ), NULL, this );
	m_slCurrentDensity->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( PANEL_CABLE_SIZE_BASE::onUpdateCurrentDensity ), NULL, this );
	m_slCurrentDensity->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( PANEL_CABLE_SIZE_BASE::onUpdateCurrentDensity ), NULL, this );
	m_slCurrentDensity->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( PANEL_CABLE_SIZE_BASE::onUpdateCurrentDensity ), NULL, this );
	m_slCurrentDensity->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( PANEL_CABLE_SIZE_BASE::onUpdateCurrentDensity ), NULL, this );
	m_slCurrentDensity->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( PANEL_CABLE_SIZE_BASE::onUpdateCurrentDensity ), NULL, this );
	m_slCurrentDensity->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( PANEL_CABLE_SIZE_BASE::onUpdateCurrentDensity ), NULL, this );
	m_slCurrentDensity->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( PANEL_CABLE_SIZE_BASE::onUpdateCurrentDensity ), NULL, this );
	m_slCurrentDensity->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( PANEL_CABLE_SIZE_BASE::onUpdateCurrentDensity ), NULL, this );
	m_slCurrentDensity->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( PANEL_CABLE_SIZE_BASE::onUpdateCurrentDensity ), NULL, this );
	m_conductorTempCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnConductorTempChange ), NULL, this );
	m_currentCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnCurrentChange ), NULL, this );
	m_lengthCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnLengthChange ), NULL, this );
	m_lengthUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUpdateUnit ), NULL, this );
	m_resistanceDcCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnResistanceDcChange ), NULL, this );
	m_vDropCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnVDropChange ), NULL, this );
	m_vDropUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUpdateUnit ), NULL, this );
	m_powerCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnPowerChange ), NULL, this );
	m_powerUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUpdateUnit ), NULL, this );

}
