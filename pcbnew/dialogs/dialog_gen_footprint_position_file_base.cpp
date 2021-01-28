///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
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

	wxBoxSizer* bSizerdirBrowse;
	bSizerdirBrowse = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextDir = new wxStaticText( this, wxID_ANY, _("Output directory:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDir->Wrap( -1 );
	bSizerdirBrowse->Add( m_staticTextDir, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_outputDirectoryName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_outputDirectoryName->SetToolTip( _("Target directory for plot files. Can be absolute or relative to the board file location.") );
	m_outputDirectoryName->SetMinSize( wxSize( 350,-1 ) );

	bSizerdirBrowse->Add( m_outputDirectoryName, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_browseButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerdirBrowse->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	bUpperSizer->Add( bSizerdirBrowse, 1, wxEXPAND|wxALL, 10 );


	m_MainSizer->Add( bUpperSizer, 0, wxEXPAND, 2 );

	wxBoxSizer* bSizerMiddle;
	bSizerMiddle = new wxBoxSizer( wxHORIZONTAL );

	wxString m_rbFormatChoices[] = { _("ASCII"), _("CSV"), _("Gerber (very experimental)") };
	int m_rbFormatNChoices = sizeof( m_rbFormatChoices ) / sizeof( wxString );
	m_rbFormat = new wxRadioBox( this, wxID_ANY, _("Format"), wxDefaultPosition, wxDefaultSize, m_rbFormatNChoices, m_rbFormatChoices, 1, wxRA_SPECIFY_COLS );
	m_rbFormat->SetSelection( 2 );
	bSizerMiddle->Add( m_rbFormat, 1, wxALL|wxEXPAND, 5 );

	wxString m_radioBoxUnitsChoices[] = { _("Inches"), _("Millimeters") };
	int m_radioBoxUnitsNChoices = sizeof( m_radioBoxUnitsChoices ) / sizeof( wxString );
	m_radioBoxUnits = new wxRadioBox( this, wxID_ANY, _("Units"), wxDefaultPosition, wxDefaultSize, m_radioBoxUnitsNChoices, m_radioBoxUnitsChoices, 1, wxRA_SPECIFY_COLS );
	m_radioBoxUnits->SetSelection( 0 );
	bSizerMiddle->Add( m_radioBoxUnits, 1, wxALL|wxEXPAND, 5 );

	wxString m_radioBoxFilesCountChoices[] = { _("Separate files for front, back"), _("Single file for board") };
	int m_radioBoxFilesCountNChoices = sizeof( m_radioBoxFilesCountChoices ) / sizeof( wxString );
	m_radioBoxFilesCount = new wxRadioBox( this, wxID_ANY, _("Files"), wxDefaultPosition, wxDefaultSize, m_radioBoxFilesCountNChoices, m_radioBoxFilesCountChoices, 1, wxRA_SPECIFY_COLS );
	m_radioBoxFilesCount->SetSelection( 0 );
	m_radioBoxFilesCount->SetToolTip( _("Creates 2 files: one for each board side or\nCreates only one file containing all footprints to place\n") );

	bSizerMiddle->Add( m_radioBoxFilesCount, 1, wxALL|wxEXPAND, 5 );


	m_MainSizer->Add( bSizerMiddle, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerLower;
	bSizerLower = new wxBoxSizer( wxVERTICAL );

	m_excludeTH = new wxCheckBox( this, wxID_ANY, _("Exclude all footprints with through hole pads"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLower->Add( m_excludeTH, 0, wxALL, 5 );

	m_cbIncludeBoardEdge = new wxCheckBox( this, wxID_ANY, _("Include board edge layer"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLower->Add( m_cbIncludeBoardEdge, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_messagesPanel = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_messagesPanel->SetMinSize( wxSize( 350,300 ) );

	bSizerLower->Add( m_messagesPanel, 1, wxEXPAND | wxALL, 5 );


	m_MainSizer->Add( bSizerLower, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_MainSizer->Add( m_staticline, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_MainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
	m_rbFormat->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onSelectFormat ), NULL, this );
	m_radioBoxUnits->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIUnits ), NULL, this );
	m_radioBoxFilesCount->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIFileOpt ), NULL, this );
	m_excludeTH->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIExcludeTH ), NULL, this );
	m_cbIncludeBoardEdge->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIincludeBoardEdge ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::OnGenerate ), NULL, this );
}

DIALOG_GEN_FOOTPRINT_POSITION_BASE::~DIALOG_GEN_FOOTPRINT_POSITION_BASE()
{
	// Disconnect Events
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
	m_rbFormat->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onSelectFormat ), NULL, this );
	m_radioBoxUnits->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIUnits ), NULL, this );
	m_radioBoxFilesCount->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIFileOpt ), NULL, this );
	m_excludeTH->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIExcludeTH ), NULL, this );
	m_cbIncludeBoardEdge->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::onUpdateUIincludeBoardEdge ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GEN_FOOTPRINT_POSITION_BASE::OnGenerate ), NULL, this );

}
