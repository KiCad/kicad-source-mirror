///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "panel_fp_user_layer_names_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_FP_USER_LAYER_NAMES_BASE::PANEL_FP_USER_LAYER_NAMES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerMargins;
	bSizerMargins = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUserLayerCount;
	bSizerUserLayerCount = new wxBoxSizer( wxHORIZONTAL );

	m_lblUserLayers = new wxStaticText( this, wxID_ANY, _("User layers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblUserLayers->Wrap( -1 );
	bSizerUserLayerCount->Add( m_lblUserLayers, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxString m_choiceUserLayersChoices[] = { _("0"), _("1"), _("2"), _("3"), _("4"), _("5"), _("6"), _("7"), _("8"), _("9") };
	int m_choiceUserLayersNChoices = sizeof( m_choiceUserLayersChoices ) / sizeof( wxString );
	m_choiceUserLayers = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUserLayersNChoices, m_choiceUserLayersChoices, 0 );
	m_choiceUserLayers->SetSelection( 0 );
	bSizerUserLayerCount->Add( m_choiceUserLayers, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bSizerUserLayerCount->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizerMargins->Add( bSizerUserLayerCount, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 8 );


	bSizerMargins->Add( 0, 10, 0, wxEXPAND, 5 );

	m_layerNamesLabel = new wxStaticText( this, wxID_ANY, _("User Layer Names"), wxDefaultPosition, wxDefaultSize, 0 );
	m_layerNamesLabel->Wrap( -1 );
	bSizerMargins->Add( m_layerNamesLabel, 0, wxTOP|wxLEFT|wxEXPAND, 8 );


	bSizerMargins->Add( 0, 4, 0, wxEXPAND, 5 );

	wxBoxSizer* layerNamesSizer;
	layerNamesSizer = new wxBoxSizer( wxVERTICAL );

	m_layerNamesGrid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxTAB_TRAVERSAL );

	// Grid
	m_layerNamesGrid->CreateGrid( 0, 2 );
	m_layerNamesGrid->EnableEditing( true );
	m_layerNamesGrid->EnableGridLines( true );
	m_layerNamesGrid->EnableDragGridSize( false );
	m_layerNamesGrid->SetMargins( 0, 0 );

	// Columns
	m_layerNamesGrid->SetColSize( 0, 200 );
	m_layerNamesGrid->SetColSize( 1, 220 );
	m_layerNamesGrid->EnableDragColMove( false );
	m_layerNamesGrid->EnableDragColSize( true );
	m_layerNamesGrid->SetColLabelValue( 0, _("Layer") );
	m_layerNamesGrid->SetColLabelValue( 1, _("Name") );
	m_layerNamesGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_layerNamesGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_layerNamesGrid->EnableDragRowSize( false );
	m_layerNamesGrid->SetRowLabelSize( 0 );
	m_layerNamesGrid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_layerNamesGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_layerNamesGrid->SetMinSize( wxSize( -1,140 ) );

	layerNamesSizer->Add( m_layerNamesGrid, 1, wxEXPAND, 5 );

	wxBoxSizer* bButtonSize;
	bButtonSize = new wxBoxSizer( wxHORIZONTAL );

	m_bpAdd = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpAdd->SetMinSize( wxSize( 30,29 ) );

	bButtonSize->Add( m_bpAdd, 0, wxBOTTOM|wxLEFT|wxTOP, 5 );


	bButtonSize->Add( 20, 0, 0, wxEXPAND, 5 );

	m_bpDelete = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpDelete->SetMinSize( wxSize( 30,29 ) );

	bButtonSize->Add( m_bpDelete, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxTOP, 5 );


	bButtonSize->Add( 0, 0, 1, wxEXPAND, 5 );


	layerNamesSizer->Add( bButtonSize, 0, wxEXPAND, 5 );


	bSizerMargins->Add( layerNamesSizer, 1, wxEXPAND, 20 );


	bSizerMain->Add( bSizerMargins, 1, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	// Connect Events
	m_choiceUserLayers->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_FP_USER_LAYER_NAMES_BASE::onUserLayerCountChange ), NULL, this );
	m_layerNamesGrid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( PANEL_FP_USER_LAYER_NAMES_BASE::onLayerChange ), NULL, this );
	m_bpAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FP_USER_LAYER_NAMES_BASE::OnAddLayerItem ), NULL, this );
	m_bpDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FP_USER_LAYER_NAMES_BASE::OnDeleteLayerItem ), NULL, this );
}

PANEL_FP_USER_LAYER_NAMES_BASE::~PANEL_FP_USER_LAYER_NAMES_BASE()
{
	// Disconnect Events
	m_choiceUserLayers->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_FP_USER_LAYER_NAMES_BASE::onUserLayerCountChange ), NULL, this );
	m_layerNamesGrid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( PANEL_FP_USER_LAYER_NAMES_BASE::onLayerChange ), NULL, this );
	m_bpAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FP_USER_LAYER_NAMES_BASE::OnAddLayerItem ), NULL, this );
	m_bpDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FP_USER_LAYER_NAMES_BASE::OnDeleteLayerItem ), NULL, this );

}
