///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_eeschema_editing_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_EESCHEMA_EDITING_OPTIONS_BASE::PANEL_EESCHEMA_EDITING_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftColumn;
	bLeftColumn = new wxBoxSizer( wxVERTICAL );

	m_editingLabel = new wxStaticText( this, wxID_ANY, _("Editing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_editingLabel->Wrap( -1 );
	bLeftColumn->Add( m_editingLabel, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftColumn->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer61;
	bSizer61 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText24 = new wxStaticText( this, wxID_ANY, _("Line drawing mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText24->Wrap( -1 );
	bSizer61->Add( m_staticText24, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_choiceLineModeChoices[] = { _("Free Angle"), _("90 deg Angle"), _("45 deg Angle") };
	int m_choiceLineModeNChoices = sizeof( m_choiceLineModeChoices ) / sizeof( wxString );
	m_choiceLineMode = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceLineModeNChoices, m_choiceLineModeChoices, 0 );
	m_choiceLineMode->SetSelection( 1 );
	bSizer61->Add( m_choiceLineMode, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizer5->Add( bSizer61, 1, wxEXPAND, 5 );

	m_staticTextArcEdit = new wxStaticText( this, wxID_ANY, _("Arc editing mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextArcEdit->Wrap( -1 );
	bSizer5->Add( m_staticTextArcEdit, 0, wxALL, 5 );

	wxString m_choiceArcModeChoices[] = { _("Keep center, adjust radius"), _("Keep endpoints or direction of starting point"), _("Keep center and radius, adjust endpoints") };
	int m_choiceArcModeNChoices = sizeof( m_choiceArcModeChoices ) / sizeof( wxString );
	m_choiceArcMode = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceArcModeNChoices, m_choiceArcModeChoices, 0 );
	m_choiceArcMode->SetSelection( 1 );
	bSizer5->Add( m_choiceArcMode, 0, wxALL|wxEXPAND, 5 );

	m_mouseDragIsDrag = new wxCheckBox( this, wxID_ANY, _("Mouse drag performs Drag (G) operation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mouseDragIsDrag->SetToolTip( _("If unchecked, mouse drag will perform move (M) operation") );

	bSizer5->Add( m_mouseDragIsDrag, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_cbAutoStartWires = new wxCheckBox( this, wxID_ANY, _("Automatically start wires on unconnected pins"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbAutoStartWires->SetToolTip( _("When enabled, you can start wiring by clicking on unconnected pins even when the wire tool is not active") );

	bSizer5->Add( m_cbAutoStartWires, 0, wxLEFT|wxTOP, 5 );

	m_escClearsNetHighlight = new wxCheckBox( this, wxID_ANY, _("<ESC> clears net highlighting"), wxDefaultPosition, wxDefaultSize, 0 );
	m_escClearsNetHighlight->SetToolTip( _("First <ESC> in selection tool clears selection, next clears net highlighting") );

	bSizer5->Add( m_escClearsNetHighlight, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_checkAutoAnnotate = new wxCheckBox( this, wxID_ANY, _("Automatically annotate symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_checkAutoAnnotate, 0, wxALL, 5 );


	bLeftColumn->Add( bSizer5, 0, wxEXPAND|wxTOP|wxLEFT, 5 );


	bLeftColumn->Add( 0, 15, 0, wxEXPAND, 5 );

	m_staticText26 = new wxStaticText( this, wxID_ANY, _("Defaults for New Objects"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText26->Wrap( -1 );
	bLeftColumn->Add( m_staticText26, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline4 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftColumn->Add( m_staticline4, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_borderColorLabel = new wxStaticText( this, wxID_ANY, _("Sheet border:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderColorLabel->Wrap( -1 );
	gbSizer1->Add( m_borderColorLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	m_borderColorSwatch = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_borderColorSwatch->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	gbSizer1->Add( m_borderColorSwatch, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	m_backgroundColorLabel = new wxStaticText( this, wxID_ANY, _("Sheet background:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_backgroundColorLabel->Wrap( -1 );
	gbSizer1->Add( m_backgroundColorLabel, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	m_backgroundColorSwatch = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_backgroundColorSwatch->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	gbSizer1->Add( m_backgroundColorSwatch, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	m_powerSymbolLabel = new wxStaticText( this, wxID_ANY, _("Power Symbols:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_powerSymbolLabel->Wrap( -1 );
	gbSizer1->Add( m_powerSymbolLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxString m_choicePowerChoices[] = { _("Default"), _("Global"), _("Local") };
	int m_choicePowerNChoices = sizeof( m_choicePowerChoices ) / sizeof( wxString );
	m_choicePower = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choicePowerNChoices, m_choicePowerChoices, 0 );
	m_choicePower->SetSelection( 0 );
	m_choicePower->SetToolTip( _("Select the assigned type of new power symbol.\nDefault will follow the symbol definition.\nGlobal will convert new power symbols to global power symbols.\nLocal will convert new power symbols to a local power symbols.") );

	gbSizer1->Add( m_choicePower, wxGBPosition( 1, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );


	bLeftColumn->Add( gbSizer1, 0, wxEXPAND|wxLEFT|wxTOP, 10 );

	wxFlexGridSizer* fgSizer4;
	fgSizer4 = new wxFlexGridSizer( 2, 4, 0, 0 );
	fgSizer4->SetFlexibleDirection( wxBOTH );
	fgSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	bLeftColumn->Add( fgSizer4, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );


	bLeftColumn->Add( bSizer6, 0, wxEXPAND|wxTOP|wxLEFT, 10 );


	bLeftColumn->Add( 0, 15, 0, wxEXPAND, 5 );

	m_leftClickCmdsBook = new wxSimplebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_pageWinLin = new wxPanel( m_leftClickCmdsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_leftClickLabel = new wxStaticText( m_pageWinLin, wxID_ANY, _("Left Click Mouse Commands"), wxDefaultPosition, wxDefaultSize, 0 );
	m_leftClickLabel->Wrap( -1 );
	bSizer8->Add( m_leftClickLabel, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline6 = new wxStaticLine( m_pageWinLin, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer8->Add( m_staticline6, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	m_hint1 = new wxStaticText( m_pageWinLin, wxID_ANY, _("Left click (and drag) actions depend on 2 modifier keys:\nShift and Ctrl"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hint1->Wrap( -1 );
	bSizer9->Add( m_hint1, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	wxFlexGridSizer* fgSizerCmdsWinLin;
	fgSizerCmdsWinLin = new wxFlexGridSizer( 0, 2, 0, 5 );
	fgSizerCmdsWinLin->SetFlexibleDirection( wxBOTH );
	fgSizerCmdsWinLin->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText91 = new wxStaticText( m_pageWinLin, wxID_ANY, _("Long Click"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText91->Wrap( -1 );
	fgSizerCmdsWinLin->Add( m_staticText91, 0, wxALL, 5 );

	m_staticText101 = new wxStaticText( m_pageWinLin, wxID_ANY, _("Clarify selection from menu"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText101->Wrap( -1 );
	fgSizerCmdsWinLin->Add( m_staticText101, 0, wxALL, 5 );

	m_staticText131 = new wxStaticText( m_pageWinLin, wxID_ANY, _("Shift"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText131->Wrap( -1 );
	fgSizerCmdsWinLin->Add( m_staticText131, 0, wxALL, 5 );

	m_staticText141 = new wxStaticText( m_pageWinLin, wxID_ANY, _("Add item(s) to selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText141->Wrap( -1 );
	fgSizerCmdsWinLin->Add( m_staticText141, 0, wxALL, 5 );

	m_staticText151 = new wxStaticText( m_pageWinLin, wxID_ANY, _("Ctrl+Shift"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText151->Wrap( -1 );
	fgSizerCmdsWinLin->Add( m_staticText151, 0, wxALL, 5 );

	m_staticText161 = new wxStaticText( m_pageWinLin, wxID_ANY, _("Remove item(s) from selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText161->Wrap( -1 );
	fgSizerCmdsWinLin->Add( m_staticText161, 0, wxALL, 5 );


	bSizer9->Add( fgSizerCmdsWinLin, 0, wxEXPAND|wxTOP, 5 );


	bSizer8->Add( bSizer9, 0, wxEXPAND|wxLEFT, 5 );


	m_pageWinLin->SetSizer( bSizer8 );
	m_pageWinLin->Layout();
	bSizer8->Fit( m_pageWinLin );
	m_leftClickCmdsBook->AddPage( m_pageWinLin, _("a page"), false );
	m_pageMac = new wxPanel( m_leftClickCmdsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );

	m_leftClickLabel1 = new wxStaticText( m_pageMac, wxID_ANY, _("Left Click Mouse Commands"), wxDefaultPosition, wxDefaultSize, 0 );
	m_leftClickLabel1->Wrap( -1 );
	bSizer10->Add( m_leftClickLabel1, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline7 = new wxStaticLine( m_pageMac, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer10->Add( m_staticline7, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );

	m_hint2 = new wxStaticText( m_pageMac, wxID_ANY, _("Left click (and drag) actions depend on 3 modifier keys:\nOption, Shift and Cmd"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hint2->Wrap( -1 );
	bSizer11->Add( m_hint2, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizerCmdsWinMac;
	fgSizerCmdsWinMac = new wxFlexGridSizer( 0, 2, 0, 5 );
	fgSizerCmdsWinMac->SetFlexibleDirection( wxBOTH );
	fgSizerCmdsWinMac->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText11 = new wxStaticText( m_pageMac, wxID_ANY, _("Long Click"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	fgSizerCmdsWinMac->Add( m_staticText11, 0, wxALL, 5 );

	m_staticText12 = new wxStaticText( m_pageMac, wxID_ANY, _("Clarify selection from menu"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	fgSizerCmdsWinMac->Add( m_staticText12, 0, wxALL, 5 );

	m_staticText9 = new wxStaticText( m_pageMac, wxID_ANY, _("Shift"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizerCmdsWinMac->Add( m_staticText9, 0, wxALL, 5 );

	m_staticText10 = new wxStaticText( m_pageMac, wxID_ANY, _("Add item(s) to selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	fgSizerCmdsWinMac->Add( m_staticText10, 0, wxALL, 5 );

	m_staticText15 = new wxStaticText( m_pageMac, wxID_ANY, _("Shift+Cmd"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	fgSizerCmdsWinMac->Add( m_staticText15, 0, wxALL, 5 );

	m_staticText16 = new wxStaticText( m_pageMac, wxID_ANY, _("Remove item(s) from selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	fgSizerCmdsWinMac->Add( m_staticText16, 0, wxALL, 5 );

	m_staticText13 = new wxStaticText( m_pageMac, wxID_ANY, _("Option"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText13->Wrap( -1 );
	fgSizerCmdsWinMac->Add( m_staticText13, 0, wxALL, 5 );

	m_staticText14 = new wxStaticText( m_pageMac, wxID_ANY, _("Clarify selection from menu"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	fgSizerCmdsWinMac->Add( m_staticText14, 0, wxALL, 5 );


	bSizer11->Add( fgSizerCmdsWinMac, 1, wxEXPAND|wxTOP, 5 );


	bSizer10->Add( bSizer11, 0, wxEXPAND|wxTOP|wxLEFT, 5 );


	m_pageMac->SetSizer( bSizer10 );
	m_pageMac->Layout();
	bSizer10->Fit( m_pageMac );
	m_leftClickCmdsBook->AddPage( m_pageMac, _("a page"), false );

	bLeftColumn->Add( m_leftClickCmdsBook, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	bPanelSizer->Add( bLeftColumn, 0, wxEXPAND|wxRIGHT, 35 );

	wxBoxSizer* bRightColumn;
	bRightColumn = new wxBoxSizer( wxVERTICAL );

	m_staticText32 = new wxStaticText( this, wxID_ANY, _("Symbol Field Automatic Placement"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText32->Wrap( -1 );
	bRightColumn->Add( m_staticText32, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline10 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bRightColumn->Add( m_staticline10, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );

	m_checkAutoplaceFields = new wxCheckBox( this, wxID_ANY, _("A&utomatically place symbol fields"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer12->Add( m_checkAutoplaceFields, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_checkAutoplaceJustify = new wxCheckBox( this, wxID_ANY, _("A&llow field autoplace to change justification"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer12->Add( m_checkAutoplaceJustify, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_checkAutoplaceAlign = new wxCheckBox( this, wxID_ANY, _("Al&ways align autoplaced fields to the 50 mil grid"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer12->Add( m_checkAutoplaceAlign, 0, wxEXPAND|wxALL, 5 );


	bRightColumn->Add( bSizer12, 0, wxEXPAND|wxTOP|wxLEFT, 5 );


	bRightColumn->Add( 0, 15, 0, wxEXPAND, 5 );

	m_staticText321 = new wxStaticText( this, wxID_ANY, _("Repeated Items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText321->Wrap( -1 );
	bRightColumn->Add( m_staticText321, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline9 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bRightColumn->Add( m_staticline9, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizerRepeatOpt1;
	fgSizerRepeatOpt1 = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgSizerRepeatOpt1->SetFlexibleDirection( wxBOTH );
	fgSizerRepeatOpt1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_hPitchLabel = new wxStaticText( this, wxID_ANY, _("&Horizontal pitch:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hPitchLabel->Wrap( -1 );
	fgSizerRepeatOpt1->Add( m_hPitchLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_hPitchCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRepeatOpt1->Add( m_hPitchCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_hPitchUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hPitchUnits->Wrap( -1 );
	fgSizerRepeatOpt1->Add( m_hPitchUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_vPitchLabel = new wxStaticText( this, wxID_ANY, _("&Vertical pitch:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_vPitchLabel->Wrap( -1 );
	fgSizerRepeatOpt1->Add( m_vPitchLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_vPitchCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRepeatOpt1->Add( m_vPitchCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_vPitchUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_vPitchUnits->Wrap( -1 );
	fgSizerRepeatOpt1->Add( m_vPitchUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_labelIncrementLabel = new wxStaticText( this, wxID_ANY, _("Label increment:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelIncrementLabel->Wrap( -1 );
	fgSizerRepeatOpt1->Add( m_labelIncrementLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_spinLabelRepeatStep = new wxSpinCtrl( this, wxID_ANY, wxT("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -1000000, 1000000, 1 );
	fgSizerRepeatOpt1->Add( m_spinLabelRepeatStep, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );


	fgSizerRepeatOpt1->Add( 0, 0, 1, wxEXPAND, 3 );


	bSizer13->Add( fgSizerRepeatOpt1, 0, wxEXPAND|wxTOP, 5 );


	bRightColumn->Add( bSizer13, 0, wxEXPAND|wxTOP|wxLEFT, 5 );


	bRightColumn->Add( 0, 15, 0, wxEXPAND, 5 );

	m_staticText322 = new wxStaticText( this, wxID_ANY, _("Dialog Preferences"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText322->Wrap( -1 );
	bRightColumn->Add( m_staticText322, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline8 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bRightColumn->Add( m_staticline8, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxVERTICAL );

	m_footprintPreview = new wxCheckBox( this, wxID_ANY, _("Show footprint previews in Symbol Chooser"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer14->Add( m_footprintPreview, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_neverShowRescue = new wxCheckBox( this, wxID_ANY, _("Never show Rescue Symbols tool"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer14->Add( m_neverShowRescue, 0, wxALL, 5 );


	bRightColumn->Add( bSizer14, 0, wxEXPAND|wxTOP|wxLEFT, 5 );


	bPanelSizer->Add( bRightColumn, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_EESCHEMA_EDITING_OPTIONS_BASE::~PANEL_EESCHEMA_EDITING_OPTIONS_BASE()
{
}
