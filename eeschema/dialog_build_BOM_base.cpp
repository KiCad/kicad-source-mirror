///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_build_BOM_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_BUILD_BOM_BASE::DIALOG_BUILD_BOM_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbOptionsSizer;
	sbOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options:") ), wxVERTICAL );
	
	wxStaticBoxSizer* sbListOptionsSizer;
	sbListOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("List items:") ), wxVERTICAL );
	
	m_ListCmpbyRefItems = new wxCheckBox( this, wxID_ANY, _("Components by reference"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbListOptionsSizer->Add( m_ListCmpbyRefItems, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ListSubCmpItems = new wxCheckBox( this, wxID_ANY, _("Sub components (i.e. U2A, U2B ...)"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbListOptionsSizer->Add( m_ListSubCmpItems, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ListCmpbyValItems = new wxCheckBox( this, wxID_ANY, _("Components by value"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbListOptionsSizer->Add( m_ListCmpbyValItems, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_GenListLabelsbyVal = new wxCheckBox( this, wxID_ANY, _("Hierarchy pins by name"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbListOptionsSizer->Add( m_GenListLabelsbyVal, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_GenListLabelsbySheet = new wxCheckBox( this, wxID_ANY, _("Hierarchy pins by sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbListOptionsSizer->Add( m_GenListLabelsbySheet, 0, wxALL, 5 );
	
	sbOptionsSizer->Add( sbListOptionsSizer, 0, wxEXPAND, 5 );
	
	wxString m_OutputFormCtrlChoices[] = { _("List"), _("Text for spreadsheet import") };
	int m_OutputFormCtrlNChoices = sizeof( m_OutputFormCtrlChoices ) / sizeof( wxString );
	m_OutputFormCtrl = new wxRadioBox( this, ID_RADIOBOX_SELECT_FORMAT, _("Output format:"), wxDefaultPosition, wxDefaultSize, m_OutputFormCtrlNChoices, m_OutputFormCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_OutputFormCtrl->SetSelection( 0 );
	sbOptionsSizer->Add( m_OutputFormCtrl, 0, wxEXPAND|wxTOP, 5 );
	
	wxString m_OutputSeparatorCtrlChoices[] = { _("Tab"), _(";"), _(",") };
	int m_OutputSeparatorCtrlNChoices = sizeof( m_OutputSeparatorCtrlChoices ) / sizeof( wxString );
	m_OutputSeparatorCtrl = new wxRadioBox( this, wxID_ANY, _("Field separator for spreadsheet import:"), wxDefaultPosition, wxDefaultSize, m_OutputSeparatorCtrlNChoices, m_OutputSeparatorCtrlChoices, 1, wxRA_SPECIFY_ROWS );
	m_OutputSeparatorCtrl->SetSelection( 0 );
	sbOptionsSizer->Add( m_OutputSeparatorCtrl, 0, wxEXPAND|wxTOP, 5 );
	
	wxStaticBoxSizer* sbBrowseOptSizer;
	sbBrowseOptSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options:") ), wxVERTICAL );
	
	m_GetListBrowser = new wxCheckBox( this, wxID_ANY, _("Launch list browser"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbBrowseOptSizer->Add( m_GetListBrowser, 0, wxALL|wxEXPAND, 5 );
	
	sbOptionsSizer->Add( sbBrowseOptSizer, 0, wxEXPAND|wxTOP, 5 );
	
	bMainSizer->Add( sbOptionsSizer, 10, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbFieldsSelectionSizer;
	sbFieldsSelectionSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Fields to add:") ), wxVERTICAL );
	
	wxStaticBoxSizer* sbFixedFieldsSizer;
	sbFixedFieldsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("System Fields:") ), wxVERTICAL );
	
	m_AddFootprintField = new wxCheckBox( this, wxID_ANY, _("Footprint"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbFixedFieldsSizer->Add( m_AddFootprintField, 0, wxALL|wxEXPAND, 5 );
	
	sbFieldsSelectionSizer->Add( sbFixedFieldsSizer, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbUsersFiledsSizer;
	sbUsersFiledsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Users Fields:") ), wxVERTICAL );
	
	m_AddField1 = new wxCheckBox( this, wxID_ANY, _("Field 1"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbUsersFiledsSizer->Add( m_AddField1, 0, wxEXPAND|wxALL, 5 );
	
	m_AddField2 = new wxCheckBox( this, wxID_ANY, _("Field 2"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbUsersFiledsSizer->Add( m_AddField2, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_AddField3 = new wxCheckBox( this, wxID_ANY, _("Field 3"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbUsersFiledsSizer->Add( m_AddField3, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_AddField4 = new wxCheckBox( this, wxID_ANY, _("Field 4"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbUsersFiledsSizer->Add( m_AddField4, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_AddField5 = new wxCheckBox( this, wxID_ANY, _("Field 5"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbUsersFiledsSizer->Add( m_AddField5, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_AddField6 = new wxCheckBox( this, wxID_ANY, _("Field 6"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbUsersFiledsSizer->Add( m_AddField6, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_AddField7 = new wxCheckBox( this, wxID_ANY, _("Field 7"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbUsersFiledsSizer->Add( m_AddField7, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_AddField8 = new wxCheckBox( this, wxID_ANY, _("Field 8"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbUsersFiledsSizer->Add( m_AddField8, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_AddAllFields = new wxCheckBox( this, wxID_ANY, _("All existing users fields"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbUsersFiledsSizer->Add( m_AddAllFields, 0, wxALL, 5 );
	
	sbFieldsSelectionSizer->Add( sbUsersFiledsSizer, 0, wxEXPAND|wxTOP, 5 );
	
	bRightSizer->Add( sbFieldsSelectionSizer, 1, wxEXPAND, 5 );
	
	
	bRightSizer->Add( 10, 10, 0, 0, 5 );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonOK->SetDefault(); 
	bRightSizer->Add( m_buttonOK, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_buttonCANCEL = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonCANCEL, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bMainSizer->Add( bRightSizer, 8, wxALL|wxEXPAND, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	m_OutputFormCtrl->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_BUILD_BOM_BASE::OnRadioboxSelectFormatSelected ), NULL, this );
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BUILD_BOM_BASE::OnOkClick ), NULL, this );
	m_buttonCANCEL->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BUILD_BOM_BASE::OnCancelClick ), NULL, this );
}

DIALOG_BUILD_BOM_BASE::~DIALOG_BUILD_BOM_BASE()
{
	// Disconnect Events
	m_OutputFormCtrl->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_BUILD_BOM_BASE::OnRadioboxSelectFormatSelected ), NULL, this );
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BUILD_BOM_BASE::OnOkClick ), NULL, this );
	m_buttonCANCEL->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BUILD_BOM_BASE::OnCancelClick ), NULL, this );
}
