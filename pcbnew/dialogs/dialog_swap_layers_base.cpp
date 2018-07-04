///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "dialog_swap_layers_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SWAP_LAYERS_BASE::DIALOG_SWAP_LAYERS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bMarginsSizer;
	bMarginsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_grid->CreateGrid( 2, 2 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );
	
	// Columns
	m_grid->SetColSize( 0, 125 );
	m_grid->SetColSize( 1, 125 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( false );
	m_grid->SetColLabelSize( 22 );
	m_grid->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_grid->EnableDragRowSize( false );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_grid->SetMinSize( wxSize( 250,150 ) );
	
	bMarginsSizer->Add( m_grid, 1, wxEXPAND|wxALL, 5 );
	
	
	bMainSizer->Add( bMarginsSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bMainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_grid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SWAP_LAYERS_BASE::OnSize ), NULL, this );
}

DIALOG_SWAP_LAYERS_BASE::~DIALOG_SWAP_LAYERS_BASE()
{
	// Disconnect Events
	m_grid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SWAP_LAYERS_BASE::OnSize ), NULL, this );
	
}
