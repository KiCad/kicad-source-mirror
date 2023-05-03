///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_display_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_DISPLAY_OPTIONS_BASE::PANEL_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxHORIZONTAL );

	m_galOptionsSizer = new wxBoxSizer( wxVERTICAL );


	bupperSizer->Add( m_galOptionsSizer, 0, wxEXPAND|wxRIGHT, 10 );

	m_optionsBook = new wxSimplebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxPanel* emptyPage;
	emptyPage = new wxPanel( m_optionsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_optionsBook->AddPage( emptyPage, _("a page"), false );
	wxPanel* pcbPage;
	pcbPage = new wxPanel( m_optionsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* pcbOptionsSizer;
	pcbOptionsSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	m_annotationsLabel = new wxStaticText( pcbPage, wxID_ANY, _("Annotations"), wxDefaultPosition, wxDefaultSize, 0 );
	m_annotationsLabel->Wrap( -1 );
	bMargins->Add( m_annotationsLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline1 = new wxStaticLine( pcbPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMargins->Add( m_staticline1, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	wxString m_ShowNetNamesOptionChoices[] = { _("Do not show"), _("Show on pads"), _("Show on tracks"), _("Show on pads and tracks") };
	int m_ShowNetNamesOptionNChoices = sizeof( m_ShowNetNamesOptionChoices ) / sizeof( wxString );
	m_ShowNetNamesOption = new wxRadioBox( pcbPage, wxID_ANY, _("Net Names"), wxDefaultPosition, wxDefaultSize, m_ShowNetNamesOptionNChoices, m_ShowNetNamesOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_ShowNetNamesOption->SetSelection( 0 );
	m_ShowNetNamesOption->SetToolTip( _("Show or hide net names on pads and/or tracks.") );

	bSizer6->Add( m_ShowNetNamesOption, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_OptDisplayPadNumber = new wxCheckBox( pcbPage, wxID_ANY, _("Show pad numbers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayPadNumber->SetValue(true);
	bSizer6->Add( m_OptDisplayPadNumber, 0, wxALL, 6 );


	bMargins->Add( bSizer6, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bMargins->Add( 0, 10, 0, wxEXPAND, 5 );

	m_clearanceLabel = new wxStaticText( pcbPage, wxID_ANY, _("Clearance Outlines"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceLabel->Wrap( -1 );
	bMargins->Add( m_clearanceLabel, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline2 = new wxStaticLine( pcbPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMargins->Add( m_staticline2, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );

	wxString m_OptDisplayTracksClearanceChoices[] = { _("Do not show"), _("Show when routing"), _("Show when routing w/ via clearance at end"), _("Show when routing and editing"), _("Show always") };
	int m_OptDisplayTracksClearanceNChoices = sizeof( m_OptDisplayTracksClearanceChoices ) / sizeof( wxString );
	m_OptDisplayTracksClearance = new wxRadioBox( pcbPage, ID_SHOW_CLEARANCE, _("Track && Via Clearances"), wxDefaultPosition, wxDefaultSize, m_OptDisplayTracksClearanceNChoices, m_OptDisplayTracksClearanceChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayTracksClearance->SetSelection( 3 );
	m_OptDisplayTracksClearance->SetToolTip( _("Show clearance outlines around tracks, and optionally the via clearance around the end of the track while routing.") );

	bSizer7->Add( m_OptDisplayTracksClearance, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_OptDisplayPadClearence = new wxCheckBox( pcbPage, wxID_ANY, _("Show pad clearance"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_OptDisplayPadClearence, 0, wxALL, 6 );


	bMargins->Add( bSizer7, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bMargins->Add( 0, 10, 0, wxEXPAND, 5 );

	m_crossProbingLabel = new wxStaticText( pcbPage, wxID_ANY, _("Cross-probing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_crossProbingLabel->Wrap( -1 );
	bMargins->Add( m_crossProbingLabel, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline3 = new wxStaticLine( pcbPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMargins->Add( m_staticline3, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_checkCrossProbeOnSelection = new wxCheckBox( pcbPage, wxID_ANY, _("Select/highlight objects corresponding to schematic selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeOnSelection->SetValue(true);
	m_checkCrossProbeOnSelection->SetToolTip( _("Highlight footprints corresponding to selected symbols") );

	bSizer8->Add( m_checkCrossProbeOnSelection, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkCrossProbeCenter = new wxCheckBox( pcbPage, wxID_ANY, _("Center view on cross-probed items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeCenter->SetValue(true);
	m_checkCrossProbeCenter->SetToolTip( _("Ensures that cross-probed footprints are visible in the current view") );

	bSizer8->Add( m_checkCrossProbeCenter, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkCrossProbeZoom = new wxCheckBox( pcbPage, wxID_ANY, _("Zoom to fit cross-probed items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeZoom->SetValue(true);
	bSizer8->Add( m_checkCrossProbeZoom, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkCrossProbeAutoHighlight = new wxCheckBox( pcbPage, wxID_ANY, _("Highlight cross-probed nets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeAutoHighlight->SetValue(true);
	m_checkCrossProbeAutoHighlight->SetToolTip( _("Highlight nets when they are highlighted in the schematic editor") );

	bSizer8->Add( m_checkCrossProbeAutoHighlight, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_live3Drefresh = new wxCheckBox( pcbPage, wxID_ANY, _("Refresh 3D view automatically"), wxDefaultPosition, wxDefaultSize, 0 );
	m_live3Drefresh->SetToolTip( _("When enabled, edits to the board will cause the 3D view to refresh (may be slow with larger boards)") );

	bSizer8->Add( m_live3Drefresh, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bMargins->Add( bSizer8, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	pcbOptionsSizer->Add( bMargins, 1, wxEXPAND, 5 );


	pcbPage->SetSizer( pcbOptionsSizer );
	pcbPage->Layout();
	pcbOptionsSizer->Fit( pcbPage );
	m_optionsBook->AddPage( pcbPage, _("a page"), false );

	bupperSizer->Add( m_optionsBook, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bupperSizer, 0, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_DISPLAY_OPTIONS_BASE::~PANEL_DISPLAY_OPTIONS_BASE()
{
}
