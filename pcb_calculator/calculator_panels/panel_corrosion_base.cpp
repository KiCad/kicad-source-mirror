///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c5)
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

	m_staticText16 = new wxStaticText( this, wxID_ANY, _("When two metals are in contact, there will be a potential difference between them that will lead to corrosion.\nIn order to avoid corrosion, it is good practice to keep the difference below 300mV.\n\nOne of the metal will be anodic (+) and will be attacked. The other one will be cathodic and will be protected.\nIn the table below, if the number is positive then the row is anodic and the column is cathodic. \n\nYou can use an interface metal, just like with the ENIG surface finish that uses nickel as an interface between gold and copper to prevent corrosion."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	bSizer6->Add( m_staticText16, 0, wxALL, 5 );


	this->SetSizer( bSizer6 );
	this->Layout();
}

PANEL_CORROSION_BASE::~PANEL_CORROSION_BASE()
{
}
