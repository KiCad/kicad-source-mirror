///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
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

	m_filesGridSizer = new wxBoxSizer( wxVERTICAL );

	m_files_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_files_grid->CreateGrid( 1, 2 );
	m_files_grid->EnableEditing( false );
	m_files_grid->EnableGridLines( true );
	m_files_grid->EnableDragGridSize( false );
	m_files_grid->SetMargins( 0, 0 );

	// Columns
	m_files_grid->SetColSize( 0, 100 );
	m_files_grid->SetColSize( 1, 180 );
	m_files_grid->EnableDragColMove( false );
	m_files_grid->EnableDragColSize( true );
	m_files_grid->SetColLabelValue( 0, _("Filename") );
	m_files_grid->SetColLabelValue( 1, _("Embedded Reference") );
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
	m_filesGridSizer->Add( m_files_grid, 5, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( m_filesGridSizer, 1, wxEXPAND|wxBOTTOM, 3 );

	m_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_browse_button = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_browse_button->SetToolTip( _("Add embedded file") );

	m_buttonsSizer->Add( m_browse_button, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	m_buttonsSizer->Add( 20, 0, 0, wxEXPAND, 5 );

	m_delete_button = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_delete_button->SetToolTip( _("Remove embedded file") );

	m_buttonsSizer->Add( m_delete_button, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	m_buttonsSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_cbEmbedFonts = new wxCheckBox( this, wxID_ANY, _("Embed fonts"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbEmbedFonts->SetToolTip( _("Store a copy of all fonts used") );

	m_buttonsSizer->Add( m_cbEmbedFonts, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 40 );


	m_buttonsSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_export = new wxButton( this, wxID_ANY, _("&Export..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonsSizer->Add( m_export, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bMainSizer->Add( m_buttonsSizer, 0, wxEXPAND|wxBOTTOM, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_files_grid->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( PANEL_EMBEDDED_FILES_BASE::onGridRightClick ), NULL, this );
	m_browse_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_EMBEDDED_FILES_BASE::onAddEmbeddedFiles ), NULL, this );
	m_delete_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_EMBEDDED_FILES_BASE::onDeleteEmbeddedFile ), NULL, this );
	m_cbEmbedFonts->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_EMBEDDED_FILES_BASE::onFontEmbedClick ), NULL, this );
	m_export->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_EMBEDDED_FILES_BASE::onExportFiles ), NULL, this );
}

PANEL_EMBEDDED_FILES_BASE::~PANEL_EMBEDDED_FILES_BASE()
{
	// Disconnect Events
	m_files_grid->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( PANEL_EMBEDDED_FILES_BASE::onGridRightClick ), NULL, this );
	m_browse_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_EMBEDDED_FILES_BASE::onAddEmbeddedFiles ), NULL, this );
	m_delete_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_EMBEDDED_FILES_BASE::onDeleteEmbeddedFile ), NULL, this );
	m_cbEmbedFonts->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_EMBEDDED_FILES_BASE::onFontEmbedClick ), NULL, this );
	m_export->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_EMBEDDED_FILES_BASE::onExportFiles ), NULL, this );

}
