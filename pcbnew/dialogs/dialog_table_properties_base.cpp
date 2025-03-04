///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"
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


	bColumns->Add( m_gridSizer, 1, wxEXPAND|wxTOP|wxRIGHT, 5 );


	bColumns->Add( 15, 0, 0, wxEXPAND, 5 );

	wxGridBagSizer* bPropertiesSizer;
	bPropertiesSizer = new wxGridBagSizer( 3, 3 );
	bPropertiesSizer->SetFlexibleDirection( wxBOTH );
	bPropertiesSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	bPropertiesSizer->SetEmptyCellSize( wxSize( 0,2 ) );

	m_layerLabel = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_layerLabel->Wrap( -1 );
	bPropertiesSizer->Add( m_layerLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_LayerSelectionCtrl = new PCB_LAYER_BOX_SELECTOR( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_LayerSelectionCtrl->SetMinSize( wxSize( 175,-1 ) );

	bPropertiesSizer->Add( m_LayerSelectionCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_cbLocked = new wxCheckBox( this, wxID_ANY, _("Locked"), wxDefaultPosition, wxDefaultSize, 0 );
	bPropertiesSizer->Add( m_cbLocked, wxGBPosition( 1, 0 ), wxGBSpan( 1, 3 ), wxBOTTOM, 20 );

	m_borderCheckbox = new wxCheckBox( this, wxID_ANY, _("External border"), wxDefaultPosition, wxDefaultSize, 0 );
	bPropertiesSizer->Add( m_borderCheckbox, wxGBPosition( 2, 0 ), wxGBSpan( 1, 2 ), wxBOTTOM, 2 );

	m_headerBorder = new wxCheckBox( this, wxID_ANY, _("Header border"), wxDefaultPosition, wxDefaultSize, 0 );
	bPropertiesSizer->Add( m_headerBorder, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxLEFT, 20 );

	m_borderWidthLabel = new wxStaticText( this, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthLabel->Wrap( -1 );
	bPropertiesSizer->Add( m_borderWidthLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_borderWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer7->Add( m_borderWidthCtrl, 0, wxEXPAND, 5 );

	m_borderWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthUnits->Wrap( -1 );
	bSizer7->Add( m_borderWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );


	bPropertiesSizer->Add( bSizer7, wxGBPosition( 4, 1 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );

	m_borderStyleLabel = new wxStaticText( this, wxID_ANY, _("Style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderStyleLabel->Wrap( -1 );
	bPropertiesSizer->Add( m_borderStyleLabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_borderStyleCombo = new wxBitmapComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_borderStyleCombo->SetMinSize( wxSize( 200,-1 ) );

	bPropertiesSizer->Add( m_borderStyleCombo, wxGBPosition( 5, 1 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );


	bPropertiesSizer->Add( 0, 15, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_rowSeparators = new wxCheckBox( this, wxID_ANY, _("Row lines"), wxDefaultPosition, wxDefaultSize, 0 );
	bPropertiesSizer->Add( m_rowSeparators, wxGBPosition( 7, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 15 );

	m_colSeparators = new wxCheckBox( this, wxID_ANY, _("Column lines"), wxDefaultPosition, wxDefaultSize, 0 );
	bPropertiesSizer->Add( m_colSeparators, wxGBPosition( 7, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 20 );

	m_separatorsWidthLabel = new wxStaticText( this, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_separatorsWidthLabel->Wrap( -1 );
	bPropertiesSizer->Add( m_separatorsWidthLabel, wxGBPosition( 9, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer71;
	bSizer71 = new wxBoxSizer( wxHORIZONTAL );

	m_separatorsWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer71->Add( m_separatorsWidthCtrl, 0, wxEXPAND, 5 );

	m_separatorsWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_separatorsWidthUnits->Wrap( -1 );
	bSizer71->Add( m_separatorsWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );


	bPropertiesSizer->Add( bSizer71, wxGBPosition( 9, 1 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );

	m_separatorsStyleLabel = new wxStaticText( this, wxID_ANY, _("Style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_separatorsStyleLabel->Wrap( -1 );
	bPropertiesSizer->Add( m_separatorsStyleLabel, wxGBPosition( 10, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_separatorsStyleCombo = new wxBitmapComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_separatorsStyleCombo->SetMinSize( wxSize( 200,-1 ) );

	bPropertiesSizer->Add( m_separatorsStyleCombo, wxGBPosition( 10, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	bPropertiesSizer->AddGrowableCol( 1 );

	bColumns->Add( bPropertiesSizer, 0, wxEXPAND|wxALL, 10 );


	bMainSizer->Add( bColumns, 1, wxEXPAND|wxLEFT, 10 );

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
	m_headerBorder->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TABLE_PROPERTIES_BASE::onHeaderChecked ), NULL, this );
	m_rowSeparators->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TABLE_PROPERTIES_BASE::onBorderChecked ), NULL, this );
	m_colSeparators->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TABLE_PROPERTIES_BASE::onBorderChecked ), NULL, this );
}

DIALOG_TABLE_PROPERTIES_BASE::~DIALOG_TABLE_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_TABLE_PROPERTIES_BASE::onSize ) );
	m_borderCheckbox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TABLE_PROPERTIES_BASE::onBorderChecked ), NULL, this );
	m_headerBorder->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TABLE_PROPERTIES_BASE::onHeaderChecked ), NULL, this );
	m_rowSeparators->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TABLE_PROPERTIES_BASE::onBorderChecked ), NULL, this );
	m_colSeparators->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TABLE_PROPERTIES_BASE::onBorderChecked ), NULL, this );

}
