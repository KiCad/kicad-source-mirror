///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"
#include "widgets/wx_html_report_box.h"
#include "widgets/wx_panel.h"

#include "panel_setup_netclasses_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_NETCLASSES_BASE::PANEL_SETUP_NETCLASSES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bpanelNetClassesSizer;
	bpanelNetClassesSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	m_splitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE|wxSP_NO_XP_THEME );
	m_splitter->SetMinimumPaneSize( 160 );

	m_netclassesPane = new WX_PANEL( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxVERTICAL );

	m_staticText3 = new wxStaticText( m_netclassesPane, wxID_ANY, _("Netclasses"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	bUpperSizer->Add( m_staticText3, 0, wxTOP|wxLEFT|wxEXPAND, 8 );


	bUpperSizer->Add( 0, 3, 0, wxEXPAND, 5 );

	m_netclassGrid = new WX_GRID( m_netclassesPane, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxTAB_TRAVERSAL|wxVSCROLL );

	// Grid
	m_netclassGrid->CreateGrid( 3, 15 );
	m_netclassGrid->EnableEditing( true );
	m_netclassGrid->EnableGridLines( true );
	m_netclassGrid->EnableDragGridSize( false );
	m_netclassGrid->SetMargins( 0, 0 );

	// Columns
	m_netclassGrid->EnableDragColMove( false );
	m_netclassGrid->EnableDragColSize( true );
	m_netclassGrid->SetColLabelValue( 0, _("Name") );
	m_netclassGrid->SetColLabelValue( 1, _("Clearance") );
	m_netclassGrid->SetColLabelValue( 2, _("Track Width") );
	m_netclassGrid->SetColLabelValue( 3, _("Via Size") );
	m_netclassGrid->SetColLabelValue( 4, _("Via Hole") );
	m_netclassGrid->SetColLabelValue( 5, _("uVia Size") );
	m_netclassGrid->SetColLabelValue( 6, _("uVia Hole") );
	m_netclassGrid->SetColLabelValue( 7, _("DP Width") );
	m_netclassGrid->SetColLabelValue( 8, _("DP Gap") );
	m_netclassGrid->SetColLabelValue( 9, _("Tuning Profile") );
	m_netclassGrid->SetColLabelValue( 10, _("PCB Color") );
	m_netclassGrid->SetColLabelValue( 11, _("Wire Thickness") );
	m_netclassGrid->SetColLabelValue( 12, _("Bus Thickness") );
	m_netclassGrid->SetColLabelValue( 13, _("Color") );
	m_netclassGrid->SetColLabelValue( 14, _("Line Style") );
	m_netclassGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_netclassGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_netclassGrid->EnableDragRowSize( true );
	m_netclassGrid->SetRowLabelValue( 0, _("Default") );
	m_netclassGrid->SetRowLabelSize( 0 );
	m_netclassGrid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_netclassGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bUpperSizer->Add( m_netclassGrid, 1, wxEXPAND|wxFIXED_MINSIZE|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* buttonBoxSizer;
	buttonBoxSizer = new wxBoxSizer( wxHORIZONTAL );

	m_addButton = new STD_BITMAP_BUTTON( m_netclassesPane, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	buttonBoxSizer->Add( m_addButton, 0, wxBOTTOM|wxLEFT, 5 );

	m_moveUpButton = new STD_BITMAP_BUTTON( m_netclassesPane, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	buttonBoxSizer->Add( m_moveUpButton, 0, wxBOTTOM|wxLEFT, 5 );

	m_moveDownButton = new STD_BITMAP_BUTTON( m_netclassesPane, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	buttonBoxSizer->Add( m_moveDownButton, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	buttonBoxSizer->Add( 20, 0, 0, wxEXPAND, 5 );

	m_removeButton = new STD_BITMAP_BUTTON( m_netclassesPane, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	buttonBoxSizer->Add( m_removeButton, 0, wxBOTTOM|wxLEFT, 5 );


	buttonBoxSizer->Add( 40, 0, 1, wxEXPAND, 5 );

	m_colorDefaultHelpText = new wxStaticText( m_netclassesPane, wxID_ANY, _("Set color to transparent to use KiCad default color."), wxDefaultPosition, wxDefaultSize, 0 );
	m_colorDefaultHelpText->Wrap( -1 );
	buttonBoxSizer->Add( m_colorDefaultHelpText, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	buttonBoxSizer->Add( 10, 0, 0, wxEXPAND, 5 );

	m_importColorsButton = new wxButton( m_netclassesPane, wxID_ANY, _("Import colors from schematic"), wxDefaultPosition, wxDefaultSize, 0 );
	buttonBoxSizer->Add( m_importColorsButton, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bUpperSizer->Add( buttonBoxSizer, 0, wxEXPAND|wxTOP|wxBOTTOM, 3 );


	m_netclassesPane->SetSizer( bUpperSizer );
	m_netclassesPane->Layout();
	bUpperSizer->Fit( m_netclassesPane );
	m_membershipPane = new WX_PANEL( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxVERTICAL );

	m_staticText5 = new wxStaticText( m_membershipPane, wxID_ANY, _("Netclass Assignments"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	bLowerSizer->Add( m_staticText5, 0, wxEXPAND|wxTOP|wxLEFT, 8 );

	wxBoxSizer* bColumns;
	bColumns = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxVERTICAL );

	m_assignmentGrid = new WX_GRID( m_membershipPane, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_DEFAULT );

	// Grid
	m_assignmentGrid->CreateGrid( 5, 2 );
	m_assignmentGrid->EnableEditing( true );
	m_assignmentGrid->EnableGridLines( true );
	m_assignmentGrid->EnableDragGridSize( false );
	m_assignmentGrid->SetMargins( 0, 0 );

	// Columns
	m_assignmentGrid->SetColSize( 0, 270 );
	m_assignmentGrid->SetColSize( 1, 160 );
	m_assignmentGrid->EnableDragColMove( false );
	m_assignmentGrid->EnableDragColSize( true );
	m_assignmentGrid->SetColLabelValue( 0, _("Pattern") );
	m_assignmentGrid->SetColLabelValue( 1, _("Net Class") );
	m_assignmentGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_assignmentGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_assignmentGrid->EnableDragRowSize( true );
	m_assignmentGrid->SetRowLabelSize( 0 );
	m_assignmentGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_assignmentGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bSizer14->Add( m_assignmentGrid, 2, wxEXPAND|wxFIXED_MINSIZE|wxLEFT, 5 );


	bColumns->Add( bSizer14, 3, wxEXPAND, 5 );

	m_matchingNets = new WX_HTML_REPORT_BOX( m_membershipPane, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	bColumns->Add( m_matchingNets, 2, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bLowerSizer->Add( bColumns, 1, wxEXPAND|wxTOP, 3 );

	wxBoxSizer* buttonBoxSizer1;
	buttonBoxSizer1 = new wxBoxSizer( wxHORIZONTAL );

	m_addAssignmentButton = new STD_BITMAP_BUTTON( m_membershipPane, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	buttonBoxSizer1->Add( m_addAssignmentButton, 0, wxBOTTOM|wxLEFT, 5 );


	buttonBoxSizer1->Add( 20, 0, 0, wxEXPAND, 5 );

	m_removeAssignmentButton = new STD_BITMAP_BUTTON( m_membershipPane, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	buttonBoxSizer1->Add( m_removeAssignmentButton, 0, wxRIGHT|wxLEFT, 5 );


	bLowerSizer->Add( buttonBoxSizer1, 0, wxEXPAND|wxTOP, 3 );


	m_membershipPane->SetSizer( bLowerSizer );
	m_membershipPane->Layout();
	bLowerSizer->Fit( m_membershipPane );
	m_splitter->SplitHorizontally( m_netclassesPane, m_membershipPane, -1 );
	bMargins->Add( m_splitter, 1, wxEXPAND, 10 );


	bpanelNetClassesSizer->Add( bMargins, 1, wxEXPAND|wxTOP, 2 );


	this->SetSizer( bpanelNetClassesSizer );
	this->Layout();
	bpanelNetClassesSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnUpdateUI ) );
	m_netclassGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnSizeNetclassGrid ), NULL, this );
	m_addButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnAddNetclassClick ), NULL, this );
	m_moveUpButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnMoveNetclassUpClick ), NULL, this );
	m_moveDownButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnMoveNetclassDownClick ), NULL, this );
	m_removeButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnRemoveNetclassClick ), NULL, this );
	m_importColorsButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnImportColorsClick ), NULL, this );
	m_membershipPane->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::onmembershipPanelSize ), NULL, this );
	m_assignmentGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnSizeAssignmentGrid ), NULL, this );
	m_assignmentGrid->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnUpdateUI ), NULL, this );
	m_addAssignmentButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnAddAssignmentClick ), NULL, this );
	m_removeAssignmentButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnRemoveAssignmentClick ), NULL, this );
}

PANEL_SETUP_NETCLASSES_BASE::~PANEL_SETUP_NETCLASSES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnUpdateUI ) );
	m_netclassGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnSizeNetclassGrid ), NULL, this );
	m_addButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnAddNetclassClick ), NULL, this );
	m_moveUpButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnMoveNetclassUpClick ), NULL, this );
	m_moveDownButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnMoveNetclassDownClick ), NULL, this );
	m_removeButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnRemoveNetclassClick ), NULL, this );
	m_importColorsButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnImportColorsClick ), NULL, this );
	m_membershipPane->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::onmembershipPanelSize ), NULL, this );
	m_assignmentGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnSizeAssignmentGrid ), NULL, this );
	m_assignmentGrid->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnUpdateUI ), NULL, this );
	m_addAssignmentButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnAddAssignmentClick ), NULL, this );
	m_removeAssignmentButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnRemoveAssignmentClick ), NULL, this );

}
