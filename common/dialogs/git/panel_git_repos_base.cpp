///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-254-gc2ef7767)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "panel_git_repos_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_GIT_REPOS_BASE::PANEL_GIT_REPOS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );

	m_staticText12 = new wxStaticText( this, wxID_ANY, _("Git Commit Data"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	bLeftSizer->Add( m_staticText12, 0, wxEXPAND|wxLEFT|wxTOP, 10 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_cbDefault = new wxCheckBox( this, wxID_ANY, _("Use default values"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbDefault->SetValue(true);
	fgSizer1->Add( m_cbDefault, 0, wxALL, 5 );


	fgSizer1->Add( 0, 0, 0, wxEXPAND, 5 );

	m_authorLabel = new wxStaticText( this, wxID_ANY, _("Author name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_authorLabel->Wrap( -1 );
	m_authorLabel->Enable( false );

	fgSizer1->Add( m_authorLabel, 0, wxALL, 5 );

	m_author = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_author->Enable( false );

	fgSizer1->Add( m_author, 0, wxALL|wxEXPAND, 5 );

	m_authorEmailLabel = new wxStaticText( this, wxID_ANY, _("Author e-mail:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_authorEmailLabel->Wrap( -1 );
	m_authorEmailLabel->Enable( false );

	fgSizer1->Add( m_authorEmailLabel, 0, wxALL, 5 );

	m_authorEmail = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_authorEmail->Enable( false );

	fgSizer1->Add( m_authorEmail, 0, wxALL|wxEXPAND, 5 );


	bLeftSizer->Add( fgSizer1, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 13 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftSizer->Add( m_staticline3, 0, wxEXPAND|wxBOTTOM, 5 );

	m_staticText20 = new wxStaticText( this, wxID_ANY, _("Git Repositories"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText20->Wrap( -1 );
	bLeftSizer->Add( m_staticText20, 0, wxEXPAND|wxLEFT|wxRIGHT, 13 );

	wxBoxSizer* bAntialiasingSizer;
	bAntialiasingSizer = new wxBoxSizer( wxVERTICAL );

	m_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxSize( 820,200 ), 0 );

	// Grid
	m_grid->CreateGrid( 0, 10 );
	m_grid->EnableEditing( false );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->SetColSize( 0, 60 );
	m_grid->SetColSize( 1, 200 );
	m_grid->SetColSize( 2, 500 );
	m_grid->SetColSize( 3, 60 );
	m_grid->SetColSize( 4, 0 );
	m_grid->SetColSize( 5, 0 );
	m_grid->SetColSize( 6, 0 );
	m_grid->SetColSize( 7, 0 );
	m_grid->SetColSize( 8, 0 );
	m_grid->SetColSize( 9, 0 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelValue( 0, _("Active") );
	m_grid->SetColLabelValue( 1, _("Name") );
	m_grid->SetColLabelValue( 2, _("Path") );
	m_grid->SetColLabelValue( 3, _("Status") );
	m_grid->SetColLabelSize( 22 );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( true );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bAntialiasingSizer->Add( m_grid, 5, wxALL|wxEXPAND, 5 );


	bLeftSizer->Add( bAntialiasingSizer, 0, wxEXPAND|wxLEFT|wxTOP, 5 );

	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_btnAddRepo = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_btnAddRepo->SetToolTip( _("Add new repository") );

	bButtonsSizer->Add( m_btnAddRepo, 0, wxALL, 5 );

	m_btnEditRepo = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_btnEditRepo->SetToolTip( _("Edit repository properties") );

	bButtonsSizer->Add( m_btnEditRepo, 0, wxALL, 5 );


	bButtonsSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_btnDelete = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_btnDelete->SetToolTip( _("Remove Git Repository") );

	bButtonsSizer->Add( m_btnDelete, 0, wxBOTTOM|wxRIGHT|wxTOP, 5 );


	bLeftSizer->Add( bButtonsSizer, 1, wxALL|wxEXPAND, 5 );


	bPanelSizer->Add( bLeftSizer, 0, wxRIGHT, 20 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );

	// Connect Events
	m_cbDefault->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_GIT_REPOS_BASE::onDefaultClick ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( PANEL_GIT_REPOS_BASE::onGridDClick ), NULL, this );
	m_btnAddRepo->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_GIT_REPOS_BASE::onAddClick ), NULL, this );
	m_btnEditRepo->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_GIT_REPOS_BASE::onEditClick ), NULL, this );
	m_btnDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_GIT_REPOS_BASE::onDeleteClick ), NULL, this );
}

PANEL_GIT_REPOS_BASE::~PANEL_GIT_REPOS_BASE()
{
	// Disconnect Events
	m_cbDefault->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_GIT_REPOS_BASE::onDefaultClick ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( PANEL_GIT_REPOS_BASE::onGridDClick ), NULL, this );
	m_btnAddRepo->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_GIT_REPOS_BASE::onAddClick ), NULL, this );
	m_btnEditRepo->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_GIT_REPOS_BASE::onEditClick ), NULL, this );
	m_btnDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_GIT_REPOS_BASE::onDeleteClick ), NULL, this );

}
