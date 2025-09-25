///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "panel_bom_presets_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_BOM_PRESETS_BASE::PANEL_BOM_PRESETS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_bomTitle = new wxStaticText( this, wxID_ANY, _("Bill of Materials Presets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bomTitle->Wrap( -1 );
	bPanelSizer->Add( m_bomTitle, 0, wxTOP|wxLEFT|wxEXPAND, 8 );


	bPanelSizer->Add( 0, 3, 0, wxEXPAND, 5 );

	m_bomPresetsGrid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_bomPresetsGrid->CreateGrid( 0, 1 );
	m_bomPresetsGrid->EnableEditing( false );
	m_bomPresetsGrid->EnableGridLines( true );
	m_bomPresetsGrid->EnableDragGridSize( false );
	m_bomPresetsGrid->SetMargins( 0, 0 );

	// Columns
	m_bomPresetsGrid->SetColSize( 0, 420 );
	m_bomPresetsGrid->EnableDragColMove( false );
	m_bomPresetsGrid->EnableDragColSize( true );
	m_bomPresetsGrid->SetColLabelValue( 0, _("Name") );
	m_bomPresetsGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_bomPresetsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_bomPresetsGrid->EnableDragRowSize( true );
	m_bomPresetsGrid->SetRowLabelSize( 0 );
	m_bomPresetsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_bomPresetsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_bomPresetsGrid->SetMinSize( wxSize( -1,180 ) );

	bPanelSizer->Add( m_bomPresetsGrid, 1, wxEXPAND|wxBOTTOM, 3 );

	m_btnDeleteBomPreset = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bPanelSizer->Add( m_btnDeleteBomPreset, 0, wxBOTTOM, 5 );


	bPanelSizer->Add( 0, 5, 0, wxEXPAND, 5 );

	m_bomFmtTitle = new wxStaticText( this, wxID_ANY, _("Bill of Materials Formatting Presets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bomFmtTitle->Wrap( -1 );
	bPanelSizer->Add( m_bomFmtTitle, 0, wxEXPAND|wxLEFT|wxTOP, 8 );


	bPanelSizer->Add( 0, 3, 0, wxEXPAND, 5 );

	m_bomFmtPresetsGrid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_bomFmtPresetsGrid->CreateGrid( 0, 1 );
	m_bomFmtPresetsGrid->EnableEditing( false );
	m_bomFmtPresetsGrid->EnableGridLines( true );
	m_bomFmtPresetsGrid->EnableDragGridSize( false );
	m_bomFmtPresetsGrid->SetMargins( 0, 0 );

	// Columns
	m_bomFmtPresetsGrid->SetColSize( 0, 420 );
	m_bomFmtPresetsGrid->EnableDragColMove( false );
	m_bomFmtPresetsGrid->EnableDragColSize( true );
	m_bomFmtPresetsGrid->SetColLabelValue( 0, _("Name") );
	m_bomFmtPresetsGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_bomFmtPresetsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_bomFmtPresetsGrid->EnableDragRowSize( true );
	m_bomFmtPresetsGrid->SetRowLabelSize( 0 );
	m_bomFmtPresetsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_bomFmtPresetsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_bomFmtPresetsGrid->SetMinSize( wxSize( -1,180 ) );

	bPanelSizer->Add( m_bomFmtPresetsGrid, 1, wxEXPAND|wxBOTTOM, 3 );

	m_btnDeleteBomFmtPreset = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bPanelSizer->Add( m_btnDeleteBomFmtPreset, 0, wxBOTTOM, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );

	// Connect Events
	m_bomPresetsGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_BOM_PRESETS_BASE::OnSizeGrid ), NULL, this );
	m_btnDeleteBomPreset->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_BOM_PRESETS_BASE::OnDeleteBomPreset ), NULL, this );
	m_bomFmtPresetsGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_BOM_PRESETS_BASE::OnSizeGrid ), NULL, this );
	m_btnDeleteBomFmtPreset->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_BOM_PRESETS_BASE::OnDeleteBomFmtPreset ), NULL, this );
}

PANEL_BOM_PRESETS_BASE::~PANEL_BOM_PRESETS_BASE()
{
	// Disconnect Events
	m_bomPresetsGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_BOM_PRESETS_BASE::OnSizeGrid ), NULL, this );
	m_btnDeleteBomPreset->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_BOM_PRESETS_BASE::OnDeleteBomPreset ), NULL, this );
	m_bomFmtPresetsGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_BOM_PRESETS_BASE::OnSizeGrid ), NULL, this );
	m_btnDeleteBomFmtPreset->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_BOM_PRESETS_BASE::OnDeleteBomFmtPreset ), NULL, this );

}
