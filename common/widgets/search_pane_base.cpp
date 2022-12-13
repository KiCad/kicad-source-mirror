///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "search_pane_base.h"

///////////////////////////////////////////////////////////////////////////

SEARCH_PANE_BASE::SEARCH_PANE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	this->SetMinSize( wxSize( 360,100 ) );

	m_sizerOuter = new wxBoxSizer( wxVERTICAL );

	m_searchCtrl1 = new wxSearchCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifndef __WXMAC__
	m_searchCtrl1->ShowSearchButton( true );
	#endif
	m_searchCtrl1->ShowCancelButton( false );
	m_sizerOuter->Add( m_searchCtrl1, 0, wxEXPAND|wxTOP, 5 );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	m_sizerOuter->Add( m_notebook, 1, wxEXPAND|wxTOP, 5 );


	this->SetSizer( m_sizerOuter );
	this->Layout();

	// Connect Events
	this->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( SEARCH_PANE_BASE::OnSetFocus ) );
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( SEARCH_PANE_BASE::OnSize ) );
	m_searchCtrl1->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( SEARCH_PANE_BASE::OnSearchTextEntry ), NULL, this );
	m_notebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( SEARCH_PANE_BASE::OnNotebookPageChanged ), NULL, this );
	m_notebook->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( SEARCH_PANE_BASE::OnSetFocus ), NULL, this );
}

SEARCH_PANE_BASE::~SEARCH_PANE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( SEARCH_PANE_BASE::OnSetFocus ) );
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( SEARCH_PANE_BASE::OnSize ) );
	m_searchCtrl1->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( SEARCH_PANE_BASE::OnSearchTextEntry ), NULL, this );
	m_notebook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( SEARCH_PANE_BASE::OnNotebookPageChanged ), NULL, this );
	m_notebook->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( SEARCH_PANE_BASE::OnSetFocus ), NULL, this );

}
