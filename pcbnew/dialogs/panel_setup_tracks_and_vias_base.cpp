///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "panel_setup_tracks_and_vias_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_TRACKS_AND_VIAS_BASE::PANEL_SETUP_TRACKS_AND_VIAS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerTracks;
	bSizerTracks = new wxBoxSizer( wxVERTICAL );

	wxStaticText* stTracksLabel;
	stTracksLabel = new wxStaticText( this, wxID_ANY, _("Tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	stTracksLabel->Wrap( -1 );
	bSizerTracks->Add( stTracksLabel, 0, wxALL, 5 );

	m_trackWidthsGrid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_trackWidthsGrid->CreateGrid( 8, 1 );
	m_trackWidthsGrid->EnableEditing( true );
	m_trackWidthsGrid->EnableGridLines( true );
	m_trackWidthsGrid->EnableDragGridSize( false );
	m_trackWidthsGrid->SetMargins( 0, 0 );

	// Columns
	m_trackWidthsGrid->SetColSize( 0, 120 );
	m_trackWidthsGrid->EnableDragColMove( false );
	m_trackWidthsGrid->EnableDragColSize( false );
	m_trackWidthsGrid->SetColLabelValue( 0, _("Width") );
	m_trackWidthsGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_trackWidthsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_trackWidthsGrid->EnableDragRowSize( false );
	m_trackWidthsGrid->SetRowLabelSize( 0 );
	m_trackWidthsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_trackWidthsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bSizerTracks->Add( m_trackWidthsGrid, 1, wxEXPAND|wxRIGHT, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_trackWidthsAddButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer3->Add( m_trackWidthsAddButton, 0, wxBOTTOM|wxRIGHT, 5 );

	m_trackWidthsSortButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer3->Add( m_trackWidthsSortButton, 0, wxRIGHT|wxLEFT, 5 );

	m_trackWidthsRemoveButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer3->Add( m_trackWidthsRemoveButton, 0, wxLEFT|wxRIGHT, 25 );


	bSizerTracks->Add( bSizer3, 0, wxEXPAND|wxTOP, 3 );


	bMainSizer->Add( bSizerTracks, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerVias;
	bSizerVias = new wxBoxSizer( wxVERTICAL );

	wxStaticText* stViasLabel;
	stViasLabel = new wxStaticText( this, wxID_ANY, _("Vias"), wxDefaultPosition, wxDefaultSize, 0 );
	stViasLabel->Wrap( -1 );
	bSizerVias->Add( stViasLabel, 0, wxALL, 5 );

	m_viaSizesGrid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_viaSizesGrid->CreateGrid( 8, 2 );
	m_viaSizesGrid->EnableEditing( true );
	m_viaSizesGrid->EnableGridLines( true );
	m_viaSizesGrid->EnableDragGridSize( false );
	m_viaSizesGrid->SetMargins( 0, 0 );

	// Columns
	m_viaSizesGrid->SetColSize( 0, 120 );
	m_viaSizesGrid->SetColSize( 1, 120 );
	m_viaSizesGrid->EnableDragColMove( false );
	m_viaSizesGrid->EnableDragColSize( false );
	m_viaSizesGrid->SetColLabelValue( 0, _("Diameter") );
	m_viaSizesGrid->SetColLabelValue( 1, _("Hole") );
	m_viaSizesGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_viaSizesGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_viaSizesGrid->EnableDragRowSize( false );
	m_viaSizesGrid->SetRowLabelSize( 0 );
	m_viaSizesGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_viaSizesGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bSizerVias->Add( m_viaSizesGrid, 1, wxEXPAND|wxRIGHT, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	m_viaSizesAddButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer4->Add( m_viaSizesAddButton, 0, wxBOTTOM|wxRIGHT, 5 );

	m_viaSizesSortButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer4->Add( m_viaSizesSortButton, 0, wxRIGHT|wxLEFT, 5 );


	bSizer4->Add( 20, 0, 0, wxEXPAND, 5 );

	m_viaSizesRemoveButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer4->Add( m_viaSizesRemoveButton, 0, wxLEFT|wxRIGHT, 5 );


	bSizerVias->Add( bSizer4, 0, wxEXPAND|wxTOP, 3 );


	bMainSizer->Add( bSizerVias, 2, wxEXPAND, 5 );

	wxBoxSizer* bSizerDiffPairs;
	bSizerDiffPairs = new wxBoxSizer( wxVERTICAL );

	wxStaticText* stDiffPairsLabel;
	stDiffPairsLabel = new wxStaticText( this, wxID_ANY, _("Differential Pairs"), wxDefaultPosition, wxDefaultSize, 0 );
	stDiffPairsLabel->Wrap( -1 );
	bSizerDiffPairs->Add( stDiffPairsLabel, 0, wxALL, 5 );

	m_diffPairsGrid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_diffPairsGrid->CreateGrid( 8, 3 );
	m_diffPairsGrid->EnableEditing( true );
	m_diffPairsGrid->EnableGridLines( true );
	m_diffPairsGrid->EnableDragGridSize( false );
	m_diffPairsGrid->SetMargins( 0, 0 );

	// Columns
	m_diffPairsGrid->SetColSize( 0, 120 );
	m_diffPairsGrid->SetColSize( 1, 120 );
	m_diffPairsGrid->SetColSize( 2, 120 );
	m_diffPairsGrid->EnableDragColMove( false );
	m_diffPairsGrid->EnableDragColSize( false );
	m_diffPairsGrid->SetColLabelValue( 0, _("Width") );
	m_diffPairsGrid->SetColLabelValue( 1, _("Gap") );
	m_diffPairsGrid->SetColLabelValue( 2, _("Via Gap") );
	m_diffPairsGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_diffPairsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_diffPairsGrid->EnableDragRowSize( false );
	m_diffPairsGrid->SetRowLabelSize( 0 );
	m_diffPairsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_diffPairsGrid->SetDefaultCellBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_diffPairsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bSizerDiffPairs->Add( m_diffPairsGrid, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	m_diffPairsAddButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer5->Add( m_diffPairsAddButton, 0, wxBOTTOM|wxRIGHT, 5 );

	m_diffPairsSortButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer5->Add( m_diffPairsSortButton, 0, wxRIGHT|wxLEFT, 5 );


	bSizer5->Add( 20, 0, 0, wxEXPAND, 5 );

	m_diffPairsRemoveButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer5->Add( m_diffPairsRemoveButton, 0, wxLEFT|wxRIGHT, 5 );


	bSizerDiffPairs->Add( bSizer5, 0, wxEXPAND|wxTOP, 3 );


	bMainSizer->Add( bSizerDiffPairs, 3, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_trackWidthsAddButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnAddTrackWidthsClick ), NULL, this );
	m_trackWidthsSortButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnSortTrackWidthsClick ), NULL, this );
	m_trackWidthsRemoveButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnRemoveTrackWidthsClick ), NULL, this );
	m_viaSizesAddButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnAddViaSizesClick ), NULL, this );
	m_viaSizesSortButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnSortViaSizesClick ), NULL, this );
	m_viaSizesRemoveButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnRemoveViaSizesClick ), NULL, this );
	m_diffPairsAddButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnAddDiffPairsClick ), NULL, this );
	m_diffPairsSortButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnSortDiffPairsClick ), NULL, this );
	m_diffPairsRemoveButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnRemoveDiffPairsClick ), NULL, this );
}

PANEL_SETUP_TRACKS_AND_VIAS_BASE::~PANEL_SETUP_TRACKS_AND_VIAS_BASE()
{
	// Disconnect Events
	m_trackWidthsAddButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnAddTrackWidthsClick ), NULL, this );
	m_trackWidthsSortButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnSortTrackWidthsClick ), NULL, this );
	m_trackWidthsRemoveButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnRemoveTrackWidthsClick ), NULL, this );
	m_viaSizesAddButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnAddViaSizesClick ), NULL, this );
	m_viaSizesSortButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnSortViaSizesClick ), NULL, this );
	m_viaSizesRemoveButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnRemoveViaSizesClick ), NULL, this );
	m_diffPairsAddButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnAddDiffPairsClick ), NULL, this );
	m_diffPairsSortButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnSortDiffPairsClick ), NULL, this );
	m_diffPairsRemoveButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TRACKS_AND_VIAS_BASE::OnRemoveDiffPairsClick ), NULL, this );

}
