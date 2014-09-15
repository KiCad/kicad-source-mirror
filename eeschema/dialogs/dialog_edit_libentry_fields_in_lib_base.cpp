///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov  6 2013)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_edit_libentry_fields_in_lib_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerFieldsSetup;
	bSizerFieldsSetup = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerFiledsList;
	bSizerFiledsList = new wxBoxSizer( wxVERTICAL );
	
	fieldListCtrl = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_HRULES|wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_VRULES );
	fieldListCtrl->SetMinSize( wxSize( 220,-1 ) );
	
	bSizerFiledsList->Add( fieldListCtrl, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 8 );
	
	addFieldButton = new wxButton( this, wxID_ANY, _("Add Field"), wxDefaultPosition, wxDefaultSize, 0 );
	addFieldButton->SetToolTip( _("Add a new custom field") );
	
	bSizerFiledsList->Add( addFieldButton, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	deleteFieldButton = new wxButton( this, wxID_ANY, _("Delete Field"), wxDefaultPosition, wxDefaultSize, 0 );
	deleteFieldButton->SetToolTip( _("Delete one of the optional fields") );
	
	bSizerFiledsList->Add( deleteFieldButton, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	moveUpButton = new wxButton( this, wxID_ANY, _("Move Up"), wxDefaultPosition, wxDefaultSize, 0 );
	moveUpButton->SetToolTip( _("Move the selected optional fields up one position") );
	
	bSizerFiledsList->Add( moveUpButton, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerFieldsSetup->Add( bSizerFiledsList, 3, wxEXPAND, 5 );
	
	wxBoxSizer* fieldEditBoxSizer;
	fieldEditBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerJustify;
	bSizerJustify = new wxBoxSizer( wxHORIZONTAL );
	
	wxString m_FieldHJustifyCtrlChoices[] = { _("Left"), _("Center"), _("Right") };
	int m_FieldHJustifyCtrlNChoices = sizeof( m_FieldHJustifyCtrlChoices ) / sizeof( wxString );
	m_FieldHJustifyCtrl = new wxRadioBox( this, wxID_ANY, _("Horiz. Justify"), wxDefaultPosition, wxDefaultSize, m_FieldHJustifyCtrlNChoices, m_FieldHJustifyCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_FieldHJustifyCtrl->SetSelection( 1 );
	m_FieldHJustifyCtrl->SetToolTip( _("Select if the component is to be rotated when drawn") );
	
	bSizerJustify->Add( m_FieldHJustifyCtrl, 1, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxString m_FieldVJustifyCtrlChoices[] = { _("Bottom"), _("Center"), _("Top") };
	int m_FieldVJustifyCtrlNChoices = sizeof( m_FieldVJustifyCtrlChoices ) / sizeof( wxString );
	m_FieldVJustifyCtrl = new wxRadioBox( this, wxID_ANY, _("Vert. Justify"), wxDefaultPosition, wxDefaultSize, m_FieldVJustifyCtrlNChoices, m_FieldVJustifyCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_FieldVJustifyCtrl->SetSelection( 0 );
	m_FieldVJustifyCtrl->SetToolTip( _("Pick the graphical transformation to be used when displaying the component, if any") );
	
	bSizerJustify->Add( m_FieldVJustifyCtrl, 1, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	fieldEditBoxSizer->Add( bSizerJustify, 0, wxEXPAND|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizerAspect;
	bSizerAspect = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* visibilitySizer;
	visibilitySizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Visibility") ), wxVERTICAL );
	
	showCheckBox = new wxCheckBox( this, wxID_ANY, _("Show"), wxDefaultPosition, wxDefaultSize, 0 );
	showCheckBox->SetToolTip( _("Check if you want this field visible") );
	
	visibilitySizer->Add( showCheckBox, 0, wxALL, 5 );
	
	rotateCheckBox = new wxCheckBox( this, wxID_ANY, _("Rotate"), wxDefaultPosition, wxDefaultSize, 0 );
	rotateCheckBox->SetToolTip( _("Check if you want this field's text rotated 90 degrees") );
	
	visibilitySizer->Add( rotateCheckBox, 0, wxALL, 5 );
	
	
	bSizerAspect->Add( visibilitySizer, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxString m_StyleRadioBoxChoices[] = { _("Normal"), _("Italic"), _("Bold"), _("Bold Italic") };
	int m_StyleRadioBoxNChoices = sizeof( m_StyleRadioBoxChoices ) / sizeof( wxString );
	m_StyleRadioBox = new wxRadioBox( this, wxID_ANY, _("Style:"), wxDefaultPosition, wxDefaultSize, m_StyleRadioBoxNChoices, m_StyleRadioBoxChoices, 1, wxRA_SPECIFY_COLS );
	m_StyleRadioBox->SetSelection( 0 );
	bSizerAspect->Add( m_StyleRadioBox, 1, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	fieldEditBoxSizer->Add( bSizerAspect, 0, wxEXPAND|wxTOP, 5 );
	
	wxBoxSizer* fieldNameBoxSizer;
	fieldNameBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	fieldNameLabel = new wxStaticText( this, wxID_ANY, _("Field Name"), wxDefaultPosition, wxDefaultSize, 0 );
	fieldNameLabel->Wrap( -1 );
	fieldNameBoxSizer->Add( fieldNameLabel, 0, 0, 5 );
	
	fieldNameTextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fieldNameTextCtrl->SetMaxLength( 0 ); 
	fieldNameTextCtrl->SetToolTip( _("The text (or value) of the currently selected field") );
	
	fieldNameBoxSizer->Add( fieldNameTextCtrl, 0, wxBOTTOM|wxEXPAND, 5 );
	
	fieldValueLabel = new wxStaticText( this, wxID_ANY, _("Field Value"), wxDefaultPosition, wxDefaultSize, 0 );
	fieldValueLabel->Wrap( -1 );
	fieldNameBoxSizer->Add( fieldValueLabel, 0, wxTOP, 5 );
	
	fieldValueTextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fieldValueTextCtrl->SetMaxLength( 0 ); 
	fieldValueTextCtrl->SetToolTip( _("The text (or value) of the currently selected field") );
	
	fieldNameBoxSizer->Add( fieldValueTextCtrl, 0, wxBOTTOM|wxEXPAND, 5 );
	
	m_show_datasheet_button = new wxButton( this, wxID_ANY, _("Show in Browser"), wxDefaultPosition, wxDefaultSize, 0 );
	m_show_datasheet_button->SetToolTip( _("If your datasheet is given as an http:// link, then pressing this button should bring it up in your webbrowser.") );
	
	fieldNameBoxSizer->Add( m_show_datasheet_button, 0, wxBOTTOM|wxEXPAND, 5 );
	
	
	fieldEditBoxSizer->Add( fieldNameBoxSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizerPosSize;
	fgSizerPosSize = new wxFlexGridSizer( 3, 3, 0, 0 );
	fgSizerPosSize->AddGrowableCol( 1 );
	fgSizerPosSize->SetFlexibleDirection( wxBOTH );
	fgSizerPosSize->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	textSizeLabel = new wxStaticText( this, wxID_ANY, _("Size"), wxDefaultPosition, wxDefaultSize, 0 );
	textSizeLabel->Wrap( -1 );
	fgSizerPosSize->Add( textSizeLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	textSizeTextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	textSizeTextCtrl->SetMaxLength( 0 ); 
	textSizeTextCtrl->SetToolTip( _("The vertical height of the currently selected field's text in the schematic") );
	
	fgSizerPosSize->Add( textSizeTextCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );
	
	m_staticTextUnitSize = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUnitSize->Wrap( -1 );
	fgSizerPosSize->Add( m_staticTextUnitSize, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	posXLabel = new wxStaticText( this, wxID_ANY, _("PosX"), wxDefaultPosition, wxDefaultSize, 0 );
	posXLabel->Wrap( -1 );
	fgSizerPosSize->Add( posXLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	posXTextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	posXTextCtrl->SetMaxLength( 0 ); 
	fgSizerPosSize->Add( posXTextCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP, 5 );
	
	m_staticTextUnitPosX = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUnitPosX->Wrap( -1 );
	fgSizerPosSize->Add( m_staticTextUnitPosX, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	posYLabel = new wxStaticText( this, wxID_ANY, _("PosY"), wxDefaultPosition, wxDefaultSize, 0 );
	posYLabel->Wrap( -1 );
	fgSizerPosSize->Add( posYLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	posYTextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	posYTextCtrl->SetMaxLength( 0 ); 
	posYTextCtrl->SetToolTip( _("The Y coordinate of the text relative to the component") );
	
	fgSizerPosSize->Add( posYTextCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextUnitPosY = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUnitPosY->Wrap( -1 );
	fgSizerPosSize->Add( m_staticTextUnitPosY, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fieldEditBoxSizer->Add( fgSizerPosSize, 1, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	
	bSizerFieldsSetup->Add( fieldEditBoxSizer, 2, wxEXPAND, 5 );
	
	
	mainSizer->Add( bSizerFieldsSetup, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	stdDialogButtonSizer = new wxStdDialogButtonSizer();
	stdDialogButtonSizerOK = new wxButton( this, wxID_OK );
	stdDialogButtonSizer->AddButton( stdDialogButtonSizerOK );
	stdDialogButtonSizerCancel = new wxButton( this, wxID_CANCEL );
	stdDialogButtonSizer->AddButton( stdDialogButtonSizerCancel );
	stdDialogButtonSizer->Realize();
	
	mainSizer->Add( stdDialogButtonSizer, 0, wxALL|wxEXPAND, 8 );
	
	
	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnCloseDialog ) );
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnInitDialog ) );
	fieldListCtrl->Connect( wxEVT_COMMAND_LIST_ITEM_DESELECTED, wxListEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnListItemDeselected ), NULL, this );
	fieldListCtrl->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnListItemSelected ), NULL, this );
	addFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::addFieldButtonHandler ), NULL, this );
	deleteFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::deleteFieldButtonHandler ), NULL, this );
	moveUpButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::moveUpButtonHandler ), NULL, this );
	m_show_datasheet_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::showButtonHandler ), NULL, this );
	stdDialogButtonSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnCancelButtonClick ), NULL, this );
	stdDialogButtonSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnOKButtonClick ), NULL, this );
}

DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::~DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnCloseDialog ) );
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnInitDialog ) );
	fieldListCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_DESELECTED, wxListEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnListItemDeselected ), NULL, this );
	fieldListCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnListItemSelected ), NULL, this );
	addFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::addFieldButtonHandler ), NULL, this );
	deleteFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::deleteFieldButtonHandler ), NULL, this );
	moveUpButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::moveUpButtonHandler ), NULL, this );
	m_show_datasheet_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::showButtonHandler ), NULL, this );
	stdDialogButtonSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnCancelButtonClick ), NULL, this );
	stdDialogButtonSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnOKButtonClick ), NULL, this );
	
}
