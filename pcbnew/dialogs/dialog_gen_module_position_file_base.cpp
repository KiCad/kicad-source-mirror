///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_gen_module_position_file_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GEN_MODULE_POSITION_BASE::DIALOG_GEN_MODULE_POSITION_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	m_MainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bDirSizer;
	bDirSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextDir = new wxStaticText( this, wxID_ANY, _("Output directory:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDir->Wrap( -1 );
	bDirSizer->Add( m_staticTextDir, 0, wxEXPAND|wxTOP|wxLEFT, 5 );
	
	wxBoxSizer* bSizerdirBrowse;
	bSizerdirBrowse = new wxBoxSizer( wxHORIZONTAL );
	
	m_outputDirectoryName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_outputDirectoryName->SetToolTip( _("Target directory for plot files. Can be absolute or relative to the board file location.") );
	m_outputDirectoryName->SetMinSize( wxSize( 350,-1 ) );
	
	bSizerdirBrowse->Add( m_outputDirectoryName, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_browseButton = new wxButton( this, wxID_ANY, _("Browse..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerdirBrowse->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );
	
	
	bDirSizer->Add( bSizerdirBrowse, 1, wxEXPAND, 5 );
	
	
	bUpperSizer->Add( bDirSizer, 1, 0, 5 );
	
	
	m_MainSizer->Add( bUpperSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerOptions;
	bSizerOptions = new wxBoxSizer( wxHORIZONTAL );
	
	wxString m_radioBoxUnitsChoices[] = { _("Inches"), _("mm") };
	int m_radioBoxUnitsNChoices = sizeof( m_radioBoxUnitsChoices ) / sizeof( wxString );
	m_radioBoxUnits = new wxRadioBox( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, m_radioBoxUnitsNChoices, m_radioBoxUnitsChoices, 1, wxRA_SPECIFY_COLS );
	m_radioBoxUnits->SetSelection( 0 );
	bSizerOptions->Add( m_radioBoxUnits, 1, wxALL, 5 );
	
	wxString m_radioBoxFilesCountChoices[] = { _("One file per side"), _("One file for board") };
	int m_radioBoxFilesCountNChoices = sizeof( m_radioBoxFilesCountChoices ) / sizeof( wxString );
	m_radioBoxFilesCount = new wxRadioBox( this, wxID_ANY, _("Files:"), wxDefaultPosition, wxDefaultSize, m_radioBoxFilesCountNChoices, m_radioBoxFilesCountChoices, 1, wxRA_SPECIFY_COLS );
	m_radioBoxFilesCount->SetSelection( 0 );
	m_radioBoxFilesCount->SetToolTip( _("Creates 2 files: one for each board side or\nCreates only one file containing all footprints to place\n") );
	
	bSizerOptions->Add( m_radioBoxFilesCount, 1, wxALL, 5 );
	
	wxString m_radioBoxForceSmdChoices[] = { _("With INSERT attribute set"), _("Force INSERT attribute for all SMD footprints") };
	int m_radioBoxForceSmdNChoices = sizeof( m_radioBoxForceSmdChoices ) / sizeof( wxString );
	m_radioBoxForceSmd = new wxRadioBox( this, wxID_ANY, _("Footprints Selection:"), wxDefaultPosition, wxDefaultSize, m_radioBoxForceSmdNChoices, m_radioBoxForceSmdChoices, 1, wxRA_SPECIFY_COLS );
	m_radioBoxForceSmd->SetSelection( 0 );
	m_radioBoxForceSmd->SetToolTip( _("Only footprints with option INSERT are listed in placement file.\nThis option can force this option for all footprints having only SMD pads.\nWarning: this options will modify the board.") );
	
	bSizerOptions->Add( m_radioBoxForceSmd, 0, wxALL, 5 );
	
	
	m_MainSizer->Add( bSizerOptions, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	wxStaticBoxSizer* sbSizerMsg;
	sbSizerMsg = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Messages:") ), wxVERTICAL );
	
	m_messagesBox = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxTE_MULTILINE|wxTE_READONLY );
	m_messagesBox->SetMinSize( wxSize( -1,150 ) );
	
	sbSizerMsg->Add( m_messagesBox, 1, wxEXPAND, 5 );
	
	
	m_MainSizer->Add( sbSizerMsg, 1, wxEXPAND, 5 );
	
	m_sdbSizerButtons = new wxStdDialogButtonSizer();
	m_sdbSizerButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsOK );
	m_sdbSizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsCancel );
	m_sdbSizerButtons->Realize();
	
	m_MainSizer->Add( m_sdbSizerButtons, 0, wxEXPAND|wxALIGN_RIGHT|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	
	this->SetSizer( m_MainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GEN_MODULE_POSITION_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
	m_sdbSizerButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GEN_MODULE_POSITION_BASE::OnOKButton ), NULL, this );
}

DIALOG_GEN_MODULE_POSITION_BASE::~DIALOG_GEN_MODULE_POSITION_BASE()
{
	// Disconnect Events
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GEN_MODULE_POSITION_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
	m_sdbSizerButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GEN_MODULE_POSITION_BASE::OnOKButton ), NULL, this );
	
}
