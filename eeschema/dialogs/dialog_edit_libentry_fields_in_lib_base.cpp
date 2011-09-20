///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 30 2011)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_edit_libentry_fields_in_lib_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* fieldsSizer;
	fieldsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Fields") ), wxHORIZONTAL );
	
	wxStaticBoxSizer* gridStaticBoxSizer;
	gridStaticBoxSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxEmptyString ), wxVERTICAL );
	
	fieldListCtrl = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_HRULES|wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_VRULES );
	fieldListCtrl->SetMinSize( wxSize( 220,-1 ) );
	
	gridStaticBoxSizer->Add( fieldListCtrl, 1, wxALL|wxEXPAND, 8 );
	
	addFieldButton = new wxButton( this, wxID_ANY, _("Add Field"), wxDefaultPosition, wxDefaultSize, 0 );
	addFieldButton->SetToolTip( _("Add a new custom field") );
	
	gridStaticBoxSizer->Add( addFieldButton, 0, wxALL|wxEXPAND, 5 );
	
	deleteFieldButton = new wxButton( this, wxID_ANY, _("Delete Field"), wxDefaultPosition, wxDefaultSize, 0 );
	deleteFieldButton->SetToolTip( _("Delete one of the optional fields") );
	
	gridStaticBoxSizer->Add( deleteFieldButton, 0, wxALL|wxEXPAND, 5 );
	
	moveUpButton = new wxButton( this, wxID_ANY, _("Move Up"), wxDefaultPosition, wxDefaultSize, 0 );
	moveUpButton->SetToolTip( _("Move the selected optional fields up one position") );
	
	gridStaticBoxSizer->Add( moveUpButton, 0, wxALL|wxEXPAND, 5 );
	
	fieldsSizer->Add( gridStaticBoxSizer, 5, wxEXPAND|wxRIGHT, 8 );
	
	wxBoxSizer* fieldEditBoxSizer;
	fieldEditBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* optionsSizer;
	optionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Text Justification:") ), wxHORIZONTAL );
	
	wxString m_FieldHJustifyCtrlChoices[] = { _("Left"), _("Center"), _("Right") };
	int m_FieldHJustifyCtrlNChoices = sizeof( m_FieldHJustifyCtrlChoices ) / sizeof( wxString );
	m_FieldHJustifyCtrl = new wxRadioBox( this, wxID_ANY, _("Horiz. Justify"), wxDefaultPosition, wxDefaultSize, m_FieldHJustifyCtrlNChoices, m_FieldHJustifyCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_FieldHJustifyCtrl->SetSelection( 1 );
	m_FieldHJustifyCtrl->SetToolTip( _("Select if the component is to be rotated when drawn") );
	
	optionsSizer->Add( m_FieldHJustifyCtrl, 1, wxBOTTOM|wxRIGHT|wxLEFT, 8 );
	
	wxString m_FieldVJustifyCtrlChoices[] = { _("Bottom"), _("Center"), _("Top") };
	int m_FieldVJustifyCtrlNChoices = sizeof( m_FieldVJustifyCtrlChoices ) / sizeof( wxString );
	m_FieldVJustifyCtrl = new wxRadioBox( this, wxID_ANY, _("Vert. Justify"), wxDefaultPosition, wxDefaultSize, m_FieldVJustifyCtrlNChoices, m_FieldVJustifyCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_FieldVJustifyCtrl->SetSelection( 0 );
	m_FieldVJustifyCtrl->SetToolTip( _("Pick the graphical transformation to be used when displaying the component, if any") );
	
	optionsSizer->Add( m_FieldVJustifyCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 8 );
	
	fieldEditBoxSizer->Add( optionsSizer, 0, wxALIGN_TOP|wxEXPAND|wxBOTTOM, 5 );
	
	wxStaticBoxSizer* visibilitySizer;
	visibilitySizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Visibility") ), wxHORIZONTAL );
	
	wxBoxSizer* bShowRotateSizer;
	bShowRotateSizer = new wxBoxSizer( wxVERTICAL );
	
	showCheckBox = new wxCheckBox( this, wxID_ANY, _("Show"), wxDefaultPosition, wxDefaultSize, 0 );
	showCheckBox->SetToolTip( _("Check if you want this field visible") );
	
	bShowRotateSizer->Add( showCheckBox, 0, wxALL, 5 );
	
	rotateCheckBox = new wxCheckBox( this, wxID_ANY, _("Rotate"), wxDefaultPosition, wxDefaultSize, 0 );
	rotateCheckBox->SetToolTip( _("Check if you want this field's text rotated 90 degrees") );
	
	bShowRotateSizer->Add( rotateCheckBox, 0, wxALL, 5 );
	
	visibilitySizer->Add( bShowRotateSizer, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_StyleRadioBoxChoices[] = { _("Normal"), _("Italic"), _("Bold"), _("Bold Italic") };
	int m_StyleRadioBoxNChoices = sizeof( m_StyleRadioBoxChoices ) / sizeof( wxString );
	m_StyleRadioBox = new wxRadioBox( this, wxID_ANY, _("Style:"), wxDefaultPosition, wxDefaultSize, m_StyleRadioBoxNChoices, m_StyleRadioBoxChoices, 1, wxRA_SPECIFY_COLS );
	m_StyleRadioBox->SetSelection( 1 );
	visibilitySizer->Add( m_StyleRadioBox, 1, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	fieldEditBoxSizer->Add( visibilitySizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* fieldNameBoxSizer;
	fieldNameBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	fieldNameLabel = new wxStaticText( this, wxID_ANY, _("Field Name"), wxDefaultPosition, wxDefaultSize, 0 );
	fieldNameLabel->Wrap( -1 );
	fieldNameBoxSizer->Add( fieldNameLabel, 0, 0, 5 );
	
	fieldNameTextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fieldNameTextCtrl->SetToolTip( _("The text (or value) of the currently selected field") );
	
	fieldNameBoxSizer->Add( fieldNameTextCtrl, 0, wxEXPAND, 5 );
	
	fieldEditBoxSizer->Add( fieldNameBoxSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* fieldTextBoxSizer;
	fieldTextBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	fieldValueLabel = new wxStaticText( this, wxID_ANY, _("Field Value"), wxDefaultPosition, wxDefaultSize, 0 );
	fieldValueLabel->Wrap( -1 );
	fieldTextBoxSizer->Add( fieldValueLabel, 0, 0, 5 );
	
	fieldValueTextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fieldValueTextCtrl->SetToolTip( _("The text (or value) of the currently selected field") );
	
	fieldTextBoxSizer->Add( fieldValueTextCtrl, 0, wxEXPAND, 5 );
	
	fieldEditBoxSizer->Add( fieldTextBoxSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* textSizeBoxSizer;
	textSizeBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	textSizeLabel = new wxStaticText( this, wxID_ANY, _("Size(\")"), wxDefaultPosition, wxDefaultSize, 0 );
	textSizeLabel->Wrap( -1 );
	textSizeBoxSizer->Add( textSizeLabel, 0, 0, 5 );
	
	textSizeTextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	textSizeTextCtrl->SetToolTip( _("The vertical height of the currently selected field's text in the schematic") );
	
	textSizeBoxSizer->Add( textSizeTextCtrl, 0, wxEXPAND, 5 );
	
	fieldEditBoxSizer->Add( textSizeBoxSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* positionBoxSizer;
	positionBoxSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* posXBoxSizer;
	posXBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	posXLabel = new wxStaticText( this, wxID_ANY, _("PosX(\")"), wxDefaultPosition, wxDefaultSize, 0 );
	posXLabel->Wrap( -1 );
	posXBoxSizer->Add( posXLabel, 0, 0, 5 );
	
	posXTextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	posXBoxSizer->Add( posXTextCtrl, 0, wxEXPAND, 5 );
	
	positionBoxSizer->Add( posXBoxSizer, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* posYBoxSizer;
	posYBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	posYLabel = new wxStaticText( this, wxID_ANY, _("PosY(\")"), wxDefaultPosition, wxDefaultSize, 0 );
	posYLabel->Wrap( -1 );
	posYBoxSizer->Add( posYLabel, 0, 0, 5 );
	
	posYTextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	posYTextCtrl->SetToolTip( _("The Y coordinate of the text relative to the component") );
	
	posYBoxSizer->Add( posYTextCtrl, 0, wxEXPAND, 5 );
	
	positionBoxSizer->Add( posYBoxSizer, 1, wxALL|wxEXPAND, 5 );
	
	fieldEditBoxSizer->Add( positionBoxSizer, 1, wxEXPAND, 5 );
	
	fieldsSizer->Add( fieldEditBoxSizer, 3, wxEXPAND, 5 );
	
	mainSizer->Add( fieldsSizer, 1, wxEXPAND|wxALL, 5 );
	
	stdDialogButtonSizer = new wxStdDialogButtonSizer();
	stdDialogButtonSizerOK = new wxButton( this, wxID_OK );
	stdDialogButtonSizer->AddButton( stdDialogButtonSizerOK );
	stdDialogButtonSizerCancel = new wxButton( this, wxID_CANCEL );
	stdDialogButtonSizer->AddButton( stdDialogButtonSizerCancel );
	stdDialogButtonSizer->Realize();
	mainSizer->Add( stdDialogButtonSizer, 0, wxALL|wxEXPAND, 8 );
	
	this->SetSizer( mainSizer );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnInitDialog ) );
	fieldListCtrl->Connect( wxEVT_COMMAND_LIST_ITEM_DESELECTED, wxListEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnListItemDeselected ), NULL, this );
	fieldListCtrl->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnListItemSelected ), NULL, this );
	addFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::addFieldButtonHandler ), NULL, this );
	deleteFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::deleteFieldButtonHandler ), NULL, this );
	moveUpButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::moveUpButtonHandler ), NULL, this );
	stdDialogButtonSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnCancelButtonClick ), NULL, this );
	stdDialogButtonSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnOKButtonClick ), NULL, this );
}

DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::~DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnInitDialog ) );
	fieldListCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_DESELECTED, wxListEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnListItemDeselected ), NULL, this );
	fieldListCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnListItemSelected ), NULL, this );
	addFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::addFieldButtonHandler ), NULL, this );
	deleteFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::deleteFieldButtonHandler ), NULL, this );
	moveUpButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::moveUpButtonHandler ), NULL, this );
	stdDialogButtonSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnCancelButtonClick ), NULL, this );
	stdDialogButtonSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnOKButtonClick ), NULL, this );
	
}
