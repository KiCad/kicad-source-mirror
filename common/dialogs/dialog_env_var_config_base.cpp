///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug  4 2017)
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
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );
	
	m_pathList = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_HRULES|wxLC_REPORT|wxLC_VRULES );
	bSizer7->Add( m_pathList, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );
	
	m_addPathButton = new wxButton( this, ID_BUTTON_ADD_PATH, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	m_addPathButton->SetToolTip( _("Add path prefix") );
	
	bSizer6->Add( m_addPathButton, 0, wxALL, 5 );
	
	m_editPathButton = new wxButton( this, ID_BUTTON_EDIT_PATH, _("Edit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_editPathButton->SetToolTip( _("Edit selected path prefix") );
	
	bSizer6->Add( m_editPathButton, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_deletePathButton = new wxButton( this, ID_BUTTON_DELETE_PATH, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
	m_deletePathButton->SetToolTip( _("Remove selected path prefix") );
	
	bSizer6->Add( m_deletePathButton, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	
	bSizer6->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizer7->Add( bSizer6, 0, wxEXPAND, 5 );
	
	
	mainSizer->Add( bSizer7, 1, wxEXPAND, 5 );
	
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
	
	mainSizer->Add( m_sdbSizer, 0, wxBOTTOM|wxALIGN_RIGHT, 5 );
	
	
	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_pathList->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( DIALOG_ENV_VAR_CONFIG_BASE::OnPathActivated ), NULL, this );
	m_pathList->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_ENV_VAR_CONFIG_BASE::OnPathSelected ), NULL, this );
	m_addPathButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ENV_VAR_CONFIG_BASE::OnAddButton ), NULL, this );
	m_editPathButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ENV_VAR_CONFIG_BASE::OnEditButton ), NULL, this );
	m_deletePathButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ENV_VAR_CONFIG_BASE::OnRemoveButton ), NULL, this );
	m_sdbSizerHelp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ENV_VAR_CONFIG_BASE::OnHelpButton ), NULL, this );
}

DIALOG_ENV_VAR_CONFIG_BASE::~DIALOG_ENV_VAR_CONFIG_BASE()
{
	// Disconnect Events
	m_pathList->Disconnect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( DIALOG_ENV_VAR_CONFIG_BASE::OnPathActivated ), NULL, this );
	m_pathList->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_ENV_VAR_CONFIG_BASE::OnPathSelected ), NULL, this );
	m_addPathButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ENV_VAR_CONFIG_BASE::OnAddButton ), NULL, this );
	m_editPathButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ENV_VAR_CONFIG_BASE::OnEditButton ), NULL, this );
	m_deletePathButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ENV_VAR_CONFIG_BASE::OnRemoveButton ), NULL, this );
	m_sdbSizerHelp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ENV_VAR_CONFIG_BASE::OnHelpButton ), NULL, this );
	
}

DIALOG_ENV_VAR_SINGLE_BASE::DIALOG_ENV_VAR_SINGLE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_envVarNameLabel = new wxStaticText( this, wxID_ANY, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_envVarNameLabel->Wrap( -1 );
	fgSizer1->Add( m_envVarNameLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_envVarName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_envVarName, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );
	
	m_buttonHelp = new wxButton( this, wxID_ANY, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_buttonHelp, 0, wxALL, 5 );
	
	m_envVarPathLabel = new wxStaticText( this, wxID_ANY, _("Value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_envVarPathLabel->Wrap( -1 );
	fgSizer1->Add( m_envVarPathLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_envVarPath = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_envVarPath->SetMinSize( wxSize( 250,-1 ) );
	
	fgSizer1->Add( m_envVarPath, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_selectPathButton = new wxButton( this, wxID_ANY, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_selectPathButton, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	
	bSizerMain->Add( fgSizer1, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerMain->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline2, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_sdbSizer2 = new wxStdDialogButtonSizer();
	m_sdbSizer2OK = new wxButton( this, wxID_OK );
	m_sdbSizer2->AddButton( m_sdbSizer2OK );
	m_sdbSizer2Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer2->AddButton( m_sdbSizer2Cancel );
	m_sdbSizer2->Realize();
	
	bSizerMain->Add( m_sdbSizer2, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxALIGN_RIGHT, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_envVarName->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_ENV_VAR_SINGLE_BASE::onVarNameChange ), NULL, this );
	m_buttonHelp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ENV_VAR_SINGLE_BASE::onHelpClick ), NULL, this );
	m_selectPathButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ENV_VAR_SINGLE_BASE::OnSelectPath ), NULL, this );
}

DIALOG_ENV_VAR_SINGLE_BASE::~DIALOG_ENV_VAR_SINGLE_BASE()
{
	// Disconnect Events
	m_envVarName->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_ENV_VAR_SINGLE_BASE::onVarNameChange ), NULL, this );
	m_buttonHelp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ENV_VAR_SINGLE_BASE::onHelpClick ), NULL, this );
	m_selectPathButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ENV_VAR_SINGLE_BASE::OnSelectPath ), NULL, this );
	
}
