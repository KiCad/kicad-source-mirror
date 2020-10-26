///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "dialog_manage_repositories_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MANAGE_REPOSITORIES_BASE::DIALOG_MANAGE_REPOSITORIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 500,300 ), wxDefaultSize );

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
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelSize( 30 );
	m_grid->SetColLabelValue( 0, _("Name") );
	m_grid->SetColLabelValue( 1, _("URL") );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( true );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_TopSizer->Add( m_grid, 1, wxALL|wxEXPAND, 5 );


	m_MainSizer->Add( m_TopSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* m_BottomSizer;
	m_BottomSizer = new wxBoxSizer( wxHORIZONTAL );

	m_buttonAdd = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_BottomSizer->Add( m_buttonAdd, 0, wxALL, 5 );

	m_buttonRemove = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_BottomSizer->Add( m_buttonRemove, 0, wxALL, 5 );


	m_BottomSizer->Add( 20, 0, 0, wxEXPAND, 5 );

	m_buttonMoveUp = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_BottomSizer->Add( m_buttonMoveUp, 0, wxALL, 5 );

	m_buttonMoveDown = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_BottomSizer->Add( m_buttonMoveDown, 0, wxALL, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1Save = new wxButton( this, wxID_SAVE );
	m_sdbSizer1->AddButton( m_sdbSizer1Save );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	m_BottomSizer->Add( m_sdbSizer1, 1, wxEXPAND, 5 );


	m_MainSizer->Add( m_BottomSizer, 0, wxEXPAND, 5 );


	this->SetSizer( m_MainSizer );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnGridCellClicked ), NULL, this );
	m_buttonAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnAddButtonClicked ), NULL, this );
	m_buttonRemove->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnRemoveButtonClicked ), NULL, this );
	m_buttonMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnMoveUpButtonClicked ), NULL, this );
	m_buttonMoveDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnMoveDownButtonClicked ), NULL, this );
	m_sdbSizer1Save->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnSaveClicked ), NULL, this );
}

DIALOG_MANAGE_REPOSITORIES_BASE::~DIALOG_MANAGE_REPOSITORIES_BASE()
{
	// Disconnect Events
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnGridCellClicked ), NULL, this );
	m_buttonAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnAddButtonClicked ), NULL, this );
	m_buttonRemove->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnRemoveButtonClicked ), NULL, this );
	m_buttonMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnMoveUpButtonClicked ), NULL, this );
	m_buttonMoveDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnMoveDownButtonClicked ), NULL, this );
	m_sdbSizer1Save->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MANAGE_REPOSITORIES_BASE::OnSaveClicked ), NULL, this );

}
