///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

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
	
	wxFlexGridSizer* fgSizerPins;
	fgSizerPins = new wxFlexGridSizer( 5, 2, 0, 0 );
	fgSizerPins->AddGrowableCol( 1 );
	fgSizerPins->SetFlexibleDirection( wxBOTH );
	fgSizerPins->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );
	
	m_staticTextPinName = new wxStaticText( this, wxID_ANY, _("Pin &name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPinName->Wrap( -1 );
	fgSizerPins->Add( m_staticTextPinName, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textPinName = new wxTextCtrl( this, ID_M_TEXTPINNAME, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textPinName->SetMaxLength( 0 ); 
	fgSizerPins->Add( m_textPinName, 0, wxEXPAND|wxTOP|wxBOTTOM, 3 );
	
	m_staticTextPadName = new wxStaticText( this, ID_M_STATICTEXTPADNAME, _("Pin n&umber:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPadName->Wrap( -1 );
	m_staticTextPadName->SetToolTip( _("Pin number: 1 to 4 ASCII letters and/or digits") );
	
	fgSizerPins->Add( m_staticTextPadName, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textPadName = new wxTextCtrl( this, ID_M_TEXTPADNAME, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textPadName->SetMaxLength( 0 ); 
	fgSizerPins->Add( m_textPadName, 0, wxEXPAND|wxTOP|wxBOTTOM, 3 );
	
	m_staticTextOrient = new wxStaticText( this, wxID_ANY, _("&Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOrient->Wrap( -1 );
	fgSizerPins->Add( m_staticTextOrient, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_choiceOrientation = new wxBitmapComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY ); 
	fgSizerPins->Add( m_choiceOrientation, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_staticTextEType = new wxStaticText( this, wxID_ANY, _("&Electrical type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextEType->Wrap( -1 );
	m_staticTextEType->SetToolTip( _("Used by the ERC.") );
	
	fgSizerPins->Add( m_staticTextEType, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_choiceElectricalType = new wxBitmapComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY ); 
	fgSizerPins->Add( m_choiceElectricalType, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_staticTextGstyle = new wxStaticText( this, wxID_ANY, _("Graphic &Style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGstyle->Wrap( -1 );
	fgSizerPins->Add( m_staticTextGstyle, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_choiceStyle = new wxBitmapComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY ); 
	fgSizerPins->Add( m_choiceStyle, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	
	bLeftSizer->Add( fgSizerPins, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* boarderSizer;
	boarderSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizerPinSharing;
	sbSizerPinSharing = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Sharing") ), wxVERTICAL );
	
	m_checkApplyToAllParts = new wxCheckBox( this, wxID_ANY, _("Common to all &units in component"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerPinSharing->Add( m_checkApplyToAllParts, 0, wxALL, 3 );
	
	m_checkApplyToAllConversions = new wxCheckBox( this, wxID_ANY, _("Common to all body &styles (DeMorgan)"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerPinSharing->Add( m_checkApplyToAllConversions, 0, wxALL, 3 );
	
	
	boarderSizer->Add( sbSizerPinSharing, 0, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* sbSizerSchematicProperties;
	sbSizerSchematicProperties = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Schematic Properties") ), wxVERTICAL );
	
	m_checkShow = new wxCheckBox( this, wxID_ANY, _("&Visible"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShow->SetValue(true); 
	sbSizerSchematicProperties->Add( m_checkShow, 0, wxALL, 3 );
	
	
	boarderSizer->Add( sbSizerSchematicProperties, 0, wxEXPAND|wxALL, 5 );
	
	
	bLeftSizer->Add( boarderSizer, 0, wxEXPAND|wxTOP|wxBOTTOM, 12 );
	
	
	bUpperSizer->Add( bLeftSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizerTextsSizes;
	fgSizerTextsSizes = new wxFlexGridSizer( 3, 3, 0, 0 );
	fgSizerTextsSizes->AddGrowableCol( 1 );
	fgSizerTextsSizes->SetFlexibleDirection( wxBOTH );
	fgSizerTextsSizes->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );
	
	m_staticTextNameSize = new wxStaticText( this, ID_M_STATICTEXTNAMESIZE, _("N&ame text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNameSize->Wrap( -1 );
	fgSizerTextsSizes->Add( m_staticTextNameSize, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textPinNameTextSize = new wxTextCtrl( this, ID_M_TEXTPINNAMETEXTSIZE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textPinNameTextSize->SetMaxLength( 0 ); 
	fgSizerTextsSizes->Add( m_textPinNameTextSize, 1, wxEXPAND|wxTOP|wxBOTTOM, 3 );
	
	m_staticNameTextSizeUnits = new wxStaticText( this, ID_M_STATICNAMETEXTSIZEUNITS, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticNameTextSizeUnits->Wrap( -1 );
	fgSizerTextsSizes->Add( m_staticNameTextSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticTextPadNameSize = new wxStaticText( this, ID_M_STATICTEXTPADNAMESIZE, _("Number te&xt size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPadNameSize->Wrap( -1 );
	fgSizerTextsSizes->Add( m_staticTextPadNameSize, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textPadNameTextSize = new wxTextCtrl( this, ID_M_TEXTPADNAMETEXTSIZE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textPadNameTextSize->SetMaxLength( 0 ); 
	fgSizerTextsSizes->Add( m_textPadNameTextSize, 0, wxTOP|wxBOTTOM|wxEXPAND, 3 );
	
	m_staticNumberTextSizeUnits = new wxStaticText( this, ID_M_STATICNUMBERTEXTSIZEUNITS, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticNumberTextSizeUnits->Wrap( -1 );
	fgSizerTextsSizes->Add( m_staticNumberTextSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticTextPinLen = new wxStaticText( this, ID_M_STATICTEXTPINLEN, _("&Length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPinLen->Wrap( -1 );
	fgSizerTextsSizes->Add( m_staticTextPinLen, 0, wxALL, 5 );
	
	m_textLength = new wxTextCtrl( this, ID_M_TEXTLENGTH, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textLength->SetMaxLength( 0 ); 
	fgSizerTextsSizes->Add( m_textLength, 0, wxTOP|wxBOTTOM|wxEXPAND, 5 );
	
	m_staticLengthUnits = new wxStaticText( this, ID_M_STATICLENGTHUNITS, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticLengthUnits->Wrap( -1 );
	fgSizerTextsSizes->Add( m_staticLengthUnits, 0, wxALL, 5 );
	
	
	bRightSizer->Add( fgSizerTextsSizes, 0, wxALL|wxEXPAND, 5 );
	
	m_panelShowPin = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE|wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
	m_panelShowPin->SetMinSize( wxSize( 150,150 ) );
	
	bRightSizer->Add( m_panelShowPin, 1, wxEXPAND | wxALL, 5 );
	
	
	bUpperSizer->Add( bRightSizer, 1, wxEXPAND|wxRIGHT, 5 );
	
	
	mainSizer->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizerButtons = new wxStdDialogButtonSizer();
	m_sdbSizerButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsOK );
	m_sdbSizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsCancel );
	m_sdbSizerButtons->Realize();
	
	mainSizer->Add( m_sdbSizerButtons, 0, wxALL|wxALIGN_RIGHT, 5 );
	
	
	this->SetSizer( mainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnCloseDialog ) );
	m_textPinName->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_textPadName->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_choiceOrientation->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_choiceElectricalType->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_choiceStyle->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_checkApplyToAllParts->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_checkApplyToAllConversions->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_checkShow->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_textPinNameTextSize->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_textPadNameTextSize->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_textLength->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_panelShowPin->Connect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPaintShowPanel ), NULL, this );
	m_sdbSizerButtonsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnCancelButtonClick ), NULL, this );
	m_sdbSizerButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnOKButtonClick ), NULL, this );
}

DIALOG_LIB_EDIT_PIN_BASE::~DIALOG_LIB_EDIT_PIN_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnCloseDialog ) );
	m_textPinName->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_textPadName->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_choiceOrientation->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_choiceElectricalType->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_choiceStyle->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_checkApplyToAllParts->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_checkApplyToAllConversions->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_checkShow->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_textPinNameTextSize->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_textPadNameTextSize->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_textLength->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPropertiesChange ), NULL, this );
	m_panelShowPin->Disconnect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnPaintShowPanel ), NULL, this );
	m_sdbSizerButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnCancelButtonClick ), NULL, this );
	m_sdbSizerButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_BASE::OnOKButtonClick ), NULL, this );
	
}
