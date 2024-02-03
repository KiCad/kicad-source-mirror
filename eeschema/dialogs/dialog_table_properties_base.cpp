///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/color_swatch.h"
#include "widgets/wx_infobar.h"

#include "dialog_table_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_TABLE_PROPERTIES_BASE::DIALOG_TABLE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_infoBar = new WX_INFOBAR( this );
	m_infoBar->SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
	m_infoBar->SetEffectDuration( 500 );
	m_infoBar->Hide();

	bMainSizer->Add( m_infoBar, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bColumns;
	bColumns = new wxBoxSizer( wxHORIZONTAL );

	m_gridSizer = new wxBoxSizer( wxVERTICAL );

	m_gridSizer->SetMinSize( wxSize( 600,400 ) );
	wxStaticText* cellContentsLabel;
	cellContentsLabel = new wxStaticText( this, wxID_ANY, _("Cell contents:"), wxDefaultPosition, wxDefaultSize, 0 );
	cellContentsLabel->Wrap( -1 );
	m_gridSizer->Add( cellContentsLabel, 0, wxTOP|wxBOTTOM, 3 );


	bColumns->Add( m_gridSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bColumns->Add( 10, 0, 0, wxEXPAND, 15 );

	wxBoxSizer* bPropertiesMargins;
	bPropertiesMargins = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* bPropertiesSizer;
	bPropertiesSizer = new wxGridBagSizer( 3, 3 );
	bPropertiesSizer->SetFlexibleDirection( wxBOTH );
	bPropertiesSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	bPropertiesSizer->SetEmptyCellSize( wxSize( 0,2 ) );

	m_borderCheckbox = new wxCheckBox( this, wxID_ANY, _("External border"), wxDefaultPosition, wxDefaultSize, 0 );
	bPropertiesSizer->Add( m_borderCheckbox, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), wxBOTTOM, 2 );

	m_headerBorder = new wxCheckBox( this, wxID_ANY, _("Header border"), wxDefaultPosition, wxDefaultSize, 0 );
	bPropertiesSizer->Add( m_headerBorder, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxLEFT, 20 );

	m_borderWidthLabel = new wxStaticText( this, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthLabel->Wrap( -1 );
	bPropertiesSizer->Add( m_borderWidthLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_borderWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer7->Add( m_borderWidthCtrl, 0, wxEXPAND, 5 );

	m_borderWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthUnits->Wrap( -1 );
	bSizer7->Add( m_borderWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );

	m_borderColorLabel = new wxStaticText( this, wxID_ANY, _("Color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderColorLabel->Wrap( -1 );
	bSizer7->Add( m_borderColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );


	bSizer7->Add( 5, 0, 0, 0, 5 );

	m_panelBorderColor = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	m_borderColorSwatch = new COLOR_SWATCH( m_panelBorderColor, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_borderColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panelBorderColor->SetSizer( bSizer2 );
	m_panelBorderColor->Layout();
	bSizer2->Fit( m_panelBorderColor );
	bSizer7->Add( m_panelBorderColor, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bPropertiesSizer->Add( bSizer7, wxGBPosition( 2, 1 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );

	m_borderStyleLabel = new wxStaticText( this, wxID_ANY, _("Style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderStyleLabel->Wrap( -1 );
	bPropertiesSizer->Add( m_borderStyleLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_borderStyleCombo = new wxBitmapComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_borderStyleCombo->SetMinSize( wxSize( 200,-1 ) );

	bPropertiesSizer->Add( m_borderStyleCombo, wxGBPosition( 3, 1 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );


	bPropertiesSizer->Add( 0, 20, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_rowSeparators = new wxCheckBox( this, wxID_ANY, _("Row lines"), wxDefaultPosition, wxDefaultSize, 0 );
	bPropertiesSizer->Add( m_rowSeparators, wxGBPosition( 5, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 15 );

	m_colSeparators = new wxCheckBox( this, wxID_ANY, _("Column lines"), wxDefaultPosition, wxDefaultSize, 0 );
	bPropertiesSizer->Add( m_colSeparators, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 20 );

	m_separatorsWidthLabel = new wxStaticText( this, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_separatorsWidthLabel->Wrap( -1 );
	bPropertiesSizer->Add( m_separatorsWidthLabel, wxGBPosition( 7, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer71;
	bSizer71 = new wxBoxSizer( wxHORIZONTAL );

	m_separatorsWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer71->Add( m_separatorsWidthCtrl, 0, wxEXPAND, 5 );

	m_separatorsWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_separatorsWidthUnits->Wrap( -1 );
	bSizer71->Add( m_separatorsWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );

	m_separatorsColorLabel = new wxStaticText( this, wxID_ANY, _("Color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_separatorsColorLabel->Wrap( -1 );
	bSizer71->Add( m_separatorsColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );


	bSizer71->Add( 5, 0, 0, 0, 5 );

	m_panelSeparatorsColor = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxVERTICAL );

	m_separatorsColorSwatch = new COLOR_SWATCH( m_panelSeparatorsColor, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer21->Add( m_separatorsColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panelSeparatorsColor->SetSizer( bSizer21 );
	m_panelSeparatorsColor->Layout();
	bSizer21->Fit( m_panelSeparatorsColor );
	bSizer71->Add( m_panelSeparatorsColor, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bPropertiesSizer->Add( bSizer71, wxGBPosition( 7, 1 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );

	m_separatorsStyleLabel = new wxStaticText( this, wxID_ANY, _("Style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_separatorsStyleLabel->Wrap( -1 );
	bPropertiesSizer->Add( m_separatorsStyleLabel, wxGBPosition( 8, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_separatorsStyleCombo = new wxBitmapComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_separatorsStyleCombo->SetMinSize( wxSize( 200,-1 ) );

	bPropertiesSizer->Add( m_separatorsStyleCombo, wxGBPosition( 8, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	bPropertiesSizer->AddGrowableCol( 1 );

	bPropertiesMargins->Add( bPropertiesSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bColumns->Add( bPropertiesMargins, 0, wxEXPAND|wxTOP, 5 );


	bMainSizer->Add( bColumns, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bButtons;
	bButtons = new wxBoxSizer( wxHORIZONTAL );


	bButtons->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bButtons->Add( m_sdbSizer1, 0, wxEXPAND|wxALL, 5 );


	bMainSizer->Add( bButtons, 0, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_TABLE_PROPERTIES_BASE::onSize ) );
	m_borderCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TABLE_PROPERTIES_BASE::onBorderChecked ), NULL, this );
	m_rowSeparators->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TABLE_PROPERTIES_BASE::onBorderChecked ), NULL, this );
	m_colSeparators->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TABLE_PROPERTIES_BASE::onBorderChecked ), NULL, this );
}

DIALOG_TABLE_PROPERTIES_BASE::~DIALOG_TABLE_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_TABLE_PROPERTIES_BASE::onSize ) );
	m_borderCheckbox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TABLE_PROPERTIES_BASE::onBorderChecked ), NULL, this );
	m_rowSeparators->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TABLE_PROPERTIES_BASE::onBorderChecked ), NULL, this );
	m_colSeparators->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TABLE_PROPERTIES_BASE::onBorderChecked ), NULL, this );

}
