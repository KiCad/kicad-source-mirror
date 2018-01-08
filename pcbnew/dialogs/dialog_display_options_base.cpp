///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jan  2 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
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
	
	sLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	
	bupperSizer->Add( sLeftSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );
	
	wxString m_ShowNetNamesOptionChoices[] = { _("Do not show"), _("On pads"), _("On tracks"), _("On pads and tracks") };
	int m_ShowNetNamesOptionNChoices = sizeof( m_ShowNetNamesOptionChoices ) / sizeof( wxString );
	m_ShowNetNamesOption = new wxRadioBox( this, wxID_ANY, _("Show Net Names:"), wxDefaultPosition, wxDefaultSize, m_ShowNetNamesOptionNChoices, m_ShowNetNamesOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_ShowNetNamesOption->SetSelection( 0 );
	m_ShowNetNamesOption->SetToolTip( _("Show or hide net names on pads and/or tracks.") );
	
	bSizer5->Add( m_ShowNetNamesOption, 1, wxALL|wxEXPAND, 5 );
	
	wxString m_OptDisplayTracksClearanceChoices[] = { _("Never"), _("New track"), _("New track with via area"), _("New and edited tracks with via area"), _("Always") };
	int m_OptDisplayTracksClearanceNChoices = sizeof( m_OptDisplayTracksClearanceChoices ) / sizeof( wxString );
	m_OptDisplayTracksClearance = new wxRadioBox( this, ID_SHOW_CLEARANCE, _("Show Track Clearance:"), wxDefaultPosition, wxDefaultSize, m_OptDisplayTracksClearanceNChoices, m_OptDisplayTracksClearanceChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayTracksClearance->SetSelection( 0 );
	m_OptDisplayTracksClearance->SetToolTip( _("Show or hide the track and via clearance area. If \"New track\" is selected, track clearance area is shown only when creating the track.") );
	
	bSizer5->Add( m_OptDisplayTracksClearance, 1, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer5;
	sbSizer5 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("User Interface:") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText1 = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("Icon scale:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgSizer1->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 3 );
	
	m_scaleSlider = new STEPPED_SLIDER( sbSizer5->GetStaticBox(), wxID_ANY, 50, 50, 275, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS );
	fgSizer1->Add( m_scaleSlider, 0, wxEXPAND|wxLEFT|wxRIGHT, 3 );
	
	m_staticText2 = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizer1->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_scaleAuto = new wxCheckBox( sbSizer5->GetStaticBox(), wxID_ANY, _("Auto"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_scaleAuto, 0, wxALL, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	sbSizer5->Add( fgSizer1, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizer5->Add( sbSizer5, 1, wxALL|wxEXPAND, 5 );
	
	
	bupperSizer->Add( bSizer5, 1, wxEXPAND, 5 );
	
	wxBoxSizer* sRightSizer;
	sRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sSketchModeSizer;
	sSketchModeSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Tracks and Vias:") ), wxVERTICAL );
	
	m_OptDisplayTracks = new wxCheckBox( sSketchModeSizer->GetStaticBox(), wxID_ANY, _("Show tracks in sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sSketchModeSizer->Add( m_OptDisplayTracks, 0, wxALL, 5 );
	
	m_OptDisplayVias = new wxCheckBox( sSketchModeSizer->GetStaticBox(), wxID_ANY, _("Show vias in sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sSketchModeSizer->Add( m_OptDisplayVias, 0, wxALL, 5 );
	
	
	sRightSizer->Add( sSketchModeSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sfootprintSizer;
	sfootprintSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Footprints:") ), wxVERTICAL );
	
	m_OptDisplayModOutlines = new wxCheckBox( sfootprintSizer->GetStaticBox(), wxID_ANY, _("Show outlines in sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sfootprintSizer->Add( m_OptDisplayModOutlines, 0, wxALL, 5 );
	
	m_OptDisplayModTexts
	= new wxCheckBox( sfootprintSizer->GetStaticBox(), wxID_ANY, _("Show text in sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sfootprintSizer->Add( m_OptDisplayModTexts
	, 0, wxALL, 5 );
	
	m_OptDisplayPads = new wxCheckBox( sfootprintSizer->GetStaticBox(), wxID_ANY, _("Show pads in sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sfootprintSizer->Add( m_OptDisplayPads, 0, wxALL, 5 );
	
	m_OptDisplayPadClearence = new wxCheckBox( sfootprintSizer->GetStaticBox(), wxID_ANY, _("Show pad clearance"), wxDefaultPosition, wxDefaultSize, 0 );
	sfootprintSizer->Add( m_OptDisplayPadClearence, 0, wxALL, 5 );
	
	m_OptDisplayPadNumber = new wxCheckBox( sfootprintSizer->GetStaticBox(), wxID_ANY, _("Show pad number"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayPadNumber->SetValue(true); 
	sfootprintSizer->Add( m_OptDisplayPadNumber, 0, wxALL, 5 );
	
	m_OptDisplayPadNoConn = new wxCheckBox( sfootprintSizer->GetStaticBox(), wxID_ANY, _("Show pad no net connection indicator"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayPadNoConn->SetValue(true); 
	sfootprintSizer->Add( m_OptDisplayPadNoConn, 0, wxALL, 5 );
	
	
	sRightSizer->Add( sfootprintSizer, 0, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* s_otherSizer;
	s_otherSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Other:") ), wxVERTICAL );
	
	m_OptDisplayDrawings = new wxCheckBox( s_otherSizer->GetStaticBox(), wxID_ANY, _("Show graphic items in sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayDrawings->SetValue(true); 
	s_otherSizer->Add( m_OptDisplayDrawings, 0, wxALL, 5 );
	
	m_Show_Page_Limits = new wxCheckBox( s_otherSizer->GetStaticBox(), wxID_ANY, _("Show page limits"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Show_Page_Limits->SetValue(true); 
	s_otherSizer->Add( m_Show_Page_Limits, 0, wxALL, 5 );
	
	
	sRightSizer->Add( s_otherSizer, 1, wxEXPAND|wxALL, 5 );
	
	
	bupperSizer->Add( sRightSizer, 0, wxEXPAND, 5 );
	
	
	bMainSizer->Add( bupperSizer, 1, wxEXPAND, 5 );
	
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
	m_scaleSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleAuto->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleAuto ), NULL, this );
}

DIALOG_DISPLAY_OPTIONS_BASE::~DIALOG_DISPLAY_OPTIONS_BASE()
{
	// Disconnect Events
	m_scaleSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleAuto->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleAuto ), NULL, this );
	
}
