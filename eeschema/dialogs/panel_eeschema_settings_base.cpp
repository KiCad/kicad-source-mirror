///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_eeschema_settings_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( PANEL_EESCHEMA_SETTINGS_BASE, wxPanel )
	EVT_CHOICE( wxID_ANY, PANEL_EESCHEMA_SETTINGS_BASE::_wxFB_OnChooseUnits )
END_EVENT_TABLE()

PANEL_EESCHEMA_SETTINGS_BASE::PANEL_EESCHEMA_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bLeftColumn;
	bLeftColumn = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgSizer3->AddGrowableCol( 0 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->AddGrowableCol( 2 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("&Measurement units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizer3->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	wxString m_choiceUnitsChoices[] = { _("inches"), _("millimeters") };
	int m_choiceUnitsNChoices = sizeof( m_choiceUnitsChoices ) / sizeof( wxString );
	m_choiceUnits = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnitsNChoices, m_choiceUnitsChoices, 0 );
	m_choiceUnits->SetSelection( 0 );
	fgSizer3->Add( m_choiceUnits, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	fgSizer3->Add( 0, 0, 1, wxEXPAND, 3 );
	
	m_textSizeLabel = new wxStaticText( this, wxID_ANY, _("Default schematic text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeLabel->Wrap( -1 );
	fgSizer3->Add( m_textSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_textSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizer3->Add( m_textSizeCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 10 );
	
	m_textSizeUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	fgSizer3->Add( m_textSizeUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_hPitchLabel = new wxStaticText( this, wxID_ANY, _("&Horizontal pitch of repeated items:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hPitchLabel->Wrap( -1 );
	fgSizer3->Add( m_hPitchLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_hPitchCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizer3->Add( m_hPitchCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_hPitchUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hPitchUnits->Wrap( -1 );
	fgSizer3->Add( m_hPitchUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_vPitchLabel = new wxStaticText( this, wxID_ANY, _("&Vertical pitch of repeated items:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_vPitchLabel->Wrap( -1 );
	fgSizer3->Add( m_vPitchLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_vPitchCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizer3->Add( m_vPitchCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_vPitchUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_vPitchUnits->Wrap( -1 );
	fgSizer3->Add( m_vPitchUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText16 = new wxStaticText( this, wxID_ANY, _("&Increment of repeated labels:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	fgSizer3->Add( m_staticText16, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_spinRepeatLabel = new wxSpinCtrl( this, wxID_ANY, wxT("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, -10, 10, 1 );
	fgSizer3->Add( m_spinRepeatLabel, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	
	fgSizer3->Add( 0, 0, 1, wxEXPAND, 3 );
	
	
	bLeftColumn->Add( fgSizer3, 0, wxLEFT|wxRIGHT, 5 );
	
	wxStaticBoxSizer* sbSizer11;
	sbSizer11 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Editing") ), wxVERTICAL );
	
	m_checkHVOrientation = new wxCheckBox( sbSizer11->GetStaticBox(), wxID_ANY, _("&Restrict buses and wires to H and V orientation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkHVOrientation->SetValue(true); 
	sbSizer11->Add( m_checkHVOrientation, 0, wxRIGHT|wxLEFT, 5 );
	
	m_mouseDragIsDrag = new wxCheckBox( sbSizer11->GetStaticBox(), wxID_ANY, _("Mouse drag performs drag (G) operation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mouseDragIsDrag->SetToolTip( _("If unchecked, mouse drag will perform move (M) operation") );
	
	sbSizer11->Add( m_mouseDragIsDrag, 0, wxALL, 5 );
	
	
	bLeftColumn->Add( sbSizer11, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Symbol Field Automatic Placement") ), wxVERTICAL );
	
	m_checkAutoplaceFields = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("A&utomatically place symbol fields"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_checkAutoplaceFields, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_checkAutoplaceJustify = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("A&llow field autoplace to change justification"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_checkAutoplaceJustify, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_checkAutoplaceAlign = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Al&ways align autoplaced fields to the 50 mil grid"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_checkAutoplaceAlign, 0, wxEXPAND|wxALL, 5 );
	
	
	bLeftColumn->Add( sbSizer1, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );
	
	m_footprintPreview = new wxCheckBox( this, wxID_ANY, _("Show footprint previews in symbol chooser"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_footprintPreview, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	
	bLeftColumn->Add( bSizer9, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	
	bPanelSizer->Add( bLeftColumn, 0, wxRIGHT|wxLEFT, 5 );
	
	
	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_EESCHEMA_SETTINGS_BASE::~PANEL_EESCHEMA_SETTINGS_BASE()
{
}
