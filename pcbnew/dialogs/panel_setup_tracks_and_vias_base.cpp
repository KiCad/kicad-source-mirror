///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "panel_setup_tracks_and_vias_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_TRACKS_AND_VIAS_BASE::PANEL_SETUP_TRACKS_AND_VIAS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_label = new wxStaticText( this, wxID_ANY, _("Pre-defined track and via dimensions:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_label->Wrap( -1 );
	bMainSizer->Add( m_label, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerLower;
	bSizerLower = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbSizer4;
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Tracks") ), wxVERTICAL );
	
	m_trackWidthsGrid = new WX_GRID( sbSizer4->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_trackWidthsGrid->CreateGrid( 8, 1 );
	m_trackWidthsGrid->EnableEditing( true );
	m_trackWidthsGrid->EnableGridLines( true );
	m_trackWidthsGrid->EnableDragGridSize( false );
	m_trackWidthsGrid->SetMargins( 0, 0 );
	
	// Columns
	m_trackWidthsGrid->SetColSize( 0, 100 );
	m_trackWidthsGrid->EnableDragColMove( false );
	m_trackWidthsGrid->EnableDragColSize( false );
	m_trackWidthsGrid->SetColLabelSize( 22 );
	m_trackWidthsGrid->SetColLabelValue( 0, _("Width") );
	m_trackWidthsGrid->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_trackWidthsGrid->EnableDragRowSize( false );
	m_trackWidthsGrid->SetRowLabelSize( 30 );
	m_trackWidthsGrid->SetRowLabelValue( 0, _("1") );
	m_trackWidthsGrid->SetRowLabelValue( 1, _("2") );
	m_trackWidthsGrid->SetRowLabelValue( 2, _("3") );
	m_trackWidthsGrid->SetRowLabelValue( 3, _("4") );
	m_trackWidthsGrid->SetRowLabelValue( 4, _("5") );
	m_trackWidthsGrid->SetRowLabelValue( 5, _("6") );
	m_trackWidthsGrid->SetRowLabelValue( 6, _("7") );
	m_trackWidthsGrid->SetRowLabelValue( 7, _("8") );
	m_trackWidthsGrid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_trackWidthsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	sbSizer4->Add( m_trackWidthsGrid, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerLower->Add( sbSizer4, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer5;
	sbSizer5 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Vias") ), wxVERTICAL );
	
	m_viaSizesGrid = new WX_GRID( sbSizer5->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_viaSizesGrid->CreateGrid( 8, 2 );
	m_viaSizesGrid->EnableEditing( true );
	m_viaSizesGrid->EnableGridLines( true );
	m_viaSizesGrid->EnableDragGridSize( false );
	m_viaSizesGrid->SetMargins( 0, 0 );
	
	// Columns
	m_viaSizesGrid->SetColSize( 0, 100 );
	m_viaSizesGrid->SetColSize( 1, 100 );
	m_viaSizesGrid->EnableDragColMove( false );
	m_viaSizesGrid->EnableDragColSize( false );
	m_viaSizesGrid->SetColLabelSize( 22 );
	m_viaSizesGrid->SetColLabelValue( 0, _("Size") );
	m_viaSizesGrid->SetColLabelValue( 1, _("Drill") );
	m_viaSizesGrid->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_viaSizesGrid->EnableDragRowSize( false );
	m_viaSizesGrid->SetRowLabelSize( 30 );
	m_viaSizesGrid->SetRowLabelValue( 0, _("1") );
	m_viaSizesGrid->SetRowLabelValue( 1, _("2") );
	m_viaSizesGrid->SetRowLabelValue( 2, _("3") );
	m_viaSizesGrid->SetRowLabelValue( 3, _("4") );
	m_viaSizesGrid->SetRowLabelValue( 4, _("5") );
	m_viaSizesGrid->SetRowLabelValue( 5, _("6") );
	m_viaSizesGrid->SetRowLabelValue( 6, _("7") );
	m_viaSizesGrid->SetRowLabelValue( 7, _("8") );
	m_viaSizesGrid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_viaSizesGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	sbSizer5->Add( m_viaSizesGrid, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerLower->Add( sbSizer5, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	wxStaticBoxSizer* sbSizer6;
	sbSizer6 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Differential Pairs") ), wxVERTICAL );
	
	m_diffPairsGrid = new WX_GRID( sbSizer6->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_diffPairsGrid->CreateGrid( 8, 3 );
	m_diffPairsGrid->EnableEditing( true );
	m_diffPairsGrid->EnableGridLines( true );
	m_diffPairsGrid->EnableDragGridSize( false );
	m_diffPairsGrid->SetMargins( 0, 0 );
	
	// Columns
	m_diffPairsGrid->SetColSize( 0, 100 );
	m_diffPairsGrid->SetColSize( 1, 100 );
	m_diffPairsGrid->SetColSize( 2, 100 );
	m_diffPairsGrid->EnableDragColMove( false );
	m_diffPairsGrid->EnableDragColSize( true );
	m_diffPairsGrid->SetColLabelSize( 22 );
	m_diffPairsGrid->SetColLabelValue( 0, _("Width") );
	m_diffPairsGrid->SetColLabelValue( 1, _("Gap") );
	m_diffPairsGrid->SetColLabelValue( 2, _("Via Gap") );
	m_diffPairsGrid->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_diffPairsGrid->EnableDragRowSize( true );
	m_diffPairsGrid->SetRowLabelSize( 30 );
	m_diffPairsGrid->SetRowLabelValue( 0, _("1") );
	m_diffPairsGrid->SetRowLabelValue( 1, _("2") );
	m_diffPairsGrid->SetRowLabelValue( 2, _("3") );
	m_diffPairsGrid->SetRowLabelValue( 3, _("4") );
	m_diffPairsGrid->SetRowLabelValue( 4, _("5") );
	m_diffPairsGrid->SetRowLabelValue( 5, _("6") );
	m_diffPairsGrid->SetRowLabelValue( 6, _("7") );
	m_diffPairsGrid->SetRowLabelValue( 7, _("8") );
	m_diffPairsGrid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_diffPairsGrid->SetDefaultCellBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_diffPairsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	sbSizer6->Add( m_diffPairsGrid, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerLower->Add( sbSizer6, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bMainSizer->Add( bSizerLower, 5, wxEXPAND|wxLEFT, 20 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_SETUP_TRACKS_AND_VIAS_BASE::~PANEL_SETUP_TRACKS_AND_VIAS_BASE()
{
}
