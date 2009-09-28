///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_copper_layers_setup_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_COPPER_LAYERS_SETUP_BASE::DIALOG_COPPER_LAYERS_SETUP_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bMainSizerLayers;
	bMainSizerLayers = new wxBoxSizer( wxHORIZONTAL );
	
	wxString m_LayersCountSelectionChoices[] = { _("1"), _("2"), _("4"), _("6"), _("8"), _("10"), _("12"), _("14"), _("16") };
	int m_LayersCountSelectionNChoices = sizeof( m_LayersCountSelectionChoices ) / sizeof( wxString );
	m_LayersCountSelection = new wxRadioBox( this, ID_LAYERS_COUNT_SELECTION, _("Layers Count"), wxDefaultPosition, wxDefaultSize, m_LayersCountSelectionNChoices, m_LayersCountSelectionChoices, 1, wxRA_SPECIFY_COLS );
	m_LayersCountSelection->SetSelection( 1 );
	bMainSizerLayers->Add( m_LayersCountSelection, 0, wxALL, 5 );
	
	m_gridLayersProperties = new wxGrid( this, ID_LAYERS_PROPERTIES, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_gridLayersProperties->CreateGrid( 16, 3 );
	m_gridLayersProperties->EnableEditing( true );
	m_gridLayersProperties->EnableGridLines( true );
	m_gridLayersProperties->EnableDragGridSize( false );
	m_gridLayersProperties->SetMargins( 0, 0 );
	
	// Columns
	m_gridLayersProperties->SetColSize( 0, 100 );
	m_gridLayersProperties->SetColSize( 1, 100 );
	m_gridLayersProperties->SetColSize( 2, 150 );
	m_gridLayersProperties->EnableDragColMove( false );
	m_gridLayersProperties->EnableDragColSize( true );
	m_gridLayersProperties->SetColLabelSize( 30 );
	m_gridLayersProperties->SetColLabelValue( 0, _("Active") );
	m_gridLayersProperties->SetColLabelValue( 1, _("Status") );
	m_gridLayersProperties->SetColLabelValue( 2, _("Name") );
	m_gridLayersProperties->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_gridLayersProperties->AutoSizeRows();
	m_gridLayersProperties->EnableDragRowSize( true );
	m_gridLayersProperties->SetRowLabelSize( 80 );
	m_gridLayersProperties->SetRowLabelValue( 0, _("Top Layer") );
	m_gridLayersProperties->SetRowLabelValue( 1, _("Inner 14") );
	m_gridLayersProperties->SetRowLabelValue( 2, _("Inner 13") );
	m_gridLayersProperties->SetRowLabelValue( 3, _("Inner 12") );
	m_gridLayersProperties->SetRowLabelValue( 4, _("Inner 11") );
	m_gridLayersProperties->SetRowLabelValue( 5, _("Inner 10") );
	m_gridLayersProperties->SetRowLabelValue( 6, _("Inner 9") );
	m_gridLayersProperties->SetRowLabelValue( 7, _("Inner 8") );
	m_gridLayersProperties->SetRowLabelValue( 8, _("Inner 7") );
	m_gridLayersProperties->SetRowLabelValue( 9, _("Inner 6") );
	m_gridLayersProperties->SetRowLabelValue( 10, _("Inner 5") );
	m_gridLayersProperties->SetRowLabelValue( 11, _("Inner 4") );
	m_gridLayersProperties->SetRowLabelValue( 12, _("Inner 3") );
	m_gridLayersProperties->SetRowLabelValue( 13, _("Inner 2") );
	m_gridLayersProperties->SetRowLabelValue( 14, _("Inner 1") );
	m_gridLayersProperties->SetRowLabelValue( 15, _("Bottom Layer") );
	m_gridLayersProperties->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_gridLayersProperties->SetDefaultCellAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	bMainSizerLayers->Add( m_gridLayersProperties, 1, wxALL|wxEXPAND, 5 );
	
	bMainSizer->Add( bMainSizerLayers, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Messages:") ), wxVERTICAL );
	
	m_MessagesList = new wxHtmlWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	m_MessagesList->SetMinSize( wxSize( -1,150 ) );
	
	sbSizer1->Add( m_MessagesList, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	bMainSizer->Add( sbSizer1, 0, wxEXPAND, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	bMainSizer->Add( m_sdbSizer1, 0, wxALIGN_RIGHT, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	m_LayersCountSelection->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_COPPER_LAYERS_SETUP_BASE::OnLayerCountClick ), NULL, this );
	m_gridLayersProperties->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_COPPER_LAYERS_SETUP_BASE::OnLayerGridLeftClick ), NULL, this );
	m_gridLayersProperties->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_COPPER_LAYERS_SETUP_BASE::OnLayerGridRighttClick ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_LAYERS_SETUP_BASE::OnCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_LAYERS_SETUP_BASE::OnOkButtonClick ), NULL, this );
}

DIALOG_COPPER_LAYERS_SETUP_BASE::~DIALOG_COPPER_LAYERS_SETUP_BASE()
{
	// Disconnect Events
	m_LayersCountSelection->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_COPPER_LAYERS_SETUP_BASE::OnLayerCountClick ), NULL, this );
	m_gridLayersProperties->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_COPPER_LAYERS_SETUP_BASE::OnLayerGridLeftClick ), NULL, this );
	m_gridLayersProperties->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_COPPER_LAYERS_SETUP_BASE::OnLayerGridRighttClick ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_LAYERS_SETUP_BASE::OnCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COPPER_LAYERS_SETUP_BASE::OnOkButtonClick ), NULL, this );
}
