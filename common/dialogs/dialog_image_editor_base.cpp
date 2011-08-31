///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 30 2011)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_image_editor_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_IMAGE_EDITOR_BASE::DIALOG_IMAGE_EDITOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
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
	
	m_buttonMirrorX = new wxButton( this, wxID_ANY, _("Mirror X"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_buttonMirrorX, 0, wxEXPAND|wxALL, 5 );
	
	m_buttonMirrorY = new wxButton( this, wxID_ANY, _("Mirror Y"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_buttonMirrorY, 0, wxEXPAND|wxALL, 5 );
	
	m_buttonRotate = new wxButton( this, wxID_ANY, _("Rotate"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_buttonRotate, 0, wxALL|wxEXPAND, 5 );
	
	m_staticTextScale = new wxStaticText( this, wxID_ANY, _("Image Scale:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextScale->Wrap( -1 );
	bSizerRight->Add( m_staticTextScale, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlScale = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_textCtrlScale, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	bSizerLeft->Add( bSizerRight, 0, wxEXPAND, 5 );
	
	bUpperSizer->Add( bSizerLeft, 1, wxEXPAND, 5 );
	
	bSizerMain->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	bSizerMain->Add( m_sdbSizer1, 0, wxALIGN_RIGHT, 5 );
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_panelDraw->Connect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_IMAGE_EDITOR_BASE::OnRedrawPanel ), NULL, this );
	m_buttonMirrorX->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMAGE_EDITOR_BASE::OnMirrorX_click ), NULL, this );
	m_buttonMirrorY->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMAGE_EDITOR_BASE::OnMirrorY_click ), NULL, this );
	m_buttonRotate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMAGE_EDITOR_BASE::OnRotateClick ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMAGE_EDITOR_BASE::OnCancel_Button ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMAGE_EDITOR_BASE::OnOK_Button ), NULL, this );
}

DIALOG_IMAGE_EDITOR_BASE::~DIALOG_IMAGE_EDITOR_BASE()
{
	// Disconnect Events
	m_panelDraw->Disconnect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_IMAGE_EDITOR_BASE::OnRedrawPanel ), NULL, this );
	m_buttonMirrorX->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMAGE_EDITOR_BASE::OnMirrorX_click ), NULL, this );
	m_buttonMirrorY->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMAGE_EDITOR_BASE::OnMirrorY_click ), NULL, this );
	m_buttonRotate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMAGE_EDITOR_BASE::OnRotateClick ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMAGE_EDITOR_BASE::OnCancel_Button ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMAGE_EDITOR_BASE::OnOK_Button ), NULL, this );
	
}
