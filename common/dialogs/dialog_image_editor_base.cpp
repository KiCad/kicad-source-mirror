///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_image_editor_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_IMAGE_EDITOR_BASE::DIALOG_IMAGE_EDITOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxHORIZONTAL );
	
	m_panelDraw = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
	m_panelDraw->SetMinSize( wxSize( 256,256 ) );
	
	bSizerLeft->Add( m_panelDraw, 1, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );
	
	m_buttonGrey = new wxButton( this, wxID_ANY, _("Grey"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_buttonGrey, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerRight->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 10 );
	
	m_staticTextScale = new wxStaticText( this, wxID_ANY, _("Image Scale:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextScale->Wrap( -1 );
	bSizerRight->Add( m_staticTextScale, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlScale = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_textCtrlScale, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bSizerLeft->Add( bSizerRight, 0, wxEXPAND|wxALL, 5 );
	
	
	bUpperSizer->Add( bSizerLeft, 1, wxEXPAND, 5 );
	
	
	bSizerMain->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerMain->Add( m_sdbSizer, 0, wxALIGN_RIGHT|wxALL, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_panelDraw->Connect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_IMAGE_EDITOR_BASE::OnRedrawPanel ), NULL, this );
	m_buttonGrey->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMAGE_EDITOR_BASE::OnGreyScaleConvert ), NULL, this );
}

DIALOG_IMAGE_EDITOR_BASE::~DIALOG_IMAGE_EDITOR_BASE()
{
	// Disconnect Events
	m_panelDraw->Disconnect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_IMAGE_EDITOR_BASE::OnRedrawPanel ), NULL, this );
	m_buttonGrey->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMAGE_EDITOR_BASE::OnGreyScaleConvert ), NULL, this );
	
}
