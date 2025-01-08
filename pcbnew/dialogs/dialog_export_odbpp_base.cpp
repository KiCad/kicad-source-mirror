///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

#include "dialog_export_odbpp_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXPORT_ODBPP_BASE::DIALOG_EXPORT_ODBPP_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	bSizerTop = new wxBoxSizer( wxHORIZONTAL );

	m_lblBrdFile = new wxStaticText( this, wxID_ANY, _("Output file:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblBrdFile->Wrap( -1 );
	bSizerTop->Add( m_lblBrdFile, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_outputFileName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_outputFileName->SetToolTip( _("Enter a filename if you do not want to use default file names\nCan be used only when printing the current sheet") );
	m_outputFileName->SetMinSize( wxSize( 350,-1 ) );

	bSizerTop->Add( m_outputFileName, 1, wxALL|wxEXPAND, 5 );

	m_browseButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bSizerTop->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bMainSizer->Add( bSizerTop, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	wxFlexGridSizer* fgSizer;
	fgSizer = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgSizer->AddGrowableCol( 1 );
	fgSizer->SetFlexibleDirection( wxBOTH );
	fgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lblUnits = new wxStaticText( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblUnits->Wrap( -1 );
	fgSizer->Add( m_lblUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_choiceUnitsChoices[] = { _("Millimeters"), _("Inches") };
	int m_choiceUnitsNChoices = sizeof( m_choiceUnitsChoices ) / sizeof( wxString );
	m_choiceUnits = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), m_choiceUnitsNChoices, m_choiceUnitsChoices, 0 );
	m_choiceUnits->SetSelection( 0 );
	fgSizer->Add( m_choiceUnits, 0, wxEXPAND, 5 );

	m_lblPrecision = new wxStaticText( this, wxID_ANY, _("Precision:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblPrecision->Wrap( -1 );
	m_lblPrecision->SetToolTip( _("The number of values following the decimal separator") );

	fgSizer->Add( m_lblPrecision, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_precision = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxSP_ARROW_KEYS, 2, 16, 6 );
	m_precision->SetToolTip( _("The number of values following the decimal separator") );

	fgSizer->Add( m_precision, 0, wxEXPAND, 5 );

	m_lblCompress = new wxStaticText( this, wxID_ANY, _("Compression format:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblCompress->Wrap( -1 );
	m_lblCompress->SetToolTip( _("Select the format to compress the output ODB++ files") );

	fgSizer->Add( m_lblCompress, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxString m_choiceCompressChoices[] = { _("None"), _("ZIP"), _("TGZ") };
	int m_choiceCompressNChoices = sizeof( m_choiceCompressChoices ) / sizeof( wxString );
	m_choiceCompress = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), m_choiceCompressNChoices, m_choiceCompressChoices, 0 );
	m_choiceCompress->SetSelection( 1 );
	fgSizer->Add( m_choiceCompress, 0, wxEXPAND, 5 );


	bSizer3->Add( fgSizer, 3, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );


	bSizer4->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizer3->Add( bSizer4, 2, wxEXPAND, 5 );


	bMainSizer->Add( bSizer3, 0, wxEXPAND|wxALL, 5 );

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
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_ODBPP_BASE::onBrowseClicked ), NULL, this );
	m_choiceCompress->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_EXPORT_ODBPP_BASE::onFormatChoice ), NULL, this );
	m_stdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_ODBPP_BASE::onOKClick ), NULL, this );
}

DIALOG_EXPORT_ODBPP_BASE::~DIALOG_EXPORT_ODBPP_BASE()
{
	// Disconnect Events
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_ODBPP_BASE::onBrowseClicked ), NULL, this );
	m_choiceCompress->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_EXPORT_ODBPP_BASE::onFormatChoice ), NULL, this );
	m_stdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_ODBPP_BASE::onOKClick ), NULL, this );

}
