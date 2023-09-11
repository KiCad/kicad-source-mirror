///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_eseries_display_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_ESERIES_DISPLAY_BASE::PANEL_ESERIES_DISPLAY_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerESeries;
	bSizerESeries = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bTablesSizerESeries;
	bTablesSizerESeries = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbLowerSizerEseries2496;
	sbLowerSizerEseries2496 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("E24,E48,E96") ), wxVERTICAL );

	m_GridEseries2496 = new wxGrid( sbLowerSizerEseries2496->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_GridEseries2496->CreateGrid( 1, 1 );
	m_GridEseries2496->EnableEditing( false );
	m_GridEseries2496->EnableGridLines( false );
	m_GridEseries2496->EnableDragGridSize( false );
	m_GridEseries2496->SetMargins( 0, 0 );

	// Columns
	m_GridEseries2496->EnableDragColMove( false );
	m_GridEseries2496->EnableDragColSize( false );
	m_GridEseries2496->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_GridEseries2496->EnableDragRowSize( false );
	m_GridEseries2496->SetRowLabelSize( 0 );
	m_GridEseries2496->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_GridEseries2496->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
	sbLowerSizerEseries2496->Add( m_GridEseries2496, 0, wxALL, 5 );


	bTablesSizerESeries->Add( sbLowerSizerEseries2496, 0, 0, 5 );

	wxStaticBoxSizer* sbLowerSizerEseries112;
	sbLowerSizerEseries112 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("E1,E3,E6,E12") ), wxVERTICAL );

	m_GridEseries112 = new wxGrid( sbLowerSizerEseries112->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_GridEseries112->CreateGrid( 1, 1 );
	m_GridEseries112->EnableEditing( false );
	m_GridEseries112->EnableGridLines( false );
	m_GridEseries112->EnableDragGridSize( false );
	m_GridEseries112->SetMargins( 0, 0 );

	// Columns
	m_GridEseries112->EnableDragColMove( false );
	m_GridEseries112->EnableDragColSize( false );
	m_GridEseries112->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_GridEseries112->EnableDragRowSize( false );
	m_GridEseries112->SetRowLabelSize( 0 );
	m_GridEseries112->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_GridEseries112->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
	sbLowerSizerEseries112->Add( m_GridEseries112, 0, wxALL, 5 );


	bTablesSizerESeries->Add( sbLowerSizerEseries112, 1, 0, 5 );


	bSizerESeries->Add( bTablesSizerESeries, 0, wxTOP, 5 );

	m_panelESeriesHelp = new HTML_WINDOW( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	m_panelESeriesHelp->SetMinSize( wxSize( -1,100 ) );

	bSizerESeries->Add( m_panelESeriesHelp, 1, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );


	this->SetSizer( bSizerESeries );
	this->Layout();
	bSizerESeries->Fit( this );
}

PANEL_ESERIES_DISPLAY_BASE::~PANEL_ESERIES_DISPLAY_BASE()
{
}
