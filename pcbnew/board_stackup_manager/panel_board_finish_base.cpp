///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

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


	bMargins->Add( fgSizer2, 1, wxEXPAND|wxTOP, 10 );


	bMainSizer->Add( bMargins, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_SETUP_BOARD_FINISH_BASE::~PANEL_SETUP_BOARD_FINISH_BASE()
{
}
