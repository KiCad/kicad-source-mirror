///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_eeschema_editing_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_EESCHEMA_EDITING_OPTIONS_BASE::PANEL_EESCHEMA_EDITING_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftColumn;
	bLeftColumn = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerEditOpt;
	sbSizerEditOpt = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Editing") ), wxVERTICAL );

	m_checkHVOrientation = new wxCheckBox( sbSizerEditOpt->GetStaticBox(), wxID_ANY, _("&Restrict buses and wires to H and V orientation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkHVOrientation->SetValue(true);
	sbSizerEditOpt->Add( m_checkHVOrientation, 0, wxRIGHT|wxLEFT, 5 );

	m_mouseDragIsDrag = new wxCheckBox( sbSizerEditOpt->GetStaticBox(), wxID_ANY, _("Mouse drag performs drag (G) operation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mouseDragIsDrag->SetToolTip( _("If unchecked, mouse drag will perform move (M) operation") );

	sbSizerEditOpt->Add( m_mouseDragIsDrag, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_cbAutoStartWires = new wxCheckBox( sbSizerEditOpt->GetStaticBox(), wxID_ANY, _("Automatically start wires on unconnected pins"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbAutoStartWires->SetToolTip( _("When enabled, you can start wiring by clicking on unconnected pins even when the wire tool is not active") );

	sbSizerEditOpt->Add( m_cbAutoStartWires, 0, wxALL, 5 );


	bLeftColumn->Add( sbSizerEditOpt, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxStaticBoxSizer* sbSizer5;
	sbSizer5 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Defaults for New Objects") ), wxVERTICAL );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );

	m_borderColorLabel = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("Sheet border:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderColorLabel->Wrap( -1 );
	bSizer6->Add( m_borderColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_borderColorSwatch = new COLOR_SWATCH( sbSizer5->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_borderColorSwatch->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	bSizer6->Add( m_borderColorSwatch, 1, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_backgroundColorLabel = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("Sheet background:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_backgroundColorLabel->Wrap( -1 );
	bSizer6->Add( m_backgroundColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_backgroundColorSwatch = new COLOR_SWATCH( sbSizer5->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_backgroundColorSwatch->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	bSizer6->Add( m_backgroundColorSwatch, 1, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	sbSizer5->Add( bSizer6, 0, wxEXPAND, 5 );


	bLeftColumn->Add( sbSizer5, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxStaticBoxSizer* sbSizerPinSel;
	sbSizerPinSel = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Selection") ), wxVERTICAL );

	m_cbPinSelectionOpt = new wxCheckBox( sbSizerPinSel->GetStaticBox(), wxID_ANY, _("Clicking on a pin selects the symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbPinSelectionOpt->SetToolTip( _("In schematic editor:\nIf enabled, clicking on a pin select the parent symbol.\nIf disabled, clicking on a pin select only the pin.") );

	sbSizerPinSel->Add( m_cbPinSelectionOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bLeftColumn->Add( sbSizerPinSel, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_leftClickCmdsBook = new wxSimplebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_pageWinLin = new wxPanel( m_leftClickCmdsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxStaticBoxSizer* mouseCmdsWinLin;
	mouseCmdsWinLin = new wxStaticBoxSizer( new wxStaticBox( m_pageWinLin, wxID_ANY, _("Left Click Mouse Commands") ), wxVERTICAL );

	m_staticText8 = new wxStaticText( mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Left click (and drag) actions depend on 3 modifier keys:\nAlt, Shift and Ctrl."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	mouseCmdsWinLin->Add( m_staticText8, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticline1 = new wxStaticLine( mouseCmdsWinLin->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mouseCmdsWinLin->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	wxFlexGridSizer* fgSizerCmdsWinLin;
	fgSizerCmdsWinLin = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizerCmdsWinLin->SetFlexibleDirection( wxBOTH );
	fgSizerCmdsWinLin->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText91 = new wxStaticText( mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Shift"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText91->Wrap( -1 );
	fgSizerCmdsWinLin->Add( m_staticText91, 0, wxALL, 5 );

	m_staticText101 = new wxStaticText( mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Add item(s) to selection."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText101->Wrap( -1 );
	fgSizerCmdsWinLin->Add( m_staticText101, 0, wxALL, 5 );

	m_staticText131 = new wxStaticText( mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Alt"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText131->Wrap( -1 );
	fgSizerCmdsWinLin->Add( m_staticText131, 0, wxALL, 5 );

	m_staticText141 = new wxStaticText( mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Toggle selected state of item(s)."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText141->Wrap( -1 );
	fgSizerCmdsWinLin->Add( m_staticText141, 0, wxALL, 5 );

	m_staticText151 = new wxStaticText( mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Alt+Shift"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText151->Wrap( -1 );
	fgSizerCmdsWinLin->Add( m_staticText151, 0, wxALL, 5 );

	m_staticText161 = new wxStaticText( mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Remove item(s) from selection."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText161->Wrap( -1 );
	fgSizerCmdsWinLin->Add( m_staticText161, 0, wxALL, 5 );

	m_staticText111 = new wxStaticText( mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Ctrl"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText111->Wrap( -1 );
	fgSizerCmdsWinLin->Add( m_staticText111, 0, wxALL, 5 );

	m_staticText121 = new wxStaticText( mouseCmdsWinLin->GetStaticBox(), wxID_ANY, _("Clarify selection from menu."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText121->Wrap( -1 );
	fgSizerCmdsWinLin->Add( m_staticText121, 0, wxALL, 5 );


	mouseCmdsWinLin->Add( fgSizerCmdsWinLin, 1, wxEXPAND, 5 );


	m_pageWinLin->SetSizer( mouseCmdsWinLin );
	m_pageWinLin->Layout();
	mouseCmdsWinLin->Fit( m_pageWinLin );
	m_leftClickCmdsBook->AddPage( m_pageWinLin, _("a page"), false );
	m_pageMac = new wxPanel( m_leftClickCmdsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxStaticBoxSizer* mouseCmdsMac;
	mouseCmdsMac = new wxStaticBoxSizer( new wxStaticBox( m_pageMac, wxID_ANY, _("Left Click Mouse Commands") ), wxVERTICAL );

	m_staticText81 = new wxStaticText( mouseCmdsMac->GetStaticBox(), wxID_ANY, _("Left click (and drag) actions depend on 3 modifier keys:\nAlt, Shift and Cmd."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText81->Wrap( -1 );
	mouseCmdsMac->Add( m_staticText81, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticline2 = new wxStaticLine( mouseCmdsMac->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mouseCmdsMac->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	wxFlexGridSizer* fgSizerCmdsWinMac;
	fgSizerCmdsWinMac = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizerCmdsWinMac->SetFlexibleDirection( wxBOTH );
	fgSizerCmdsWinMac->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText9 = new wxStaticText( mouseCmdsMac->GetStaticBox(), wxID_ANY, _("Shift"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizerCmdsWinMac->Add( m_staticText9, 0, wxALL, 5 );

	m_staticText10 = new wxStaticText( mouseCmdsMac->GetStaticBox(), wxID_ANY, _("Add item(s) to selection."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	fgSizerCmdsWinMac->Add( m_staticText10, 0, wxALL, 5 );

	m_staticText13 = new wxStaticText( mouseCmdsMac->GetStaticBox(), wxID_ANY, _("Cmd"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText13->Wrap( -1 );
	fgSizerCmdsWinMac->Add( m_staticText13, 0, wxALL, 5 );

	m_staticText14 = new wxStaticText( mouseCmdsMac->GetStaticBox(), wxID_ANY, _("Toggle selected state of item(s)."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	fgSizerCmdsWinMac->Add( m_staticText14, 0, wxALL, 5 );

	m_staticText15 = new wxStaticText( mouseCmdsMac->GetStaticBox(), wxID_ANY, _("Cmd+Shift"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	fgSizerCmdsWinMac->Add( m_staticText15, 0, wxALL, 5 );

	m_staticText16 = new wxStaticText( mouseCmdsMac->GetStaticBox(), wxID_ANY, _("Remove item(s) from selection."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	fgSizerCmdsWinMac->Add( m_staticText16, 0, wxALL, 5 );

	m_staticText11 = new wxStaticText( mouseCmdsMac->GetStaticBox(), wxID_ANY, _("Alt"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	fgSizerCmdsWinMac->Add( m_staticText11, 0, wxALL, 5 );

	m_staticText12 = new wxStaticText( mouseCmdsMac->GetStaticBox(), wxID_ANY, _("Clarify selection from menu."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	fgSizerCmdsWinMac->Add( m_staticText12, 0, wxALL, 5 );


	mouseCmdsMac->Add( fgSizerCmdsWinMac, 1, wxEXPAND, 5 );


	m_pageMac->SetSizer( mouseCmdsMac );
	m_pageMac->Layout();
	mouseCmdsMac->Fit( m_pageMac );
	m_leftClickCmdsBook->AddPage( m_pageMac, _("a page"), false );

	bLeftColumn->Add( m_leftClickCmdsBook, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	bPanelSizer->Add( bLeftColumn, 1, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	wxBoxSizer* bRightColumn;
	bRightColumn = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerFieldAutoPlace;
	sbSizerFieldAutoPlace = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Symbol Field Automatic Placement") ), wxVERTICAL );

	m_checkAutoplaceFields = new wxCheckBox( sbSizerFieldAutoPlace->GetStaticBox(), wxID_ANY, _("A&utomatically place symbol fields"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerFieldAutoPlace->Add( m_checkAutoplaceFields, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_checkAutoplaceJustify = new wxCheckBox( sbSizerFieldAutoPlace->GetStaticBox(), wxID_ANY, _("A&llow field autoplace to change justification"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerFieldAutoPlace->Add( m_checkAutoplaceJustify, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_checkAutoplaceAlign = new wxCheckBox( sbSizerFieldAutoPlace->GetStaticBox(), wxID_ANY, _("Al&ways align autoplaced fields to the 50 mil grid"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerFieldAutoPlace->Add( m_checkAutoplaceAlign, 0, wxEXPAND|wxALL, 5 );


	bRightColumn->Add( sbSizerFieldAutoPlace, 0, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* repeatSizer;
	repeatSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Repeated Items") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerRepeatOpt1;
	fgSizerRepeatOpt1 = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgSizerRepeatOpt1->AddGrowableCol( 0 );
	fgSizerRepeatOpt1->AddGrowableCol( 1 );
	fgSizerRepeatOpt1->AddGrowableCol( 2 );
	fgSizerRepeatOpt1->SetFlexibleDirection( wxBOTH );
	fgSizerRepeatOpt1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_hPitchLabel = new wxStaticText( repeatSizer->GetStaticBox(), wxID_ANY, _("&Horizontal pitch:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hPitchLabel->Wrap( -1 );
	fgSizerRepeatOpt1->Add( m_hPitchLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_hPitchCtrl = new wxTextCtrl( repeatSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizerRepeatOpt1->Add( m_hPitchCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_hPitchUnits = new wxStaticText( repeatSizer->GetStaticBox(), wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hPitchUnits->Wrap( -1 );
	fgSizerRepeatOpt1->Add( m_hPitchUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_vPitchLabel = new wxStaticText( repeatSizer->GetStaticBox(), wxID_ANY, _("&Vertical pitch:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_vPitchLabel->Wrap( -1 );
	fgSizerRepeatOpt1->Add( m_vPitchLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_vPitchCtrl = new wxTextCtrl( repeatSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizerRepeatOpt1->Add( m_vPitchCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_vPitchUnits = new wxStaticText( repeatSizer->GetStaticBox(), wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_vPitchUnits->Wrap( -1 );
	fgSizerRepeatOpt1->Add( m_vPitchUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_labelIncrementLabel = new wxStaticText( repeatSizer->GetStaticBox(), wxID_ANY, _("Label increment:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelIncrementLabel->Wrap( -1 );
	fgSizerRepeatOpt1->Add( m_labelIncrementLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_spinLabelRepeatStep = new wxSpinCtrl( repeatSizer->GetStaticBox(), wxID_ANY, wxT("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, -10, 10, 1 );
	fgSizerRepeatOpt1->Add( m_spinLabelRepeatStep, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );


	fgSizerRepeatOpt1->Add( 0, 0, 1, wxEXPAND, 3 );


	repeatSizer->Add( fgSizerRepeatOpt1, 1, wxEXPAND, 5 );


	bRightColumn->Add( repeatSizer, 0, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbSizer6;
	sbSizer6 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Dialog Preferences") ), wxVERTICAL );

	m_footprintPreview = new wxCheckBox( sbSizer6->GetStaticBox(), wxID_ANY, _("Show footprint previews in Symbol Chooser"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer6->Add( m_footprintPreview, 0, wxRIGHT|wxLEFT, 5 );

	m_navigatorStaysOpen = new wxCheckBox( sbSizer6->GetStaticBox(), wxID_ANY, _("Keep hierarchy navigator open"), wxDefaultPosition, wxDefaultSize, 0 );
	m_navigatorStaysOpen->SetValue(true);
	sbSizer6->Add( m_navigatorStaysOpen, 0, wxALL, 5 );


	bRightColumn->Add( sbSizer6, 1, wxEXPAND|wxALL, 5 );


	bPanelSizer->Add( bRightColumn, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_EESCHEMA_EDITING_OPTIONS_BASE::~PANEL_EESCHEMA_EDITING_OPTIONS_BASE()
{
}
