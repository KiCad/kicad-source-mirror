///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_design_inspector_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_INSPECTOR_BASE::DIALOG_INSPECTOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_panelList = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerListMain;
	bSizerListMain = new wxBoxSizer( wxVERTICAL );

	m_gridListItems = new wxGrid( m_panelList, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_gridListItems->CreateGrid( 1, 5 );
	m_gridListItems->EnableEditing( true );
	m_gridListItems->EnableGridLines( true );
	m_gridListItems->EnableDragGridSize( false );
	m_gridListItems->SetMargins( 0, 0 );

	// Columns
	m_gridListItems->EnableDragColMove( false );
	m_gridListItems->EnableDragColSize( true );
	m_gridListItems->SetColLabelValue( 0, _("-") );
	m_gridListItems->SetColLabelValue( 1, _("Type") );
	m_gridListItems->SetColLabelValue( 2, _("Count") );
	m_gridListItems->SetColLabelValue( 3, _("Comment") );
	m_gridListItems->SetColLabelValue( 4, _("Text") );
	m_gridListItems->SetColLabelSize( 30 );
	m_gridListItems->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_gridListItems->EnableDragRowSize( true );
	m_gridListItems->SetRowLabelSize( 40 );
	m_gridListItems->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_gridListItems->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bSizerListMain->Add( m_gridListItems, 1, wxALL|wxEXPAND, 5 );


	m_panelList->SetSizer( bSizerListMain );
	m_panelList->Layout();
	bSizerListMain->Fit( m_panelList );
	bSizerMain->Add( m_panelList, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerMain->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_gridListItems->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_INSPECTOR_BASE::onCellClicked ), NULL, this );
}

DIALOG_INSPECTOR_BASE::~DIALOG_INSPECTOR_BASE()
{
	// Disconnect Events
	m_gridListItems->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_INSPECTOR_BASE::onCellClicked ), NULL, this );

}
