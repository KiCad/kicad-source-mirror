///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug  4 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"

#include "dialog_dxf_import_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DXF_IMPORT_BASE::DIALOG_DXF_IMPORT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerFile;
	bSizerFile = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticTextFile = new wxStaticText( this, wxID_ANY, _("File:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFile->Wrap( -1 );
	bSizerFile->Add( m_staticTextFile, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_textCtrlFileName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlFileName->SetMinSize( wxSize( 300,-1 ) );
	
	bSizerFile->Add( m_textCtrlFileName, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxTOP, 5 );
	
	m_buttonBrowse = new wxButton( this, wxID_ANY, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerFile->Add( m_buttonBrowse, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxTOP, 5 );
	
	
	bSizerMain->Add( bSizerFile, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizerMiddle;
	bSizerMiddle = new wxBoxSizer( wxHORIZONTAL );
	
	wxString m_rbOffsetOptionChoices[] = { _("Center of page"), _("Upper left corner of page"), _("Center left side of page"), _("Lower left corner of page"), _("User defined position") };
	int m_rbOffsetOptionNChoices = sizeof( m_rbOffsetOptionChoices ) / sizeof( wxString );
	m_rbOffsetOption = new wxRadioBox( this, wxID_ORIGIN_SELECT, _("Place DXF Origin (0,0) Point:"), wxDefaultPosition, wxDefaultSize, m_rbOffsetOptionNChoices, m_rbOffsetOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_rbOffsetOption->SetSelection( 0 );
	bSizerMiddle->Add( m_rbOffsetOption, 1, wxALL, 5 );
	
	wxBoxSizer* bSizerUserPos;
	bSizerUserPos = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerPosSettings;
	bSizerPosSettings = new wxBoxSizer( wxVERTICAL );
	
	m_staticText6 = new wxStaticText( this, wxID_ANY, _("User defined position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	bSizerPosSettings->Add( m_staticText6, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizerUserPosition;
	fgSizerUserPosition = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerUserPosition->AddGrowableCol( 2 );
	fgSizerUserPosition->SetFlexibleDirection( wxBOTH );
	fgSizerUserPosition->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	fgSizerUserPosition->Add( 0, 0, 0, wxRIGHT|wxLEFT, 5 );
	
	m_staticTextXpos = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextXpos->Wrap( -1 );
	fgSizerUserPosition->Add( m_staticTextXpos, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_DxfPcbXCoord = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_DxfPcbXCoord->HasFlag( wxTE_MULTILINE ) )
	{
	m_DxfPcbXCoord->SetMaxLength( 10 );
	}
	#else
	m_DxfPcbXCoord->SetMaxLength( 10 );
	#endif
	m_DxfPcbXCoord->SetToolTip( _("DXF origin on PCB Grid, X Coordinate") );
	
	fgSizerUserPosition->Add( m_DxfPcbXCoord, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );
	
	
	fgSizerUserPosition->Add( 0, 0, 0, wxRIGHT|wxLEFT, 5 );
	
	m_staticTextYpos = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextYpos->Wrap( -1 );
	fgSizerUserPosition->Add( m_staticTextYpos, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_DxfPcbYCoord = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_DxfPcbYCoord->HasFlag( wxTE_MULTILINE ) )
	{
	m_DxfPcbYCoord->SetMaxLength( 10 );
	}
	#else
	m_DxfPcbYCoord->SetMaxLength( 10 );
	#endif
	m_DxfPcbYCoord->SetToolTip( _("DXF origin on PCB Grid, Y Coordinate") );
	
	fgSizerUserPosition->Add( m_DxfPcbYCoord, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	
	fgSizerUserPosition->Add( 0, 0, 0, wxRIGHT|wxLEFT, 5 );
	
	m_staticTextUnits = new wxStaticText( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUnits->Wrap( -1 );
	fgSizerUserPosition->Add( m_staticTextUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	wxString m_DxfPcbPositionUnitsChoices[] = { _("mm"), _("inch") };
	int m_DxfPcbPositionUnitsNChoices = sizeof( m_DxfPcbPositionUnitsChoices ) / sizeof( wxString );
	m_DxfPcbPositionUnits = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_DxfPcbPositionUnitsNChoices, m_DxfPcbPositionUnitsChoices, 0 );
	m_DxfPcbPositionUnits->SetSelection( 0 );
	m_DxfPcbPositionUnits->SetToolTip( _("Select PCB grid units") );
	
	fgSizerUserPosition->Add( m_DxfPcbPositionUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	
	bSizerPosSettings->Add( fgSizerUserPosition, 1, wxEXPAND, 5 );
	
	
	bSizerUserPos->Add( bSizerPosSettings, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	
	bSizerMiddle->Add( bSizerUserPos, 1, 0, 5 );
	
	
	bSizerMain->Add( bSizerMiddle, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizerLayer;
	bSizerLayer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextPrms = new wxStaticText( this, wxID_ANY, _("Import parameters:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPrms->Wrap( -1 );
	bSizerLayer->Add( m_staticTextPrms, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer7->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );
	
	wxFlexGridSizer* fgSizerImportSettings;
	fgSizerImportSettings = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerImportSettings->AddGrowableCol( 1 );
	fgSizerImportSettings->SetFlexibleDirection( wxBOTH );
	fgSizerImportSettings->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextLineWidth = new wxStaticText( this, wxID_ANY, _("Default line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLineWidth->Wrap( -1 );
	fgSizerImportSettings->Add( m_staticTextLineWidth, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlLineWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerImportSettings->Add( m_textCtrlLineWidth, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_choiceUnitLineWidthChoices[] = { _("mm"), _("mils"), _("inches") };
	int m_choiceUnitLineWidthNChoices = sizeof( m_choiceUnitLineWidthChoices ) / sizeof( wxString );
	m_choiceUnitLineWidth = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnitLineWidthNChoices, m_choiceUnitLineWidthChoices, 0 );
	m_choiceUnitLineWidth->SetSelection( 0 );
	fgSizerImportSettings->Add( m_choiceUnitLineWidth, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextBrdlayer = new wxStaticText( this, wxID_ANY, _("Graphic layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBrdlayer->Wrap( -1 );
	fgSizerImportSettings->Add( m_staticTextBrdlayer, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	m_SelLayerBox = new PCB_LAYER_BOX_SELECTOR( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	fgSizerImportSettings->Add( m_SelLayerBox, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );
	
	
	bSizer7->Add( fgSizerImportSettings, 1, wxEXPAND, 5 );
	
	
	bSizerLayer->Add( bSizer7, 1, wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizerLayer, 0, wxALL|wxEXPAND, 5 );
	
	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerMain->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_buttonBrowse->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DXF_IMPORT_BASE::OnBrowseDxfFiles ), NULL, this );
	m_rbOffsetOption->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_DXF_IMPORT_BASE::OriginOptionOnUpdateUI ), NULL, this );
	m_DxfPcbPositionUnits->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_DXF_IMPORT_BASE::onUnitPositionSelection ), NULL, this );
	m_choiceUnitLineWidth->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_DXF_IMPORT_BASE::onUnitWidthSelection ), NULL, this );
}

DIALOG_DXF_IMPORT_BASE::~DIALOG_DXF_IMPORT_BASE()
{
	// Disconnect Events
	m_buttonBrowse->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DXF_IMPORT_BASE::OnBrowseDxfFiles ), NULL, this );
	m_rbOffsetOption->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_DXF_IMPORT_BASE::OriginOptionOnUpdateUI ), NULL, this );
	m_DxfPcbPositionUnits->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_DXF_IMPORT_BASE::onUnitPositionSelection ), NULL, this );
	m_choiceUnitLineWidth->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_DXF_IMPORT_BASE::onUnitWidthSelection ), NULL, this );
	
}
