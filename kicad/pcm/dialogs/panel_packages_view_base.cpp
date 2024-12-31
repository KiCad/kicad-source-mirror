///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/html_window.h"
#include "widgets/wx_panel.h"

#include "panel_packages_view_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PACKAGES_VIEW_BASE::PANEL_PACKAGES_VIEW_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	m_splitter1 = new WX_SPLITTER_WINDOW( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE );
	m_splitter1->SetSashGravity( 0.25 );
	m_splitter1->Connect( wxEVT_IDLE, wxIdleEventHandler( PANEL_PACKAGES_VIEW_BASE::m_splitter1OnIdle ), NULL, this );

	m_panelList = new WX_PANEL( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE|wxTAB_TRAVERSAL );
	wxBoxSizer* bPanelListSizer;
	bPanelListSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );

	m_searchCtrl = new wxSearchCtrl( m_panelList, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifndef __WXMAC__
	m_searchCtrl->ShowSearchButton( true );
	#endif
	m_searchCtrl->ShowCancelButton( false );
	bSizer8->Add( m_searchCtrl, 1, wxEXPAND|wxALL, 5 );

	m_buttonUpdateAll = new wxButton( m_panelList, wxID_ANY, _("Update All"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_buttonUpdateAll, 0, wxBOTTOM|wxRIGHT|wxTOP, 5 );


	bPanelListSizer->Add( bSizer8, 0, wxEXPAND, 5 );

	m_packageListWindow = new wxScrolledWindow( m_panelList, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE|wxFULL_REPAINT_ON_RESIZE|wxVSCROLL );
	m_packageListWindow->SetScrollRate( 5, 5 );
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );


	m_packageListWindow->SetSizer( bSizer2 );
	m_packageListWindow->Layout();
	bSizer2->Fit( m_packageListWindow );
	bPanelListSizer->Add( m_packageListWindow, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_panelList->SetSizer( bPanelListSizer );
	m_panelList->Layout();
	bPanelListSizer->Fit( m_panelList );
	m_panelDetails = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bPanelDetailsSizer;
	bPanelDetailsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_infoScrollWindow = new wxScrolledWindow( m_panelDetails, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_infoScrollWindow->SetScrollRate( 5, 5 );
	m_infoScrollWindow->SetMinSize( wxSize( 480,-1 ) );

	wxBoxSizer* bSizerScrolledWindow;
	bSizerScrolledWindow = new wxBoxSizer( wxVERTICAL );

	m_infoText = new HTML_WINDOW( m_infoScrollWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_NEVER );
	m_infoText->SetMinSize( wxSize( -1,100 ) );

	bSizerScrolledWindow->Add( m_infoText, 0, wxEXPAND, 5 );

	m_sizerVersions = new wxBoxSizer( wxVERTICAL );

	m_gridVersions = new WX_GRID( m_infoScrollWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_gridVersions->CreateGrid( 0, 5 );
	m_gridVersions->EnableEditing( false );
	m_gridVersions->EnableGridLines( true );
	m_gridVersions->EnableDragGridSize( false );
	m_gridVersions->SetMargins( 0, 0 );

	// Columns
	m_gridVersions->AutoSizeColumns();
	m_gridVersions->EnableDragColMove( false );
	m_gridVersions->EnableDragColSize( true );
	m_gridVersions->SetColLabelValue( 0, _("Version") );
	m_gridVersions->SetColLabelValue( 1, _("Download Size") );
	m_gridVersions->SetColLabelValue( 2, _("Install Size") );
	m_gridVersions->SetColLabelValue( 3, _("Compatible") );
	m_gridVersions->SetColLabelValue( 4, _("Status") );
	m_gridVersions->SetColLabelSize( 22 );
	m_gridVersions->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_gridVersions->EnableDragRowSize( false );
	m_gridVersions->SetRowLabelSize( 0 );
	m_gridVersions->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_gridVersions->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
	m_sizerVersions->Add( m_gridVersions, 0, wxEXPAND|wxRIGHT, 5 );

	wxBoxSizer* bSizerVersionButtons;
	bSizerVersionButtons = new wxBoxSizer( wxHORIZONTAL );

	m_showAllVersions = new wxCheckBox( m_infoScrollWindow, wxID_ANY, _("Show all versions"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerVersionButtons->Add( m_showAllVersions, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );


	bSizerVersionButtons->Add( 0, 0, 1, wxEXPAND, 5 );

	m_buttonDownload = new wxButton( m_infoScrollWindow, wxID_ANY, _("Download"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerVersionButtons->Add( m_buttonDownload, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_buttonAction = new wxButton( m_infoScrollWindow, wxID_ANY, _("Install"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerVersionButtons->Add( m_buttonAction, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );


	m_sizerVersions->Add( bSizerVersionButtons, 0, wxEXPAND|wxRIGHT, 5 );


	bSizerScrolledWindow->Add( m_sizerVersions, 0, wxEXPAND|wxRIGHT|wxLEFT, 3 );


	m_infoScrollWindow->SetSizer( bSizerScrolledWindow );
	m_infoScrollWindow->Layout();
	bSizerScrolledWindow->Fit( m_infoScrollWindow );
	bPanelDetailsSizer->Add( m_infoScrollWindow, 1, wxEXPAND, 5 );


	m_panelDetails->SetSizer( bPanelDetailsSizer );
	m_panelDetails->Layout();
	bPanelDetailsSizer->Fit( m_panelDetails );
	m_splitter1->SplitVertically( m_panelList, m_panelDetails, 0 );
	bSizer1->Add( m_splitter1, 1, wxEXPAND|wxALL, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();
	bSizer1->Fit( this );

	// Connect Events
	m_buttonUpdateAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnUpdateAllClicked ), NULL, this );
	m_infoScrollWindow->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_PACKAGES_VIEW_BASE::OnSizeInfoBox ), NULL, this );
	m_infoText->Connect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( PANEL_PACKAGES_VIEW_BASE::OnURLClicked ), NULL, this );
	m_infoText->Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PACKAGES_VIEW_BASE::OnInfoMouseWheel ), NULL, this );
	m_gridVersions->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( PANEL_PACKAGES_VIEW_BASE::OnVersionsCellClicked ), NULL, this );
	m_showAllVersions->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnShowAllVersionsClicked ), NULL, this );
	m_buttonDownload->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnDownloadVersionClicked ), NULL, this );
	m_buttonAction->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnVersionActionClicked ), NULL, this );
}

PANEL_PACKAGES_VIEW_BASE::~PANEL_PACKAGES_VIEW_BASE()
{
	// Disconnect Events
	m_buttonUpdateAll->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnUpdateAllClicked ), NULL, this );
	m_infoScrollWindow->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_PACKAGES_VIEW_BASE::OnSizeInfoBox ), NULL, this );
	m_infoText->Disconnect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( PANEL_PACKAGES_VIEW_BASE::OnURLClicked ), NULL, this );
	m_infoText->Disconnect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( PANEL_PACKAGES_VIEW_BASE::OnInfoMouseWheel ), NULL, this );
	m_gridVersions->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( PANEL_PACKAGES_VIEW_BASE::OnVersionsCellClicked ), NULL, this );
	m_showAllVersions->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnShowAllVersionsClicked ), NULL, this );
	m_buttonDownload->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnDownloadVersionClicked ), NULL, this );
	m_buttonAction->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnVersionActionClicked ), NULL, this );

}
