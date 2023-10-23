///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
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
	m_panelDraw->SetMinSize( wxSize( 300,300 ) );

	bSizerLeft->Add( m_panelDraw, 1, wxEXPAND | wxALL, 5 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 5, 5 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextScale = new wxStaticText( this, wxID_ANY, _("Scale:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextScale->Wrap( -1 );
	gbSizer1->Add( m_staticTextScale, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlScale = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_textCtrlScale, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticTextPPI = new wxStaticText( this, wxID_ANY, _("PPI:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPPI->Wrap( -1 );
	gbSizer1->Add( m_staticTextPPI, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stPPI_Value = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stPPI_Value->Wrap( -1 );
	gbSizer1->Add( m_stPPI_Value, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_buttonGrey = new wxButton( this, wxID_ANY, _("Convert to Greyscale"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_buttonGrey, wxGBPosition( 4, 0 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );


	gbSizer1->AddGrowableCol( 1 );

	bSizerLeft->Add( gbSizer1, 0, wxEXPAND|wxALL, 10 );


	bUpperSizer->Add( bSizerLeft, 1, wxEXPAND, 5 );


	bSizerMain->Add( bUpperSizer, 1, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

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
