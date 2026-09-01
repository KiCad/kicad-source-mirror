///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "panel_setup_via_stacks_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_VIA_STACKS_BASE::PANEL_SETUP_VIA_STACKS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_grid->CreateGrid( 0, 0 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( true );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bMainSizer->Add( m_grid, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bButtonSizer;
	bButtonSizer = new wxBoxSizer( wxHORIZONTAL );


	bButtonSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_addButton = new wxButton( this, wxID_ANY, _("Add..."), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonSizer->Add( m_addButton, 0, wxALL, 5 );

	m_editButton = new wxButton( this, wxID_ANY, _("Edit..."), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonSizer->Add( m_editButton, 0, wxALL, 5 );

	m_removeButton = new wxButton( this, wxID_ANY, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonSizer->Add( m_removeButton, 0, wxALL, 5 );


	bMainSizer->Add( bButtonSizer, 0, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
}

PANEL_SETUP_VIA_STACKS_BASE::~PANEL_SETUP_VIA_STACKS_BASE()
{
}
