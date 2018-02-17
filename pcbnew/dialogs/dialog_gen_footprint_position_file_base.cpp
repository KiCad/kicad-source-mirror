///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx_html_report_panel.h"

#include "dialog_gen_footprint_position_file_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GEN_FOOTPRINT_POSITION_BASE::DIALOG_GEN_FOOTPRINT_POSITION_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	m_MainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bDirSizer;
	bDirSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextDir = new wxStaticText( this, wxID_ANY, _("Output directory:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDir->Wrap( -1 );
	bDirSizer->Add( m_staticTextDir, 0, wxEXPAND, 10 );
	
	wxBoxSizer* bSizerdirBrowse;
	bSizerdirBrowse = new wxBoxSizer( wxHORIZONTAL );
	
	m_outputDirectoryName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_outputDirectoryName->SetToolTip( _("Target directory for plot files. Can be absolute or relative to the board file location.") );
	m_outputDirectoryName->SetMinSize( wxSize( 350,-1 ) );
	
	bSizerdirBrowse->Add( m_outputDirectoryName, 1, wxBOTTOM|wxEXPAND|wxTOP, 4 );
	
	m_browseButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	bSizerdirBrowse->Add( m_browseButton, 0, wxRIGHT, 1 );
	
	
	bDirSizer->Add( bSizerdirBrowse, 1, wxEXPAND, 5 );
	
	
	bUpperSizer->Add( bDirSizer, 1, wxALL, 10 );
	
	
	m_MainSizer->Add( bUpperSizer, 0, wxEXPAND|wxLEFT|wxRIGHT, 2 );
	
	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	wxBoxSizer* bOptionsSizer;
	bOptionsSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxString m_rbFormatChoices[] = { _("ASCII"), _("CSV") };
	int m_rbFormatNChoices = sizeof( m_rbFormatChoices ) / sizeof( wxString );
	m_rbFormat = new wxRadioBox( this, wxID_ANY, _("Format:"), wxDefaultPosition, wxDefaultSize, m_rbFormatNChoices, m_rbFormatChoices, 1, wxRA_SPECIFY_COLS );
	m_rbFormat->SetSelection( 0 );
	m_rbFormat->SetMinSize( wxSize( 90,-1 ) );
	
	bOptionsSizer->Add( m_rbFormat, 0, wxALL, 5 );
	
	wxString m_radioBoxUnitsChoices[] = { _("Inches"), _("mm") };
	int m_radioBoxUnitsNChoices = sizeof( m_radioBoxUnitsChoices ) / sizeof( wxString );
	m_radioBoxUnits = new wxRadioBox( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, m_radioBoxUnitsNChoices, m_radioBoxUnitsChoices, 1, wxRA_SPECIFY_COLS );
	m_radioBoxUnits->SetSelection( 0 );
	m_radioBoxUnits->SetMinSize( wxSize( 90,-1 ) );
	
	bOptionsSizer->Add( m_radioBoxUnits, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_radioBoxFilesCountChoices[] = { _("One file per side"), _("Single file for board") };
	int m_radioBoxFilesCountNChoices = sizeof( m_radioBoxFilesCountChoices ) / sizeof( wxString );
	m_radioBoxFilesCount = new wxRadioBox( this, wxID_ANY, _("Files:"), wxDefaultPosition, wxDefaultSize, m_radioBoxFilesCountNChoices, m_radioBoxFilesCountChoices, 1, wxRA_SPECIFY_COLS );
	m_radioBoxFilesCount->SetSelection( 1 );
	m_radioBoxFilesCount->SetToolTip( _("Creates 2 files: one for each board side or\nCreates only one file containing all footprints to place\n") );
	
	bOptionsSizer->Add( m_radioBoxFilesCount, 1, wxALL, 5 );
	
	
	gbSizer1->Add( bOptionsSizer, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );
	
	wxString m_radioBoxForceSmdChoices[] = { _("With INSERT attribute set"), _("Force INSERT attribute for all SMD footprints") };
	int m_radioBoxForceSmdNChoices = sizeof( m_radioBoxForceSmdChoices ) / sizeof( wxString );
	m_radioBoxForceSmd = new wxRadioBox( this, wxID_ANY, _("Footprints Selection:"), wxDefaultPosition, wxDefaultSize, m_radioBoxForceSmdNChoices, m_radioBoxForceSmdChoices, 1, wxRA_SPECIFY_COLS );
	m_radioBoxForceSmd->SetSelection( 0 );
	m_radioBoxForceSmd->SetToolTip( _("Only footprints with option INSERT are listed in placement file.\nThis option can force this option for all footprints having only SMD pads.\nWarning: this options will modify the board.") );
	
	gbSizer1->Add( m_radioBoxForceSmd, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );
	
	
	gbSizer1->Add( 0, 0, wxGBPosition( 0, 1 ), wxGBSpan( 2, 1 ), wxEXPAND, 5 );
	
	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxVERTICAL );
	
	bButtonsSizer->SetMinSize( wxSize( 140,-1 ) ); 
	m_generateButton = new wxButton( this, wxID_ANY, _("Generate File"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_generateButton, 0, wxALL|wxEXPAND, 5 );
	
	m_closeButton = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_closeButton, 0, wxALL|wxEXPAND|wxTOP, 5 );
	
	
	gbSizer1->Add( bButtonsSizer, wxGBPosition( 0, 2 ), wxGBSpan( 2, 1 ), wxEXPAND|wxLEFT|wxTOP, 25 );
	
	
	gbSizer1->AddGrowableCol( 1 );
	
	m_MainSizer->Add( gbSizer1, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	m_messagesPanel = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_messagesPanel->SetMinSize( wxSize( 300,150 ) );
	
	bSizer7->Add( m_messagesPanel, 1, wxEXPAND | wxALL, 5 );
	
	
	m_MainSizer->Add( bSizer7, 1, wxEXPAND, 5 );
	
	
	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
	m_generateButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::OnGenerate ), NULL, this );
}

DIALOG_GEN_FOOTPRINT_POSITION_BASE::~DIALOG_GEN_FOOTPRINT_POSITION_BASE()
{
	// Disconnect Events
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
	m_generateButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::OnGenerate ), NULL, this );
	
}
