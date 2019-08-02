///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 10 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_board_statistics_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_BOARD_STATISTICS_BASE::DIALOG_BOARD_STATISTICS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainBoxSizer;
	bMainBoxSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizerContents;
	fgSizerContents = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizerContents->AddGrowableCol( 0 );
	fgSizerContents->AddGrowableCol( 1 );
	fgSizerContents->AddGrowableRow( 0 );
	fgSizerContents->AddGrowableRow( 1 );
	fgSizerContents->SetFlexibleDirection( wxBOTH );
	fgSizerContents->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	wxBoxSizer* bSizerComponents;
	bSizerComponents = new wxBoxSizer( wxVERTICAL );

	wxStaticText* componentsLabel;
	componentsLabel = new wxStaticText( this, wxID_ANY, _("Components"), wxDefaultPosition, wxDefaultSize, 0 );
	componentsLabel->Wrap( -1 );
	bSizerComponents->Add( componentsLabel, 0, wxALL, 5 );

	m_gridComponents = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );

	// Grid
	m_gridComponents->CreateGrid( 5, 4 );
	m_gridComponents->EnableEditing( false );
	m_gridComponents->EnableGridLines( false );
	m_gridComponents->EnableDragGridSize( false );
	m_gridComponents->SetMargins( 0, 0 );

	// Columns
	m_gridComponents->EnableDragColMove( false );
	m_gridComponents->EnableDragColSize( true );
	m_gridComponents->SetColLabelSize( 0 );
	m_gridComponents->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_gridComponents->EnableDragRowSize( true );
	m_gridComponents->SetRowLabelSize( 0 );
	m_gridComponents->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance
	m_gridComponents->SetLabelFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	// Cell Defaults
	m_gridComponents->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_TOP );
	m_gridComponents->SetMaxSize( wxSize( -1,300 ) );

	bSizerComponents->Add( m_gridComponents, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizerContents->Add( bSizerComponents, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerPads;
	bSizerPads = new wxBoxSizer( wxVERTICAL );

	wxStaticText* padsLabel;
	padsLabel = new wxStaticText( this, wxID_ANY, _("Pads"), wxDefaultPosition, wxDefaultSize, 0 );
	padsLabel->Wrap( -1 );
	bSizerPads->Add( padsLabel, 0, wxALL, 5 );

	m_gridPads = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );

	// Grid
	m_gridPads->CreateGrid( 5, 2 );
	m_gridPads->EnableEditing( false );
	m_gridPads->EnableGridLines( false );
	m_gridPads->EnableDragGridSize( false );
	m_gridPads->SetMargins( 0, 0 );

	// Columns
	m_gridPads->EnableDragColMove( false );
	m_gridPads->EnableDragColSize( true );
	m_gridPads->SetColLabelSize( 0 );
	m_gridPads->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_gridPads->EnableDragRowSize( true );
	m_gridPads->SetRowLabelSize( 0 );
	m_gridPads->SetRowLabelAlignment( wxALIGN_RIGHT, wxALIGN_CENTER );

	// Label Appearance
	m_gridPads->SetLabelFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	// Cell Defaults
	m_gridPads->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_TOP );
	m_gridPads->SetMaxSize( wxSize( -1,300 ) );

	bSizerPads->Add( m_gridPads, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	fgSizerContents->Add( bSizerPads, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerBrdSize;
	bSizerBrdSize = new wxBoxSizer( wxVERTICAL );

	wxStaticText* boardLabel;
	boardLabel = new wxStaticText( this, wxID_ANY, _("Board Size"), wxDefaultPosition, wxDefaultSize, 0 );
	boardLabel->Wrap( -1 );
	bSizerBrdSize->Add( boardLabel, 0, wxALL, 5 );

	m_gridBoard = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );

	// Grid
	m_gridBoard->CreateGrid( 3, 2 );
	m_gridBoard->EnableEditing( false );
	m_gridBoard->EnableGridLines( false );
	m_gridBoard->EnableDragGridSize( false );
	m_gridBoard->SetMargins( 0, 0 );

	// Columns
	m_gridBoard->EnableDragColMove( false );
	m_gridBoard->EnableDragColSize( true );
	m_gridBoard->SetColLabelSize( 0 );
	m_gridBoard->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_gridBoard->EnableDragRowSize( true );
	m_gridBoard->SetRowLabelSize( 0 );
	m_gridBoard->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance
	m_gridBoard->SetLabelFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	// Cell Defaults
	m_gridBoard->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_TOP );
	m_gridBoard->SetMaxSize( wxSize( -1,300 ) );

	bSizerBrdSize->Add( m_gridBoard, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizerContents->Add( bSizerBrdSize, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerVias;
	bSizerVias = new wxBoxSizer( wxVERTICAL );

	viasLabel = new wxStaticText( this, wxID_ANY, _("Vias"), wxDefaultPosition, wxDefaultSize, 0 );
	viasLabel->Wrap( -1 );
	bSizerVias->Add( viasLabel, 0, wxALL, 5 );

	m_gridVias = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );

	// Grid
	m_gridVias->CreateGrid( 4, 2 );
	m_gridVias->EnableEditing( false );
	m_gridVias->EnableGridLines( false );
	m_gridVias->EnableDragGridSize( false );
	m_gridVias->SetMargins( 0, 0 );

	// Columns
	m_gridVias->EnableDragColMove( false );
	m_gridVias->EnableDragColSize( true );
	m_gridVias->SetColLabelSize( 0 );
	m_gridVias->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_gridVias->EnableDragRowSize( true );
	m_gridVias->SetRowLabelSize( 0 );
	m_gridVias->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance
	m_gridVias->SetLabelFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	// Cell Defaults
	m_gridVias->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_TOP );
	m_gridVias->SetMaxSize( wxSize( -1,300 ) );

	bSizerVias->Add( m_gridVias, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizerContents->Add( bSizerVias, 1, wxEXPAND, 5 );


	bMainBoxSizer->Add( fgSizerContents, 1, wxEXPAND|wxLEFT|wxRIGHT, 10 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainBoxSizer->Add( m_staticline1, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	wxGridSizer* gOptionsSizer;
	gOptionsSizer = new wxGridSizer( 0, 2, 0, 0 );

	m_checkBoxSubtractHoles = new wxCheckBox( this, wxID_ANY, _("Subtract holes from board area"), wxDefaultPosition, wxDefaultSize, 0 );
	gOptionsSizer->Add( m_checkBoxSubtractHoles, 0, wxALL|wxEXPAND, 5 );

	m_checkBoxExcludeComponentsNoPins = new wxCheckBox( this, wxID_ANY, _("Exclude components with no pins"), wxDefaultPosition, wxDefaultSize, 0 );
	gOptionsSizer->Add( m_checkBoxExcludeComponentsNoPins, 0, wxALL|wxEXPAND, 5 );


	bMainBoxSizer->Add( gOptionsSizer, 0, wxEXPAND|wxLEFT|wxRIGHT, 10 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainBoxSizer->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );


	bSizerBottom->Add( 0, 0, 1, wxEXPAND, 5 );

	m_buttonSaveReport = new wxButton( this, wxID_ANY, _("Generate Report File"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_buttonSaveReport, 0, wxALL, 5 );

	m_sdbControlSizer = new wxStdDialogButtonSizer();
	m_sdbControlSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbControlSizer->AddButton( m_sdbControlSizerCancel );
	m_sdbControlSizer->Realize();

	bSizerBottom->Add( m_sdbControlSizer, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxTOP, 5 );


	bMainBoxSizer->Add( bSizerBottom, 0, wxEXPAND, 5 );


	this->SetSizer( bMainBoxSizer );
	this->Layout();
	bMainBoxSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_checkBoxSubtractHoles->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::checkboxClicked ), NULL, this );
	m_checkBoxExcludeComponentsNoPins->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::checkboxClicked ), NULL, this );
	m_buttonSaveReport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::saveReportClicked ), NULL, this );
}

DIALOG_BOARD_STATISTICS_BASE::~DIALOG_BOARD_STATISTICS_BASE()
{
	// Disconnect Events
	m_checkBoxSubtractHoles->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::checkboxClicked ), NULL, this );
	m_checkBoxExcludeComponentsNoPins->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::checkboxClicked ), NULL, this );
	m_buttonSaveReport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::saveReportClicked ), NULL, this );

}
