///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "panel_modedit_defaults_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_MODEDIT_DEFAULTS_BASE::PANEL_MODEDIT_DEFAULTS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerMargins;
	bSizerMargins = new wxBoxSizer( wxVERTICAL );
	
	m_staticText13 = new wxStaticText( this, wxID_ANY, _("Default values for new footprints:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText13->Wrap( -1 );
	bSizerMargins->Add( m_staticText13, 0, wxTOP|wxLEFT, 5 );
	
	wxFlexGridSizer* defaultValuesSizer;
	defaultValuesSizer = new wxFlexGridSizer( 0, 4, 5, 5 );
	defaultValuesSizer->AddGrowableCol( 1 );
	defaultValuesSizer->SetFlexibleDirection( wxBOTH );
	defaultValuesSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextRef = new wxStaticText( this, wxID_ANY, _("&Reference:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRef->Wrap( -1 );
	defaultValuesSizer->Add( m_staticTextRef, 0, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );
	
	m_textCtrlRefText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlRefText->SetToolTip( _("Default text for reference\nLeave blank to use the footprint name") );
	
	defaultValuesSizer->Add( m_textCtrlRefText, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxRIGHT, 5 );
	
	wxString m_choiceLayerReferenceChoices[] = { _("SilkScreen"), _("Fab. Layer") };
	int m_choiceLayerReferenceNChoices = sizeof( m_choiceLayerReferenceChoices ) / sizeof( wxString );
	m_choiceLayerReference = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceLayerReferenceNChoices, m_choiceLayerReferenceChoices, 0 );
	m_choiceLayerReference->SetSelection( 0 );
	defaultValuesSizer->Add( m_choiceLayerReference, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );
	
	wxString m_choiceVisibleReferenceChoices[] = { _("Visible"), _("Invisible") };
	int m_choiceVisibleReferenceNChoices = sizeof( m_choiceVisibleReferenceChoices ) / sizeof( wxString );
	m_choiceVisibleReference = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceVisibleReferenceNChoices, m_choiceVisibleReferenceChoices, 0 );
	m_choiceVisibleReference->SetSelection( 0 );
	defaultValuesSizer->Add( m_choiceVisibleReference, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );
	
	m_staticTextValue = new wxStaticText( this, wxID_ANY, _("V&alue:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextValue->Wrap( -1 );
	defaultValuesSizer->Add( m_staticTextValue, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlValueText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlValueText->SetToolTip( _("Default text for value\nLeave blank to use the footprint name") );
	m_textCtrlValueText->SetMinSize( wxSize( 160,-1 ) );
	
	defaultValuesSizer->Add( m_textCtrlValueText, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );
	
	wxString m_choiceLayerValueChoices[] = { _("SilkScreen"), _("Fab. Layer") };
	int m_choiceLayerValueNChoices = sizeof( m_choiceLayerValueChoices ) / sizeof( wxString );
	m_choiceLayerValue = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceLayerValueNChoices, m_choiceLayerValueChoices, 0 );
	m_choiceLayerValue->SetSelection( 1 );
	defaultValuesSizer->Add( m_choiceLayerValue, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	wxString m_choiceVisibleValueChoices[] = { _("Visible"), _("Invisible") };
	int m_choiceVisibleValueNChoices = sizeof( m_choiceVisibleValueChoices ) / sizeof( wxString );
	m_choiceVisibleValue = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceVisibleValueNChoices, m_choiceVisibleValueChoices, 0 );
	m_choiceVisibleValue->SetSelection( 0 );
	defaultValuesSizer->Add( m_choiceVisibleValue, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	
	bSizerMargins->Add( defaultValuesSizer, 0, wxEXPAND|wxLEFT, 25 );
	
	
	bSizerMargins->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_staticTextInfo = new wxStaticText( this, wxID_ANY, _("Leave reference and/or value blank to use footprint name."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfo->Wrap( -1 );
	m_staticTextInfo->SetFont( wxFont( 12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	bSizerMargins->Add( m_staticTextInfo, 0, wxBOTTOM|wxLEFT, 25 );
	
	
	bSizerMargins->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 10 );
	
	wxBoxSizer* defaultSizesSizer1;
	defaultSizesSizer1 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Default properties for new graphic items:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	defaultSizesSizer1->Add( m_staticText1, 0, wxBOTTOM|wxRIGHT, 5 );
	
	m_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	
	// Grid
	m_grid->CreateGrid( 5, 5 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );
	
	// Columns
	m_grid->SetColSize( 0, 110 );
	m_grid->SetColSize( 1, 100 );
	m_grid->SetColSize( 2, 100 );
	m_grid->SetColSize( 3, 100 );
	m_grid->SetColSize( 4, 60 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelSize( 22 );
	m_grid->SetColLabelValue( 0, _("Line Thickness") );
	m_grid->SetColLabelValue( 1, _("Text Width") );
	m_grid->SetColLabelValue( 2, _("Text Height") );
	m_grid->SetColLabelValue( 3, _("Text Thickness") );
	m_grid->SetColLabelValue( 4, _("Italic") );
	m_grid->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_grid->EnableDragRowSize( false );
	m_grid->SetRowLabelSize( 125 );
	m_grid->SetRowLabelValue( 0, _("Silk Layers") );
	m_grid->SetRowLabelValue( 1, _("Copper Layers") );
	m_grid->SetRowLabelValue( 2, _("Edge Cuts") );
	m_grid->SetRowLabelValue( 3, _("Courtyards") );
	m_grid->SetRowLabelValue( 4, _("Other Layers") );
	m_grid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_grid->SetToolTip( _("Net Class parameters") );
	
	defaultSizesSizer1->Add( m_grid, 1, wxBOTTOM|wxLEFT, 20 );
	
	
	bSizerMargins->Add( defaultSizesSizer1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerMain->Add( bSizerMargins, 1, wxRIGHT|wxLEFT, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
}

PANEL_MODEDIT_DEFAULTS_BASE::~PANEL_MODEDIT_DEFAULTS_BASE()
{
}
