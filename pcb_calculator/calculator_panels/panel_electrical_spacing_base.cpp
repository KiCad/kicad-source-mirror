///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_electrical_spacing_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_ELECTRICAL_SPACING_BASE::PANEL_ELECTRICAL_SPACING_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	m_notebook1 = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_IPC2221 = new PANEL_ELECTRICAL_SPACING_IPC2221( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_notebook1->AddPage( m_IPC2221, _("IPC 2221"), false );
	m_IEC60664 = new PANEL_ELECTRICAL_SPACING_IEC60664( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_notebook1->AddPage( m_IEC60664, _("IEC 60664"), false );

	bSizer4->Add( m_notebook1, 1, wxALL|wxEXPAND, 0 );


	this->SetSizer( bSizer4 );
	this->Layout();
}

PANEL_ELECTRICAL_SPACING_BASE::~PANEL_ELECTRICAL_SPACING_BASE()
{
}
