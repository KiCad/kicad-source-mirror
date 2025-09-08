///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "panel_display_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_DISPLAY_OPTIONS_BASE::PANEL_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );

	m_galOptionsSizer = new wxBoxSizer( wxVERTICAL );


	bSizer11->Add( m_galOptionsSizer, 0, wxEXPAND|wxRIGHT, 10 );


	bSizer11->Add( 0, 8, 0, 0, 5 );

	wxBoxSizer* bSizerPads;
	bSizerPads = new wxBoxSizer( wxVERTICAL );

	m_padsLabel = new wxStaticText( this, wxID_ANY, _("Pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_padsLabel->Wrap( -1 );
	bSizerPads->Add( m_padsLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticlinePads = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerPads->Add( m_staticlinePads, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_OptUseViaColorForNormalTHPadstacks = new wxCheckBox( this, wxID_ANY, _("Use via color for normal through hole padstacks"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerPads->Add( m_OptUseViaColorForNormalTHPadstacks, 0, wxALL, 5 );


	bSizerPads->Add( 0, 8, 0, wxEXPAND, 5 );

	m_clearanceLabel = new wxStaticText( this, wxID_ANY, _("Clearance Outlines"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceLabel->Wrap( -1 );
	bSizerPads->Add( m_clearanceLabel, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerPads->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxGridBagSizer* gbSizer2;
	gbSizer2 = new wxGridBagSizer( 2, 0 );
	gbSizer2->SetFlexibleDirection( wxBOTH );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_trackClearancesLabel = new wxStaticText( this, wxID_ANY, _("Tracks:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trackClearancesLabel->Wrap( -1 );
	gbSizer2->Add( m_trackClearancesLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );

	wxString m_OptDisplayTracksClearanceChoices[] = { _("Do not show clearances"), _("Show when routing"), _("Show when routing w/ via clearance at end"), _("Show when routing and editing"), _("Show always") };
	int m_OptDisplayTracksClearanceNChoices = sizeof( m_OptDisplayTracksClearanceChoices ) / sizeof( wxString );
	m_OptDisplayTracksClearance = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_OptDisplayTracksClearanceNChoices, m_OptDisplayTracksClearanceChoices, 0 );
	m_OptDisplayTracksClearance->SetSelection( 0 );
	gbSizer2->Add( m_OptDisplayTracksClearance, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_OptDisplayPadClearence = new wxCheckBox( this, wxID_ANY, _("Show pad clearance"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_OptDisplayPadClearence, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxALL, 5 );


	bSizerPads->Add( gbSizer2, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizerPads->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizer11->Add( bSizerPads, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bupperSizer->Add( bSizer11, 1, wxEXPAND, 5 );


	bupperSizer->Add( 15, 0, 0, 0, 5 );

	m_optionsBook = new wxSimplebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxPanel* fpEditorPage;
	fpEditorPage = new wxPanel( m_optionsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* fpEditorOptionsSizer;
	fpEditorOptionsSizer = new wxBoxSizer( wxVERTICAL );

	m_layerNamesLabel = new wxStaticText( fpEditorPage, wxID_ANY, _("Layer Names"), wxDefaultPosition, wxDefaultSize, 0 );
	m_layerNamesLabel->Wrap( -1 );
	fpEditorOptionsSizer->Add( m_layerNamesLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );


	fpEditorOptionsSizer->Add( 0, 3, 0, 0, 5 );

	m_layerNameitemsGrid = new WX_GRID( fpEditorPage, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxTAB_TRAVERSAL );

	// Grid
	m_layerNameitemsGrid->CreateGrid( 0, 2 );
	m_layerNameitemsGrid->EnableEditing( true );
	m_layerNameitemsGrid->EnableGridLines( true );
	m_layerNameitemsGrid->EnableDragGridSize( false );
	m_layerNameitemsGrid->SetMargins( 0, 0 );

	// Columns
	m_layerNameitemsGrid->SetColSize( 0, 200 );
	m_layerNameitemsGrid->SetColSize( 1, 220 );
	m_layerNameitemsGrid->EnableDragColMove( false );
	m_layerNameitemsGrid->EnableDragColSize( true );
	m_layerNameitemsGrid->SetColLabelValue( 0, _("Layer") );
	m_layerNameitemsGrid->SetColLabelValue( 1, _("Name") );
	m_layerNameitemsGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_layerNameitemsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_layerNameitemsGrid->EnableDragRowSize( false );
	m_layerNameitemsGrid->SetRowLabelSize( 0 );
	m_layerNameitemsGrid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_layerNameitemsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_layerNameitemsGrid->SetMinSize( wxSize( -1,140 ) );

	fpEditorOptionsSizer->Add( m_layerNameitemsGrid, 0, wxEXPAND|wxLEFT, 10 );

	wxBoxSizer* bButtonSize1;
	bButtonSize1 = new wxBoxSizer( wxHORIZONTAL );

	m_bpAddLayer = new STD_BITMAP_BUTTON( fpEditorPage, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize1->Add( m_bpAddLayer, 0, wxBOTTOM|wxLEFT|wxTOP, 5 );


	bButtonSize1->Add( 20, 0, 0, 0, 5 );

	m_bpDeleteLayer = new STD_BITMAP_BUTTON( fpEditorPage, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSize1->Add( m_bpDeleteLayer, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxTOP, 5 );


	fpEditorOptionsSizer->Add( bButtonSize1, 0, wxLEFT, 5 );


	fpEditorPage->SetSizer( fpEditorOptionsSizer );
	fpEditorPage->Layout();
	fpEditorOptionsSizer->Fit( fpEditorPage );
	m_optionsBook->AddPage( fpEditorPage, _("Footprint Editor"), false );
	wxPanel* pcbPage;
	pcbPage = new wxPanel( m_optionsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* pcbOptionsSizer;
	pcbOptionsSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	m_annotationsLabel = new wxStaticText( pcbPage, wxID_ANY, _("Annotations"), wxDefaultPosition, wxDefaultSize, 0 );
	m_annotationsLabel->Wrap( -1 );
	bMargins->Add( m_annotationsLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline1 = new wxStaticLine( pcbPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMargins->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 2, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_netNamesLabel = new wxStaticText( pcbPage, wxID_ANY, _("Net names:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_netNamesLabel->Wrap( -1 );
	gbSizer1->Add( m_netNamesLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );

	wxString m_ShowNetNamesOptionChoices[] = { _("Do not show"), _("Show on pads"), _("Show on tracks"), _("Show on pads & tracks") };
	int m_ShowNetNamesOptionNChoices = sizeof( m_ShowNetNamesOptionChoices ) / sizeof( wxString );
	m_ShowNetNamesOption = new wxChoice( pcbPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ShowNetNamesOptionNChoices, m_ShowNetNamesOptionChoices, 0 );
	m_ShowNetNamesOption->SetSelection( 0 );
	gbSizer1->Add( m_ShowNetNamesOption, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_OptDisplayPadNumber = new wxCheckBox( pcbPage, wxID_ANY, _("Show pad numbers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayPadNumber->SetValue(true);
	gbSizer1->Add( m_OptDisplayPadNumber, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxALL, 5 );


	bMargins->Add( gbSizer1, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bMargins->Add( 0, 8, 0, 0, 5 );

	m_staticText4 = new wxStaticText( pcbPage, wxID_ANY, _("Selection && Highlighting"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	bMargins->Add( m_staticText4, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline4 = new wxStaticLine( pcbPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMargins->Add( m_staticline4, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	m_checkForceShowFieldsWhenFPSelected = new wxCheckBox( pcbPage, wxID_ANY, _("Show all fields when parent footprint is selected"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkForceShowFieldsWhenFPSelected->SetValue(true);
	bSizer9->Add( m_checkForceShowFieldsWhenFPSelected, 0, wxALL, 5 );


	bMargins->Add( bSizer9, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bMargins->Add( 0, 8, 0, 0, 5 );

	m_crossProbingLabel = new wxStaticText( pcbPage, wxID_ANY, _("Cross-probing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_crossProbingLabel->Wrap( -1 );
	bMargins->Add( m_crossProbingLabel, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline3 = new wxStaticLine( pcbPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMargins->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_checkCrossProbeOnSelection = new wxCheckBox( pcbPage, wxID_ANY, _("Select/highlight objects corresponding to schematic selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeOnSelection->SetValue(true);
	m_checkCrossProbeOnSelection->SetToolTip( _("Highlight footprints corresponding to selected symbols") );

	bSizer8->Add( m_checkCrossProbeOnSelection, 0, wxALL, 5 );

	m_checkCrossProbeCenter = new wxCheckBox( pcbPage, wxID_ANY, _("Center view on cross-probed items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeCenter->SetValue(true);
	m_checkCrossProbeCenter->SetToolTip( _("Ensures that cross-probed footprints are visible in the current view") );

	bSizer8->Add( m_checkCrossProbeCenter, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkCrossProbeZoom = new wxCheckBox( pcbPage, wxID_ANY, _("Zoom to fit cross-probed items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeZoom->SetValue(true);
	bSizer8->Add( m_checkCrossProbeZoom, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkCrossProbeAutoHighlight = new wxCheckBox( pcbPage, wxID_ANY, _("Highlight cross-probed nets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeAutoHighlight->SetValue(true);
	m_checkCrossProbeAutoHighlight->SetToolTip( _("Highlight nets when they are highlighted in the schematic editor") );

	bSizer8->Add( m_checkCrossProbeAutoHighlight, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkCrossProbeFlash = new wxCheckBox( pcbPage, wxID_ANY, _("Flash cross-probed selection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeFlash->SetToolTip( _("Temporarily flash the newly cross-probed selection 3 times") );
	bSizer8->Add( m_checkCrossProbeFlash, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_live3Drefresh = new wxCheckBox( pcbPage, wxID_ANY, _("Refresh 3D view automatically"), wxDefaultPosition, wxDefaultSize, 0 );
	m_live3Drefresh->SetToolTip( _("When enabled, edits to the board will cause the 3D view to refresh (may be slow with larger boards)") );

	bSizer8->Add( m_live3Drefresh, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bMargins->Add( bSizer8, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bMargins->Add( 0, 8, 1, wxEXPAND, 5 );


	pcbOptionsSizer->Add( bMargins, 0, wxEXPAND, 5 );


	pcbPage->SetSizer( pcbOptionsSizer );
	pcbPage->Layout();
	pcbOptionsSizer->Fit( pcbPage );
	m_optionsBook->AddPage( pcbPage, _("PCB Editor"), true );

	bupperSizer->Add( m_optionsBook, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bupperSizer, 0, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_layerNameitemsGrid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( PANEL_DISPLAY_OPTIONS_BASE::onLayerChange ), NULL, this );
	m_layerNameitemsGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_DISPLAY_OPTIONS_BASE::OnGridSize ), NULL, this );
	m_bpAddLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DISPLAY_OPTIONS_BASE::OnAddLayerItem ), NULL, this );
	m_bpDeleteLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DISPLAY_OPTIONS_BASE::OnDeleteLayerItem ), NULL, this );
}

PANEL_DISPLAY_OPTIONS_BASE::~PANEL_DISPLAY_OPTIONS_BASE()
{
	// Disconnect Events
	m_layerNameitemsGrid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( PANEL_DISPLAY_OPTIONS_BASE::onLayerChange ), NULL, this );
	m_layerNameitemsGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_DISPLAY_OPTIONS_BASE::OnGridSize ), NULL, this );
	m_bpAddLayer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DISPLAY_OPTIONS_BASE::OnAddLayerItem ), NULL, this );
	m_bpDeleteLayer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DISPLAY_OPTIONS_BASE::OnDeleteLayerItem ), NULL, this );

}
