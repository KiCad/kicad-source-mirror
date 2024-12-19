///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
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

	wxBoxSizer* bColumns;
	bColumns = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbModeSizer;
	sbModeSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Mode") ), wxVERTICAL );

	m_rbMarkObstacles = new wxRadioButton( sbModeSizer->GetStaticBox(), wxID_ANY, _("Highlight collisions"), wxDefaultPosition, wxDefaultSize, 0 );
	sbModeSizer->Add( m_rbMarkObstacles, 0, wxRIGHT, 5 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	m_freeAngleMode = new wxCheckBox( sbModeSizer->GetStaticBox(), wxID_ANY, _("Free angle mode"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_freeAngleMode, 0, wxTOP|wxRIGHT|wxLEFT, 3 );

	m_violateDrc = new wxCheckBox( sbModeSizer->GetStaticBox(), wxID_ANY, _("Allow DRC violations"), wxDefaultPosition, wxDefaultSize, 0 );
	m_violateDrc->SetToolTip( _("(Highlight collisions mode only) - allows one to establish a track even if is violating the DRC rules.") );

	bSizer2->Add( m_violateDrc, 0, wxTOP|wxRIGHT|wxLEFT, 3 );


	sbModeSizer->Add( bSizer2, 0, wxRIGHT|wxLEFT, 20 );


	sbModeSizer->Add( 0, 15, 1, wxEXPAND, 5 );

	m_rbShove = new wxRadioButton( sbModeSizer->GetStaticBox(), wxID_ANY, _("Shove"), wxDefaultPosition, wxDefaultSize, 0 );
	sbModeSizer->Add( m_rbShove, 0, wxRIGHT, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	m_shoveVias = new wxCheckBox( sbModeSizer->GetStaticBox(), wxID_ANY, _("Shove vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shoveVias->SetToolTip( _("When disabled, vias are treated as un-movable objects and hugged instead of shoved.") );

	bSizer3->Add( m_shoveVias, 0, wxTOP|wxRIGHT|wxLEFT, 3 );

	m_backPressure = new wxCheckBox( sbModeSizer->GetStaticBox(), wxID_ANY, _("Jump over obstacles"), wxDefaultPosition, wxDefaultSize, 0 );
	m_backPressure->SetToolTip( _("When enabled, the router tries to move colliding tracks behind solid obstacles (e.g. pads) instead of \"reflecting\" back the collision") );

	bSizer3->Add( m_backPressure, 0, wxTOP|wxRIGHT|wxLEFT, 3 );


	sbModeSizer->Add( bSizer3, 0, wxRIGHT|wxLEFT, 20 );


	sbModeSizer->Add( 0, 15, 1, wxEXPAND, 5 );

	m_rbWalkaround = new wxRadioButton( sbModeSizer->GetStaticBox(), wxID_ANY, _("Walk around"), wxDefaultPosition, wxDefaultSize, 0 );
	sbModeSizer->Add( m_rbWalkaround, 0, wxBOTTOM|wxRIGHT, 5 );


	bColumns->Add( sbModeSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bColumns->Add( 5, 0, 0, 0, 5 );

	wxStaticBoxSizer* bOptions;
	bOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("General Options") ), wxVERTICAL );

	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 0, 1, 2, 0 );

	m_removeLoops = new wxCheckBox( bOptions->GetStaticBox(), wxID_ANY, _("Remove redundant tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_removeLoops->SetToolTip( _("If the new track has the same connection as an already existing track, the old track is removed.") );

	gSizer1->Add( m_removeLoops, 0, wxRIGHT|wxLEFT, 5 );

	m_smartPads = new wxCheckBox( bOptions->GetStaticBox(), wxID_ANY, _("Optimize pad connections"), wxDefaultPosition, wxDefaultSize, 0 );
	m_smartPads->SetToolTip( _("When enabled, the router tries to break out pads/vias in a clean way, avoiding acute angles and jagged breakout tracks.") );

	gSizer1->Add( m_smartPads, 0, wxRIGHT|wxLEFT, 5 );

	m_smoothDragged = new wxCheckBox( bOptions->GetStaticBox(), wxID_ANY, _("Smooth dragged segments"), wxDefaultPosition, wxDefaultSize, 0 );
	m_smoothDragged->SetToolTip( _("When enabled, the router attempts to merge several jagged segments into a single straight one (dragging mode).") );

	gSizer1->Add( m_smoothDragged, 0, wxRIGHT|wxLEFT, 5 );

	m_suggestEnding = new wxCheckBox( bOptions->GetStaticBox(), wxID_ANY, _("Suggest track finish"), wxDefaultPosition, wxDefaultSize, 0 );
	m_suggestEnding->Enable( false );

	gSizer1->Add( m_suggestEnding, 0, wxRIGHT|wxLEFT, 5 );

	m_optimizeEntireDraggedTrack = new wxCheckBox( bOptions->GetStaticBox(), wxID_ANY, _("Optimize entire track being dragged"), wxDefaultPosition, wxDefaultSize, 0 );
	m_optimizeEntireDraggedTrack->SetToolTip( _("When enabled, the entire portion of the track that is visible on the screen will be optimized and re-routed when a segment is dragged.  When disabled, only the area near the segment being dragged will be optimized.") );

	gSizer1->Add( m_optimizeEntireDraggedTrack, 0, wxRIGHT|wxLEFT, 5 );

	m_autoPosture = new wxCheckBox( bOptions->GetStaticBox(), wxID_ANY, _("Use mouse path to set track posture"), wxDefaultPosition, wxDefaultSize, 0 );
	m_autoPosture->SetToolTip( _("When enabled, the posture of tracks will be guided by how the mouse is moved from the starting location") );

	gSizer1->Add( m_autoPosture, 0, wxRIGHT|wxLEFT, 5 );

	m_fixAllSegments = new wxCheckBox( bOptions->GetStaticBox(), wxID_ANY, _("Fix all segments on click"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fixAllSegments->SetToolTip( _("When enabled, all track segments will be fixed in place up to the cursor location.  When disabled, the last segment (closest to the cursor) will remain free and follow the cursor.") );

	gSizer1->Add( m_fixAllSegments, 0, wxRIGHT|wxLEFT, 5 );


	bOptions->Add( gSizer1, 1, wxEXPAND, 5 );


	bColumns->Add( bOptions, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bColumns, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

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
	m_rbMarkObstacles->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::onModeChange ), NULL, this );
	m_freeAngleMode->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::onFreeAngleModeChange ), NULL, this );
	m_rbShove->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::onModeChange ), NULL, this );
	m_rbWalkaround->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::onModeChange ), NULL, this );
	m_stdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::OnOkClick ), NULL, this );
}

DIALOG_PNS_SETTINGS_BASE::~DIALOG_PNS_SETTINGS_BASE()
{
	// Disconnect Events
	m_rbMarkObstacles->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::onModeChange ), NULL, this );
	m_freeAngleMode->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::onFreeAngleModeChange ), NULL, this );
	m_rbShove->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::onModeChange ), NULL, this );
	m_rbWalkaround->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::onModeChange ), NULL, this );
	m_stdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PNS_SETTINGS_BASE::OnOkClick ), NULL, this );

}
