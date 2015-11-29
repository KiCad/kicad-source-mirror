///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_env_var_config_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_ENV_VAR_CONFIG_BASE::DIALOG_ENV_VAR_CONFIG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bleftSizer;
	bleftSizer = new wxBoxSizer( wxVERTICAL );
	
	m_grid = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_grid->CreateGrid( 0, 2 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( true );
	m_grid->SetMargins( 0, 0 );
	
	// Columns
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelSize( 30 );
	m_grid->SetColLabelValue( 0, _("Name") );
	m_grid->SetColLabelValue( 1, _("Path") );
	m_grid->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_grid->EnableDragRowSize( true );
	m_grid->SetRowLabelSize( 40 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bleftSizer->Add( m_grid, 1, wxALL|wxEXPAND, 5 );
	
	
	bupperSizer->Add( bleftSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* brightSizer;
	brightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonAdd = new wxButton( this, wxID_ANY, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonAdd->SetToolTip( _("Add a new entry to the table.") );
	
	brightSizer->Add( m_buttonAdd, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonDelete = new wxButton( this, wxID_ANY, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonDelete->SetToolTip( _("Remove the selectect entry from the table.") );
	
	brightSizer->Add( m_buttonDelete, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	
	bupperSizer->Add( brightSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	mainSizer->Add( bupperSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizerHelp = new wxButton( this, wxID_HELP );
	m_sdbSizer->AddButton( m_sdbSizerHelp );
	m_sdbSizer->Realize();
	
	mainSizer->Add( m_sdbSizer, 0, wxALL|wxALIGN_RIGHT, 5 );
	
	
	this->SetSizer( mainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_buttonAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ENV_VAR_CONFIG_BASE::OnAddRow ), NULL, this );
	m_buttonDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ENV_VAR_CONFIG_BASE::OnDeleteSelectedRows ), NULL, this );
	m_sdbSizerHelp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ENV_VAR_CONFIG_BASE::OnHelpRequest ), NULL, this );
}

DIALOG_ENV_VAR_CONFIG_BASE::~DIALOG_ENV_VAR_CONFIG_BASE()
{
	// Disconnect Events
	m_buttonAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ENV_VAR_CONFIG_BASE::OnAddRow ), NULL, this );
	m_buttonDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ENV_VAR_CONFIG_BASE::OnDeleteSelectedRows ), NULL, this );
	m_sdbSizerHelp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ENV_VAR_CONFIG_BASE::OnHelpRequest ), NULL, this );
	
}
