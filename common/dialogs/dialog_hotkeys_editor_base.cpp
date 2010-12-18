///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_hotkeys_editor_base.h"

///////////////////////////////////////////////////////////////////////////

HOTKEYS_EDITOR_DIALOG_BASE::HOTKEYS_EDITOR_DIALOG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_hotkeyGrid = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDOUBLE_BORDER|wxTAB_TRAVERSAL|wxWANTS_CHARS );
	
	// Grid
	m_hotkeyGrid->CreateGrid( 1, 2 );
	m_hotkeyGrid->EnableEditing( false );
	m_hotkeyGrid->EnableGridLines( true );
	m_hotkeyGrid->EnableDragGridSize( false );
	m_hotkeyGrid->SetMargins( 0, 0 );
	
	// Columns
	m_hotkeyGrid->AutoSizeColumns();
	m_hotkeyGrid->EnableDragColMove( false );
	m_hotkeyGrid->EnableDragColSize( true );
	m_hotkeyGrid->SetColLabelSize( 30 );
	m_hotkeyGrid->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_hotkeyGrid->EnableDragRowSize( true );
	m_hotkeyGrid->SetRowLabelSize( 0 );
	m_hotkeyGrid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_hotkeyGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bMainSizer->Add( m_hotkeyGrid, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* b_buttonsSizer;
	b_buttonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_OKButton = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	b_buttonsSizer->Add( m_OKButton, 0, wxALL|wxEXPAND, 5 );
	
	 m_cancelButton = new wxButton( this, wxID_ANY, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	b_buttonsSizer->Add(  m_cancelButton, 0, wxALL|wxEXPAND, 5 );
	
	m_undoButton = new wxButton( this, wxID_CANCEL, _("Undo"), wxDefaultPosition, wxDefaultSize, 0 );
	b_buttonsSizer->Add( m_undoButton, 0, wxALL|wxEXPAND, 5 );
	
	bMainSizer->Add( b_buttonsSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	m_hotkeyGrid->Connect( wxEVT_CHAR, wxKeyEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::OnKeyPressed ), NULL, this );
	m_hotkeyGrid->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::OnRightClickOnCell ), NULL, this );
	m_hotkeyGrid->Connect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::OnClickOnCell ), NULL, this );
	m_OKButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::OnOKClicked ), NULL, this );
	 m_cancelButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::CancelClicked ), NULL, this );
	m_undoButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::UndoClicked ), NULL, this );
}

HOTKEYS_EDITOR_DIALOG_BASE::~HOTKEYS_EDITOR_DIALOG_BASE()
{
	// Disconnect Events
	m_hotkeyGrid->Disconnect( wxEVT_CHAR, wxKeyEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::OnKeyPressed ), NULL, this );
	m_hotkeyGrid->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::OnRightClickOnCell ), NULL, this );
	m_hotkeyGrid->Disconnect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::OnClickOnCell ), NULL, this );
	m_OKButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::OnOKClicked ), NULL, this );
	 m_cancelButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::CancelClicked ), NULL, this );
	m_undoButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HOTKEYS_EDITOR_DIALOG_BASE::UndoClicked ), NULL, this );
}
