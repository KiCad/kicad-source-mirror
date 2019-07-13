///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 10 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "panel_setup_netclasses_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_NETCLASSES_BASE::PANEL_SETUP_NETCLASSES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bpanelNetClassesSizer;
	bpanelNetClassesSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	m_netclassesPane = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxStaticBoxSizer* sbSizerUpper;
	sbSizerUpper = new wxStaticBoxSizer( new wxStaticBox( m_netclassesPane, wxID_ANY, _("Net Classes") ), wxVERTICAL );

	sbSizerUpper->SetMinSize( wxSize( -1,220 ) );
	m_netclassGrid = new WX_GRID( m_netclassesPane, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_DEFAULT|wxHSCROLL|wxTAB_TRAVERSAL|wxVSCROLL );

	// Grid
	m_netclassGrid->CreateGrid( 1, 9 );
	m_netclassGrid->EnableEditing( true );
	m_netclassGrid->EnableGridLines( true );
	m_netclassGrid->EnableDragGridSize( false );
	m_netclassGrid->SetMargins( 0, 0 );

	// Columns
	m_netclassGrid->SetColSize( 0, 130 );
	m_netclassGrid->SetColSize( 1, 96 );
	m_netclassGrid->SetColSize( 2, 96 );
	m_netclassGrid->SetColSize( 3, 96 );
	m_netclassGrid->SetColSize( 4, 96 );
	m_netclassGrid->SetColSize( 5, 96 );
	m_netclassGrid->SetColSize( 6, 96 );
	m_netclassGrid->SetColSize( 7, 96 );
	m_netclassGrid->SetColSize( 8, 96 );
	m_netclassGrid->EnableDragColMove( false );
	m_netclassGrid->EnableDragColSize( true );
	m_netclassGrid->SetColLabelSize( 24 );
	m_netclassGrid->SetColLabelValue( 0, _("Name") );
	m_netclassGrid->SetColLabelValue( 1, _("Clearance") );
	m_netclassGrid->SetColLabelValue( 2, _("Track Width") );
	m_netclassGrid->SetColLabelValue( 3, _("Via Size") );
	m_netclassGrid->SetColLabelValue( 4, _("Via Drill") );
	m_netclassGrid->SetColLabelValue( 5, _("uVia Size") );
	m_netclassGrid->SetColLabelValue( 6, _("uVia Drill") );
	m_netclassGrid->SetColLabelValue( 7, _("dPair Width") );
	m_netclassGrid->SetColLabelValue( 8, _("dPair Gap") );
	m_netclassGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_netclassGrid->EnableDragRowSize( true );
	m_netclassGrid->SetRowLabelSize( 0 );
	m_netclassGrid->SetRowLabelValue( 0, _("Default") );
	m_netclassGrid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance
	m_netclassGrid->SetLabelFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	// Cell Defaults
	m_netclassGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_netclassGrid->SetToolTip( _("Net Class parameters") );

	sbSizerUpper->Add( m_netclassGrid, 1, wxEXPAND, 5 );

	wxBoxSizer* buttonBoxSizer;
	buttonBoxSizer = new wxBoxSizer( wxHORIZONTAL );

	m_addButton = new wxBitmapButton( m_netclassesPane, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_addButton->SetMinSize( wxSize( 30,29 ) );

	buttonBoxSizer->Add( m_addButton, 0, wxRIGHT, 5 );


	buttonBoxSizer->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_removeButton = new wxBitmapButton( m_netclassesPane, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_removeButton->SetMinSize( wxSize( 30,29 ) );

	buttonBoxSizer->Add( m_removeButton, 0, wxRIGHT|wxLEFT, 5 );


	sbSizerUpper->Add( buttonBoxSizer, 0, wxTOP|wxEXPAND, 2 );


	m_netclassesPane->SetSizer( sbSizerUpper );
	m_netclassesPane->Layout();
	sbSizerUpper->Fit( m_netclassesPane );
	bMargins->Add( m_netclassesPane, 1, wxALL|wxEXPAND, 5 );

	m_membershipPane = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxStaticBoxSizer* sbMembership;
	sbMembership = new wxStaticBoxSizer( new wxStaticBox( m_membershipPane, wxID_ANY, _("Net Class Memberships") ), wxHORIZONTAL );

	wxBoxSizer* bLeft;
	bLeft = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbFilters;
	sbFilters = new wxStaticBoxSizer( new wxStaticBox( m_membershipPane, wxID_ANY, _("Filter Nets") ), wxVERTICAL );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	m_ncfilterLabel = new wxStaticText( sbFilters->GetStaticBox(), wxID_ANY, _("Net class filter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ncfilterLabel->Wrap( -1 );
	m_ncfilterLabel->SetMinSize( wxSize( 120,-1 ) );

	bSizer9->Add( m_ncfilterLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_netClassFilterChoices;
	m_netClassFilter = new wxChoice( sbFilters->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_netClassFilterChoices, 0 );
	m_netClassFilter->SetSelection( 0 );
	bSizer9->Add( m_netClassFilter, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	sbFilters->Add( bSizer9, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );

	m_filterLabel = new wxStaticText( sbFilters->GetStaticBox(), wxID_ANY, _("Net name filter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_filterLabel->Wrap( -1 );
	m_filterLabel->SetMinSize( wxSize( 120,-1 ) );

	bSizer10->Add( m_filterLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_netNameFilter = new wxTextCtrl( sbFilters->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer10->Add( m_netNameFilter, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	sbFilters->Add( bSizer10, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxHORIZONTAL );

	m_showAllButton = new wxButton( sbFilters->GetStaticBox(), wxID_ANY, _("Show All Nets"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer13->Add( m_showAllButton, 1, wxALL, 5 );


	bSizer13->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_filterNetsButton = new wxButton( sbFilters->GetStaticBox(), wxID_ANY, _("Apply Filters"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer13->Add( m_filterNetsButton, 1, wxALL, 5 );


	sbFilters->Add( bSizer13, 1, wxEXPAND|wxTOP|wxBOTTOM, 6 );


	bLeft->Add( sbFilters, 0, wxEXPAND|wxBOTTOM, 5 );

	wxStaticBoxSizer* sbEdit;
	sbEdit = new wxStaticBoxSizer( new wxStaticBox( m_membershipPane, wxID_ANY, _("Assign Net Class") ), wxVERTICAL );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxHORIZONTAL );

	m_assignLabel = new wxStaticText( sbEdit->GetStaticBox(), wxID_ANY, _("New net class:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_assignLabel->Wrap( -1 );
	m_assignLabel->SetMinSize( wxSize( 120,-1 ) );

	bSizer11->Add( m_assignLabel, 0, wxALL, 5 );

	wxArrayString m_assignNetClassChoices;
	m_assignNetClass = new wxChoice( sbEdit->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_assignNetClassChoices, 0 );
	m_assignNetClass->SetSelection( 0 );
	bSizer11->Add( m_assignNetClass, 1, wxALL, 5 );


	sbEdit->Add( bSizer11, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxHORIZONTAL );

	m_assignAllButton = new wxButton( sbEdit->GetStaticBox(), wxID_ANY, _("Assign To Listed Nets"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer12->Add( m_assignAllButton, 1, wxALL, 5 );


	bSizer12->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_assignSelectedButton = new wxButton( sbEdit->GetStaticBox(), wxID_ANY, _("Assign To Selected Nets"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer12->Add( m_assignSelectedButton, 1, wxALL, 5 );


	sbEdit->Add( bSizer12, 0, wxEXPAND|wxTOP, 6 );


	bLeft->Add( sbEdit, 1, wxEXPAND|wxTOP, 8 );


	sbMembership->Add( bLeft, 1, wxEXPAND|wxRIGHT, 5 );

	wxBoxSizer* bRight;
	bRight = new wxBoxSizer( wxVERTICAL );

	m_membershipGrid = new WX_GRID( m_membershipPane, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_DEFAULT );

	// Grid
	m_membershipGrid->CreateGrid( 0, 2 );
	m_membershipGrid->EnableEditing( true );
	m_membershipGrid->EnableGridLines( true );
	m_membershipGrid->EnableDragGridSize( false );
	m_membershipGrid->SetMargins( 0, 0 );

	// Columns
	m_membershipGrid->EnableDragColMove( false );
	m_membershipGrid->EnableDragColSize( true );
	m_membershipGrid->SetColLabelSize( 24 );
	m_membershipGrid->SetColLabelValue( 0, _("Net") );
	m_membershipGrid->SetColLabelValue( 1, _("Net Class") );
	m_membershipGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_membershipGrid->EnableDragRowSize( true );
	m_membershipGrid->SetRowLabelSize( 0 );
	m_membershipGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance
	m_membershipGrid->SetLabelFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	// Cell Defaults
	m_membershipGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bRight->Add( m_membershipGrid, 1, wxEXPAND|wxBOTTOM|wxLEFT, 5 );


	sbMembership->Add( bRight, 1, wxEXPAND|wxLEFT, 5 );


	m_membershipPane->SetSizer( sbMembership );
	m_membershipPane->Layout();
	sbMembership->Fit( m_membershipPane );
	bMargins->Add( m_membershipPane, 1, wxALL|wxEXPAND, 5 );


	bpanelNetClassesSizer->Add( bMargins, 1, wxEXPAND, 5 );


	this->SetSizer( bpanelNetClassesSizer );
	this->Layout();
	bpanelNetClassesSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnUpdateUI ) );
	m_netclassGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnSizeNetclassGrid ), NULL, this );
	m_addButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnAddNetclassClick ), NULL, this );
	m_removeButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnRemoveNetclassClick ), NULL, this );
	m_showAllButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnShowAll ), NULL, this );
	m_filterNetsButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnApplyFilters ), NULL, this );
	m_assignAllButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnAssignAll ), NULL, this );
	m_assignSelectedButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnAssignSelected ), NULL, this );
	m_membershipGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnSizeMembershipGrid ), NULL, this );
}

PANEL_SETUP_NETCLASSES_BASE::~PANEL_SETUP_NETCLASSES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnUpdateUI ) );
	m_netclassGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnSizeNetclassGrid ), NULL, this );
	m_addButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnAddNetclassClick ), NULL, this );
	m_removeButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnRemoveNetclassClick ), NULL, this );
	m_showAllButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnShowAll ), NULL, this );
	m_filterNetsButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnApplyFilters ), NULL, this );
	m_assignAllButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnAssignAll ), NULL, this );
	m_assignSelectedButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnAssignSelected ), NULL, this );
	m_membershipGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnSizeMembershipGrid ), NULL, this );

}
