///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_packages_view_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PACKAGES_VIEW_BASE::PANEL_PACKAGES_VIEW_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	m_splitter1 = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE );
	m_splitter1->SetSashGravity( 0.5 );
	m_splitter1->Connect( wxEVT_IDLE, wxIdleEventHandler( PANEL_PACKAGES_VIEW_BASE::m_splitter1OnIdle ), NULL, this );
	m_splitter1->SetMinimumPaneSize( 400 );

	m_panelList = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bPanelListSizer;
	bPanelListSizer = new wxBoxSizer( wxVERTICAL );

	m_searchCtrl = new wxSearchCtrl( m_panelList, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifndef __WXMAC__
	m_searchCtrl->ShowSearchButton( true );
	#endif
	m_searchCtrl->ShowCancelButton( false );
	bPanelListSizer->Add( m_searchCtrl, 0, wxEXPAND, 5 );

	m_packageListWindow = new wxScrolledWindow( m_panelList, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxALWAYS_SHOW_SB|wxBORDER_SUNKEN|wxFULL_REPAINT_ON_RESIZE|wxVSCROLL );
	m_packageListWindow->SetScrollRate( 5, 5 );
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );


	m_packageListWindow->SetSizer( bSizer2 );
	m_packageListWindow->Layout();
	bSizer2->Fit( m_packageListWindow );
	bPanelListSizer->Add( m_packageListWindow, 1, wxEXPAND, 5 );


	m_panelList->SetSizer( bPanelListSizer );
	m_panelList->Layout();
	bPanelListSizer->Fit( m_panelList );
	m_panelDetails = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bPanelDetailsSizer;
	bPanelDetailsSizer = new wxBoxSizer( wxVERTICAL );

	m_scrolledWindow5 = new wxScrolledWindow( m_panelDetails, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_scrolledWindow5->SetScrollRate( 5, 5 );
	wxBoxSizer* bSizerScrolledWindow;
	bSizerScrolledWindow = new wxBoxSizer( wxVERTICAL );

	m_infoText = new wxRichTextCtrl( m_scrolledWindow5, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_AUTO_URL );
	m_infoText->Enable( false );
	m_infoText->SetMinSize( wxSize( -1,300 ) );

	bSizerScrolledWindow->Add( m_infoText, 0, wxEXPAND, 5 );

	m_sizerVersions = new wxBoxSizer( wxVERTICAL );

	m_gridVersions = new WX_GRID( m_scrolledWindow5, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

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
	m_gridVersions->SetColLabelSize( 22 );
	m_gridVersions->SetColLabelValue( 0, _("Version") );
	m_gridVersions->SetColLabelValue( 1, _("Download Size") );
	m_gridVersions->SetColLabelValue( 2, _("Install Size") );
	m_gridVersions->SetColLabelValue( 3, _("Comp") );
	m_gridVersions->SetColLabelValue( 4, _("Status") );
	m_gridVersions->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_gridVersions->EnableDragRowSize( false );
	m_gridVersions->SetRowLabelSize( 0 );
	m_gridVersions->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_gridVersions->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
	m_sizerVersions->Add( m_gridVersions, 0, wxEXPAND|wxRIGHT, 15 );

	wxBoxSizer* bSizerVersionButtons;
	bSizerVersionButtons = new wxBoxSizer( wxHORIZONTAL );

	m_showAllVersions = new wxCheckBox( m_scrolledWindow5, wxID_ANY, _("Show all versions"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerVersionButtons->Add( m_showAllVersions, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );


	bSizerVersionButtons->Add( 0, 0, 1, wxEXPAND, 5 );

	m_buttonDownload = new wxButton( m_scrolledWindow5, wxID_ANY, _("Download"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerVersionButtons->Add( m_buttonDownload, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_buttonInstall = new wxButton( m_scrolledWindow5, wxID_ANY, _("Install"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerVersionButtons->Add( m_buttonInstall, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );


	m_sizerVersions->Add( bSizerVersionButtons, 0, wxEXPAND|wxRIGHT, 15 );


	bSizerScrolledWindow->Add( m_sizerVersions, 0, wxEXPAND|wxRIGHT|wxLEFT, 3 );


	m_scrolledWindow5->SetSizer( bSizerScrolledWindow );
	m_scrolledWindow5->Layout();
	bSizerScrolledWindow->Fit( m_scrolledWindow5 );
	bPanelDetailsSizer->Add( m_scrolledWindow5, 1, wxEXPAND, 5 );


	m_panelDetails->SetSizer( bPanelDetailsSizer );
	m_panelDetails->Layout();
	bPanelDetailsSizer->Fit( m_panelDetails );
	m_splitter1->SplitVertically( m_panelList, m_panelDetails, 0 );
	bSizer1->Add( m_splitter1, 1, wxEXPAND|wxALL, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();
	bSizer1->Fit( this );

	// Connect Events
	m_scrolledWindow5->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_PACKAGES_VIEW_BASE::OnSizeInfoBox ), NULL, this );
	m_gridVersions->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( PANEL_PACKAGES_VIEW_BASE::OnVersionsCellClicked ), NULL, this );
	m_showAllVersions->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnShowAllVersionsClicked ), NULL, this );
	m_buttonDownload->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnDownloadVersionClicked ), NULL, this );
	m_buttonInstall->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnInstallVersionClicked ), NULL, this );
}

PANEL_PACKAGES_VIEW_BASE::~PANEL_PACKAGES_VIEW_BASE()
{
	// Disconnect Events
	m_scrolledWindow5->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_PACKAGES_VIEW_BASE::OnSizeInfoBox ), NULL, this );
	m_gridVersions->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( PANEL_PACKAGES_VIEW_BASE::OnVersionsCellClicked ), NULL, this );
	m_showAllVersions->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnShowAllVersionsClicked ), NULL, this );
	m_buttonDownload->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnDownloadVersionClicked ), NULL, this );
	m_buttonInstall->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnInstallVersionClicked ), NULL, this );

}
