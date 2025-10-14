///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_html_report_panel.h"

#include "dialog_export_2581_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXPORT_2581_BASE::DIALOG_EXPORT_2581_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	bSizerTop = new wxBoxSizer( wxHORIZONTAL );

	m_lblBrdFile = new wxStaticText( this, wxID_ANY, _("File:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblBrdFile->Wrap( -1 );
	bSizerTop->Add( m_lblBrdFile, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_outputFileName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_outputFileName->SetToolTip( _("Enter a filename if you do not want to use default file names\nCan be used only when printing the current sheet") );
	m_outputFileName->SetMinSize( wxSize( 350,-1 ) );

	bSizerTop->Add( m_outputFileName, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_browseButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bSizerTop->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL, 10 );


	bMainSizer->Add( bSizerTop, 0, wxEXPAND|wxALL, 10 );

	wxBoxSizer* bSizerMiddle;
	bSizerMiddle = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeftCol;
	bSizerLeftCol = new wxBoxSizer( wxVERTICAL );

	m_fileFormatLabel = new wxStaticText( this, wxID_ANY, _("File Format"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fileFormatLabel->Wrap( -1 );
	bSizerLeftCol->Add( m_fileFormatLabel, 0, wxTOP|wxRIGHT|wxLEFT, 8 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerLeftCol->Add( m_staticline1, 0, wxEXPAND|wxBOTTOM, 5 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 5, 5 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lblUnits = new wxStaticText( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblUnits->Wrap( -1 );
	gbSizer1->Add( m_lblUnits, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxString m_choiceUnitsChoices[] = { _("Millimeters"), _("Inches") };
	int m_choiceUnitsNChoices = sizeof( m_choiceUnitsChoices ) / sizeof( wxString );
	m_choiceUnits = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnitsNChoices, m_choiceUnitsChoices, 0 );
	m_choiceUnits->SetSelection( 0 );
	gbSizer1->Add( m_choiceUnits, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 10 );

	m_lblPrecision = new wxStaticText( this, wxID_ANY, _("Precision:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblPrecision->Wrap( -1 );
	m_lblPrecision->SetToolTip( _("The number of values following the decimal separator") );

	gbSizer1->Add( m_lblPrecision, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 15 );

	m_precision = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 4, 10, 6 );
	m_precision->SetToolTip( _("The number of values following the decimal separator") );

	gbSizer1->Add( m_precision, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 10 );

	m_lblVersion = new wxStaticText( this, wxID_ANY, _("Version:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblVersion->Wrap( -1 );
	gbSizer1->Add( m_lblVersion, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxString m_versionChoiceChoices[] = { _("B"), _("C") };
	int m_versionChoiceNChoices = sizeof( m_versionChoiceChoices ) / sizeof( wxString );
	m_versionChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_versionChoiceNChoices, m_versionChoiceChoices, 0 );
	m_versionChoice->SetSelection( 1 );
	gbSizer1->Add( m_versionChoice, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 10 );

	m_cbCompress = new wxCheckBox( this, wxID_ANY, _("Compress output"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbCompress->SetToolTip( _("Compress output into 'zip' file") );

	gbSizer1->Add( m_cbCompress, wxGBPosition( 3, 0 ), wxGBSpan( 1, 2 ), wxTOP, 5 );


	gbSizer1->AddGrowableCol( 1 );

	bSizerLeftCol->Add( gbSizer1, 1, wxEXPAND|wxTOP|wxRIGHT, 5 );


	bSizerMiddle->Add( bSizerLeftCol, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	bSizerMiddle->Add( 10, 0, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerRightCol;
	bSizerRightCol = new wxBoxSizer( wxVERTICAL );

	m_columnsLabel = new wxStaticText( this, wxID_ANY, _("BOM Columns"), wxDefaultPosition, wxDefaultSize, 0 );
	m_columnsLabel->Wrap( -1 );
	bSizerRightCol->Add( m_columnsLabel, 0, wxTOP|wxRIGHT|wxLEFT, 8 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerRightCol->Add( m_staticline2, 0, wxEXPAND|wxBOTTOM, 5 );

	wxFlexGridSizer* fgSizer4;
	fgSizer4 = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgSizer4->AddGrowableCol( 1 );
	fgSizer4->SetFlexibleDirection( wxBOTH );
	fgSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lblOEM = new wxStaticText( this, wxID_ANY, _("Internal ID:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblOEM->Wrap( -1 );
	m_lblOEM->SetToolTip( _("Part ID number used internally during design.\nThis number must be unique to each part.") );

	fgSizer4->Add( m_lblOEM, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxString m_oemRefChoices[] = { _("Generate unique") };
	int m_oemRefNChoices = sizeof( m_oemRefChoices ) / sizeof( wxString );
	m_oemRef = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_oemRefNChoices, m_oemRefChoices, 0 );
	m_oemRef->SetSelection( 0 );
	m_oemRef->SetToolTip( _("Part ID number used internally during design.\nThis number must be unique to each part.") );

	fgSizer4->Add( m_oemRef, 0, wxEXPAND|wxRIGHT, 5 );

	m_staticText6 = new wxStaticText( this, wxID_ANY, _("Manufacturer P/N:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	m_staticText6->SetToolTip( _("Column containing the manufacturer part number") );

	fgSizer4->Add( m_staticText6, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxString m_choiceMPNChoices[] = { _("Omit") };
	int m_choiceMPNNChoices = sizeof( m_choiceMPNChoices ) / sizeof( wxString );
	m_choiceMPN = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceMPNNChoices, m_choiceMPNChoices, 0 );
	m_choiceMPN->SetSelection( 0 );
	m_choiceMPN->SetToolTip( _("Column containing the manufacturer part number") );

	fgSizer4->Add( m_choiceMPN, 0, wxEXPAND|wxRIGHT, 5 );

	m_staticText7 = new wxStaticText( this, wxID_ANY, _("Manufacturer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	fgSizer4->Add( m_staticText7, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxString m_choiceMfgChoices[] = { _("N/A") };
	int m_choiceMfgNChoices = sizeof( m_choiceMfgChoices ) / sizeof( wxString );
	m_choiceMfg = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceMfgNChoices, m_choiceMfgChoices, 0 );
	m_choiceMfg->SetSelection( 0 );
	m_choiceMfg->Enable( false );

	fgSizer4->Add( m_choiceMfg, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_staticText8 = new wxStaticText( this, wxID_ANY, _("Distributor P/N:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	fgSizer4->Add( m_staticText8, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_choiceDistPNChoices[] = { _("Omit") };
	int m_choiceDistPNNChoices = sizeof( m_choiceDistPNChoices ) / sizeof( wxString );
	m_choiceDistPN = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceDistPNNChoices, m_choiceDistPNChoices, 0 );
	m_choiceDistPN->SetSelection( 0 );
	m_choiceDistPN->SetToolTip( _("Column containing the distributor part number") );

	fgSizer4->Add( m_choiceDistPN, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Distributor:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizer4->Add( m_staticText9, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textDistributor = new wxTextCtrl( this, wxID_ANY, _("N/A"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	fgSizer4->Add( m_textDistributor, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );


	bSizerRightCol->Add( fgSizer4, 1, wxALL|wxEXPAND, 5 );


	bSizerMiddle->Add( bSizerRightCol, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	bMainSizer->Add( bSizerMiddle, 0, wxEXPAND|wxBOTTOM, 5 );

	m_messagesPanel = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_messagesPanel->SetMinSize( wxSize( -300,150 ) );

	bMainSizer->Add( m_messagesPanel, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();

	bMainSizer->Add( m_stdButtons, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_2581_BASE::onBrowseClicked ), NULL, this );
	m_cbCompress->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_2581_BASE::onCompressCheck ), NULL, this );
	m_choiceMPN->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_EXPORT_2581_BASE::onMfgPNChange ), NULL, this );
	m_choiceDistPN->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_EXPORT_2581_BASE::onDistPNChange ), NULL, this );
	m_stdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_2581_BASE::onOKClick ), NULL, this );
}

DIALOG_EXPORT_2581_BASE::~DIALOG_EXPORT_2581_BASE()
{
	// Disconnect Events
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_2581_BASE::onBrowseClicked ), NULL, this );
	m_cbCompress->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_2581_BASE::onCompressCheck ), NULL, this );
	m_choiceMPN->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_EXPORT_2581_BASE::onMfgPNChange ), NULL, this );
	m_choiceDistPN->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_EXPORT_2581_BASE::onDistPNChange ), NULL, this );
	m_stdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_2581_BASE::onOKClick ), NULL, this );

}
