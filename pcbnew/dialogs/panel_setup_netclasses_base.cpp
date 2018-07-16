///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "panel_setup_netclasses_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_NETCLASSES_BASE::PANEL_SETUP_NETCLASSES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bpanelNetClassesSizer;
	bpanelNetClassesSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizerUpper;
	sbSizerUpper = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Net Classes") ), wxVERTICAL );
	
	sbSizerUpper->SetMinSize( wxSize( -1,220 ) ); 
	m_netclassGrid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxTAB_TRAVERSAL|wxVSCROLL );
	
	// Grid
	m_netclassGrid->CreateGrid( 1, 9 );
	m_netclassGrid->EnableEditing( true );
	m_netclassGrid->EnableGridLines( true );
	m_netclassGrid->EnableDragGridSize( false );
	m_netclassGrid->SetMargins( 0, 0 );
	
	// Columns
	m_netclassGrid->SetColSize( 0, 120 );
	m_netclassGrid->SetColSize( 1, 88 );
	m_netclassGrid->SetColSize( 2, 88 );
	m_netclassGrid->SetColSize( 3, 88 );
	m_netclassGrid->SetColSize( 4, 88 );
	m_netclassGrid->SetColSize( 5, 88 );
	m_netclassGrid->SetColSize( 6, 88 );
	m_netclassGrid->SetColSize( 7, 88 );
	m_netclassGrid->SetColSize( 8, 88 );
	m_netclassGrid->EnableDragColMove( false );
	m_netclassGrid->EnableDragColSize( true );
	m_netclassGrid->SetColLabelSize( 22 );
	m_netclassGrid->SetColLabelValue( 0, _("Name") );
	m_netclassGrid->SetColLabelValue( 1, _("Clearance") );
	m_netclassGrid->SetColLabelValue( 2, _("Track Width") );
	m_netclassGrid->SetColLabelValue( 3, _("Via Size") );
	m_netclassGrid->SetColLabelValue( 4, _("Via Drill") );
	m_netclassGrid->SetColLabelValue( 5, _("uVia Size") );
	m_netclassGrid->SetColLabelValue( 6, _("uVia Drill") );
	m_netclassGrid->SetColLabelValue( 7, _("dPair Width") );
	m_netclassGrid->SetColLabelValue( 8, _("dPair Gap") );
	m_netclassGrid->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_netclassGrid->EnableDragRowSize( false );
	m_netclassGrid->SetRowLabelSize( 0 );
	m_netclassGrid->SetRowLabelValue( 0, _("Default") );
	m_netclassGrid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_netclassGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_netclassGrid->SetToolTip( _("Net Class parameters") );
	
	sbSizerUpper->Add( m_netclassGrid, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* buttonBoxSizer;
	buttonBoxSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_addButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW );
	m_addButton->SetMinSize( wxSize( 29,29 ) );
	
	buttonBoxSizer->Add( m_addButton, 0, 0, 5 );
	
	m_removeButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW );
	m_removeButton->SetMinSize( wxSize( 29,29 ) );
	
	buttonBoxSizer->Add( m_removeButton, 0, wxRIGHT, 10 );
	
	
	sbSizerUpper->Add( buttonBoxSizer, 0, wxEXPAND|wxALL, 2 );
	
	
	bpanelNetClassesSizer->Add( sbSizerUpper, 4, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerLower;
	bSizerLower = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbSizerNetSelectMain;
	sbSizerNetSelectMain = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Net Class Membership") ), wxVERTICAL );
	
	m_textNetFilter = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerNetSelectMain->Add( m_textNetFilter, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_membershipGrid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_membershipGrid->CreateGrid( 0, 2 );
	m_membershipGrid->EnableEditing( true );
	m_membershipGrid->EnableGridLines( true );
	m_membershipGrid->EnableDragGridSize( false );
	m_membershipGrid->SetMargins( 0, 0 );
	
	// Columns
	m_membershipGrid->EnableDragColMove( false );
	m_membershipGrid->EnableDragColSize( true );
	m_membershipGrid->SetColLabelSize( 22 );
	m_membershipGrid->SetColLabelValue( 0, _("Net") );
	m_membershipGrid->SetColLabelValue( 1, _("Net Class") );
	m_membershipGrid->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_membershipGrid->EnableDragRowSize( true );
	m_membershipGrid->SetRowLabelSize( 0 );
	m_membershipGrid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_membershipGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	sbSizerNetSelectMain->Add( m_membershipGrid, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerLower->Add( sbSizerNetSelectMain, 1, wxEXPAND|wxTOP|wxRIGHT, 5 );
	
	wxStaticBoxSizer* sbOtherValuesSizer;
	sbOtherValuesSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Non-netclass Predefined Values") ), wxHORIZONTAL );
	
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText24 = new wxStaticText( this, wxID_ANY, _("Tracks:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText24->Wrap( -1 );
	bSizer10->Add( m_staticText24, 0, wxRIGHT|wxLEFT, 5 );
	
	m_trackWidthsGrid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_trackWidthsGrid->CreateGrid( 8, 1 );
	m_trackWidthsGrid->EnableEditing( true );
	m_trackWidthsGrid->EnableGridLines( true );
	m_trackWidthsGrid->EnableDragGridSize( false );
	m_trackWidthsGrid->SetMargins( 0, 0 );
	
	// Columns
	m_trackWidthsGrid->SetColSize( 0, 78 );
	m_trackWidthsGrid->EnableDragColMove( false );
	m_trackWidthsGrid->EnableDragColSize( false );
	m_trackWidthsGrid->SetColLabelSize( 22 );
	m_trackWidthsGrid->SetColLabelValue( 0, _("Width") );
	m_trackWidthsGrid->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_trackWidthsGrid->EnableDragRowSize( false );
	m_trackWidthsGrid->SetRowLabelSize( 0 );
	m_trackWidthsGrid->SetRowLabelValue( 0, _("Track 1") );
	m_trackWidthsGrid->SetRowLabelValue( 1, _("Track 2") );
	m_trackWidthsGrid->SetRowLabelValue( 2, _("Track 3") );
	m_trackWidthsGrid->SetRowLabelValue( 3, _("Track 4") );
	m_trackWidthsGrid->SetRowLabelValue( 4, _("Track 5") );
	m_trackWidthsGrid->SetRowLabelValue( 5, _("Track 6") );
	m_trackWidthsGrid->SetRowLabelValue( 6, _("Track 7") );
	m_trackWidthsGrid->SetRowLabelValue( 7, _("Track 8") );
	m_trackWidthsGrid->SetRowLabelValue( 8, _("Track 9") );
	m_trackWidthsGrid->SetRowLabelValue( 9, _("Track 10") );
	m_trackWidthsGrid->SetRowLabelValue( 10, _("Track 11") );
	m_trackWidthsGrid->SetRowLabelValue( 11, _("Track 12") );
	m_trackWidthsGrid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_trackWidthsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer10->Add( m_trackWidthsGrid, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	sbOtherValuesSizer->Add( bSizer10, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText25 = new wxStaticText( this, wxID_ANY, _("Vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText25->Wrap( -1 );
	bSizer11->Add( m_staticText25, 0, wxRIGHT|wxLEFT, 5 );
	
	m_viaSizesGrid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_viaSizesGrid->CreateGrid( 8, 2 );
	m_viaSizesGrid->EnableEditing( true );
	m_viaSizesGrid->EnableGridLines( true );
	m_viaSizesGrid->EnableDragGridSize( false );
	m_viaSizesGrid->SetMargins( 0, 0 );
	
	// Columns
	m_viaSizesGrid->SetColSize( 0, 78 );
	m_viaSizesGrid->SetColSize( 1, 78 );
	m_viaSizesGrid->EnableDragColMove( false );
	m_viaSizesGrid->EnableDragColSize( false );
	m_viaSizesGrid->SetColLabelSize( 22 );
	m_viaSizesGrid->SetColLabelValue( 0, _("Size") );
	m_viaSizesGrid->SetColLabelValue( 1, _("Drill") );
	m_viaSizesGrid->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_viaSizesGrid->EnableDragRowSize( false );
	m_viaSizesGrid->SetRowLabelSize( 0 );
	m_viaSizesGrid->SetRowLabelValue( 0, _("Via 1") );
	m_viaSizesGrid->SetRowLabelValue( 1, _("Via 2") );
	m_viaSizesGrid->SetRowLabelValue( 2, _("Via 3") );
	m_viaSizesGrid->SetRowLabelValue( 3, _("Via 4") );
	m_viaSizesGrid->SetRowLabelValue( 4, _("Via 5") );
	m_viaSizesGrid->SetRowLabelValue( 5, _("Via 6") );
	m_viaSizesGrid->SetRowLabelValue( 6, _("Via 7") );
	m_viaSizesGrid->SetRowLabelValue( 7, _("Via 8") );
	m_viaSizesGrid->SetRowLabelValue( 8, _("Via 9") );
	m_viaSizesGrid->SetRowLabelValue( 9, _("Via 10") );
	m_viaSizesGrid->SetRowLabelValue( 10, _("Via 11") );
	m_viaSizesGrid->SetRowLabelValue( 11, _("Via 12") );
	m_viaSizesGrid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_viaSizesGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer11->Add( m_viaSizesGrid, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	sbOtherValuesSizer->Add( bSizer11, 0, wxEXPAND|wxLEFT, 5 );
	
	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText26 = new wxStaticText( this, wxID_ANY, _("Differential Pairs:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText26->Wrap( -1 );
	bSizer12->Add( m_staticText26, 0, wxRIGHT|wxLEFT, 5 );
	
	m_diffPairsGrid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_diffPairsGrid->CreateGrid( 8, 3 );
	m_diffPairsGrid->EnableEditing( true );
	m_diffPairsGrid->EnableGridLines( true );
	m_diffPairsGrid->EnableDragGridSize( false );
	m_diffPairsGrid->SetMargins( 0, 0 );
	
	// Columns
	m_diffPairsGrid->SetColSize( 0, 78 );
	m_diffPairsGrid->SetColSize( 1, 78 );
	m_diffPairsGrid->SetColSize( 2, 78 );
	m_diffPairsGrid->EnableDragColMove( false );
	m_diffPairsGrid->EnableDragColSize( true );
	m_diffPairsGrid->SetColLabelSize( 22 );
	m_diffPairsGrid->SetColLabelValue( 0, _("Width") );
	m_diffPairsGrid->SetColLabelValue( 1, _("Gap") );
	m_diffPairsGrid->SetColLabelValue( 2, _("Via Gap") );
	m_diffPairsGrid->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_diffPairsGrid->EnableDragRowSize( true );
	m_diffPairsGrid->SetRowLabelSize( 0 );
	m_diffPairsGrid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_diffPairsGrid->SetDefaultCellBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_INFOBK ) );
	m_diffPairsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer12->Add( m_diffPairsGrid, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	sbOtherValuesSizer->Add( bSizer12, 0, wxEXPAND|wxLEFT, 5 );
	
	
	bSizerLower->Add( sbOtherValuesSizer, 0, wxEXPAND|wxTOP|wxLEFT, 5 );
	
	
	bpanelNetClassesSizer->Add( bSizerLower, 5, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	
	this->SetSizer( bpanelNetClassesSizer );
	this->Layout();
	bpanelNetClassesSizer->Fit( this );
	
	// Connect Events
	m_netclassGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnSizeNetclassGrid ), NULL, this );
	m_addButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnAddNetclassClick ), NULL, this );
	m_removeButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnRemoveNetclassClick ), NULL, this );
	m_textNetFilter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnFilterChanged ), NULL, this );
	m_membershipGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnSizeMembershipGrid ), NULL, this );
	m_membershipGrid->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnUpdateUI ), NULL, this );
}

PANEL_SETUP_NETCLASSES_BASE::~PANEL_SETUP_NETCLASSES_BASE()
{
	// Disconnect Events
	m_netclassGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnSizeNetclassGrid ), NULL, this );
	m_addButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnAddNetclassClick ), NULL, this );
	m_removeButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnRemoveNetclassClick ), NULL, this );
	m_textNetFilter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnFilterChanged ), NULL, this );
	m_membershipGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnSizeMembershipGrid ), NULL, this );
	m_membershipGrid->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnUpdateUI ), NULL, this );
	
}
