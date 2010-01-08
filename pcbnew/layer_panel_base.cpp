///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 29 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "layer_panel_base.h"

///////////////////////////////////////////////////////////////////////////

LAYER_PANEL_BASE::LAYER_PANEL_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	this->SetMinSize( wxSize( 400,400 ) );
	
	wxBoxSizer* boxSizer;
	boxSizer = new wxBoxSizer( wxVERTICAL );
	
	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_LayerPanel->SetToolTip( _("Layer selection and visibility control") );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );
	
	m_LayerScrolledWindow = new wxScrolledWindow( m_LayerPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_LayerScrolledWindow->SetScrollRate( 5, 5 );
	m_LayersFlexGridSizer = new wxFlexGridSizer( 0, 4, 3, 5 );
	m_LayersFlexGridSizer->SetFlexibleDirection( wxHORIZONTAL );
	m_LayersFlexGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_LayerScrolledWindow->SetSizer( m_LayersFlexGridSizer );
	m_LayerScrolledWindow->Layout();
	m_LayersFlexGridSizer->Fit( m_LayerScrolledWindow );
	bSizer3->Add( m_LayerScrolledWindow, 1, wxBOTTOM|wxEXPAND|wxTOP, 5 );
	
	m_LayerPanel->SetSizer( bSizer3 );
	m_LayerPanel->Layout();
	bSizer3->Fit( m_LayerPanel );
	m_notebook->AddPage( m_LayerPanel, _("Layers"), true );
	m_Page1Panel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Page1Panel->SetToolTip( _("Part depiction and visibility") );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );
	
	m_Page1Panel->SetSizer( bSizer4 );
	m_Page1Panel->Layout();
	bSizer4->Fit( m_Page1Panel );
	m_notebook->AddPage( m_Page1Panel, _("Rendering"), false );
	
	boxSizer->Add( m_notebook, 1, wxEXPAND | wxALL, 5 );
	
	this->SetSizer( boxSizer );
	this->Layout();
	boxSizer->Fit( this );
	
	// Connect Events
	m_LayerScrolledWindow->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( LAYER_PANEL_BASE::OnLeftDblClickLayers ), NULL, this );
	m_LayerScrolledWindow->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( LAYER_PANEL_BASE::OnLeftDownLayers ), NULL, this );
	m_LayerScrolledWindow->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( LAYER_PANEL_BASE::OnRightDownLayers ), NULL, this );
}

LAYER_PANEL_BASE::~LAYER_PANEL_BASE()
{
	// Disconnect Events
	m_LayerScrolledWindow->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( LAYER_PANEL_BASE::OnLeftDblClickLayers ), NULL, this );
	m_LayerScrolledWindow->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( LAYER_PANEL_BASE::OnLeftDownLayers ), NULL, this );
	m_LayerScrolledWindow->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( LAYER_PANEL_BASE::OnRightDownLayers ), NULL, this );
}
