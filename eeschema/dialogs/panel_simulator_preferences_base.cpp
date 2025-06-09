///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_simulator_preferences_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SIMULATOR_PREFERENCES_BASE::PANEL_SIMULATOR_PREFERENCES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bScrollSizer;
	bScrollSizer = new wxBoxSizer( wxVERTICAL );

	m_lblScrollHeading = new wxStaticText( this, wxID_ANY, _("Scroll Gestures"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblScrollHeading->Wrap( -1 );
	bScrollSizer->Add( m_lblScrollHeading, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 13 );

	m_scrollLine = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bScrollSizer->Add( m_scrollLine, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bScrollMargins;
	bScrollMargins = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bScrollSizerLeft;
	bScrollSizerLeft = new wxBoxSizer( wxVERTICAL );

	m_lblVScrollMovement = new wxStaticText( this, wxID_ANY, _("Vertical Touchpad or Scroll Wheel Movement"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblVScrollMovement->Wrap( -1 );
	bScrollSizerLeft->Add( m_lblVScrollMovement, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bScrollSizerLeft->Add( 0, 5, 0, wxEXPAND, 5 );

	wxFlexGridSizer* fgVScroll;
	fgVScroll = new wxFlexGridSizer( 0, 2, 5, 0 );
	fgVScroll->AddGrowableCol( 0 );
	fgVScroll->SetFlexibleDirection( wxBOTH );
	fgVScroll->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lblVScrollModifier = new wxStaticText( this, wxID_ANY, _("Modifier"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblVScrollModifier->Wrap( -1 );
	fgVScroll->Add( m_lblVScrollModifier, 0, wxALIGN_BOTTOM, 5 );

	m_lblVScrollAction = new wxStaticText( this, wxID_ANY, _("Action"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblVScrollAction->Wrap( -1 );
	fgVScroll->Add( m_lblVScrollAction, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_BOTTOM, 5 );

	m_lblVScrollUnmodified = new wxStaticText( this, wxID_ANY, _("None:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblVScrollUnmodified->Wrap( -1 );
	fgVScroll->Add( m_lblVScrollUnmodified, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceVScrollUnmodifiedChoices;
	m_choiceVScrollUnmodified = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceVScrollUnmodifiedChoices, 0 );
	m_choiceVScrollUnmodified->SetSelection( 0 );
	fgVScroll->Add( m_choiceVScrollUnmodified, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_lblVScrollCtrl = new wxStaticText( this, wxID_ANY, _("Ctrl:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblVScrollCtrl->Wrap( -1 );
	fgVScroll->Add( m_lblVScrollCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceVScrollCtrlChoices;
	m_choiceVScrollCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceVScrollCtrlChoices, 0 );
	m_choiceVScrollCtrl->SetSelection( 0 );
	fgVScroll->Add( m_choiceVScrollCtrl, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_lblVScrollShift = new wxStaticText( this, wxID_ANY, _("Shift:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblVScrollShift->Wrap( -1 );
	fgVScroll->Add( m_lblVScrollShift, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceVScrollShiftChoices;
	m_choiceVScrollShift = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceVScrollShiftChoices, 0 );
	m_choiceVScrollShift->SetSelection( 0 );
	fgVScroll->Add( m_choiceVScrollShift, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_lblVScrollAlt = new wxStaticText( this, wxID_ANY, _("Alt:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblVScrollAlt->Wrap( -1 );
	fgVScroll->Add( m_lblVScrollAlt, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceVScrollAltChoices;
	m_choiceVScrollAlt = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceVScrollAltChoices, 0 );
	m_choiceVScrollAlt->SetSelection( 0 );
	fgVScroll->Add( m_choiceVScrollAlt, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bScrollSizerLeft->Add( fgVScroll, 0, wxRIGHT|wxLEFT, 24 );


	bScrollSizerLeft->Add( 0, 20, 0, wxEXPAND, 5 );

	m_lblHScrollMovement = new wxStaticText( this, wxID_ANY, _("Horizontal Touchpad Movement"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblHScrollMovement->Wrap( -1 );
	bScrollSizerLeft->Add( m_lblHScrollMovement, 0, wxALL, 5 );


	bScrollSizerLeft->Add( 0, 5, 0, wxEXPAND, 5 );

	wxFlexGridSizer* fgHScroll;
	fgHScroll = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgHScroll->AddGrowableCol( 0 );
	fgHScroll->SetFlexibleDirection( wxBOTH );
	fgHScroll->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lblHScrollModifier = new wxStaticText( this, wxID_ANY, _("Modifier"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblHScrollModifier->Wrap( -1 );
	fgHScroll->Add( m_lblHScrollModifier, 0, wxALIGN_BOTTOM, 5 );

	m_lblHScrollAction = new wxStaticText( this, wxID_ANY, _("Action"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblHScrollAction->Wrap( -1 );
	fgHScroll->Add( m_lblHScrollAction, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_BOTTOM, 5 );

	m_lblHScrollAny = new wxStaticText( this, wxID_ANY, _("Any:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblHScrollAny->Wrap( -1 );
	fgHScroll->Add( m_lblHScrollAny, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceHScrollChoices;
	m_choiceHScroll = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceHScrollChoices, 0 );
	m_choiceHScroll->SetSelection( 0 );
	fgHScroll->Add( m_choiceHScroll, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bScrollSizerLeft->Add( fgHScroll, 0, wxRIGHT|wxLEFT, 24 );


	bScrollMargins->Add( bScrollSizerLeft, 1, wxEXPAND|wxLEFT, 5 );

	wxBoxSizer* bScrollSizerRight;
	bScrollSizerRight = new wxBoxSizer( wxVERTICAL );

	m_btnMouseDefaults = new wxButton( this, wxID_ANY, _("Reset to Mouse Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bScrollSizerRight->Add( m_btnMouseDefaults, 0, wxEXPAND|wxBOTTOM|wxLEFT, 5 );

	m_btnTrackpadDefaults = new wxButton( this, wxID_ANY, _("Reset to Trackpad Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bScrollSizerRight->Add( m_btnTrackpadDefaults, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );


	bScrollMargins->Add( bScrollSizerRight, 0, wxEXPAND|wxLEFT, 50 );


	bScrollSizer->Add( bScrollMargins, 1, wxEXPAND|wxTOP|wxRIGHT, 10 );


	bMainSizer->Add( bScrollSizer, 1, wxEXPAND|wxRIGHT, 10 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_btnMouseDefaults->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SIMULATOR_PREFERENCES_BASE::onMouseDefaults ), NULL, this );
	m_btnTrackpadDefaults->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SIMULATOR_PREFERENCES_BASE::onTrackpadDefaults ), NULL, this );
}

PANEL_SIMULATOR_PREFERENCES_BASE::~PANEL_SIMULATOR_PREFERENCES_BASE()
{
	// Disconnect Events
	m_btnMouseDefaults->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SIMULATOR_PREFERENCES_BASE::onMouseDefaults ), NULL, this );
	m_btnTrackpadDefaults->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SIMULATOR_PREFERENCES_BASE::onTrackpadDefaults ), NULL, this );

}
