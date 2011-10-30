///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  6 2011)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "sweet_editor_panel.h"

///////////////////////////////////////////////////////////////////////////

SWEET_EDITOR_PANEL::SWEET_EDITOR_PANEL( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );
	
	m_top_bottom = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	m_top_bottom->SetSashGravity( 1 );
	m_top_bottom->Connect( wxEVT_IDLE, wxIdleEventHandler( SWEET_EDITOR_PANEL::m_top_bottomOnIdle ), NULL, this );
	
	m_panel9 = new wxPanel( m_top_bottom, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );
	
	m_splitter3 = new wxSplitterWindow( m_panel9, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	m_splitter3->Connect( wxEVT_IDLE, wxIdleEventHandler( SWEET_EDITOR_PANEL::m_splitter3OnIdle ), NULL, this );
	
	m_scrolledTextWindow = new wxScrolledWindow( m_splitter3, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_scrolledTextWindow->SetScrollRate( 5, 5 );
	wxStaticBoxSizer* m_scrolledTextSizer;
	m_scrolledTextSizer = new wxStaticBoxSizer( new wxStaticBox( m_scrolledTextWindow, wxID_ANY, _("Sweet") ), wxVERTICAL );
	
	m_sweet_scroll_window = new wxTextCtrl( m_scrolledTextWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxALWAYS_SHOW_SB );
	m_scrolledTextSizer->Add( m_sweet_scroll_window, 1, wxALL|wxEXPAND, 5 );
	
	m_scrolledTextWindow->SetSizer( m_scrolledTextSizer );
	m_scrolledTextWindow->Layout();
	m_scrolledTextSizer->Fit( m_scrolledTextWindow );
	m_gal_scrolled_window = new wxScrolledWindow( m_splitter3, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_gal_scrolled_window->SetScrollRate( 5, 5 );
	wxStaticBoxSizer* m_gal_sizer;
	m_gal_sizer = new wxStaticBoxSizer( new wxStaticBox( m_gal_scrolled_window, wxID_ANY, _("Visual Part") ), wxVERTICAL );
	
	m_gal = new SCH::CANVAS( m_gal_scrolled_window );
	m_gal_sizer->Add( m_gal, 0, wxALL, 5 );
	
	m_gal_scrolled_window->SetSizer( m_gal_sizer );
	m_gal_scrolled_window->Layout();
	m_gal_sizer->Fit( m_gal_scrolled_window );
	m_splitter3->SplitVertically( m_scrolledTextWindow, m_gal_scrolled_window, 0 );
	bSizer10->Add( m_splitter3, 1, wxEXPAND, 5 );
	
	m_panel9->SetSizer( bSizer10 );
	m_panel9->Layout();
	bSizer10->Fit( m_panel9 );
	m_scrolledWindow3 = new wxScrolledWindow( m_top_bottom, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_scrolledWindow3->SetScrollRate( 5, 5 );
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( m_scrolledWindow3, wxID_ANY, _("Parsing Errors") ), wxVERTICAL );
	
	m_htmlWin2 = new wxHtmlWindow( m_scrolledWindow3, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	sbSizer1->Add( m_htmlWin2, 1, wxALL|wxEXPAND, 5 );
	
	m_scrolledWindow3->SetSizer( sbSizer1 );
	m_scrolledWindow3->Layout();
	sbSizer1->Fit( m_scrolledWindow3 );
	m_top_bottom->SplitHorizontally( m_panel9, m_scrolledWindow3, 0 );
	bSizer2->Add( m_top_bottom, 1, wxEXPAND, 5 );
	
	this->SetSizer( bSizer2 );
	this->Layout();
}

SWEET_EDITOR_PANEL::~SWEET_EDITOR_PANEL()
{
}
