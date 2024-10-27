///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
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
	m_menuFile = new wxMenu();
	wxMenuItem* m_menuOpenLogfile;
	m_menuOpenLogfile = new wxMenuItem( m_menuFile, wxID_ANY, wxString( wxT("Open") ) , wxEmptyString, wxITEM_NORMAL );
	m_menuFile->Append( m_menuOpenLogfile );

	wxMenuItem* m_menuOpenTestcase;
	m_menuOpenTestcase = new wxMenuItem( m_menuFile, wxID_ANY, wxString( wxT("Open") ) , wxEmptyString, wxITEM_NORMAL );
	m_menuFile->Append( m_menuOpenTestcase );

	wxMenuItem* m_menuSaveTestcase;
	m_menuSaveTestcase = new wxMenuItem( m_menuFile, wxID_ANY, wxString( wxT("Save Testcase") ) , wxEmptyString, wxITEM_NORMAL );
	m_menuFile->Append( m_menuSaveTestcase );

	wxMenuItem* m_menuExit;
	m_menuExit = new wxMenuItem( m_menuFile, wxID_ANY, wxString( wxT("Exit") ) , wxEmptyString, wxITEM_NORMAL );
	m_menuFile->Append( m_menuExit );

	m_menubar1->Append( m_menuFile, wxT("File") );

	m_menuView = new wxMenu();
	wxMenuItem* m_menuShowRPIs;
	m_menuShowRPIs = new wxMenuItem( m_menuView, wxID_ANY, wxString( wxT("Show PREVIEW_ITEMS") ) , wxEmptyString, wxITEM_CHECK );
	m_menuView->Append( m_menuShowRPIs );
	m_menuShowRPIs->Check( true );

	wxMenuItem* m_menuOverrideLineWidth;
	m_menuOverrideLineWidth = new wxMenuItem( m_menuView, wxID_ANY, wxString( wxT("Override line width") ) , wxEmptyString, wxITEM_CHECK );
	m_menuView->Append( m_menuOverrideLineWidth );

	wxMenuItem* m_menuShowVertexNumbers;
	m_menuShowVertexNumbers = new wxMenuItem( m_menuView, wxID_ANY, wxString( wxT("Show Vertex Numbers") ) , wxEmptyString, wxITEM_CHECK );
	m_menuView->Append( m_menuShowVertexNumbers );

	m_menubar1->Append( m_menuView, wxT("View") );

	this->SetMenuBar( m_menubar1 );

	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	m_topBarSizer = new wxFlexGridSizer( 3, 11, 0, 0 );
	m_topBarSizer->SetFlexibleDirection( wxBOTH );
	m_topBarSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_rewindText = new wxStaticText( this, wxID_ANY, wxT("Rewind: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_rewindText->Wrap( -1 );
	m_topBarSizer->Add( m_rewindText, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_rewindLeft = new wxButton( this, wxID_ANY, wxT("<"), wxDefaultPosition, wxSize( 50,-1 ), 0 );
	m_rewindLeft->SetMaxSize( wxSize( 50,-1 ) );

	m_topBarSizer->Add( m_rewindLeft, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_rewindSlider = new wxSlider( this, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxSize( 200,-1 ), wxSL_HORIZONTAL );
	m_rewindSlider->SetMinSize( wxSize( 200,-1 ) );

	m_topBarSizer->Add( m_rewindSlider, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_rewindRight = new wxButton( this, wxID_ANY, wxT(">"), wxDefaultPosition, wxSize( 50,-1 ), 0 );
	m_rewindRight->SetMaxSize( wxSize( 50,-1 ) );

	m_topBarSizer->Add( m_rewindRight, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_rewindPos = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 50,-1 ), wxTE_PROCESS_ENTER );
	m_rewindPos->SetMaxSize( wxSize( 50,-1 ) );

	m_topBarSizer->Add( m_rewindPos, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText2 = new wxStaticText( this, wxID_ANY, wxT("Filter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	m_topBarSizer->Add( m_staticText2, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_filterString = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_topBarSizer->Add( m_filterString, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_algoStatus = new wxStaticText( this, wxID_ANY, wxT("MyLabel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_algoStatus->Wrap( -1 );
	m_algoStatus->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_topBarSizer->Add( m_algoStatus, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	m_topBarSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_ideLabel = new wxStaticText( this, wxID_ANY, wxT("IDE:"), wxPoint( -1,-1 ), wxDefaultSize, 0 );
	m_ideLabel->Wrap( -1 );
	m_ideLabel->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_topBarSizer->Add( m_ideLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxString m_ideChoiceChoices[] = { wxT("VS Code"), wxT("Visual Studio (full)"), wxT("CLion"), wxT("Emacs") };
	int m_ideChoiceNChoices = sizeof( m_ideChoiceChoices ) / sizeof( wxString );
	m_ideChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ideChoiceNChoices, m_ideChoiceChoices, 0 );
	m_ideChoice->SetSelection( 0 );
	m_ideChoice->SetToolTip( wxT("Select IDE for go to line functionality") );

	m_topBarSizer->Add( m_ideChoice, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	m_mainSizer->Add( m_topBarSizer, 0, 0, 5 );

	m_mainSplitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	m_mainSplitter->Connect( wxEVT_IDLE, wxIdleEventHandler( PNS_LOG_VIEWER_FRAME_BASE::m_mainSplitterOnIdle ), NULL, this );
	m_mainSplitter->SetMinimumPaneSize( 150 );

	m_panelProps = new wxPanel( m_mainSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );

	m_propsNotebook = new wxNotebook( m_panelProps, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelListView = new wxPanel( m_propsNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	m_itemList = new wxTreeListCtrl( m_panelListView, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTL_CHECKBOX|wxTL_DEFAULT_STYLE|wxTL_MULTIPLE );

	bSizer6->Add( m_itemList, 1, wxALL|wxEXPAND, 5 );


	m_panelListView->SetSizer( bSizer6 );
	m_panelListView->Layout();
	bSizer6->Fit( m_panelListView );
	m_propsNotebook->AddPage( m_panelListView, wxT("Geometry"), false );
	m_panelConsole = new wxPanel( m_propsNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );

	m_consoleText = new wxTextCtrl( m_panelConsole, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH );
	bSizer7->Add( m_consoleText, 1, wxALL|wxEXPAND, 5 );


	m_panelConsole->SetSizer( bSizer7 );
	m_panelConsole->Layout();
	bSizer7->Fit( m_panelConsole );
	m_propsNotebook->AddPage( m_panelConsole, wxT("Console"), true );

	bSizer5->Add( m_propsNotebook, 1, wxEXPAND | wxALL, 5 );


	m_panelProps->SetSizer( bSizer5 );
	m_panelProps->Layout();
	bSizer5->Fit( m_panelProps );
	m_mainSplitter->Initialize( m_panelProps );
	m_mainSizer->Add( m_mainSplitter, 1, wxEXPAND, 5 );


	this->SetSizer( m_mainSizer );
	this->Layout();
	m_statusBar = this->CreateStatusBar( 1, wxSTB_SIZEGRIP, wxID_ANY );

	this->Centre( wxBOTH );

	// Connect Events
	m_menuFile->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onOpen ), this, m_menuOpenLogfile->GetId());
	m_menuFile->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onOpen ), this, m_menuOpenTestcase->GetId());
	m_menuFile->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onSaveAs ), this, m_menuSaveTestcase->GetId());
	m_menuFile->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onExit ), this, m_menuExit->GetId());
	m_menuView->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onShowRPIsChecked ), this, m_menuShowRPIs->GetId());
	m_menuView->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onShowThinLinesChecked ), this, m_menuOverrideLineWidth->GetId());
	m_menuView->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( PNS_LOG_VIEWER_FRAME_BASE::onShowVerticesChecked ), this, m_menuShowVertexNumbers->GetId());
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

}
