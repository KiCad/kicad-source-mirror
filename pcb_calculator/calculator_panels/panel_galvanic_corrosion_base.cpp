///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-39-g3487c3cb)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_galvanic_corrosion_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_GALVANIC_CORROSION_BASE::PANEL_GALVANIC_CORROSION_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_scrolledWindow1 = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxTAB_TRAVERSAL|wxVSCROLL );
	m_scrolledWindow1->SetScrollRate( 5, 5 );
	wxBoxSizer* bSizerGrid;
	bSizerGrid = new wxBoxSizer( wxVERTICAL );

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
	bSizerGrid->Add( m_table, 0, 0, 5 );


	m_scrolledWindow1->SetSizer( bSizerGrid );
	m_scrolledWindow1->Layout();
	bSizerGrid->Fit( m_scrolledWindow1 );
	bSizerMain->Add( m_scrolledWindow1, 1, wxEXPAND|wxALL, 5 );

	m_helpText = new HTML_WINDOW( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	m_helpText->SetMinSize( wxSize( 400,110 ) );

	bSizerMain->Add( m_helpText, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerVoltage;
	bSizerVoltage = new wxBoxSizer( wxHORIZONTAL );

	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Threshold voltage:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bSizerVoltage->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxTOP, 5 );

	m_corFilterCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_corFilterCtrl->SetMinSize( wxSize( 100,-1 ) );

	bSizerVoltage->Add( m_corFilterCtrl, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	m_staticText3 = new wxStaticText( this, wxID_ANY, _("mV"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	bSizerVoltage->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bSizerBottom->Add( bSizerVoltage, 0, 0, 5 );

	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	m_staticline->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizerBottom->Add( m_staticline, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bSizerOpts;
	bSizerOpts = new wxBoxSizer( wxHORIZONTAL );

	m_stOpts = new wxStaticText( this, wxID_ANY, _("Material names:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stOpts->Wrap( -1 );
	bSizerOpts->Add( m_stOpts, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_radioBtnSymbol = new wxRadioButton( this, wxID_ANY, _("Chemical symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerOpts->Add( m_radioBtnSymbol, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_radioBtnName = new wxRadioButton( this, wxID_ANY, _("Names"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerOpts->Add( m_radioBtnName, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerBottom->Add( bSizerOpts, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bSizerMain->Add( bSizerBottom, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();

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
