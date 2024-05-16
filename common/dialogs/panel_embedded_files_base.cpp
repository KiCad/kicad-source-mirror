///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "panel_embedded_files_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_EMBEDDED_FILES_BASE::PANEL_EMBEDDED_FILES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* m_global_sizer;
	m_global_sizer = new wxBoxSizer( wxVERTICAL );

	m_files_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_files_grid->CreateGrid( 1, 2 );
	m_files_grid->EnableEditing( false );
	m_files_grid->EnableGridLines( true );
	m_files_grid->EnableDragGridSize( false );
	m_files_grid->SetMargins( 0, 0 );

	// Columns
	m_files_grid->SetColSize( 0, 440 );
	m_files_grid->SetColSize( 1, 180 );
	m_files_grid->EnableDragColMove( false );
	m_files_grid->EnableDragColSize( true );
	m_files_grid->SetColLabelValue( 0, _("Filename") );
	m_files_grid->SetColLabelValue( 1, _("Internal Reference") );
	m_files_grid->SetColLabelSize( 22 );
	m_files_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_files_grid->AutoSizeRows();
	m_files_grid->EnableDragRowSize( false );
	m_files_grid->SetRowLabelSize( 0 );
	m_files_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_files_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_global_sizer->Add( m_files_grid, 5, wxALL|wxEXPAND, 5 );


	bMainSizer->Add( m_global_sizer, 1, wxEXPAND, 5 );

	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_browse_button = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_browse_button->SetToolTip( _("Add embedded file") );

	bButtonsSizer->Add( m_browse_button, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bButtonsSizer->Add( 20, 0, 0, wxEXPAND, 5 );

	m_delete_button = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_delete_button->SetToolTip( _("Remove embedded file") );

	bButtonsSizer->Add( m_delete_button, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bButtonsSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_export = new wxButton( this, wxID_ANY, _("&Export"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_export, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bMainSizer->Add( bButtonsSizer, 0, wxEXPAND|wxALL, 3 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	m_cbEmbedFonts = new wxCheckBox( this, wxID_ANY, _("Embed Fonts"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbEmbedFonts->SetToolTip( _("Store a copy of all fonts used") );

	bSizer4->Add( m_cbEmbedFonts, 0, wxALL, 5 );


	bMainSizer->Add( bSizer4, 0, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_EMBEDDED_FILES_BASE::onSize ) );
	m_files_grid->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( PANEL_EMBEDDED_FILES_BASE::onGridRightClick ), NULL, this );
	m_browse_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_EMBEDDED_FILES_BASE::onAddEmbeddedFile ), NULL, this );
	m_delete_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_EMBEDDED_FILES_BASE::onDeleteEmbeddedFile ), NULL, this );
	m_export->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_EMBEDDED_FILES_BASE::onExportFiles ), NULL, this );
}

PANEL_EMBEDDED_FILES_BASE::~PANEL_EMBEDDED_FILES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_EMBEDDED_FILES_BASE::onSize ) );
	m_files_grid->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( PANEL_EMBEDDED_FILES_BASE::onGridRightClick ), NULL, this );
	m_browse_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_EMBEDDED_FILES_BASE::onAddEmbeddedFile ), NULL, this );
	m_delete_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_EMBEDDED_FILES_BASE::onDeleteEmbeddedFile ), NULL, this );
	m_export->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_EMBEDDED_FILES_BASE::onExportFiles ), NULL, this );

}
