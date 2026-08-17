///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_via_stitch_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_VIA_STITCH_PROPERTIES_BASE::DIALOG_VIA_STITCH_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* m_MainSizer;
	m_MainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bGeneralSizer;
	bGeneralSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	m_middleBoxSizer = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 3, 3 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( 20,10 ) );

	m_netLabel = new wxStaticText( this, wxID_ANY, _("Net:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_netLabel->Wrap( -1 );
	gbSizer1->Add( m_netLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP, 6 );

	m_netSelector = new NET_SELECTOR( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_netSelector, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_posXLabel = new wxStaticText( this, wxID_ANY, _("Position X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posXLabel->Wrap( -1 );
	gbSizer1->Add( m_posXLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_posXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_posXCtrl->SetMinSize( wxSize( 155,-1 ) );

	gbSizer1->Add( m_posXCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_posXUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posXUnits->Wrap( -1 );
	gbSizer1->Add( m_posXUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_posYLabel = new wxStaticText( this, wxID_ANY, _("Position Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posYLabel->Wrap( -1 );
	gbSizer1->Add( m_posYLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_posYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_posYCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_posYUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posYUnits->Wrap( -1 );
	gbSizer1->Add( m_posYUnits, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_patternLabel = new wxStaticText( this, wxID_ANY, _("Pattern:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_patternLabel->Wrap( -1 );
	gbSizer1->Add( m_patternLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_patternCombo = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_patternCombo->Append( _("Normal Grid") );
	m_patternCombo->Append( _("Staggered Grid") );
	m_patternCombo->Append( _("Poisson") );
	gbSizer1->Add( m_patternCombo, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_pitchLabel = new wxStaticText( this, wxID_ANY, _("Pitch:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pitchLabel->Wrap( -1 );
	gbSizer1->Add( m_pitchLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_pitchCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_pitchCtrl, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_textSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	gbSizer1->Add( m_textSizeUnits, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_seedLabel = new wxStaticText( this, wxID_ANY, _("Seed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_seedLabel->Wrap( -1 );
	gbSizer1->Add( m_seedLabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_seedCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_seedCtrl, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_pitchLabel1 = new wxStaticText( this, wxID_ANY, _("Via Properties:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pitchLabel1->Wrap( -1 );
	gbSizer1->Add( m_pitchLabel1, wxGBPosition( 8, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_viaProperties = new wxButton( this, wxID_ANY, _("Configure"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_viaProperties, wxGBPosition( 8, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	gbSizer1->AddGrowableCol( 1 );

	m_middleBoxSizer->Add( gbSizer1, 1, wxEXPAND|wxBOTTOM|wxLEFT, 5 );


	bSizer9->Add( m_middleBoxSizer, 0, wxALL|wxEXPAND, 5 );


	bSizer8->Add( bSizer9, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerDisplayPad;
	bSizerDisplayPad = new wxBoxSizer( wxVERTICAL );

	m_panelShowPreview = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), wxDefaultSize, m_galOptions, EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO);
	m_panelShowPreview->SetMinSize( wxSize( 280,-1 ) );

	bSizerDisplayPad->Add( m_panelShowPreview, 12, wxEXPAND|wxALL, 5 );


	bSizer8->Add( bSizerDisplayPad, 1, wxEXPAND|wxLEFT, 10 );


	bGeneralSizer->Add( bSizer8, 1, wxEXPAND, 5 );


	bSizerUpper->Add( bGeneralSizer, 1, wxEXPAND|wxRIGHT, 5 );


	m_MainSizer->Add( bSizerUpper, 1, wxEXPAND|wxTOP|wxLEFT, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_MainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_VIA_STITCH_PROPERTIES_BASE::OnUpdateUI ) );
	m_patternCombo->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_VIA_STITCH_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_patternCombo->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_VIA_STITCH_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_pitchCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_VIA_STITCH_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_seedCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_VIA_STITCH_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_viaProperties->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_VIA_STITCH_PROPERTIES_BASE::OnViaConfigureClicked ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_VIA_STITCH_PROPERTIES_BASE::OnCancel ), NULL, this );
}

DIALOG_VIA_STITCH_PROPERTIES_BASE::~DIALOG_VIA_STITCH_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_VIA_STITCH_PROPERTIES_BASE::OnUpdateUI ) );
	m_patternCombo->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_VIA_STITCH_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_patternCombo->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_VIA_STITCH_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_pitchCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_VIA_STITCH_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_seedCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_VIA_STITCH_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_viaProperties->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_VIA_STITCH_PROPERTIES_BASE::OnViaConfigureClicked ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_VIA_STITCH_PROPERTIES_BASE::OnCancel ), NULL, this );

}
