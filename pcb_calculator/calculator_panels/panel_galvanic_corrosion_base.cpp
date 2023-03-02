///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_galvanic_corrosion_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_GALVANIC_CORROSION_BASE::PANEL_GALVANIC_CORROSION_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	m_scrolledWindow1 = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxTAB_TRAVERSAL|wxVSCROLL );
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
	m_table->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_BOTTOM );
	bSizer7->Add( m_table, 0, 0, 5 );


	m_scrolledWindow1->SetSizer( bSizer7 );
	m_scrolledWindow1->Layout();
	bSizer7->Fit( m_scrolledWindow1 );
	bSizer6->Add( m_scrolledWindow1, 1, wxEXPAND|wxALL, 5 );

	m_helpText = new HTML_WINDOW( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	m_helpText->SetMinSize( wxSize( 400,110 ) );

	bSizer6->Add( m_helpText, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Threshold voltage:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bSizer3->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxTOP, 5 );

	m_corFilterCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_corFilterCtrl->SetMinSize( wxSize( 100,-1 ) );

	bSizer3->Add( m_corFilterCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_staticText3 = new wxStaticText( this, wxID_ANY, _("mV"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	bSizer3->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	m_staticline1->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer3->Add( m_staticline1, 0, wxEXPAND, 5 );


	bSizer5->Add( bSizer3, 0, 0, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	m_radioBtnSymbol = new wxRadioButton( this, wxID_ANY, _("Symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_radioBtnSymbol, 0, 0, 5 );

	m_radioBtnName = new wxRadioButton( this, wxID_ANY, _("Names"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_radioBtnName, 0, 0, 5 );


	bSizer5->Add( bSizer4, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bSizer6->Add( bSizer5, 0, wxEXPAND, 5 );


	this->SetSizer( bSizer6 );
	this->Layout();
	bSizer6->Fit( this );

	// Connect Events
	m_corFilterCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_GALVANIC_CORROSION_BASE::OnCorFilterChange ), NULL, this );
	m_radioBtnSymbol->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_GALVANIC_CORROSION_BASE::OnNomenclatureChange ), NULL, this );
	m_radioBtnName->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_GALVANIC_CORROSION_BASE::OnNomenclatureChange ), NULL, this );
}

PANEL_GALVANIC_CORROSION_BASE::~PANEL_GALVANIC_CORROSION_BASE()
{
	// Disconnect Events
	m_corFilterCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_GALVANIC_CORROSION_BASE::OnCorFilterChange ), NULL, this );
	m_radioBtnSymbol->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_GALVANIC_CORROSION_BASE::OnNomenclatureChange ), NULL, this );
	m_radioBtnName->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_GALVANIC_CORROSION_BASE::OnNomenclatureChange ), NULL, this );

}
