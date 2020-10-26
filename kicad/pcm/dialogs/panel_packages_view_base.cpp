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

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_searchBitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_searchBitmap, 0, wxALL, 5 );

	m_searchCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_searchCtrl, 1, wxALL|wxEXPAND, 5 );


	bSizer1->Add( bSizer7, 0, wxEXPAND, 5 );

	m_splitter1 = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	m_splitter1->SetSashGravity( 0.55 );
	m_splitter1->Connect( wxEVT_IDLE, wxIdleEventHandler( PANEL_PACKAGES_VIEW_BASE::m_splitter1OnIdle ), NULL, this );
	m_splitter1->SetMinimumPaneSize( 300 );

	m_packageListWindow = new wxScrolledWindow( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxALWAYS_SHOW_SB|wxBORDER_SUNKEN|wxFULL_REPAINT_ON_RESIZE|wxVSCROLL );
	m_packageListWindow->SetScrollRate( 5, 5 );
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );


	m_packageListWindow->SetSizer( bSizer2 );
	m_packageListWindow->Layout();
	bSizer2->Fit( m_packageListWindow );
	m_panelDetails = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	m_notebook = new wxNotebook( m_panelDetails, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelDescription = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	m_infoText = new wxRichTextCtrl( m_panelDescription, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_AUTO_URL|wxTE_READONLY|wxVSCROLL|wxHSCROLL|wxNO_BORDER|wxWANTS_CHARS );
	bSizer4->Add( m_infoText, 1, wxEXPAND, 5 );


	m_panelDescription->SetSizer( bSizer4 );
	m_panelDescription->Layout();
	bSizer4->Fit( m_panelDescription );
	m_notebook->AddPage( m_panelDescription, _("Description"), true );
	m_panelVersions = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );

	m_showAllVersions = new wxCheckBox( m_panelVersions, wxID_ANY, _("Show all versions"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_showAllVersions, 0, wxALL, 5 );

	m_gridVersions = new WX_GRID( m_panelVersions, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

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
	m_gridVersions->SetColLabelSize( 30 );
	m_gridVersions->SetColLabelValue( 0, _("Version") );
	m_gridVersions->SetColLabelValue( 1, _("Dl Size") );
	m_gridVersions->SetColLabelValue( 2, _("Inst Size") );
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
	bSizer5->Add( m_gridVersions, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );


	bSizer6->Add( 0, 0, 1, wxEXPAND, 5 );

	m_buttonDownload = new wxButton( m_panelVersions, wxID_ANY, _("Download"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( m_buttonDownload, 0, wxALL, 5 );

	m_buttonInstall = new wxButton( m_panelVersions, wxID_ANY, _("Install"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( m_buttonInstall, 0, wxALL, 5 );


	bSizer6->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizer5->Add( bSizer6, 0, wxEXPAND, 5 );


	m_panelVersions->SetSizer( bSizer5 );
	m_panelVersions->Layout();
	bSizer5->Fit( m_panelVersions );
	m_notebook->AddPage( m_panelVersions, _("Versions"), false );

	bSizer3->Add( m_notebook, 1, wxEXPAND, 5 );


	m_panelDetails->SetSizer( bSizer3 );
	m_panelDetails->Layout();
	bSizer3->Fit( m_panelDetails );
	m_splitter1->SplitVertically( m_packageListWindow, m_panelDetails, 0 );
	bSizer1->Add( m_splitter1, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();

	// Connect Events
	m_searchCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnSearchTextChanged ), NULL, this );
	m_showAllVersions->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnShowAllVersionsClicked ), NULL, this );
	m_gridVersions->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( PANEL_PACKAGES_VIEW_BASE::OnVersionsCellClicked ), NULL, this );
	m_buttonDownload->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnDownloadVersionClicked ), NULL, this );
	m_buttonInstall->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnInstallVersionClicked ), NULL, this );
}

PANEL_PACKAGES_VIEW_BASE::~PANEL_PACKAGES_VIEW_BASE()
{
	// Disconnect Events
	m_searchCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnSearchTextChanged ), NULL, this );
	m_showAllVersions->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnShowAllVersionsClicked ), NULL, this );
	m_gridVersions->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( PANEL_PACKAGES_VIEW_BASE::OnVersionsCellClicked ), NULL, this );
	m_buttonDownload->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnDownloadVersionClicked ), NULL, this );
	m_buttonInstall->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PACKAGES_VIEW_BASE::OnInstallVersionClicked ), NULL, this );

}
