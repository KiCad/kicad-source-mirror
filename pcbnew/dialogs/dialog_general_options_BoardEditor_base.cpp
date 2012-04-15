///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar 17 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_general_options_BoardEditor_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_PolarDisplayChoices[] = { _("No Display"), _("Display") };
	int m_PolarDisplayNChoices = sizeof( m_PolarDisplayChoices ) / sizeof( wxString );
	m_PolarDisplay = new wxRadioBox( this, wxID_POLAR_CTRL, _("Display Polar Coord"), wxDefaultPosition, wxDefaultSize, m_PolarDisplayNChoices, m_PolarDisplayChoices, 1, wxRA_SPECIFY_COLS );
	m_PolarDisplay->SetSelection( 1 );
	m_PolarDisplay->SetToolTip( _("Activates the display of relative coordinates from relative origin (set by the space key)\nto the cursor, in polar coordinates (angle and distance)") );
	
	bLeftSizer->Add( m_PolarDisplay, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_UnitsSelectionChoices[] = { _("Inches"), _("Millimeters") };
	int m_UnitsSelectionNChoices = sizeof( m_UnitsSelectionChoices ) / sizeof( wxString );
	m_UnitsSelection = new wxRadioBox( this, wxID_UNITS, _("Units"), wxDefaultPosition, wxDefaultSize, m_UnitsSelectionNChoices, m_UnitsSelectionChoices, 1, wxRA_SPECIFY_COLS );
	m_UnitsSelection->SetSelection( 1 );
	m_UnitsSelection->SetToolTip( _("Selection of units used to display dimensions and positions of items") );
	
	bLeftSizer->Add( m_UnitsSelection, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_CursorShapeChoices[] = { _("Small cross"), _("Full screen cursor") };
	int m_CursorShapeNChoices = sizeof( m_CursorShapeChoices ) / sizeof( wxString );
	m_CursorShape = new wxRadioBox( this, wxID_CURSOR_SHAPE, _("Cursor"), wxDefaultPosition, wxDefaultSize, m_CursorShapeNChoices, m_CursorShapeChoices, 1, wxRA_SPECIFY_COLS );
	m_CursorShape->SetSelection( 0 );
	m_CursorShape->SetToolTip( _("Main cursor shape selection (small cross or large cursor)") );
	
	bLeftSizer->Add( m_CursorShape, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerUpper->Add( bLeftSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bMiddleLeftSizer;
	bMiddleLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextmaxlinks = new wxStaticText( this, wxID_ANY, _("Max Links:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextmaxlinks->Wrap( -1 );
	bMiddleLeftSizer->Add( m_staticTextmaxlinks, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_MaxShowLinks = new wxSpinCtrl( this, wxID_ANY, wxT("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 5, 1 );
	m_MaxShowLinks->SetToolTip( _("Adjust the number of ratsnets shown from cursor to closest pads") );
	
	bMiddleLeftSizer->Add( m_MaxShowLinks, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_staticTextautosave = new wxStaticText( this, wxID_ANY, _("Auto Save (minutes):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextautosave->Wrap( -1 );
	bMiddleLeftSizer->Add( m_staticTextautosave, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_SaveTime = new wxSpinCtrl( this, wxID_ANY, wxT("0"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 60, 0 );
	m_SaveTime->SetToolTip( _("Delay after the first change to create a backup file of the board on disk.") );
	
	bMiddleLeftSizer->Add( m_SaveTime, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_staticTextRotationAngle = new wxStaticText( this, wxID_ANY, _("Rotation Angle"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRotationAngle->Wrap( -1 );
	bMiddleLeftSizer->Add( m_staticTextRotationAngle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxString m_RotationAngleChoices[] = { _("45"), _("90") };
	int m_RotationAngleNChoices = sizeof( m_RotationAngleChoices ) / sizeof( wxString );
	m_RotationAngle = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_RotationAngleNChoices, m_RotationAngleChoices, 0 );
	m_RotationAngle->SetSelection( 0 );
	m_RotationAngle->SetToolTip( _("Footprints rotation increment, for rotate menu or hot key.") );
	
	bMiddleLeftSizer->Add( m_RotationAngle, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	
	bSizerUpper->Add( bMiddleLeftSizer, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* bMiddleRightBoxSizer;
	bMiddleRightBoxSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options:") ), wxVERTICAL );
	
	m_DrcOn = new wxCheckBox( this, wxID_DRC_ONOFF, _("Drc ON"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DrcOn->SetValue(true); 
	m_DrcOn->SetToolTip( _("Enable/disable the DRC control.\nWhen DRC is disable, all connections are allowed.") );
	
	bMiddleRightBoxSizer->Add( m_DrcOn, 0, wxALL|wxEXPAND, 5 );
	
	m_ShowGlobalRatsnest = new wxCheckBox( this, wxID_GENERAL_RATSNEST, _("Show Ratsnest"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ShowGlobalRatsnest->SetToolTip( _("Show (or not) the full rastnest.") );
	
	bMiddleRightBoxSizer->Add( m_ShowGlobalRatsnest, 0, wxALL, 5 );
	
	m_ShowModuleRatsnest = new wxCheckBox( this, wxID_RATSNEST_MODULE, _("Show Mod Ratsnest"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ShowModuleRatsnest->SetToolTip( _("Shows (or not) the local ratsnest relative to a footprint, when moving it.\nThis ratsnest is useful to place a footprint.") );
	
	bMiddleRightBoxSizer->Add( m_ShowModuleRatsnest, 0, wxALL, 5 );
	
	m_TrackAutodel = new wxCheckBox( this, wxID_TRACK_AUTODEL, _("Tracks Auto Del"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackAutodel->SetToolTip( _("Enable/disable the automatic track deletion when recreating a track.") );
	
	bMiddleRightBoxSizer->Add( m_TrackAutodel, 0, wxALL, 5 );
	
	m_Track_45_Only_Ctrl = new wxCheckBox( this, wxID_TRACKS45, _("Track only 45 degrees"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Track_45_Only_Ctrl->SetToolTip( _("If enabled, force tracks directions to H, V or 45 degrees, when creating a track.") );
	
	bMiddleRightBoxSizer->Add( m_Track_45_Only_Ctrl, 0, wxALL, 5 );
	
	m_Segments_45_Only_Ctrl = new wxCheckBox( this, wxID_SEGMENTS45, _("Segments 45 Only"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Segments_45_Only_Ctrl->SetToolTip( _("If enabled, force segments directions to H, V or 45 degrees, when creating a segment on technical layers.") );
	
	bMiddleRightBoxSizer->Add( m_Segments_45_Only_Ctrl, 0, wxALL, 5 );
	
	m_Track_DoubleSegm_Ctrl = new wxCheckBox( this, wxID_ANY, _("Double Segm Track"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Track_DoubleSegm_Ctrl->SetToolTip( _("If enabled, uses two track segments, with 45 degrees angle between them when creating a new track ") );
	
	bMiddleRightBoxSizer->Add( m_Track_DoubleSegm_Ctrl, 0, wxALL, 5 );
	
	
	bSizerUpper->Add( bMiddleRightBoxSizer, 1, 0, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_MagneticPadOptCtrlChoices[] = { _("Never"), _("When creating tracks"), _("Always") };
	int m_MagneticPadOptCtrlNChoices = sizeof( m_MagneticPadOptCtrlChoices ) / sizeof( wxString );
	m_MagneticPadOptCtrl = new wxRadioBox( this, wxID_ANY, _("Magnetic Pads"), wxDefaultPosition, wxDefaultSize, m_MagneticPadOptCtrlNChoices, m_MagneticPadOptCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_MagneticPadOptCtrl->SetSelection( 0 );
	m_MagneticPadOptCtrl->SetToolTip( _("control the capture of the pcb cursor when the mouse cursor enters a pad area") );
	
	bRightSizer->Add( m_MagneticPadOptCtrl, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_MagneticTrackOptCtrlChoices[] = { _("Never"), _("When creating tracks"), _("Always") };
	int m_MagneticTrackOptCtrlNChoices = sizeof( m_MagneticTrackOptCtrlChoices ) / sizeof( wxString );
	m_MagneticTrackOptCtrl = new wxRadioBox( this, wxID_MAGNETIC_TRACKS, _("Magnetic Tracks"), wxDefaultPosition, wxDefaultSize, m_MagneticTrackOptCtrlNChoices, m_MagneticTrackOptCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_MagneticTrackOptCtrl->SetSelection( 0 );
	m_MagneticTrackOptCtrl->SetToolTip( _("Control the capture of the pcb cursor when the mouse cursor enters a track") );
	
	bRightSizer->Add( m_MagneticTrackOptCtrl, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer2PAN;
	sbSizer2PAN = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Pan:") ), wxVERTICAL );
	
	m_MiddleButtonPANOpt = new wxCheckBox( this, wxID_MIDDLEBUTTONPAN, _("Middle Button PAN Enabled"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MiddleButtonPANOpt->SetToolTip( _("Use middle mouse button dragging to pan") );
	
	sbSizer2PAN->Add( m_MiddleButtonPANOpt, 0, wxALL, 5 );
	
	m_OptMiddleButtonPanLimited = new wxCheckBox( this, wxID_MIDDLEBUTTONPAN, _("Middle Button PAN Limited"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptMiddleButtonPanLimited->SetToolTip( _("Middle mouse button panning limited by current scrollbar size") );
	
	sbSizer2PAN->Add( m_OptMiddleButtonPanLimited, 0, wxALL, 5 );
	
	m_AutoPANOpt = new wxCheckBox( this, wxID_AUTOPAN, _("Auto PAN"), wxDefaultPosition, wxDefaultSize, 0 );
	m_AutoPANOpt->SetToolTip( _("Allows auto pan when creating a track, or moving an item.") );
	
	sbSizer2PAN->Add( m_AutoPANOpt, 0, wxALL, 5 );
	
	
	bRightSizer->Add( sbSizer2PAN, 1, wxEXPAND|wxBOTTOM|wxLEFT, 5 );
	
	
	bSizerUpper->Add( bRightSizer, 1, wxEXPAND, 5 );
	
	
	bMainSizer->Add( bSizerUpper, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bMainSizer->Add( m_sdbSizer, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	m_MiddleButtonPANOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnMiddleBtnPanEnbl ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnOkClick ), NULL, this );
}

DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::~DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE()
{
	// Disconnect Events
	m_MiddleButtonPANOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnMiddleBtnPanEnbl ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnOkClick ), NULL, this );
	
}
