///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-39-g3487c3cb)
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

	wxStaticBoxSizer* bOptionsSizer;
	bOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Editing Options") ), wxVERTICAL );

	m_sizerBoardEdit = new wxBoxSizer( wxVERTICAL );

	m_flipLeftRight = new wxCheckBox( bOptionsSizer->GetStaticBox(), wxID_ANY, _("Flip board items L/R (default is T/B)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizerBoardEdit->Add( m_flipLeftRight, 0, wxBOTTOM|wxLEFT, 5 );

	m_allowFreePads = new wxCheckBox( bOptionsSizer->GetStaticBox(), wxID_ANY, _("Allow free pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_allowFreePads->SetToolTip( _("If checked, pads can be moved with respect to the rest of the footprint.") );

	m_sizerBoardEdit->Add( m_allowFreePads, 0, wxBOTTOM|wxLEFT, 5 );

	m_staticline3 = new wxStaticLine( bOptionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_sizerBoardEdit->Add( m_staticline3, 0, wxEXPAND|wxBOTTOM, 6 );


	bOptionsSizer->Add( m_sizerBoardEdit, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerUniversal;
	bSizerUniversal = new wxBoxSizer( wxVERTICAL );

	m_cbConstrainHV45Mode = new wxCheckBox( bOptionsSizer->GetStaticBox(), wxID_ANY, _("Constrain actions to H, V, 45 degrees"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerUniversal->Add( m_cbConstrainHV45Mode, 0, wxBOTTOM|wxTOP|wxLEFT, 5 );

	wxBoxSizer* bSizerRotationStep;
	bSizerRotationStep = new wxBoxSizer( wxHORIZONTAL );

	m_rotationAngleLabel = new wxStaticText( bOptionsSizer->GetStaticBox(), wxID_ANY, _("Step for &rotate commands:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rotationAngleLabel->Wrap( -1 );
	bSizerRotationStep->Add( m_rotationAngleLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_rotationAngleCtrl = new wxTextCtrl( bOptionsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_rotationAngleCtrl->SetToolTip( _("Set increment (in degrees) for context menu and hotkey rotation.") );
	m_rotationAngleCtrl->SetMinSize( wxSize( 60,-1 ) );

	bSizerRotationStep->Add( m_rotationAngleCtrl, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_rotationAngleUnits = new wxStaticText( bOptionsSizer->GetStaticBox(), wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rotationAngleUnits->Wrap( -1 );
	bSizerRotationStep->Add( m_rotationAngleUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	bSizerUniversal->Add( bSizerRotationStep, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer101;
	bSizer101 = new wxBoxSizer( wxVERTICAL );

	m_staticline4 = new wxStaticLine( bOptionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer101->Add( m_staticline4, 0, wxEXPAND|wxBOTTOM, 6 );

	m_arcEditModeLabel = new wxStaticText( bOptionsSizer->GetStaticBox(), wxID_ANY, _("Arc editing mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_arcEditModeLabel->Wrap( -1 );
	bSizer101->Add( m_arcEditModeLabel, 0, wxTOP|wxLEFT, 5 );


	bSizer101->Add( 0, 3, 0, wxEXPAND, 5 );

	wxString m_arcEditModeChoices[] = { _("Keep center, adjust radius"), _("Keep endpoints or direction of starting point") };
	int m_arcEditModeNChoices = sizeof( m_arcEditModeChoices ) / sizeof( wxString );
	m_arcEditMode = new wxChoice( bOptionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_arcEditModeNChoices, m_arcEditModeChoices, 0 );
	m_arcEditMode->SetSelection( 0 );
	bSizer101->Add( m_arcEditMode, 0, wxBOTTOM|wxLEFT, 5 );


	bSizerUniversal->Add( bSizer101, 1, wxEXPAND, 5 );


	bOptionsSizer->Add( bSizerUniversal, 0, wxEXPAND, 5 );


	bMiddleLeftSizer->Add( bOptionsSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_mouseCmdsWinLin = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Left Click Mouse Commands") ), wxVERTICAL );

	m_staticText181 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Left click (and drag) actions depend on 2 modifier keys:\nShift and Ctrl."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText181->Wrap( -1 );
	m_mouseCmdsWinLin->Add( m_staticText181, 0, wxALL, 5 );

	wxStaticLine* staticline11;
	staticline11 = new wxStaticLine( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_mouseCmdsWinLin->Add( staticline11, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxFlexGridSizer* fgSizerCmdsWinLin;
	fgSizerCmdsWinLin = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizerCmdsWinLin->SetFlexibleDirection( wxBOTH );
	fgSizerCmdsWinLin->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* staticText61;
	staticText61 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("No modifier"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText61->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText61, 0, wxALL, 5 );

	wxStaticText* staticText71;
	staticText71 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Select item(s)."), wxDefaultPosition, wxDefaultSize, 0 );
	staticText71->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText71, 0, wxALL, 5 );

	wxStaticText* staticText611;
	staticText611 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Long Click"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText611->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText611, 0, wxALL, 5 );

	wxStaticText* staticText711;
	staticText711 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Clarify selection from menu."), wxDefaultPosition, wxDefaultSize, 0 );
	staticText711->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText711, 0, wxALL, 5 );

	wxStaticText* staticText81;
	staticText81 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Shift"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText81->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText81, 0, wxALL, 5 );

	wxStaticText* staticText91;
	staticText91 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Add item(s) to selection."), wxDefaultPosition, wxDefaultSize, 0 );
	staticText91->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText91, 0, wxALL, 5 );

	wxStaticText* staticText121;
	staticText121 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Ctrl+Shift"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText121->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText121, 0, wxALL, 5 );

	wxStaticText* staticText131;
	staticText131 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Remove item(s) from selection."), wxDefaultPosition, wxDefaultSize, 0 );
	staticText131->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText131, 0, wxALL, 5 );

	wxStaticText* staticText161;
	staticText161 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Ctrl"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText161->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText161, 0, wxALL, 5 );

	wxString m_rbCtrlClickActionChoices[] = { _("Toggle selection."), _("Highlight net (for pads or tracks).") };
	int m_rbCtrlClickActionNChoices = sizeof( m_rbCtrlClickActionChoices ) / sizeof( wxString );
	m_rbCtrlClickAction = new wxRadioBox( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Action"), wxDefaultPosition, wxDefaultSize, m_rbCtrlClickActionNChoices, m_rbCtrlClickActionChoices, 1, wxRA_SPECIFY_COLS );
	m_rbCtrlClickAction->SetSelection( 0 );
	fgSizerCmdsWinLin->Add( m_rbCtrlClickAction, 0, wxALL, 5 );


	m_mouseCmdsWinLin->Add( fgSizerCmdsWinLin, 1, wxEXPAND, 5 );


	bMiddleLeftSizer->Add( m_mouseCmdsWinLin, 1, wxEXPAND|wxALL, 5 );

	m_mouseCmdsOSX = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Left Click Mouse Commands") ), wxVERTICAL );

	m_staticText1811 = new wxStaticText( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, _("Left click (and drag) actions depend on 3 modifier keys:\nOption, Shift and Cmd."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1811->Wrap( -1 );
	m_mouseCmdsOSX->Add( m_staticText1811, 0, wxALL, 5 );

	wxStaticLine* staticline111;
	staticline111 = new wxStaticLine( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_mouseCmdsOSX->Add( staticline111, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxFlexGridSizer* fgSizerCmdsOSX;
	fgSizerCmdsOSX = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizerCmdsOSX->SetFlexibleDirection( wxBOTH );
	fgSizerCmdsOSX->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* staticText62;
	staticText62 = new wxStaticText( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, _("No modifier"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText62->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText62, 0, wxALL, 5 );

	wxStaticText* staticText72;
	staticText72 = new wxStaticText( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, _("Select item(s)."), wxDefaultPosition, wxDefaultSize, 0 );
	staticText72->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText72, 0, wxALL, 5 );

	wxStaticText* staticText162;
	staticText162 = new wxStaticText( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, _("Long Click"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText162->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText162, 0, wxALL, 5 );

	wxStaticText* staticText172;
	staticText172 = new wxStaticText( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, _("Clarify selection from menu."), wxDefaultPosition, wxDefaultSize, 0 );
	staticText172->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText172, 0, wxALL, 5 );

	wxStaticText* staticText82;
	staticText82 = new wxStaticText( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, _("Shift"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText82->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText82, 0, wxALL, 5 );

	wxStaticText* staticText92;
	staticText92 = new wxStaticText( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, _("Add item(s) to selection."), wxDefaultPosition, wxDefaultSize, 0 );
	staticText92->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText92, 0, wxALL, 5 );

	wxStaticText* staticText122;
	staticText122 = new wxStaticText( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, _("Shift+Cmd"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText122->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText122, 0, wxALL, 5 );

	wxStaticText* staticText132;
	staticText132 = new wxStaticText( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, _("Remove item(s) from selection."), wxDefaultPosition, wxDefaultSize, 0 );
	staticText132->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText132, 0, wxALL, 5 );

	wxStaticText* staticText142;
	staticText142 = new wxStaticText( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, _("Cmd"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText142->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText142, 0, wxALL, 5 );

	wxString m_rbCtrlClickActionMacChoices[] = { _("Toggle selection."), _("Highlight net (for pads or tracks).") };
	int m_rbCtrlClickActionMacNChoices = sizeof( m_rbCtrlClickActionMacChoices ) / sizeof( wxString );
	m_rbCtrlClickActionMac = new wxRadioBox( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, _("Action"), wxDefaultPosition, wxDefaultSize, m_rbCtrlClickActionMacNChoices, m_rbCtrlClickActionMacChoices, 1, wxRA_SPECIFY_COLS );
	m_rbCtrlClickActionMac->SetSelection( 0 );
	fgSizerCmdsOSX->Add( m_rbCtrlClickActionMac, 0, wxALL, 5 );

	wxStaticText* staticText102;
	staticText102 = new wxStaticText( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, _("Option"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText102->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText102, 0, wxALL, 5 );

	wxStaticText* staticText112;
	staticText112 = new wxStaticText( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, _("Clarify selection from menu."), wxDefaultPosition, wxDefaultSize, 0 );
	staticText112->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText112, 0, wxALL, 5 );


	m_mouseCmdsOSX->Add( fgSizerCmdsOSX, 1, wxEXPAND, 5 );


	bMiddleLeftSizer->Add( m_mouseCmdsOSX, 1, wxEXPAND|wxALL, 5 );


	bMargins->Add( bMiddleLeftSizer, 0, wxEXPAND|wxTOP|wxRIGHT, 5 );

	m_optionsBook = new wxSimplebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxPanel* fpPage;
	fpPage = new wxPanel( m_optionsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbFPMagnets;
	sbFPMagnets = new wxStaticBoxSizer( new wxStaticBox( fpPage, wxID_ANY, _("Magnetic Points") ), wxVERTICAL );

	m_magneticPads = new wxCheckBox( sbFPMagnets->GetStaticBox(), wxID_ANY, _("Magnetic pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_magneticPads->SetValue(true);
	sbFPMagnets->Add( m_magneticPads, 0, wxBOTTOM|wxLEFT, 5 );

	m_magneticGraphics = new wxCheckBox( sbFPMagnets->GetStaticBox(), wxID_ANY, _("Magnetic graphics"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFPMagnets->Add( m_magneticGraphics, 0, wxBOTTOM|wxLEFT, 5 );


	bSizer10->Add( sbFPMagnets, 0, wxEXPAND|wxTOP, 5 );


	fpPage->SetSizer( bSizer10 );
	fpPage->Layout();
	bSizer10->Fit( fpPage );
	m_optionsBook->AddPage( fpPage, _("a page"), false );
	wxPanel* pcbPage;
	pcbPage = new wxPanel( m_optionsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* pcbOptionsSizer;
	pcbOptionsSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbMagnets;
	sbMagnets = new wxStaticBoxSizer( new wxStaticBox( pcbPage, wxID_ANY, _("Magnetic Points") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 2, 3, 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText2 = new wxStaticText( sbMagnets->GetStaticBox(), wxID_ANY, _("Snap to pads:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	m_staticText2->SetToolTip( _("Capture cursor when the mouse enters a pad area") );

	fgSizer2->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxString m_magneticPadChoiceChoices[] = { _("Never"), _("When creating tracks"), _("Always") };
	int m_magneticPadChoiceNChoices = sizeof( m_magneticPadChoiceChoices ) / sizeof( wxString );
	m_magneticPadChoice = new wxChoice( sbMagnets->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_magneticPadChoiceNChoices, m_magneticPadChoiceChoices, 0 );
	m_magneticPadChoice->SetSelection( 1 );
	m_magneticPadChoice->SetToolTip( _("Capture cursor when the mouse enters a pad area") );

	fgSizer2->Add( m_magneticPadChoice, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticText21 = new wxStaticText( sbMagnets->GetStaticBox(), wxID_ANY, _("Snap to tracks:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	m_staticText21->SetToolTip( _("Capture cursor when the mouse approaches a track") );

	fgSizer2->Add( m_staticText21, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_magneticTrackChoiceChoices[] = { _("Never"), _("When creating tracks"), _("Always") };
	int m_magneticTrackChoiceNChoices = sizeof( m_magneticTrackChoiceChoices ) / sizeof( wxString );
	m_magneticTrackChoice = new wxChoice( sbMagnets->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_magneticTrackChoiceNChoices, m_magneticTrackChoiceChoices, 0 );
	m_magneticTrackChoice->SetSelection( 1 );
	m_magneticTrackChoice->SetToolTip( _("Capture cursor when the mouse approaches a track") );

	fgSizer2->Add( m_magneticTrackChoice, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_staticText211 = new wxStaticText( sbMagnets->GetStaticBox(), wxID_ANY, _("Snap to graphics:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText211->Wrap( -1 );
	m_staticText211->SetToolTip( _("Capture cursor when the mouse approaches graphical control points") );

	fgSizer2->Add( m_staticText211, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_magneticGraphicsChoiceChoices[] = { _("Always"), _("Never") };
	int m_magneticGraphicsChoiceNChoices = sizeof( m_magneticGraphicsChoiceChoices ) / sizeof( wxString );
	m_magneticGraphicsChoice = new wxChoice( sbMagnets->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_magneticGraphicsChoiceNChoices, m_magneticGraphicsChoiceChoices, 0 );
	m_magneticGraphicsChoice->SetSelection( 0 );
	m_magneticGraphicsChoice->SetToolTip( _("Capture cursor when the mouse approaches graphical control points") );

	fgSizer2->Add( m_magneticGraphicsChoice, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	sbMagnets->Add( fgSizer2, 1, wxEXPAND|wxBOTTOM, 5 );


	pcbOptionsSizer->Add( sbMagnets, 0, wxEXPAND|wxTOP, 5 );

	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( pcbPage, wxID_ANY, _("Ratsnest") ), wxVERTICAL );

	m_showSelectedRatsnest = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Always show selected ratsnest"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer3->Add( m_showSelectedRatsnest, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_OptDisplayCurvedRatsnestLines = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Show ratsnest with curved lines"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer3->Add( m_OptDisplayCurvedRatsnestLines, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	pcbOptionsSizer->Add( sbSizer3, 0, wxEXPAND|wxTOP, 5 );

	wxStaticBoxSizer* sbSizer41;
	sbSizer41 = new wxStaticBoxSizer( new wxStaticBox( pcbPage, wxID_ANY, _("Track Editing") ), wxVERTICAL );

	m_staticText5 = new wxStaticText( sbSizer41->GetStaticBox(), wxID_ANY, _("Mouse drag track behavior:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	m_staticText5->SetToolTip( _("Choose the action to perform when dragging a track segment with the mouse") );

	sbSizer41->Add( m_staticText5, 0, wxBOTTOM|wxRIGHT|wxLEFT, 3 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_rbTrackDragMove = new wxRadioButton( sbSizer41->GetStaticBox(), wxID_ANY, _("Move"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_rbTrackDragMove->SetToolTip( _("Moves the track segment without moving connected tracks") );

	bSizer8->Add( m_rbTrackDragMove, 0, wxBOTTOM, 3 );

	m_rbTrackDrag45 = new wxRadioButton( sbSizer41->GetStaticBox(), wxID_ANY, _("Drag (45 degree mode)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbTrackDrag45->SetToolTip( _("Drags the track segment while keeping connected tracks at 45 degrees.") );

	bSizer8->Add( m_rbTrackDrag45, 0, wxBOTTOM, 3 );

	m_rbTrackDragFree = new wxRadioButton( sbSizer41->GetStaticBox(), wxID_ANY, _("Drag (free angle)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbTrackDragFree->SetToolTip( _("Drags the nearest joint in the track without restricting the track angle.") );

	bSizer8->Add( m_rbTrackDragFree, 0, 0, 3 );


	sbSizer41->Add( bSizer8, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	pcbOptionsSizer->Add( sbSizer41, 0, wxEXPAND|wxTOP, 5 );

	wxStaticBoxSizer* sbSizerMisc;
	sbSizerMisc = new wxStaticBoxSizer( new wxStaticBox( pcbPage, wxID_ANY, _("Miscellaneous") ), wxVERTICAL );

	m_escClearsNetHighlight = new wxCheckBox( sbSizerMisc->GetStaticBox(), wxID_ANY, _("<ESC> clears net highlighting"), wxDefaultPosition, wxDefaultSize, 0 );
	m_escClearsNetHighlight->SetValue(true);
	sbSizerMisc->Add( m_escClearsNetHighlight, 0, wxBOTTOM|wxLEFT, 5 );

	m_showPageLimits = new wxCheckBox( sbSizerMisc->GetStaticBox(), wxID_ANY, _("Show page limits"), wxDefaultPosition, wxDefaultSize, 0 );
	m_showPageLimits->SetValue(true);
	m_showPageLimits->SetToolTip( _("Draw an outline to show the sheet size.") );

	sbSizerMisc->Add( m_showPageLimits, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbCourtyardCollisions = new wxCheckBox( sbSizerMisc->GetStaticBox(), wxID_ANY, _("Show courtyard collisions when moving/dragging"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbCourtyardCollisions->SetValue(true);
	sbSizerMisc->Add( m_cbCourtyardCollisions, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_autoRefillZones = new wxCheckBox( sbSizerMisc->GetStaticBox(), wxID_ANY, _("Auto-refill zones"), wxDefaultPosition, wxDefaultSize, 0 );
	m_autoRefillZones->SetValue(true);
	m_autoRefillZones->SetToolTip( _("If checked, zones will be re-filled after each edit operation") );

	sbSizerMisc->Add( m_autoRefillZones, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	pcbOptionsSizer->Add( sbSizerMisc, 0, wxEXPAND|wxTOP, 5 );


	pcbPage->SetSizer( pcbOptionsSizer );
	pcbPage->Layout();
	pcbOptionsSizer->Fit( pcbPage );
	m_optionsBook->AddPage( pcbPage, _("a page"), false );

	bMargins->Add( m_optionsBook, 0, wxEXPAND | wxALL, 5 );


	bPanelSizer->Add( bMargins, 1, wxRIGHT, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_EDIT_OPTIONS_BASE::~PANEL_EDIT_OPTIONS_BASE()
{
}
