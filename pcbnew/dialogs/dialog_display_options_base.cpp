///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version May  6 2016)
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
	
	wxBoxSizer* sLeftSizer;
	sLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sSketchModeSizer;
	sSketchModeSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Tracks and Vias:") ), wxVERTICAL );
	
	m_OptDisplayTracks = new wxCheckBox( sSketchModeSizer->GetStaticBox(), wxID_ANY, _("Show tracks in sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sSketchModeSizer->Add( m_OptDisplayTracks, 0, wxALL, 5 );
	
	m_OptDisplayVias = new wxCheckBox( sSketchModeSizer->GetStaticBox(), wxID_ANY, _("Show vias in sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sSketchModeSizer->Add( m_OptDisplayVias, 0, wxALL, 5 );
	
	
	sLeftSizer->Add( sSketchModeSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sOpenGLRenderingSizer;
	sOpenGLRenderingSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("OpenGL Rendering:") ), wxVERTICAL );
	
	wxString m_choiceAntialiasingChoices[] = { _("No Antialiasing"), _("Subpixel Antialiasing (High Quality)"), _("Subpixel Antialiasing (Ultra Quality)"), _("Supersampling (2x)"), _("Supersampling (4x)") };
	int m_choiceAntialiasingNChoices = sizeof( m_choiceAntialiasingChoices ) / sizeof( wxString );
	m_choiceAntialiasing = new wxChoice( sOpenGLRenderingSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceAntialiasingNChoices, m_choiceAntialiasingChoices, 0 );
	m_choiceAntialiasing->SetSelection( 0 );
	sOpenGLRenderingSizer->Add( m_choiceAntialiasing, 0, wxALL|wxEXPAND, 5 );
	
	
	sLeftSizer->Add( sOpenGLRenderingSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sGridSettings;
	sGridSettings = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Grid Display (OpenGL && Cairo)") ), wxVERTICAL );
	
	wxString m_gridStyleChoices[] = { _("Dots"), _("Lines") };
	int m_gridStyleNChoices = sizeof( m_gridStyleChoices ) / sizeof( wxString );
	m_gridStyle = new wxRadioBox( sGridSettings->GetStaticBox(), wxID_ANY, _("Grid Style"), wxDefaultPosition, wxDefaultSize, m_gridStyleNChoices, m_gridStyleChoices, 1, wxRA_SPECIFY_COLS );
	m_gridStyle->SetSelection( 0 );
	sGridSettings->Add( m_gridStyle, 0, wxALL|wxEXPAND, 5 );
	
	wxFlexGridSizer* sGridSettingsGrid;
	sGridSettingsGrid = new wxFlexGridSizer( 0, 4, 0, 0 );
	sGridSettingsGrid->AddGrowableCol( 1 );
	sGridSettingsGrid->SetFlexibleDirection( wxBOTH );
	sGridSettingsGrid->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	l_gridLineWidth = new wxStaticText( sGridSettings->GetStaticBox(), wxID_ANY, _("Grid thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	l_gridLineWidth->Wrap( -1 );
	sGridSettingsGrid->Add( l_gridLineWidth, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_gridLineWidth = new wxTextCtrl( sGridSettings->GetStaticBox(), wxID_ANY, _("0.5"), wxDefaultPosition, wxDefaultSize, 0 );
	sGridSettingsGrid->Add( m_gridLineWidth, 0, wxEXPAND, 5 );
	
	m_gridLineWidthSpinBtn = new wxSpinButton( sGridSettings->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS );
	sGridSettingsGrid->Add( m_gridLineWidthSpinBtn, 0, wxALL, 0 );
	
	l_gridLineWidthUnits = new wxStaticText( sGridSettings->GetStaticBox(), wxID_ANY, _("px"), wxDefaultPosition, wxDefaultSize, 0 );
	l_gridLineWidthUnits->Wrap( -1 );
	sGridSettingsGrid->Add( l_gridLineWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	l_gridMinSpacing = new wxStaticText( sGridSettings->GetStaticBox(), wxID_ANY, _("Min grid spacing:"), wxDefaultPosition, wxDefaultSize, 0 );
	l_gridMinSpacing->Wrap( -1 );
	sGridSettingsGrid->Add( l_gridMinSpacing, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_gridMinSpacing = new wxTextCtrl( sGridSettings->GetStaticBox(), wxID_ANY, _("10"), wxDefaultPosition, wxDefaultSize, 0 );
	sGridSettingsGrid->Add( m_gridMinSpacing, 0, wxEXPAND, 5 );
	
	m_gridMinSpacingSpinBtn = new wxSpinButton( sGridSettings->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS );
	sGridSettingsGrid->Add( m_gridMinSpacingSpinBtn, 0, wxALL, 0 );
	
	l_gridMinSpacingUnits = new wxStaticText( sGridSettings->GetStaticBox(), wxID_ANY, _("px"), wxDefaultPosition, wxDefaultSize, 0 );
	l_gridMinSpacingUnits->Wrap( -1 );
	sGridSettingsGrid->Add( l_gridMinSpacingUnits, 0, wxALL, 5 );
	
	
	sGridSettings->Add( sGridSettingsGrid, 1, wxALL|wxEXPAND, 5 );
	
	
	sLeftSizer->Add( sGridSettings, 1, wxALL|wxEXPAND, 5 );
	
	
	bupperSizer->Add( sLeftSizer, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sMiddleSizer;
	sMiddleSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Routing Help:") ), wxVERTICAL );
	
	wxString m_ShowNetNamesOptionChoices[] = { _("Do not show"), _("On pads"), _("On tracks"), _("On pads and tracks") };
	int m_ShowNetNamesOptionNChoices = sizeof( m_ShowNetNamesOptionChoices ) / sizeof( wxString );
	m_ShowNetNamesOption = new wxRadioBox( sMiddleSizer->GetStaticBox(), wxID_ANY, _("Show Net Names:"), wxDefaultPosition, wxDefaultSize, m_ShowNetNamesOptionNChoices, m_ShowNetNamesOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_ShowNetNamesOption->SetSelection( 0 );
	m_ShowNetNamesOption->SetToolTip( _("Show or hide net names on pads and/or tracks") );
	
	sMiddleSizer->Add( m_ShowNetNamesOption, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_OptDisplayTracksClearanceChoices[] = { _("Never"), _("New track"), _("New track with via area"), _("New and edited tracks with via area"), _("Always") };
	int m_OptDisplayTracksClearanceNChoices = sizeof( m_OptDisplayTracksClearanceChoices ) / sizeof( wxString );
	m_OptDisplayTracksClearance = new wxRadioBox( sMiddleSizer->GetStaticBox(), ID_SHOW_CLEARANCE, _("Show Track Clearance:"), wxDefaultPosition, wxDefaultSize, m_OptDisplayTracksClearanceNChoices, m_OptDisplayTracksClearanceChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayTracksClearance->SetSelection( 0 );
	m_OptDisplayTracksClearance->SetToolTip( _("Show or hide the track and via clearance area.\nIf New track is selected,  track clearance area is shown only when creating the track.") );
	
	sMiddleSizer->Add( m_OptDisplayTracksClearance, 1, wxALL|wxEXPAND, 5 );
	
	
	bupperSizer->Add( sMiddleSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* sRightSizer;
	sRightSizer = new wxBoxSizer( wxVERTICAL );
	
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
	
	
	bupperSizer->Add( sRightSizer, 0, 0, 5 );
	
	
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
