///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_board_stackup_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_BOARD_STACKUP_BASE::PANEL_SETUP_BOARD_STACKUP_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerBrdThickness;
	bSizerBrdThickness = new wxBoxSizer( wxHORIZONTAL );

	m_thicknessLabel = new wxStaticText( this, wxID_ANY, _("Board thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessLabel->Wrap( -1 );
	bSizerBrdThickness->Add( m_thicknessLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_thicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBrdThickness->Add( m_thicknessCtrl, 0, wxALL, 5 );


	bSizerBrdThickness->Add( 20, 0, 0, 0, 5 );

	m_staticTextCT = new wxStaticText( this, wxID_ANY, _("Current thickness from stackup:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCT->Wrap( -1 );
	bSizerBrdThickness->Add( m_staticTextCT, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_tcCTValue = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	bSizerBrdThickness->Add( m_tcCTValue, 0, wxALL, 5 );


	bSizerBrdThickness->Add( 5, 0, 0, 0, 5 );

	m_buttonSetDielectricThickness = new wxButton( this, wxID_ANY, _("Set Dielectric Thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSetDielectricThickness->SetToolTip( _("Set thickness of all not locked dielectric layers.\nThe thickness will be the same for all not locked dielectric layers.") );

	bSizerBrdThickness->Add( m_buttonSetDielectricThickness, 0, wxALL, 5 );


	bMainSizer->Add( bSizerBrdThickness, 0, wxEXPAND|wxALL, 5 );

	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	m_sizerStackup = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );

	m_scGridWin = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_scGridWin->SetScrollRate( 5, 5 );
	m_fgGridSizer = new wxFlexGridSizer( 0, 9, 0, 2 );
	m_fgGridSizer->SetFlexibleDirection( wxHORIZONTAL );
	m_fgGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText7 = new wxStaticText( m_scGridWin, wxID_ANY, _("Layer"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticText7->Wrap( -1 );
	m_staticText7->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgGridSizer->Add( m_staticText7, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 2 );

	m_staticText8 = new wxStaticText( m_scGridWin, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticText8->Wrap( -1 );
	m_staticText8->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgGridSizer->Add( m_staticText8, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText9 = new wxStaticText( m_scGridWin, wxID_ANY, _("Type"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticText9->Wrap( -1 );
	m_staticText9->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgGridSizer->Add( m_staticText9, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 2 );

	m_staticText10 = new wxStaticText( m_scGridWin, wxID_ANY, _("Material"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticText10->Wrap( -1 );
	m_staticText10->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgGridSizer->Add( m_staticText10, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 2 );

	m_staticText101 = new wxStaticText( m_scGridWin, wxID_ANY, _("Thickness"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticText101->Wrap( -1 );
	m_staticText101->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgGridSizer->Add( m_staticText101, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 2 );

	m_bitmapLockThickness = new wxStaticBitmap( m_scGridWin, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_fgGridSizer->Add( m_bitmapLockThickness, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText102 = new wxStaticText( m_scGridWin, wxID_ANY, _("Color"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticText102->Wrap( -1 );
	m_staticText102->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgGridSizer->Add( m_staticText102, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 2 );

	m_staticText103 = new wxStaticText( m_scGridWin, wxID_ANY, _("Epsilon R"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticText103->Wrap( -1 );
	m_staticText103->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgGridSizer->Add( m_staticText103, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 2 );

	m_staticText104 = new wxStaticText( m_scGridWin, wxID_ANY, _("Loss Tan"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticText104->Wrap( -1 );
	m_staticText104->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgGridSizer->Add( m_staticText104, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 2 );


	m_scGridWin->SetSizer( m_fgGridSizer );
	m_scGridWin->Layout();
	m_fgGridSizer->Fit( m_scGridWin );
	bSizer5->Add( m_scGridWin, 1, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );


	m_sizerStackup->Add( bSizer5, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );

	wxString m_rbDielectricConstraintChoices[] = { _("No constraint"), _("Impedance controlled") };
	int m_rbDielectricConstraintNChoices = sizeof( m_rbDielectricConstraintChoices ) / sizeof( wxString );
	m_rbDielectricConstraint = new wxRadioBox( this, wxID_ANY, _("Impedance Control"), wxDefaultPosition, wxDefaultSize, m_rbDielectricConstraintNChoices, m_rbDielectricConstraintChoices, 1, wxRA_SPECIFY_COLS );
	m_rbDielectricConstraint->SetSelection( 0 );
	m_rbDielectricConstraint->SetToolTip( _("If Impedance Controlled option is set,\nLoss tangent and EpsilonR will be added to constraints.") );

	bSizerRight->Add( m_rbDielectricConstraint, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	wxStaticBoxSizer* sbSizerBrdOptions;
	sbSizerBrdOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Board Finish") ), wxVERTICAL );

	m_cbCastellatedPads = new wxCheckBox( sbSizerBrdOptions->GetStaticBox(), wxID_ANY, _("Has castellated pads"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerBrdOptions->Add( m_cbCastellatedPads, 0, wxBOTTOM, 5 );

	m_cbEgdesPlated = new wxCheckBox( sbSizerBrdOptions->GetStaticBox(), wxID_ANY, _("Plated board edge"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerBrdOptions->Add( m_cbEgdesPlated, 0, wxBOTTOM, 5 );

	m_staticTextFinish = new wxStaticText( sbSizerBrdOptions->GetStaticBox(), wxID_ANY, _("Copper finish:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFinish->Wrap( -1 );
	sbSizerBrdOptions->Add( m_staticTextFinish, 0, wxTOP|wxRIGHT, 10 );

	wxArrayString m_choiceFinishChoices;
	m_choiceFinish = new wxChoice( sbSizerBrdOptions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceFinishChoices, 0 );
	m_choiceFinish->SetSelection( 0 );
	sbSizerBrdOptions->Add( m_choiceFinish, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_staticTextEdgeConn = new wxStaticText( sbSizerBrdOptions->GetStaticBox(), wxID_ANY, _("Edge card connectors:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextEdgeConn->Wrap( -1 );
	sbSizerBrdOptions->Add( m_staticTextEdgeConn, 0, wxTOP, 10 );

	wxString m_choiceEdgeConnChoices[] = { _("None"), _("Yes"), _("Yes, bevelled") };
	int m_choiceEdgeConnNChoices = sizeof( m_choiceEdgeConnChoices ) / sizeof( wxString );
	m_choiceEdgeConn = new wxChoice( sbSizerBrdOptions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceEdgeConnNChoices, m_choiceEdgeConnChoices, 0 );
	m_choiceEdgeConn->SetSelection( 0 );
	m_choiceEdgeConn->SetToolTip( _("Options for edge card connectors.") );

	sbSizerBrdOptions->Add( m_choiceEdgeConn, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );


	bSizerRight->Add( sbSizerBrdOptions, 1, wxEXPAND|wxRIGHT, 5 );

	m_buttonExport = new wxButton( this, wxID_ANY, _("Export to Clipboard"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_buttonExport, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	m_sizerStackup->Add( bSizerRight, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( m_sizerStackup, 1, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_thicknessCtrl->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onUpdateThicknessValue ), NULL, this );
	m_buttonSetDielectricThickness->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onCalculateDielectricThickness ), NULL, this );
	m_buttonExport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onExportToClipboard ), NULL, this );
}

PANEL_SETUP_BOARD_STACKUP_BASE::~PANEL_SETUP_BOARD_STACKUP_BASE()
{
	// Disconnect Events
	m_thicknessCtrl->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onUpdateThicknessValue ), NULL, this );
	m_buttonSetDielectricThickness->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onCalculateDielectricThickness ), NULL, this );
	m_buttonExport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onExportToClipboard ), NULL, this );

}
