///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "dialog_footprint_associations_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FOOTPRINT_ASSOCIATIONS_BASE::DIALOG_FOOTPRINT_ASSOCIATIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainBoxSizer;
	bMainBoxSizer = new wxBoxSizer( wxVERTICAL );

	m_libraryAssociationLabel = new wxStaticText( this, wxID_ANY, _("Library Association"), wxDefaultPosition, wxDefaultSize, 0 );
	m_libraryAssociationLabel->Wrap( -1 );
	bMainBoxSizer->Add( m_libraryAssociationLabel, 0, wxTOP|wxRIGHT|wxLEFT, 10 );


	bMainBoxSizer->Add( 0, 5, 0, wxEXPAND, 5 );

	m_gridLibrary = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );

	// Grid
	m_gridLibrary->CreateGrid( 2, 3 );
	m_gridLibrary->EnableEditing( false );
	m_gridLibrary->EnableGridLines( false );
	m_gridLibrary->EnableDragGridSize( false );
	m_gridLibrary->SetMargins( 0, 0 );

	// Columns
	m_gridLibrary->SetColSize( 0, 100 );
	m_gridLibrary->SetColSize( 1, 280 );
	m_gridLibrary->SetColSize( 2, 360 );
	m_gridLibrary->EnableDragColMove( false );
	m_gridLibrary->EnableDragColSize( true );
	m_gridLibrary->SetColLabelSize( 0 );
	m_gridLibrary->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_gridLibrary->EnableDragRowSize( false );
	m_gridLibrary->SetRowLabelSize( 0 );
	m_gridLibrary->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_gridLibrary->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_gridLibrary->SetMaxSize( wxSize( -1,300 ) );

	bMainBoxSizer->Add( m_gridLibrary, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 10 );


	bMainBoxSizer->Add( 0, 5, 1, wxEXPAND, 5 );

	m_symbolAssociationLabel = new wxStaticText( this, wxID_ANY, _("Schematic Association"), wxDefaultPosition, wxDefaultSize, 0 );
	m_symbolAssociationLabel->Wrap( -1 );
	bMainBoxSizer->Add( m_symbolAssociationLabel, 0, wxTOP|wxRIGHT|wxLEFT, 10 );


	bMainBoxSizer->Add( 0, 5, 0, wxEXPAND, 5 );

	m_gridSymbol = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );

	// Grid
	m_gridSymbol->CreateGrid( 1, 3 );
	m_gridSymbol->EnableEditing( false );
	m_gridSymbol->EnableGridLines( false );
	m_gridSymbol->EnableDragGridSize( false );
	m_gridSymbol->SetMargins( 0, 0 );

	// Columns
	m_gridSymbol->SetColSize( 0, 100 );
	m_gridSymbol->SetColSize( 1, 280 );
	m_gridSymbol->SetColSize( 2, 360 );
	m_gridSymbol->EnableDragColMove( false );
	m_gridSymbol->EnableDragColSize( true );
	m_gridSymbol->SetColLabelSize( 0 );
	m_gridSymbol->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_gridSymbol->EnableDragRowSize( false );
	m_gridSymbol->SetRowLabelSize( 0 );
	m_gridSymbol->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_gridSymbol->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bMainBoxSizer->Add( m_gridSymbol, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );

	m_sdbControlSizer = new wxStdDialogButtonSizer();
	m_sdbControlSizerOK = new wxButton( this, wxID_OK );
	m_sdbControlSizer->AddButton( m_sdbControlSizerOK );
	m_sdbControlSizer->Realize();

	bSizerBottom->Add( m_sdbControlSizer, 1, wxBOTTOM|wxLEFT|wxRIGHT|wxTOP, 5 );


	bMainBoxSizer->Add( bSizerBottom, 0, wxEXPAND, 5 );


	this->SetSizer( bMainBoxSizer );
	this->Layout();
	bMainBoxSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FOOTPRINT_ASSOCIATIONS_BASE::windowSize ) );
	m_gridSymbol->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FOOTPRINT_ASSOCIATIONS_BASE::drillGridSize ), NULL, this );
}

DIALOG_FOOTPRINT_ASSOCIATIONS_BASE::~DIALOG_FOOTPRINT_ASSOCIATIONS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FOOTPRINT_ASSOCIATIONS_BASE::windowSize ) );
	m_gridSymbol->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FOOTPRINT_ASSOCIATIONS_BASE::drillGridSize ), NULL, this );

}
