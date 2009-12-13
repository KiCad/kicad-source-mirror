///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_global_pads_edition_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GLOBAL_PADS_EDITION_BASE::DIALOG_GLOBAL_PADS_EDITION_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Pad Filter :") ), wxVERTICAL );
	
	m_Pad_Shape_Filter_CB = new wxCheckBox( this, wxID_ANY, wxT("Do not modify pads having a different shape"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbSizer1->Add( m_Pad_Shape_Filter_CB, 0, wxALL, 5 );
	
	m_Pad_Layer_Filter_CB = new wxCheckBox( this, wxID_ANY, wxT("Do not modify pads having different layers"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbSizer1->Add( m_Pad_Layer_Filter_CB, 0, wxALL, 5 );
	
	m_Pad_Orient_Filter_CB = new wxCheckBox( this, wxID_ANY, wxT("Do not modify pads having a different orientation"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbSizer1->Add( m_Pad_Orient_Filter_CB, 0, wxALL, 5 );
	
	bLeftSizer->Add( sbSizer1, 1, wxEXPAND, 5 );
	
	bMainSizer->Add( bLeftSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonPadEditor = new wxButton( this, ID_CHANGE_GET_PAD_SETTINGS, wxT("Pad Editor"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonPadEditor, 0, wxALL|wxEXPAND, 5 );
	
	
	bRightSizer->Add( 10, 10, 0, 0, 5 );
	
	m_button2 = new wxButton( this, ID_CHANGE_CURRENT_MODULE, wxT("Change Pads on Module"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_button2, 0, wxALL|wxEXPAND, 5 );
	
	m_button3 = new wxButton( this, ID_CHANGE_ID_MODULES, wxT("Change Pads on Same Modules"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_button3, 0, wxALL|wxEXPAND, 5 );
	
	m_button4 = new wxButton( this, wxID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_button4, 0, wxALL|wxEXPAND, 5 );
	
	bMainSizer->Add( bRightSizer, 0, wxEXPAND, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	m_buttonPadEditor->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_PADS_EDITION_BASE::InstallPadEditor ), NULL, this );
	m_button2->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_PADS_EDITION_BASE::PadPropertiesAccept ), NULL, this );
	m_button3->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_PADS_EDITION_BASE::PadPropertiesAccept ), NULL, this );
	m_button4->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_PADS_EDITION_BASE::OnCancelClick ), NULL, this );
}

DIALOG_GLOBAL_PADS_EDITION_BASE::~DIALOG_GLOBAL_PADS_EDITION_BASE()
{
	// Disconnect Events
	m_buttonPadEditor->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_PADS_EDITION_BASE::InstallPadEditor ), NULL, this );
	m_button2->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_PADS_EDITION_BASE::PadPropertiesAccept ), NULL, this );
	m_button3->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_PADS_EDITION_BASE::PadPropertiesAccept ), NULL, this );
	m_button4->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_PADS_EDITION_BASE::OnCancelClick ), NULL, this );
}
