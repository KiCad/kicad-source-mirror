///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_setup_drill_chart_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_DRILL_CHART_BASE::PANEL_SETUP_DRILL_CHART_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMain;
	bMain = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSymbols;
	sbSymbols = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Symbols") ), wxVERTICAL );

	wxBoxSizer* bGroupBy;
	bGroupBy = new wxBoxSizer( wxVERTICAL );

	m_groupByLabel = new wxStaticText( sbSymbols->GetStaticBox(), wxID_ANY, _("Group holes by:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_groupByLabel->Wrap( -1 );
	bGroupBy->Add( m_groupByLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	wxString m_groupByListChoices[] = { _("Hole size"), _("Slot shape"), _("Plating"), _("Layer span"), _("Drill operation"), _("Hole function"), _("IPC-4761 protection"), _("Post-machining") };
	int m_groupByListNChoices = sizeof( m_groupByListChoices ) / sizeof( wxString );
	m_groupByList = new wxCheckListBox( sbSymbols->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_groupByListNChoices, m_groupByListChoices, 0 );
	m_groupByList->SetMinSize( wxSize( -1,140 ) );

	bGroupBy->Add( m_groupByList, 1, wxEXPAND|wxALL, 5 );


	sbSymbols->Add( bGroupBy, 0, wxEXPAND, 0 );

	wxFlexGridSizer* fgSymbols;
	fgSymbols = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgSymbols->AddGrowableCol( 1 );
	fgSymbols->SetFlexibleDirection( wxBOTH );
	fgSymbols->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_markPolicyLabel = new wxStaticText( sbSymbols->GetStaticBox(), wxID_ANY, _("Default marks:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_markPolicyLabel->Wrap( -1 );
	fgSymbols->Add( m_markPolicyLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_markPolicyCtrlChoices[] = { _("Curated shapes"), _("Letters"), _("Curated shapes, then letters"), _("Hole size text") };
	int m_markPolicyCtrlNChoices = sizeof( m_markPolicyCtrlChoices ) / sizeof( wxString );
	m_markPolicyCtrl = new wxChoice( sbSymbols->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_markPolicyCtrlNChoices, m_markPolicyCtrlChoices, 0 );
	m_markPolicyCtrl->SetSelection( 0 );
	fgSymbols->Add( m_markPolicyCtrl, 0, wxEXPAND, 5 );

	m_markPolicyPad = new wxStaticText( sbSymbols->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_markPolicyPad->Wrap( -1 );
	fgSymbols->Add( m_markPolicyPad, 0, wxRIGHT, 5 );

	m_symbolSizeLabel = new wxStaticText( sbSymbols->GetStaticBox(), wxID_ANY, _("Symbol size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_symbolSizeLabel->Wrap( -1 );
	fgSymbols->Add( m_symbolSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_symbolSizeCtrl = new wxTextCtrl( sbSymbols->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSymbols->Add( m_symbolSizeCtrl, 0, wxEXPAND, 5 );

	m_symbolSizeUnits = new wxStaticText( sbSymbols->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_symbolSizeUnits->Wrap( -1 );
	fgSymbols->Add( m_symbolSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_symbolWidthLabel = new wxStaticText( sbSymbols->GetStaticBox(), wxID_ANY, _("Symbol line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_symbolWidthLabel->Wrap( -1 );
	fgSymbols->Add( m_symbolWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_symbolWidthCtrl = new wxTextCtrl( sbSymbols->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSymbols->Add( m_symbolWidthCtrl, 0, wxEXPAND, 5 );

	m_symbolWidthUnits = new wxStaticText( sbSymbols->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_symbolWidthUnits->Wrap( -1 );
	fgSymbols->Add( m_symbolWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	sbSymbols->Add( fgSymbols, 0, wxEXPAND|wxALL, 5 );

	m_freezeAssignments = new wxCheckBox( sbSymbols->GetStaticBox(), wxID_ANY, _("Keep existing symbol assignments when holes change"), wxDefaultPosition, wxDefaultSize, 0 );
	m_freezeAssignments->SetValue(true);
	sbSymbols->Add( m_freezeAssignments, 0, wxALL|wxEXPAND, 5 );


	bMain->Add( sbSymbols, 1, wxEXPAND|wxALL, 5 );

	m_editGroupsButton = new wxButton( this, wxID_ANY, _("Close and Edit Drill Groups..."), wxDefaultPosition, wxDefaultSize, 0 );
	bMain->Add( m_editGroupsButton, 0, wxALL, 5 );


	this->SetSizer( bMain );
	this->Layout();
	bMain->Fit( this );

	// Connect Events
	m_editGroupsButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_DRILL_CHART_BASE::onEditGroups ), NULL, this );
}

PANEL_SETUP_DRILL_CHART_BASE::~PANEL_SETUP_DRILL_CHART_BASE()
{
	// Disconnect Events
	m_editGroupsButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_DRILL_CHART_BASE::onEditGroups ), NULL, this );

}
