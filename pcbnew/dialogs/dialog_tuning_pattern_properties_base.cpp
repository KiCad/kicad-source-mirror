///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_tuning_pattern_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_TUNING_PATTERN_PROPERTIES_BASE::DIALOG_TUNING_PATTERN_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* singleTrackSizer;
	singleTrackSizer = new wxBoxSizer( wxHORIZONTAL );

	m_legend = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	singleTrackSizer->Add( m_legend, 0, wxEXPAND|wxRIGHT|wxLEFT, 15 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 4, 4 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( 10,8 ) );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_radioBtnLength = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtnLength->SetValue( true );
	bSizer3->Add( m_radioBtnLength, 0, wxALL, 5 );

	m_targetLengthLabel = new wxStaticText( this, wxID_ANY, _("Target Length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_targetLengthLabel->Wrap( -1 );
	bSizer3->Add( m_targetLengthLabel, 0, wxALL, 5 );


	gbSizer1->Add( bSizer3, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), 0, 5 );

	m_targetLengthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_targetLengthCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_targetLengthUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_targetLengthUnits->Wrap( -1 );
	gbSizer1->Add( m_targetLengthUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxHORIZONTAL );

	m_radioBtnDelay = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer31->Add( m_radioBtnDelay, 0, wxALL, 5 );

	m_targetDelayLabel = new wxStaticText( this, wxID_ANY, _("Target Delay:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_targetDelayLabel->Wrap( -1 );
	bSizer31->Add( m_targetDelayLabel, 0, wxALL, 5 );


	gbSizer1->Add( bSizer31, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), 0, 5 );

	m_targetDelayCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_targetDelayCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_targetDelayUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_targetDelayUnits->Wrap( -1 );
	gbSizer1->Add( m_targetDelayUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_overrideCustomRules = new wxCheckBox( this, wxID_ANY, _("Override custom rules"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_overrideCustomRules, wxGBPosition( 0, 4 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_sourceInfo = new wxStaticText( this, wxID_ANY, _("(from 'rule name')"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sourceInfo->Wrap( -1 );
	gbSizer1->Add( m_sourceInfo, wxGBPosition( 2, 1 ), wxGBSpan( 1, 6 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_track_minALabel = new wxStaticText( this, wxID_ANY, _("Minimum amplitude (A):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_track_minALabel->Wrap( -1 );
	gbSizer1->Add( m_track_minALabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 1 );

	m_minACtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_minACtrl, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_minAUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minAUnits->Wrap( -1 );
	gbSizer1->Add( m_minAUnits, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_maxALabel = new wxStaticText( this, wxID_ANY, _("Maximum amplitude (A):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maxALabel->Wrap( -1 );
	gbSizer1->Add( m_maxALabel, wxGBPosition( 5, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_maxACtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_maxACtrl, wxGBPosition( 5, 5 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_maxAUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maxAUnits->Wrap( -1 );
	gbSizer1->Add( m_maxAUnits, wxGBPosition( 5, 6 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_spacingLabel = new wxStaticText( this, wxID_ANY, _("Spacing (s):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spacingLabel->Wrap( -1 );
	gbSizer1->Add( m_spacingLabel, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 1 );

	m_spacingCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_spacingCtrl->SetToolTip( _("Minimum spacing between adjacent tuning segments. The resulting spacing may be greater based on design rules.") );

	gbSizer1->Add( m_spacingCtrl, wxGBPosition( 6, 1 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_spacingUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spacingUnits->Wrap( -1 );
	gbSizer1->Add( m_spacingUnits, wxGBPosition( 6, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_cornerLabel = new wxStaticText( this, wxID_ANY, _("Corner style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerLabel->Wrap( -1 );
	gbSizer1->Add( m_cornerLabel, wxGBPosition( 8, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 1 );

	wxString m_cornerCtrlChoices[] = { _("Chamfer"), _("Fillet") };
	int m_cornerCtrlNChoices = sizeof( m_cornerCtrlChoices ) / sizeof( wxString );
	m_cornerCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cornerCtrlNChoices, m_cornerCtrlChoices, 0 );
	m_cornerCtrl->SetSelection( 0 );
	gbSizer1->Add( m_cornerCtrl, wxGBPosition( 8, 1 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );

	m_rLabel = new wxStaticText( this, wxID_ANY, _("Radius (r):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rLabel->Wrap( -1 );
	gbSizer1->Add( m_rLabel, wxGBPosition( 8, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_rCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_rCtrl, wxGBPosition( 8, 5 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_rUnits = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rUnits->Wrap( -1 );
	gbSizer1->Add( m_rUnits, wxGBPosition( 8, 6 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_singleSided = new wxCheckBox( this, wxID_ANY, _("Single-sided"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_singleSided, wxGBPosition( 9, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->AddGrowableCol( 1 );
	gbSizer1->AddGrowableCol( 5 );

	singleTrackSizer->Add( gbSizer1, 1, wxEXPAND, 5 );


	bMainSizer->Add( singleTrackSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();

	bMainSizer->Add( m_stdButtons, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_radioBtnLength->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_TUNING_PATTERN_PROPERTIES_BASE::onRadioBtnTargetLengthClick ), NULL, this );
	m_radioBtnDelay->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_TUNING_PATTERN_PROPERTIES_BASE::onRadioBtnTargetDelayClick ), NULL, this );
	m_overrideCustomRules->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TUNING_PATTERN_PROPERTIES_BASE::onOverrideCustomRules ), NULL, this );
}

DIALOG_TUNING_PATTERN_PROPERTIES_BASE::~DIALOG_TUNING_PATTERN_PROPERTIES_BASE()
{
	// Disconnect Events
	m_radioBtnLength->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_TUNING_PATTERN_PROPERTIES_BASE::onRadioBtnTargetLengthClick ), NULL, this );
	m_radioBtnDelay->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_TUNING_PATTERN_PROPERTIES_BASE::onRadioBtnTargetDelayClick ), NULL, this );
	m_overrideCustomRules->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TUNING_PATTERN_PROPERTIES_BASE::onOverrideCustomRules ), NULL, this );

}
