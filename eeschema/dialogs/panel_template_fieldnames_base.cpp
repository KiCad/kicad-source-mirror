///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "panel_template_fieldnames_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_TEMPLATE_FIELDNAMES_BASE::PANEL_TEMPLATE_FIELDNAMES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_title = new wxStaticText( this, wxID_ANY, _("Field Name Templates"), wxDefaultPosition, wxDefaultSize, 0 );
	m_title->Wrap( -1 );
	bPanelSizer->Add( m_title, 0, wxTOP|wxLEFT|wxEXPAND, 8 );


	bPanelSizer->Add( 0, 3, 0, wxEXPAND, 5 );

	m_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_grid->CreateGrid( 0, 3 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->SetColSize( 0, 180 );
	m_grid->SetColSize( 1, 48 );
	m_grid->SetColSize( 2, 48 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelValue( 0, _("Name") );
	m_grid->SetColLabelValue( 1, _("Visible") );
	m_grid->SetColLabelValue( 2, _("URL") );
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

	bPanelSizer->Add( m_grid, 1, wxEXPAND|wxRIGHT, 5 );

	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );

	m_addFieldButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer10->Add( m_addFieldButton, 0, wxBOTTOM, 5 );

	m_bpMoveUp = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpMoveUp->SetToolTip( _("Move up") );

	bSizer10->Add( m_bpMoveUp, 0, wxBOTTOM|wxLEFT, 5 );

	m_bpMoveDown = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpMoveDown->SetToolTip( _("Move down") );

	bSizer10->Add( m_bpMoveDown, 0, wxBOTTOM|wxLEFT, 5 );


	bSizer10->Add( 20, 0, 0, wxEXPAND, 5 );

	m_deleteFieldButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer10->Add( m_deleteFieldButton, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bPanelSizer->Add( bSizer10, 0, wxEXPAND|wxTOP, 3 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );

	// Connect Events
	m_addFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TEMPLATE_FIELDNAMES_BASE::OnAddButtonClick ), NULL, this );
	m_bpMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TEMPLATE_FIELDNAMES_BASE::OnMoveUp ), NULL, this );
	m_bpMoveDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TEMPLATE_FIELDNAMES_BASE::OnMoveDown ), NULL, this );
	m_deleteFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TEMPLATE_FIELDNAMES_BASE::OnDeleteButtonClick ), NULL, this );
}

PANEL_TEMPLATE_FIELDNAMES_BASE::~PANEL_TEMPLATE_FIELDNAMES_BASE()
{
	// Disconnect Events
	m_addFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TEMPLATE_FIELDNAMES_BASE::OnAddButtonClick ), NULL, this );
	m_bpMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TEMPLATE_FIELDNAMES_BASE::OnMoveUp ), NULL, this );
	m_bpMoveDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TEMPLATE_FIELDNAMES_BASE::OnMoveDown ), NULL, this );
	m_deleteFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TEMPLATE_FIELDNAMES_BASE::OnDeleteButtonClick ), NULL, this );

}
