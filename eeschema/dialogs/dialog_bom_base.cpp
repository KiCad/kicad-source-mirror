///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_bom_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_BOM_BASE::DIALOG_BOM_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextPluginTitle = new wxStaticText( this, wxID_ANY, _("BOM plugins:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPluginTitle->Wrap( -1 );
	bLeftSizer->Add( m_staticTextPluginTitle, 0, wxTOP, 5 );
	
	m_lbPlugins = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	m_lbPlugins->SetMinSize( wxSize( 250,-1 ) );
	
	bLeftSizer->Add( m_lbPlugins, 1, wxEXPAND, 5 );
	
	
	bUpperSizer->Add( bLeftSizer, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bNameSizer;
	bNameSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticTextName = new wxStaticText( this, wxID_ANY, _("Plugin nickname:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextName->Wrap( -1 );
	bNameSizer->Add( m_staticTextName, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlName = new wxTextCtrl( this, IN_NAMELINE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bNameSizer->Add( m_textCtrlName, 1, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	
	bRightSizer->Add( bNameSizer, 0, wxEXPAND|wxTOP|wxLEFT, 6 );
	
	m_Messages = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_Messages->SetMinSize( wxSize( 300,200 ) );
	
	bRightSizer->Add( m_Messages, 1, wxEXPAND|wxLEFT, 5 );
	
	
	bUpperSizer->Add( bRightSizer, 2, wxEXPAND|wxRIGHT|wxTOP, 10 );
	
	
	bMainSizer->Add( bUpperSizer, 2, wxEXPAND|wxALL, 5 );
	
	wxBoxSizer* bPluginButtons;
	bPluginButtons = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonAddPlugin = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 30,30 ), wxBU_AUTODRAW );
	m_buttonAddPlugin->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	m_buttonAddPlugin->SetToolTip( _("Add a new plugin and its command line to the list") );
	
	bPluginButtons->Add( m_buttonAddPlugin, 0, wxRIGHT|wxLEFT, 5 );
	
	m_buttonEdit = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 30,30 ), wxBU_AUTODRAW );
	m_buttonEdit->SetToolTip( _("Edit the plugin file in the text editor") );
	
	bPluginButtons->Add( m_buttonEdit, 0, wxRIGHT, 5 );
	
	
	bPluginButtons->Add( 0, 0, 0, wxRIGHT|wxLEFT, 5 );
	
	m_buttonDelPlugin = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 30,30 ), wxBU_AUTODRAW );
	m_buttonDelPlugin->SetToolTip( _("Remove the current plugin from list") );
	
	bPluginButtons->Add( m_buttonDelPlugin, 0, wxRIGHT, 5 );
	
	
	bMainSizer->Add( bPluginButtons, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );
	
	wxBoxSizer* bbottomSizer;
	bbottomSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextCmd = new wxStaticText( this, wxID_ANY, _("Command line:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCmd->Wrap( -1 );
	bbottomSizer->Add( m_staticTextCmd, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlCommand = new wxTextCtrl( this, ID_CMDLINE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlCommand->SetMinSize( wxSize( 380,-1 ) );
	
	bbottomSizer->Add( m_textCtrlCommand, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_checkBoxShowConsole = new wxCheckBox( this, wxID_ANY, _("Show console window"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxShowConsole->SetValue(true); 
	m_checkBoxShowConsole->SetToolTip( _("By default, command line runs with hidden console window and output is redirected to \"Plugin info\" field.\nSet this option to show the window of the running command.") );
	
	bbottomSizer->Add( m_checkBoxShowConsole, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	
	bMainSizer->Add( bbottomSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );
	
	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1Help = new wxButton( this, wxID_HELP );
	m_sdbSizer1->AddButton( m_sdbSizer1Help );
	m_sdbSizer1->Realize();
	
	bMainSizer->Add( m_sdbSizer1, 0, wxEXPAND|wxALL, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_BOM_BASE::OnIdle ) );
	m_lbPlugins->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_BOM_BASE::OnPluginSelected ), NULL, this );
	m_textCtrlName->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BOM_BASE::OnNameEdited ), NULL, this );
	m_buttonAddPlugin->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnAddPlugin ), NULL, this );
	m_buttonEdit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnEditPlugin ), NULL, this );
	m_buttonDelPlugin->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnRemovePlugin ), NULL, this );
	m_textCtrlCommand->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BOM_BASE::OnCommandLineEdited ), NULL, this );
	m_checkBoxShowConsole->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnShowConsoleChanged ), NULL, this );
	m_sdbSizer1Help->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnHelp ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnRunPlugin ), NULL, this );
}

DIALOG_BOM_BASE::~DIALOG_BOM_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_BOM_BASE::OnIdle ) );
	m_lbPlugins->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_BOM_BASE::OnPluginSelected ), NULL, this );
	m_textCtrlName->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BOM_BASE::OnNameEdited ), NULL, this );
	m_buttonAddPlugin->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnAddPlugin ), NULL, this );
	m_buttonEdit->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnEditPlugin ), NULL, this );
	m_buttonDelPlugin->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnRemovePlugin ), NULL, this );
	m_textCtrlCommand->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BOM_BASE::OnCommandLineEdited ), NULL, this );
	m_checkBoxShowConsole->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnShowConsoleChanged ), NULL, this );
	m_sdbSizer1Help->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnHelp ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnRunPlugin ), NULL, this );
	
}
