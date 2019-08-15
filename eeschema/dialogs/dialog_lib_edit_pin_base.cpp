///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug 15 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pin_shape_combobox.h"
#include "pin_type_combobox.h"
#include "wx/bmpcbox.h"

#include "dialog_lib_edit_pin_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LIB_EDIT_PIN_BASE::DIALOG_LIB_EDIT_PIN_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 5, 5 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_pinNameLabel = new wxStaticText( this, wxID_ANY, _("Pin &name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinNameLabel->Wrap( -1 );
	gbSizer1->Add( m_pinNameLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_textPinName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_textPinName, wxGBPosition( 0, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_pinNumberLabel = new wxStaticText( this, wxID_ANY, _("Pin num&ber:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinNumberLabel->Wrap( -1 );
	m_pinNumberLabel->SetToolTip( _("Pin number: 1 to 4 ASCII letters and/or digits") );

	gbSizer1->Add( m_pinNumberLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_textPinNumber = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_textPinNumber, wxGBPosition( 1, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_posXLabel = new wxStaticText( this, wxID_ANY, _("&X position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posXLabel->Wrap( -1 );
	gbSizer1->Add( m_posXLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_posXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_posXCtrl->SetMinSize( wxSize( 110,-1 ) );

	gbSizer1->Add( m_posXCtrl, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_posYLabel = new wxStaticText( this, wxID_ANY, _("&Y position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posYLabel->Wrap( -1 );
	gbSizer1->Add( m_posYLabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_posYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_posYCtrl, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_pinLengthLabel = new wxStaticText( this, wxID_ANY, _("&Pin length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinLengthLabel->Wrap( -1 );
	gbSizer1->Add( m_pinLengthLabel, wxGBPosition( 7, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_pinLengthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_pinLengthCtrl, wxGBPosition( 7, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_nameSizeLabel = new wxStaticText( this, wxID_ANY, _("N&ame text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_nameSizeLabel->Wrap( -1 );
	gbSizer1->Add( m_nameSizeLabel, wxGBPosition( 8, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_nameSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_nameSizeCtrl, wxGBPosition( 8, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_numberSizeLabel = new wxStaticText( this, wxID_ANY, _("Number text si&ze:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_numberSizeLabel->Wrap( -1 );
	gbSizer1->Add( m_numberSizeLabel, wxGBPosition( 9, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_numberSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_numberSizeCtrl, wxGBPosition( 9, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticTextEType = new wxStaticText( this, wxID_ANY, _("Electrical type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextEType->Wrap( -1 );
	m_staticTextEType->SetToolTip( _("Used by the ERC.") );

	gbSizer1->Add( m_staticTextEType, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_choiceElectricalType = new PinTypeComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN|wxCB_READONLY );
	gbSizer1->Add( m_choiceElectricalType, wxGBPosition( 2, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticTextGstyle = new wxStaticText( this, wxID_ANY, _("Graphic style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGstyle->Wrap( -1 );
	gbSizer1->Add( m_staticTextGstyle, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_choiceStyle = new PinShapeComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN|wxCB_READONLY );
	gbSizer1->Add( m_choiceStyle, wxGBPosition( 3, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticTextOrient = new wxStaticText( this, wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOrient->Wrap( -1 );
	gbSizer1->Add( m_staticTextOrient, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_choiceOrientation = new wxBitmapComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN|wxCB_READONLY );
	gbSizer1->Add( m_choiceOrientation, wxGBPosition( 6, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_posXUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posXUnits->Wrap( -1 );
	gbSizer1->Add( m_posXUnits, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_posYUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posYUnits->Wrap( -1 );
	gbSizer1->Add( m_posYUnits, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_pinLengthUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinLengthUnits->Wrap( -1 );
	gbSizer1->Add( m_pinLengthUnits, wxGBPosition( 7, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_nameSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_nameSizeUnits->Wrap( -1 );
	gbSizer1->Add( m_nameSizeUnits, wxGBPosition( 8, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_numberSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_numberSizeUnits->Wrap( -1 );
	gbSizer1->Add( m_numberSizeUnits, wxGBPosition( 9, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->AddGrowableCol( 1 );

	bLeftSizer->Add( gbSizer1, 1, wxEXPAND, 5 );


	bUpperSizer->Add( bLeftSizer, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* checkboxesSizer;
	checkboxesSizer = new wxBoxSizer( wxVERTICAL );

	m_checkApplyToAllParts = new wxCheckBox( this, wxID_ANY, _("Common to all &units in symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	checkboxesSizer->Add( m_checkApplyToAllParts, 0, wxBOTTOM, 3 );

	m_checkApplyToAllConversions = new wxCheckBox( this, wxID_ANY, _("Common to all body &styles (DeMorgan)"), wxDefaultPosition, wxDefaultSize, 0 );
	checkboxesSizer->Add( m_checkApplyToAllConversions, 0, wxBOTTOM, 3 );


	checkboxesSizer->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_checkShow = new wxCheckBox( this, wxID_ANY, _("&Visible"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShow->SetValue(true);
	checkboxesSizer->Add( m_checkShow, 0, wxBOTTOM, 3 );


	bRightSizer->Add( checkboxesSizer, 0, wxEXPAND|wxALL, 5 );

	m_panelShowPin = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxFULL_REPAINT_ON_RESIZE|wxTAB_TRAVERSAL );
	m_panelShowPin->SetMinSize( wxSize( 150,150 ) );

	bRightSizer->Add( m_panelShowPin, 1, wxEXPAND|wxLEFT, 5 );


	bUpperSizer->Add( bRightSizer, 1, wxEXPAND|wxALL, 5 );


	mainSizer->Add( bUpperSizer, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline1, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_sdbSizerButtons = new wxStdDialogButtonSizer();
	m_sdbSizerButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsOK );
	m_sdbSizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsCancel );
	m_sdbSizerButtons->Realize();

	mainSizer->Add( m_sdbSizerButtons, 0, wxALL|wxALIGN_RIGHT, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_textPinName->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_textPinNumber->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_pinLengthCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_nameSizeCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_numberSizeCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_choiceElectricalType->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_choiceStyle->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_choiceOrientation->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_checkApplyToAllParts->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_checkApplyToAllConversions->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_checkShow->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_panelShowPin->Connect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPaintShowPanel ), NULL, this );
}

DIALOG_LIB_EDIT_PIN_BASE::~DIALOG_LIB_EDIT_PIN_BASE()
{
	// Disconnect Events
	m_textPinName->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_textPinNumber->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_pinLengthCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_nameSizeCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_numberSizeCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_choiceElectricalType->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_choiceStyle->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_choiceOrientation->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_checkApplyToAllParts->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_checkApplyToAllConversions->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_checkShow->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_panelShowPin->Disconnect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPaintShowPanel ), NULL, this );

}
