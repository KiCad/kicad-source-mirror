///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov 22 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dlg_3d_pathconfig_base.h"

///////////////////////////////////////////////////////////////////////////

DLG_3D_PATH_CONFIG_BASE::DLG_3D_PATH_CONFIG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );
	
	m_EnvVars = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_EnvVars->CreateGrid( 1, 2 );
	m_EnvVars->EnableEditing( true );
	m_EnvVars->EnableGridLines( true );
	m_EnvVars->EnableDragGridSize( false );
	m_EnvVars->SetMargins( 0, 0 );
	
	// Columns
	m_EnvVars->SetColSize( 0, 150 );
	m_EnvVars->SetColSize( 1, 300 );
	m_EnvVars->EnableDragColMove( false );
	m_EnvVars->EnableDragColSize( true );
	m_EnvVars->SetColLabelSize( 30 );
	m_EnvVars->SetColLabelValue( 0, _("Name") );
	m_EnvVars->SetColLabelValue( 1, _("Path") );
	m_EnvVars->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_EnvVars->EnableDragRowSize( true );
	m_EnvVars->SetRowLabelSize( 80 );
	m_EnvVars->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_EnvVars->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_EnvVars->SetMinSize( wxSize( 250,100 ) );
	
	bSizer5->Add( m_EnvVars, 1, wxALL|wxEXPAND, 5 );
	
	m_btnEnvCfg = new wxButton( this, wxID_ANY, _("Configure Environment Variables"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_btnEnvCfg, 0, wxALIGN_CENTER|wxALL, 5 );
	
	
	bSizerMain->Add( bSizer5, 0, wxEXPAND, 5 );
	
	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bSizerGrid;
	bSizerGrid = new wxBoxSizer( wxHORIZONTAL );
	
	m_Aliases = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_Aliases->CreateGrid( 1, 3 );
	m_Aliases->EnableEditing( true );
	m_Aliases->EnableGridLines( true );
	m_Aliases->EnableDragGridSize( false );
	m_Aliases->SetMargins( 0, 0 );
	
	// Columns
	m_Aliases->SetColSize( 0, 80 );
	m_Aliases->SetColSize( 1, 300 );
	m_Aliases->SetColSize( 2, 120 );
	m_Aliases->EnableDragColMove( false );
	m_Aliases->EnableDragColSize( true );
	m_Aliases->SetColLabelSize( 30 );
	m_Aliases->SetColLabelValue( 0, _("Alias") );
	m_Aliases->SetColLabelValue( 1, _("Path") );
	m_Aliases->SetColLabelValue( 2, _("Description") );
	m_Aliases->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_Aliases->AutoSizeRows();
	m_Aliases->EnableDragRowSize( false );
	m_Aliases->SetRowLabelSize( 80 );
	m_Aliases->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_Aliases->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_Aliases->SetMinSize( wxSize( -1,150 ) );
	
	bSizerGrid->Add( m_Aliases, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizerGrid, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxHORIZONTAL );
	
	m_btnAddAlias = new wxButton( this, wxID_ANY, _("Add Alias"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_btnAddAlias, 0, wxALL, 5 );
	
	m_btnDelAlias = new wxButton( this, wxID_ANY, _("Remove Alias"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_btnDelAlias, 0, wxALL, 5 );
	
	m_btnMoveUp = new wxButton( this, wxID_ANY, _("Move Up"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_btnMoveUp, 0, wxALL, 5 );
	
	m_btnMoveDown = new wxButton( this, wxID_ANY, _("Move Down"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_btnMoveDown, 0, wxALL, 5 );
	
	
	bSizerMain->Add( bSizerButtons, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer2 = new wxStdDialogButtonSizer();
	m_sdbSizer2OK = new wxButton( this, wxID_OK );
	m_sdbSizer2->AddButton( m_sdbSizer2OK );
	m_sdbSizer2Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer2->AddButton( m_sdbSizer2Cancel );
	m_sdbSizer2Help = new wxButton( this, wxID_HELP );
	m_sdbSizer2->AddButton( m_sdbSizer2Help );
	m_sdbSizer2->Realize();
	
	bSizerMain->Add( m_sdbSizer2, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_btnEnvCfg->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnConfigEnvVar ), NULL, this );
	m_btnAddAlias->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnAddAlias ), NULL, this );
	m_btnDelAlias->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnDelAlias ), NULL, this );
	m_btnMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnAliasMoveUp ), NULL, this );
	m_btnMoveDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnAliasMoveDown ), NULL, this );
	m_sdbSizer2Help->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnHelp ), NULL, this );
}

DLG_3D_PATH_CONFIG_BASE::~DLG_3D_PATH_CONFIG_BASE()
{
	// Disconnect Events
	m_btnEnvCfg->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnConfigEnvVar ), NULL, this );
	m_btnAddAlias->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnAddAlias ), NULL, this );
	m_btnDelAlias->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnDelAlias ), NULL, this );
	m_btnMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnAliasMoveUp ), NULL, this );
	m_btnMoveDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnAliasMoveDown ), NULL, this );
	m_sdbSizer2Help->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnHelp ), NULL, this );
	
}
