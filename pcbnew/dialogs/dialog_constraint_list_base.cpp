///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_constraint_list_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_CONSTRAINT_LIST_BASE::DIALOG_CONSTRAINT_LIST_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_list = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL );
	m_list->SetMinSize( wxSize( 480,200 ) );

	bMainSizer->Add( m_list, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bButtonSizer;
	bButtonSizer = new wxBoxSizer( wxHORIZONTAL );

	m_deleteButton = new wxButton( this, wxID_DELETE, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonSizer->Add( m_deleteButton, 0, wxALL, 5 );


	bButtonSizer->Add( 0, 0, 1, wxRIGHT|wxLEFT, 5 );

	m_closeButton = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonSizer->Add( m_closeButton, 0, wxALL, 5 );


	bMainSizer->Add( bButtonSizer, 0, wxEXPAND, 0 );


	this->SetSizer( bMainSizer );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_list->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( DIALOG_CONSTRAINT_LIST_BASE::onRowActivated ), NULL, this );
	m_deleteButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONSTRAINT_LIST_BASE::onDelete ), NULL, this );
}

DIALOG_CONSTRAINT_LIST_BASE::~DIALOG_CONSTRAINT_LIST_BASE()
{
	// Disconnect Events
	m_list->Disconnect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( DIALOG_CONSTRAINT_LIST_BASE::onRowActivated ), NULL, this );
	m_deleteButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONSTRAINT_LIST_BASE::onDelete ), NULL, this );

}
