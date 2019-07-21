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

	wxGridBagSizer* gbContentsSizer;
	gbContentsSizer = new wxGridBagSizer( 10, 20 );
	gbContentsSizer->SetFlexibleDirection( wxBOTH );
	gbContentsSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	wxStaticText* componentsLabel;
	componentsLabel = new wxStaticText( this, wxID_ANY, _("Components"), wxDefaultPosition, wxDefaultSize, 0 );
	componentsLabel->Wrap( -1 );
	bSizer2->Add( componentsLabel, 0, wxALL, 5 );

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
	bSizer2->Add( m_gridComponents, 0, wxALIGN_CENTER|wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	gbContentsSizer->Add( bSizer2, wxGBPosition( 0, 0 ), wxGBSpan( 2, 1 ), wxEXPAND, 5 );

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

	bSizerPads->Add( m_gridPads, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	gbContentsSizer->Add( bSizerPads, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

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

	bSizerBrdSize->Add( m_gridBoard, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	gbContentsSizer->Add( bSizerBrdSize, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );


	gbContentsSizer->AddGrowableCol( 0 );
	gbContentsSizer->AddGrowableCol( 1 );
	gbContentsSizer->AddGrowableRow( 0 );
	gbContentsSizer->AddGrowableRow( 1 );

	bMainBoxSizer->Add( gbContentsSizer, 1, wxEXPAND|wxALL, 10 );

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

	m_sdbControlSizer = new wxStdDialogButtonSizer();
	m_sdbControlSizerOK = new wxButton( this, wxID_OK );
	m_sdbControlSizer->AddButton( m_sdbControlSizerOK );
	m_sdbControlSizer->Realize();

	bMainBoxSizer->Add( m_sdbControlSizer, 0, wxALIGN_RIGHT|wxBOTTOM|wxLEFT|wxRIGHT|wxTOP, 5 );


	this->SetSizer( bMainBoxSizer );
	this->Layout();
	bMainBoxSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_checkBoxSubtractHoles->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::checkboxClicked ), NULL, this );
	m_checkBoxExcludeComponentsNoPins->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::checkboxClicked ), NULL, this );
}

DIALOG_BOARD_STATISTICS_BASE::~DIALOG_BOARD_STATISTICS_BASE()
{
	// Disconnect Events
	m_checkBoxSubtractHoles->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::checkboxClicked ), NULL, this );
	m_checkBoxExcludeComponentsNoPins->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::checkboxClicked ), NULL, this );

}
