///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_libedit_settings_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_LIBEDIT_SETTINGS_BASE::PANEL_LIBEDIT_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* p1mainSizer;
	p1mainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftColumn;
	bLeftColumn = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer;
	fgSizer = new wxFlexGridSizer( 0, 3, 3, 0 );
	fgSizer->AddGrowableCol( 0 );
	fgSizer->AddGrowableCol( 1 );
	fgSizer->AddGrowableCol( 2 );
	fgSizer->SetFlexibleDirection( wxBOTH );
	fgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_lineWidthLabel = new wxStaticText( this, wxID_ANY, _("&Default line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthLabel->Wrap( -1 );
	fgSizer->Add( m_lineWidthLabel, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_lineWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizer->Add( m_lineWidthCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_lineWidthUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthUnits->Wrap( -1 );
	fgSizer->Add( m_lineWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 3 );
	
	
	fgSizer->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 10 );
	
	
	fgSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_pinLengthLabel = new wxStaticText( this, wxID_ANY, _("D&efault pin length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinLengthLabel->Wrap( -1 );
	fgSizer->Add( m_pinLengthLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );
	
	m_pinLengthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizer->Add( m_pinLengthCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_pinLengthUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinLengthUnits->Wrap( -1 );
	fgSizer->Add( m_pinLengthUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 3 );
	
	m_pinNumSizeLabel = new wxStaticText( this, wxID_ANY, _("De&fault pin number size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinNumSizeLabel->Wrap( -1 );
	fgSizer->Add( m_pinNumSizeLabel, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_pinNumSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizer->Add( m_pinNumSizeCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_pinNumSizeUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinNumSizeUnits->Wrap( -1 );
	fgSizer->Add( m_pinNumSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 3 );
	
	m_pinNameSizeLabel = new wxStaticText( this, wxID_ANY, _("Def&ault pin name size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinNameSizeLabel->Wrap( -1 );
	fgSizer->Add( m_pinNameSizeLabel, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_pinNameSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizer->Add( m_pinNameSizeCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_pinNameSizeUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinNameSizeUnits->Wrap( -1 );
	fgSizer->Add( m_pinNameSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 3 );
	
	m_checkShowPinElectricalType = new wxCheckBox( this, wxID_ANY, _("Show pin &electrical type"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_checkShowPinElectricalType, 0, wxTOP|wxBOTTOM|wxRIGHT, 3 );
	
	
	fgSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 10 );
	
	
	fgSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_hPitchLabel = new wxStaticText( this, wxID_ANY, _("&Horizontal pitch of repeated items:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hPitchLabel->Wrap( -1 );
	fgSizer->Add( m_hPitchLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );
	
	m_hPitchCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS );
	fgSizer->Add( m_hPitchCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_hPitchUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hPitchUnits->Wrap( -1 );
	fgSizer->Add( m_hPitchUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );
	
	m_vPitchLabel = new wxStaticText( this, wxID_ANY, _("&Vertical pitch of repeated items:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_vPitchLabel->Wrap( -1 );
	fgSizer->Add( m_vPitchLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_vPitchCtrl = new wxTextCtrl( this, wxID_ANY, _("100"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS );
	fgSizer->Add( m_vPitchCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_vPitchUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_vPitchUnits->Wrap( -1 );
	fgSizer->Add( m_vPitchUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_pinPitchLabel = new wxStaticText( this, wxID_ANY, _("&Pitch of repeated pins:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinPitchLabel->Wrap( -1 );
	fgSizer->Add( m_pinPitchLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	wxString m_choicePinDisplacementChoices[] = { _("100"), _("50") };
	int m_choicePinDisplacementNChoices = sizeof( m_choicePinDisplacementChoices ) / sizeof( wxString );
	m_choicePinDisplacement = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choicePinDisplacementNChoices, m_choicePinDisplacementChoices, 0 );
	m_choicePinDisplacement->SetSelection( 0 );
	fgSizer->Add( m_choicePinDisplacement, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_pinPitchUnis = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinPitchUnis->Wrap( -1 );
	fgSizer->Add( m_pinPitchUnis, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_labelIncrementLabel = new wxStaticText( this, wxID_ANY, _("&Increment of repeated labels:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelIncrementLabel->Wrap( -1 );
	fgSizer->Add( m_labelIncrementLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );
	
	m_spinRepeatLabel = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10, 10, 1 );
	fgSizer->Add( m_spinRepeatLabel, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	fgSizer->Add( 0, 0, 0, 0, 5 );
	
	
	bLeftColumn->Add( fgSizer, 0, wxALL, 5 );
	
	
	p1mainSizer->Add( bLeftColumn, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	
	this->SetSizer( p1mainSizer );
	this->Layout();
	p1mainSizer->Fit( this );
}

PANEL_LIBEDIT_SETTINGS_BASE::~PANEL_LIBEDIT_SETTINGS_BASE()
{
}
