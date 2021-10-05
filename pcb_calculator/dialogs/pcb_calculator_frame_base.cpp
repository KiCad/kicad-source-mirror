///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
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
	m_panelRegulators = new PANEL_REGULATOR( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Notebook->AddPage( m_panelRegulators, _("Regulators"), true );
	m_panelAttenuators = new PANEL_ATTENUATORS( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Notebook->AddPage( m_panelAttenuators, _("RF Attenuators"), false );
	m_panelESeries = new PANEL_E_SERIE( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Notebook->AddPage( m_panelESeries, _("E-Series"), false );
	m_panelColorCode = new PANEL_COLOR_CODE( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Notebook->AddPage( m_panelColorCode, _("Color Code"), false );
	m_panelTransline = new PANEL_TRANSLINE( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Notebook->AddPage( m_panelTransline, _("TransLine"), false );
	m_panelViaSize = new PANEL_VIA_SIZE( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Notebook->AddPage( m_panelViaSize, _("Via Size"), false );
	m_panelTrackWidth = new PANEL_TRACK_WIDTH( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Notebook->AddPage( m_panelTrackWidth, _("Track Width"), false );
	m_panelElectricalSpacing = new PANEL_ELECTRICAL_SPACING( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Notebook->AddPage( m_panelElectricalSpacing, _("Electrical Spacing"), false );
	m_panelBoardClass = new PANEL_BOARD_CLASS( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Notebook->AddPage( m_panelBoardClass, _("Board Classes"), false );

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
