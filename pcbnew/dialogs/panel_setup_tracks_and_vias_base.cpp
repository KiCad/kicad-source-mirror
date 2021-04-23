///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "panel_setup_tracks_and_vias_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_TRACKS_AND_VIAS_BASE::PANEL_SETUP_TRACKS_AND_VIAS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
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
	m_trackWidthsGrid->SetMargins( 40, 0 );

	// Columns
	m_trackWidthsGrid->EnableDragColMove( false );
	m_trackWidthsGrid->EnableDragColSize( false );
	m_trackWidthsGrid->SetColLabelSize( 24 );
	m_trackWidthsGrid->SetColLabelValue( 0, _("Width") );
	m_trackWidthsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_trackWidthsGrid->EnableDragRowSize( false );
	m_trackWidthsGrid->SetRowLabelSize( 0 );
	m_trackWidthsGrid->SetRowLabelValue( 0, _("1") );
	m_trackWidthsGrid->SetRowLabelValue( 1, _("2") );
	m_trackWidthsGrid->SetRowLabelValue( 2, _("3") );
	m_trackWidthsGrid->SetRowLabelValue( 3, _("4") );
	m_trackWidthsGrid->SetRowLabelValue( 4, _("5") );
	m_trackWidthsGrid->SetRowLabelValue( 5, _("6") );
	m_trackWidthsGrid->SetRowLabelValue( 6, _("7") );
	m_trackWidthsGrid->SetRowLabelValue( 7, _("8") );
	m_trackWidthsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_trackWidthsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	sbSizer4->Add( m_trackWidthsGrid, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_trackWidthsAddButton = new wxBitmapButton( sbSizer4->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer3->Add( m_trackWidthsAddButton, 0, wxLEFT|wxRIGHT, 5 );


	bSizer3->Add( 20, 0, 0, wxEXPAND, 5 );

	m_trackWidthsRemoveButton = new wxBitmapButton( sbSizer4->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer3->Add( m_trackWidthsRemoveButton, 0, wxLEFT|wxRIGHT, 5 );


	sbSizer4->Add( bSizer3, 0, wxEXPAND|wxTOP, 2 );


	bSizerLower->Add( sbSizer4, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer5;
	sbSizer5 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Vias") ), wxVERTICAL );

	m_viaSizesGrid = new WX_GRID( sbSizer5->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_viaSizesGrid->CreateGrid( 8, 2 );
	m_viaSizesGrid->EnableEditing( true );
	m_viaSizesGrid->EnableGridLines( true );
	m_viaSizesGrid->EnableDragGridSize( false );
	m_viaSizesGrid->SetMargins( 40, 0 );

	// Columns
	m_viaSizesGrid->EnableDragColMove( false );
	m_viaSizesGrid->EnableDragColSize( false );
	m_viaSizesGrid->SetColLabelSize( 24 );
	m_viaSizesGrid->SetColLabelValue( 0, _("Size") );
	m_viaSizesGrid->SetColLabelValue( 1, _("Hole") );
	m_viaSizesGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_viaSizesGrid->EnableDragRowSize( false );
	m_viaSizesGrid->SetRowLabelSize( 0 );
	m_viaSizesGrid->SetRowLabelValue( 0, _("1") );
	m_viaSizesGrid->SetRowLabelValue( 1, _("2") );
	m_viaSizesGrid->SetRowLabelValue( 2, _("3") );
	m_viaSizesGrid->SetRowLabelValue( 3, _("4") );
	m_viaSizesGrid->SetRowLabelValue( 4, _("5") );
	m_viaSizesGrid->SetRowLabelValue( 5, _("6") );
	m_viaSizesGrid->SetRowLabelValue( 6, _("7") );
	m_viaSizesGrid->SetRowLabelValue( 7, _("8") );
	m_viaSizesGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_viaSizesGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	sbSizer5->Add( m_viaSizesGrid, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	m_viaSizesAddButton = new wxBitmapButton( sbSizer5->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer4->Add( m_viaSizesAddButton, 0, wxLEFT|wxRIGHT, 5 );


	bSizer4->Add( 20, 0, 0, wxEXPAND, 5 );

	m_viaSizesRemoveButton = new wxBitmapButton( sbSizer5->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer4->Add( m_viaSizesRemoveButton, 0, wxLEFT|wxRIGHT, 5 );


	sbSizer5->Add( bSizer4, 0, wxEXPAND|wxTOP, 2 );


	bSizerLower->Add( sbSizer5, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer6;
	sbSizer6 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Differential Pairs") ), wxVERTICAL );

	m_diffPairsGrid = new WX_GRID( sbSizer6->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_diffPairsGrid->CreateGrid( 8, 3 );
	m_diffPairsGrid->EnableEditing( true );
	m_diffPairsGrid->EnableGridLines( true );
	m_diffPairsGrid->EnableDragGridSize( false );
	m_diffPairsGrid->SetMargins( 40, 0 );

	// Columns
	m_diffPairsGrid->EnableDragColMove( false );
	m_diffPairsGrid->EnableDragColSize( false );
	m_diffPairsGrid->SetColLabelSize( 24 );
	m_diffPairsGrid->SetColLabelValue( 0, _("Width") );
	m_diffPairsGrid->SetColLabelValue( 1, _("Gap") );
	m_diffPairsGrid->SetColLabelValue( 2, _("Via Gap") );
	m_diffPairsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_diffPairsGrid->EnableDragRowSize( false );
	m_diffPairsGrid->SetRowLabelSize( 0 );
	m_diffPairsGrid->SetRowLabelValue( 0, _("1") );
	m_diffPairsGrid->SetRowLabelValue( 1, _("2") );
	m_diffPairsGrid->SetRowLabelValue( 2, _("3") );
	m_diffPairsGrid->SetRowLabelValue( 3, _("4") );
	m_diffPairsGrid->SetRowLabelValue( 4, _("5") );
	m_diffPairsGrid->SetRowLabelValue( 5, _("6") );
	m_diffPairsGrid->SetRowLabelValue( 6, _("7") );
	m_diffPairsGrid->SetRowLabelValue( 7, _("8") );
	m_diffPairsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_diffPairsGrid->SetDefaultCellBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_diffPairsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	sbSizer6->Add( m_diffPairsGrid, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	m_diffPairsAddButton = new wxBitmapButton( sbSizer6->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer5->Add( m_diffPairsAddButton, 0, wxLEFT|wxRIGHT, 5 );


	bSizer5->Add( 20, 0, 0, wxEXPAND, 5 );

	m_diffPairsRemoveButton = new wxBitmapButton( sbSizer6->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer5->Add( m_diffPairsRemoveButton, 0, wxLEFT|wxRIGHT, 5 );


	sbSizer6->Add( bSizer5, 0, wxEXPAND|wxTOP, 2 );


	bSizerLower->Add( sbSizer6, 0, wxEXPAND, 5 );


	bMainSizer->Add( bSizerLower, 5, wxEXPAND|wxLEFT, 20 );


	this->SetSizer( bMainSizer );
	this->Layout();

	// Connect Events
	m_trackWidthsAddButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnAddTrackWidthsClick ), NULL, this );
	m_trackWidthsRemoveButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnRemoveTrackWidthsClick ), NULL, this );
	m_viaSizesAddButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnAddViaSizesClick ), NULL, this );
	m_viaSizesRemoveButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnRemoveViaSizesClick ), NULL, this );
	m_diffPairsAddButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnAddDiffPairsClick ), NULL, this );
	m_diffPairsRemoveButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnRemoveDiffPairsClick ), NULL, this );
}

PANEL_SETUP_TRACKS_AND_VIAS_BASE::~PANEL_SETUP_TRACKS_AND_VIAS_BASE()
{
	// Disconnect Events
	m_trackWidthsAddButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnAddTrackWidthsClick ), NULL, this );
	m_trackWidthsRemoveButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnRemoveTrackWidthsClick ), NULL, this );
	m_viaSizesAddButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnAddViaSizesClick ), NULL, this );
	m_viaSizesRemoveButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnRemoveViaSizesClick ), NULL, this );
	m_diffPairsAddButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnAddDiffPairsClick ), NULL, this );
	m_diffPairsRemoveButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnRemoveDiffPairsClick ), NULL, this );

}
