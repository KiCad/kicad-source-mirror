///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jan 15 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_find_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FIND_BASE::DIALOG_FIND_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxVERTICAL );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Search for:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bSizerLeft->Add( m_staticText1, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_SearchTextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 200,-1 ), 0 );
	m_SearchTextCtrl->SetMaxLength( 0 ); 
	bSizerLeft->Add( m_SearchTextCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_NoMouseWarpCheckBox = new wxCheckBox( this, wxID_ANY, _("Do not warp mouse pointer"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLeft->Add( m_NoMouseWarpCheckBox, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizerLeft, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );
	
	m_button1 = new wxButton( this, wxID_ANY, _("Find Item"), wxDefaultPosition, wxDefaultSize, 0 );
	m_button1->SetDefault(); 
	bSizerRight->Add( m_button1, 1, wxALL|wxEXPAND, 5 );
	
	m_button2 = new wxButton( this, wxID_ANY, _("Find Marker"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_button2, 1, wxALL|wxEXPAND, 5 );
	
	m_button3 = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_button3, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizerRight, 0, wxALL, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_FIND_BASE::onClose ) );
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_FIND_BASE::OnInitDialog ) );
	m_button1->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onButtonFindItemClick ), NULL, this );
	m_button2->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onButtonFindMarkerClick ), NULL, this );
	m_button3->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onButtonCloseClick ), NULL, this );
}

DIALOG_FIND_BASE::~DIALOG_FIND_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_FIND_BASE::onClose ) );
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_FIND_BASE::OnInitDialog ) );
	m_button1->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onButtonFindItemClick ), NULL, this );
	m_button2->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onButtonFindMarkerClick ), NULL, this );
	m_button3->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onButtonCloseClick ), NULL, this );
	
}
