///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-282-g1fa54006)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/unit_selector.h"

#include "panel_electrical_spacing_ipc2221_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_ELECTRICAL_SPACING_IPC2221_BASE::PANEL_ELECTRICAL_SPACING_IPC2221_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerElectricalClearance;
	bSizerElectricalClearance = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftSizerElectricalClearance;
	bLeftSizerElectricalClearance = new wxBoxSizer( wxVERTICAL );

	m_stSpacingUnit = new wxStaticText( this, wxID_ANY, _("Unit:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stSpacingUnit->Wrap( -1 );
	bLeftSizerElectricalClearance->Add( m_stSpacingUnit, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_ElectricalSpacingUnitsSelectorChoices;
	m_ElectricalSpacingUnitsSelector = new UNIT_SELECTOR_LEN( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ElectricalSpacingUnitsSelectorChoices, 0 );
	m_ElectricalSpacingUnitsSelector->SetSelection( -1 );
	m_ElectricalSpacingUnitsSelector->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_LIGHT, false, wxEmptyString ) );

	bLeftSizerElectricalClearance->Add( m_ElectricalSpacingUnitsSelector, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftSizerElectricalClearance->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 10 );

	m_stVoltage = new wxStaticText( this, wxID_ANY, _("Voltage > 500 V:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stVoltage->Wrap( -1 );
	bLeftSizerElectricalClearance->Add( m_stVoltage, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_ElectricalSpacingVoltage = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizerElectricalClearance->Add( m_ElectricalSpacingVoltage, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_buttonElectSpacingRefresh = new wxButton( this, wxID_ANY, _("Update Values"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizerElectricalClearance->Add( m_buttonElectSpacingRefresh, 0, wxEXPAND|wxALL, 5 );


	bSizerElectricalClearance->Add( bLeftSizerElectricalClearance, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_electricalSpacingSizer = new wxBoxSizer( wxVERTICAL );

	m_staticTextElectricalSpacing = new wxStaticText( this, wxID_ANY, _("Note: Values are minimal values (from IPC 2221)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextElectricalSpacing->Wrap( -1 );
	m_staticTextElectricalSpacing->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_electricalSpacingSizer->Add( m_staticTextElectricalSpacing, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_gridElectricalSpacingValues = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_gridElectricalSpacingValues->CreateGrid( 10, 7 );
	m_gridElectricalSpacingValues->EnableEditing( false );
	m_gridElectricalSpacingValues->EnableGridLines( true );
	m_gridElectricalSpacingValues->EnableDragGridSize( false );
	m_gridElectricalSpacingValues->SetMargins( 0, 0 );

	// Columns
	m_gridElectricalSpacingValues->SetColSize( 0, 100 );
	m_gridElectricalSpacingValues->SetColSize( 1, 100 );
	m_gridElectricalSpacingValues->SetColSize( 2, 100 );
	m_gridElectricalSpacingValues->SetColSize( 3, 100 );
	m_gridElectricalSpacingValues->SetColSize( 4, 100 );
	m_gridElectricalSpacingValues->SetColSize( 5, 100 );
	m_gridElectricalSpacingValues->SetColSize( 6, 100 );
	m_gridElectricalSpacingValues->EnableDragColMove( false );
	m_gridElectricalSpacingValues->EnableDragColSize( true );
	m_gridElectricalSpacingValues->SetColLabelValue( 0, _("B1") );
	m_gridElectricalSpacingValues->SetColLabelValue( 1, _("B2") );
	m_gridElectricalSpacingValues->SetColLabelValue( 2, _("B3") );
	m_gridElectricalSpacingValues->SetColLabelValue( 3, _("B4") );
	m_gridElectricalSpacingValues->SetColLabelValue( 4, _("A5") );
	m_gridElectricalSpacingValues->SetColLabelValue( 5, _("A6") );
	m_gridElectricalSpacingValues->SetColLabelValue( 6, _("A7") );
	m_gridElectricalSpacingValues->SetColLabelSize( 30 );
	m_gridElectricalSpacingValues->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_gridElectricalSpacingValues->SetRowSize( 0, 24 );
	m_gridElectricalSpacingValues->SetRowSize( 1, 24 );
	m_gridElectricalSpacingValues->SetRowSize( 2, 24 );
	m_gridElectricalSpacingValues->SetRowSize( 3, 24 );
	m_gridElectricalSpacingValues->SetRowSize( 4, 24 );
	m_gridElectricalSpacingValues->SetRowSize( 5, 24 );
	m_gridElectricalSpacingValues->SetRowSize( 6, 24 );
	m_gridElectricalSpacingValues->SetRowSize( 7, 24 );
	m_gridElectricalSpacingValues->SetRowSize( 8, 24 );
	m_gridElectricalSpacingValues->SetRowSize( 9, 24 );
	m_gridElectricalSpacingValues->EnableDragRowSize( false );
	m_gridElectricalSpacingValues->SetRowLabelValue( 0, _("0 .. 15 V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 1, _("16 .. 30 V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 2, _("31 .. 50 V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 3, _("51 .. 100 V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 4, _("101 .. 150 V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 5, _("151 .. 170 V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 6, _("171 .. 250 V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 7, _("251 .. 300 V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 8, _("301 .. 500 V") );
	m_gridElectricalSpacingValues->SetRowLabelValue( 9, _(" > 500 V") );
	m_gridElectricalSpacingValues->SetRowLabelSize( 100 );
	m_gridElectricalSpacingValues->SetRowLabelAlignment( wxALIGN_RIGHT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_gridElectricalSpacingValues->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
	m_electricalSpacingSizer->Add( m_gridElectricalSpacingValues, 0, wxALL, 5 );

	m_stHelp = new wxStaticText( this, wxID_ANY, _("*  B1 - Internal Conductors\n*  B2 - External Conductors, uncoated, sea level to 3050 m\n*  B3 - External Conductors, uncoated, over 3050 m\n*  B4 - External Conductors, with permanent polymer coating (any elevation)\n*  A5 - External Conductors, with conformal coating over assembly (any elevation)\n*  A6 - External Component lead/termination, uncoated\n*  A7 - External Component lead termination, with conformal coating (any elevation)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHelp->Wrap( -1 );
	m_electricalSpacingSizer->Add( m_stHelp, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerElectricalClearance->Add( m_electricalSpacingSizer, 1, wxEXPAND|wxLEFT, 20 );


	this->SetSizer( bSizerElectricalClearance );
	this->Layout();

	// Connect Events
	m_ElectricalSpacingUnitsSelector->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IPC2221_BASE::OnElectricalSpacingUnitsSelection ), NULL, this );
	m_buttonElectSpacingRefresh->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IPC2221_BASE::OnElectricalSpacingRefresh ), NULL, this );
}

PANEL_ELECTRICAL_SPACING_IPC2221_BASE::~PANEL_ELECTRICAL_SPACING_IPC2221_BASE()
{
	// Disconnect Events
	m_ElectricalSpacingUnitsSelector->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IPC2221_BASE::OnElectricalSpacingUnitsSelection ), NULL, this );
	m_buttonElectSpacingRefresh->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IPC2221_BASE::OnElectricalSpacingRefresh ), NULL, this );

}
