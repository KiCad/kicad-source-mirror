///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_corrosion_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_CORROSION_BASE::PANEL_CORROSION_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	m_scrolledWindow1 = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_scrolledWindow1->SetScrollRate( 5, 5 );
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );

	m_table = new wxGrid( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_table->CreateGrid( 1, 1 );
	m_table->EnableEditing( true );
	m_table->EnableGridLines( true );
	m_table->EnableDragGridSize( false );
	m_table->SetMargins( 0, 0 );

	// Columns
	m_table->EnableDragColMove( false );
	m_table->EnableDragColSize( true );
	m_table->SetColLabelValue( 0, _("Copper (Cu)") );
	m_table->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_table->EnableDragRowSize( true );
	m_table->SetRowLabelValue( 0, _("Copper (Cu)") );
	m_table->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_table->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer7->Add( m_table, 1, wxALL|wxEXPAND, 5 );


	m_scrolledWindow1->SetSizer( bSizer7 );
	m_scrolledWindow1->Layout();
	bSizer7->Fit( m_scrolledWindow1 );
	bSizer6->Add( m_scrolledWindow1, 1, wxEXPAND | wxALL, 5 );

	m_staticText16 = new wxStaticText( this, wxID_ANY, _("This table shows the difference in electrochemical potential between various metals and alloys. A positive number indicates that the row is anodic and the column is cathodic.\nGalvanic corrosion affects different metals in contact and under certain conditions.\nThe anode of an electrochemical pair gets oxidized and eaten away, while the cathode gets dissolved metals plated onto it but stays protected.\nEN 50310 suggests a voltage difference below 300mV. Known practices make use of a third interface metal in between the main pair(ie the ENIG surface finish)."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	bSizer6->Add( m_staticText16, 0, wxALL, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Threshold voltage:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bSizer3->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_corFilterCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_corFilterCtrl, 0, wxALL, 5 );

	m_staticText3 = new wxStaticText( this, wxID_ANY, _("mV"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	bSizer3->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxRIGHT, 5 );


	bSizer6->Add( bSizer3, 0, wxEXPAND, 5 );


	this->SetSizer( bSizer6 );
	this->Layout();

	// Connect Events
	m_corFilterCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CORROSION_BASE::OnCorFilterChange ), NULL, this );
}

PANEL_CORROSION_BASE::~PANEL_CORROSION_BASE()
{
	// Disconnect Events
	m_corFilterCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CORROSION_BASE::OnCorFilterChange ), NULL, this );

}
