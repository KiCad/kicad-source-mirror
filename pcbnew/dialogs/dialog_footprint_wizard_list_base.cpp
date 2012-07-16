///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_footprint_wizard_list_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FOOTPRINT_WIZARD_LIST_BASE::DIALOG_FOOTPRINT_WIZARD_LIST_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );
	
	m_footprintWizardsGrid = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_footprintWizardsGrid->CreateGrid( 0, 3 );
	m_footprintWizardsGrid->EnableEditing( false );
	m_footprintWizardsGrid->EnableGridLines( true );
	m_footprintWizardsGrid->EnableDragGridSize( false );
	m_footprintWizardsGrid->SetMargins( 0, 0 );
	
	// Columns
	m_footprintWizardsGrid->SetColSize( 0, 80 );
	m_footprintWizardsGrid->SetColSize( 1, 80 );
	m_footprintWizardsGrid->SetColSize( 2, 325 );
	m_footprintWizardsGrid->EnableDragColMove( false );
	m_footprintWizardsGrid->EnableDragColSize( true );
	m_footprintWizardsGrid->SetColLabelSize( 20 );
	m_footprintWizardsGrid->SetColLabelValue( 0, _("Preview") );
	m_footprintWizardsGrid->SetColLabelValue( 1, _("Name") );
	m_footprintWizardsGrid->SetColLabelValue( 2, _("Description") );
	m_footprintWizardsGrid->SetColLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTRE );
	
	// Rows
	m_footprintWizardsGrid->AutoSizeRows();
	m_footprintWizardsGrid->EnableDragRowSize( true );
	m_footprintWizardsGrid->SetRowLabelSize( 1 );
	m_footprintWizardsGrid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_footprintWizardsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_footprintWizardsGrid->SetMinSize( wxSize( -1,120 ) );
	
	bSizer4->Add( m_footprintWizardsGrid, 1, wxALL, 5 );
	
	m_btOpen = new wxButton( this, wxID_ANY, _("Open"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_btOpen, 0, wxALIGN_CENTER|wxALL, 5 );
	
	this->SetSizer( bSizer4 );
	this->Layout();
	bSizer4->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_footprintWizardsGrid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_FOOTPRINT_WIZARD_LIST_BASE::OnCellWizardClick ), NULL, this );
	m_btOpen->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_WIZARD_LIST_BASE::OnOpenButtonClick ), NULL, this );
}

DIALOG_FOOTPRINT_WIZARD_LIST_BASE::~DIALOG_FOOTPRINT_WIZARD_LIST_BASE()
{
	// Disconnect Events
	m_footprintWizardsGrid->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_FOOTPRINT_WIZARD_LIST_BASE::OnCellWizardClick ), NULL, this );
	m_btOpen->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_WIZARD_LIST_BASE::OnOpenButtonClick ), NULL, this );
	
}
