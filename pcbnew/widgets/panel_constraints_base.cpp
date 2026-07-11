///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_constraints_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_CONSTRAINTS_BASE::PANEL_CONSTRAINTS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_list = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL );
	bMainSizer->Add( m_list, 1, wxEXPAND|wxALL, 3 );

	wxBoxSizer* bButtonSizer;
	bButtonSizer = new wxBoxSizer( wxHORIZONTAL );

	m_refreshButton = new wxButton( this, wxID_REFRESH, _("Refresh"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonSizer->Add( m_refreshButton, 0, wxALL, 3 );

	m_deleteButton = new wxButton( this, wxID_DELETE, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonSizer->Add( m_deleteButton, 0, wxALL, 3 );


	bMainSizer->Add( bButtonSizer, 0, wxEXPAND, 0 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_list->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( PANEL_CONSTRAINTS_BASE::onRowActivated ), NULL, this );
	m_refreshButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_CONSTRAINTS_BASE::onRefresh ), NULL, this );
	m_deleteButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_CONSTRAINTS_BASE::onDelete ), NULL, this );
}

PANEL_CONSTRAINTS_BASE::~PANEL_CONSTRAINTS_BASE()
{
	// Disconnect Events
	m_list->Disconnect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( PANEL_CONSTRAINTS_BASE::onRowActivated ), NULL, this );
	m_refreshButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_CONSTRAINTS_BASE::onRefresh ), NULL, this );
	m_deleteButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_CONSTRAINTS_BASE::onDelete ), NULL, this );

}
