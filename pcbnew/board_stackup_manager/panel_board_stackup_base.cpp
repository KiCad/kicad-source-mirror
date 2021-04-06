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

	wxBoxSizer* bTopSizer;
	bTopSizer = new wxBoxSizer( wxHORIZONTAL );

	m_lblCopperLayers = new wxStaticText( this, wxID_ANY, _("Copper layers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblCopperLayers->Wrap( -1 );
	m_lblCopperLayers->SetToolTip( _("Select the number of copper layers in the stackup") );

	bTopSizer->Add( m_lblCopperLayers, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	wxString m_choiceCopperLayersChoices[] = { _("2"), _("4"), _("6"), _("8"), _("10"), _("12"), _("14"), _("16"), _("18"), _("20"), _("22"), _("24"), _("26"), _("28"), _("30"), _("32") };
	int m_choiceCopperLayersNChoices = sizeof( m_choiceCopperLayersChoices ) / sizeof( wxString );
	m_choiceCopperLayers = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceCopperLayersNChoices, m_choiceCopperLayersChoices, 0 );
	m_choiceCopperLayers->SetSelection( 0 );
	m_choiceCopperLayers->SetToolTip( _("Select the number of copper layers in the stackup") );

	bTopSizer->Add( m_choiceCopperLayers, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bTopSizer->Add( 40, 0, 1, wxEXPAND, 5 );

	m_impedanceControlled = new wxCheckBox( this, wxID_ANY, _("Impedance controlled"), wxDefaultPosition, wxDefaultSize, 0 );
	m_impedanceControlled->SetToolTip( _("If Impedance Controlled option is set,\nLoss tangent and EpsilonR will be added to constraints.") );

	bTopSizer->Add( m_impedanceControlled, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bTopSizer->Add( 40, 0, 1, wxEXPAND, 5 );

	m_buttonAddDielectricLayer = new wxButton( this, wxID_ANY, _("Add Dielectric Layer"), wxDefaultPosition, wxDefaultSize, 0 );
	bTopSizer->Add( m_buttonAddDielectricLayer, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_buttonRemoveDielectricLayer = new wxButton( this, wxID_ANY, _("Remove Dielectric Layer"), wxDefaultPosition, wxDefaultSize, 0 );
	bTopSizer->Add( m_buttonRemoveDielectricLayer, 0, wxEXPAND|wxALL, 5 );


	bMainSizer->Add( bTopSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* m_sizerStackup;
	m_sizerStackup = new wxBoxSizer( wxVERTICAL );

	m_scGridWin = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN|wxHSCROLL|wxVSCROLL );
	m_scGridWin->SetScrollRate( 5, 5 );
	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	m_fgGridSizer = new wxFlexGridSizer( 0, 9, 0, 4 );
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


	bMargins->Add( m_fgGridSizer, 1, wxEXPAND|wxLEFT, 5 );


	m_scGridWin->SetSizer( bMargins );
	m_scGridWin->Layout();
	bMargins->Fit( m_scGridWin );
	m_sizerStackup->Add( m_scGridWin, 1, wxEXPAND|wxRIGHT, 10 );


	bMainSizer->Add( m_sizerStackup, 3, wxEXPAND|wxBOTTOM|wxLEFT, 5 );

	wxBoxSizer* bBottomSizer;
	bBottomSizer = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextCT = new wxStaticText( this, wxID_ANY, _("Board thickness from stackup:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
	m_staticTextCT->Wrap( -1 );
	bBottomSizer->Add( m_staticTextCT, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_tcCTValue = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	bBottomSizer->Add( m_tcCTValue, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bBottomSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_buttonExport = new wxButton( this, wxID_ANY, _("Export to Clipboard"), wxDefaultPosition, wxDefaultSize, 0 );
	bBottomSizer->Add( m_buttonExport, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bMainSizer->Add( bBottomSizer, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_buttonAddDielectricLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onAddDielectricLayer ), NULL, this );
	m_buttonRemoveDielectricLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onRemoveDielectricLayer ), NULL, this );
	m_buttonRemoveDielectricLayer->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onRemoveDielUI ), NULL, this );
	m_tcCTValue->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onUpdateThicknessValue ), NULL, this );
	m_buttonExport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onExportToClipboard ), NULL, this );
}

PANEL_SETUP_BOARD_STACKUP_BASE::~PANEL_SETUP_BOARD_STACKUP_BASE()
{
	// Disconnect Events
	m_buttonAddDielectricLayer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onAddDielectricLayer ), NULL, this );
	m_buttonRemoveDielectricLayer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onRemoveDielectricLayer ), NULL, this );
	m_buttonRemoveDielectricLayer->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onRemoveDielUI ), NULL, this );
	m_tcCTValue->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onUpdateThicknessValue ), NULL, this );
	m_buttonExport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_BOARD_STACKUP_BASE::onExportToClipboard ), NULL, this );

}
