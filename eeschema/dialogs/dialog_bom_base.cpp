///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

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

	m_staticTextGeneratorTitle = new wxStaticText( this, wxID_ANY, _("BOM generator scripts:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGeneratorTitle->Wrap( -1 );
	bLeftSizer->Add( m_staticTextGeneratorTitle, 0, wxTOP, 5 );

	m_lbGenerators = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_lbGenerators->SetMinSize( wxSize( 250,-1 ) );

	bLeftSizer->Add( m_lbGenerators, 1, wxEXPAND, 5 );


	bUpperSizer->Add( bLeftSizer, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bNameSizer;
	bNameSizer = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextName = new wxStaticText( this, wxID_ANY, _("Generator nickname:"), wxDefaultPosition, wxDefaultSize, 0 );
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

	wxBoxSizer* bGeneratorButtons;
	bGeneratorButtons = new wxBoxSizer( wxHORIZONTAL );

	m_buttonAddGenerator = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_buttonAddGenerator->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	m_buttonAddGenerator->SetToolTip( _("Add a new BOM generator and its command line to the list") );

	bGeneratorButtons->Add( m_buttonAddGenerator, 0, wxRIGHT|wxLEFT, 5 );

	m_buttonEdit = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_buttonEdit->SetToolTip( _("Edit the script file in the text editor") );

	bGeneratorButtons->Add( m_buttonEdit, 0, wxRIGHT, 5 );


	bGeneratorButtons->Add( 20, 0, 0, 0, 5 );

	m_buttonDelGenerator = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_buttonDelGenerator->SetToolTip( _("Remove the current generator script from list") );

	bGeneratorButtons->Add( m_buttonDelGenerator, 0, wxRIGHT, 5 );


	bMainSizer->Add( bGeneratorButtons, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bbottomSizer;
	bbottomSizer = new wxBoxSizer( wxVERTICAL );

	m_staticTextCmd = new wxStaticText( this, wxID_ANY, _("Command line running the generator:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCmd->Wrap( -1 );
	bbottomSizer->Add( m_staticTextCmd, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_textCtrlCommand = new wxTextCtrl( this, ID_CMDLINE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlCommand->SetMinSize( wxSize( 380,-1 ) );

	bbottomSizer->Add( m_textCtrlCommand, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkBoxShowConsole = new wxCheckBox( this, wxID_ANY, _("Show console window"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxShowConsole->SetToolTip( _("By default, command line runs with hidden console window and output is redirected to the info display.\nSet this option to show the window of the running command.") );

	bbottomSizer->Add( m_checkBoxShowConsole, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bbottomSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );

	m_buttonReset = new wxButton( this, wxID_ANY, _("Reset to Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonReset->SetToolTip( _("Reset the list of BOM generator scripts to the default settings") );

	bSizer8->Add( m_buttonReset, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );


	bSizer8->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizerHelp = new wxButton( this, wxID_HELP );
	m_sdbSizer->AddButton( m_sdbSizerHelp );
	m_sdbSizer->Realize();

	bSizer8->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	bMainSizer->Add( bSizer8, 0, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_BOM_BASE::OnIdle ) );
	m_lbGenerators->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_BOM_BASE::OnGeneratorSelected ), NULL, this );
	m_textCtrlName->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BOM_BASE::OnNameEdited ), NULL, this );
	m_buttonAddGenerator->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnAddGenerator ), NULL, this );
	m_buttonEdit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnEditGenerator ), NULL, this );
	m_buttonDelGenerator->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnRemoveGenerator ), NULL, this );
	m_textCtrlCommand->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BOM_BASE::OnCommandLineEdited ), NULL, this );
	m_checkBoxShowConsole->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnShowConsoleChanged ), NULL, this );
	m_sdbSizerHelp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnHelp ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnRunGenerator ), NULL, this );
}

DIALOG_BOM_BASE::~DIALOG_BOM_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_BOM_BASE::OnIdle ) );
	m_lbGenerators->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_BOM_BASE::OnGeneratorSelected ), NULL, this );
	m_textCtrlName->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BOM_BASE::OnNameEdited ), NULL, this );
	m_buttonAddGenerator->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnAddGenerator ), NULL, this );
	m_buttonEdit->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnEditGenerator ), NULL, this );
	m_buttonDelGenerator->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnRemoveGenerator ), NULL, this );
	m_textCtrlCommand->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BOM_BASE::OnCommandLineEdited ), NULL, this );
	m_checkBoxShowConsole->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnShowConsoleChanged ), NULL, this );
	m_sdbSizerHelp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnHelp ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_BASE::OnRunGenerator ), NULL, this );

}
