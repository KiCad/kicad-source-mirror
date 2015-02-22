///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_footprint_wizard_list_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FOOTPRINT_WIZARD_LIST_BASE::DIALOG_FOOTPRINT_WIZARD_LIST_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 400,200 ), wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
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
	m_footprintWizardsGrid->SetMinSize( wxSize( 485,120 ) );
	
	bSizerMain->Add( m_footprintWizardsGrid, 1, wxALL|wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
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
	m_footprintWizardsGrid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_FOOTPRINT_WIZARD_LIST_BASE::OnCellWizardClick ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_WIZARD_LIST_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_WIZARD_LIST_BASE::OnOpenButtonClick ), NULL, this );
}

DIALOG_FOOTPRINT_WIZARD_LIST_BASE::~DIALOG_FOOTPRINT_WIZARD_LIST_BASE()
{
	// Disconnect Events
	m_footprintWizardsGrid->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_FOOTPRINT_WIZARD_LIST_BASE::OnCellWizardClick ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_WIZARD_LIST_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_WIZARD_LIST_BASE::OnOpenButtonClick ), NULL, this );
	
}
