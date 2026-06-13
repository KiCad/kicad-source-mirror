///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_kicad_diff_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_KICAD_DIFF_BASE::DIALOG_KICAD_DIFF_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 800,600 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgPathsSizer;
	fgPathsSizer = new wxFlexGridSizer( 2, 2, 3, 5 );
	fgPathsSizer->AddGrowableCol( 1 );
	fgPathsSizer->SetFlexibleDirection( wxBOTH );
	fgPathsSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_labelReference = new wxStaticText( this, wxID_ANY, _("Reference:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelReference->Wrap( -1 );
	fgPathsSizer->Add( m_labelReference, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_pathReference = new wxStaticText( this, wxID_ANY, _("(none)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pathReference->Wrap( -1 );
	fgPathsSizer->Add( m_pathReference, 1, wxEXPAND|wxALL, 5 );

	m_labelComparison = new wxStaticText( this, wxID_ANY, _("Comparison:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelComparison->Wrap( -1 );
	fgPathsSizer->Add( m_labelComparison, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_pathComparison = new wxStaticText( this, wxID_ANY, _("(none)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pathComparison->Wrap( -1 );
	fgPathsSizer->Add( m_pathComparison, 1, wxEXPAND|wxALL, 5 );


	bMainSizer->Add( fgPathsSizer, 0, wxEXPAND|wxALL, 5 );

	m_separator = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_separator, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_splitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE );
	m_splitter->SetSashGravity( 0.3 );
	m_splitter->Connect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_KICAD_DIFF_BASE::m_splitterOnIdle ), NULL, this );

	m_panelTree = new wxPanel( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bTreeSizer;
	bTreeSizer = new wxBoxSizer( wxVERTICAL );

	m_treeChanges = new wxTreeCtrl( m_panelTree, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE|wxTR_HIDE_ROOT|wxTR_HAS_BUTTONS|wxTR_LINES_AT_ROOT );
	m_treeChanges->SetMinSize( wxSize( 220,300 ) );

	bTreeSizer->Add( m_treeChanges, 1, wxEXPAND, 0 );


	m_panelTree->SetSizer( bTreeSizer );
	m_panelTree->Layout();
	bTreeSizer->Fit( m_panelTree );
	m_panelDetail = new wxPanel( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bDetailSizer;
	bDetailSizer = new wxBoxSizer( wxVERTICAL );

	m_labelSummary = new wxStaticText( m_panelDetail, wxID_ANY, _("Select a change in the tree to view details"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelSummary->Wrap( -1 );
	bDetailSizer->Add( m_labelSummary, 0, wxALL|wxEXPAND, 5 );

	m_listProperties = new wxListCtrl( m_panelDetail, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_HRULES|wxLC_VRULES );
	m_listProperties->SetMinSize( wxSize( 400,200 ) );

	bDetailSizer->Add( m_listProperties, 1, wxALL|wxEXPAND, 5 );


	m_panelDetail->SetSizer( bDetailSizer );
	m_panelDetail->Layout();
	bDetailSizer->Fit( m_panelDetail );
	m_splitter->SplitVertically( m_panelTree, m_panelDetail, 280 );
	bMainSizer->Add( m_splitter, 1, wxEXPAND|wxALL, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_KICAD_DIFF_BASE::OnClose ) );
	m_treeChanges->Connect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( DIALOG_KICAD_DIFF_BASE::OnTreeSelectionChanged ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_KICAD_DIFF_BASE::OnOK ), NULL, this );
}

DIALOG_KICAD_DIFF_BASE::~DIALOG_KICAD_DIFF_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_KICAD_DIFF_BASE::OnClose ) );
	m_treeChanges->Disconnect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( DIALOG_KICAD_DIFF_BASE::OnTreeSelectionChanged ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_KICAD_DIFF_BASE::OnOK ), NULL, this );

}
