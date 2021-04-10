///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_pns_settings_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PNS_SETTINGS_BASE::DIALOG_PNS_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxString m_modeChoices[] = { _("Highlight collisions"), _("Shove"), _("Walk around") };
	int m_modeNChoices = sizeof( m_modeChoices ) / sizeof( wxString );
	m_mode = new wxRadioBox( this, wxID_ANY, _("Mode"), wxDefaultPosition, wxDefaultSize, m_modeNChoices, m_modeChoices, 1, wxRA_SPECIFY_COLS );
	m_mode->SetSelection( 0 );
	bMainSizer->Add( m_mode, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* bOptions;
	bOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText4 = new wxStaticText( bOptions->GetStaticBox(), wxID_ANY, _("Mouse drag behavior:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	fgSizer1->Add( m_staticText4, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxString m_dragToolModeChoices[] = { _("Move item"), _("Interactive drag") };
	int m_dragToolModeNChoices = sizeof( m_dragToolModeChoices ) / sizeof( wxString );
	m_dragToolMode = new wxChoice( bOptions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dragToolModeNChoices, m_dragToolModeChoices, 0 );
	m_dragToolMode->SetSelection( 0 );
	fgSizer1->Add( m_dragToolMode, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bOptions->Add( fgSizer1, 1, wxEXPAND, 5 );

	m_freeAngleMode = new wxCheckBox( bOptions->GetStaticBox(), wxID_ANY, _("Free angle mode (no shove/walkaround)"), wxDefaultPosition, wxDefaultSize, 0 );
	bOptions->Add( m_freeAngleMode, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_shoveVias = new wxCheckBox( bOptions->GetStaticBox(), wxID_ANY, _("Shove vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shoveVias->Enable( false );
	m_shoveVias->SetToolTip( _("When disabled, vias are treated as un-movable objects and hugged instead of shoved.") );

	bOptions->Add( m_shoveVias, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_backPressure = new wxCheckBox( bOptions->GetStaticBox(), wxID_ANY, _("Jump over obstacles"), wxDefaultPosition, wxDefaultSize, 0 );
	m_backPressure->SetToolTip( _("When enabled, the router tries to move colliding traces behind solid obstacles (e.g. pads) instead of \"reflecting\" back the collision") );

	bOptions->Add( m_backPressure, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_removeLoops = new wxCheckBox( bOptions->GetStaticBox(), wxID_ANY, _("Remove redundant tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_removeLoops->SetToolTip( _("Removes loops while routing (e.g. if the new track ensures same connectivity as an already existing one, the old track is removed).\nLoop removal works locally (only between the start and end of the currently routed trace).") );

	bOptions->Add( m_removeLoops, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_smartPads = new wxCheckBox( bOptions->GetStaticBox(), wxID_ANY, _("Optimize pad connections"), wxDefaultPosition, wxDefaultSize, 0 );
	m_smartPads->SetToolTip( _("When enabled, the router tries to break out pads/vias in a clean way, avoiding acute angles and jagged breakout traces.") );

	bOptions->Add( m_smartPads, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_smoothDragged = new wxCheckBox( bOptions->GetStaticBox(), wxID_ANY, _("Smooth dragged segments"), wxDefaultPosition, wxDefaultSize, 0 );
	m_smoothDragged->SetToolTip( _("When enabled, the router attempts to merge several jagged segments into a single straight one (dragging mode).") );

	bOptions->Add( m_smoothDragged, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_violateDrc = new wxCheckBox( bOptions->GetStaticBox(), wxID_ANY, _("Allow DRC violations"), wxDefaultPosition, wxDefaultSize, 0 );
	m_violateDrc->SetToolTip( _("(Highlight collisions mode only) - allows one to establish a track even if is violating the DRC rules.") );

	bOptions->Add( m_violateDrc, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_suggestEnding = new wxCheckBox( bOptions->GetStaticBox(), wxID_ANY, _("Suggest track finish"), wxDefaultPosition, wxDefaultSize, 0 );
	m_suggestEnding->Enable( false );

	bOptions->Add( m_suggestEnding, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_optimizeEntireDraggedTrack = new wxCheckBox( bOptions->GetStaticBox(), wxID_ANY, _("Optimize entire track being dragged"), wxDefaultPosition, wxDefaultSize, 0 );
	m_optimizeEntireDraggedTrack->SetToolTip( _("When enabled, the entire track will be optimized and re-routed when dragged.  When disabled, only the area near the segment being dragged will be optimized.") );

	bOptions->Add( m_optimizeEntireDraggedTrack, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_autoPosture = new wxCheckBox( bOptions->GetStaticBox(), wxID_ANY, _("Use mouse path to set track posture"), wxDefaultPosition, wxDefaultSize, 0 );
	m_autoPosture->SetToolTip( _("When enabled, the posture of tracks will be guided by how the mouse is moved from the starting location") );

	bOptions->Add( m_autoPosture, 0, wxALL, 5 );

	m_fixAllSegments = new wxCheckBox( bOptions->GetStaticBox(), wxID_ANY, _("Fix all segments on click"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fixAllSegments->SetToolTip( _("When enabled, all track segments will be fixed in place up to the cursor location.  When disabled, the last segment (closest to the cursor) will remain free and follow the cursor.") );

	bOptions->Add( m_fixAllSegments, 0, wxALL, 5 );


	bMainSizer->Add( bOptions, 1, wxEXPAND|wxALL, 5 );

	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();

	bMainSizer->Add( m_stdButtons, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_mode->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::onModeChange ), NULL, this );
	m_freeAngleMode->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::onFreeAngleModeChange ), NULL, this );
	m_stdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::OnOkClick ), NULL, this );
}

DIALOG_PNS_SETTINGS_BASE::~DIALOG_PNS_SETTINGS_BASE()
{
	// Disconnect Events
	m_mode->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::onModeChange ), NULL, this );
	m_freeAngleMode->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::onFreeAngleModeChange ), NULL, this );
	m_stdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::OnOkClick ), NULL, this );

}
