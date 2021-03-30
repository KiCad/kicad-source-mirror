///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_edit_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_EDIT_OPTIONS_BASE::PANEL_EDIT_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bMiddleLeftSizer;
	bMiddleLeftSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* bOptionsSizer;
	bOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Editing Options") ), wxVERTICAL );

	wxBoxSizer* bSizeFPEdit;
	bSizeFPEdit = new wxBoxSizer( wxVERTICAL );

	m_magneticPads = new wxCheckBox( bOptionsSizer->GetStaticBox(), wxID_ANY, _("Magnetic pads"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizeFPEdit->Add( m_magneticPads, 0, wxBOTTOM, 3 );

	m_magneticGraphics = new wxCheckBox( bOptionsSizer->GetStaticBox(), wxID_ANY, _("Magnetic graphics"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizeFPEdit->Add( m_magneticGraphics, 0, wxBOTTOM, 15 );


	bOptionsSizer->Add( bSizeFPEdit, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerBoardEdit;
	bSizerBoardEdit = new wxBoxSizer( wxVERTICAL );

	m_flipLeftRight = new wxCheckBox( bOptionsSizer->GetStaticBox(), wxID_ANY, _("Flip board items L/R (default is T/B)"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBoardEdit->Add( m_flipLeftRight, 0, wxBOTTOM, 15 );


	bOptionsSizer->Add( bSizerBoardEdit, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerUniversal;
	bSizerUniversal = new wxBoxSizer( wxVERTICAL );

	m_segments45OnlyCtrl = new wxCheckBox( bOptionsSizer->GetStaticBox(), wxID_SEGMENTS45, _("L&imit graphic lines to H, V and 45 degrees"), wxDefaultPosition, wxDefaultSize, 0 );
	m_segments45OnlyCtrl->SetToolTip( _("When drawing graphic lines force to horizontal, vertical, or 45 degrees.") );

	bSizerUniversal->Add( m_segments45OnlyCtrl, 0, wxBOTTOM, 3 );

	wxFlexGridSizer* fgSizer12;
	fgSizer12 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer12->AddGrowableCol( 1 );
	fgSizer12->SetFlexibleDirection( wxBOTH );
	fgSizer12->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextRotationAngle = new wxStaticText( bOptionsSizer->GetStaticBox(), wxID_ANY, _("&Rotation angle:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRotationAngle->Wrap( -1 );
	fgSizer12->Add( m_staticTextRotationAngle, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_rotationAngle = new wxTextCtrl( bOptionsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_rotationAngle->SetToolTip( _("Set increment (in degrees) for context menu and hotkey rotation.") );

	fgSizer12->Add( m_rotationAngle, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );


	bSizerUniversal->Add( fgSizer12, 0, wxEXPAND, 5 );


	bOptionsSizer->Add( bSizerUniversal, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bMiddleLeftSizer->Add( bOptionsSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_mouseCmdsWinLin = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Left Click Mouse Commands") ), wxVERTICAL );

	m_staticText181 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Left click (and drag) actions depend on 3 modifier keys:\nAlt, Shift and Ctrl."), wxDefaultPosition, wxDefaultSize, 0 );
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

	wxStaticText* staticText81;
	staticText81 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Shift"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText81->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText81, 0, wxALL, 5 );

	wxStaticText* staticText91;
	staticText91 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Add item(s) to selection."), wxDefaultPosition, wxDefaultSize, 0 );
	staticText91->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText91, 0, wxALL, 5 );

	wxStaticText* staticText121;
	staticText121 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Shift+Alt"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText121->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText121, 0, wxALL, 5 );

	wxStaticText* staticText131;
	staticText131 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Remove item(s) from selection."), wxDefaultPosition, wxDefaultSize, 0 );
	staticText131->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText131, 0, wxALL, 5 );

	wxStaticText* staticText141;
	staticText141 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Alt"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText141->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText141, 0, wxALL, 5 );

	wxStaticText* staticText151;
	staticText151 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Toggle selected state of item(s)."), wxDefaultPosition, wxDefaultSize, 0 );
	staticText151->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText151, 0, wxALL, 5 );

	wxStaticText* staticText101;
	staticText101 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Ctrl"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText101->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText101, 0, wxALL, 5 );

	wxStaticText* staticText111;
	staticText111 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Clarify selection from menu."), wxDefaultPosition, wxDefaultSize, 0 );
	staticText111->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText111, 0, wxALL, 5 );

	wxStaticText* staticText161;
	staticText161 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Ctrl+Shift"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText161->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText161, 0, wxALL, 5 );

	wxStaticText* staticText171;
	staticText171 = new wxStaticText( m_mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Highlight net (for pads or tracks)."), wxDefaultPosition, wxDefaultSize, 0 );
	staticText171->Wrap( -1 );
	fgSizerCmdsWinLin->Add( staticText171, 0, wxALL, 5 );


	m_mouseCmdsWinLin->Add( fgSizerCmdsWinLin, 1, wxEXPAND, 5 );


	bMiddleLeftSizer->Add( m_mouseCmdsWinLin, 1, wxEXPAND|wxALL, 5 );

	m_mouseCmdsOSX = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Left Click Mouse Commands") ), wxVERTICAL );

	m_staticText1811 = new wxStaticText( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, _("Left click (and drag) actions depend on 3 modifier keys:\nAlt, Shift and Cmd."), wxDefaultPosition, wxDefaultSize, 0 );
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

	wxStaticText* staticText152;
	staticText152 = new wxStaticText( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, _("Toggle selected state of item(s)."), wxDefaultPosition, wxDefaultSize, 0 );
	staticText152->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText152, 0, wxALL, 5 );

	wxStaticText* staticText102;
	staticText102 = new wxStaticText( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, _("Alt"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText102->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText102, 0, wxALL, 5 );

	wxStaticText* staticText112;
	staticText112 = new wxStaticText( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, _("Clarify selection from menu."), wxDefaultPosition, wxDefaultSize, 0 );
	staticText112->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText112, 0, wxALL, 5 );

	wxStaticText* staticText162;
	staticText162 = new wxStaticText( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, _("Alt+Cmd"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText162->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText162, 0, wxALL, 5 );

	wxStaticText* staticText172;
	staticText172 = new wxStaticText( m_mouseCmdsOSX->GetStaticBox(), wxID_ANY, _("Highlight net (for pads or tracks)."), wxDefaultPosition, wxDefaultSize, 0 );
	staticText172->Wrap( -1 );
	fgSizerCmdsOSX->Add( staticText172, 0, wxALL, 5 );


	m_mouseCmdsOSX->Add( fgSizerCmdsOSX, 1, wxEXPAND, 5 );


	bMiddleLeftSizer->Add( m_mouseCmdsOSX, 1, wxEXPAND|wxALL, 5 );


	bMargins->Add( bMiddleLeftSizer, 1, wxEXPAND|wxRIGHT, 5 );

	m_optionsBook = new wxSimplebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxPanel* emptyPage;
	emptyPage = new wxPanel( m_optionsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_optionsBook->AddPage( emptyPage, _("a page"), false );
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


	pcbOptionsSizer->Add( sbMagnets, 0, wxEXPAND, 5 );

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

	wxStaticBoxSizer* sbSizer4;
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( pcbPage, wxID_ANY, _("Miscellaneous") ), wxVERTICAL );

	m_Show_Page_Limits = new wxCheckBox( sbSizer4->GetStaticBox(), wxID_ANY, _("Show page limits"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Show_Page_Limits->SetValue(true);
	m_Show_Page_Limits->SetToolTip( _("Draw an outline to show the sheet size.") );

	sbSizer4->Add( m_Show_Page_Limits, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_Auto_Refill_Zones = new wxCheckBox( sbSizer4->GetStaticBox(), wxID_ANY, _("Refill zones after Zone Properties dialog"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Auto_Refill_Zones->SetValue(true);
	m_Auto_Refill_Zones->SetToolTip( _("If checked, zones will be re-filled after editing the properties of the zone using the Zone Properties dialog") );

	sbSizer4->Add( m_Auto_Refill_Zones, 0, wxALL, 5 );

	m_Allow_Free_Pads = new wxCheckBox( sbSizer4->GetStaticBox(), wxID_ANY, _("Allow free pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Allow_Free_Pads->SetToolTip( _("If checked, pads can be moved with respect to the rest of the footprint.") );

	sbSizer4->Add( m_Allow_Free_Pads, 0, wxALL, 5 );


	pcbOptionsSizer->Add( sbSizer4, 1, wxEXPAND|wxTOP, 5 );


	pcbPage->SetSizer( pcbOptionsSizer );
	pcbPage->Layout();
	pcbOptionsSizer->Fit( pcbPage );
	m_optionsBook->AddPage( pcbPage, _("a page"), false );

	bMargins->Add( m_optionsBook, 1, wxEXPAND | wxALL, 5 );


	bPanelSizer->Add( bMargins, 1, wxRIGHT, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_EDIT_OPTIONS_BASE::~PANEL_EDIT_OPTIONS_BASE()
{
}
