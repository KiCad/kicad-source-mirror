///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

#include "dialog_gendrill_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GENDRILL_BASE::DIALOG_GENDRILL_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxHORIZONTAL );

	staticTextOutputDir = new wxStaticText( this, wxID_ANY, _("Output folder:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticTextOutputDir->Wrap( -1 );
	bupperSizer->Add( staticTextOutputDir, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 10 );

	m_outputDirectoryName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bupperSizer->Add( m_outputDirectoryName, 1, wxALIGN_CENTER_VERTICAL|wxLEFT, 0 );

	m_browseButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bupperSizer->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bMainSizer->Add( bupperSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bmiddlerSizer;
	bmiddlerSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftCol;
	bLeftCol = new wxBoxSizer( wxVERTICAL );

	m_formatLabel = new wxStaticText( this, wxID_ANY, _("Format"), wxDefaultPosition, wxDefaultSize, 0 );
	m_formatLabel->Wrap( -1 );
	bLeftCol->Add( m_formatLabel, 0, wxRIGHT|wxLEFT, 13 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftCol->Add( m_staticline1, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerMargins;
	bSizerMargins = new wxBoxSizer( wxVERTICAL );

	m_rbExcellon = new wxRadioButton( this, wxID_ANY, _("Excellon"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbExcellon->SetValue( true );
	bSizerMargins->Add( m_rbExcellon, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizerExcellonOptions;
	fgSizerExcellonOptions = new wxFlexGridSizer( 4, 1, 3, 0 );
	fgSizerExcellonOptions->SetFlexibleDirection( wxBOTH );
	fgSizerExcellonOptions->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_Check_Mirror = new wxCheckBox( this, wxID_ANY, _("Mirror Y axis"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Check_Mirror->SetToolTip( _("Not recommended.\nUsed mostly by users who make the boards themselves.") );

	fgSizerExcellonOptions->Add( m_Check_Mirror, 0, wxLEFT, 5 );

	m_Check_Minimal = new wxCheckBox( this, wxID_ANY, _("Minimal header"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Check_Minimal->SetToolTip( _("Not recommended.\nOnly use it for board houses which do not accept fully featured headers.") );

	fgSizerExcellonOptions->Add( m_Check_Minimal, 0, wxRIGHT|wxLEFT, 5 );

	m_Check_Merge_PTH_NPTH = new wxCheckBox( this, wxID_ANY, _("PTH and NPTH in single file"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Check_Merge_PTH_NPTH->SetToolTip( _("Not recommended.\nOnly use for board houses which ask for merged PTH and NPTH into a single file.") );

	fgSizerExcellonOptions->Add( m_Check_Merge_PTH_NPTH, 0, wxLEFT, 5 );

	m_altDrillMode = new wxCheckBox( this, wxID_ANY, _("Use alternate drill mode for oval holes"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerExcellonOptions->Add( m_altDrillMode, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerMargins->Add( fgSizerExcellonOptions, 0, wxEXPAND|wxLEFT, 20 );


	bSizerMargins->Add( 0, 3, 1, wxEXPAND, 5 );

	m_rbGerberX2 = new wxRadioButton( this, wxID_ANY, _("Gerber X2"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerMargins->Add( m_rbGerberX2, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxFlexGridSizer* fgSizerGerberX2Options;
	fgSizerGerberX2Options = new wxFlexGridSizer( 4, 1, 3, 0 );
	fgSizerGerberX2Options->SetFlexibleDirection( wxBOTH );
	fgSizerGerberX2Options->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_generateTentingLayers = new wxCheckBox( this, wxID_ANY, _("Generate tenting layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_generateTentingLayers->Enable( false );

	fgSizerGerberX2Options->Add( m_generateTentingLayers, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizerMargins->Add( fgSizerGerberX2Options, 1, wxEXPAND|wxLEFT, 20 );


	bSizerMargins->Add( 0, 6, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	m_cbGenerateMap = new wxCheckBox( this, wxID_ANY, _("Generate map:"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_cbGenerateMap, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	wxString m_choiceDrillMapChoices[] = { _("Postscript"), _("Gerber X2"), _("DXF"), _("SVG"), _("PDF") };
	int m_choiceDrillMapNChoices = sizeof( m_choiceDrillMapChoices ) / sizeof( wxString );
	m_choiceDrillMap = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceDrillMapNChoices, m_choiceDrillMapChoices, 0 );
	m_choiceDrillMap->SetSelection( 1 );
	bSizer9->Add( m_choiceDrillMap, 1, wxALL, 5 );


	bSizerMargins->Add( bSizer9, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bLeftCol->Add( bSizerMargins, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bmiddlerSizer->Add( bLeftCol, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bmiddlerSizer->Add( 15, 0, 0, 0, 5 );

	wxBoxSizer* bRightCol;
	bRightCol = new wxBoxSizer( wxVERTICAL );

	m_optionsLabel = new wxStaticText( this, wxID_ANY, _("Options"), wxDefaultPosition, wxDefaultSize, 0 );
	m_optionsLabel->Wrap( -1 );
	bRightCol->Add( m_optionsLabel, 0, wxRIGHT|wxLEFT, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bRightCol->Add( m_staticline2, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_originLabel = new wxStaticText( this, wxID_ANY, _("Origin:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_originLabel->Wrap( -1 );
	fgSizer1->Add( m_originLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_originChoices[] = { _("Absolute"), _("Drill/place file origin") };
	int m_originNChoices = sizeof( m_originChoices ) / sizeof( wxString );
	m_origin = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_originNChoices, m_originChoices, 0 );
	m_origin->SetSelection( 0 );
	fgSizer1->Add( m_origin, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_unitsLabel = new wxStaticText( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitsLabel->Wrap( -1 );
	fgSizer1->Add( m_unitsLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_unitsChoices[] = { _("Millimeters"), _("Inches") };
	int m_unitsNChoices = sizeof( m_unitsChoices ) / sizeof( wxString );
	m_units = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_unitsNChoices, m_unitsChoices, 0 );
	m_units->SetSelection( 0 );
	fgSizer1->Add( m_units, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_zerosLabel = new wxStaticText( this, wxID_ANY, _("Zeros:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_zerosLabel->Wrap( -1 );
	fgSizer1->Add( m_zerosLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_zerosChoices[] = { _("Decimal format (recommended)"), _("Suppress leading zeros"), _("Suppress trailing zeros"), _("Keep zeros") };
	int m_zerosNChoices = sizeof( m_zerosChoices ) / sizeof( wxString );
	m_zeros = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_zerosNChoices, m_zerosChoices, 0 );
	m_zeros->SetSelection( 0 );
	fgSizer1->Add( m_zeros, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizer1->Add( 0, 8, 1, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_precisionLabel = new wxStaticText( this, wxID_ANY, _("Precision:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_precisionLabel->Wrap( -1 );
	fgSizer1->Add( m_precisionLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_staticTextPrecision = new wxStaticText( this, wxID_ANY, _("Precision"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPrecision->Wrap( -1 );
	fgSizer1->Add( m_staticTextPrecision, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );


	bRightCol->Add( fgSizer1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bmiddlerSizer->Add( bRightCol, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bmiddlerSizer, 0, wxEXPAND|wxTOP, 8 );

	bMsgSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Messages") ), wxVERTICAL );

	m_messagesBox = new wxTextCtrl( bMsgSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_messagesBox->SetMinSize( wxSize( -1,90 ) );

	bMsgSizer->Add( m_messagesBox, 1, wxEXPAND, 5 );


	bMainSizer->Add( bMsgSizer, 1, wxALL|wxEXPAND, 10 );

	m_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_buttonReport = new wxButton( this, wxID_ANY, _("Generate Report File..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonsSizer->Add( m_buttonReport, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_buttonsSizer->Add( m_sdbSizer, 1, wxEXPAND, 5 );


	bMainSizer->Add( m_buttonsSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_GENDRILL_BASE::onCloseDlg ) );
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onOutputDirectoryBrowseClicked ), NULL, this );
	m_rbExcellon->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onFileFormatSelection ), NULL, this );
	m_rbGerberX2->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onFileFormatSelection ), NULL, this );
	m_units->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onSelDrillUnitsSelected ), NULL, this );
	m_zeros->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onSelZerosFmtSelected ), NULL, this );
	m_buttonReport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onGenReportFile ), NULL, this );
}

DIALOG_GENDRILL_BASE::~DIALOG_GENDRILL_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_GENDRILL_BASE::onCloseDlg ) );
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onOutputDirectoryBrowseClicked ), NULL, this );
	m_rbExcellon->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onFileFormatSelection ), NULL, this );
	m_rbGerberX2->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onFileFormatSelection ), NULL, this );
	m_units->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onSelDrillUnitsSelected ), NULL, this );
	m_zeros->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onSelZerosFmtSelected ), NULL, this );
	m_buttonReport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onGenReportFile ), NULL, this );

}
