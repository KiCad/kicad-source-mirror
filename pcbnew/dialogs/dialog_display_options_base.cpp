///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  6 2014)
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
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sLeftBoxSizer;
	sLeftBoxSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Tracks and vias:") ), wxVERTICAL );
	
	wxString m_OptDisplayTracksChoices[] = { _("Sketch"), _("Filled") };
	int m_OptDisplayTracksNChoices = sizeof( m_OptDisplayTracksChoices ) / sizeof( wxString );
	m_OptDisplayTracks = new wxRadioBox( this, wxID_DISPLAY_TRACK, _("Tracks:"), wxDefaultPosition, wxDefaultSize, m_OptDisplayTracksNChoices, m_OptDisplayTracksChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayTracks->SetSelection( 1 );
	m_OptDisplayTracks->SetToolTip( _("Select how tracks are displayed") );
	
	sLeftBoxSizer->Add( m_OptDisplayTracks, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_OptDisplayViasChoices[] = { _("Sketch"), _("Filled") };
	int m_OptDisplayViasNChoices = sizeof( m_OptDisplayViasChoices ) / sizeof( wxString );
	m_OptDisplayVias = new wxRadioBox( this, ID_VIAS_SHAPES, _("Via Shapes:"), wxDefaultPosition, wxDefaultSize, m_OptDisplayViasNChoices, m_OptDisplayViasChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayVias->SetSelection( 1 );
	sLeftBoxSizer->Add( m_OptDisplayVias, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_OptDisplayViaHoleChoices[] = { _("Never"), _("Defined holes"), _("Always") };
	int m_OptDisplayViaHoleNChoices = sizeof( m_OptDisplayViaHoleChoices ) / sizeof( wxString );
	m_OptDisplayViaHole = new wxRadioBox( this, ID_VIAS_HOLES, _("Show Via Holes:"), wxDefaultPosition, wxDefaultSize, m_OptDisplayViaHoleNChoices, m_OptDisplayViaHoleChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayViaHole->SetSelection( 1 );
	m_OptDisplayViaHole->SetToolTip( _("Show (or not) via holes.\nIf Defined Holes is selected, only the non default size holes are shown") );
	
	sLeftBoxSizer->Add( m_OptDisplayViaHole, 0, wxALL|wxEXPAND, 5 );
	
	
	bMainSizer->Add( sLeftBoxSizer, 0, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* sbMiddleLeftSizer;
	sbMiddleLeftSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Routing help:") ), wxVERTICAL );
	
	wxString m_ShowNetNamesOptionChoices[] = { _("Do not show"), _("On pads"), _("On tracks"), _("On pads and tracks") };
	int m_ShowNetNamesOptionNChoices = sizeof( m_ShowNetNamesOptionChoices ) / sizeof( wxString );
	m_ShowNetNamesOption = new wxRadioBox( this, wxID_ANY, _("Show Net Names:"), wxDefaultPosition, wxDefaultSize, m_ShowNetNamesOptionNChoices, m_ShowNetNamesOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_ShowNetNamesOption->SetSelection( 3 );
	m_ShowNetNamesOption->SetToolTip( _("Show or not net names on pads and/or tracks") );
	
	sbMiddleLeftSizer->Add( m_ShowNetNamesOption, 0, wxALL, 5 );
	
	wxString m_OptDisplayTracksClearanceChoices[] = { _("Never"), _("New track"), _("New track with via area"), _("New and edited tracks with via area"), _("Always") };
	int m_OptDisplayTracksClearanceNChoices = sizeof( m_OptDisplayTracksClearanceChoices ) / sizeof( wxString );
	m_OptDisplayTracksClearance = new wxRadioBox( this, ID_SHOW_CLEARANCE, _("Show Tracks Clearance:"), wxDefaultPosition, wxDefaultSize, m_OptDisplayTracksClearanceNChoices, m_OptDisplayTracksClearanceChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayTracksClearance->SetSelection( 3 );
	m_OptDisplayTracksClearance->SetToolTip( _("Show( or not) tracks clearance area.\nIf New track is selected,  track clearance area is shown only when creating the track.") );
	
	sbMiddleLeftSizer->Add( m_OptDisplayTracksClearance, 0, wxALL|wxEXPAND, 5 );
	
	
	bMainSizer->Add( sbMiddleLeftSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sMiddleRightSizer;
	sMiddleRightSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Footprints:") ), wxHORIZONTAL );
	
	wxBoxSizer* bLModuleSizer;
	bLModuleSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_OptDisplayModEdgesChoices[] = { _("Line"), _("Filled"), _("Sketch") };
	int m_OptDisplayModEdgesNChoices = sizeof( m_OptDisplayModEdgesChoices ) / sizeof( wxString );
	m_OptDisplayModEdges = new wxRadioBox( this, ID_EDGES_MODULES, _("Footprint Edges:"), wxDefaultPosition, wxDefaultSize, m_OptDisplayModEdgesNChoices, m_OptDisplayModEdgesChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayModEdges->SetSelection( 1 );
	bLModuleSizer->Add( m_OptDisplayModEdges, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_OptDisplayModTextsChoices[] = { _("Line"), _("Filled"), _("Sketch") };
	int m_OptDisplayModTextsNChoices = sizeof( m_OptDisplayModTextsChoices ) / sizeof( wxString );
	m_OptDisplayModTexts = new wxRadioBox( this, ID_TEXT_MODULES, _("Texts:"), wxDefaultPosition, wxDefaultSize, m_OptDisplayModTextsNChoices, m_OptDisplayModTextsChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayModTexts->SetSelection( 1 );
	bLModuleSizer->Add( m_OptDisplayModTexts, 0, wxALL|wxEXPAND, 5 );
	
	
	sMiddleRightSizer->Add( bLModuleSizer, 0, 0, 5 );
	
	wxStaticBoxSizer* bRModuleSizer;
	bRModuleSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Pad Options:") ), wxVERTICAL );
	
	wxString m_OptDisplayPadsChoices[] = { _("Sketch"), _("Filled") };
	int m_OptDisplayPadsNChoices = sizeof( m_OptDisplayPadsChoices ) / sizeof( wxString );
	m_OptDisplayPads = new wxRadioBox( this, ID_PADS_SHAPES, _("Pad Shapes:"), wxDefaultPosition, wxDefaultSize, m_OptDisplayPadsNChoices, m_OptDisplayPadsChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayPads->SetSelection( 1 );
	bRModuleSizer->Add( m_OptDisplayPads, 0, wxALL|wxEXPAND, 5 );
	
	m_OptDisplayPadClearence = new wxCheckBox( this, wxID_ANY, _("Show pad clearance"), wxDefaultPosition, wxDefaultSize, 0 );
	bRModuleSizer->Add( m_OptDisplayPadClearence, 0, wxALL, 5 );
	
	m_OptDisplayPadNumber = new wxCheckBox( this, wxID_ANY, _("Show pad number"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayPadNumber->SetValue(true); 
	bRModuleSizer->Add( m_OptDisplayPadNumber, 0, wxALL, 5 );
	
	m_OptDisplayPadNoConn = new wxCheckBox( this, wxID_ANY, _("Show pad NoConnect"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayPadNoConn->SetValue(true); 
	bRModuleSizer->Add( m_OptDisplayPadNoConn, 0, wxALL, 5 );
	
	
	sMiddleRightSizer->Add( bRModuleSizer, 0, 0, 5 );
	
	
	bMainSizer->Add( sMiddleRightSizer, 0, wxEXPAND|wxALL, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sRightUpperSizer;
	sRightUpperSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Others:") ), wxVERTICAL );
	
	wxString m_OptDisplayDrawingsChoices[] = { _("Line"), _("Filled"), _("Sketch") };
	int m_OptDisplayDrawingsNChoices = sizeof( m_OptDisplayDrawingsChoices ) / sizeof( wxString );
	m_OptDisplayDrawings = new wxRadioBox( this, wxID_ANY, _("Display other items:"), wxDefaultPosition, wxDefaultSize, m_OptDisplayDrawingsNChoices, m_OptDisplayDrawingsChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayDrawings->SetSelection( 1 );
	sRightUpperSizer->Add( m_OptDisplayDrawings, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_Show_Page_LimitsChoices[] = { _("Yes"), _("No") };
	int m_Show_Page_LimitsNChoices = sizeof( m_Show_Page_LimitsChoices ) / sizeof( wxString );
	m_Show_Page_Limits = new wxRadioBox( this, wxID_ANY, _("Show page limits"), wxDefaultPosition, wxDefaultSize, m_Show_Page_LimitsNChoices, m_Show_Page_LimitsChoices, 1, wxRA_SPECIFY_COLS );
	m_Show_Page_Limits->SetSelection( 1 );
	sRightUpperSizer->Add( m_Show_Page_Limits, 0, wxALL|wxEXPAND, 5 );
	
	
	bRightSizer->Add( sRightUpperSizer, 1, wxEXPAND, 5 );
	
	
	bRightSizer->Add( 10, 10, 0, 0, 5 );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonOK, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonCANCEL = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonCANCEL, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	
	bMainSizer->Add( bRightSizer, 0, wxEXPAND|wxALL, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnOkClick ), NULL, this );
	m_buttonCANCEL->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnCancelClick ), NULL, this );
}

DIALOG_DISPLAY_OPTIONS_BASE::~DIALOG_DISPLAY_OPTIONS_BASE()
{
	// Disconnect Events
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnOkClick ), NULL, this );
	m_buttonCANCEL->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnCancelClick ), NULL, this );
	
}
