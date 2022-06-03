///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Feb  6 2021)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pns_log_viewer_frame_base.h"

///////////////////////////////////////////////////////////////////////////

PNS_LOG_VIEWER_FRAME_BASE::PNS_LOG_VIEWER_FRAME_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_menubar1 = new wxMenuBar( 0 );
	m_menu1 = new wxMenu();
	wxMenuItem* m_menuItem1;
	m_menuItem1 = new wxMenuItem( m_menu1, wxID_ANY, wxString( wxT("Reload") ) , wxEmptyString, wxITEM_NORMAL );
	m_menu1->Append( m_menuItem1 );

	wxMenuItem* m_menuItem2;
	m_menuItem2 = new wxMenuItem( m_menu1, wxID_ANY, wxString( wxT("Exit") ) , wxEmptyString, wxITEM_NORMAL );
	m_menu1->Append( m_menuItem2 );

	m_menubar1->Append( m_menu1, wxT("File") );

	this->SetMenuBar( m_menubar1 );

	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 3, 10, 0, 0 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_rewindText = new wxStaticText( this, wxID_ANY, wxT("Rewind: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_rewindText->Wrap( -1 );
	fgSizer3->Add( m_rewindText, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_rewindLeft = new wxButton( this, wxID_ANY, wxT("<"), wxDefaultPosition, wxSize( 50,-1 ), 0 );
	m_rewindLeft->SetMaxSize( wxSize( 50,-1 ) );

	fgSizer3->Add( m_rewindLeft, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_rewindSlider = new wxSlider( this, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxSize( 200,-1 ), wxSL_HORIZONTAL );
	m_rewindSlider->SetMinSize( wxSize( 200,-1 ) );

	fgSizer3->Add( m_rewindSlider, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_rewindRight = new wxButton( this, wxID_ANY, wxT(">"), wxDefaultPosition, wxSize( 50,-1 ), 0 );
	m_rewindRight->SetMaxSize( wxSize( 50,-1 ) );

	fgSizer3->Add( m_rewindRight, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_rewindPos = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 50,-1 ), wxTE_PROCESS_ENTER );
	m_rewindPos->SetMaxSize( wxSize( 50,-1 ) );

	fgSizer3->Add( m_rewindPos, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_staticText2 = new wxStaticText( this, wxID_ANY, wxT("Filter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizer3->Add( m_staticText2, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_filterString = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_filterString, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_chkShowRPItems = new wxCheckBox( this, wxID_ANY, wxT("Show RPIs"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_chkShowRPItems, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_chkThinLines = new wxCheckBox( this, wxID_ANY, wxT("Thin lines"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_chkThinLines, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_showVertices = new wxCheckBox( this, wxID_ANY, wxT("Show Vertices"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_showVertices, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_algoStatus = new wxStaticText( this, wxID_ANY, wxT("MyLabel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_algoStatus->Wrap( -1 );
	m_algoStatus->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	fgSizer3->Add( m_algoStatus, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	m_mainSizer->Add( fgSizer3, 0, wxEXPAND, 5 );

	m_viewSizer = new wxBoxSizer( wxVERTICAL );


	m_mainSizer->Add( m_viewSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	m_itemList = new wxTreeListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTL_CHECKBOX|wxTL_DEFAULT_STYLE|wxTL_MULTIPLE );

	bSizer6->Add( m_itemList, 1, wxALL|wxEXPAND, 5 );


	m_mainSizer->Add( bSizer6, 1, wxEXPAND, 5 );


	this->SetSizer( m_mainSizer );
	this->Layout();
	m_statusBar = this->CreateStatusBar( 1, wxSTB_SIZEGRIP, wxID_ANY );

	this->Centre( wxBOTH );

	// Connect Events
	m_menu1->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onReload ), this, m_menuItem1->GetId());
	m_menu1->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onExit ), this, m_menuItem2->GetId());
	m_rewindLeft->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onBtnRewindLeft ), NULL, this );
	m_rewindSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindScroll ), NULL, this );
	m_rewindSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindScroll ), NULL, this );
	m_rewindSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindScroll ), NULL, this );
	m_rewindSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindScroll ), NULL, this );
	m_rewindSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindScroll ), NULL, this );
	m_rewindSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindScroll ), NULL, this );
	m_rewindSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindScroll ), NULL, this );
	m_rewindSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindScroll ), NULL, this );
	m_rewindSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindScroll ), NULL, this );
	m_rewindRight->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onBtnRewindRight ), NULL, this );
	m_rewindPos->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindCountText2 ), NULL, this );
	m_rewindPos->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindCountText ), NULL, this );
	m_filterString->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onFilterText ), NULL, this );
	m_chkShowRPItems->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onShowRPIsChecked ), NULL, this );
	m_chkThinLines->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onShowThinLinesChecked ), NULL, this );
	m_showVertices->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onShowVerticesChecked ), NULL, this );
}

PNS_LOG_VIEWER_FRAME_BASE::~PNS_LOG_VIEWER_FRAME_BASE()
{
	// Disconnect Events
	m_rewindLeft->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onBtnRewindLeft ), NULL, this );
	m_rewindSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindScroll ), NULL, this );
	m_rewindSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindScroll ), NULL, this );
	m_rewindSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindScroll ), NULL, this );
	m_rewindSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindScroll ), NULL, this );
	m_rewindSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindScroll ), NULL, this );
	m_rewindSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindScroll ), NULL, this );
	m_rewindSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindScroll ), NULL, this );
	m_rewindSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindScroll ), NULL, this );
	m_rewindSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindScroll ), NULL, this );
	m_rewindRight->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onBtnRewindRight ), NULL, this );
	m_rewindPos->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindCountText2 ), NULL, this );
	m_rewindPos->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onRewindCountText ), NULL, this );
	m_filterString->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onFilterText ), NULL, this );
	m_chkShowRPItems->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onShowRPIsChecked ), NULL, this );
	m_chkThinLines->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onShowThinLinesChecked ), NULL, this );
	m_showVertices->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onShowVerticesChecked ), NULL, this );

}
