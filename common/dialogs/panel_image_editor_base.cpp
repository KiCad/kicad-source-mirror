///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-282-g1fa54006)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_image_editor_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_IMAGE_EDITOR_BASE::PANEL_IMAGE_EDITOR_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxHORIZONTAL );

	m_panelDraw = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxTAB_TRAVERSAL|wxBORDER_SIMPLE );
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

	wxBoxSizer* bSizerPPI;
	bSizerPPI = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextPPI = new wxStaticText( this, wxID_ANY, _("PPI:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPPI->Wrap( -1 );
	bSizerPPI->Add( m_staticTextPPI, 0, wxALL, 5 );

	m_stPPI_Value = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stPPI_Value->Wrap( -1 );
	bSizerPPI->Add( m_stPPI_Value, 1, wxALL|wxEXPAND, 5 );


	bSizerRight->Add( bSizerPPI, 0, wxEXPAND, 5 );


	bSizerLeft->Add( bSizerRight, 0, wxEXPAND|wxALL, 5 );


	bUpperSizer->Add( bSizerLeft, 1, wxEXPAND, 5 );


	bSizerMain->Add( bUpperSizer, 1, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();

	// Connect Events
	m_panelDraw->Connect( wxEVT_PAINT, wxPaintEventHandler( PANEL_IMAGE_EDITOR_BASE::OnRedrawPanel ), NULL, this );
	m_buttonGrey->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_IMAGE_EDITOR_BASE::OnGreyScaleConvert ), NULL, this );
}

PANEL_IMAGE_EDITOR_BASE::~PANEL_IMAGE_EDITOR_BASE()
{
	// Disconnect Events
	m_panelDraw->Disconnect( wxEVT_PAINT, wxPaintEventHandler( PANEL_IMAGE_EDITOR_BASE::OnRedrawPanel ), NULL, this );
	m_buttonGrey->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_IMAGE_EDITOR_BASE::OnGreyScaleConvert ), NULL, this );

}
