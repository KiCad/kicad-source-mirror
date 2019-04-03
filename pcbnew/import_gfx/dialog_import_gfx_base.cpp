///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar 20 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"

#include "dialog_import_gfx_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_IMPORT_GFX_BASE::DIALOG_IMPORT_GFX_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerFile;
	bSizerFile = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextFile = new wxStaticText( this, wxID_ANY, _("File:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFile->Wrap( -1 );
	bSizerFile->Add( m_staticTextFile, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_textCtrlFileName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlFileName->SetMinSize( wxSize( 300,-1 ) );

	bSizerFile->Add( m_textCtrlFileName, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_buttonBrowse = new wxButton( this, wxID_ANY, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerFile->Add( m_buttonBrowse, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizerMain->Add( bSizerFile, 0, wxALL|wxEXPAND, 5 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bSizerPlacement;
	bSizerPlacement = new wxBoxSizer( wxVERTICAL );

	m_staticTextPlacement = new wxStaticText( this, wxID_ANY, _("Placement:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPlacement->Wrap( -1 );
	m_staticTextPlacement->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizerPlacement->Add( m_staticTextPlacement, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bSizerOptions;
	bSizerOptions = new wxBoxSizer( wxVERTICAL );

	m_rbInteractivePlacement = new wxRadioButton( this, wxID_ANY, _("Interactive placement"), wxDefaultPosition, wxDefaultSize, wxRB_SINGLE );
	m_rbInteractivePlacement->SetValue( true );
	bSizerOptions->Add( m_rbInteractivePlacement, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );

	wxBoxSizer* bSizerUserPos;
	bSizerUserPos = new wxBoxSizer( wxHORIZONTAL );

	m_rbAbsolutePlacement = new wxRadioButton( this, wxID_ANY, _("At"), wxDefaultPosition, wxDefaultSize, wxRB_SINGLE );
	bSizerUserPos->Add( m_rbAbsolutePlacement, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizerPosSettings;
	bSizerPosSettings = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextXpos = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextXpos->Wrap( -1 );
	bSizerPosSettings->Add( m_staticTextXpos, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

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

	bSizerPosSettings->Add( m_DxfPcbXCoord, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticTextYpos = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextYpos->Wrap( -1 );
	bSizerPosSettings->Add( m_staticTextYpos, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

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

	bSizerPosSettings->Add( m_DxfPcbYCoord, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticTextUnits = new wxStaticText( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUnits->Wrap( -1 );
	bSizerPosSettings->Add( m_staticTextUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxString m_DxfPcbPositionUnitsChoices[] = { _("mm"), _("inch") };
	int m_DxfPcbPositionUnitsNChoices = sizeof( m_DxfPcbPositionUnitsChoices ) / sizeof( wxString );
	m_DxfPcbPositionUnits = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_DxfPcbPositionUnitsNChoices, m_DxfPcbPositionUnitsChoices, 0 );
	m_DxfPcbPositionUnits->SetSelection( 0 );
	m_DxfPcbPositionUnits->SetToolTip( _("Select PCB grid units") );

	bSizerPosSettings->Add( m_DxfPcbPositionUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizerUserPos->Add( bSizerPosSettings, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND|wxTOP, 5 );


	bSizerOptions->Add( bSizerUserPos, 0, wxEXPAND, 5 );


	bSizerPlacement->Add( bSizerOptions, 1, wxEXPAND|wxLEFT, 20 );


	bSizerMain->Add( bSizerPlacement, 1, wxEXPAND, 5 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline3, 0, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bSizerLayer;
	bSizerLayer = new wxBoxSizer( wxVERTICAL );

	m_staticTextPrms = new wxStaticText( this, wxID_ANY, _("Import parameters:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPrms->Wrap( -1 );
	m_staticTextPrms->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizerLayer->Add( m_staticTextPrms, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );


	bSizer7->Add( 0, 0, 0, wxRIGHT|wxLEFT, 10 );

	wxFlexGridSizer* fgSizerImportSettings;
	fgSizerImportSettings = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgSizerImportSettings->AddGrowableCol( 1 );
	fgSizerImportSettings->SetFlexibleDirection( wxBOTH );
	fgSizerImportSettings->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextLineWidth = new wxStaticText( this, wxID_ANY, _("Line width (DXF import):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLineWidth->Wrap( -1 );
	fgSizerImportSettings->Add( m_staticTextLineWidth, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlLineWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerImportSettings->Add( m_textCtrlLineWidth, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxString m_choiceUnitLineWidthChoices[] = { _("mm"), _("mils"), _("inches") };
	int m_choiceUnitLineWidthNChoices = sizeof( m_choiceUnitLineWidthChoices ) / sizeof( wxString );
	m_choiceUnitLineWidth = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnitLineWidthNChoices, m_choiceUnitLineWidthChoices, 0 );
	m_choiceUnitLineWidth->SetSelection( 0 );
	fgSizerImportSettings->Add( m_choiceUnitLineWidth, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticTextBrdlayer = new wxStaticText( this, wxID_ANY, _("Graphic layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBrdlayer->Wrap( -1 );
	fgSizerImportSettings->Add( m_staticTextBrdlayer, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_SelLayerBox = new PCB_LAYER_BOX_SELECTOR( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgSizerImportSettings->Add( m_SelLayerBox, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	fgSizerImportSettings->Add( 0, 0, 0, 0, 5 );

	m_staticTextscale = new wxStaticText( this, wxID_ANY, _("Import scale:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextscale->Wrap( -1 );
	fgSizerImportSettings->Add( m_staticTextscale, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlImportScale = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerImportSettings->Add( m_textCtrlImportScale, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	fgSizerImportSettings->Add( 0, 0, 0, 0, 5 );


	bSizer7->Add( fgSizerImportSettings, 1, wxEXPAND|wxRIGHT|wxTOP, 5 );


	bSizerLayer->Add( bSizer7, 1, wxEXPAND, 5 );


	bSizerMain->Add( bSizerLayer, 0, wxBOTTOM|wxEXPAND, 5 );


	bSizerMain->Add( 0, 0, 0, wxEXPAND, 5 );

	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline, 0, wxALL|wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerMain->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_buttonBrowse->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onBrowseFiles ), NULL, this );
	m_rbInteractivePlacement->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onInteractivePlacement ), NULL, this );
	m_rbInteractivePlacement->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORT_GFX_BASE::originOptionOnUpdateUI ), NULL, this );
	m_rbAbsolutePlacement->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onAbsolutePlacement ), NULL, this );
	m_rbAbsolutePlacement->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORT_GFX_BASE::originOptionOnUpdateUI ), NULL, this );
	m_DxfPcbPositionUnits->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onUnitPositionSelection ), NULL, this );
	m_choiceUnitLineWidth->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onUnitWidthSelection ), NULL, this );
}

DIALOG_IMPORT_GFX_BASE::~DIALOG_IMPORT_GFX_BASE()
{
	// Disconnect Events
	m_buttonBrowse->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onBrowseFiles ), NULL, this );
	m_rbInteractivePlacement->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onInteractivePlacement ), NULL, this );
	m_rbInteractivePlacement->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORT_GFX_BASE::originOptionOnUpdateUI ), NULL, this );
	m_rbAbsolutePlacement->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onAbsolutePlacement ), NULL, this );
	m_rbAbsolutePlacement->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORT_GFX_BASE::originOptionOnUpdateUI ), NULL, this );
	m_DxfPcbPositionUnits->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onUnitPositionSelection ), NULL, this );
	m_choiceUnitLineWidth->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onUnitWidthSelection ), NULL, this );

}
