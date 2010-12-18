///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_orient_footprints_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_ORIENT_FOOTPRINTS_BASE::DIALOG_ORIENT_FOOTPRINTS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxVERTICAL );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bSizerLeft->Add( m_staticText1, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OrientationCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OrientationCtrl->SetToolTip( _("New orientation (0.1 degree resolution)") );
	
	bSizerLeft->Add( m_OrientationCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Filter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bSizerLeft->Add( m_staticText2, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_FilterPattern = new wxTextCtrl( this, wxID_ANY, _("*"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FilterPattern->SetToolTip( _("Filter to select footprints by reference") );
	
	bSizerLeft->Add( m_FilterPattern, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerLeft->Add( 0, 0, 0, wxTOP|wxBOTTOM, 5 );
	
	m_ApplyToLocked = new wxCheckBox( this, wxID_ANY, _("Include Locked Footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ApplyToLocked->SetToolTip( _("Force locked footprints to be modified") );
	
	bSizerLeft->Add( m_ApplyToLocked, 0, wxALL, 5 );
	
	bSizerMain->Add( bSizerLeft, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonOK->SetDefault(); 
	bSizerRight->Add( m_buttonOK, 0, wxALL, 5 );
	
	m_buttonClose = new wxButton( this, wxID_ANY, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_buttonClose, 0, wxALL, 5 );
	
	bSizerMain->Add( bSizerRight, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ORIENT_FOOTPRINTS_BASE::OnOkClick ), NULL, this );
	m_buttonClose->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ORIENT_FOOTPRINTS_BASE::OnCancelClick ), NULL, this );
}

DIALOG_ORIENT_FOOTPRINTS_BASE::~DIALOG_ORIENT_FOOTPRINTS_BASE()
{
	// Disconnect Events
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ORIENT_FOOTPRINTS_BASE::OnOkClick ), NULL, this );
	m_buttonClose->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ORIENT_FOOTPRINTS_BASE::OnCancelClick ), NULL, this );
	
}
