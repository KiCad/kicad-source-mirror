///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_general_options_BoardEditor_base.h"

///////////////////////////////////////////////////////////////////////////

DialogGeneralOptionsBoardEditor_base::DialogGeneralOptionsBoardEditor_base( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );
	
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
	
	wxString m_CursorShapeChoices[] = { _("Smass cross"), _("Full screen cursor") };
	int m_CursorShapeNChoices = sizeof( m_CursorShapeChoices ) / sizeof( wxString );
	m_CursorShape = new wxRadioBox( this, wxID_CURSOR_SHAPE, _("Cursor"), wxDefaultPosition, wxDefaultSize, m_CursorShapeNChoices, m_CursorShapeChoices, 1, wxRA_SPECIFY_COLS );
	m_CursorShape->SetSelection( 1 );
	m_CursorShape->SetToolTip( _("Main cursor shape selection (small cross or large cursor)") );
	
	bLeftSizer->Add( m_CursorShape, 0, wxALL|wxEXPAND, 5 );
	
	bMainSizer->Add( bLeftSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bMiddleLeftSizer;
	bMiddleLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_LayerNumberChoices[] = { _("1"), _("2"), _("4"), _("6"), _("8"), _("10"), _("12"), _("14"), _("16") };
	int m_LayerNumberNChoices = sizeof( m_LayerNumberChoices ) / sizeof( wxString );
	m_LayerNumber = new wxRadioBox( this, wxID_LAYER_NUMBER, _("Layers:"), wxDefaultPosition, wxDefaultSize, m_LayerNumberNChoices, m_LayerNumberChoices, 3, wxRA_SPECIFY_COLS );
	m_LayerNumber->SetSelection( 1 );
	m_LayerNumber->SetToolTip( _("Active copper layers count selection") );
	
	bMiddleLeftSizer->Add( m_LayerNumber, 0, wxALL|wxEXPAND, 5 );
	
	m_staticTextmaxlinks = new wxStaticText( this, wxID_ANY, _("Max Links:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextmaxlinks->Wrap( -1 );
	bMiddleLeftSizer->Add( m_staticTextmaxlinks, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_MaxShowLinks = new wxSpinCtrl( this, wxID_ANY, wxT("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 5, 1 );
	m_MaxShowLinks->SetToolTip( _("Adjust the number of ratsnets shown from cursor to closest pads") );
	
	bMiddleLeftSizer->Add( m_MaxShowLinks, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_staticTextautosave = new wxStaticText( this, wxID_ANY, _("Auto Save (minuts):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextautosave->Wrap( -1 );
	bMiddleLeftSizer->Add( m_staticTextautosave, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_SaveTime = new wxSpinCtrl( this, wxID_ANY, wxT("0"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 60, 0 );
	m_SaveTime->SetToolTip( _("Delay after the first change to create a backup file of the board on disk.") );
	
	bMiddleLeftSizer->Add( m_SaveTime, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	bMainSizer->Add( bMiddleLeftSizer, 1, wxEXPAND, 5 );
	
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
	
	m_AutoPANOpt = new wxCheckBox( this, wxID_AUTOPAN, _("Auto PAN"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_AutoPANOpt->SetToolTip( _("Allows auto pan when creating a track, or moving an item.") );
	
	bMiddleRightBoxSizer->Add( m_AutoPANOpt, 0, wxALL, 5 );
	
	m_Track_DoubleSegm_Ctrl = new wxCheckBox( this, wxID_ANY, _("Double Segm Track"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_Track_DoubleSegm_Ctrl->SetToolTip( _("If enabled, uses two track segments, with 45 degrees angle between them when creating a new track ") );
	
	bMiddleRightBoxSizer->Add( m_Track_DoubleSegm_Ctrl, 0, wxALL, 5 );
	
	bMainSizer->Add( bMiddleRightBoxSizer, 1, 0, 5 );
	
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
	
	m_buttonOK = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonOK->SetDefault(); 
	m_buttonOK->SetForegroundColour( wxColour( 174, 0, 0 ) );
	
	bRightSizer->Add( m_buttonOK, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_buttonCANCEL = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonCANCEL->SetForegroundColour( wxColour( 0, 0, 200 ) );
	
	bRightSizer->Add( m_buttonCANCEL, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	bMainSizer->Add( bRightSizer, 1, wxEXPAND, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DialogGeneralOptionsBoardEditor_base::OnInitDialog ) );
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogGeneralOptionsBoardEditor_base::OnOkClick ), NULL, this );
	m_buttonCANCEL->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogGeneralOptionsBoardEditor_base::OnCancelClick ), NULL, this );
}

DialogGeneralOptionsBoardEditor_base::~DialogGeneralOptionsBoardEditor_base()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DialogGeneralOptionsBoardEditor_base::OnInitDialog ) );
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogGeneralOptionsBoardEditor_base::OnOkClick ), NULL, this );
	m_buttonCANCEL->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogGeneralOptionsBoardEditor_base::OnCancelClick ), NULL, this );
}
