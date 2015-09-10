///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_display_options_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DISPLAY_OPTIONS_BASE::DIALOG_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sLeftBoxSizer;
	sLeftBoxSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Tracks and Vias:") ), wxVERTICAL );
	
	m_OptDisplayTracks = new wxCheckBox( this, wxID_ANY, _("Show tracks in sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sLeftBoxSizer->Add( m_OptDisplayTracks, 0, wxALL, 5 );
	
	m_OptDisplayVias = new wxCheckBox( this, wxID_ANY, _("Show vias in sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sLeftBoxSizer->Add( m_OptDisplayVias, 0, wxALL, 5 );
	
	wxString m_OptDisplayViaHoleChoices[] = { _("Never"), _("Defined holes"), _("Always") };
	int m_OptDisplayViaHoleNChoices = sizeof( m_OptDisplayViaHoleChoices ) / sizeof( wxString );
	m_OptDisplayViaHole = new wxRadioBox( this, ID_VIAS_HOLES, _("Show Via Holes:"), wxDefaultPosition, wxDefaultSize, m_OptDisplayViaHoleNChoices, m_OptDisplayViaHoleChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayViaHole->SetSelection( 0 );
	m_OptDisplayViaHole->SetToolTip( _("Show or hide via holes.\nIf Defined Holes is selected, only the non default size holes are shown") );
	
	sLeftBoxSizer->Add( m_OptDisplayViaHole, 1, wxALL|wxEXPAND, 5 );
	
	
	bupperSizer->Add( sLeftBoxSizer, 0, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* sbMiddleLeftSizer;
	sbMiddleLeftSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Routing Help:") ), wxVERTICAL );
	
	wxString m_ShowNetNamesOptionChoices[] = { _("Do not show"), _("On pads"), _("On tracks"), _("On pads and tracks") };
	int m_ShowNetNamesOptionNChoices = sizeof( m_ShowNetNamesOptionChoices ) / sizeof( wxString );
	m_ShowNetNamesOption = new wxRadioBox( this, wxID_ANY, _("Show Net Names:"), wxDefaultPosition, wxDefaultSize, m_ShowNetNamesOptionNChoices, m_ShowNetNamesOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_ShowNetNamesOption->SetSelection( 0 );
	m_ShowNetNamesOption->SetToolTip( _("Show or hide net names on pads and/or tracks") );
	
	sbMiddleLeftSizer->Add( m_ShowNetNamesOption, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_OptDisplayTracksClearanceChoices[] = { _("Never"), _("New track"), _("New track with via area"), _("New and edited tracks with via area"), _("Always") };
	int m_OptDisplayTracksClearanceNChoices = sizeof( m_OptDisplayTracksClearanceChoices ) / sizeof( wxString );
	m_OptDisplayTracksClearance = new wxRadioBox( this, ID_SHOW_CLEARANCE, _("Show Track Clearance:"), wxDefaultPosition, wxDefaultSize, m_OptDisplayTracksClearanceNChoices, m_OptDisplayTracksClearanceChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayTracksClearance->SetSelection( 0 );
	m_OptDisplayTracksClearance->SetToolTip( _("Show or hide the track and via clearance area.\nIf New track is selected,  track clearance area is shown only when creating the track.") );
	
	sbMiddleLeftSizer->Add( m_OptDisplayTracksClearance, 1, wxALL|wxEXPAND, 5 );
	
	
	bupperSizer->Add( sbMiddleLeftSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* b_rightSizer;
	b_rightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sfootprintSizer;
	sfootprintSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Footprints:") ), wxVERTICAL );
	
	m_OptDisplayModOutlines = new wxCheckBox( this, wxID_ANY, _("Show outlines in sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sfootprintSizer->Add( m_OptDisplayModOutlines, 0, wxALL, 5 );
	
	m_OptDisplayModTexts
	= new wxCheckBox( this, wxID_ANY, _("Show text in sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sfootprintSizer->Add( m_OptDisplayModTexts
	, 0, wxALL, 5 );
	
	m_OptDisplayPads = new wxCheckBox( this, wxID_ANY, _("Show pads in sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sfootprintSizer->Add( m_OptDisplayPads, 0, wxALL, 5 );
	
	m_OptDisplayPadClearence = new wxCheckBox( this, wxID_ANY, _("Show pad clearance"), wxDefaultPosition, wxDefaultSize, 0 );
	sfootprintSizer->Add( m_OptDisplayPadClearence, 0, wxALL, 5 );
	
	m_OptDisplayPadNumber = new wxCheckBox( this, wxID_ANY, _("Show pad number"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayPadNumber->SetValue(true); 
	sfootprintSizer->Add( m_OptDisplayPadNumber, 0, wxALL, 5 );
	
	m_OptDisplayPadNoConn = new wxCheckBox( this, wxID_ANY, _("Show pad no net connection indicator"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayPadNoConn->SetValue(true); 
	sfootprintSizer->Add( m_OptDisplayPadNoConn, 0, wxALL, 5 );
	
	
	b_rightSizer->Add( sfootprintSizer, 0, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* s_otherSizer;
	s_otherSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Other:") ), wxVERTICAL );
	
	m_OptDisplayDrawings = new wxCheckBox( this, wxID_ANY, _("Show graphic items in sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayDrawings->SetValue(true); 
	s_otherSizer->Add( m_OptDisplayDrawings, 0, wxALL, 5 );
	
	m_Show_Page_Limits = new wxCheckBox( this, wxID_ANY, _("Show page limits"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Show_Page_Limits->SetValue(true); 
	s_otherSizer->Add( m_Show_Page_Limits, 0, wxALL, 5 );
	
	
	b_rightSizer->Add( s_otherSizer, 1, wxEXPAND|wxALL, 5 );
	
	
	bupperSizer->Add( b_rightSizer, 0, 0, 5 );
	
	
	bMainSizer->Add( bupperSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bMainSizer->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	// Connect Events
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnOkClick ), NULL, this );
}

DIALOG_DISPLAY_OPTIONS_BASE::~DIALOG_DISPLAY_OPTIONS_BASE()
{
	// Disconnect Events
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnOkClick ), NULL, this );
	
}
