///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_display_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_DISPLAY_OPTIONS_BASE::PANEL_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxHORIZONTAL );

	m_galOptionsSizer = new wxBoxSizer( wxVERTICAL );


	bupperSizer->Add( m_galOptionsSizer, 1, wxEXPAND, 5 );

	m_optionsBook = new wxSimplebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxPanel* emptyPage;
	emptyPage = new wxPanel( m_optionsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_optionsBook->AddPage( emptyPage, _("a page"), false );
	wxPanel* pcbPage;
	pcbPage = new wxPanel( m_optionsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* pcbOptionsSizer;
	pcbOptionsSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbAnnotations;
	sbAnnotations = new wxStaticBoxSizer( new wxStaticBox( pcbPage, wxID_ANY, _("Annotations") ), wxVERTICAL );

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


	pcbOptionsSizer->Add( sbAnnotations, 0, wxEXPAND|wxBOTTOM, 5 );

	wxStaticBoxSizer* sbClearance;
	sbClearance = new wxStaticBoxSizer( new wxStaticBox( pcbPage, wxID_ANY, _("Clearance Outlines") ), wxVERTICAL );

	wxString m_OptDisplayTracksClearanceChoices[] = { _("Do not show"), _("Show when routing"), _("Show when routing w/ via clearance at end"), _("Show when routing and editing"), _("Show always") };
	int m_OptDisplayTracksClearanceNChoices = sizeof( m_OptDisplayTracksClearanceChoices ) / sizeof( wxString );
	m_OptDisplayTracksClearance = new wxRadioBox( sbClearance->GetStaticBox(), ID_SHOW_CLEARANCE, _("Track && Via Clearances"), wxDefaultPosition, wxDefaultSize, m_OptDisplayTracksClearanceNChoices, m_OptDisplayTracksClearanceChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayTracksClearance->SetSelection( 3 );
	m_OptDisplayTracksClearance->SetToolTip( _("Show or hide the track and via clearance area. If \"New track\" is selected, track clearance area is shown only when creating the track.") );

	sbClearance->Add( m_OptDisplayTracksClearance, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_OptDisplayPadClearence = new wxCheckBox( sbClearance->GetStaticBox(), wxID_ANY, _("Show pad clearance"), wxDefaultPosition, wxDefaultSize, 0 );
	sbClearance->Add( m_OptDisplayPadClearence, 0, wxALL, 6 );


	pcbOptionsSizer->Add( sbClearance, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );

	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( pcbPage, wxID_ANY, _("Cross-probing") ), wxVERTICAL );

	m_checkCrossProbeCenter = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Center view on cross-probed items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeCenter->SetValue(true);
	sbSizer3->Add( m_checkCrossProbeCenter, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkCrossProbeZoom = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Zoom to fit cross-probed items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeZoom->SetValue(true);
	sbSizer3->Add( m_checkCrossProbeZoom, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkCrossProbeAutoHighlight = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Highlight cross-probed nets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeAutoHighlight->SetValue(true);
	m_checkCrossProbeAutoHighlight->SetToolTip( _("Highlight nets when they are highlighted in the schematic editor") );

	sbSizer3->Add( m_checkCrossProbeAutoHighlight, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_live3Drefresh = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Refresh 3D view automatically"), wxDefaultPosition, wxDefaultSize, 0 );
	m_live3Drefresh->SetToolTip( _("When enabled, edits to the board will cause the 3D view to refresh (may be slow with larger boards)") );

	sbSizer3->Add( m_live3Drefresh, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	pcbOptionsSizer->Add( sbSizer3, 1, wxEXPAND|wxTOP, 5 );


	pcbPage->SetSizer( pcbOptionsSizer );
	pcbPage->Layout();
	pcbOptionsSizer->Fit( pcbPage );
	m_optionsBook->AddPage( pcbPage, _("a page"), false );

	bupperSizer->Add( m_optionsBook, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bupperSizer, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_DISPLAY_OPTIONS_BASE::~PANEL_DISPLAY_OPTIONS_BASE()
{
}
