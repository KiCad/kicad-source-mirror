///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_package_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PACKAGE_BASE::PANEL_PACKAGE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	this->SetMinSize( wxSize( -1,84 ) );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );

	bSizer5->SetMinSize( wxSize( 70,-1 ) );

	bSizer5->Add( 0, 0, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );


	bSizer6->Add( 0, 0, 1, wxEXPAND, 5 );

	m_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( m_bitmap, 0, wxALL, 5 );


	bSizer6->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizer5->Add( bSizer6, 1, wxEXPAND, 5 );


	bSizer5->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizer1->Add( bSizer5, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	m_name = new wxStaticText( this, wxID_ANY, _("Name"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END );
	m_name->Wrap( -1 );
	bSizer2->Add( m_name, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_descSizer = new wxBoxSizer( wxVERTICAL );

	m_desc = new wxStaticText( this, wxID_ANY, _("Description"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE );
	m_desc->Wrap( -1 );
	m_descSizer->Add( m_desc, 1, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );


	bSizer3->Add( m_descSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );


	bSizer4->Add( 0, 0, 1, wxEXPAND, 5 );

	m_button = new wxButton( this, wxID_ANY, _("Install"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_button, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_splitButton = new SPLIT_BUTTON( this, wxID_ANY, _( "Update" ) );
	bSizer4->Add( m_splitButton, 0, wxBOTTOM|wxRIGHT|wxTOP, 5 );


	bSizer3->Add( bSizer4, 0, wxEXPAND, 5 );


	bSizer2->Add( bSizer3, 1, wxEXPAND, 5 );


	bSizer1->Add( bSizer2, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();
	bSizer1->Fit( this );

	// Connect Events
	this->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( PANEL_PACKAGE_BASE::OnClick ) );
	this->Connect( wxEVT_PAINT, wxPaintEventHandler( PANEL_PACKAGE_BASE::OnPaint ) );
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_PACKAGE_BASE::OnSize ) );
	m_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PACKAGE_BASE::OnButtonClicked ), NULL, this );
}

PANEL_PACKAGE_BASE::~PANEL_PACKAGE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( PANEL_PACKAGE_BASE::OnClick ) );
	this->Disconnect( wxEVT_PAINT, wxPaintEventHandler( PANEL_PACKAGE_BASE::OnPaint ) );
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_PACKAGE_BASE::OnSize ) );
	m_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PACKAGE_BASE::OnButtonClicked ), NULL, this );

}
