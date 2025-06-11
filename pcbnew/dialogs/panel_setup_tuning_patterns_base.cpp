///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/text_ctrl_eval.h"

#include "panel_setup_tuning_patterns_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_TUNING_PATTERNS_BASE::PANEL_SETUP_TUNING_PATTERNS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_singleTrackLabel = new wxStaticText( this, wxID_ANY, _("Default Properties for Single Track Tuning"), wxDefaultPosition, wxDefaultSize, 0 );
	m_singleTrackLabel->Wrap( -1 );
	bMainSizer->Add( m_singleTrackLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* singleTrackSizer;
	singleTrackSizer = new wxBoxSizer( wxHORIZONTAL );

	m_singleTrackLegend = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	singleTrackSizer->Add( m_singleTrackLegend, 0, wxEXPAND|wxRIGHT|wxLEFT, 15 );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 5, 5, 5 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_track_minALabel = new wxStaticText( this, wxID_ANY, _("Minimum amplitude (A):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_track_minALabel->Wrap( -1 );
	fgSizer3->Add( m_track_minALabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );

	m_track_minACtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_track_minACtrl, 1, 0, 5 );

	m_track_minAUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_track_minAUnits->Wrap( -1 );
	bSizer8->Add( m_track_minAUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	fgSizer3->Add( bSizer8, 1, wxEXPAND, 5 );

	m_track_maxALabel = new wxStaticText( this, wxID_ANY, _("Maximum amplitude (A):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_track_maxALabel->Wrap( -1 );
	fgSizer3->Add( m_track_maxALabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );

	m_track_maxACtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_track_maxACtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_track_maxAUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_track_maxAUnits->Wrap( -1 );
	fgSizer3->Add( m_track_maxAUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_track_spacingLabel = new wxStaticText( this, wxID_ANY, _("Spacing (s):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_track_spacingLabel->Wrap( -1 );
	fgSizer3->Add( m_track_spacingLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	m_track_spacingCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_track_spacingCtrl->SetToolTip( _("Minimum spacing between adjacent tuning segments. The resulting spacing may be greater based on design rules.") );

	bSizer9->Add( m_track_spacingCtrl, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_track_spacingUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_track_spacingUnits->Wrap( -1 );
	bSizer9->Add( m_track_spacingUnits, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer3->Add( bSizer9, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer3->Add( 0, 5, 1, wxEXPAND, 5 );

	m_track_cornerLabel = new wxStaticText( this, wxID_ANY, _("Corner style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_track_cornerLabel->Wrap( -1 );
	fgSizer3->Add( m_track_cornerLabel, 1, wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_track_cornerCtrlChoices[] = { _("Chamfer"), _("Fillet") };
	int m_track_cornerCtrlNChoices = sizeof( m_track_cornerCtrlChoices ) / sizeof( wxString );
	m_track_cornerCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_track_cornerCtrlNChoices, m_track_cornerCtrlChoices, 0 );
	m_track_cornerCtrl->SetSelection( 0 );
	fgSizer3->Add( m_track_cornerCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_track_rLabel = new wxStaticText( this, wxID_ANY, _("Radius (r):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_track_rLabel->Wrap( -1 );
	fgSizer3->Add( m_track_rLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );

	m_track_rCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_track_rCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_track_rUnits = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_track_rUnits->Wrap( -1 );
	fgSizer3->Add( m_track_rUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );

	m_track_singleSided = new wxCheckBox( this, wxID_ANY, _("Single-sided"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_track_singleSided, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );


	singleTrackSizer->Add( fgSizer3, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( singleTrackSizer, 0, wxEXPAND|wxTOP|wxRIGHT, 5 );


	bMainSizer->Add( 0, 10, 0, wxEXPAND, 5 );

	m_diffPairsLabel = new wxStaticText( this, wxID_ANY, _("Default Properties for Differential Pairs"), wxDefaultPosition, wxDefaultSize, 0 );
	m_diffPairsLabel->Wrap( -1 );
	bMainSizer->Add( m_diffPairsLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline11 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline11, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* diffPairSizer;
	diffPairSizer = new wxBoxSizer( wxHORIZONTAL );

	m_diffPairLegend = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	diffPairSizer->Add( m_diffPairLegend, 0, wxEXPAND|wxRIGHT|wxLEFT, 15 );

	wxFlexGridSizer* fgSizer32;
	fgSizer32 = new wxFlexGridSizer( 0, 5, 5, 5 );
	fgSizer32->AddGrowableCol( 1 );
	fgSizer32->SetFlexibleDirection( wxBOTH );
	fgSizer32->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_dp_minALabel = new wxStaticText( this, wxID_ANY, _("Minimum amplitude (A):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dp_minALabel->Wrap( -1 );
	fgSizer32->Add( m_dp_minALabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer81;
	bSizer81 = new wxBoxSizer( wxHORIZONTAL );

	m_dp_minACtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer81->Add( m_dp_minACtrl, 1, 0, 5 );

	m_dp_minAUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dp_minAUnits->Wrap( -1 );
	bSizer81->Add( m_dp_minAUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	fgSizer32->Add( bSizer81, 1, wxEXPAND, 5 );

	m_dp_maxALabel = new wxStaticText( this, wxID_ANY, _("Maximum amplitude (A):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dp_maxALabel->Wrap( -1 );
	fgSizer32->Add( m_dp_maxALabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );

	m_dp_maxACtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer32->Add( m_dp_maxACtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_dp_maxAUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dp_maxAUnits->Wrap( -1 );
	fgSizer32->Add( m_dp_maxAUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_dp_spacingLabel = new wxStaticText( this, wxID_ANY, _("Spacing (s):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dp_spacingLabel->Wrap( -1 );
	fgSizer32->Add( m_dp_spacingLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer91;
	bSizer91 = new wxBoxSizer( wxHORIZONTAL );

	m_dp_spacingCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_dp_spacingCtrl->SetToolTip( _("Minimum spacing between adjacent tuning segments. The resulting spacing may be greater based on design rules.") );

	bSizer91->Add( m_dp_spacingCtrl, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_dp_spacingUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dp_spacingUnits->Wrap( -1 );
	bSizer91->Add( m_dp_spacingUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	fgSizer32->Add( bSizer91, 1, wxEXPAND, 5 );


	fgSizer32->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer32->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer32->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer32->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer32->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer32->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer32->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer32->Add( 0, 5, 1, wxEXPAND, 5 );

	m_dp_cornerLabel = new wxStaticText( this, wxID_ANY, _("Corner style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dp_cornerLabel->Wrap( -1 );
	fgSizer32->Add( m_dp_cornerLabel, 1, wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_dp_cornerCtrlChoices[] = { _("Chamfer"), _("Fillet") };
	int m_dp_cornerCtrlNChoices = sizeof( m_dp_cornerCtrlChoices ) / sizeof( wxString );
	m_dp_cornerCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dp_cornerCtrlNChoices, m_dp_cornerCtrlChoices, 0 );
	m_dp_cornerCtrl->SetSelection( 0 );
	fgSizer32->Add( m_dp_cornerCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_dp_rLabel = new wxStaticText( this, wxID_ANY, _("Radius (r):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dp_rLabel->Wrap( -1 );
	fgSizer32->Add( m_dp_rLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );

	m_dp_rCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer32->Add( m_dp_rCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_dp_rUnits = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dp_rUnits->Wrap( -1 );
	fgSizer32->Add( m_dp_rUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer32->Add( 0, 0, 1, wxEXPAND, 5 );

	m_dp_singleSided = new wxCheckBox( this, wxID_ANY, _("Single-sided"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer32->Add( m_dp_singleSided, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer32->Add( 0, 0, 1, wxEXPAND, 5 );


	diffPairSizer->Add( fgSizer32, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( diffPairSizer, 0, wxEXPAND|wxTOP|wxRIGHT, 5 );


	bMainSizer->Add( 0, 10, 0, wxEXPAND, 5 );

	m_diffPairsLabel1 = new wxStaticText( this, wxID_ANY, _("Default Properties for Differential Pair Skews"), wxDefaultPosition, wxDefaultSize, 0 );
	m_diffPairsLabel1->Wrap( -1 );
	bMainSizer->Add( m_diffPairsLabel1, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline111 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline111, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* skewSizer;
	skewSizer = new wxBoxSizer( wxHORIZONTAL );

	m_skewLegend = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	skewSizer->Add( m_skewLegend, 0, wxEXPAND|wxRIGHT|wxLEFT, 15 );

	wxFlexGridSizer* fgSizer31;
	fgSizer31 = new wxFlexGridSizer( 0, 5, 5, 5 );
	fgSizer31->AddGrowableCol( 1 );
	fgSizer31->SetFlexibleDirection( wxBOTH );
	fgSizer31->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_skew_minALabel = new wxStaticText( this, wxID_ANY, _("Minimum amplitude (A):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_skew_minALabel->Wrap( -1 );
	fgSizer31->Add( m_skew_minALabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer82;
	bSizer82 = new wxBoxSizer( wxHORIZONTAL );

	m_skew_minACtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer82->Add( m_skew_minACtrl, 1, 0, 5 );

	m_skew_minAUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_skew_minAUnits->Wrap( -1 );
	bSizer82->Add( m_skew_minAUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	fgSizer31->Add( bSizer82, 1, wxEXPAND, 5 );

	m_skew_maxALabel = new wxStaticText( this, wxID_ANY, _("Maximum amplitude (A):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_skew_maxALabel->Wrap( -1 );
	fgSizer31->Add( m_skew_maxALabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );

	m_skew_maxACtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer31->Add( m_skew_maxACtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_skew_maxAUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_skew_maxAUnits->Wrap( -1 );
	fgSizer31->Add( m_skew_maxAUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_skew_spacingLabel = new wxStaticText( this, wxID_ANY, _("Spacing (s):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_skew_spacingLabel->Wrap( -1 );
	fgSizer31->Add( m_skew_spacingLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer92;
	bSizer92 = new wxBoxSizer( wxHORIZONTAL );

	m_skew_spacingCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_skew_spacingCtrl->SetToolTip( _("Minimum spacing between adjacent tuning segments. The resulting spacing may be greater based on design rules.") );

	bSizer92->Add( m_skew_spacingCtrl, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_skew_spacingUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_skew_spacingUnits->Wrap( -1 );
	bSizer92->Add( m_skew_spacingUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	fgSizer31->Add( bSizer92, 1, wxEXPAND, 5 );


	fgSizer31->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer31->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer31->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer31->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer31->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer31->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer31->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer31->Add( 0, 5, 1, wxEXPAND, 5 );

	m_skew_cornerLabel = new wxStaticText( this, wxID_ANY, _("Corner style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_skew_cornerLabel->Wrap( -1 );
	fgSizer31->Add( m_skew_cornerLabel, 1, wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_skew_cornerCtrlChoices[] = { _("Chamfer"), _("Fillet") };
	int m_skew_cornerCtrlNChoices = sizeof( m_skew_cornerCtrlChoices ) / sizeof( wxString );
	m_skew_cornerCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_skew_cornerCtrlNChoices, m_skew_cornerCtrlChoices, 0 );
	m_skew_cornerCtrl->SetSelection( 0 );
	fgSizer31->Add( m_skew_cornerCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_skew_rLabel = new wxStaticText( this, wxID_ANY, _("Radius (r):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_skew_rLabel->Wrap( -1 );
	fgSizer31->Add( m_skew_rLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );

	m_skew_rCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer31->Add( m_skew_rCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_skew_rUnits = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_skew_rUnits->Wrap( -1 );
	fgSizer31->Add( m_skew_rUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer31->Add( 0, 0, 1, wxEXPAND, 5 );

	m_skew_singleSided = new wxCheckBox( this, wxID_ANY, _("Single-sided"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer31->Add( m_skew_singleSided, 0, wxALL, 5 );


	fgSizer31->Add( 0, 0, 1, wxEXPAND, 5 );


	skewSizer->Add( fgSizer31, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( skewSizer, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_SETUP_TUNING_PATTERNS_BASE::~PANEL_SETUP_TUNING_PATTERNS_BASE()
{
}
