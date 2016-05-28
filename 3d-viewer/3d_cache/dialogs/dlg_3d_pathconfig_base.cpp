///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version May 21 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dlg_3d_pathconfig_base.h"

///////////////////////////////////////////////////////////////////////////

DLG_3D_PATH_CONFIG_BASE::DLG_3D_PATH_CONFIG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 600,150 ), wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerGrid;
	bSizerGrid = new wxBoxSizer( wxHORIZONTAL );

	m_Aliases = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_Aliases->CreateGrid( 1, 3 );
	m_Aliases->EnableEditing( true );
	m_Aliases->EnableGridLines( true );
	m_Aliases->EnableDragGridSize( false );
	m_Aliases->SetMargins( 0, 0 );

	// Columns
	m_Aliases->SetColSize( 0, 80 );
	m_Aliases->SetColSize( 1, 300 );
	m_Aliases->SetColSize( 2, 120 );
	m_Aliases->EnableDragColMove( false );
	m_Aliases->EnableDragColSize( true );
	m_Aliases->SetColLabelSize( 30 );
	m_Aliases->SetColLabelValue( 0, _("Alias") );
	m_Aliases->SetColLabelValue( 1, _("Path") );
	m_Aliases->SetColLabelValue( 2, _("Description") );
	m_Aliases->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );

	// Rows
	m_Aliases->AutoSizeRows();
	m_Aliases->EnableDragRowSize( false );
	m_Aliases->SetRowLabelSize( 80 );
	m_Aliases->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );

	// Label Appearance

	// Cell Defaults
	m_Aliases->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizerGrid->Add( m_Aliases, 1, wxALL|wxEXPAND, 5 );


	bSizerMain->Add( bSizerGrid, 1, wxEXPAND, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxHORIZONTAL );

	m_btnAddAlias = new wxButton( this, wxID_ANY, _("Add Alias"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_btnAddAlias, 0, wxALL, 5 );

	m_btnDelAlias = new wxButton( this, wxID_ANY, _("Remove Alias"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_btnDelAlias, 0, wxALL, 5 );

	m_btnMoveUp = new wxButton( this, wxID_ANY, _("Move Up"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_btnMoveUp, 0, wxALL, 5 );

	m_btnMoveDown = new wxButton( this, wxID_ANY, _("Move Down"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_btnMoveDown, 0, wxALL, 5 );

	m_btnOK = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_btnOK, 0, wxALL, 5 );

	m_btnCancel = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_btnCancel, 0, wxALL, 5 );


	bSizerMain->Add( bSizerButtons, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_btnAddAlias->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnAddAlias ), NULL, this );
	m_btnDelAlias->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnDelAlias ), NULL, this );
	m_btnMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnAliasMoveUp ), NULL, this );
	m_btnMoveDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnAliasMoveDown ), NULL, this );
}

DLG_3D_PATH_CONFIG_BASE::~DLG_3D_PATH_CONFIG_BASE()
{
	// Disconnect Events
	m_btnAddAlias->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnAddAlias ), NULL, this );
	m_btnDelAlias->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnDelAlias ), NULL, this );
	m_btnMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnAliasMoveUp ), NULL, this );
	m_btnMoveDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DLG_3D_PATH_CONFIG_BASE::OnAliasMoveDown ), NULL, this );

}
