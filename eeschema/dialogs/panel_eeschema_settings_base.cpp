///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_eeschema_settings_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( PANEL_EESCHEMA_SETTINGS_BASE, wxPanel )
	EVT_CHOICE( wxID_ANY, PANEL_EESCHEMA_SETTINGS_BASE::_wxFB_OnChooseUnits )
END_EVENT_TABLE()

PANEL_EESCHEMA_SETTINGS_BASE::PANEL_EESCHEMA_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftColumn;
	bLeftColumn = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer7;
	sbSizer7 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Units") ), wxHORIZONTAL );

	m_staticText2 = new wxStaticText( sbSizer7->GetStaticBox(), wxID_ANY, _("&Measurement units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	sbSizer7->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxString m_choiceUnitsChoices[] = { _("inches"), _("millimeters") };
	int m_choiceUnitsNChoices = sizeof( m_choiceUnitsChoices ) / sizeof( wxString );
	m_choiceUnits = new wxChoice( sbSizer7->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnitsNChoices, m_choiceUnitsChoices, 0 );
	m_choiceUnits->SetSelection( 0 );
	sbSizer7->Add( m_choiceUnits, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bLeftColumn->Add( sbSizer7, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxStaticBoxSizer* sbSizerEditOpt;
	sbSizerEditOpt = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Editing") ), wxVERTICAL );

	m_checkHVOrientation = new wxCheckBox( sbSizerEditOpt->GetStaticBox(), wxID_ANY, _("&Restrict buses and wires to H and V orientation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkHVOrientation->SetValue(true);
	sbSizerEditOpt->Add( m_checkHVOrientation, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_mouseDragIsDrag = new wxCheckBox( sbSizerEditOpt->GetStaticBox(), wxID_ANY, _("Mouse drag performs drag (G) operation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mouseDragIsDrag->SetToolTip( _("If unchecked, mouse drag will perform move (M) operation") );

	sbSizerEditOpt->Add( m_mouseDragIsDrag, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_cbAutoStartWires = new wxCheckBox( sbSizerEditOpt->GetStaticBox(), wxID_ANY, _("Automatically start wires on junctions and unused anchors"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerEditOpt->Add( m_cbAutoStartWires, 0, wxALL, 5 );


	bLeftColumn->Add( sbSizerEditOpt, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxStaticBoxSizer* sbSizer5;
	sbSizer5 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Defaults for New Objects") ), wxVERTICAL );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );

	m_borderColorLabel = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("Sheet border:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderColorLabel->Wrap( -1 );
	bSizer6->Add( m_borderColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_borderColorSwatch = new COLOR_SWATCH( sbSizer5->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_borderColorSwatch->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_borderColorSwatch->SetMinSize( wxSize( 48,24 ) );

	bSizer6->Add( m_borderColorSwatch, 1, wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_backgroundColorLabel = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("Sheet background:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_backgroundColorLabel->Wrap( -1 );
	bSizer6->Add( m_backgroundColorLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_backgroundColorSwatch = new COLOR_SWATCH( sbSizer5->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_backgroundColorSwatch->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_backgroundColorSwatch->SetMinSize( wxSize( 48,24 ) );

	bSizer6->Add( m_backgroundColorSwatch, 1, wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	sbSizer5->Add( bSizer6, 0, wxEXPAND|wxBOTTOM, 5 );


	bLeftColumn->Add( sbSizer5, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxStaticBoxSizer* sbSizerPinSel;
	sbSizerPinSel = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Selection") ), wxVERTICAL );

	m_cbPinSelectionOpt = new wxCheckBox( sbSizerPinSel->GetStaticBox(), wxID_ANY, _("Clicking on a pin selects the symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbPinSelectionOpt->SetToolTip( _("In schematic editor:\nIf enabled, clicking on a pin select the parent symbol.\nIf disabled, clicking on a pin select only the pin.") );

	sbSizerPinSel->Add( m_cbPinSelectionOpt, 0, wxALL, 5 );


	bLeftColumn->Add( sbSizerPinSel, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	bPanelSizer->Add( bLeftColumn, 1, wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bRightColumn;
	bRightColumn = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerFieldAutoPlace;
	sbSizerFieldAutoPlace = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Symbol Field Automatic Placement") ), wxVERTICAL );

	m_checkAutoplaceFields = new wxCheckBox( sbSizerFieldAutoPlace->GetStaticBox(), wxID_ANY, _("A&utomatically place symbol fields"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerFieldAutoPlace->Add( m_checkAutoplaceFields, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

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
	sbSizer6->Add( m_footprintPreview, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_navigatorStaysOpen = new wxCheckBox( sbSizer6->GetStaticBox(), wxID_ANY, _("Keep hierarchy navigator open"), wxDefaultPosition, wxDefaultSize, 0 );
	m_navigatorStaysOpen->SetValue(true);
	sbSizer6->Add( m_navigatorStaysOpen, 0, wxALL, 5 );


	bRightColumn->Add( sbSizer6, 0, wxEXPAND|wxALL, 5 );


	bPanelSizer->Add( bRightColumn, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_EESCHEMA_SETTINGS_BASE::~PANEL_EESCHEMA_SETTINGS_BASE()
{
}
