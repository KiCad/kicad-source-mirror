///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 23 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_pcbnew_display_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PCBNEW_DISPLAY_OPTIONS_BASE::PANEL_PCBNEW_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxHORIZONTAL );

	m_galOptionsSizer = new wxBoxSizer( wxVERTICAL );


	bupperSizer->Add( m_galOptionsSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbAnnotations;
	sbAnnotations = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Annotations") ), wxVERTICAL );

	wxString m_ShowNetNamesOptionChoices[] = { _("Do not show"), _("Show on pads"), _("Show on tracks"), _("Show on pads and tracks") };
	int m_ShowNetNamesOptionNChoices = sizeof( m_ShowNetNamesOptionChoices ) / sizeof( wxString );
	m_ShowNetNamesOption = new wxRadioBox( sbAnnotations->GetStaticBox(), wxID_ANY, _("Net Names"), wxDefaultPosition, wxDefaultSize, m_ShowNetNamesOptionNChoices, m_ShowNetNamesOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_ShowNetNamesOption->SetSelection( 0 );
	m_ShowNetNamesOption->SetToolTip( _("Show or hide net names on pads and/or tracks.") );

	sbAnnotations->Add( m_ShowNetNamesOption, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_OptDisplayPadNumber = new wxCheckBox( sbAnnotations->GetStaticBox(), wxID_ANY, _("Show pad numbers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayPadNumber->SetValue(true);
	sbAnnotations->Add( m_OptDisplayPadNumber, 0, wxALL, 6 );

	m_OptDisplayPadNoConn = new wxCheckBox( sbAnnotations->GetStaticBox(), wxID_ANY, _("Show pad <no net> indicator"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayPadNoConn->SetValue(true);
	sbAnnotations->Add( m_OptDisplayPadNoConn, 0, wxBOTTOM|wxLEFT|wxRIGHT, 6 );


	bRightSizer->Add( sbAnnotations, 0, wxEXPAND|wxBOTTOM, 5 );

	wxStaticBoxSizer* sbClearance;
	sbClearance = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Clearance Outlines") ), wxVERTICAL );

	wxString m_OptDisplayTracksClearanceChoices[] = { _("Do not show"), _("Show when creating tracks"), _("Show with via clearance at end"), _("Show when creating and editing tracks"), _("Show always") };
	int m_OptDisplayTracksClearanceNChoices = sizeof( m_OptDisplayTracksClearanceChoices ) / sizeof( wxString );
	m_OptDisplayTracksClearance = new wxRadioBox( sbClearance->GetStaticBox(), ID_SHOW_CLEARANCE, _("Track Clearance"), wxDefaultPosition, wxDefaultSize, m_OptDisplayTracksClearanceNChoices, m_OptDisplayTracksClearanceChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayTracksClearance->SetSelection( 2 );
	m_OptDisplayTracksClearance->SetToolTip( _("Show or hide the track and via clearance area. If \"New track\" is selected, track clearance area is shown only when creating the track.") );

	sbClearance->Add( m_OptDisplayTracksClearance, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_OptDisplayPadClearence = new wxCheckBox( sbClearance->GetStaticBox(), wxID_ANY, _("Show pad clearance"), wxDefaultPosition, wxDefaultSize, 0 );
	sbClearance->Add( m_OptDisplayPadClearence, 0, wxALL, 6 );


	bRightSizer->Add( sbClearance, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	bupperSizer->Add( bRightSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bupperSizer, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_PCBNEW_DISPLAY_OPTIONS_BASE::~PANEL_PCBNEW_DISPLAY_OPTIONS_BASE()
{
}
