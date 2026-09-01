///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"

#include "dialog_microvia_stack_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MICROVIA_STACK_BASE::DIALOG_MICROVIA_STACK_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer;
	fgSizer = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgSizer->AddGrowableCol( 1 );
	fgSizer->SetFlexibleDirection( wxBOTH );
	fgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_startLayerLabel = new wxStaticText( this, wxID_ANY, _("Start layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_startLayerLabel->Wrap( -1 );
	fgSizer->Add( m_startLayerLabel, 0, wxALL, 5 );

	m_startLayer = new PCB_LAYER_BOX_SELECTOR( this, wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgSizer->Add( m_startLayer, 0, wxEXPAND, 5 );


	fgSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_endLayerLabel = new wxStaticText( this, wxID_ANY, _("End layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_endLayerLabel->Wrap( -1 );
	fgSizer->Add( m_endLayerLabel, 0, wxALL, 5 );

	m_endLayer = new PCB_LAYER_BOX_SELECTOR( this, wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgSizer->Add( m_endLayer, 0, wxEXPAND, 5 );


	fgSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_typeLabel = new wxStaticText( this, wxID_ANY, _("Type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_typeLabel->Wrap( -1 );
	fgSizer->Add( m_typeLabel, 0, wxALL, 5 );

	wxString m_typeChoiceChoices[] = { _("Stacked"), _("Staggered") };
	int m_typeChoiceNChoices = sizeof( m_typeChoiceChoices ) / sizeof( wxString );
	m_typeChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_typeChoiceNChoices, m_typeChoiceChoices, 0 );
	m_typeChoice->SetSelection( 0 );
	fgSizer->Add( m_typeChoice, 0, wxALL|wxEXPAND, 5 );


	fgSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_viaSizeLabel = new wxStaticText( this, wxID_ANY, _("Via diameter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaSizeLabel->Wrap( -1 );
	fgSizer->Add( m_viaSizeLabel, 0, wxALL, 5 );

	m_viaSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_viaSizeCtrl, 0, wxALL|wxEXPAND, 5 );

	m_viaSizeUnit = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaSizeUnit->Wrap( -1 );
	fgSizer->Add( m_viaSizeUnit, 0, wxALL, 5 );

	m_viaDrillLabel = new wxStaticText( this, wxID_ANY, _("Via hole:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaDrillLabel->Wrap( -1 );
	fgSizer->Add( m_viaDrillLabel, 0, wxALL, 5 );

	m_viaDrillCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_viaDrillCtrl, 0, wxALL|wxEXPAND, 5 );

	m_viaDrillUnit = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaDrillUnit->Wrap( -1 );
	fgSizer->Add( m_viaDrillUnit, 0, wxALL, 5 );

	m_useNetclass = new wxCheckBox( this, wxID_ANY, _("Use netclass values"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_useNetclass, 0, wxALL, 5 );


	fgSizer->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_filled = new wxCheckBox( this, wxID_ANY, _("Copper-filled"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_filled, 0, wxALL, 5 );

	m_capped = new wxCheckBox( this, wxID_ANY, _("Capped"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_capped, 0, wxALL, 5 );


	fgSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_pitchLabel = new wxStaticText( this, wxID_ANY, _("Pitch:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pitchLabel->Wrap( -1 );
	fgSizer->Add( m_pitchLabel, 0, wxALL, 5 );

	m_pitchCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_pitchCtrl, 0, wxALL|wxEXPAND, 5 );

	m_pitchUnit = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pitchUnit->Wrap( -1 );
	fgSizer->Add( m_pitchUnit, 0, wxALL, 5 );


	bMainSizer->Add( fgSizer, 1, wxALL|wxEXPAND, 5 );

	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();

	bMainSizer->Add( m_stdButtons, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_MICROVIA_STACK_BASE::~DIALOG_MICROVIA_STACK_BASE()
{
}
