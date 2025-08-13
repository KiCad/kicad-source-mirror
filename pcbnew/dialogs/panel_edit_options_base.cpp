///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_edit_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_EDIT_OPTIONS_BASE::PANEL_EDIT_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bMiddleLeftSizer;
	bMiddleLeftSizer = new wxBoxSizer( wxVERTICAL );

	m_staticText31 = new wxStaticText( this, wxID_ANY, _("Editing Options"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText31->Wrap( -1 );
	bMiddleLeftSizer->Add( m_staticText31, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMiddleLeftSizer->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizerUniversal;
	bSizerUniversal = new wxBoxSizer( wxVERTICAL );

	m_cbConstrainHV45Mode = new wxCheckBox( this, wxID_ANY, _("Constrain actions to H, V, 45 degrees"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerUniversal->Add( m_cbConstrainHV45Mode, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	wxBoxSizer* bSizerRotationStep;
	bSizerRotationStep = new wxBoxSizer( wxHORIZONTAL );

	m_rotationAngleLabel = new wxStaticText( this, wxID_ANY, _("Step for &rotate commands:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rotationAngleLabel->Wrap( -1 );
	bSizerRotationStep->Add( m_rotationAngleLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_rotationAngleCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_rotationAngleCtrl->SetToolTip( _("Set increment (in degrees) for context menu and hotkey rotation.") );
	m_rotationAngleCtrl->SetMinSize( wxSize( 60,-1 ) );

	bSizerRotationStep->Add( m_rotationAngleCtrl, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_rotationAngleUnits = new wxStaticText( this, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rotationAngleUnits->Wrap( -1 );
	bSizerRotationStep->Add( m_rotationAngleUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	bSizerUniversal->Add( bSizerRotationStep, 0, wxEXPAND, 5 );


	bSizerUniversal->Add( 0, 3, 0, 0, 5 );

	m_arcEditModeLabel = new wxStaticText( this, wxID_ANY, _("Arc editing mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_arcEditModeLabel->Wrap( -1 );
	bSizerUniversal->Add( m_arcEditModeLabel, 0, wxLEFT, 5 );


	bSizerUniversal->Add( 0, 3, 0, wxEXPAND, 5 );

	wxString m_arcEditModeChoices[] = { _("Keep center, adjust radius"), _("Keep endpoints or direction of starting point"), _("Keep center and radius, adjust endpoints") };
	int m_arcEditModeNChoices = sizeof( m_arcEditModeChoices ) / sizeof( wxString );
	m_arcEditMode = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_arcEditModeNChoices, m_arcEditModeChoices, 0 );
	m_arcEditMode->SetSelection( 0 );
	bSizerUniversal->Add( m_arcEditMode, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bMiddleLeftSizer->Add( bSizerUniversal, 0, wxEXPAND|wxALL, 5 );

	m_sizerBoardEdit = new wxBoxSizer( wxVERTICAL );


	m_sizerBoardEdit->Add( 0, 3, 0, wxEXPAND, 5 );

	m_trackMouseDragLabel = new wxStaticText( this, wxID_ANY, _("Track mouse-drag mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trackMouseDragLabel->Wrap( -1 );
	m_sizerBoardEdit->Add( m_trackMouseDragLabel, 0, wxRIGHT|wxLEFT, 5 );


	m_sizerBoardEdit->Add( 0, 3, 0, wxEXPAND, 5 );

	wxString m_trackMouseDragCtrlChoices[] = { _("Move"), _("Drag (45 degree mode)"), _("Drag (free angle)") };
	int m_trackMouseDragCtrlNChoices = sizeof( m_trackMouseDragCtrlChoices ) / sizeof( wxString );
	m_trackMouseDragCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_trackMouseDragCtrlNChoices, m_trackMouseDragCtrlChoices, 0 );
	m_trackMouseDragCtrl->SetSelection( 0 );
	m_sizerBoardEdit->Add( m_trackMouseDragCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	wxBoxSizer* bSizerFlip;
	bSizerFlip = new wxBoxSizer( wxHORIZONTAL );

	m_staticText33 = new wxStaticText( this, wxID_ANY, _("Flip board items:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText33->Wrap( -1 );
	bSizerFlip->Add( m_staticText33, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_rbFlipLeftRight = new wxRadioButton( this, wxID_ANY, _("Left/right"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerFlip->Add( m_rbFlipLeftRight, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );

	m_rbFlipTopBottom = new wxRadioButton( this, wxID_ANY, _("Top/bottom"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerFlip->Add( m_rbFlipTopBottom, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );


	m_sizerBoardEdit->Add( bSizerFlip, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_allowFreePads = new wxCheckBox( this, wxID_ANY, _("Allow free pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_allowFreePads->SetToolTip( _("If checked, pads can be moved with respect to the rest of the footprint.") );

	m_sizerBoardEdit->Add( m_allowFreePads, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );


	bMiddleLeftSizer->Add( m_sizerBoardEdit, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticText32 = new wxStaticText( this, wxID_ANY, _("Left Click Mouse Commands"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText32->Wrap( -1 );
	bMiddleLeftSizer->Add( m_staticText32, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline4 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMiddleLeftSizer->Add( m_staticline4, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_mouseCmdsWinLin = new wxBoxSizer( wxVERTICAL );

	m_stHint1 = new wxStaticText( this, wxID_ANY, _("Left click (and drag) actions depend on 2 modifier keys:\nShift and Ctrl"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHint1->Wrap( -1 );
	m_mouseCmdsWinLin->Add( m_stHint1, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizerCmdsWinLin;
	fgSizerCmdsWinLin = new wxFlexGridSizer( 0, 2, 8, 0 );
	fgSizerCmdsWinLin->SetFlexibleDirection( wxBOTH );
	fgSizerCmdsWinLin->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* staticText61;
	staticText61 = new wxStaticText( this, wxID_ANY, _("Click:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText61->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText61, 0, wxRIGHT|wxLEFT, 5 );

	wxStaticText* staticText71;
	staticText71 = new wxStaticText( this, wxID_ANY, _("Select item(s)"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText71->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText71, 0, wxRIGHT|wxLEFT, 5 );

	wxStaticText* staticText611;
	staticText611 = new wxStaticText( this, wxID_ANY, _("Long Click:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText611->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText611, 0, wxRIGHT|wxLEFT, 5 );

	wxStaticText* staticText711;
	staticText711 = new wxStaticText( this, wxID_ANY, _("Clarify selection from menu"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText711->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText711, 0, wxRIGHT|wxLEFT, 5 );

	wxStaticText* staticText81;
	staticText81 = new wxStaticText( this, wxID_ANY, _("Shift:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText81->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText81, 0, wxRIGHT|wxLEFT, 5 );

	wxStaticText* staticText91;
	staticText91 = new wxStaticText( this, wxID_ANY, _("Add item(s) to selection"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText91->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText91, 0, wxRIGHT|wxLEFT, 5 );

	wxStaticText* staticText121;
	staticText121 = new wxStaticText( this, wxID_ANY, _("Ctrl+Shift:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText121->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText121, 0, wxRIGHT|wxLEFT, 5 );

	wxStaticText* staticText131;
	staticText131 = new wxStaticText( this, wxID_ANY, _("Remove item(s) from selection"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText131->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText131, 0, wxRIGHT|wxLEFT, 5 );

	wxStaticText* staticText161;
	staticText161 = new wxStaticText( this, wxID_ANY, _("Ctrl:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText161->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText161, 0, wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxVERTICAL );

	m_rbToggleSel = new wxRadioButton( this, wxID_ANY, _("Toggle selection"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizer16->Add( m_rbToggleSel, 0, 0, 5 );

	m_rbHighlightNet = new wxRadioButton( this, wxID_ANY, _("Highlight net (for pads/tracks)"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer16->Add( m_rbHighlightNet, 0, wxTOP, 2 );


	fgSizerCmdsWinLin->Add( bSizer16, 1, wxEXPAND|wxLEFT, 5 );


	m_mouseCmdsWinLin->Add( fgSizerCmdsWinLin, 1, wxEXPAND|wxTOP, 5 );


	bMiddleLeftSizer->Add( m_mouseCmdsWinLin, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_mouseCmdsOSX = new wxBoxSizer( wxVERTICAL );

	m_stHint2 = new wxStaticText( this, wxID_ANY, _("Left click (and drag) actions depend on 3 modifier keys:\nOption, Shift and Cmd"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHint2->Wrap( -1 );
	m_mouseCmdsOSX->Add( m_stHint2, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizerCmdsOSX;
	fgSizerCmdsOSX = new wxFlexGridSizer( 0, 2, 8, 0 );
	fgSizerCmdsOSX->SetFlexibleDirection( wxBOTH );
	fgSizerCmdsOSX->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* staticText62;
	staticText62 = new wxStaticText( this, wxID_ANY, _("Click:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText62->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText62, 0, wxRIGHT|wxLEFT, 5 );

	wxStaticText* staticText72;
	staticText72 = new wxStaticText( this, wxID_ANY, _("Select item(s)"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText72->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText72, 0, wxRIGHT|wxLEFT, 5 );

	wxStaticText* staticText162;
	staticText162 = new wxStaticText( this, wxID_ANY, _("Long Click:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText162->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText162, 0, wxRIGHT|wxLEFT, 5 );

	wxStaticText* staticText172;
	staticText172 = new wxStaticText( this, wxID_ANY, _("Clarify selection from menu"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText172->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText172, 0, wxRIGHT|wxLEFT, 5 );

	wxStaticText* staticText82;
	staticText82 = new wxStaticText( this, wxID_ANY, _("Shift:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText82->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText82, 0, wxRIGHT|wxLEFT, 5 );

	wxStaticText* staticText92;
	staticText92 = new wxStaticText( this, wxID_ANY, _("Add item(s) to selection"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText92->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText92, 0, wxRIGHT|wxLEFT, 5 );

	wxStaticText* staticText122;
	staticText122 = new wxStaticText( this, wxID_ANY, _("Shift+Cmd:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText122->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText122, 0, wxRIGHT|wxLEFT, 5 );

	wxStaticText* staticText132;
	staticText132 = new wxStaticText( this, wxID_ANY, _("Remove item(s) from selection"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText132->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText132, 0, wxRIGHT|wxLEFT, 5 );

	wxStaticText* staticText142;
	staticText142 = new wxStaticText( this, wxID_ANY, _("Cmd:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText142->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText142, 0, wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer161;
	bSizer161 = new wxBoxSizer( wxVERTICAL );

	m_rbToggleSelMac = new wxRadioButton( this, wxID_ANY, _("Toggle selection"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizer161->Add( m_rbToggleSelMac, 0, 0, 5 );

	m_rbHighlightNetMac = new wxRadioButton( this, wxID_ANY, _("Highlight net (for pads/tracks)"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer161->Add( m_rbHighlightNetMac, 0, wxTOP, 2 );


	fgSizerCmdsOSX->Add( bSizer161, 1, wxEXPAND|wxLEFT, 5 );

	wxStaticText* staticText102;
	staticText102 = new wxStaticText( this, wxID_ANY, _("Option:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText102->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText102, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	wxStaticText* staticText112;
	staticText112 = new wxStaticText( this, wxID_ANY, _("Clarify selection from menu"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText112->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText112, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	m_mouseCmdsOSX->Add( fgSizerCmdsOSX, 1, wxEXPAND|wxTOP, 5 );


	bMiddleLeftSizer->Add( m_mouseCmdsOSX, 1, wxEXPAND|wxALL, 5 );


	bMargins->Add( bMiddleLeftSizer, 0, wxEXPAND|wxRIGHT, 10 );


	bMargins->Add( 20, 0, 0, wxEXPAND, 5 );

	m_optionsBook = new wxSimplebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxPanel* fpPage;
	fpPage = new wxPanel( m_optionsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* fpOptionsSizer;
	fpOptionsSizer = new wxBoxSizer( wxVERTICAL );

	m_staticText34 = new wxStaticText( fpPage, wxID_ANY, _("Magnetic Points"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText34->Wrap( -1 );
	fpOptionsSizer->Add( m_staticText34, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline5 = new wxStaticLine( fpPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fpOptionsSizer->Add( m_staticline5, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );

	m_magneticPads = new wxCheckBox( fpPage, wxID_ANY, _("Magnetic pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_magneticPads->SetValue(true);
	bSizer13->Add( m_magneticPads, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_magneticGraphics = new wxCheckBox( fpPage, wxID_ANY, _("Magnetic graphics"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer13->Add( m_magneticGraphics, 0, wxBOTTOM|wxLEFT, 5 );


	fpOptionsSizer->Add( bSizer13, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	fpPage->SetSizer( fpOptionsSizer );
	fpPage->Layout();
	fpOptionsSizer->Fit( fpPage );
	m_optionsBook->AddPage( fpPage, _("a page"), false );
	wxPanel* pcbPage;
	pcbPage = new wxPanel( m_optionsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* pcbOptionsSizer;
	pcbOptionsSizer = new wxBoxSizer( wxVERTICAL );

	stMagneticPtsLabel1 = new wxStaticText( pcbPage, wxID_ANY, _("Magnetic Points"), wxDefaultPosition, wxDefaultSize, 0 );
	stMagneticPtsLabel1->Wrap( -1 );
	pcbOptionsSizer->Add( stMagneticPtsLabel1, 0, wxTOP|wxLEFT, 13 );

	m_staticline6 = new wxStaticLine( pcbPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	pcbOptionsSizer->Add( m_staticline6, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxFlexGridSizer* fgMagneticPoints;
	fgMagneticPoints = new wxFlexGridSizer( 0, 2, 5, 0 );
	fgMagneticPoints->SetFlexibleDirection( wxVERTICAL );
	fgMagneticPoints->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText2 = new wxStaticText( pcbPage, wxID_ANY, _("Snap to pads:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	m_staticText2->SetToolTip( _("Capture cursor when the mouse enters a pad area") );

	fgMagneticPoints->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxString m_magneticPadChoiceChoices[] = { _("Never"), _("When routing tracks"), _("Always") };
	int m_magneticPadChoiceNChoices = sizeof( m_magneticPadChoiceChoices ) / sizeof( wxString );
	m_magneticPadChoice = new wxChoice( pcbPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_magneticPadChoiceNChoices, m_magneticPadChoiceChoices, 0 );
	m_magneticPadChoice->SetSelection( 1 );
	m_magneticPadChoice->SetToolTip( _("Capture cursor when the mouse enters a pad area") );

	fgMagneticPoints->Add( m_magneticPadChoice, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticText21 = new wxStaticText( pcbPage, wxID_ANY, _("Snap to tracks and vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	m_staticText21->SetToolTip( _("Capture cursor when the mouse approaches a track") );

	fgMagneticPoints->Add( m_staticText21, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_magneticTrackChoiceChoices[] = { _("Never"), _("When routing tracks"), _("Always") };
	int m_magneticTrackChoiceNChoices = sizeof( m_magneticTrackChoiceChoices ) / sizeof( wxString );
	m_magneticTrackChoice = new wxChoice( pcbPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_magneticTrackChoiceNChoices, m_magneticTrackChoiceChoices, 0 );
	m_magneticTrackChoice->SetSelection( 1 );
	m_magneticTrackChoice->SetToolTip( _("Capture cursor when the mouse approaches a track") );

	fgMagneticPoints->Add( m_magneticTrackChoice, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_staticText211 = new wxStaticText( pcbPage, wxID_ANY, _("Snap to graphics:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText211->Wrap( -1 );
	m_staticText211->SetToolTip( _("Capture cursor when the mouse approaches graphical control points") );

	fgMagneticPoints->Add( m_staticText211, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxString m_magneticGraphicsChoiceChoices[] = { _("Always"), _("Never") };
	int m_magneticGraphicsChoiceNChoices = sizeof( m_magneticGraphicsChoiceChoices ) / sizeof( wxString );
	m_magneticGraphicsChoice = new wxChoice( pcbPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_magneticGraphicsChoiceNChoices, m_magneticGraphicsChoiceChoices, 0 );
	m_magneticGraphicsChoice->SetSelection( 0 );
	m_magneticGraphicsChoice->SetToolTip( _("Capture cursor when the mouse approaches graphical control points") );

	fgMagneticPoints->Add( m_magneticGraphicsChoice, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	pcbOptionsSizer->Add( fgMagneticPoints, 0, wxEXPAND|wxALL, 5 );


	pcbOptionsSizer->Add( 0, 5, 0, wxEXPAND, 5 );

	stRatsnestLabel = new wxStaticText( pcbPage, wxID_ANY, _("Ratsnest"), wxDefaultPosition, wxDefaultSize, 0 );
	stRatsnestLabel->Wrap( -1 );
	pcbOptionsSizer->Add( stRatsnestLabel, 0, wxTOP|wxLEFT, 13 );

	m_staticline7 = new wxStaticLine( pcbPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	pcbOptionsSizer->Add( m_staticline7, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bRatsnest;
	bRatsnest = new wxBoxSizer( wxVERTICAL );

	m_showSelectedRatsnest = new wxCheckBox( pcbPage, wxID_ANY, _("Always show selected ratsnest"), wxDefaultPosition, wxDefaultSize, 0 );
	bRatsnest->Add( m_showSelectedRatsnest, 0, wxALL, 5 );

	m_OptDisplayCurvedRatsnestLines = new wxCheckBox( pcbPage, wxID_ANY, _("Show ratsnest with curved lines"), wxDefaultPosition, wxDefaultSize, 0 );
	bRatsnest->Add( m_OptDisplayCurvedRatsnestLines, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bRatsnestLineThickness;
	bRatsnestLineThickness = new wxBoxSizer( wxHORIZONTAL );

	m_ratsnestThicknessLabel = new wxStaticText( pcbPage, wxID_ANY, _("Ratsnest line thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ratsnestThicknessLabel->Wrap( -1 );
	bRatsnestLineThickness->Add( m_ratsnestThicknessLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_ratsnestThickness = new wxSpinCtrlDouble( pcbPage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0.5, 10, 0.5, 0.5 );
	m_ratsnestThickness->SetDigits( 1 );
	bRatsnestLineThickness->Add( m_ratsnestThickness, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bRatsnest->Add( bRatsnestLineThickness, 1, wxEXPAND|wxBOTTOM, 5 );


	pcbOptionsSizer->Add( bRatsnest, 0, wxEXPAND|wxALL, 5 );

	stMiscellaneousLabel = new wxStaticText( pcbPage, wxID_ANY, _("Miscellaneous"), wxDefaultPosition, wxDefaultSize, 0 );
	stMiscellaneousLabel->Wrap( -1 );
	pcbOptionsSizer->Add( stMiscellaneousLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline8 = new wxStaticLine( pcbPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	pcbOptionsSizer->Add( m_staticline8, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bMiscellaneous;
	bMiscellaneous = new wxBoxSizer( wxVERTICAL );

	m_escClearsNetHighlight = new wxCheckBox( pcbPage, wxID_ANY, _("<ESC> clears net highlighting"), wxDefaultPosition, wxDefaultSize, 0 );
	m_escClearsNetHighlight->SetValue(true);
	bMiscellaneous->Add( m_escClearsNetHighlight, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_showPageLimits = new wxCheckBox( pcbPage, wxID_ANY, _("Show page limits"), wxDefaultPosition, wxDefaultSize, 0 );
	m_showPageLimits->SetValue(true);
	m_showPageLimits->SetToolTip( _("Draw an outline to show the sheet size.") );

	bMiscellaneous->Add( m_showPageLimits, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbCourtyardCollisions = new wxCheckBox( pcbPage, wxID_ANY, _("Show courtyard collisions when moving/dragging"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbCourtyardCollisions->SetValue(true);
	bMiscellaneous->Add( m_cbCourtyardCollisions, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_autoRefillZones = new wxCheckBox( pcbPage, wxID_ANY, _("Automatically refill zones"), wxDefaultPosition, wxDefaultSize, 0 );
	m_autoRefillZones->SetValue(true);
	m_autoRefillZones->SetToolTip( _("If checked, zones will be re-filled after each edit operation") );

	bMiscellaneous->Add( m_autoRefillZones, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	pcbOptionsSizer->Add( bMiscellaneous, 1, wxEXPAND|wxALL, 5 );


	pcbPage->SetSizer( pcbOptionsSizer );
	pcbPage->Layout();
	pcbOptionsSizer->Fit( pcbPage );
	m_optionsBook->AddPage( pcbPage, _("a page"), false );

	bMargins->Add( m_optionsBook, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bPanelSizer->Add( bMargins, 1, wxRIGHT, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_EDIT_OPTIONS_BASE::~PANEL_EDIT_OPTIONS_BASE()
{
}
