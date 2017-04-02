///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Feb 19 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_edit_component_in_schematic_fbp.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* upperSizer;
	upperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* optionsSizer;
	optionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxEmptyString ), wxVERTICAL );
	
	m_staticTextUnit = new wxStaticText( optionsSizer->GetStaticBox(), wxID_ANY, _("Unit:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUnit->Wrap( -1 );
	optionsSizer->Add( m_staticTextUnit, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxArrayString unitChoiceChoices;
	unitChoice = new wxChoice( optionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, unitChoiceChoices, 0 );
	unitChoice->SetSelection( 0 );
	optionsSizer->Add( unitChoice, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerUnitsInterchangeable;
	bSizerUnitsInterchangeable = new wxBoxSizer( wxHORIZONTAL );
	
	unitsInterchageableText = new wxStaticText( optionsSizer->GetStaticBox(), wxID_ANY, _("Interchangeable Unit:"), wxDefaultPosition, wxDefaultSize, 0 );
	unitsInterchageableText->Wrap( -1 );
	bSizerUnitsInterchangeable->Add( unitsInterchageableText, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	unitsInterchageableLabel = new wxStaticText( optionsSizer->GetStaticBox(), wxID_ANY, _("Yes"), wxDefaultPosition, wxDefaultSize, 0 );
	unitsInterchageableLabel->Wrap( -1 );
	bSizerUnitsInterchangeable->Add( unitsInterchageableLabel, 0, wxALL, 5 );
	
	
	optionsSizer->Add( bSizerUnitsInterchangeable, 0, wxEXPAND, 5 );
	
	wxString orientationRadioBoxChoices[] = { _("0"), _("+90"), _("+180"), _("-90") };
	int orientationRadioBoxNChoices = sizeof( orientationRadioBoxChoices ) / sizeof( wxString );
	orientationRadioBox = new wxRadioBox( optionsSizer->GetStaticBox(), wxID_ANY, _("Orientation (degrees):"), wxDefaultPosition, wxDefaultSize, orientationRadioBoxNChoices, orientationRadioBoxChoices, 1, wxRA_SPECIFY_COLS );
	orientationRadioBox->SetSelection( 0 );
	orientationRadioBox->SetToolTip( _("Select if the component is to be rotated when drawn") );
	
	optionsSizer->Add( orientationRadioBox, 0, wxEXPAND|wxALL, 5 );
	
	wxString mirrorRadioBoxChoices[] = { _("Default"), _("Mirror Horizontally"), _("Mirror Vertically") };
	int mirrorRadioBoxNChoices = sizeof( mirrorRadioBoxChoices ) / sizeof( wxString );
	mirrorRadioBox = new wxRadioBox( optionsSizer->GetStaticBox(), wxID_ANY, _("Position:"), wxDefaultPosition, wxDefaultSize, mirrorRadioBoxNChoices, mirrorRadioBoxChoices, 1, wxRA_SPECIFY_COLS );
	mirrorRadioBox->SetSelection( 0 );
	mirrorRadioBox->SetToolTip( _("Pick the graphical transformation to be used when displaying the component, if any") );
	
	optionsSizer->Add( mirrorRadioBox, 0, wxALL|wxEXPAND, 5 );
	
	convertCheckBox = new wxCheckBox( optionsSizer->GetStaticBox(), wxID_ANY, _("Convert Shape"), wxDefaultPosition, wxDefaultSize, 0 );
	convertCheckBox->SetToolTip( _("Use the alternate shape of this component.\nFor gates, this is the \"De Morgan\" conversion") );
	
	optionsSizer->Add( convertCheckBox, 0, wxALL, 5 );
	
	wxStaticBoxSizer* sbSizerChipName;
	sbSizerChipName = new wxStaticBoxSizer( new wxStaticBox( optionsSizer->GetStaticBox(), wxID_ANY, _("Component Name:") ), wxVERTICAL );
	
	chipnameTextCtrl = new wxTextCtrl( sbSizerChipName->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	chipnameTextCtrl->SetToolTip( _("Name of the symbol in the library from which this component come from") );
	
	sbSizerChipName->Add( chipnameTextCtrl, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizerChpinameButt;
	bSizerChpinameButt = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonTestChipName = new wxButton( sbSizerChipName->GetStaticBox(), wxID_ANY, _("Validate"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerChpinameButt->Add( m_buttonTestChipName, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_buttonSelectChipName = new wxButton( sbSizerChipName->GetStaticBox(), wxID_ANY, _("Change"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerChpinameButt->Add( m_buttonSelectChipName, 0, wxTOP|wxBOTTOM, 5 );
	
	
	sbSizerChipName->Add( bSizerChpinameButt, 1, wxEXPAND, 5 );
	
	
	optionsSizer->Add( sbSizerChipName, 0, wxEXPAND|wxALL, 5 );
	
	m_staticTextTimeStamp = new wxStaticText( optionsSizer->GetStaticBox(), wxID_ANY, _("Component ID:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTimeStamp->Wrap( -1 );
	optionsSizer->Add( m_staticTextTimeStamp, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlTimeStamp = new wxTextCtrl( optionsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_textCtrlTimeStamp->SetToolTip( _("Unique ID that identifies the component") );
	
	optionsSizer->Add( m_textCtrlTimeStamp, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( optionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	optionsSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	spiceFieldsButton = new wxButton( optionsSizer->GetStaticBox(), wxID_ANY, _("Edit Spice Model"), wxDefaultPosition, wxDefaultSize, 0 );
	optionsSizer->Add( spiceFieldsButton, 0, wxALL|wxEXPAND, 5 );
	
	defaultsButton = new wxButton( optionsSizer->GetStaticBox(), wxID_ANY, _("Reset to Default"), wxDefaultPosition, wxDefaultSize, 0 );
	defaultsButton->SetToolTip( _("Set position and style of fields and component orientation  to default lib value.\nFields texts are not modified.") );
	
	optionsSizer->Add( defaultsButton, 0, wxALL|wxEXPAND, 5 );
	
	
	upperSizer->Add( optionsSizer, 0, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* fieldsSizer;
	fieldsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("  Fields") ), wxHORIZONTAL );
	
	wxStaticBoxSizer* gridStaticBoxSizer;
	gridStaticBoxSizer = new wxStaticBoxSizer( new wxStaticBox( fieldsSizer->GetStaticBox(), wxID_ANY, wxEmptyString ), wxVERTICAL );
	
	fieldListCtrl = new wxListCtrl( gridStaticBoxSizer->GetStaticBox(), wxID_ANY, wxPoint( -1,-1 ), wxDefaultSize, wxLC_HRULES|wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_VRULES );
	fieldListCtrl->SetMinSize( wxSize( 220,-1 ) );
	
	gridStaticBoxSizer->Add( fieldListCtrl, 1, wxALL|wxEXPAND, 8 );
	
	addFieldButton = new wxButton( gridStaticBoxSizer->GetStaticBox(), wxID_ANY, _("New Field"), wxDefaultPosition, wxDefaultSize, 0 );
	addFieldButton->SetToolTip( _("Create new custom field") );
	
	gridStaticBoxSizer->Add( addFieldButton, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	deleteFieldButton = new wxButton( gridStaticBoxSizer->GetStaticBox(), wxID_ANY, _("Delete Field"), wxDefaultPosition, wxDefaultSize, 0 );
	deleteFieldButton->SetToolTip( _("Delete optional field") );
	
	gridStaticBoxSizer->Add( deleteFieldButton, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	moveUpButton = new wxButton( gridStaticBoxSizer->GetStaticBox(), wxID_ANY, _("Move Up"), wxDefaultPosition, wxDefaultSize, 0 );
	moveUpButton->SetToolTip( _("Move the selected optional field up one position") );
	
	gridStaticBoxSizer->Add( moveUpButton, 0, wxALL|wxEXPAND, 5 );
	
	
	fieldsSizer->Add( gridStaticBoxSizer, 3, wxEXPAND|wxRIGHT|wxLEFT, 0 );
	
	wxBoxSizer* fieldEditBoxSizer;
	fieldEditBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerJustification;
	bSizerJustification = new wxBoxSizer( wxHORIZONTAL );
	
	wxString m_FieldHJustifyCtrlChoices[] = { _("Align Left"), _("Center"), _("Align Right") };
	int m_FieldHJustifyCtrlNChoices = sizeof( m_FieldHJustifyCtrlChoices ) / sizeof( wxString );
	m_FieldHJustifyCtrl = new wxRadioBox( fieldsSizer->GetStaticBox(), wxID_ANY, _("Horizontal Position:"), wxDefaultPosition, wxDefaultSize, m_FieldHJustifyCtrlNChoices, m_FieldHJustifyCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_FieldHJustifyCtrl->SetSelection( 0 );
	bSizerJustification->Add( m_FieldHJustifyCtrl, 1, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	wxString m_FieldVJustifyCtrlChoices[] = { _("Align Top"), _("Center"), _("Align Bottom") };
	int m_FieldVJustifyCtrlNChoices = sizeof( m_FieldVJustifyCtrlChoices ) / sizeof( wxString );
	m_FieldVJustifyCtrl = new wxRadioBox( fieldsSizer->GetStaticBox(), wxID_ANY, _("Vertical Position:"), wxDefaultPosition, wxDefaultSize, m_FieldVJustifyCtrlNChoices, m_FieldVJustifyCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_FieldVJustifyCtrl->SetSelection( 0 );
	bSizerJustification->Add( m_FieldVJustifyCtrl, 1, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	fieldEditBoxSizer->Add( bSizerJustification, 1, wxEXPAND|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizerStyle;
	bSizerStyle = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* visibilitySizer;
	visibilitySizer = new wxStaticBoxSizer( new wxStaticBox( fieldsSizer->GetStaticBox(), wxID_ANY, _("Visibility:") ), wxVERTICAL );
	
	showCheckBox = new wxCheckBox( visibilitySizer->GetStaticBox(), wxID_ANY, _("Show"), wxDefaultPosition, wxDefaultSize, 0 );
	showCheckBox->SetToolTip( _("Make selected field visible") );
	
	visibilitySizer->Add( showCheckBox, 0, wxALL, 5 );
	
	rotateCheckBox = new wxCheckBox( visibilitySizer->GetStaticBox(), wxID_ANY, _("Rotate"), wxDefaultPosition, wxDefaultSize, 0 );
	rotateCheckBox->SetToolTip( _("Rotated 90 degrees the selected field") );
	
	visibilitySizer->Add( rotateCheckBox, 0, wxALL, 5 );
	
	
	bSizerStyle->Add( visibilitySizer, 1, wxEXPAND|wxALL, 5 );
	
	wxString m_StyleRadioBoxChoices[] = { _("Normal"), _("Italic"), _("Bold"), _("Bold Italic") };
	int m_StyleRadioBoxNChoices = sizeof( m_StyleRadioBoxChoices ) / sizeof( wxString );
	m_StyleRadioBox = new wxRadioBox( fieldsSizer->GetStaticBox(), wxID_ANY, _("Font Style:"), wxDefaultPosition, wxDefaultSize, m_StyleRadioBoxNChoices, m_StyleRadioBoxChoices, 1, wxRA_SPECIFY_COLS );
	m_StyleRadioBox->SetSelection( 0 );
	bSizerStyle->Add( m_StyleRadioBox, 1, wxEXPAND|wxALL, 5 );
	
	
	fieldEditBoxSizer->Add( bSizerStyle, 1, wxEXPAND|wxBOTTOM, 5 );
	
	wxBoxSizer* fieldNameBoxSizer;
	fieldNameBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	fieldNameLabel = new wxStaticText( fieldsSizer->GetStaticBox(), wxID_ANY, _("Field Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	fieldNameLabel->Wrap( -1 );
	fieldNameBoxSizer->Add( fieldNameLabel, 0, wxTOP, 5 );
	
	fieldNameTextCtrl = new wxTextCtrl( fieldsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fieldNameTextCtrl->SetToolTip( _("Name of the selected field. Fixed field names are not editable") );
	
	fieldNameBoxSizer->Add( fieldNameTextCtrl, 0, wxBOTTOM|wxEXPAND, 5 );
	
	fieldValueLabel = new wxStaticText( fieldsSizer->GetStaticBox(), wxID_ANY, _("Field Value:"), wxDefaultPosition, wxDefaultSize, 0 );
	fieldValueLabel->Wrap( -1 );
	fieldNameBoxSizer->Add( fieldValueLabel, 0, wxALIGN_TOP|wxTOP, 5 );
	
	fieldValueTextCtrl = new wxTextCtrl( fieldsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fieldValueTextCtrl->SetToolTip( _("Name of the selected field. Fixed field names are not editable") );
	
	fieldNameBoxSizer->Add( fieldValueTextCtrl, 0, wxEXPAND|wxBOTTOM, 5 );
	
	m_show_datasheet_button = new wxButton( fieldsSizer->GetStaticBox(), wxID_ANY, _("Open in Browser"), wxDefaultPosition, wxDefaultSize, 0 );
	m_show_datasheet_button->SetToolTip( _("If your datasheet is an http:// link or a complete file path, then it may show in your browser by pressing this button.") );
	
	fieldNameBoxSizer->Add( m_show_datasheet_button, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	
	fieldEditBoxSizer->Add( fieldNameBoxSizer, 0, wxBOTTOM|wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizerPosSize;
	fgSizerPosSize = new wxFlexGridSizer( 3, 3, 6, 0 );
	fgSizerPosSize->AddGrowableCol( 1 );
	fgSizerPosSize->SetFlexibleDirection( wxBOTH );
	fgSizerPosSize->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	textSizeLabel = new wxStaticText( fieldsSizer->GetStaticBox(), wxID_ANY, _("Font Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	textSizeLabel->Wrap( -1 );
	fgSizerPosSize->Add( textSizeLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	textSizeTextCtrl = new wxTextCtrl( fieldsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxPoint( -1,-1 ), wxDefaultSize, 0 );
	textSizeTextCtrl->SetToolTip( _("Font Size of the selected field") );
	
	fgSizerPosSize->Add( textSizeTextCtrl, 0, wxEXPAND|wxBOTTOM, 0 );
	
	m_staticTextUnitSize = new wxStaticText( fieldsSizer->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUnitSize->Wrap( -1 );
	fgSizerPosSize->Add( m_staticTextUnitSize, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	posXLabel = new wxStaticText( fieldsSizer->GetStaticBox(), wxID_ANY, _("Position X:"), wxDefaultPosition, wxDefaultSize, 0 );
	posXLabel->Wrap( -1 );
	fgSizerPosSize->Add( posXLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	posXTextCtrl = new wxTextCtrl( fieldsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxPoint( -1,-1 ), wxDefaultSize, 0 );
	posXTextCtrl->SetToolTip( _("X coordinate of the selected field") );
	
	fgSizerPosSize->Add( posXTextCtrl, 0, wxEXPAND|wxTOP, 0 );
	
	m_staticTextUnitPosX = new wxStaticText( fieldsSizer->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUnitPosX->Wrap( -1 );
	fgSizerPosSize->Add( m_staticTextUnitPosX, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	posYLabel = new wxStaticText( fieldsSizer->GetStaticBox(), wxID_ANY, _("Position Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	posYLabel->Wrap( -1 );
	fgSizerPosSize->Add( posYLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	posYTextCtrl = new wxTextCtrl( fieldsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxPoint( -1,-1 ), wxDefaultSize, 0 );
	posYTextCtrl->SetToolTip( _("X coordinate of the selected field") );
	
	fgSizerPosSize->Add( posYTextCtrl, 0, wxEXPAND, 5 );
	
	m_staticTextUnitPosY = new wxStaticText( fieldsSizer->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUnitPosY->Wrap( -1 );
	fgSizerPosSize->Add( m_staticTextUnitPosY, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fieldEditBoxSizer->Add( fgSizerPosSize, 1, wxEXPAND|wxTOP, 5 );
	
	
	fieldsSizer->Add( fieldEditBoxSizer, 2, wxEXPAND, 5 );
	
	
	upperSizer->Add( fieldsSizer, 1, wxALL|wxEXPAND, 5 );
	
	
	mainSizer->Add( upperSizer, 1, wxEXPAND, 5 );
	
	stdDialogButtonSizer = new wxStdDialogButtonSizer();
	stdDialogButtonSizerOK = new wxButton( this, wxID_OK );
	stdDialogButtonSizer->AddButton( stdDialogButtonSizerOK );
	stdDialogButtonSizerCancel = new wxButton( this, wxID_CANCEL );
	stdDialogButtonSizer->AddButton( stdDialogButtonSizerCancel );
	stdDialogButtonSizer->Realize();
	
	mainSizer->Add( stdDialogButtonSizer, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( mainSizer );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::OnCloseDialog ) );
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::OnInitDlg ) );
	m_buttonTestChipName->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::OnTestChipName ), NULL, this );
	m_buttonSelectChipName->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::OnSelectChipName ), NULL, this );
	spiceFieldsButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::EditSpiceModel ), NULL, this );
	defaultsButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::SetInitCmp ), NULL, this );
	fieldListCtrl->Connect( wxEVT_COMMAND_LIST_ITEM_DESELECTED, wxListEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::OnListItemDeselected ), NULL, this );
	fieldListCtrl->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::OnListItemSelected ), NULL, this );
	addFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::addFieldButtonHandler ), NULL, this );
	deleteFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::deleteFieldButtonHandler ), NULL, this );
	moveUpButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::moveUpButtonHandler ), NULL, this );
	m_show_datasheet_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::showButtonHandler ), NULL, this );
	stdDialogButtonSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::OnCancelButtonClick ), NULL, this );
	stdDialogButtonSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::OnOKButtonClick ), NULL, this );
}

DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::~DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::OnCloseDialog ) );
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::OnInitDlg ) );
	m_buttonTestChipName->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::OnTestChipName ), NULL, this );
	m_buttonSelectChipName->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::OnSelectChipName ), NULL, this );
	spiceFieldsButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::EditSpiceModel ), NULL, this );
	defaultsButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::SetInitCmp ), NULL, this );
	fieldListCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_DESELECTED, wxListEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::OnListItemDeselected ), NULL, this );
	fieldListCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::OnListItemSelected ), NULL, this );
	addFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::addFieldButtonHandler ), NULL, this );
	deleteFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::deleteFieldButtonHandler ), NULL, this );
	moveUpButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::moveUpButtonHandler ), NULL, this );
	m_show_datasheet_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::showButtonHandler ), NULL, this );
	stdDialogButtonSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::OnCancelButtonClick ), NULL, this );
	stdDialogButtonSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP::OnOKButtonClick ), NULL, this );
	
}
