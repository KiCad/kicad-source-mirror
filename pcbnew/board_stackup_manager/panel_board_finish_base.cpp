///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/via_cross_section_preview.h"

#include "panel_board_finish_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_BOARD_FINISH_BASE::PANEL_SETUP_BOARD_FINISH_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	m_cbEgdesPlated = new wxCheckBox( this, wxID_ANY, _("Plated board edge"), wxDefaultPosition, wxDefaultSize, 0 );
	bMargins->Add( m_cbEgdesPlated, 0, wxBOTTOM|wxLEFT, 5 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 2, 5, 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextFinish = new wxStaticText( this, wxID_ANY, _("Copper finish:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFinish->Wrap( -1 );
	fgSizer2->Add( m_staticTextFinish, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_choiceFinishChoices;
	m_choiceFinish = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceFinishChoices, 0 );
	m_choiceFinish->SetSelection( 0 );
	fgSizer2->Add( m_choiceFinish, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 2 );

	m_staticTextEdgeConn = new wxStaticText( this, wxID_ANY, _("Edge card connectors:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextEdgeConn->Wrap( -1 );
	fgSizer2->Add( m_staticTextEdgeConn, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_choiceEdgeConnChoices[] = { _("None"), _("Yes"), _("Yes, bevelled") };
	int m_choiceEdgeConnNChoices = sizeof( m_choiceEdgeConnChoices ) / sizeof( wxString );
	m_choiceEdgeConn = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceEdgeConnNChoices, m_choiceEdgeConnChoices, 0 );
	m_choiceEdgeConn->SetSelection( 0 );
	m_choiceEdgeConn->SetToolTip( _("Options for edge card connectors.") );

	fgSizer2->Add( m_choiceEdgeConn, 0, wxEXPAND, 2 );


	bMargins->Add( fgSizer2, 0, wxEXPAND|wxTOP, 10 );

	wxStaticBoxSizer* sbViaProtection;
	sbViaProtection = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Via Protection") ), wxHORIZONTAL );

	wxBoxSizer* bViaProtectionCtrls;
	bViaProtectionCtrls = new wxBoxSizer( wxVERTICAL );

	m_viaProtectionHint = new wxStaticText( sbViaProtection->GetStaticBox(), wxID_ANY, _("Defaults for vias set to \"From rules\"."), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaProtectionHint->Wrap( -1 );
	bViaProtectionCtrls->Add( m_viaProtectionHint, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	wxFlexGridSizer* fgViaProtection;
	fgViaProtection = new wxFlexGridSizer( 0, 3, 5, 0 );
	fgViaProtection->SetFlexibleDirection( wxBOTH );
	fgViaProtection->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_fillViasLabel = new wxStaticText( sbViaProtection->GetStaticBox(), wxID_ANY, _("Fill vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fillViasLabel->Wrap( -1 );
	fgViaProtection->Add( m_fillViasLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_fillVias = new wxCheckBox( sbViaProtection->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fillVias->SetToolTip( _("Filled: the hole is completely filled.") );

	fgViaProtection->Add( m_fillVias, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	fgViaProtection->Add( 0, 0, 1, wxEXPAND, 5 );

	m_capViasLabel = new wxStaticText( sbViaProtection->GetStaticBox(), wxID_ANY, _("Cap vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_capViasLabel->Wrap( -1 );
	fgViaProtection->Add( m_capViasLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_capVias = new wxCheckBox( sbViaProtection->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_capVias->SetToolTip( _("Capped: copper is plated over the filled hole, giving a flat solderable surface.") );

	fgViaProtection->Add( m_capVias, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	fgViaProtection->Add( 0, 0, 1, wxEXPAND, 5 );

	m_plugViasLabel = new wxStaticText( sbViaProtection->GetStaticBox(), wxID_ANY, _("Plug vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plugViasLabel->Wrap( -1 );
	fgViaProtection->Add( m_plugViasLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_plugViasFront = new wxCheckBox( sbViaProtection->GetStaticBox(), wxID_ANY, _("Front"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plugViasFront->SetToolTip( _("Plugged: the hole is partly filled from this side.") );

	fgViaProtection->Add( m_plugViasFront, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_plugViasBack = new wxCheckBox( sbViaProtection->GetStaticBox(), wxID_ANY, _("Back"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plugViasBack->SetToolTip( _("Plugged: the hole is partly filled from this side.") );

	fgViaProtection->Add( m_plugViasBack, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_tentViasLabel = new wxStaticText( sbViaProtection->GetStaticBox(), wxID_ANY, _("Tent vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_tentViasLabel->Wrap( -1 );
	fgViaProtection->Add( m_tentViasLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_tentViasFront = new wxCheckBox( sbViaProtection->GetStaticBox(), wxID_ANY, _("Front"), wxDefaultPosition, wxDefaultSize, 0 );
	m_tentViasFront->SetToolTip( _("Tented: vias are covered with solder mask.\nNot tented: vias are not covered with solder mask.") );

	fgViaProtection->Add( m_tentViasFront, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_tentViasBack = new wxCheckBox( sbViaProtection->GetStaticBox(), wxID_ANY, _("Back"), wxDefaultPosition, wxDefaultSize, 0 );
	m_tentViasBack->SetToolTip( _("Tented: vias are covered with solder mask.\nNot tented: vias are not covered with solder mask.") );

	fgViaProtection->Add( m_tentViasBack, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_coverViasLabel = new wxStaticText( sbViaProtection->GetStaticBox(), wxID_ANY, _("Cover vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_coverViasLabel->Wrap( -1 );
	fgViaProtection->Add( m_coverViasLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_coverViasFront = new wxCheckBox( sbViaProtection->GetStaticBox(), wxID_ANY, _("Front"), wxDefaultPosition, wxDefaultSize, 0 );
	m_coverViasFront->SetToolTip( _("Covered: a second coat is applied over the via on this side.") );

	fgViaProtection->Add( m_coverViasFront, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_coverViasBack = new wxCheckBox( sbViaProtection->GetStaticBox(), wxID_ANY, _("Back"), wxDefaultPosition, wxDefaultSize, 0 );
	m_coverViasBack->SetToolTip( _("Covered: a second coat is applied over the via on this side.") );

	fgViaProtection->Add( m_coverViasBack, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	bViaProtectionCtrls->Add( fgViaProtection, 0, wxEXPAND, 5 );


	sbViaProtection->Add( bViaProtectionCtrls, 0, wxBOTTOM|wxEXPAND, 5 );

	m_viaPreview = new VIA_CROSS_SECTION_PREVIEW( sbViaProtection->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	sbViaProtection->Add( m_viaPreview, 1, wxALL|wxEXPAND, 5 );


	bMargins->Add( sbViaProtection, 0, wxEXPAND|wxTOP, 5 );


	bMainSizer->Add( bMargins, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_SETUP_BOARD_FINISH_BASE::~PANEL_SETUP_BOARD_FINISH_BASE()
{
}
