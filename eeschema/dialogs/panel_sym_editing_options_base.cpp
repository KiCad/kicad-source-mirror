///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_sym_editing_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SYM_EDITING_OPTIONS_BASE::PANEL_SYM_EDITING_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* p1mainSizer;
	p1mainSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* leftColumn;
	leftColumn = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* defaults;
	defaults = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Defaults for New Objects") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer;
	fgSizer = new wxFlexGridSizer( 0, 3, 3, 0 );
	fgSizer->AddGrowableCol( 1 );
	fgSizer->SetFlexibleDirection( wxBOTH );
	fgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lineWidthLabel = new wxStaticText( defaults->GetStaticBox(), wxID_ANY, _("&Default line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthLabel->Wrap( -1 );
	fgSizer->Add( m_lineWidthLabel, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_lineWidthCtrl = new wxTextCtrl( defaults->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizer->Add( m_lineWidthCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_lineWidthUnits = new wxStaticText( defaults->GetStaticBox(), wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthUnits->Wrap( -1 );
	fgSizer->Add( m_lineWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 3 );

	m_textSizeLabel = new wxStaticText( defaults->GetStaticBox(), wxID_ANY, _("Default text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeLabel->Wrap( -1 );
	fgSizer->Add( m_textSizeLabel, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_textSizeCtrl = new wxTextCtrl( defaults->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_textSizeCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_textSizeUnits = new wxStaticText( defaults->GetStaticBox(), wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	fgSizer->Add( m_textSizeUnits, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	fgSizer->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_pinLengthLabel = new wxStaticText( defaults->GetStaticBox(), wxID_ANY, _("D&efault pin length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinLengthLabel->Wrap( -1 );
	fgSizer->Add( m_pinLengthLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_pinLengthCtrl = new wxTextCtrl( defaults->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizer->Add( m_pinLengthCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_pinLengthUnits = new wxStaticText( defaults->GetStaticBox(), wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinLengthUnits->Wrap( -1 );
	fgSizer->Add( m_pinLengthUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 3 );

	m_pinNumSizeLabel = new wxStaticText( defaults->GetStaticBox(), wxID_ANY, _("De&fault pin number size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinNumSizeLabel->Wrap( -1 );
	fgSizer->Add( m_pinNumSizeLabel, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_pinNumSizeCtrl = new wxTextCtrl( defaults->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizer->Add( m_pinNumSizeCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_pinNumSizeUnits = new wxStaticText( defaults->GetStaticBox(), wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinNumSizeUnits->Wrap( -1 );
	fgSizer->Add( m_pinNumSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 3 );

	m_pinNameSizeLabel = new wxStaticText( defaults->GetStaticBox(), wxID_ANY, _("Def&ault pin name size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinNameSizeLabel->Wrap( -1 );
	fgSizer->Add( m_pinNameSizeLabel, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_pinNameSizeCtrl = new wxTextCtrl( defaults->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizer->Add( m_pinNameSizeCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_pinNameSizeUnits = new wxStaticText( defaults->GetStaticBox(), wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinNameSizeUnits->Wrap( -1 );
	fgSizer->Add( m_pinNameSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 3 );


	defaults->Add( fgSizer, 0, wxBOTTOM|wxEXPAND, 5 );

	m_cbShowPinElectricalType = new wxCheckBox( defaults->GetStaticBox(), wxID_ANY, _("Show pin &electrical type"), wxDefaultPosition, wxDefaultSize, 0 );
	defaults->Add( m_cbShowPinElectricalType, 0, wxBOTTOM|wxLEFT, 5 );


	leftColumn->Add( defaults, 0, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* repeats;
	repeats = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Repeated Items") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 3, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_hPitchLabel = new wxStaticText( repeats->GetStaticBox(), wxID_ANY, _("&Horizontal pitch:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hPitchLabel->Wrap( -1 );
	fgSizer1->Add( m_hPitchLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_hPitchCtrl = new wxTextCtrl( repeats->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS );
	fgSizer1->Add( m_hPitchCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_hPitchUnits = new wxStaticText( repeats->GetStaticBox(), wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hPitchUnits->Wrap( -1 );
	fgSizer1->Add( m_hPitchUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );

	m_vPitchLabel = new wxStaticText( repeats->GetStaticBox(), wxID_ANY, _("&Vertical pitch:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_vPitchLabel->Wrap( -1 );
	fgSizer1->Add( m_vPitchLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_vPitchCtrl = new wxTextCtrl( repeats->GetStaticBox(), wxID_ANY, _("100"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS );
	fgSizer1->Add( m_vPitchCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_vPitchUnits = new wxStaticText( repeats->GetStaticBox(), wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_vPitchUnits->Wrap( -1 );
	fgSizer1->Add( m_vPitchUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_pinPitchLabel = new wxStaticText( repeats->GetStaticBox(), wxID_ANY, _("&Pitch of repeated pins:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinPitchLabel->Wrap( -1 );
	fgSizer1->Add( m_pinPitchLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxString m_choicePinDisplacementChoices[] = { _("100"), _("50") };
	int m_choicePinDisplacementNChoices = sizeof( m_choicePinDisplacementChoices ) / sizeof( wxString );
	m_choicePinDisplacement = new wxChoice( repeats->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choicePinDisplacementNChoices, m_choicePinDisplacementChoices, 0 );
	m_choicePinDisplacement->SetSelection( 0 );
	fgSizer1->Add( m_choicePinDisplacement, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_pinPitchUnits = new wxStaticText( repeats->GetStaticBox(), wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinPitchUnits->Wrap( -1 );
	fgSizer1->Add( m_pinPitchUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_labelIncrementLabel1 = new wxStaticText( repeats->GetStaticBox(), wxID_ANY, _("Label increment:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelIncrementLabel1->Wrap( -1 );
	fgSizer1->Add( m_labelIncrementLabel1, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_spinRepeatLabel = new wxSpinCtrl( repeats->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10, 10, 1 );
	fgSizer1->Add( m_spinRepeatLabel, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	fgSizer1->Add( 0, 0, 0, 0, 5 );


	repeats->Add( fgSizer1, 1, wxEXPAND, 5 );


	leftColumn->Add( repeats, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	p1mainSizer->Add( leftColumn, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	p1mainSizer->Add( 0, 0, 1, wxEXPAND, 5 );


	this->SetSizer( p1mainSizer );
	this->Layout();
	p1mainSizer->Fit( this );
}

PANEL_SYM_EDITING_OPTIONS_BASE::~PANEL_SYM_EDITING_OPTIONS_BASE()
{
}
