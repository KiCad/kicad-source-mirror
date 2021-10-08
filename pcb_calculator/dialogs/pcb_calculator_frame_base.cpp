///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c5)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_calculator_frame_base.h"

///////////////////////////////////////////////////////////////////////////

PCB_CALCULATOR_FRAME_BASE::PCB_CALCULATOR_FRAME_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : KIWAY_PLAYER( parent, id, title, pos, size, style, name )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_menubar = new wxMenuBar( 0 );
	this->SetMenuBar( m_menubar );

	wxBoxSizer* bmainFrameSizer;
	bmainFrameSizer = new wxBoxSizer( wxVERTICAL );

	m_Notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	bmainFrameSizer->Add( m_Notebook, 1, wxEXPAND, 5 );


	this->SetSizer( bmainFrameSizer );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( PCB_CALCULATOR_FRAME_BASE::OnClosePcbCalc ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PCB_CALCULATOR_FRAME_BASE::OnUpdateUI ) );
}

PCB_CALCULATOR_FRAME_BASE::~PCB_CALCULATOR_FRAME_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( PCB_CALCULATOR_FRAME_BASE::OnClosePcbCalc ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PCB_CALCULATOR_FRAME_BASE::OnUpdateUI ) );

}
