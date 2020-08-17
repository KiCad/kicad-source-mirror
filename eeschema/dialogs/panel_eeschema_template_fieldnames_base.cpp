///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "panel_eeschema_template_fieldnames_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_EESCHEMA_TEMPLATE_FIELDNAMES_BASE::PANEL_EESCHEMA_TEMPLATE_FIELDNAMES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	m_leftMargin = new wxBoxSizer( wxHORIZONTAL );

	m_topMargin = new wxBoxSizer( wxVERTICAL );

	m_title = new wxStaticText( this, wxID_ANY, _("Field name templates:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_title->Wrap( -1 );
	m_topMargin->Add( m_title, 0, wxTOP|wxBOTTOM, 5 );

	m_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_grid->CreateGrid( 0, 3 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->SetColSize( 0, 300 );
	m_grid->SetColSize( 1, 60 );
	m_grid->SetColSize( 2, 60 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelSize( 22 );
	m_grid->SetColLabelValue( 0, _("Name") );
	m_grid->SetColLabelValue( 1, _("Visible") );
	m_grid->SetColLabelValue( 2, _("URL") );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( true );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_grid->SetMinSize( wxSize( -1,180 ) );

	m_topMargin->Add( m_grid, 1, wxEXPAND, 10 );

	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );

	m_addFieldButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer10->Add( m_addFieldButton, 0, wxTOP|wxRIGHT, 5 );


	bSizer10->Add( 20, 0, 0, wxEXPAND, 5 );

	m_deleteFieldButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer10->Add( m_deleteFieldButton, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	m_topMargin->Add( bSizer10, 0, wxEXPAND, 10 );


	m_leftMargin->Add( m_topMargin, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	bPanelSizer->Add( m_leftMargin, 1, wxEXPAND, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );

	// Connect Events
	m_grid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_EESCHEMA_TEMPLATE_FIELDNAMES_BASE::OnSizeGrid ), NULL, this );
	m_addFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_EESCHEMA_TEMPLATE_FIELDNAMES_BASE::OnAddButtonClick ), NULL, this );
	m_deleteFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_EESCHEMA_TEMPLATE_FIELDNAMES_BASE::OnDeleteButtonClick ), NULL, this );
}

PANEL_EESCHEMA_TEMPLATE_FIELDNAMES_BASE::~PANEL_EESCHEMA_TEMPLATE_FIELDNAMES_BASE()
{
	// Disconnect Events
	m_grid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_EESCHEMA_TEMPLATE_FIELDNAMES_BASE::OnSizeGrid ), NULL, this );
	m_addFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_EESCHEMA_TEMPLATE_FIELDNAMES_BASE::OnAddButtonClick ), NULL, this );
	m_deleteFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_EESCHEMA_TEMPLATE_FIELDNAMES_BASE::OnDeleteButtonClick ), NULL, this );

}
