///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jan  1 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_select_net_from_list_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SELECT_NET_FROM_LIST_BASE::DIALOG_SELECT_NET_FROM_LIST_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 400,200 ), wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextFilter = new wxStaticText( this, wxID_ANY, _("Net name filter"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFilter->Wrap( -1 );
	fgSizer1->Add( m_staticTextFilter, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlFilter = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textCtrlFilter, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_cbShowZeroPad = new wxCheckBox( this, wxID_ANY, _("Show zero pad nets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbShowZeroPad->SetValue(true); 
	fgSizer1->Add( m_cbShowZeroPad, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerMain->Add( fgSizer1, 0, wxEXPAND, 5 );
	
	m_netsListGrid = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_netsListGrid->CreateGrid( 1, 2 );
	m_netsListGrid->EnableEditing( false );
	m_netsListGrid->EnableGridLines( true );
	m_netsListGrid->EnableDragGridSize( false );
	m_netsListGrid->SetMargins( 0, 0 );
	
	// Columns
	m_netsListGrid->SetColSize( 0, 325 );
	m_netsListGrid->SetColSize( 1, 100 );
	m_netsListGrid->EnableDragColMove( false );
	m_netsListGrid->EnableDragColSize( true );
	m_netsListGrid->SetColLabelSize( 20 );
	m_netsListGrid->SetColLabelValue( 0, _("Net name") );
	m_netsListGrid->SetColLabelValue( 1, _("Number of pads") );
	m_netsListGrid->SetColLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTRE );
	
	// Rows
	m_netsListGrid->AutoSizeRows();
	m_netsListGrid->EnableDragRowSize( true );
	m_netsListGrid->SetRowLabelSize( 50 );
	m_netsListGrid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_netsListGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_netsListGrid->SetMinSize( wxSize( 485,300 ) );
	
	bSizerMain->Add( m_netsListGrid, 1, wxALL|wxEXPAND, 5 );
	
	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerMain->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_textCtrlFilter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SELECT_NET_FROM_LIST_BASE::onFilterChange ), NULL, this );
	m_cbShowZeroPad->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SELECT_NET_FROM_LIST_BASE::onFilterChange ), NULL, this );
	m_netsListGrid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_SELECT_NET_FROM_LIST_BASE::onCellClick ), NULL, this );
}

DIALOG_SELECT_NET_FROM_LIST_BASE::~DIALOG_SELECT_NET_FROM_LIST_BASE()
{
	// Disconnect Events
	m_textCtrlFilter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SELECT_NET_FROM_LIST_BASE::onFilterChange ), NULL, this );
	m_cbShowZeroPad->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SELECT_NET_FROM_LIST_BASE::onFilterChange ), NULL, this );
	m_netsListGrid->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_SELECT_NET_FROM_LIST_BASE::onCellClick ), NULL, this );
	
}
