///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Feb  6 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/two_column_tree_list.h"

#include "dialog_choose_component_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_CHOOSE_COMPONENT_BASE::DIALOG_CHOOSE_COMPONENT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	m_splitter1 = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE );
	m_splitter1->SetSashGravity( 0.9 );
	m_splitter1->Connect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::m_splitter1OnIdle ), NULL, this );
	m_splitter1->SetMinimumPaneSize( 1 );
	
	m_panel3 = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );
	
	m_searchBox = new wxSearchCtrl( m_panel3, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifndef __WXMAC__
	m_searchBox->ShowSearchButton( true );
	#endif
	m_searchBox->ShowCancelButton( false );
	bSizer10->Add( m_searchBox, 0, wxALL|wxEXPAND, 5 );
	
	m_libraryComponentTree = new TWO_COLUMN_TREE_LIST( m_panel3, wxID_ANY, wxDefaultPosition, wxSize( 320,240 ), wxTL_DEFAULT_STYLE );
	
	bSizer10->Add( m_libraryComponentTree, 1, wxEXPAND | wxALL, 5 );
	
	m_componentDetails = new wxHtmlWindow( m_panel3, wxID_ANY, wxDefaultPosition, wxSize( 320,240 ), wxHW_SCROLLBAR_AUTO|wxSUNKEN_BORDER );
	bSizer10->Add( m_componentDetails, 1, wxALL|wxEXPAND, 5 );
	
	
	m_panel3->SetSizer( bSizer10 );
	m_panel3->Layout();
	bSizer10->Fit( m_panel3 );
	m_panel4 = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );
	
	m_componentView = new wxPanel( m_panel4, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxFULL_REPAINT_ON_RESIZE|wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
	bSizer12->Add( m_componentView, 1, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxHORIZONTAL );
	
	wxArrayString m_chooseFootprintChoices;
	m_chooseFootprint = new wxChoice( m_panel4, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_chooseFootprintChoices, 0 );
	m_chooseFootprint->SetSelection( 0 );
	bSizer14->Add( m_chooseFootprint, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizer13->Add( bSizer14, 0, wxEXPAND, 5 );
	
	m_footprintView = new wxPanel( m_panel4, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxFULL_REPAINT_ON_RESIZE|wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
	bSizer13->Add( m_footprintView, 1, wxEXPAND | wxALL, 5 );
	
	
	bSizer12->Add( bSizer13, 1, wxEXPAND, 5 );
	
	
	bSizer11->Add( bSizer12, 1, wxEXPAND, 5 );
	
	
	m_panel4->SetSizer( bSizer11 );
	m_panel4->Layout();
	bSizer11->Fit( m_panel4 );
	m_splitter1->SplitVertically( m_panel3, m_panel4, -300 );
	bSizerMain->Add( m_splitter1, 1, wxALL|wxEXPAND, 5 );
	
	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();
	
	bSizerMain->Add( m_stdButtons, 0, wxBOTTOM|wxEXPAND, 10 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnInitDialog ) );
	m_searchBox->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnInterceptSearchBoxKey ), NULL, this );
	m_searchBox->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnSearchBoxChange ), NULL, this );
	m_searchBox->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnSearchBoxEnter ), NULL, this );
	m_libraryComponentTree->Connect( wxEVT_KEY_UP, wxKeyEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnInterceptTreeEnter ), NULL, this );
	m_libraryComponentTree->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnTreeMouseUp ), NULL, this );
	m_libraryComponentTree->Connect( wxEVT_TREELIST_ITEM_ACTIVATED, wxTreeListEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnDoubleClickTreeActivation ), NULL, this );
	m_libraryComponentTree->Connect( wxEVT_TREELIST_SELECTION_CHANGED, wxTreeListEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnTreeSelect ), NULL, this );
	m_componentDetails->Connect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnDatasheetClick ), NULL, this );
	m_componentView->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnStartComponentBrowser ), NULL, this );
	m_componentView->Connect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnHandlePreviewRepaint ), NULL, this );
}

DIALOG_CHOOSE_COMPONENT_BASE::~DIALOG_CHOOSE_COMPONENT_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnInitDialog ) );
	m_searchBox->Disconnect( wxEVT_KEY_DOWN, wxKeyEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnInterceptSearchBoxKey ), NULL, this );
	m_searchBox->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnSearchBoxChange ), NULL, this );
	m_searchBox->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnSearchBoxEnter ), NULL, this );
	m_libraryComponentTree->Disconnect( wxEVT_KEY_UP, wxKeyEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnInterceptTreeEnter ), NULL, this );
	m_libraryComponentTree->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnTreeMouseUp ), NULL, this );
	m_libraryComponentTree->Disconnect( wxEVT_TREELIST_ITEM_ACTIVATED, wxTreeListEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnDoubleClickTreeActivation ), NULL, this );
	m_libraryComponentTree->Disconnect( wxEVT_TREELIST_SELECTION_CHANGED, wxTreeListEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnTreeSelect ), NULL, this );
	m_componentDetails->Disconnect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnDatasheetClick ), NULL, this );
	m_componentView->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnStartComponentBrowser ), NULL, this );
	m_componentView->Disconnect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_CHOOSE_COMPONENT_BASE::OnHandlePreviewRepaint ), NULL, this );
	
}
