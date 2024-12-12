///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_drc_group_header_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_DRC_GROUP_HEADER_BASE::PANEL_DRC_GROUP_HEADER_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxHORIZONTAL );

	m_dataGrid = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_dataGrid->CreateGrid( 0, 3 );
	m_dataGrid->EnableEditing( true );
	m_dataGrid->EnableGridLines( true );
	m_dataGrid->EnableDragGridSize( false );
	m_dataGrid->SetMargins( 0, 0 );

	// Columns
	m_dataGrid->AutoSizeColumns();
	m_dataGrid->EnableDragColMove( false );
	m_dataGrid->EnableDragColSize( true );
	m_dataGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_dataGrid->EnableDragRowSize( true );
	m_dataGrid->SetRowLabelSize( 0 );
	m_dataGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_dataGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	mainSizer->Add( m_dataGrid, 1, wxALL|wxEXPAND, 5 );


	this->SetSizer( mainSizer );
	this->Layout();

	// Connect Events
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_DRC_GROUP_HEADER_BASE::OnSize ) );
	m_dataGrid->Connect( wxEVT_GRID_COL_SIZE, wxGridSizeEventHandler( PANEL_DRC_GROUP_HEADER_BASE::OnGridSize ), NULL, this );
}

PANEL_DRC_GROUP_HEADER_BASE::~PANEL_DRC_GROUP_HEADER_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_DRC_GROUP_HEADER_BASE::OnSize ) );
	m_dataGrid->Disconnect( wxEVT_GRID_COL_SIZE, wxGridSizeEventHandler( PANEL_DRC_GROUP_HEADER_BASE::OnGridSize ), NULL, this );

}
