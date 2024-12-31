///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "dialog_manage_repositories_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MANAGE_REPOSITORIES_BASE::DIALOG_MANAGE_REPOSITORIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* m_MainSizer;
	m_MainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* m_TopSizer;
	m_TopSizer = new wxBoxSizer( wxVERTICAL );

	m_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_grid->CreateGrid( 5, 2 );
	m_grid->EnableEditing( false );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->SetColSize( 0, 200 );
	m_grid->SetColSize( 1, 400 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelValue( 0, _("Name") );
	m_grid->SetColLabelValue( 1, _("URL") );
	m_grid->SetColLabelSize( 22 );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( true );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_TopSizer->Add( m_grid, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_buttonAdd = new SPLIT_BUTTON( this, wxID_ANY, _( "Add Existing" ) );
	m_buttonAdd->SetToolTip( _("Add repository") );

	bButtonsSizer->Add( m_buttonAdd, 0, wxRIGHT|wxLEFT, 5 );

	m_buttonMoveUp = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_buttonMoveUp->SetToolTip( _("Move up") );

	bButtonsSizer->Add( m_buttonMoveUp, 0, wxRIGHT, 5 );

	m_buttonMoveDown = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_buttonMoveDown->SetToolTip( _("Move down") );

	bButtonsSizer->Add( m_buttonMoveDown, 0, wxRIGHT, 5 );


	bButtonsSizer->Add( 20, 0, 0, wxEXPAND, 5 );

	m_buttonRemove = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_buttonRemove->SetToolTip( _("Remove repository") );

	bButtonsSizer->Add( m_buttonRemove, 0, 0, 5 );


	m_TopSizer->Add( bButtonsSizer, 0, wxRIGHT, 5 );


	m_MainSizer->Add( m_TopSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1Save = new wxButton( this, wxID_SAVE );
	m_sdbSizer1->AddButton( m_sdbSizer1Save );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	m_MainSizer->Add( m_sdbSizer1, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnGridCellClicked ), NULL, this );
	m_buttonMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnMoveUpButtonClicked ), NULL, this );
	m_buttonMoveDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnMoveDownButtonClicked ), NULL, this );
	m_buttonRemove->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnRemoveButtonClicked ), NULL, this );
	m_sdbSizer1Save->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnSaveClicked ), NULL, this );
}

DIALOG_MANAGE_REPOSITORIES_BASE::~DIALOG_MANAGE_REPOSITORIES_BASE()
{
	// Disconnect Events
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnGridCellClicked ), NULL, this );
	m_buttonMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnMoveUpButtonClicked ), NULL, this );
	m_buttonMoveDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnMoveDownButtonClicked ), NULL, this );
	m_buttonRemove->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnRemoveButtonClicked ), NULL, this );
	m_sdbSizer1Save->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnSaveClicked ), NULL, this );

}
