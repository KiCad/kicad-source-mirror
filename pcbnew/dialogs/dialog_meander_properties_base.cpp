///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/text_ctrl_eval.h"

#include "dialog_meander_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MEANDER_PROPERTIES_BASE::DIALOG_MEANDER_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* singleTrackSizer;
	singleTrackSizer = new wxBoxSizer( wxHORIZONTAL );

	m_legend = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	singleTrackSizer->Add( m_legend, 0, wxEXPAND|wxRIGHT|wxLEFT, 15 );

	wxFlexGridSizer* fgSizer31;
	fgSizer31 = new wxFlexGridSizer( 0, 5, 5, 5 );
	fgSizer31->AddGrowableCol( 1 );
	fgSizer31->SetFlexibleDirection( wxBOTH );
	fgSizer31->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_track_minALabel = new wxStaticText( this, wxID_ANY, _("Amplitude (A) min:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_track_minALabel->Wrap( -1 );
	fgSizer31->Add( m_track_minALabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );

	m_minACtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_minACtrl, 1, 0, 5 );

	m_minAUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minAUnits->Wrap( -1 );
	bSizer8->Add( m_minAUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	fgSizer31->Add( bSizer8, 1, wxEXPAND, 5 );

	m_maxALabel = new wxStaticText( this, wxID_ANY, _("Max:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maxALabel->Wrap( -1 );
	fgSizer31->Add( m_maxALabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );

	m_maxACtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer31->Add( m_maxACtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_maxAUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maxAUnits->Wrap( -1 );
	fgSizer31->Add( m_maxAUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spacingLabel = new wxStaticText( this, wxID_ANY, _("Spacing (s):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spacingLabel->Wrap( -1 );
	fgSizer31->Add( m_spacingLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	m_spacingCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_spacingCtrl->SetToolTip( _("Minimum spacing between adjacent meander segments. The resulting spacing may be greater based on design rules.") );

	bSizer9->Add( m_spacingCtrl, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_spacingUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spacingUnits->Wrap( -1 );
	bSizer9->Add( m_spacingUnits, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer31->Add( bSizer9, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer31->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer31->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer31->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer31->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer31->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer31->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer31->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer31->Add( 0, 5, 1, wxEXPAND, 5 );

	m_cornerLabel = new wxStaticText( this, wxID_ANY, _("Corner style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerLabel->Wrap( -1 );
	fgSizer31->Add( m_cornerLabel, 1, wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_cornerCtrlChoices[] = { _("Chamfer"), _("Fillet") };
	int m_cornerCtrlNChoices = sizeof( m_cornerCtrlChoices ) / sizeof( wxString );
	m_cornerCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cornerCtrlNChoices, m_cornerCtrlChoices, 0 );
	m_cornerCtrl->SetSelection( 0 );
	fgSizer31->Add( m_cornerCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_rLabel = new wxStaticText( this, wxID_ANY, _("Radius (r):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rLabel->Wrap( -1 );
	fgSizer31->Add( m_rLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );

	m_rCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer31->Add( m_rCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_rUnits = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rUnits->Wrap( -1 );
	fgSizer31->Add( m_rUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer31->Add( 0, 0, 1, wxEXPAND, 5 );

	m_singleSided = new wxCheckBox( this, wxID_ANY, _("Single-sided"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer31->Add( m_singleSided, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer31->Add( 0, 0, 1, wxEXPAND, 5 );


	singleTrackSizer->Add( fgSizer31, 0, wxEXPAND|wxLEFT, 5 );


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
}

DIALOG_MEANDER_PROPERTIES_BASE::~DIALOG_MEANDER_PROPERTIES_BASE()
{
}
