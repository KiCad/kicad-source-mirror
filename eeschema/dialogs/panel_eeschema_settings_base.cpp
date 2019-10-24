///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 24 2019)
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

	wxFlexGridSizer* fgSizerRepeatOpt;
	fgSizerRepeatOpt = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgSizerRepeatOpt->AddGrowableCol( 0 );
	fgSizerRepeatOpt->AddGrowableCol( 1 );
	fgSizerRepeatOpt->AddGrowableCol( 2 );
	fgSizerRepeatOpt->SetFlexibleDirection( wxBOTH );
	fgSizerRepeatOpt->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText2 = new wxStaticText( this, wxID_ANY, _("&Measurement units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizerRepeatOpt->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_choiceUnitsChoices[] = { _("inches"), _("millimeters") };
	int m_choiceUnitsNChoices = sizeof( m_choiceUnitsChoices ) / sizeof( wxString );
	m_choiceUnits = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnitsNChoices, m_choiceUnitsChoices, 0 );
	m_choiceUnits->SetSelection( 0 );
	fgSizerRepeatOpt->Add( m_choiceUnits, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	fgSizerRepeatOpt->Add( 0, 0, 1, wxEXPAND, 3 );

	m_textSizeLabel = new wxStaticText( this, wxID_ANY, _("Default schematic text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeLabel->Wrap( -1 );
	fgSizerRepeatOpt->Add( m_textSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_textSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizerRepeatOpt->Add( m_textSizeCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 10 );

	m_textSizeUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	fgSizerRepeatOpt->Add( m_textSizeUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_hPitchLabel = new wxStaticText( this, wxID_ANY, _("&Horizontal pitch of repeated items:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hPitchLabel->Wrap( -1 );
	fgSizerRepeatOpt->Add( m_hPitchLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_hPitchCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizerRepeatOpt->Add( m_hPitchCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_hPitchUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hPitchUnits->Wrap( -1 );
	fgSizerRepeatOpt->Add( m_hPitchUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_vPitchLabel = new wxStaticText( this, wxID_ANY, _("&Vertical pitch of repeated items:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_vPitchLabel->Wrap( -1 );
	fgSizerRepeatOpt->Add( m_vPitchLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_vPitchCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizerRepeatOpt->Add( m_vPitchCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_vPitchUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_vPitchUnits->Wrap( -1 );
	fgSizerRepeatOpt->Add( m_vPitchUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText16 = new wxStaticText( this, wxID_ANY, _("&Increment of repeated labels:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	fgSizerRepeatOpt->Add( m_staticText16, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_spinRepeatLabel = new wxSpinCtrl( this, wxID_ANY, wxT("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, -10, 10, 1 );
	fgSizerRepeatOpt->Add( m_spinRepeatLabel, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );


	fgSizerRepeatOpt->Add( 0, 0, 1, wxEXPAND, 3 );


	bLeftColumn->Add( fgSizerRepeatOpt, 0, wxLEFT|wxRIGHT, 5 );

	wxStaticBoxSizer* sbSizerEditOpt;
	sbSizerEditOpt = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Editing") ), wxVERTICAL );

	m_checkHVOrientation = new wxCheckBox( sbSizerEditOpt->GetStaticBox(), wxID_ANY, _("&Restrict buses and wires to H and V orientation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkHVOrientation->SetValue(true);
	sbSizerEditOpt->Add( m_checkHVOrientation, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_mouseDragIsDrag = new wxCheckBox( sbSizerEditOpt->GetStaticBox(), wxID_ANY, _("Mouse drag performs drag (G) operation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mouseDragIsDrag->SetToolTip( _("If unchecked, mouse drag will perform move (M) operation") );

	sbSizerEditOpt->Add( m_mouseDragIsDrag, 0, wxALL, 5 );


	bLeftColumn->Add( sbSizerEditOpt, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxStaticBoxSizer* sbSizerPinSel;
	sbSizerPinSel = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Selection") ), wxVERTICAL );

	m_cbPinSelectionOpt = new wxCheckBox( sbSizerPinSel->GetStaticBox(), wxID_ANY, _("Select a pin select the symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbPinSelectionOpt->SetToolTip( _("In schematic editor:\nIf enabled, clicking on a pin select the parent symbol.\nIf disabled, clicking on a pin select only the pin.") );

	sbSizerPinSel->Add( m_cbPinSelectionOpt, 0, wxALL, 5 );


	bLeftColumn->Add( sbSizerPinSel, 0, wxEXPAND, 5 );


	bPanelSizer->Add( bLeftColumn, 0, wxRIGHT|wxLEFT, 5 );

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


	bRightColumn->Add( sbSizerFieldAutoPlace, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxBoxSizer* bSizerPreviewOpt;
	bSizerPreviewOpt = new wxBoxSizer( wxVERTICAL );

	m_footprintPreview = new wxCheckBox( this, wxID_ANY, _("Show footprint previews in symbol chooser"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerPreviewOpt->Add( m_footprintPreview, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_navigatorStaysOpen = new wxCheckBox( this, wxID_ANY, _("Keep hierarchy navigator open"), wxDefaultPosition, wxDefaultSize, 0 );
	m_navigatorStaysOpen->SetValue(true);
	bSizerPreviewOpt->Add( m_navigatorStaysOpen, 0, wxALL, 5 );


	bRightColumn->Add( bSizerPreviewOpt, 0, wxALL, 5 );


	bPanelSizer->Add( bRightColumn, 1, wxEXPAND, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_EESCHEMA_SETTINGS_BASE::~PANEL_EESCHEMA_SETTINGS_BASE()
{
}
