///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_gendrill_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GENDRILL_BASE::DIALOG_GENDRILL_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxHORIZONTAL );

	staticTextOutputDir = new wxStaticText( this, wxID_ANY, _("Output folder:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticTextOutputDir->Wrap( -1 );
	bupperSizer->Add( staticTextOutputDir, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 10 );

	m_outputDirectoryName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bupperSizer->Add( m_outputDirectoryName, 1, wxALIGN_CENTER_VERTICAL|wxLEFT, 0 );

	m_browseButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bupperSizer->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 7 );


	bMainSizer->Add( bupperSizer, 0, wxEXPAND|wxTOP, 10 );

	wxBoxSizer* bmiddlerSizer;
	bmiddlerSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bMiddleSizer;
	bMiddleSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer6;
	sbSizer6 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Drill File Format") ), wxVERTICAL );

	m_rbExcellon = new wxRadioButton( sbSizer6->GetStaticBox(), wxID_ANY, _("Excellon"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer6->Add( m_rbExcellon, 0, wxBOTTOM|wxRIGHT, 5 );

	wxBoxSizer* bSizerExcellonOptions;
	bSizerExcellonOptions = new wxBoxSizer( wxVERTICAL );

	m_Check_Mirror = new wxCheckBox( sbSizer6->GetStaticBox(), wxID_ANY, _("Mirror Y axis"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Check_Mirror->SetToolTip( _("Not recommended.\nUsed mostly by users who make the boards themselves.") );

	bSizerExcellonOptions->Add( m_Check_Mirror, 0, wxLEFT, 5 );

	m_Check_Minimal = new wxCheckBox( sbSizer6->GetStaticBox(), wxID_ANY, _("Minimal header"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Check_Minimal->SetToolTip( _("Not recommended.\nOnly use it for board houses which do not accept fully featured headers.") );

	bSizerExcellonOptions->Add( m_Check_Minimal, 0, wxRIGHT|wxLEFT, 5 );

	m_Check_Merge_PTH_NPTH = new wxCheckBox( sbSizer6->GetStaticBox(), wxID_ANY, _("PTH and NPTH in single file"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Check_Merge_PTH_NPTH->SetToolTip( _("Not recommended.\nOnly use for board houses which ask for merged PTH and NPTH into a single file.") );

	bSizerExcellonOptions->Add( m_Check_Merge_PTH_NPTH, 0, wxLEFT, 5 );

	wxString m_radioBoxOvalHoleModeChoices[] = { _("Use route command (recommended)"), _("Use alternate drill mode") };
	int m_radioBoxOvalHoleModeNChoices = sizeof( m_radioBoxOvalHoleModeChoices ) / sizeof( wxString );
	m_radioBoxOvalHoleMode = new wxRadioBox( sbSizer6->GetStaticBox(), wxID_ANY, _("Oval Holes Drill Mode"), wxDefaultPosition, wxDefaultSize, m_radioBoxOvalHoleModeNChoices, m_radioBoxOvalHoleModeChoices, 1, wxRA_SPECIFY_COLS );
	m_radioBoxOvalHoleMode->SetSelection( 0 );
	m_radioBoxOvalHoleMode->SetToolTip( _("Oval holes frequently create problems for board houses.\n\"Use route command\" uses the usual G00 route command (recommended)\n \"Use alternate mode\" uses another drill/ route command (G85)\n(Use it only if the recommended command does not work)") );

	bSizerExcellonOptions->Add( m_radioBoxOvalHoleMode, 0, wxALL|wxEXPAND, 5 );


	sbSizer6->Add( bSizerExcellonOptions, 1, wxEXPAND|wxLEFT, 12 );

	m_rbGerberX2 = new wxRadioButton( sbSizer6->GetStaticBox(), wxID_ANY, _("Gerber X2"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer6->Add( m_rbGerberX2, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bMiddleSizer->Add( sbSizer6, 1, wxEXPAND|wxALL, 5 );

	wxString m_Choice_Drill_MapChoices[] = { _("HPGL"), _("PostScript"), _("Gerber"), _("DXF"), _("SVG"), _("PDF") };
	int m_Choice_Drill_MapNChoices = sizeof( m_Choice_Drill_MapChoices ) / sizeof( wxString );
	m_Choice_Drill_Map = new wxRadioBox( this, wxID_ANY, _("Map File Format"), wxDefaultPosition, wxDefaultSize, m_Choice_Drill_MapNChoices, m_Choice_Drill_MapChoices, 1, wxRA_SPECIFY_COLS );
	m_Choice_Drill_Map->SetSelection( 1 );
	m_Choice_Drill_Map->SetToolTip( _("Creates a drill map in PS, HPGL or other formats") );

	bMiddleSizer->Add( m_Choice_Drill_Map, 0, wxALL|wxEXPAND, 5 );


	bmiddlerSizer->Add( bMiddleSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );

	wxString m_Choice_Drill_OffsetChoices[] = { _("Absolute"), _("Drill/place file origin") };
	int m_Choice_Drill_OffsetNChoices = sizeof( m_Choice_Drill_OffsetChoices ) / sizeof( wxString );
	m_Choice_Drill_Offset = new wxRadioBox( this, wxID_ANY, _("Drill Origin"), wxDefaultPosition, wxDefaultSize, m_Choice_Drill_OffsetNChoices, m_Choice_Drill_OffsetChoices, 1, wxRA_SPECIFY_COLS );
	m_Choice_Drill_Offset->SetSelection( 0 );
	m_Choice_Drill_Offset->SetToolTip( _("Choose the coordinate origin: absolute or relative to the drill/place file origin") );

	bLeftSizer->Add( m_Choice_Drill_Offset, 0, wxALL|wxEXPAND, 5 );

	wxString m_Choice_UnitChoices[] = { _("Millimeters"), _("Inches") };
	int m_Choice_UnitNChoices = sizeof( m_Choice_UnitChoices ) / sizeof( wxString );
	m_Choice_Unit = new wxRadioBox( this, wxID_ANY, _("Drill Units"), wxDefaultPosition, wxDefaultSize, m_Choice_UnitNChoices, m_Choice_UnitChoices, 1, wxRA_SPECIFY_COLS );
	m_Choice_Unit->SetSelection( 0 );
	bLeftSizer->Add( m_Choice_Unit, 0, wxALL|wxEXPAND, 5 );

	wxString m_Choice_Zeros_FormatChoices[] = { _("Decimal format (recommended)"), _("Suppress leading zeros"), _("Suppress trailing zeros"), _("Keep zeros") };
	int m_Choice_Zeros_FormatNChoices = sizeof( m_Choice_Zeros_FormatChoices ) / sizeof( wxString );
	m_Choice_Zeros_Format = new wxRadioBox( this, wxID_ANY, _("Zeros Format"), wxDefaultPosition, wxDefaultSize, m_Choice_Zeros_FormatNChoices, m_Choice_Zeros_FormatChoices, 1, wxRA_SPECIFY_COLS );
	m_Choice_Zeros_Format->SetSelection( 0 );
	m_Choice_Zeros_Format->SetToolTip( _("Choose EXCELLON numbers notation") );

	bLeftSizer->Add( m_Choice_Zeros_Format, 0, wxALL|wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextTitle = new wxStaticText( this, wxID_ANY, _("Precision:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTitle->Wrap( -1 );
	fgSizer1->Add( m_staticTextTitle, 0, wxALL, 10 );

	m_staticTextPrecision = new wxStaticText( this, wxID_ANY, _("Precision"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPrecision->Wrap( -1 );
	fgSizer1->Add( m_staticTextPrecision, 0, wxALL, 10 );


	bLeftSizer->Add( fgSizer1, 0, wxEXPAND, 5 );


	bmiddlerSizer->Add( bLeftSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* bRightBoxSizer;
	bRightBoxSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerHoles;
	sbSizerHoles = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Hole Counts") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	staticTextPlatedPads = new wxStaticText( sbSizerHoles->GetStaticBox(), wxID_ANY, _("Plated pads:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticTextPlatedPads->Wrap( -1 );
	fgSizer2->Add( staticTextPlatedPads, 0, wxLEFT|wxRIGHT, 5 );

	m_PlatedPadsCountInfoMsg = new wxStaticText( sbSizerHoles->GetStaticBox(), wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PlatedPadsCountInfoMsg->Wrap( -1 );
	fgSizer2->Add( m_PlatedPadsCountInfoMsg, 0, wxALIGN_RIGHT|wxLEFT|wxRIGHT, 5 );

	staticTextNonPlatedPads = new wxStaticText( sbSizerHoles->GetStaticBox(), wxID_ANY, _("Non-plated pads:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticTextNonPlatedPads->Wrap( -1 );
	fgSizer2->Add( staticTextNonPlatedPads, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_NotPlatedPadsCountInfoMsg = new wxStaticText( sbSizerHoles->GetStaticBox(), wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_NotPlatedPadsCountInfoMsg->Wrap( -1 );
	fgSizer2->Add( m_NotPlatedPadsCountInfoMsg, 0, wxALIGN_RIGHT|wxLEFT|wxRIGHT|wxTOP, 5 );

	staticTextThroughVias = new wxStaticText( sbSizerHoles->GetStaticBox(), wxID_ANY, _("Through vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticTextThroughVias->Wrap( -1 );
	fgSizer2->Add( staticTextThroughVias, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_ThroughViasInfoMsg = new wxStaticText( sbSizerHoles->GetStaticBox(), wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThroughViasInfoMsg->Wrap( -1 );
	fgSizer2->Add( m_ThroughViasInfoMsg, 0, wxALIGN_RIGHT|wxLEFT|wxRIGHT|wxTOP, 5 );

	staticTextMicroVias = new wxStaticText( sbSizerHoles->GetStaticBox(), wxID_ANY, _("Micro vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticTextMicroVias->Wrap( -1 );
	fgSizer2->Add( staticTextMicroVias, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_MicroViasInfoMsg = new wxStaticText( sbSizerHoles->GetStaticBox(), wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MicroViasInfoMsg->Wrap( -1 );
	fgSizer2->Add( m_MicroViasInfoMsg, 0, wxALIGN_RIGHT|wxLEFT|wxRIGHT|wxTOP, 5 );

	staticTextBuriedVias = new wxStaticText( sbSizerHoles->GetStaticBox(), wxID_ANY, _("Buried vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticTextBuriedVias->Wrap( -1 );
	fgSizer2->Add( staticTextBuriedVias, 0, wxALL, 5 );

	m_BuriedViasInfoMsg = new wxStaticText( sbSizerHoles->GetStaticBox(), wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BuriedViasInfoMsg->Wrap( -1 );
	fgSizer2->Add( m_BuriedViasInfoMsg, 0, wxALIGN_RIGHT|wxALL, 5 );


	sbSizerHoles->Add( fgSizer2, 1, wxEXPAND, 5 );


	bRightBoxSizer->Add( sbSizerHoles, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bmiddlerSizer->Add( bRightBoxSizer, 1, wxEXPAND|wxTOP, 5 );


	bMainSizer->Add( bmiddlerSizer, 0, wxEXPAND|wxTOP, 2 );

	wxStaticBoxSizer* bMsgSizer;
	bMsgSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Messages") ), wxVERTICAL );

	m_messagesBox = new wxTextCtrl( bMsgSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_messagesBox->SetMinSize( wxSize( -1,90 ) );

	bMsgSizer->Add( m_messagesBox, 1, wxALL|wxEXPAND, 5 );


	bMainSizer->Add( bMsgSizer, 1, wxALL|wxEXPAND, 5 );

	m_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_buttonReport = new wxButton( this, wxID_ANY, _("Generate Report File..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonsSizer->Add( m_buttonReport, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerApply = new wxButton( this, wxID_APPLY );
	m_sdbSizer->AddButton( m_sdbSizerApply );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_buttonsSizer->Add( m_sdbSizer, 1, wxEXPAND, 5 );


	bMainSizer->Add( m_buttonsSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_GENDRILL_BASE::onCloseDlg ) );
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
	m_rbExcellon->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onFileFormatSelection ), NULL, this );
	m_rbGerberX2->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onFileFormatSelection ), NULL, this );
	m_Choice_Unit->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnSelDrillUnitsSelected ), NULL, this );
	m_Choice_Zeros_Format->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnSelZerosFmtSelected ), NULL, this );
	m_buttonReport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnGenReportFile ), NULL, this );
	m_sdbSizerApply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnGenMapFile ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onQuitDlg ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnGenDrillFile ), NULL, this );
}

DIALOG_GENDRILL_BASE::~DIALOG_GENDRILL_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_GENDRILL_BASE::onCloseDlg ) );
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
	m_rbExcellon->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onFileFormatSelection ), NULL, this );
	m_rbGerberX2->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onFileFormatSelection ), NULL, this );
	m_Choice_Unit->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnSelDrillUnitsSelected ), NULL, this );
	m_Choice_Zeros_Format->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnSelZerosFmtSelected ), NULL, this );
	m_buttonReport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnGenReportFile ), NULL, this );
	m_sdbSizerApply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnGenMapFile ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onQuitDlg ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnGenDrillFile ), NULL, this );

}
