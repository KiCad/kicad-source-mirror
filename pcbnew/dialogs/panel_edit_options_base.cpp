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

	m_MagneticPads = new wxCheckBox( bOptionsSizer->GetStaticBox(), wxID_ANY, _("Magnetic pads"), wxDefaultPosition, wxDefaultSize, 0 );
	bOptionsSizer->Add( m_MagneticPads, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_Segments_45_Only_Ctrl = new wxCheckBox( bOptionsSizer->GetStaticBox(), wxID_SEGMENTS45, _("L&imit graphic lines to H, V and 45 degrees"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Segments_45_Only_Ctrl->SetToolTip( _("Force line segment directions to H, V or 45 degrees when drawing on technical layers.") );

	bOptionsSizer->Add( m_Segments_45_Only_Ctrl, 0, wxALL, 5 );

	m_FlipLeftRight = new wxCheckBox( bOptionsSizer->GetStaticBox(), wxID_ANY, _("Flip board items L/R (default is T/B)"), wxDefaultPosition, wxDefaultSize, 0 );
	bOptionsSizer->Add( m_FlipLeftRight, 0, wxALL, 5 );

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


	bOptionsSizer->Add( fgSizer12, 0, wxEXPAND|wxTOP, 5 );


	bOptionsSizer->Add( 0, 30, 0, 0, 5 );

	wxStaticBoxSizer* sbSizerMouseCmdWinLin;
	sbSizerMouseCmdWinLin = new wxStaticBoxSizer( new wxStaticBox( bOptionsSizer->GetStaticBox(), wxID_ANY, _("Left Click Mouse Commands") ), wxVERTICAL );

	m_staticText181 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("Left mouse click action depends on  3 modifier keys:\nAlt, Shift, Ctrl/Cmd."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText181->Wrap( -1 );
	sbSizerMouseCmdWinLin->Add( m_staticText181, 0, wxALL, 5 );

	m_staticline11 = new wxStaticLine( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sbSizerMouseCmdWinLin->Add( m_staticline11, 0, wxEXPAND | wxALL, 5 );

	m_fgSizerLMBWinLin = new wxFlexGridSizer( 0, 2, 0, 0 );
	m_fgSizerLMBWinLin->SetFlexibleDirection( wxBOTH );
	m_fgSizerLMBWinLin->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText61 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("No modifier"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText61->Wrap( -1 );
	m_fgSizerLMBWinLin->Add( m_staticText61, 0, wxALL, 5 );

	m_staticText71 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("Item/block selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText71->Wrap( -1 );
	m_fgSizerLMBWinLin->Add( m_staticText71, 0, wxALL, 5 );

	m_staticText81 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("SHIFT"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText81->Wrap( -1 );
	m_fgSizerLMBWinLin->Add( m_staticText81, 0, wxALL, 5 );

	m_staticText91 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("New selection added to current selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText91->Wrap( -1 );
	m_fgSizerLMBWinLin->Add( m_staticText91, 0, wxALL, 5 );

	m_staticText121 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("SHIFT+ALT"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText121->Wrap( -1 );
	m_fgSizerLMBWinLin->Add( m_staticText121, 0, wxALL, 5 );

	m_staticText131 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("New selection removed from current selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText131->Wrap( -1 );
	m_fgSizerLMBWinLin->Add( m_staticText131, 0, wxALL, 5 );

	m_staticText101 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("CTRL"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText101->Wrap( -1 );
	m_fgSizerLMBWinLin->Add( m_staticText101, 0, wxALL, 5 );

	m_staticText111 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("Show full disambiguation context menu"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText111->Wrap( -1 );
	m_fgSizerLMBWinLin->Add( m_staticText111, 0, wxALL, 5 );

	m_staticText141 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("ALT"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText141->Wrap( -1 );
	m_fgSizerLMBWinLin->Add( m_staticText141, 0, wxALL, 5 );

	m_staticText151 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("Toggle new selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText151->Wrap( -1 );
	m_fgSizerLMBWinLin->Add( m_staticText151, 0, wxALL, 5 );

	m_staticText161 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("CTRL+SHIFT"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText161->Wrap( -1 );
	m_fgSizerLMBWinLin->Add( m_staticText161, 0, wxALL, 5 );

	m_staticText171 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("Highlight net (for pads or tracks)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText171->Wrap( -1 );
	m_fgSizerLMBWinLin->Add( m_staticText171, 0, wxALL, 5 );


	sbSizerMouseCmdWinLin->Add( m_fgSizerLMBWinLin, 1, wxEXPAND, 5 );

	m_fgSizerLMB_OSX = new wxFlexGridSizer( 0, 2, 0, 0 );
	m_fgSizerLMB_OSX->SetFlexibleDirection( wxBOTH );
	m_fgSizerLMB_OSX->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText6 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("No modifier"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	m_fgSizerLMB_OSX->Add( m_staticText6, 0, wxALL, 5 );

	m_staticText7 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("Item/block selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	m_fgSizerLMB_OSX->Add( m_staticText7, 0, wxALL, 5 );

	m_staticText8 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("SHIFT"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	m_fgSizerLMB_OSX->Add( m_staticText8, 0, wxALL, 5 );

	m_staticText9 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("New selection added to current selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	m_fgSizerLMB_OSX->Add( m_staticText9, 0, wxALL, 5 );

	m_staticText12 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("CTRL+SHIFT"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	m_fgSizerLMB_OSX->Add( m_staticText12, 0, wxALL, 5 );

	m_staticText13 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("New selection removed from current selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText13->Wrap( -1 );
	m_fgSizerLMB_OSX->Add( m_staticText13, 0, wxALL, 5 );

	m_staticText10 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("ALT"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	m_fgSizerLMB_OSX->Add( m_staticText10, 0, wxALL, 5 );

	m_staticText11 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("Show full disambiguation context menu"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	m_fgSizerLMB_OSX->Add( m_staticText11, 0, wxALL, 5 );

	m_staticText14 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("CTRL"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	m_fgSizerLMB_OSX->Add( m_staticText14, 0, wxALL, 5 );

	m_staticText15 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("Toggle new selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	m_fgSizerLMB_OSX->Add( m_staticText15, 0, wxALL, 5 );

	m_staticText16 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("CTRL+ALT"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	m_fgSizerLMB_OSX->Add( m_staticText16, 0, wxALL, 5 );

	m_staticText17 = new wxStaticText( sbSizerMouseCmdWinLin->GetStaticBox(), wxID_ANY, _("Highlight net (for pads or tracks)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText17->Wrap( -1 );
	m_fgSizerLMB_OSX->Add( m_staticText17, 0, wxALL, 5 );


	sbSizerMouseCmdWinLin->Add( m_fgSizerLMB_OSX, 1, wxEXPAND, 5 );


	bOptionsSizer->Add( sbSizerMouseCmdWinLin, 1, wxEXPAND, 5 );


	bMiddleLeftSizer->Add( bOptionsSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


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


	pcbOptionsSizer->Add( sbMagnets, 1, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( pcbPage, wxID_ANY, _("Ratsnest") ), wxVERTICAL );

	m_showSelectedRatsnest = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Always show selected ratsnest"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer3->Add( m_showSelectedRatsnest, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_OptDisplayCurvedRatsnestLines = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Show ratsnest with curved lines"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer3->Add( m_OptDisplayCurvedRatsnestLines, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	pcbOptionsSizer->Add( sbSizer3, 1, wxEXPAND|wxTOP, 5 );

	wxStaticBoxSizer* sbSizer4;
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( pcbPage, wxID_ANY, _("Annotations") ), wxVERTICAL );

	m_Show_Page_Limits = new wxCheckBox( sbSizer4->GetStaticBox(), wxID_ANY, _("Show page limits"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Show_Page_Limits->SetValue(true);
	sbSizer4->Add( m_Show_Page_Limits, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	pcbOptionsSizer->Add( sbSizer4, 0, wxEXPAND|wxTOP, 5 );

	wxStaticBoxSizer* sbSizer41;
	sbSizer41 = new wxStaticBoxSizer( new wxStaticBox( pcbPage, wxID_ANY, _("Track Editing") ), wxVERTICAL );

	m_staticText5 = new wxStaticText( sbSizer41->GetStaticBox(), wxID_ANY, _("Mouse drag track behavior:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	m_staticText5->SetToolTip( _("Choose the action to perform when dragging a track segment with the mouse") );

	sbSizer41->Add( m_staticText5, 0, wxBOTTOM|wxRIGHT|wxLEFT, 3 );

	m_rbTrackDragMove = new wxRadioButton( sbSizer41->GetStaticBox(), wxID_ANY, _("Move"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_rbTrackDragMove->SetToolTip( _("Moves the track segment without moving connected tracks") );

	sbSizer41->Add( m_rbTrackDragMove, 0, wxRIGHT|wxLEFT, 5 );


	sbSizer41->Add( 0, 5, 0, wxEXPAND, 5 );

	m_rbTrackDrag45 = new wxRadioButton( sbSizer41->GetStaticBox(), wxID_ANY, _("Drag (45 degree mode)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbTrackDrag45->SetToolTip( _("Drags the track segment while keeping connected tracks at 45 degrees.") );

	sbSizer41->Add( m_rbTrackDrag45, 0, wxRIGHT|wxLEFT, 5 );


	sbSizer41->Add( 0, 5, 0, wxEXPAND, 5 );

	m_rbTrackDragFree = new wxRadioButton( sbSizer41->GetStaticBox(), wxID_ANY, _("Drag (free angle)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbTrackDragFree->SetToolTip( _("Drags the nearest joint in the track without restricting the track angle.") );

	sbSizer41->Add( m_rbTrackDragFree, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	pcbOptionsSizer->Add( sbSizer41, 1, wxEXPAND|wxTOP, 5 );


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
