///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_lib_edit_pin_table_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LIB_EDIT_PIN_TABLE_BASE::DIALOG_LIB_EDIT_PIN_TABLE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* top_sizer;
	top_sizer = new wxBoxSizer( wxVERTICAL );
	
	m_Pins = new wxDataViewCtrl( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxDV_HORIZ_RULES|wxDV_MULTIPLE|wxDV_ROW_LINES|wxDV_VERT_RULES );
	m_Pins->SetMinSize( wxSize( 400,400 ) );
	
	top_sizer->Add( m_Pins, 1, wxALL|wxEXPAND, 5 );
	
	m_Summary = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxNO_BORDER );
	top_sizer->Add( m_Summary, 0, wxALL|wxEXPAND, 5 );
	
	m_Buttons = new wxStdDialogButtonSizer();
	m_ButtonsOK = new wxButton( this, wxID_OK );
	m_Buttons->AddButton( m_ButtonsOK );
	m_Buttons->Realize();
	
	top_sizer->Add( m_Buttons, 0, wxEXPAND|wxALL, 5 );
	
	
	this->SetSizer( top_sizer );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxID_ANY, wxEVT_COMMAND_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK, wxDataViewEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnColumnHeaderRightClicked ) );
}

DIALOG_LIB_EDIT_PIN_TABLE_BASE::~DIALOG_LIB_EDIT_PIN_TABLE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK, wxDataViewEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnColumnHeaderRightClicked ) );
	
}
