///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "panel_setup_net_chains_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_NET_CHAINS_BASE::PANEL_SETUP_NET_CHAINS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* outerSizer;
	outerSizer = new wxBoxSizer( wxVERTICAL );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_chainsTab = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bChainsTab;
	bChainsTab = new wxBoxSizer( wxVERTICAL );

	m_chainsHeader = new wxStaticText( m_chainsTab, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_chainsHeader->Wrap( -1 );
	bChainsTab->Add( m_chainsHeader, 0, wxLEFT|wxTOP, 8 );


	bChainsTab->Add( 0, 3, 0, wxEXPAND, 5 );

	m_chainsSplitter = new wxSplitterWindow( m_chainsTab, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE );
	m_chainsSplitter->SetSashGravity( 0.7 );
	m_chainsSplitter->Connect( wxEVT_IDLE, wxIdleEventHandler( PANEL_SETUP_NET_CHAINS_BASE::m_chainsSplitterOnIdle ), NULL, this );
	m_chainsSplitter->SetMinimumPaneSize( 80 );

	m_chainsGridPanel = new wxPanel( m_chainsSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bGridSizer;
	bGridSizer = new wxBoxSizer( wxVERTICAL );

	m_chainsGrid = new WX_GRID( m_chainsGridPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_chainsGrid->CreateGrid( 0, 5 );
	m_chainsGrid->EnableEditing( true );
	m_chainsGrid->EnableGridLines( true );
	m_chainsGrid->EnableDragGridSize( true );
	m_chainsGrid->SetMargins( 0, 0 );

	// Columns
	m_chainsGrid->SetColSize( 0, 160 );
	m_chainsGrid->SetColSize( 1, 200 );
	m_chainsGrid->SetColSize( 2, 140 );
	m_chainsGrid->SetColSize( 3, 140 );
	m_chainsGrid->SetColSize( 4, 60 );
	m_chainsGrid->EnableDragColMove( false );
	m_chainsGrid->EnableDragColSize( true );
	m_chainsGrid->SetColLabelValue( 0, _("Name") );
	m_chainsGrid->SetColLabelValue( 1, _("Members") );
	m_chainsGrid->SetColLabelValue( 2, _("Net Chain Class") );
	m_chainsGrid->SetColLabelValue( 3, _("Net Class") );
	m_chainsGrid->SetColLabelValue( 4, _("Colour") );
	m_chainsGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_chainsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_chainsGrid->EnableDragRowSize( true );
	m_chainsGrid->SetRowLabelSize( 0 );
	m_chainsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_chainsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_chainsGrid->SetMinSize( wxSize( -1,160 ) );

	bGridSizer->Add( m_chainsGrid, 1, wxEXPAND, 0 );


	m_chainsGridPanel->SetSizer( bGridSizer );
	m_chainsGridPanel->Layout();
	bGridSizer->Fit( m_chainsGridPanel );
	m_membersPanel = new wxPanel( m_chainsSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bMembersSizer;
	bMembersSizer = new wxBoxSizer( wxVERTICAL );

	m_membersLabel = new wxStaticText( m_membersPanel, wxID_ANY, _("Member Nets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_membersLabel->Wrap( -1 );
	bMembersSizer->Add( m_membersLabel, 0, wxLEFT|wxTOP, 5 );

	m_membersListBox = new wxListBox( m_membersPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE );
	bMembersSizer->Add( m_membersListBox, 1, wxALL|wxEXPAND, 5 );


	m_membersPanel->SetSizer( bMembersSizer );
	m_membersPanel->Layout();
	bMembersSizer->Fit( m_membersPanel );
	m_chainsSplitter->SplitHorizontally( m_chainsGridPanel, m_membersPanel, -120 );
	bChainsTab->Add( m_chainsSplitter, 1, wxEXPAND, 5 );

	wxBoxSizer* bChainsButtons;
	bChainsButtons = new wxBoxSizer( wxHORIZONTAL );

	m_deleteChainButton = new STD_BITMAP_BUTTON( m_chainsTab, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_deleteChainButton->SetToolTip( _("Delete the selected committed net chain") );

	bChainsButtons->Add( m_deleteChainButton, 0, wxBOTTOM|wxRIGHT, 5 );


	bChainsTab->Add( bChainsButtons, 0, wxEXPAND|wxTOP, 3 );


	m_chainsTab->SetSizer( bChainsTab );
	m_chainsTab->Layout();
	bChainsTab->Fit( m_chainsTab );
	m_notebook->AddPage( m_chainsTab, _("Chains"), true );
	m_classesTab = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bClassesTab;
	bClassesTab = new wxBoxSizer( wxVERTICAL );

	m_classesHeader = new wxStaticText( m_classesTab, wxID_ANY, _("Group net chains under a class label so DRC rules can target the whole group via inNetChainClass('name')."), wxDefaultPosition, wxDefaultSize, 0 );
	m_classesHeader->Wrap( 600 );
	bClassesTab->Add( m_classesHeader, 0, wxLEFT|wxTOP, 8 );


	bClassesTab->Add( 0, 3, 0, wxEXPAND, 5 );

	m_classesGrid = new WX_GRID( m_classesTab, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_classesGrid->CreateGrid( 0, 2 );
	m_classesGrid->EnableEditing( true );
	m_classesGrid->EnableGridLines( true );
	m_classesGrid->EnableDragGridSize( false );
	m_classesGrid->SetMargins( 0, 0 );

	// Columns
	m_classesGrid->SetColSize( 0, 220 );
	m_classesGrid->SetColSize( 1, 80 );
	m_classesGrid->EnableDragColMove( false );
	m_classesGrid->EnableDragColSize( true );
	m_classesGrid->SetColLabelValue( 0, _("Class Name") );
	m_classesGrid->SetColLabelValue( 1, _("Members") );
	m_classesGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_classesGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_classesGrid->EnableDragRowSize( true );
	m_classesGrid->SetRowLabelSize( 0 );
	m_classesGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_classesGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_classesGrid->SetMinSize( wxSize( -1,200 ) );

	bClassesTab->Add( m_classesGrid, 1, wxEXPAND|wxLEFT|wxRIGHT, 8 );

	wxBoxSizer* bClassesButtons;
	bClassesButtons = new wxBoxSizer( wxHORIZONTAL );

	m_addClassButton = new STD_BITMAP_BUTTON( m_classesTab, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_addClassButton->SetToolTip( _("Add a new net chain class") );

	bClassesButtons->Add( m_addClassButton, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_renameClassButton = new STD_BITMAP_BUTTON( m_classesTab, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_renameClassButton->SetToolTip( _("Rename the selected class (updates every chain that uses it)") );

	bClassesButtons->Add( m_renameClassButton, 0, wxBOTTOM|wxRIGHT, 5 );

	m_deleteClassButton = new STD_BITMAP_BUTTON( m_classesTab, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_deleteClassButton->SetToolTip( _("Delete the selected class (chains revert to no class)") );

	bClassesButtons->Add( m_deleteClassButton, 0, wxBOTTOM|wxRIGHT, 5 );


	bClassesTab->Add( bClassesButtons, 0, wxEXPAND|wxTOP, 3 );


	m_classesTab->SetSizer( bClassesTab );
	m_classesTab->Layout();
	bClassesTab->Fit( m_classesTab );
	m_notebook->AddPage( m_classesTab, _("Net Chain Classes"), false );

	outerSizer->Add( m_notebook, 1, wxEXPAND|wxALL, 5 );


	this->SetSizer( outerSizer );
	this->Layout();
	outerSizer->Fit( this );

	// Connect Events
	m_chainsGrid->Connect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( PANEL_SETUP_NET_CHAINS_BASE::OnChainGridSelectionChanged ), NULL, this );
	m_deleteChainButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NET_CHAINS_BASE::OnDeleteChainClicked ), NULL, this );
	m_addClassButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NET_CHAINS_BASE::OnClassAddClicked ), NULL, this );
	m_renameClassButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NET_CHAINS_BASE::OnClassRenameClicked ), NULL, this );
	m_deleteClassButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NET_CHAINS_BASE::OnClassDeleteClicked ), NULL, this );
}

PANEL_SETUP_NET_CHAINS_BASE::~PANEL_SETUP_NET_CHAINS_BASE()
{
	// Disconnect Events
	m_chainsGrid->Disconnect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( PANEL_SETUP_NET_CHAINS_BASE::OnChainGridSelectionChanged ), NULL, this );
	m_deleteChainButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NET_CHAINS_BASE::OnDeleteChainClicked ), NULL, this );
	m_addClassButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NET_CHAINS_BASE::OnClassAddClicked ), NULL, this );
	m_renameClassButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NET_CHAINS_BASE::OnClassRenameClicked ), NULL, this );
	m_deleteClassButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NET_CHAINS_BASE::OnClassDeleteClicked ), NULL, this );

}
