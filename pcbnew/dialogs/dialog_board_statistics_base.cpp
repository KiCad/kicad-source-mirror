///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-62-g497c85bd-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "dialog_board_statistics_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_BOARD_STATISTICS_BASE::DIALOG_BOARD_STATISTICS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainBoxSizer;
	bMainBoxSizer = new wxBoxSizer( wxVERTICAL );

	topNotebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_generalPanel = new wxPanel( topNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bGeneralPanelSizer;
	bGeneralPanelSizer = new wxBoxSizer( wxVERTICAL );

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

	m_componentsLabel = new wxStaticText( m_generalPanel, wxID_ANY, _("Components"), wxDefaultPosition, wxDefaultSize, 0 );
	m_componentsLabel->Wrap( -1 );
	bSizerComponents->Add( m_componentsLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerComponents->Add( 0, 2, 0, 0, 5 );

	m_gridComponents = new wxGrid( m_generalPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );

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

	// Cell Defaults
	m_gridComponents->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
	m_gridComponents->SetMaxSize( wxSize( -1,300 ) );

	bSizerComponents->Add( m_gridComponents, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizerContents->Add( bSizerComponents, 1, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );

	wxBoxSizer* bSizerPads;
	bSizerPads = new wxBoxSizer( wxVERTICAL );

	m_padsLabel = new wxStaticText( m_generalPanel, wxID_ANY, _("Pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_padsLabel->Wrap( -1 );
	bSizerPads->Add( m_padsLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerPads->Add( 0, 2, 0, 0, 5 );

	m_gridPads = new wxGrid( m_generalPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );

	// Grid
	m_gridPads->CreateGrid( 7, 2 );
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

	// Cell Defaults
	m_gridPads->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
	m_gridPads->SetMaxSize( wxSize( -1,300 ) );

	bSizerPads->Add( m_gridPads, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	fgSizerContents->Add( bSizerPads, 1, wxEXPAND|wxBOTTOM|wxLEFT, 5 );

	wxBoxSizer* bSizerBrdSize;
	bSizerBrdSize = new wxBoxSizer( wxVERTICAL );

	m_boardLabel = new wxStaticText( m_generalPanel, wxID_ANY, _("Board"), wxDefaultPosition, wxDefaultSize, 0 );
	m_boardLabel->Wrap( -1 );
	bSizerBrdSize->Add( m_boardLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerBrdSize->Add( 0, 2, 0, 0, 5 );

	m_gridBoard = new wxGrid( m_generalPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );

	// Grid
	m_gridBoard->CreateGrid( 12, 2 );
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

	// Cell Defaults
	m_gridBoard->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
	m_gridBoard->SetMaxSize( wxSize( -1,300 ) );

	bSizerBrdSize->Add( m_gridBoard, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizerContents->Add( bSizerBrdSize, 1, wxEXPAND|wxRIGHT, 5 );

	wxBoxSizer* bSizerVias;
	bSizerVias = new wxBoxSizer( wxVERTICAL );

	m_viasLabel = new wxStaticText( m_generalPanel, wxID_ANY, _("Vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viasLabel->Wrap( -1 );
	bSizerVias->Add( m_viasLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerVias->Add( 0, 2, 0, 0, 5 );

	m_gridVias = new wxGrid( m_generalPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );

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

	// Cell Defaults
	m_gridVias->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
	m_gridVias->SetMaxSize( wxSize( -1,300 ) );

	bSizerVias->Add( m_gridVias, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizerContents->Add( bSizerVias, 1, wxEXPAND|wxLEFT, 5 );


	bGeneralPanelSizer->Add( fgSizerContents, 1, wxEXPAND, 5 );

	wxGridSizer* gOptionsSizer;
	gOptionsSizer = new wxGridSizer( 0, 1, 4, 0 );

	m_checkBoxSubtractHoles = new wxCheckBox( m_generalPanel, wxID_ANY, _("Subtract holes from board area"), wxDefaultPosition, wxDefaultSize, 0 );
	gOptionsSizer->Add( m_checkBoxSubtractHoles, 0, wxEXPAND|wxRIGHT, 4 );

	m_checkBoxSubtractHolesFromCopper = new wxCheckBox( m_generalPanel, wxID_ANY, _("Subtract holes from copper areas"), wxDefaultPosition, wxDefaultSize, 0 );
	gOptionsSizer->Add( m_checkBoxSubtractHolesFromCopper, 0, wxRIGHT, 5 );

	m_checkBoxExcludeComponentsNoPins = new wxCheckBox( m_generalPanel, wxID_ANY, _("Exclude footprints with no pads"), wxDefaultPosition, wxDefaultSize, 0 );
	gOptionsSizer->Add( m_checkBoxExcludeComponentsNoPins, 0, wxEXPAND|wxRIGHT, 5 );


	bGeneralPanelSizer->Add( gOptionsSizer, 0, wxEXPAND|wxALL, 5 );


	m_generalPanel->SetSizer( bGeneralPanelSizer );
	m_generalPanel->Layout();
	bGeneralPanelSizer->Fit( m_generalPanel );
	topNotebook->AddPage( m_generalPanel, _("General"), true );
	m_drillsPanel = new wxPanel( topNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bDrillsSizer;
	bDrillsSizer = new wxBoxSizer( wxVERTICAL );

	m_gridDrills = new WX_GRID( m_drillsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );

	// Grid
	m_gridDrills->CreateGrid( 0, 8 );
	m_gridDrills->EnableEditing( false );
	m_gridDrills->EnableGridLines( true );
	m_gridDrills->EnableDragGridSize( false );
	m_gridDrills->SetMargins( 0, 0 );

	// Columns
	m_gridDrills->AutoSizeColumns();
	m_gridDrills->EnableDragColMove( true );
	m_gridDrills->EnableDragColSize( true );
	m_gridDrills->SetColLabelValue( 0, _("Count") );
	m_gridDrills->SetColLabelValue( 1, _("Shape") );
	m_gridDrills->SetColLabelValue( 2, _("X Size") );
	m_gridDrills->SetColLabelValue( 3, _("Y Size") );
	m_gridDrills->SetColLabelValue( 4, _("Plated") );
	m_gridDrills->SetColLabelValue( 5, _("Via/Pad") );
	m_gridDrills->SetColLabelValue( 6, _("Start Layer") );
	m_gridDrills->SetColLabelValue( 7, _("Stop Layer") );
	m_gridDrills->SetColLabelSize( 22 );
	m_gridDrills->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_gridDrills->EnableDragRowSize( false );
	m_gridDrills->SetRowLabelSize( 0 );
	m_gridDrills->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_gridDrills->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
	bDrillsSizer->Add( m_gridDrills, 1, wxALL|wxEXPAND, 5 );


	m_drillsPanel->SetSizer( bDrillsSizer );
	m_drillsPanel->Layout();
	bDrillsSizer->Fit( m_drillsPanel );
	topNotebook->AddPage( m_drillsPanel, _("Drill Holes"), false );

	bMainBoxSizer->Add( topNotebook, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );

	m_buttonSaveReport = new wxButton( this, wxID_ANY, _("Generate Report File..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_buttonSaveReport, 0, wxALIGN_CENTER_VERTICAL|wxALL, 15 );

	m_sdbControlSizer = new wxStdDialogButtonSizer();
	m_sdbControlSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbControlSizer->AddButton( m_sdbControlSizerCancel );
	m_sdbControlSizer->Realize();

	bSizerBottom->Add( m_sdbControlSizer, 1, wxBOTTOM|wxLEFT|wxRIGHT|wxTOP|wxALIGN_CENTER_VERTICAL, 5 );


	bMainBoxSizer->Add( bSizerBottom, 0, wxEXPAND, 5 );


	this->SetSizer( bMainBoxSizer );
	this->Layout();
	bMainBoxSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_BOARD_STATISTICS_BASE::windowSize ) );
	m_checkBoxSubtractHoles->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::checkboxClicked ), NULL, this );
	m_checkBoxSubtractHolesFromCopper->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::checkboxClicked ), NULL, this );
	m_checkBoxExcludeComponentsNoPins->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::checkboxClicked ), NULL, this );
	m_gridDrills->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_BOARD_STATISTICS_BASE::drillGridSize ), NULL, this );
	m_buttonSaveReport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::saveReportClicked ), NULL, this );
}

DIALOG_BOARD_STATISTICS_BASE::~DIALOG_BOARD_STATISTICS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_BOARD_STATISTICS_BASE::windowSize ) );
	m_checkBoxSubtractHoles->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::checkboxClicked ), NULL, this );
	m_checkBoxSubtractHolesFromCopper->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::checkboxClicked ), NULL, this );
	m_checkBoxExcludeComponentsNoPins->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::checkboxClicked ), NULL, this );
	m_gridDrills->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_BOARD_STATISTICS_BASE::drillGridSize ), NULL, this );
	m_buttonSaveReport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::saveReportClicked ), NULL, this );

}
