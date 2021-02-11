///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Dec 30 2020)
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

	m_staticTextLayer = new wxStaticText( m_scGridWin, wxID_ANY, _("Layer"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticTextLayer->Wrap( -1 );
	m_staticTextLayer->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgGridSizer->Add( m_staticTextLayer, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_HORIZONTAL, 2 );

	m_staticTextLayerId = new wxStaticText( m_scGridWin, wxID_ANY, _("Id"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticTextLayerId->Wrap( -1 );
	m_staticTextLayerId->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgGridSizer->Add( m_staticTextLayerId, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_staticTextType = new wxStaticText( m_scGridWin, wxID_ANY, _("Type"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticTextType->Wrap( -1 );
	m_staticTextType->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgGridSizer->Add( m_staticTextType, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_HORIZONTAL, 2 );

	m_staticTextMaterial = new wxStaticText( m_scGridWin, wxID_ANY, _("Material"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticTextMaterial->Wrap( -1 );
	m_staticTextMaterial->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgGridSizer->Add( m_staticTextMaterial, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_HORIZONTAL, 2 );

	m_staticTextThickness = new wxStaticText( m_scGridWin, wxID_ANY, _("Thickness"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticTextThickness->Wrap( -1 );
	m_staticTextThickness->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgGridSizer->Add( m_staticTextThickness, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_HORIZONTAL, 2 );

	m_bitmapLockThickness = new wxStaticBitmap( m_scGridWin, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_fgGridSizer->Add( m_bitmapLockThickness, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_staticTextColor = new wxStaticText( m_scGridWin, wxID_ANY, _("Color"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticTextColor->Wrap( -1 );
	m_staticTextColor->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgGridSizer->Add( m_staticTextColor, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_HORIZONTAL, 2 );

	m_staticTextEpsilonR = new wxStaticText( m_scGridWin, wxID_ANY, _("Epsilon R"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticTextEpsilonR->Wrap( -1 );
	m_staticTextEpsilonR->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgGridSizer->Add( m_staticTextEpsilonR, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_HORIZONTAL, 2 );

	m_staticTextLossTg = new wxStaticText( m_scGridWin, wxID_ANY, _("Loss Tg"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticTextLossTg->Wrap( -1 );
	m_staticTextLossTg->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgGridSizer->Add( m_staticTextLossTg, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_HORIZONTAL, 2 );


	m_scGridWin->SetSizer( m_fgGridSizer );
	m_scGridWin->Layout();
	m_fgGridSizer->Fit( m_scGridWin );
	bSizer5->Add( m_scGridWin, 1, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );


	m_sizerStackup->Add( bSizer5, 3, wxEXPAND, 5 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );

	wxString m_rbDielectricConstraintChoices[] = { _("No constraint"), _("Impedance controlled") };
	int m_rbDielectricConstraintNChoices = sizeof( m_rbDielectricConstraintChoices ) / sizeof( wxString );
	m_rbDielectricConstraint = new wxRadioBox( this, wxID_ANY, _("Impedance Control"), wxDefaultPosition, wxDefaultSize, m_rbDielectricConstraintNChoices, m_rbDielectricConstraintChoices, 1, wxRA_SPECIFY_COLS );
	m_rbDielectricConstraint->SetSelection( 0 );
	m_rbDielectricConstraint->SetToolTip( _("If Impedance Controlled option is set,\nLoss tangent and EpsilonR will be added to constraints.") );

	bSizerRight->Add( m_rbDielectricConstraint, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_buttonAddDielectricLayer = new wxButton( this, wxID_ANY, _("Add Dielectric Layer"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_buttonAddDielectricLayer, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_buttonRemoveDielectricLayer = new wxButton( this, wxID_ANY, _("Remove Dielectric Layer"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_buttonRemoveDielectricLayer, 0, wxTOP|wxBOTTOM|wxRIGHT|wxEXPAND, 5 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerRight->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	wxStaticBoxSizer* sbSizerBrdOptions;
	sbSizerBrdOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Board Finish") ), wxVERTICAL );

	m_cbCastellatedPads = new wxCheckBox( sbSizerBrdOptions->GetStaticBox(), wxID_ANY, _("Has castellated pads"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerBrdOptions->Add( m_cbCastellatedPads, 0, wxTOP|wxBOTTOM, 5 );

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


	bSizerRight->Add( sbSizerBrdOptions, 0, wxEXPAND|wxTOP|wxRIGHT, 5 );


	bSizerRight->Add( 0, 0, 1, wxEXPAND, 5 );


	m_sizerStackup->Add( bSizerRight, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( m_sizerStackup, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerBrdThickness;
	bSizerBrdThickness = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextCT = new wxStaticText( this, wxID_ANY, _("Board thickness from stackup:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
	m_staticTextCT->Wrap( -1 );
	bSizerBrdThickness->Add( m_staticTextCT, 2, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_tcCTValue = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	bSizerBrdThickness->Add( m_tcCTValue, 1, wxALL, 5 );


	bSizer6->Add( bSizerBrdThickness, 2, wxEXPAND|wxALL, 5 );


	bSizer6->Add( 0, 0, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );


	bSizer7->Add( 0, 0, 1, wxEXPAND, 5 );

	m_buttonExport = new wxButton( this, wxID_ANY, _("Export to Clipboard"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_buttonExport, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bSizer7->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizer6->Add( bSizer7, 1, wxEXPAND, 5 );


	bMainSizer->Add( bSizer6, 0, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::OnUpdateUI ) );
	m_buttonAddDielectricLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onAddDielectricLayer ), NULL, this );
	m_buttonRemoveDielectricLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onRemoveDielectricLayer ), NULL, this );
	m_buttonRemoveDielectricLayer->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onRemoveDielUI ), NULL, this );
	m_tcCTValue->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onUpdateThicknessValue ), NULL, this );
	m_buttonExport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onExportToClipboard ), NULL, this );
}

PANEL_SETUP_BOARD_STACKUP_BASE::~PANEL_SETUP_BOARD_STACKUP_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::OnUpdateUI ) );
	m_buttonAddDielectricLayer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onAddDielectricLayer ), NULL, this );
	m_buttonRemoveDielectricLayer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onRemoveDielectricLayer ), NULL, this );
	m_buttonRemoveDielectricLayer->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onRemoveDielUI ), NULL, this );
	m_tcCTValue->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onUpdateThicknessValue ), NULL, this );
	m_buttonExport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onExportToClipboard ), NULL, this );

}
