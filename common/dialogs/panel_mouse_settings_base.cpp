///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_mouse_settings_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_MOUSE_SETTINGS_BASE::PANEL_MOUSE_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	m_panZoomLabel = new wxStaticText( this, wxID_ANY, _("Pan and Zoom"), wxDefaultPosition, wxDefaultSize, 0 );
	m_panZoomLabel->Wrap( -1 );
	bSizer1->Add( m_panZoomLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer1->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_checkZoomCenter = new wxCheckBox( this, wxID_ANY, _("Center and warp cursor on zoom"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkZoomCenter->SetToolTip( _("Center the cursor on screen when zooming.") );

	gbSizer1->Add( m_checkZoomCenter, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALL, 5 );


	gbSizer1->Add( 30, 0, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_checkAutoPan = new wxCheckBox( this, wxID_ANY, _("Automatically pan while moving object"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkAutoPan->SetToolTip( _("When drawing a track or moving an item, pan when approaching the edge of the display.") );

	gbSizer1->Add( m_checkAutoPan, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	m_checkZoomAcceleration = new wxCheckBox( this, wxID_ANY, _("Use zoom acceleration"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkZoomAcceleration->SetToolTip( _("Zoom faster when scrolling quickly") );

	gbSizer1->Add( m_checkZoomAcceleration, wxGBPosition( 1, 0 ), wxGBSpan( 1, 3 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_zoomSizer = new wxBoxSizer( wxHORIZONTAL );

	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Zoom speed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	m_zoomSizer->Add( m_staticText1, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 8 );

	m_zoomSpeed = new wxSlider( this, wxID_ANY, 5, 1, 10, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	m_zoomSpeed->Enable( false );
	m_zoomSpeed->SetToolTip( _("How far to zoom in for each rotation of the mouse wheel") );
	m_zoomSpeed->SetMinSize( wxSize( 120,-1 ) );

	m_zoomSizer->Add( m_zoomSpeed, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 0 );

	m_checkAutoZoomSpeed = new wxCheckBox( this, wxID_ANY, _("Automatic"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkAutoZoomSpeed->SetValue(true);
	m_checkAutoZoomSpeed->SetToolTip( _("Pick the zoom speed automatically") );

	m_zoomSizer->Add( m_checkAutoZoomSpeed, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 10 );


	gbSizer1->Add( m_zoomSizer, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxEXPAND|wxTOP, 5 );

	m_panSizer = new wxBoxSizer( wxHORIZONTAL );

	m_staticText22 = new wxStaticText( this, wxID_ANY, _("Auto pan speed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText22->Wrap( -1 );
	m_panSizer->Add( m_staticText22, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 8 );

	m_autoPanSpeed = new wxSlider( this, wxID_ANY, 5, 1, 10, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	m_autoPanSpeed->SetToolTip( _("How fast to pan when moving an object off the edge of the screen") );
	m_autoPanSpeed->SetMinSize( wxSize( 120,-1 ) );

	m_panSizer->Add( m_autoPanSpeed, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 0 );


	gbSizer1->Add( m_panSizer, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxEXPAND|wxTOP, 5 );


	gbSizer1->AddGrowableCol( 0 );
	gbSizer1->AddGrowableCol( 1 );
	gbSizer1->AddGrowableCol( 2 );

	bSizer8->Add( gbSizer1, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizer1->Add( bSizer8, 0, wxEXPAND|wxRIGHT, 10 );


	bSizer1->Add( 0, 15, 0, wxEXPAND, 5 );

	m_dragLabel = new wxStaticText( this, wxID_ANY, _("Drag Gestures"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dragLabel->Wrap( -1 );
	bSizer1->Add( m_dragLabel, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer1->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );


	bSizer9->Add( 0, 5, 0, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgSizer1->AddGrowableCol( 2 );
	fgSizer1->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_leftButtonDragLabel = new wxStaticText( this, wxID_ANY, _("Left button drag:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_leftButtonDragLabel->Wrap( -1 );
	fgSizer1->Add( m_leftButtonDragLabel, 0, wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_choiceLeftButtonDragChoices[] = { _("Draw selection rectangle"), _("Drag selected objects; otherwise draw selection rectangle"), _("Drag any object (selected or not)") };
	int m_choiceLeftButtonDragNChoices = sizeof( m_choiceLeftButtonDragChoices ) / sizeof( wxString );
	m_choiceLeftButtonDrag = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceLeftButtonDragNChoices, m_choiceLeftButtonDragChoices, 0 );
	m_choiceLeftButtonDrag->SetSelection( 0 );
	fgSizer1->Add( m_choiceLeftButtonDrag, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Middle button drag:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	fgSizer1->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	wxString m_choiceMiddleButtonDragChoices[] = { _("Pan"), _("Zoom"), _("None") };
	int m_choiceMiddleButtonDragNChoices = sizeof( m_choiceMiddleButtonDragChoices ) / sizeof( wxString );
	m_choiceMiddleButtonDrag = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceMiddleButtonDragNChoices, m_choiceMiddleButtonDragChoices, 0 );
	m_choiceMiddleButtonDrag->SetSelection( 0 );
	fgSizer1->Add( m_choiceMiddleButtonDrag, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText31 = new wxStaticText( this, wxID_ANY, _("Right button drag:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText31->Wrap( -1 );
	fgSizer1->Add( m_staticText31, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	wxString m_choiceRightButtonDragChoices[] = { _("Pan"), _("Zoom"), _("None") };
	int m_choiceRightButtonDragNChoices = sizeof( m_choiceRightButtonDragChoices ) / sizeof( wxString );
	m_choiceRightButtonDrag = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceRightButtonDragNChoices, m_choiceRightButtonDragChoices, 0 );
	m_choiceRightButtonDrag->SetSelection( 0 );
	fgSizer1->Add( m_choiceRightButtonDrag, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_panMoveKeyLabel = new wxStaticText( this, wxID_ANY, _("Pan on mouse movement with key:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_panMoveKeyLabel->Wrap( -1 );
	fgSizer1->Add( m_panMoveKeyLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	wxString m_choicePanMoveKeyChoices[] = { _("None"), _("Alt"), _("Ctrl"), _("Shift") };
	int m_choicePanMoveKeyNChoices = sizeof( m_choicePanMoveKeyChoices ) / sizeof( wxString );
	m_choicePanMoveKey = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choicePanMoveKeyNChoices, m_choicePanMoveKeyChoices, 0 );
	m_choicePanMoveKey->SetSelection( 0 );
	fgSizer1->Add( m_choicePanMoveKey, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizer9->Add( fgSizer1, 1, wxEXPAND|wxALL, 5 );


	bSizer1->Add( bSizer9, 0, wxEXPAND|wxLEFT, 5 );


	bSizer1->Add( 0, 15, 0, wxEXPAND, 5 );

	m_scrollLabel = new wxStaticText( this, wxID_ANY, _("Scroll Gestures"), wxDefaultPosition, wxDefaultSize, 0 );
	m_scrollLabel->Wrap( -1 );
	bSizer1->Add( m_scrollLabel, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer1->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText21 = new wxStaticText( this, wxID_ANY, _("Vertical touchpad or scroll wheel movement:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	bSizer4->Add( m_staticText21, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_scrollWarning = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_scrollWarning->SetToolTip( _("Only one action can be assigned to each column") );

	bSizer4->Add( m_scrollWarning, 0, wxRIGHT|wxLEFT, 5 );


	bSizerLeft->Add( bSizer4, 0, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 6, 8, 0 );
	fgSizer2->AddGrowableCol( 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText19 = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText19->Wrap( -1 );
	fgSizer2->Add( m_staticText19, 0, wxALIGN_RIGHT|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticText17 = new wxStaticText( this, wxID_ANY, _("--"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText17->Wrap( -1 );
	fgSizer2->Add( m_staticText17, 0, wxTOP|wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_lblCtrl = new wxStaticText( this, wxID_ANY, _("Ctrl"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblCtrl->Wrap( -1 );
	fgSizer2->Add( m_lblCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText8 = new wxStaticText( this, wxID_ANY, _("Shift"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	fgSizer2->Add( m_staticText8, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_lblAlt = new wxStaticText( this, wxID_ANY, _("Alt"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblAlt->Wrap( -1 );
	fgSizer2->Add( m_lblAlt, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText18 = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	fgSizer2->Add( m_staticText18, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_staticText10 = new wxStaticText( this, wxID_ANY, _("Zoom:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	fgSizer2->Add( m_staticText10, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_rbZoomNone = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	fgSizer2->Add( m_rbZoomNone, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_rbZoomCtrl = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_rbZoomCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

	m_rbZoomShift = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_rbZoomShift, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_rbZoomAlt = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_rbZoomAlt, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkZoomReverse = new wxCheckBox( this, wxID_ANY, _("Reverse"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_checkZoomReverse, 0, wxRIGHT|wxLEFT, 5 );

	m_staticText11 = new wxStaticText( this, wxID_ANY, _("Pan up/down:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	fgSizer2->Add( m_staticText11, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_rbPanVNone = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	fgSizer2->Add( m_rbPanVNone, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

	m_rbPanVCtrl = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_rbPanVCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

	m_rbPanVShift = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_rbPanVShift, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_rbPanVAlt = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_rbPanVAlt, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_staticText211 = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText211->Wrap( -1 );
	fgSizer2->Add( m_staticText211, 0, wxALL, 5 );

	m_staticText20 = new wxStaticText( this, wxID_ANY, _("Pan left/right:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText20->Wrap( -1 );
	fgSizer2->Add( m_staticText20, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_rbPanHNone = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	fgSizer2->Add( m_rbPanHNone, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

	m_rbPanHCtrl = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_rbPanHCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

	m_rbPanHShift = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_rbPanHShift, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_rbPanHAlt = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_rbPanHAlt, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_checkPanHReverse = new wxCheckBox( this, wxID_ANY, _("Reverse"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_checkPanHReverse, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	fgSizer2->Add( 0, 10, 1, wxEXPAND, 5 );


	fgSizer2->Add( 42, 0, 1, wxEXPAND, 5 );


	fgSizer2->Add( 42, 0, 1, wxEXPAND, 5 );


	fgSizer2->Add( 42, 0, 1, wxEXPAND, 5 );


	fgSizer2->Add( 42, 0, 1, wxEXPAND, 5 );


	bSizerLeft->Add( fgSizer2, 0, wxRIGHT|wxLEFT, 24 );

	m_checkEnablePanH = new wxCheckBox( this, wxID_ANY, _("Pan left/right with horizontal movement"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkEnablePanH->SetToolTip( _("Pan the canvas left and right when scrolling left to right on the touchpad") );

	bSizerLeft->Add( m_checkEnablePanH, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bMargins->Add( bSizerLeft, 1, wxEXPAND|wxLEFT, 5 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );

	m_mouseDefaults = new wxButton( this, wxID_ANY, _("Reset to Mouse Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_mouseDefaults, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_trackpadDefaults = new wxButton( this, wxID_ANY, _("Reset to Trackpad Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_trackpadDefaults, 0, wxEXPAND|wxALL, 5 );


	bMargins->Add( bSizerRight, 0, wxEXPAND|wxLEFT, 5 );


	bSizer1->Add( bMargins, 1, wxEXPAND|wxTOP|wxRIGHT, 10 );


	bSizer10->Add( bSizer1, 1, 0, 5 );


	this->SetSizer( bSizer10 );
	this->Layout();
	bSizer10->Fit( this );

	// Connect Events
	m_rbZoomNone->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbZoomCtrl->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbZoomShift->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbZoomAlt->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbPanVNone->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbPanVCtrl->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbPanVShift->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbPanVAlt->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbPanHNone->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbPanHCtrl->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbPanHShift->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbPanHAlt->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_mouseDefaults->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::onMouseDefaults ), NULL, this );
	m_trackpadDefaults->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::onTrackpadDefaults ), NULL, this );
}

PANEL_MOUSE_SETTINGS_BASE::~PANEL_MOUSE_SETTINGS_BASE()
{
	// Disconnect Events
	m_rbZoomNone->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbZoomCtrl->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbZoomShift->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbZoomAlt->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbPanVNone->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbPanVCtrl->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbPanVShift->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbPanVAlt->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbPanHNone->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbPanHCtrl->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbPanHShift->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_rbPanHAlt->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::OnScrollRadioButton ), NULL, this );
	m_mouseDefaults->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::onMouseDefaults ), NULL, this );
	m_trackpadDefaults->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_MOUSE_SETTINGS_BASE::onTrackpadDefaults ), NULL, this );

}
