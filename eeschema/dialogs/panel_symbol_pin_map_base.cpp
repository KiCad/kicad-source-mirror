///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "panel_symbol_pin_map_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SYMBOL_PIN_MAP_BASE::PANEL_SYMBOL_PIN_MAP_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_grid->CreateGrid( 0, 3 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( true );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_grid->SetMinSize( wxSize( -1,180 ) );

	bPanelSizer->Add( m_grid, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bButtonSizer;
	bButtonSizer = new wxBoxSizer( wxHORIZONTAL );

	m_addMapButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_addMapButton->SetToolTip( _("Add pin map") );

	bButtonSizer->Add( m_addMapButton, 0, wxALL, 5 );

	m_removeMapButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_removeMapButton->SetToolTip( _("Remove pin map") );

	bButtonSizer->Add( m_removeMapButton, 0, wxALL, 5 );


	bPanelSizer->Add( bButtonSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );

	// Connect Events
	m_grid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SYMBOL_PIN_MAP_BASE::OnSizeGrid ), NULL, this );
	m_addMapButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYMBOL_PIN_MAP_BASE::OnAddMap ), NULL, this );
	m_removeMapButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYMBOL_PIN_MAP_BASE::OnRemoveMap ), NULL, this );
}

PANEL_SYMBOL_PIN_MAP_BASE::~PANEL_SYMBOL_PIN_MAP_BASE()
{
	// Disconnect Events
	m_grid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SYMBOL_PIN_MAP_BASE::OnSizeGrid ), NULL, this );
	m_addMapButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYMBOL_PIN_MAP_BASE::OnAddMap ), NULL, this );
	m_removeMapButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYMBOL_PIN_MAP_BASE::OnRemoveMap ), NULL, this );

}
