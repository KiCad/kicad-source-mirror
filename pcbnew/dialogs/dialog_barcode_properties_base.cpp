///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Feb 20 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

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
    bGeneralSizer->Add( m_staticText28, 0, wxALL, 5 );

    m_textCtrl11 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
    bGeneralSizer->Add( m_textCtrl11, 0, wxALL|wxEXPAND, 5 );

    m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    bGeneralSizer->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );

    wxBoxSizer* bSizer9;
    bSizer9 = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer* m_LeftBoxSizer;
    m_LeftBoxSizer = new wxBoxSizer( wxVERTICAL );

    wxFlexGridSizer* fgSizerShapeType;
    fgSizerShapeType = new wxFlexGridSizer( 0, 3, 2, 0 );
    fgSizerShapeType->AddGrowableCol( 1 );
    fgSizerShapeType->SetFlexibleDirection( wxBOTH );
    fgSizerShapeType->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    m_staticText44 = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText44->Wrap( -1 );
    fgSizerShapeType->Add( m_staticText44, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 3 );

    wxString m_PadTypeChoices[] = { _("Through-hole"), _("SMD"), _("Edge Connector"), _("NPTH, Mechanical"), _("SMD Aperture") };
    int m_PadTypeNChoices = sizeof( m_PadTypeChoices ) / sizeof( wxString );
    m_PadType = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PadTypeNChoices, m_PadTypeChoices, 0 );
    m_PadType->SetSelection( 0 );
    fgSizerShapeType->Add( m_PadType, 0, wxEXPAND|wxALL, 3 );


    fgSizerShapeType->Add( 0, 0, 1, wxEXPAND, 5 );

    m_posXLabel = new wxStaticText( this, wxID_ANY, _("Position X:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_posXLabel->Wrap( -1 );
    fgSizerShapeType->Add( m_posXLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 3 );

    m_posXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    fgSizerShapeType->Add( m_posXCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );

    m_posXUnits = new wxStaticText( this, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
    m_posXUnits->Wrap( -1 );
    fgSizerShapeType->Add( m_posXUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 3 );

    m_posYLabel = new wxStaticText( this, wxID_ANY, _("Position Y:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_posYLabel->Wrap( -1 );
    fgSizerShapeType->Add( m_posYLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 3 );

    m_posYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    fgSizerShapeType->Add( m_posYCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );

    m_posYUnits = new wxStaticText( this, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
    m_posYUnits->Wrap( -1 );
    fgSizerShapeType->Add( m_posYUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 3 );

    m_sizeXLabel = new wxStaticText( this, wxID_ANY, _("Size X:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_sizeXLabel->Wrap( -1 );
    fgSizerShapeType->Add( m_sizeXLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 3 );

    m_sizeXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    fgSizerShapeType->Add( m_sizeXCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );

    m_sizeXUnits = new wxStaticText( this, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
    m_sizeXUnits->Wrap( -1 );
    fgSizerShapeType->Add( m_sizeXUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 3 );

    m_sizeYLabel = new wxStaticText( this, wxID_ANY, _("Size Y:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_sizeYLabel->Wrap( -1 );
    fgSizerShapeType->Add( m_sizeYLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 3 );

    m_sizeYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    fgSizerShapeType->Add( m_sizeYCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 3 );

    m_sizeYUnits = new wxStaticText( this, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
    m_sizeYUnits->Wrap( -1 );
    fgSizerShapeType->Add( m_sizeYUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 3 );

    m_PadOrientText = new wxStaticText( this, wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_PadOrientText->Wrap( -1 );
    fgSizerShapeType->Add( m_PadOrientText, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 3 );

    m_orientation = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
    m_orientation->Append( _("0") );
    m_orientation->Append( _("90") );
    m_orientation->Append( _("-90") );
    m_orientation->Append( _("180") );
    fgSizerShapeType->Add( m_orientation, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 3 );

    m_staticText491 = new wxStaticText( this, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText491->Wrap( -1 );
    fgSizerShapeType->Add( m_staticText491, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 3 );

    m_offsetXLabel = new wxStaticText( this, wxID_ANY, _("Margin X:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_offsetXLabel->Wrap( -1 );
    fgSizerShapeType->Add( m_offsetXLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );

    m_offsetXCtrl = new wxTextCtrl( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
    fgSizerShapeType->Add( m_offsetXCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 3 );

    m_offsetXUnits = new wxStaticText( this, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
    m_offsetXUnits->Wrap( -1 );
    fgSizerShapeType->Add( m_offsetXUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 3 );

    m_offsetYLabel = new wxStaticText( this, wxID_ANY, _("Margin Y:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_offsetYLabel->Wrap( -1 );
    fgSizerShapeType->Add( m_offsetYLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 3 );

    m_offsetYCtrl = new wxTextCtrl( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
    fgSizerShapeType->Add( m_offsetYCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );

    m_offsetYUnits = new wxStaticText( this, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
    m_offsetYUnits->Wrap( -1 );
    fgSizerShapeType->Add( m_offsetYUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 3 );


    m_LeftBoxSizer->Add( fgSizerShapeType, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );


    bSizer9->Add( m_LeftBoxSizer, 0, wxALL|wxEXPAND, 5 );

    m_middleBoxSizer = new wxBoxSizer( wxVERTICAL );

    m_checkBox1 = new wxCheckBox( this, wxID_ANY, _("Show Text"), wxDefaultPosition, wxDefaultSize, 0 );
    m_middleBoxSizer->Add( m_checkBox1, 0, wxALL, 5 );

    wxString m_barcodeChoices[] = { _("Code 39 (ISO 16388)"), _("Code 128 (ISO 15417)"), _("QR Code (ISO 18004)"), _("Micro QR Code") };
    int m_barcodeNChoices = sizeof( m_barcodeChoices ) / sizeof( wxString );
    m_barcode = new wxRadioBox( this, wxID_ANY, _("Code"), wxDefaultPosition, wxDefaultSize, m_barcodeNChoices, m_barcodeChoices, 1, wxRA_SPECIFY_COLS );
    m_barcode->SetSelection( 0 );
    m_middleBoxSizer->Add( m_barcode, 0, wxALL|wxEXPAND, 5 );

    wxString m_errorCorrectionChoices[] = { _("~20% (Level L)"), _("~37% (Level M)"), _("~55% (Level Q)"), _("~65% (Level H)") };
    int m_errorCorrectionNChoices = sizeof( m_errorCorrectionChoices ) / sizeof( wxString );
    m_errorCorrection = new wxRadioBox( this, wxID_ANY, _("Error Correction"), wxDefaultPosition, wxDefaultSize, m_errorCorrectionNChoices, m_errorCorrectionChoices, 1, wxRA_SPECIFY_COLS );
    m_errorCorrection->SetSelection( 1 );
    m_middleBoxSizer->Add( m_errorCorrection, 0, wxALL|wxEXPAND, 5 );


    bSizer9->Add( m_middleBoxSizer, 0, wxEXPAND|wxALL, 3 );


    bGeneralSizer->Add( bSizer9, 1, wxEXPAND, 5 );


    bSizerUpper->Add( bGeneralSizer, 1, wxEXPAND, 5 );

    wxBoxSizer* bSizerDisplayPad;
    bSizerDisplayPad = new wxBoxSizer( wxVERTICAL );


    bSizerDisplayPad->Add( 0, 0, 1, wxEXPAND, 5 );

    m_panelShowBarcodeGal = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), wxDefaultSize, m_galOptions, EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO);
    m_panelShowBarcodeGal->SetMinSize( wxSize( 280,-1 ) );

    bSizerDisplayPad->Add( m_panelShowBarcodeGal, 12, wxEXPAND|wxALL, 5 );


    bSizerDisplayPad->Add( 0, 0, 1, wxEXPAND, 5 );


    bSizerUpper->Add( bSizerDisplayPad, 1, wxEXPAND|wxTOP|wxRIGHT, 10 );


    m_MainSizer->Add( bSizerUpper, 1, wxEXPAND, 5 );

    m_staticline13 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    m_MainSizer->Add( m_staticline13, 0, wxEXPAND|wxTOP|wxBOTTOM, 10 );

    m_sdbSizer = new wxStdDialogButtonSizer();
    m_sdbSizerOK = new wxButton( this, wxID_OK );
    m_sdbSizer->AddButton( m_sdbSizerOK );
    m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
    m_sdbSizer->AddButton( m_sdbSizerCancel );
    m_sdbSizer->Realize();

    m_MainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


    this->SetSizer( m_MainSizer );
    this->Layout();

    this->Centre( wxBOTH );

    // Connect Events
    this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnInitDialog ) );
    m_PadType->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::PadTypeSelected ), NULL, this );
    m_sizeXCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
    m_sizeYCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
    m_orientation->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::PadOrientEvent ), NULL, this );
    m_orientation->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::PadOrientEvent ), NULL, this );
    m_offsetXCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
    m_offsetYCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
    m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnCancel ), NULL, this );
}

DIALOG_BARCODE_PROPERTIES_BASE::~DIALOG_BARCODE_PROPERTIES_BASE()
{
    // Disconnect Events
    this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnInitDialog ) );
    m_PadType->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::PadTypeSelected ), NULL, this );
    m_sizeXCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
    m_sizeYCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
    m_orientation->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::PadOrientEvent ), NULL, this );
    m_orientation->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::PadOrientEvent ), NULL, this );
    m_offsetXCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
    m_offsetYCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
    m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BARCODE_PROPERTIES_BASE::OnCancel ), NULL, this );

}
