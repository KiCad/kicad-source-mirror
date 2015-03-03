///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  6 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_pns_length_tuning_settings_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE::DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 345,668 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Length/skew") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer4;
	fgSizer4 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer4->AddGrowableCol( 1 );
	fgSizer4->SetFlexibleDirection( wxBOTH );
	fgSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText4 = new wxStaticText( this, wxID_ANY, _("Tune from:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	fgSizer4->Add( m_staticText4, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxArrayString m_choicePathFromChoices;
	m_choicePathFrom = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choicePathFromChoices, 0 );
	m_choicePathFrom->SetSelection( 0 );
	fgSizer4->Add( m_choicePathFrom, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText15 = new wxStaticText( this, wxID_ANY, _("Tune to:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	fgSizer4->Add( m_staticText15, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxArrayString m_choice4Choices;
	m_choice4 = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choice4Choices, 0 );
	m_choice4->SetSelection( 0 );
	fgSizer4->Add( m_choice4, 0, wxALL, 5 );
	
	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Constraint:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	fgSizer4->Add( m_staticText3, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_constraintSourceChoices[] = { _("from Design Rules"), _("manual") };
	int m_constraintSourceNChoices = sizeof( m_constraintSourceChoices ) / sizeof( wxString );
	m_constraintSource = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_constraintSourceNChoices, m_constraintSourceChoices, 0 );
	m_constraintSource->SetSelection( 1 );
	m_constraintSource->Enable( false );
	
	fgSizer4->Add( m_constraintSource, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_targetLengthLabel = new wxStaticText( this, wxID_ANY, _("Target length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_targetLengthLabel->Wrap( -1 );
	fgSizer4->Add( m_targetLengthLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxGridSizer* gSizer2;
	gSizer2 = new wxGridSizer( 0, 2, 0, 0 );
	
	m_targetLengthText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gSizer2->Add( m_targetLengthText, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_targetLengthUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_targetLengthUnit->Wrap( -1 );
	gSizer2->Add( m_targetLengthUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	fgSizer4->Add( gSizer2, 1, wxEXPAND, 5 );
	
	
	sbSizer1->Add( fgSizer4, 1, wxEXPAND, 5 );
	
	
	bMainSizer->Add( sbSizer1, 0, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Meandering") ), wxVERTICAL );
	
	m_legend = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer2->Add( m_legend, 1, wxALL|wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer3->AddGrowableCol( 2 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Min amplitude (Amin):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizer3->Add( m_staticText9, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_minAmplText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_minAmplText, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_minAmplUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minAmplUnit->Wrap( -1 );
	fgSizer3->Add( m_minAmplUnit, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_staticText91 = new wxStaticText( this, wxID_ANY, _("Max amplitude (Amax):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText91->Wrap( -1 );
	fgSizer3->Add( m_staticText91, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_maxAmplText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_maxAmplText, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_maxAmplUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maxAmplUnit->Wrap( -1 );
	fgSizer3->Add( m_maxAmplUnit, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_staticText11 = new wxStaticText( this, wxID_ANY, _("Spacing (s):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	fgSizer3->Add( m_staticText11, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_spacingText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_spacingText, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_spacingUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spacingUnit->Wrap( -1 );
	fgSizer3->Add( m_spacingUnit, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_staticText13 = new wxStaticText( this, wxID_ANY, _("Miter radius (r):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText13->Wrap( -1 );
	fgSizer3->Add( m_staticText13, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_radiusText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_radiusText, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_radiusUnit = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radiusUnit->Wrap( -1 );
	fgSizer3->Add( m_radiusUnit, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_staticText14 = new wxStaticText( this, wxID_ANY, _("Miter style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	m_staticText14->Enable( false );
	
	fgSizer3->Add( m_staticText14, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_miterStyleChoices[] = { _("45 degree"), _("arc") };
	int m_miterStyleNChoices = sizeof( m_miterStyleChoices ) / sizeof( wxString );
	m_miterStyle = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_miterStyleNChoices, m_miterStyleChoices, 0 );
	m_miterStyle->SetSelection( 0 );
	m_miterStyle->Enable( false );
	
	fgSizer3->Add( m_miterStyle, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	sbSizer2->Add( fgSizer3, 1, wxEXPAND, 5 );
	
	
	bMainSizer->Add( sbSizer2, 1, wxALL|wxEXPAND, 5 );
	
	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();
	
	bMainSizer->Add( m_stdButtons, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE::OnClose ) );
	m_stdButtonsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE::OnCancelClick ), NULL, this );
	m_stdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE::OnOkClick ), NULL, this );
}

DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE::~DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE::OnClose ) );
	m_stdButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE::OnCancelClick ), NULL, this );
	m_stdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE::OnOkClick ), NULL, this );
	
}
