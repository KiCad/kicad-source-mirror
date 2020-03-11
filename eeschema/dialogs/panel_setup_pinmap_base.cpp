///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_setup_pinmap_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_PINMAP_BASE::PANEL_SETUP_PINMAP_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* m_panelMatrixSizer;
	m_panelMatrixSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Pin to Pin Connections") ), wxVERTICAL );

	m_matrixPanel = new wxPanel( sbSizer3->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_matrixPanel->SetMinSize( wxSize( 500,420 ) );

	sbSizer3->Add( m_matrixPanel, 1, wxEXPAND | wxALL, 5 );


	m_panelMatrixSizer->Add( sbSizer3, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );


	m_panelMatrixSizer->Add( 0, 0, 0, wxEXPAND|wxTOP, 5 );

	m_ResetOptButton = new wxButton( this, ID_RESET_MATRIX, _("Reset to Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	m_panelMatrixSizer->Add( m_ResetOptButton, 0, wxRIGHT|wxLEFT, 10 );


	bSizer2->Add( m_panelMatrixSizer, 1, wxEXPAND|wxLEFT, 10 );


	this->SetSizer( bSizer2 );
	this->Layout();
	bSizer2->Fit( this );

	// Connect Events
	m_ResetOptButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_PINMAP_BASE::OnResetMatrixClick ), NULL, this );
}

PANEL_SETUP_PINMAP_BASE::~PANEL_SETUP_PINMAP_BASE()
{
	// Disconnect Events
	m_ResetOptButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_PINMAP_BASE::OnResetMatrixClick ), NULL, this );

}
