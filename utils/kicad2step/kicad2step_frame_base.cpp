///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 10 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "kicad2step_frame_base.h"

///////////////////////////////////////////////////////////////////////////

KICAD2STEP_FRAME_BASE::KICAD2STEP_FRAME_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_panelKicad2Step = new PANEL_KICAD2STEP( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	bSizerMain->Add( m_panelKicad2Step, 1, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();

	this->Centre( wxBOTH );
}

KICAD2STEP_FRAME_BASE::~KICAD2STEP_FRAME_BASE()
{
}
