///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_setup_pinmap_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_PINMAP_BASE::PANEL_SETUP_PINMAP_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* m_panelMatrixSizer;
	m_panelMatrixSizer = new wxBoxSizer( wxVERTICAL );

	m_matrixPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_matrixPanel->SetMinSize( wxSize( 500,424 ) );

	m_panelMatrixSizer->Add( m_matrixPanel, 1, wxEXPAND | wxALL, 5 );


	bSizer2->Add( m_panelMatrixSizer, 1, wxEXPAND|wxTOP, 5 );


	this->SetSizer( bSizer2 );
	this->Layout();
	bSizer2->Fit( this );
}

PANEL_SETUP_PINMAP_BASE::~PANEL_SETUP_PINMAP_BASE()
{
}
