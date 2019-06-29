///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 23 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/text_ctrl_eval.h"

#include "dialog_pns_length_tuning_settings_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE::DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerUpper;
	sbSizerUpper = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Length / Skew") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerLenSkew;
	fgSizerLenSkew = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerLenSkew->AddGrowableCol( 1 );
	fgSizerLenSkew->SetFlexibleDirection( wxBOTH );
	fgSizerLenSkew->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText4 = new wxStaticText( sbSizerUpper->GetStaticBox(), wxID_ANY, _("Tune from:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	fgSizerLenSkew->Add( m_staticText4, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_choicePathFromChoices;
	m_choicePathFrom = new wxChoice( sbSizerUpper->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choicePathFromChoices, 0 );
	m_choicePathFrom->SetSelection( 0 );
	fgSizerLenSkew->Add( m_choicePathFrom, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	fgSizerLenSkew->Add( 0, 0, 0, 0, 5 );

	m_staticText15 = new wxStaticText( sbSizerUpper->GetStaticBox(), wxID_ANY, _("Tune to:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	fgSizerLenSkew->Add( m_staticText15, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_choice4Choices;
	m_choice4 = new wxChoice( sbSizerUpper->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choice4Choices, 0 );
	m_choice4->SetSelection( 0 );
	fgSizerLenSkew->Add( m_choice4, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	fgSizerLenSkew->Add( 0, 0, 0, 0, 5 );

	m_staticText3 = new wxStaticText( sbSizerUpper->GetStaticBox(), wxID_ANY, _("Constraint:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	fgSizerLenSkew->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxString m_constraintSourceChoices[] = { _("From Design Rules"), _("Manual") };
	int m_constraintSourceNChoices = sizeof( m_constraintSourceChoices ) / sizeof( wxString );
	m_constraintSource = new wxChoice( sbSizerUpper->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_constraintSourceNChoices, m_constraintSourceChoices, 0 );
	m_constraintSource->SetSelection( 1 );
	m_constraintSource->Enable( false );

	fgSizerLenSkew->Add( m_constraintSource, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	fgSizerLenSkew->Add( 0, 0, 0, 0, 5 );

	m_targetLengthLabel = new wxStaticText( sbSizerUpper->GetStaticBox(), wxID_ANY, _("Target length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_targetLengthLabel->Wrap( -1 );
	fgSizerLenSkew->Add( m_targetLengthLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_targetLengthText = new wxTextCtrl( sbSizerUpper->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerLenSkew->Add( m_targetLengthText, 0, wxALL|wxEXPAND, 5 );

	m_targetLengthUnit = new wxStaticText( sbSizerUpper->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_targetLengthUnit->Wrap( -1 );
	fgSizerLenSkew->Add( m_targetLengthUnit, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	sbSizerUpper->Add( fgSizerLenSkew, 1, wxEXPAND, 5 );


	bMainSizer->Add( sbSizerUpper, 0, wxEXPAND|wxALL, 10 );

	wxStaticBoxSizer* sbSizerLower;
	sbSizerLower = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Meandering") ), wxHORIZONTAL );

	m_legend = new wxStaticBitmap( sbSizerLower->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLower->Add( m_legend, 0, wxALL|wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_minAmplLabel = new wxStaticText( sbSizerLower->GetStaticBox(), wxID_ANY, _("Min amplitude (Amin):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minAmplLabel->Wrap( -1 );
	fgSizer3->Add( m_minAmplLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_minAmplText = new wxTextCtrl( sbSizerLower->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_minAmplText, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_minAmplUnit = new wxStaticText( sbSizerLower->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minAmplUnit->Wrap( -1 );
	fgSizer3->Add( m_minAmplUnit, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );

	m_maxAmplLabel = new wxStaticText( sbSizerLower->GetStaticBox(), wxID_ANY, _("Max amplitude (Amax):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maxAmplLabel->Wrap( -1 );
	fgSizer3->Add( m_maxAmplLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_maxAmplText = new wxTextCtrl( sbSizerLower->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_maxAmplText, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_maxAmplUnit = new wxStaticText( sbSizerLower->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maxAmplUnit->Wrap( -1 );
	fgSizer3->Add( m_maxAmplUnit, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );

	m_spacingLabel = new wxStaticText( sbSizerLower->GetStaticBox(), wxID_ANY, _("Spacing (s):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spacingLabel->Wrap( -1 );
	fgSizer3->Add( m_spacingLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_spacingText = new wxTextCtrl( sbSizerLower->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_spacingText, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_spacingUnit = new wxStaticText( sbSizerLower->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spacingUnit->Wrap( -1 );
	fgSizer3->Add( m_spacingUnit, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );

	m_staticText14 = new wxStaticText( sbSizerLower->GetStaticBox(), wxID_ANY, _("Miter style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	fgSizer3->Add( m_staticText14, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxString m_miterStyleChoices[] = { _("45 degree"), _("arc") };
	int m_miterStyleNChoices = sizeof( m_miterStyleChoices ) / sizeof( wxString );
	m_miterStyle = new wxChoice( sbSizerLower->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_miterStyleNChoices, m_miterStyleChoices, 0 );
	m_miterStyle->SetSelection( 0 );
	fgSizer3->Add( m_miterStyle, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );

	m_radiusLabel = new wxStaticText( sbSizerLower->GetStaticBox(), wxID_ANY, _("Miter radius (r):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radiusLabel->Wrap( -1 );
	fgSizer3->Add( m_radiusLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_radiusText = new TEXT_CTRL_EVAL( sbSizerLower->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_radiusText, 0, wxALL|wxEXPAND, 5 );

	m_radiusUnit = new wxStaticText( sbSizerLower->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radiusUnit->Wrap( -1 );
	fgSizer3->Add( m_radiusUnit, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	sbSizerLower->Add( fgSizer3, 1, wxEXPAND, 5 );


	bMainSizer->Add( sbSizerLower, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );

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
}

DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE::~DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE()
{
}
