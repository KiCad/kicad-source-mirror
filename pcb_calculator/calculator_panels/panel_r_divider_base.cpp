///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_r_divider_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_R_DIVIDER_BASE::PANEL_R_DIVIDER_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerRDivider;
	bSizerRDivider = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMiddleSizerRDivider;
	bMiddleSizerRDivider = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizerRDividerInput;
	sbSizerRDividerInput = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Inputs") ), wxVERTICAL );

	wxBoxSizer* bSizerMode;
	bSizerMode = new wxBoxSizer( wxHORIZONTAL );

	m_rbModeRatio = new wxRadioButton( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("Resistance ratio"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_rbModeRatio->SetValue( true );
	bSizerMode->Add( m_rbModeRatio, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_rbModeVoltage = new wxRadioButton( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("Voltage divider"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerMode->Add( m_rbModeVoltage, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	sbSizerRDividerInput->Add( bSizerMode, 0, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizerInputs;
	fgSizerInputs = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerInputs->AddGrowableCol( 1 );
	fgSizerInputs->SetFlexibleDirection( wxHORIZONTAL );
	fgSizerInputs->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_ratioLabel = new wxStaticText( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("Ratio (R1/R2):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ratioLabel->Wrap( -1 );
	fgSizerInputs->Add( m_ratioLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_ratioValue = new wxTextCtrl( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ratioValue->SetMinSize( wxSize( 100,-1 ) );

	fgSizerInputs->Add( m_ratioValue, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );


	fgSizerInputs->Add( 0, 0, 1, wxEXPAND, 5 );

	m_vinLabel = new wxStaticText( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("Vin:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_vinLabel->Wrap( -1 );
	fgSizerInputs->Add( m_vinLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_vinValue = new wxTextCtrl( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_vinValue->SetMinSize( wxSize( 100,-1 ) );

	fgSizerInputs->Add( m_vinValue, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );

	m_vinUnit = new wxStaticText( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_vinUnit->Wrap( -1 );
	fgSizerInputs->Add( m_vinUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_voutLabel = new wxStaticText( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("Vout:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_voutLabel->Wrap( -1 );
	fgSizerInputs->Add( m_voutLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_voutValue = new wxTextCtrl( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_voutValue->SetMinSize( wxSize( 100,-1 ) );

	fgSizerInputs->Add( m_voutValue, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );

	m_voutUnit = new wxStaticText( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_voutUnit->Wrap( -1 );
	fgSizerInputs->Add( m_voutUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_impedanceLabel = new wxStaticText( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("Impedance mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_impedanceLabel->Wrap( -1 );
	fgSizerInputs->Add( m_impedanceLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxBoxSizer* bSizerImpedanceMode;
	bSizerImpedanceMode = new wxBoxSizer( wxHORIZONTAL );

	m_impedanceSeries = new wxRadioButton( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("R1 + R2"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_impedanceSeries->SetValue( true );
	bSizerImpedanceMode->Add( m_impedanceSeries, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_impedanceParallel = new wxRadioButton( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("Output"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerImpedanceMode->Add( m_impedanceParallel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	fgSizerInputs->Add( bSizerImpedanceMode, 0, wxALIGN_CENTER_VERTICAL, 0 );


	fgSizerInputs->Add( 0, 0, 1, wxEXPAND, 5 );

	m_impedanceValueLabel = new wxStaticText( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("Target impedance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_impedanceValueLabel->Wrap( -1 );
	fgSizerInputs->Add( m_impedanceValueLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_impedanceValue = new wxTextCtrl( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_impedanceValue->SetMinSize( wxSize( 100,-1 ) );

	fgSizerInputs->Add( m_impedanceValue, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );

	m_impedanceUnit = new wxStaticText( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("kΩ"), wxDefaultPosition, wxDefaultSize, 0 );
	m_impedanceUnit->Wrap( -1 );
	fgSizerInputs->Add( m_impedanceUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	sbSizerRDividerInput->Add( fgSizerInputs, 0, wxEXPAND|wxALL, 5 );

	m_staticline6 = new wxStaticLine( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sbSizerRDividerInput->Add( m_staticline6, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxFlexGridSizer* fgSizerESeriesButtons;
	fgSizerESeriesButtons = new wxFlexGridSizer( 2, 4, 0, 0 );
	fgSizerESeriesButtons->SetFlexibleDirection( wxBOTH );
	fgSizerESeriesButtons->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_e3 = new wxRadioButton( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("E3"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	fgSizerESeriesButtons->Add( m_e3, 0, wxALL, 5 );

	m_e6 = new wxRadioButton( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("E6"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerESeriesButtons->Add( m_e6, 0, wxALL, 5 );

	m_e12 = new wxRadioButton( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("E12"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerESeriesButtons->Add( m_e12, 0, wxALL, 5 );


	fgSizerESeriesButtons->Add( 0, 0, 1, wxEXPAND, 5 );

	m_e24 = new wxRadioButton( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("E24"), wxDefaultPosition, wxDefaultSize, 0 );
	m_e24->SetValue( true );
	fgSizerESeriesButtons->Add( m_e24, 0, wxALL, 5 );

	m_e48 = new wxRadioButton( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("E48"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerESeriesButtons->Add( m_e48, 0, wxALL, 5 );

	m_e96 = new wxRadioButton( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("E96"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerESeriesButtons->Add( m_e96, 0, wxALL, 5 );

	m_e192 = new wxRadioButton( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("E192"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerESeriesButtons->Add( m_e192, 0, wxALL, 5 );


	sbSizerRDividerInput->Add( fgSizerESeriesButtons, 0, wxEXPAND, 5 );

	m_staticline61 = new wxStaticLine( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sbSizerRDividerInput->Add( m_staticline61, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );

	wxBoxSizer* bSizerCalculate;
	bSizerCalculate = new wxBoxSizer( wxHORIZONTAL );


	bSizerCalculate->Add( 0, 0, 1, wxEXPAND, 5 );

	m_btnCalculate = new wxButton( sbSizerRDividerInput->GetStaticBox(), wxID_ANY, _("Calculate"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerCalculate->Add( m_btnCalculate, 0, wxALL, 5 );


	bSizerCalculate->Add( 0, 0, 1, wxEXPAND, 5 );


	sbSizerRDividerInput->Add( bSizerCalculate, 1, wxEXPAND, 5 );


	bMiddleSizerRDivider->Add( sbSizerRDividerInput, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxStaticBoxSizer* sbSizerRDividerSolutions;
	sbSizerRDividerSolutions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Solutions") ), wxVERTICAL );

	m_resultsGrid = new wxGrid( sbSizerRDividerSolutions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_resultsGrid->CreateGrid( 9, 6 );
	m_resultsGrid->EnableEditing( false );
	m_resultsGrid->EnableGridLines( true );
	m_resultsGrid->EnableDragGridSize( false );
	m_resultsGrid->SetMargins( 0, 0 );

	// Columns
	m_resultsGrid->AutoSizeColumns();
	m_resultsGrid->EnableDragColMove( false );
	m_resultsGrid->EnableDragColSize( true );
	m_resultsGrid->SetColLabelValue( 0, _("R1") );
	m_resultsGrid->SetColLabelValue( 1, _("R2") );
	m_resultsGrid->SetColLabelValue( 2, _("Ratio") );
	m_resultsGrid->SetColLabelValue( 3, _("Ratio error") );
	m_resultsGrid->SetColLabelValue( 4, _("Impedance") );
	m_resultsGrid->SetColLabelValue( 5, _("Impedance error") );
	m_resultsGrid->SetColLabelSize( 30 );
	m_resultsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_resultsGrid->EnableDragRowSize( false );
	m_resultsGrid->SetRowLabelSize( 0 );
	m_resultsGrid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_resultsGrid->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
	sbSizerRDividerSolutions->Add( m_resultsGrid, 1, wxALL|wxEXPAND, 5 );


	bMiddleSizerRDivider->Add( sbSizerRDividerSolutions, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerRDivider->Add( bMiddleSizerRDivider, 0, wxEXPAND|wxRIGHT|wxTOP, 5 );

	wxBoxSizer* bLowerRDivider;
	bLowerRDivider = new wxBoxSizer( wxHORIZONTAL );

	m_bitmapDivider = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bLowerRDivider->Add( m_bitmapDivider, 0, wxALIGN_CENTER_VERTICAL|wxALL, 10 );

	wxStaticBoxSizer* sbLowerSizerRDividerHelp;
	sbLowerSizerRDividerHelp = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Help") ), wxVERTICAL );

	m_panelHelp = new HTML_WINDOW( sbLowerSizerRDividerHelp->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	m_panelHelp->SetMinSize( wxSize( -1,100 ) );

	sbLowerSizerRDividerHelp->Add( m_panelHelp, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bLowerRDivider->Add( sbLowerSizerRDividerHelp, 1, wxEXPAND|wxALL, 5 );


	bSizerRDivider->Add( bLowerRDivider, 1, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );


	this->SetSizer( bSizerRDivider );
	this->Layout();
	bSizerRDivider->Fit( this );

	// Connect Events
	m_rbModeRatio->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnModeChange ), NULL, this );
	m_rbModeVoltage->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnModeChange ), NULL, this );
	m_e3->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnESeriesSelection ), NULL, this );
	m_e6->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnESeriesSelection ), NULL, this );
	m_e12->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnESeriesSelection ), NULL, this );
	m_e24->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnESeriesSelection ), NULL, this );
	m_e48->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnESeriesSelection ), NULL, this );
	m_e96->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnESeriesSelection ), NULL, this );
	m_e192->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnESeriesSelection ), NULL, this );
	m_btnCalculate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnCalculate ), NULL, this );
}

PANEL_R_DIVIDER_BASE::~PANEL_R_DIVIDER_BASE()
{
	// Disconnect Events
	m_rbModeRatio->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnModeChange ), NULL, this );
	m_rbModeVoltage->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnModeChange ), NULL, this );
	m_e3->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnESeriesSelection ), NULL, this );
	m_e6->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnESeriesSelection ), NULL, this );
	m_e12->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnESeriesSelection ), NULL, this );
	m_e24->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnESeriesSelection ), NULL, this );
	m_e48->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnESeriesSelection ), NULL, this );
	m_e96->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnESeriesSelection ), NULL, this );
	m_e192->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnESeriesSelection ), NULL, this );
	m_btnCalculate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_R_DIVIDER_BASE::OnCalculate ), NULL, this );

}
