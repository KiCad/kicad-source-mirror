///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 19 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_general_options_BoardEditor_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextautosave = new wxStaticText( this, wxID_ANY, _("&Auto save (minutes):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextautosave->Wrap( -1 );
	fgSizer1->Add( m_staticTextautosave, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_SaveTime = new wxSpinCtrl( this, wxID_ANY, wxT("0"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 60, 0 );
	m_SaveTime->SetToolTip( _("Delay after the first change to create a backup file of the board on disk. If set to 0, auto backup is disabled.") );
	
	fgSizer1->Add( m_SaveTime, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );
	
	
	bLeftSizer->Add( fgSizer1, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer5;
	sbSizer5 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("User Interface:") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer11;
	fgSizer11 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer11->AddGrowableCol( 1 );
	fgSizer11->SetFlexibleDirection( wxBOTH );
	fgSizer11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText1 = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("Icon scale:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgSizer11->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 3 );
	
	m_scaleSlider = new STEPPED_SLIDER( sbSizer5->GetStaticBox(), wxID_ANY, 50, 50, 275, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS );
	fgSizer11->Add( m_scaleSlider, 0, wxBOTTOM|wxEXPAND, 4 );
	
	m_staticText2 = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizer11->Add( m_staticText2, 0, wxALIGN_BOTTOM|wxBOTTOM, 2 );
	
	
	fgSizer11->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_scaleAuto = new wxCheckBox( sbSizer5->GetStaticBox(), wxID_ANY, _("Auto"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_scaleAuto, 0, wxLEFT, 9 );
	
	
	fgSizer11->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	sbSizer5->Add( fgSizer11, 0, wxALL|wxEXPAND, 5 );
	
	m_checkBoxIconsInMenus = new wxCheckBox( sbSizer5->GetStaticBox(), wxID_ANY, _("Show icons in menus"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer5->Add( m_checkBoxIconsInMenus, 0, wxALL, 5 );
	
	
	bLeftSizer->Add( sbSizer5, 1, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer2PAN;
	sbSizer2PAN = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Pan and Zoom:") ), wxVERTICAL );
	
	m_ZoomCenterOpt = new wxCheckBox( sbSizer2PAN->GetStaticBox(), wxID_ANY, _("Ce&nter and warp cursor on zoom"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ZoomCenterOpt->SetToolTip( _("Center the cursor on screen when zooming.") );
	
	sbSizer2PAN->Add( m_ZoomCenterOpt, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_MousewheelPANOpt = new wxCheckBox( sbSizer2PAN->GetStaticBox(), wxID_ANY, _("Use touchpad to pan"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MousewheelPANOpt->SetToolTip( _("Enable touchpad-friendly controls (pan with scroll action, zoom with Ctrl+scroll).") );
	
	sbSizer2PAN->Add( m_MousewheelPANOpt, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_AutoPANOpt = new wxCheckBox( sbSizer2PAN->GetStaticBox(), wxID_AUTOPAN, _("&Pan while moving object"), wxDefaultPosition, wxDefaultSize, 0 );
	m_AutoPANOpt->SetToolTip( _("When drawing a track or moving an item, pan when approaching the edge of the display.") );
	
	sbSizer2PAN->Add( m_AutoPANOpt, 0, wxALL, 5 );
	
	
	bLeftSizer->Add( sbSizer2PAN, 1, wxEXPAND|wxALL, 5 );
	
	
	bSizerUpper->Add( bLeftSizer, 2, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bMiddleLeftSizer;
	bMiddleLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_PolarDisplayChoices[] = { _("Cartesian coordinates"), _("Polar coordinates") };
	int m_PolarDisplayNChoices = sizeof( m_PolarDisplayChoices ) / sizeof( wxString );
	m_PolarDisplay = new wxRadioBox( this, wxID_POLAR_CTRL, _("Coordinates:"), wxDefaultPosition, wxDefaultSize, m_PolarDisplayNChoices, m_PolarDisplayChoices, 1, wxRA_SPECIFY_COLS );
	m_PolarDisplay->SetSelection( 0 );
	m_PolarDisplay->SetToolTip( _("Set display of relative (dx/dy) coordinates to Cartesian (rectangular) or polar (angle/distance).") );
	
	bMiddleLeftSizer->Add( m_PolarDisplay, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_UnitsSelectionChoices[] = { _("Inches"), _("Millimeters") };
	int m_UnitsSelectionNChoices = sizeof( m_UnitsSelectionChoices ) / sizeof( wxString );
	m_UnitsSelection = new wxRadioBox( this, wxID_UNITS, _("Units:"), wxDefaultPosition, wxDefaultSize, m_UnitsSelectionNChoices, m_UnitsSelectionChoices, 1, wxRA_SPECIFY_COLS );
	m_UnitsSelection->SetSelection( 0 );
	m_UnitsSelection->SetToolTip( _("Set units used to display dimensions and positions.") );
	
	bMiddleLeftSizer->Add( m_UnitsSelection, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* bOptionsSizer;
	bOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options:") ), wxVERTICAL );
	
	m_ShowGlobalRatsnest = new wxCheckBox( bOptionsSizer->GetStaticBox(), wxID_GENERAL_RATSNEST, _("&Show ratsnest"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ShowGlobalRatsnest->SetValue(true); 
	m_ShowGlobalRatsnest->SetToolTip( _("Show the full ratsnest.") );
	
	bOptionsSizer->Add( m_ShowGlobalRatsnest, 0, wxALL, 5 );
	
	m_Show_Page_Limits = new wxCheckBox( bOptionsSizer->GetStaticBox(), wxID_ANY, _("Show page limits"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Show_Page_Limits->SetValue(true); 
	bOptionsSizer->Add( m_Show_Page_Limits, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	
	bOptionsSizer->Add( 0, 0, 0, wxALL|wxEXPAND, 5 );
	
	m_Segments_45_Only_Ctrl = new wxCheckBox( bOptionsSizer->GetStaticBox(), wxID_SEGMENTS45, _("L&imit graphic lines to 45 degrees"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Segments_45_Only_Ctrl->SetToolTip( _("Force line segment directions to H, V or 45 degrees when drawing on technical layers.") );
	
	bOptionsSizer->Add( m_Segments_45_Only_Ctrl, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_UseEditKeyForWidth = new wxCheckBox( bOptionsSizer->GetStaticBox(), wxID_ANY, _("Edit action changes track width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_UseEditKeyForWidth->SetToolTip( _("When active, hitting Edit hotkey or double-clicking on a track or via changes its width/diameter to the one selected in the main toolbar. ") );
	
	bOptionsSizer->Add( m_UseEditKeyForWidth, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_dragSelects = new wxCheckBox( bOptionsSizer->GetStaticBox(), wxID_ANY, _("Prefer selection to dragging"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dragSelects->SetToolTip( _("When enabled and nothing is selected, drag gesture will draw a selection box, even if there are items under the cursor that could be immediately dragged.") );
	
	bOptionsSizer->Add( m_dragSelects, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	wxFlexGridSizer* fgSizer12;
	fgSizer12 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer12->AddGrowableCol( 1 );
	fgSizer12->SetFlexibleDirection( wxBOTH );
	fgSizer12->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextRotationAngle = new wxStaticText( bOptionsSizer->GetStaticBox(), wxID_ANY, _("&Rotation angle:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRotationAngle->Wrap( -1 );
	fgSizer12->Add( m_staticTextRotationAngle, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_RotationAngle = new wxTextCtrl( bOptionsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_RotationAngle->SetToolTip( _("Set increment (in degrees) for context menu and hotkey rotation.") );
	
	fgSizer12->Add( m_RotationAngle, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );
	
	
	bOptionsSizer->Add( fgSizer12, 1, wxEXPAND, 5 );
	
	
	bMiddleLeftSizer->Add( bOptionsSizer, 1, wxEXPAND|wxALL, 5 );
	
	
	bSizerUpper->Add( bMiddleLeftSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_MagneticPadOptCtrlChoices[] = { _("Never"), _("When creating tracks"), _("Always") };
	int m_MagneticPadOptCtrlNChoices = sizeof( m_MagneticPadOptCtrlChoices ) / sizeof( wxString );
	m_MagneticPadOptCtrl = new wxRadioBox( this, wxID_ANY, _("Magnetic Pads:"), wxDefaultPosition, wxDefaultSize, m_MagneticPadOptCtrlNChoices, m_MagneticPadOptCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_MagneticPadOptCtrl->SetSelection( 2 );
	m_MagneticPadOptCtrl->SetToolTip( _("Control capture of the cursor when the mouse enters a pad area.") );
	
	bRightSizer->Add( m_MagneticPadOptCtrl, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_MagneticTrackOptCtrlChoices[] = { _("Never"), _("When creating tracks"), _("Always") };
	int m_MagneticTrackOptCtrlNChoices = sizeof( m_MagneticTrackOptCtrlChoices ) / sizeof( wxString );
	m_MagneticTrackOptCtrl = new wxRadioBox( this, wxID_MAGNETIC_TRACKS, _("Magnetic Tracks:"), wxDefaultPosition, wxDefaultSize, m_MagneticTrackOptCtrlNChoices, m_MagneticTrackOptCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_MagneticTrackOptCtrl->SetSelection( 0 );
	m_MagneticTrackOptCtrl->SetToolTip( _("Control capture of the cursor when the mouse approaches a track.") );
	
	bRightSizer->Add( m_MagneticTrackOptCtrl, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* bLegacyOptionsSizer;
	bLegacyOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Legacy Routing Options:") ), wxVERTICAL );
	
	m_DrcOn = new wxCheckBox( bLegacyOptionsSizer->GetStaticBox(), wxID_DRC_ONOFF, _("&Enforce design rules when routing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DrcOn->SetValue(true); 
	m_DrcOn->SetToolTip( _("Enable DRC control. When DRC control is disabled, all connections are allowed.") );
	
	bLegacyOptionsSizer->Add( m_DrcOn, 0, wxALL|wxEXPAND, 5 );
	
	m_TrackAutodel = new wxCheckBox( bLegacyOptionsSizer->GetStaticBox(), wxID_TRACK_AUTODEL, _("Auto-delete old tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackAutodel->SetValue(true); 
	m_TrackAutodel->SetToolTip( _("Enable automatic track deletion when redrawing a track.") );
	
	bLegacyOptionsSizer->Add( m_TrackAutodel, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_Track_45_Only_Ctrl = new wxCheckBox( bLegacyOptionsSizer->GetStaticBox(), wxID_TRACKS45, _("&Limit tracks to 45 degrees"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Track_45_Only_Ctrl->SetValue(true); 
	m_Track_45_Only_Ctrl->SetToolTip( _("Force track directions to H, V or 45 degrees when drawing a track.") );
	
	bLegacyOptionsSizer->Add( m_Track_45_Only_Ctrl, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_Track_DoubleSegm_Ctrl = new wxCheckBox( bLegacyOptionsSizer->GetStaticBox(), wxID_ANY, _("&Use double segmented tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Track_DoubleSegm_Ctrl->SetValue(true); 
	m_Track_DoubleSegm_Ctrl->SetToolTip( _("Use two track segments, with 45 degrees angle between them, when drawing a new track ") );
	
	bLegacyOptionsSizer->Add( m_Track_DoubleSegm_Ctrl, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	
	bRightSizer->Add( bLegacyOptionsSizer, 1, wxEXPAND|wxALL, 5 );
	
	
	bSizerUpper->Add( bRightSizer, 0, wxALL|wxEXPAND, 5 );
	
	
	bMainSizer->Add( bSizerUpper, 0, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bMainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	// Connect Events
	m_scaleSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleSlider ), NULL, this );
	m_scaleAuto->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleAuto ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnOkClick ), NULL, this );
}

DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::~DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE()
{
	// Disconnect Events
	m_scaleSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleSlider ), NULL, this );
	m_scaleAuto->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnScaleAuto ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE::OnOkClick ), NULL, this );
	
}
