///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_config_equfiles_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_CONFIG_EQUFILES_BASE::DIALOG_CONFIG_EQUFILES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbEquivChoiceSizer;
	sbEquivChoiceSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Footprint/Component equ files (.equ files)") ), wxHORIZONTAL );
	
	wxBoxSizer* bSizerFlist;
	bSizerFlist = new wxBoxSizer( wxVERTICAL );
	
	m_ListEquiv = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_EXTENDED|wxLB_HSCROLL|wxLB_NEEDED_SB|wxLB_SINGLE ); 
	m_ListEquiv->SetMinSize( wxSize( 350,-1 ) );
	
	bSizerFlist->Add( m_ListEquiv, 1, wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	sbEquivChoiceSizer->Add( bSizerFlist, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxVERTICAL );
	
	m_buttonAddEqu = new wxButton( this, ID_ADD_EQU, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonAddEqu, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_buttonRemoveEqu = new wxButton( this, ID_REMOVE_EQU, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonRemoveEqu->SetToolTip( _("Unload the selected library") );
	
	bSizerButtons->Add( m_buttonRemoveEqu, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_buttonMoveUp = new wxButton( this, ID_EQU_UP, _("Move Up"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonMoveUp, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_buttonMoveDown = new wxButton( this, ID_EQU_DOWN, _("Move Down"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonMoveDown, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_buttonEdit = new wxButton( this, wxID_ANY, _("Edit Equ File"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonEdit, 0, wxALL|wxEXPAND, 5 );
	
	
	sbEquivChoiceSizer->Add( bSizerButtons, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bMainSizer->Add( sbEquivChoiceSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerLower;
	bSizerLower = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerEnvVar;
	bSizerEnvVar = new wxBoxSizer( wxVERTICAL );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Available environment variables for relative paths:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bSizerEnvVar->Add( m_staticText2, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_gridEnvVars = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_gridEnvVars->CreateGrid( 2, 2 );
	m_gridEnvVars->EnableEditing( true );
	m_gridEnvVars->EnableGridLines( true );
	m_gridEnvVars->EnableDragGridSize( false );
	m_gridEnvVars->SetMargins( 0, 0 );
	
	// Columns
	m_gridEnvVars->EnableDragColMove( false );
	m_gridEnvVars->EnableDragColSize( true );
	m_gridEnvVars->SetColLabelSize( 25 );
	m_gridEnvVars->SetColLabelValue( 0, _("Name") );
	m_gridEnvVars->SetColLabelValue( 1, _("Value") );
	m_gridEnvVars->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_gridEnvVars->AutoSizeRows();
	m_gridEnvVars->EnableDragRowSize( true );
	m_gridEnvVars->SetRowLabelSize( 30 );
	m_gridEnvVars->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_gridEnvVars->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizerEnvVar->Add( m_gridEnvVars, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerLower->Add( bSizerEnvVar, 1, wxEXPAND, 5 );
	
	wxString m_rbPathOptionChoiceChoices[] = { _("Absolute path"), _("Relative path") };
	int m_rbPathOptionChoiceNChoices = sizeof( m_rbPathOptionChoiceChoices ) / sizeof( wxString );
	m_rbPathOptionChoice = new wxRadioBox( this, wxID_ANY, _("Path option:"), wxDefaultPosition, wxDefaultSize, m_rbPathOptionChoiceNChoices, m_rbPathOptionChoiceChoices, 1, wxRA_SPECIFY_COLS );
	m_rbPathOptionChoice->SetSelection( 1 );
	bSizerLower->Add( m_rbPathOptionChoice, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bMainSizer->Add( bSizerLower, 0, wxEXPAND, 5 );
	
	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline2, 0, wxEXPAND|wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bMainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnCloseWindow ) );
	m_buttonAddEqu->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnAddFiles ), NULL, this );
	m_buttonRemoveEqu->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnRemoveFiles ), NULL, this );
	m_buttonMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnButtonMoveUp ), NULL, this );
	m_buttonMoveDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnButtonMoveDown ), NULL, this );
	m_buttonEdit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnEditEquFile ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnOkClick ), NULL, this );
}

DIALOG_CONFIG_EQUFILES_BASE::~DIALOG_CONFIG_EQUFILES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnCloseWindow ) );
	m_buttonAddEqu->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnAddFiles ), NULL, this );
	m_buttonRemoveEqu->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnRemoveFiles ), NULL, this );
	m_buttonMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnButtonMoveUp ), NULL, this );
	m_buttonMoveDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnButtonMoveDown ), NULL, this );
	m_buttonEdit->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnEditEquFile ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnOkClick ), NULL, this );
	
}
