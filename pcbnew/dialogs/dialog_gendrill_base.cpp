///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Feb 19 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_gendrill_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GENDRILL_BASE::DIALOG_GENDRILL_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* bdirnameSizer;
	bdirnameSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Output directory:") ), wxHORIZONTAL );
	
	m_outputDirectoryName = new wxTextCtrl( bdirnameSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bdirnameSizer->Add( m_outputDirectoryName, 1, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_buttonBrowse = new wxButton( bdirnameSizer->GetStaticBox(), wxID_ANY, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
	bdirnameSizer->Add( m_buttonBrowse, 0, wxBOTTOM|wxLEFT, 5 );
	
	
	bupperSizer->Add( bdirnameSizer, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	
	bMainSizer->Add( bupperSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bmiddlerSizer;
	bmiddlerSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* m_LeftBoxSizer;
	m_LeftBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_rbFileFormatChoices[] = { _("Excellon"), _("Gerber X2 (experimental)") };
	int m_rbFileFormatNChoices = sizeof( m_rbFileFormatChoices ) / sizeof( wxString );
	m_rbFileFormat = new wxRadioBox( this, wxID_ANY, _("File Format:"), wxDefaultPosition, wxDefaultSize, m_rbFileFormatNChoices, m_rbFileFormatChoices, 1, wxRA_SPECIFY_COLS );
	m_rbFileFormat->SetSelection( 0 );
	m_LeftBoxSizer->Add( m_rbFileFormat, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_Choice_UnitChoices[] = { _("Millimeters"), _("Inches") };
	int m_Choice_UnitNChoices = sizeof( m_Choice_UnitChoices ) / sizeof( wxString );
	m_Choice_Unit = new wxRadioBox( this, wxID_ANY, _("Drill Units:"), wxDefaultPosition, wxDefaultSize, m_Choice_UnitNChoices, m_Choice_UnitChoices, 1, wxRA_SPECIFY_COLS );
	m_Choice_Unit->SetSelection( 1 );
	m_LeftBoxSizer->Add( m_Choice_Unit, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_Choice_Zeros_FormatChoices[] = { _("Decimal format"), _("Suppress leading zeros"), _("Suppress trailing zeros"), _("Keep zeros") };
	int m_Choice_Zeros_FormatNChoices = sizeof( m_Choice_Zeros_FormatChoices ) / sizeof( wxString );
	m_Choice_Zeros_Format = new wxRadioBox( this, wxID_ANY, _("Zeros Format"), wxDefaultPosition, wxDefaultSize, m_Choice_Zeros_FormatNChoices, m_Choice_Zeros_FormatChoices, 1, wxRA_SPECIFY_COLS );
	m_Choice_Zeros_Format->SetSelection( 0 );
	m_Choice_Zeros_Format->SetToolTip( _("Choose EXCELLON numbers notation") );
	
	m_LeftBoxSizer->Add( m_Choice_Zeros_Format, 0, wxALL|wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextTitle = new wxStaticText( this, wxID_ANY, _("Precision"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTitle->Wrap( -1 );
	fgSizer1->Add( m_staticTextTitle, 0, wxALL, 5 );
	
	m_staticTextPrecision = new wxStaticText( this, wxID_ANY, _("Precision"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPrecision->Wrap( -1 );
	fgSizer1->Add( m_staticTextPrecision, 0, wxALL, 5 );
	
	
	m_LeftBoxSizer->Add( fgSizer1, 0, wxEXPAND, 5 );
	
	
	bmiddlerSizer->Add( m_LeftBoxSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bMiddleBoxSizer;
	bMiddleBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_Choice_Drill_MapChoices[] = { _("HPGL"), _("PostScript"), _("Gerber"), _("DXF"), _("SVG"), _("PDF") };
	int m_Choice_Drill_MapNChoices = sizeof( m_Choice_Drill_MapChoices ) / sizeof( wxString );
	m_Choice_Drill_Map = new wxRadioBox( this, wxID_ANY, _("Drill Map File Format:"), wxDefaultPosition, wxDefaultSize, m_Choice_Drill_MapNChoices, m_Choice_Drill_MapChoices, 1, wxRA_SPECIFY_COLS );
	m_Choice_Drill_Map->SetSelection( 1 );
	m_Choice_Drill_Map->SetToolTip( _("Creates a drill map in PS, HPGL or other formats") );
	
	bMiddleBoxSizer->Add( m_Choice_Drill_Map, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbExcellonOptSizer;
	sbExcellonOptSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Excellon Drill File Options:") ), wxVERTICAL );
	
	m_Check_Mirror = new wxCheckBox( sbExcellonOptSizer->GetStaticBox(), wxID_ANY, _("Mirror y axis"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Check_Mirror->SetToolTip( _("Not recommended.\nUsed mostly by users who make the boards themselves.") );
	
	sbExcellonOptSizer->Add( m_Check_Mirror, 0, wxRIGHT|wxLEFT, 5 );
	
	m_Check_Minimal = new wxCheckBox( sbExcellonOptSizer->GetStaticBox(), wxID_ANY, _("Minimal header"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Check_Minimal->SetToolTip( _("Not recommended.\nOnly use it for board houses which do not accept fully featured headers.") );
	
	sbExcellonOptSizer->Add( m_Check_Minimal, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Check_Merge_PTH_NPTH = new wxCheckBox( sbExcellonOptSizer->GetStaticBox(), wxID_ANY, _("Merge PTH and NPTH holes into one file"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Check_Merge_PTH_NPTH->SetToolTip( _("Not recommended.\nOnly use for board houses which ask for merged PTH and NTPH into a single file.") );
	
	sbExcellonOptSizer->Add( m_Check_Merge_PTH_NPTH, 0, wxALL, 5 );
	
	
	bMiddleBoxSizer->Add( sbExcellonOptSizer, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	wxString m_Choice_Drill_OffsetChoices[] = { _("Absolute"), _("Auxiliary axis") };
	int m_Choice_Drill_OffsetNChoices = sizeof( m_Choice_Drill_OffsetChoices ) / sizeof( wxString );
	m_Choice_Drill_Offset = new wxRadioBox( this, wxID_ANY, _("Drill Origin:"), wxDefaultPosition, wxDefaultSize, m_Choice_Drill_OffsetNChoices, m_Choice_Drill_OffsetChoices, 1, wxRA_SPECIFY_COLS );
	m_Choice_Drill_Offset->SetSelection( 0 );
	m_Choice_Drill_Offset->SetToolTip( _("Choose the coordinate origin: absolute or relative to the auxiliray axis") );
	
	bMiddleBoxSizer->Add( m_Choice_Drill_Offset, 0, wxALL|wxEXPAND, 5 );
	
	
	bmiddlerSizer->Add( bMiddleBoxSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bRightBoxSizer;
	bRightBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizerInfo;
	sbSizerInfo = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Info:") ), wxVERTICAL );
	
	m_DefaultViasDrillSizer = new wxStaticBoxSizer( new wxStaticBox( sbSizerInfo->GetStaticBox(), wxID_ANY, _("Default Vias Drill:") ), wxVERTICAL );
	
	m_ViaDrillValue = new wxStaticText( m_DefaultViasDrillSizer->GetStaticBox(), wxID_ANY, _("Via Drill Value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaDrillValue->Wrap( -1 );
	m_DefaultViasDrillSizer->Add( m_ViaDrillValue, 0, wxALL, 5 );
	
	
	sbSizerInfo->Add( m_DefaultViasDrillSizer, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_MicroViasDrillSizer = new wxStaticBoxSizer( new wxStaticBox( sbSizerInfo->GetStaticBox(), wxID_ANY, _("Micro Vias Drill:") ), wxVERTICAL );
	
	m_MicroViaDrillValue = new wxStaticText( m_MicroViasDrillSizer->GetStaticBox(), wxID_ANY, _("Micro Via Drill Value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MicroViaDrillValue->Wrap( -1 );
	m_MicroViasDrillSizer->Add( m_MicroViaDrillValue, 0, wxALL, 5 );
	
	
	sbSizerInfo->Add( m_MicroViasDrillSizer, 0, wxEXPAND|wxBOTTOM, 5 );
	
	wxStaticBoxSizer* sbSizerHoles;
	sbSizerHoles = new wxStaticBoxSizer( new wxStaticBox( sbSizerInfo->GetStaticBox(), wxID_ANY, _("Holes Count:") ), wxVERTICAL );
	
	m_PlatedPadsCountInfoMsg = new wxStaticText( sbSizerHoles->GetStaticBox(), wxID_ANY, _("Plated Pads:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PlatedPadsCountInfoMsg->Wrap( -1 );
	sbSizerHoles->Add( m_PlatedPadsCountInfoMsg, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_NotPlatedPadsCountInfoMsg = new wxStaticText( sbSizerHoles->GetStaticBox(), wxID_ANY, _("Not Plated Pads:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_NotPlatedPadsCountInfoMsg->Wrap( -1 );
	sbSizerHoles->Add( m_NotPlatedPadsCountInfoMsg, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ThroughViasInfoMsg = new wxStaticText( sbSizerHoles->GetStaticBox(), wxID_ANY, _("Through Vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThroughViasInfoMsg->Wrap( -1 );
	sbSizerHoles->Add( m_ThroughViasInfoMsg, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_MicroViasInfoMsg = new wxStaticText( sbSizerHoles->GetStaticBox(), wxID_ANY, _("Micro Vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MicroViasInfoMsg->Wrap( -1 );
	sbSizerHoles->Add( m_MicroViasInfoMsg, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_BuriedViasInfoMsg = new wxStaticText( sbSizerHoles->GetStaticBox(), wxID_ANY, _("Buried Vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BuriedViasInfoMsg->Wrap( -1 );
	sbSizerHoles->Add( m_BuriedViasInfoMsg, 0, wxALL, 5 );
	
	
	sbSizerInfo->Add( sbSizerHoles, 1, wxEXPAND|wxBOTTOM, 5 );
	
	
	bRightBoxSizer->Add( sbSizerInfo, 0, wxEXPAND|wxTOP, 5 );
	
	
	bmiddlerSizer->Add( bRightBoxSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxVERTICAL );
	
	
	bSizerButtons->Add( 10, 20, 0, 0, 5 );
	
	m_buttonDrill = new wxButton( this, ID_GEN_DRILL_FILE, _("Drill File"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonDrill->SetDefault(); 
	bSizerButtons->Add( m_buttonDrill, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonMap = new wxButton( this, wxID_ANY, _("Map File"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonMap, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonReport = new wxButton( this, wxID_ANY, _("Report File"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonReport, 0, wxALL|wxEXPAND, 5 );
	
	m_CancelButton = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_CancelButton, 0, wxALL|wxEXPAND, 5 );
	
	
	bmiddlerSizer->Add( bSizerButtons, 1, wxEXPAND, 5 );
	
	
	bMainSizer->Add( bmiddlerSizer, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* bmsgSizer;
	bmsgSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Messages:") ), wxVERTICAL );
	
	m_messagesBox = new wxTextCtrl( bmsgSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_messagesBox->SetMinSize( wxSize( -1,90 ) );
	
	bmsgSizer->Add( m_messagesBox, 1, wxALL|wxEXPAND, 5 );
	
	
	bMainSizer->Add( bmsgSizer, 1, wxEXPAND, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_buttonBrowse->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
	m_rbFileFormat->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onFileFormatSelection ), NULL, this );
	m_Choice_Unit->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnSelDrillUnitsSelected ), NULL, this );
	m_Choice_Zeros_Format->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnSelZerosFmtSelected ), NULL, this );
	m_buttonDrill->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnGenDrillFile ), NULL, this );
	m_buttonMap->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnGenMapFile ), NULL, this );
	m_buttonReport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnGenReportFile ), NULL, this );
	m_CancelButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnCancelClick ), NULL, this );
}

DIALOG_GENDRILL_BASE::~DIALOG_GENDRILL_BASE()
{
	// Disconnect Events
	m_buttonBrowse->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
	m_rbFileFormat->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::onFileFormatSelection ), NULL, this );
	m_Choice_Unit->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnSelDrillUnitsSelected ), NULL, this );
	m_Choice_Zeros_Format->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnSelZerosFmtSelected ), NULL, this );
	m_buttonDrill->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnGenDrillFile ), NULL, this );
	m_buttonMap->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnGenMapFile ), NULL, this );
	m_buttonReport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnGenReportFile ), NULL, this );
	m_CancelButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnCancelClick ), NULL, this );
	
}
