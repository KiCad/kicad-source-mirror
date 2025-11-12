///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"

#include "dialog_barcode_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_BARCODE_PROPERTIES_BASE::DIALOG_BARCODE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* m_MainSizer;
	m_MainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bGeneralSizer;
	bGeneralSizer = new wxBoxSizer( wxVERTICAL );

	m_staticText28 = new wxStaticText( this, wxID_ANY, _("Text:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText28->Wrap( -1 );
	bGeneralSizer->Add( m_staticText28, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	bGeneralSizer->Add( 0, 3, 0, wxEXPAND, 5 );

	m_textInput = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bGeneralSizer->Add( m_textInput, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* m_LeftBoxSizer;
	m_LeftBoxSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerCheckboxes;
	bSizerCheckboxes = new wxBoxSizer( wxHORIZONTAL );

	m_cbLocked = new wxCheckBox( this, wxID_ANY, _("Locked"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerCheckboxes->Add( m_cbLocked, 1, 0, 5 );

	m_cbKnockout = new wxCheckBox( this, wxID_ANY, _("Knockout"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbKnockout->SetToolTip( _("If checked, the barcode will be inverted with a margin around it") );

	bSizerCheckboxes->Add( m_cbKnockout, 1, wxLEFT, 10 );

	m_cbShowText = new wxCheckBox( this, wxID_ANY, _("Show text"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerCheckboxes->Add( m_cbShowText, 1, wxLEFT, 25 );


	m_LeftBoxSizer->Add( bSizerCheckboxes, 1, wxALL|wxEXPAND, 5 );


	bSizer9->Add( m_LeftBoxSizer, 0, wxEXPAND|wxTOP|wxRIGHT, 5 );

	m_middleBoxSizer = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 3, 3 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( 20,10 ) );

	m_layerLabel = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_layerLabel->Wrap( -1 );
	gbSizer1->Add( m_layerLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP, 6 );

	m_cbLayer = new PCB_LAYER_BOX_SELECTOR( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	gbSizer1->Add( m_cbLayer, wxGBPosition( 0, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxRIGHT, 6 );

	m_posXLabel = new wxStaticText( this, wxID_ANY, _("Position X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posXLabel->Wrap( -1 );
	gbSizer1->Add( m_posXLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_posXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_posXCtrl->SetMinSize( wxSize( 155,-1 ) );

	gbSizer1->Add( m_posXCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_posXUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posXUnits->Wrap( -1 );
	gbSizer1->Add( m_posXUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_posYLabel = new wxStaticText( this, wxID_ANY, _("Position Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posYLabel->Wrap( -1 );
	gbSizer1->Add( m_posYLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_posYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_posYCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_posYUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posYUnits->Wrap( -1 );
	gbSizer1->Add( m_posYUnits, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_sizeXLabel = new wxStaticText( this, wxID_ANY, _("Size X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizeXLabel->Wrap( -1 );
	gbSizer1->Add( m_sizeXLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_sizeXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_sizeXCtrl, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_sizeXUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizeXUnits->Wrap( -1 );
	gbSizer1->Add( m_sizeXUnits, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_sizeYLabel = new wxStaticText( this, wxID_ANY, _("Size Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizeYLabel->Wrap( -1 );
	gbSizer1->Add( m_sizeYLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_sizeYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_sizeYCtrl, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_sizeYUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizeYUnits->Wrap( -1 );
	gbSizer1->Add( m_sizeYUnits, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_orientationLabel = new wxStaticText( this, wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_orientationLabel->Wrap( -1 );
	gbSizer1->Add( m_orientationLabel, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_orientationCtrl = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_orientationCtrl->Append( _("0") );
	m_orientationCtrl->Append( _("90") );
	m_orientationCtrl->Append( _("-90") );
	m_orientationCtrl->Append( _("180") );
	gbSizer1->Add( m_orientationCtrl, wxGBPosition( 6, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_orientationUnits = new wxStaticText( this, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_orientationUnits->Wrap( -1 );
	gbSizer1->Add( m_orientationUnits, wxGBPosition( 6, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_textSizeLabel = new wxStaticText( this, wxID_ANY, _("Text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeLabel->Wrap( -1 );
	gbSizer1->Add( m_textSizeLabel, wxGBPosition( 8, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_textSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_textSizeCtrl, wxGBPosition( 8, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_textSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	gbSizer1->Add( m_textSizeUnits, wxGBPosition( 8, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_marginXLabel = new wxStaticText( this, wxID_ANY, _("Min margin X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_marginXLabel->Wrap( -1 );
	gbSizer1->Add( m_marginXLabel, wxGBPosition( 10, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_marginXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_marginXCtrl, wxGBPosition( 10, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_marginXUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_marginXUnits->Wrap( -1 );
	gbSizer1->Add( m_marginXUnits, wxGBPosition( 10, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_marginYLabel = new wxStaticText( this, wxID_ANY, _("Min margin Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_marginYLabel->Wrap( -1 );
	gbSizer1->Add( m_marginYLabel, wxGBPosition( 11, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_marginYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_marginYCtrl, wxGBPosition( 11, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_marginYUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_marginYUnits->Wrap( -1 );
	gbSizer1->Add( m_marginYUnits, wxGBPosition( 11, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxString m_barcodeChoices[] = { _("Code 39 (ISO 16388)"), _("Code 128 (ISO 15417)"), _("Data Matrix (ECC 200)"), _("QR Code (ISO 18004)"), _("Micro QR Code") };
	int m_barcodeNChoices = sizeof( m_barcodeChoices ) / sizeof( wxString );
	m_barcode = new wxRadioBox( this, wxID_ANY, _("Code"), wxDefaultPosition, wxDefaultSize, m_barcodeNChoices, m_barcodeChoices, 1, wxRA_SPECIFY_COLS );
	m_barcode->SetSelection( 2 );
	gbSizer1->Add( m_barcode, wxGBPosition( 0, 4 ), wxGBSpan( 6, 1 ), wxALL|wxEXPAND, 5 );

	wxString m_errorCorrectionChoices[] = { _("~20% (Level L)"), _("~37% (Level M)"), _("~55% (Level Q)"), _("~65% (Level H)") };
	int m_errorCorrectionNChoices = sizeof( m_errorCorrectionChoices ) / sizeof( wxString );
	m_errorCorrection = new wxRadioBox( this, wxID_ANY, _("Error Correction"), wxDefaultPosition, wxDefaultSize, m_errorCorrectionNChoices, m_errorCorrectionChoices, 1, wxRA_SPECIFY_COLS );
	m_errorCorrection->SetSelection( 2 );
	gbSizer1->Add( m_errorCorrection, wxGBPosition( 6, 4 ), wxGBSpan( 6, 1 ), wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	gbSizer1->AddGrowableCol( 1 );

	m_middleBoxSizer->Add( gbSizer1, 1, wxEXPAND|wxBOTTOM|wxLEFT, 5 );


	bSizer9->Add( m_middleBoxSizer, 0, wxALL|wxEXPAND, 5 );


	bSizer8->Add( bSizer9, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerDisplayPad;
	bSizerDisplayPad = new wxBoxSizer( wxVERTICAL );

	m_panelShowBarcodeGal = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), wxDefaultSize, m_galOptions, EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO);
	m_panelShowBarcodeGal->SetMinSize( wxSize( 280,-1 ) );

	bSizerDisplayPad->Add( m_panelShowBarcodeGal, 12, wxEXPAND|wxALL, 5 );


	bSizer8->Add( bSizerDisplayPad, 1, wxEXPAND|wxLEFT, 10 );


	bGeneralSizer->Add( bSizer8, 1, wxEXPAND, 5 );


	bSizerUpper->Add( bGeneralSizer, 1, wxEXPAND|wxRIGHT, 5 );


	m_MainSizer->Add( bSizerUpper, 1, wxEXPAND|wxTOP|wxLEFT, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_MainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnUpdateUI ) );
	m_textInput->Connect( wxEVT_KEY_UP, wxKeyEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnTextValueChanged ), NULL, this );
	m_cbKnockout->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbShowText->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbLayer->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_sizeXCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_sizeYCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_orientationCtrl->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_orientationCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_textSizeCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_marginXCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_marginYCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_barcode->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_errorCorrection->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnCancel ), NULL, this );
}

DIALOG_BARCODE_PROPERTIES_BASE::~DIALOG_BARCODE_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnUpdateUI ) );
	m_textInput->Disconnect( wxEVT_KEY_UP, wxKeyEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnTextValueChanged ), NULL, this );
	m_cbKnockout->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbShowText->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbLayer->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_sizeXCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_sizeYCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_orientationCtrl->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_orientationCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_textSizeCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_marginXCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_marginYCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_barcode->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_errorCorrection->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnCancel ), NULL, this );

}
