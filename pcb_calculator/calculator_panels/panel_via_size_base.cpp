///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/unit_selector.h"

#include "panel_via_size_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_VIA_SIZE_BASE::PANEL_VIA_SIZE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerViaSize;
	bSizerViaSize = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerViaLeftColumn;
	bSizerViaLeftColumn = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerVS_Inputs;
	sbSizerVS_Inputs = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Parameters") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerVS_Inputs;
	fgSizerVS_Inputs = new wxFlexGridSizer( 0, 3, 4, 0 );
	fgSizerVS_Inputs->AddGrowableCol( 1 );
	fgSizerVS_Inputs->SetFlexibleDirection( wxBOTH );
	fgSizerVS_Inputs->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextHoleDia = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Finished hole diameter (D):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHoleDia->Wrap( -1 );
	m_staticTextHoleDia->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	fgSizerVS_Inputs->Add( m_staticTextHoleDia, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlHoleDia = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlHoleDia, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_choiceHoleDiaChoices;
	m_choiceHoleDia = new UNIT_SELECTOR_LEN( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceHoleDiaChoices, 0 );
	m_choiceHoleDia->SetSelection( 0 );
	fgSizerVS_Inputs->Add( m_choiceHoleDia, 0, wxEXPAND, 5 );

	m_staticTextPlatingThickness = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Plating thickness (T):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPlatingThickness->Wrap( -1 );
	fgSizerVS_Inputs->Add( m_staticTextPlatingThickness, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlPlatingThickness = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlPlatingThickness, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_choicePlatingThicknessChoices;
	m_choicePlatingThickness = new UNIT_SELECTOR_LEN( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choicePlatingThicknessChoices, 0 );
	m_choicePlatingThickness->SetSelection( 0 );
	fgSizerVS_Inputs->Add( m_choicePlatingThickness, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticTextViaLength = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Via length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextViaLength->Wrap( -1 );
	m_staticTextViaLength->SetToolTip( _("Via length is the board thickness for through hole vias") );

	fgSizerVS_Inputs->Add( m_staticTextViaLength, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlViaLength = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlViaLength, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_choiceViaLengthChoices;
	m_choiceViaLength = new UNIT_SELECTOR_LEN( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceViaLengthChoices, 0 );
	m_choiceViaLength->SetSelection( 0 );
	fgSizerVS_Inputs->Add( m_choiceViaLength, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticTextViaPadDia = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Via pad diameter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextViaPadDia->Wrap( -1 );
	m_staticTextViaPadDia->SetToolTip( _("Diameter of pad surrounding via (annular ring)") );

	fgSizerVS_Inputs->Add( m_staticTextViaPadDia, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlViaPadDia = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlViaPadDia, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_choiceViaPadDiaChoices;
	m_choiceViaPadDia = new UNIT_SELECTOR_LEN( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceViaPadDiaChoices, 0 );
	m_choiceViaPadDia->SetSelection( 0 );
	fgSizerVS_Inputs->Add( m_choiceViaPadDia, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticTextClearanceDia = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Clearance hole diameter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextClearanceDia->Wrap( -1 );
	m_staticTextClearanceDia->SetToolTip( _("Diameter of clearance hole in ground plane(s)") );

	fgSizerVS_Inputs->Add( m_staticTextClearanceDia, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlClearanceDia = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlClearanceDia, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_choiceClearanceDiaChoices;
	m_choiceClearanceDia = new UNIT_SELECTOR_LEN( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceClearanceDiaChoices, 0 );
	m_choiceClearanceDia->SetSelection( 0 );
	fgSizerVS_Inputs->Add( m_choiceClearanceDia, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticTextImpedance = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Z0:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextImpedance->Wrap( -1 );
	m_staticTextImpedance->SetToolTip( _("Characteristic impedance of conductor") );

	fgSizerVS_Inputs->Add( m_staticTextImpedance, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlImpedance = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlImpedance, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_choiceImpedanceChoices;
	m_choiceImpedance = new UNIT_SELECTOR_RESISTOR( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceImpedanceChoices, 0 );
	m_choiceImpedance->SetSelection( 0 );
	fgSizerVS_Inputs->Add( m_choiceImpedance, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticAppliedCurrent = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Applied current:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticAppliedCurrent->Wrap( -1 );
	fgSizerVS_Inputs->Add( m_staticAppliedCurrent, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlAppliedCurrent = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlAppliedCurrent, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_staticTextAppliedCurrentUnits = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("A"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAppliedCurrentUnits->Wrap( -1 );
	fgSizerVS_Inputs->Add( m_staticTextAppliedCurrentUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticTextResistivity = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Plating resistivity:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextResistivity->Wrap( -1 );
	m_staticTextResistivity->SetToolTip( _("Specific resistance in ohms * meters") );

	fgSizerVS_Inputs->Add( m_staticTextResistivity, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizerResistivity;
	bSizerResistivity = new wxBoxSizer( wxHORIZONTAL );

	m_textCtrlPlatingResistivity = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerResistivity->Add( m_textCtrlPlatingResistivity, 1, wxEXPAND|wxLEFT, 5 );

	m_button_ResistivityVia = new wxButton( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizerResistivity->Add( m_button_ResistivityVia, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizerVS_Inputs->Add( bSizerResistivity, 1, wxEXPAND, 5 );

	m_viaResistivityUnits = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("ohm-meter"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaResistivityUnits->Wrap( -1 );
	fgSizerVS_Inputs->Add( m_viaResistivityUnits, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticTextPermittivity = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Substrate relative permittivity:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPermittivity->Wrap( -1 );
	m_staticTextPermittivity->SetToolTip( _("Relative dielectric constant (epsilon r)") );

	fgSizerVS_Inputs->Add( m_staticTextPermittivity, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizerPermittivity;
	bSizerPermittivity = new wxBoxSizer( wxHORIZONTAL );

	m_textCtrlPlatingPermittivity = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerPermittivity->Add( m_textCtrlPlatingPermittivity, 1, wxEXPAND|wxLEFT, 5 );

	m_button_Permittivity = new wxButton( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizerPermittivity->Add( m_button_Permittivity, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizerVS_Inputs->Add( bSizerPermittivity, 1, wxEXPAND, 5 );


	fgSizerVS_Inputs->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextTemperatureDiff = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Temperature rise:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTemperatureDiff->Wrap( -1 );
	m_staticTextTemperatureDiff->SetToolTip( _("Maximum acceptable rise in temperature") );

	fgSizerVS_Inputs->Add( m_staticTextTemperatureDiff, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlTemperatureDiff = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlTemperatureDiff, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_viaTempUnits = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("deg C"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaTempUnits->Wrap( -1 );
	fgSizerVS_Inputs->Add( m_viaTempUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticTextRiseTime = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("Pulse rise time:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRiseTime->Wrap( -1 );
	m_staticTextRiseTime->SetToolTip( _("Pulse rise time to calculate reactance") );

	fgSizerVS_Inputs->Add( m_staticTextRiseTime, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlRiseTime = new wxTextCtrl( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerVS_Inputs->Add( m_textCtrlRiseTime, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticTextRiseTimeUnits = new wxStaticText( sbSizerVS_Inputs->GetStaticBox(), wxID_ANY, _("ns"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRiseTimeUnits->Wrap( -1 );
	m_staticTextRiseTimeUnits->SetToolTip( _("nanoseconds") );

	fgSizerVS_Inputs->Add( m_staticTextRiseTimeUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbSizerVS_Inputs->Add( fgSizerVS_Inputs, 0, wxALL|wxEXPAND, 5 );


	bSizerViaLeftColumn->Add( sbSizerVS_Inputs, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextWarning = new wxStaticText( this, wxID_ANY, _("Warning:\nVia pad diameter >= Clearance hole diameter.\nSome parameters cannot be calculated for a via inside a copper zone."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextWarning->Wrap( -1 );
	m_staticTextWarning->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizerViaLeftColumn->Add( m_staticTextWarning, 0, wxALL|wxEXPAND, 10 );


	bSizerViaSize->Add( bSizerViaLeftColumn, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );

	bSizerRight->SetMinSize( wxSize( -1,460 ) );
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerVS_Result;
	sbSizerVS_Result = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Results") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerTW_Results11;
	fgSizerTW_Results11 = new wxFlexGridSizer( 0, 3, 5, 0 );
	fgSizerTW_Results11->AddGrowableCol( 1 );
	fgSizerTW_Results11->SetFlexibleDirection( wxBOTH );
	fgSizerTW_Results11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextArea11 = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Resistance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextArea11->Wrap( -1 );
	fgSizerTW_Results11->Add( m_staticTextArea11, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_ViaResistance = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaResistance->Wrap( -1 );
	fgSizerTW_Results11->Add( m_ViaResistance, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_viaResUnits = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("ohm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaResUnits->Wrap( -1 );
	fgSizerTW_Results11->Add( m_viaResUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText65111 = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Voltage drop:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText65111->Wrap( -1 );
	fgSizerTW_Results11->Add( m_staticText65111, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_ViaVoltageDrop = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaVoltageDrop->Wrap( -1 );
	fgSizerTW_Results11->Add( m_ViaVoltageDrop, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticText8411 = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8411->Wrap( -1 );
	fgSizerTW_Results11->Add( m_staticText8411, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText66111 = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Power loss:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText66111->Wrap( -1 );
	fgSizerTW_Results11->Add( m_staticText66111, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_ViaPowerLoss = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaPowerLoss->Wrap( -1 );
	fgSizerTW_Results11->Add( m_ViaPowerLoss, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticText8311 = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("W"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8311->Wrap( -1 );
	fgSizerTW_Results11->Add( m_staticText8311, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText79211 = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Thermal resistance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText79211->Wrap( -1 );
	m_staticText79211->SetToolTip( _("Using thermal conductivity value 401 Watts/(meter-Kelvin)") );

	fgSizerTW_Results11->Add( m_staticText79211, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_ViaThermalResistance = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaThermalResistance->Wrap( -1 );
	fgSizerTW_Results11->Add( m_ViaThermalResistance, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_viaThermalResUnits = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("deg C/W"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaThermalResUnits->Wrap( -1 );
	fgSizerTW_Results11->Add( m_viaThermalResUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticTextAmpacity = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Estimated ampacity:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAmpacity->Wrap( -1 );
	m_staticTextAmpacity->SetToolTip( _("Based on temperature rise") );

	fgSizerTW_Results11->Add( m_staticTextAmpacity, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_ViaAmpacity = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaAmpacity->Wrap( -1 );
	fgSizerTW_Results11->Add( m_ViaAmpacity, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticTextAmpacityUnits = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("A"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAmpacityUnits->Wrap( -1 );
	fgSizerTW_Results11->Add( m_staticTextAmpacityUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticTextCapacitance = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Capacitance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCapacitance->Wrap( -1 );
	fgSizerTW_Results11->Add( m_staticTextCapacitance, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_ViaCapacitance = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaCapacitance->Wrap( -1 );
	fgSizerTW_Results11->Add( m_ViaCapacitance, 0, wxRIGHT|wxLEFT, 5 );

	m_staticTextCapacitanceUnits = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("pF"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCapacitanceUnits->Wrap( -1 );
	m_staticTextCapacitanceUnits->SetToolTip( _("pico-Farad") );

	fgSizerTW_Results11->Add( m_staticTextCapacitanceUnits, 0, 0, 5 );

	m_staticTextRiseTimeOutput = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Rise time degradation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRiseTimeOutput->Wrap( -1 );
	m_staticTextRiseTimeOutput->SetToolTip( _("Rise time degradation for given Z0 and calculated capacitance") );

	fgSizerTW_Results11->Add( m_staticTextRiseTimeOutput, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_RiseTimeOutput = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RiseTimeOutput->Wrap( -1 );
	fgSizerTW_Results11->Add( m_RiseTimeOutput, 0, wxRIGHT|wxLEFT, 5 );

	m_staticTextRiseTimeOutputUnits = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("ps"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRiseTimeOutputUnits->Wrap( -1 );
	m_staticTextRiseTimeOutputUnits->SetToolTip( _("picoseconds") );

	fgSizerTW_Results11->Add( m_staticTextRiseTimeOutputUnits, 0, 0, 5 );

	m_staticTextInductance = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Inductance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInductance->Wrap( -1 );
	fgSizerTW_Results11->Add( m_staticTextInductance, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_Inductance = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Inductance->Wrap( -1 );
	fgSizerTW_Results11->Add( m_Inductance, 0, wxRIGHT|wxLEFT, 5 );

	m_staticTextInductanceUnits = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("nH"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInductanceUnits->Wrap( -1 );
	m_staticTextInductanceUnits->SetToolTip( _("nano-Henry") );

	fgSizerTW_Results11->Add( m_staticTextInductanceUnits, 0, 0, 5 );

	m_staticTextReactance = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("Reactance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextReactance->Wrap( -1 );
	m_staticTextReactance->SetToolTip( _("Inductive reactance for given rise time and calculated inductance") );

	fgSizerTW_Results11->Add( m_staticTextReactance, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_Reactance = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Reactance->Wrap( -1 );
	fgSizerTW_Results11->Add( m_Reactance, 0, wxRIGHT|wxLEFT, 5 );

	m_viaReactanceUnits = new wxStaticText( sbSizerVS_Result->GetStaticBox(), wxID_ANY, _("ohm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaReactanceUnits->Wrap( -1 );
	fgSizerTW_Results11->Add( m_viaReactanceUnits, 0, 0, 5 );


	sbSizerVS_Result->Add( fgSizerTW_Results11, 0, wxALL|wxEXPAND, 5 );


	bSizer6->Add( sbSizerVS_Result, 0, wxALL, 5 );

	m_viaBitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_viaBitmap->SetToolTip( _("Top view of via") );

	bSizer6->Add( m_viaBitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 10 );


	bSizerRight->Add( bSizer6, 0, 0, 5 );


	bSizerRight->Add( 0, 0, 1, 0, 5 );

	m_buttonViaReset = new wxButton( this, wxID_ANY, _("Reset to Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_buttonViaReset, 0, wxALIGN_RIGHT|wxALL, 10 );


	bSizerViaSize->Add( bSizerRight, 1, wxEXPAND, 5 );


	this->SetSizer( bSizerViaSize );
	this->Layout();
	bSizerViaSize->Fit( this );

	// Connect Events
	m_textCtrlHoleDia->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_choiceHoleDia->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlPlatingThickness->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_choicePlatingThickness->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlViaLength->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_choiceViaLength->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlViaPadDia->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_choiceViaPadDia->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlClearanceDia->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_choiceClearanceDia->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlImpedance->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_choiceImpedance->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlAppliedCurrent->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlPlatingResistivity->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_button_ResistivityVia->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaRho_Button ), NULL, this );
	m_textCtrlPlatingPermittivity->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_button_Permittivity->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaEpsilonR_Button ), NULL, this );
	m_textCtrlTemperatureDiff->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlRiseTime->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_staticTextWarning->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_VIA_SIZE_BASE::onUpdateViaCalcErrorText ), NULL, this );
	m_buttonViaReset->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaResetButtonClick ), NULL, this );
}

PANEL_VIA_SIZE_BASE::~PANEL_VIA_SIZE_BASE()
{
	// Disconnect Events
	m_textCtrlHoleDia->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_choiceHoleDia->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlPlatingThickness->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_choicePlatingThickness->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlViaLength->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_choiceViaLength->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlViaPadDia->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_choiceViaPadDia->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlClearanceDia->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_choiceClearanceDia->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlImpedance->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_choiceImpedance->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlAppliedCurrent->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlPlatingResistivity->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_button_ResistivityVia->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaRho_Button ), NULL, this );
	m_textCtrlPlatingPermittivity->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_button_Permittivity->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaEpsilonR_Button ), NULL, this );
	m_textCtrlTemperatureDiff->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_textCtrlRiseTime->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaCalculate ), NULL, this );
	m_staticTextWarning->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_VIA_SIZE_BASE::onUpdateViaCalcErrorText ), NULL, this );
	m_buttonViaReset->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_VIA_SIZE_BASE::OnViaResetButtonClick ), NULL, this );

}
