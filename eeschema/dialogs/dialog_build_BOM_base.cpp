///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct  8 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_build_BOM_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_BUILD_BOM_BASE::DIALOG_BUILD_BOM_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );
	
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
	
	m_GenListLabelsbyVal = new wxCheckBox( this, wxID_ANY, _("Hierarchical pins by name"), wxDefaultPosition, wxDefaultSize, 0 );
	sbListOptionsSizer->Add( m_GenListLabelsbyVal, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_GenListLabelsbySheet = new wxCheckBox( this, wxID_ANY, _("Hierarchical pins by sheet"), wxDefaultPosition, wxDefaultSize, 0 );
	sbListOptionsSizer->Add( m_GenListLabelsbySheet, 0, wxALL, 5 );
	
	
	sbOptionsSizer->Add( sbListOptionsSizer, 0, wxEXPAND, 5 );
	
	wxString m_OutputFormCtrlChoices[] = { _("List"), _("List for spreadsheet import (by ref)"), _("List for spreadsheet import (by grouped ref)"), _("List for spreadsheet import (by value)") };
	int m_OutputFormCtrlNChoices = sizeof( m_OutputFormCtrlChoices ) / sizeof( wxString );
	m_OutputFormCtrl = new wxRadioBox( this, ID_RADIOBOX_SELECT_FORMAT, _("Output format:"), wxDefaultPosition, wxDefaultSize, m_OutputFormCtrlNChoices, m_OutputFormCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_OutputFormCtrl->SetSelection( 1 );
	sbOptionsSizer->Add( m_OutputFormCtrl, 0, wxEXPAND|wxTOP, 5 );
	
	wxString m_OutputSeparatorCtrlChoices[] = { _("Tab"), _(";"), _(",") };
	int m_OutputSeparatorCtrlNChoices = sizeof( m_OutputSeparatorCtrlChoices ) / sizeof( wxString );
	m_OutputSeparatorCtrl = new wxRadioBox( this, wxID_ANY, _("Field separator for spreadsheet import:"), wxDefaultPosition, wxDefaultSize, m_OutputSeparatorCtrlNChoices, m_OutputSeparatorCtrlChoices, 1, wxRA_SPECIFY_ROWS );
	m_OutputSeparatorCtrl->SetSelection( 0 );
	sbOptionsSizer->Add( m_OutputSeparatorCtrl, 0, wxEXPAND|wxTOP, 5 );
	
	m_GetListBrowser = new wxCheckBox( this, wxID_ANY, _("Launch list browser"), wxDefaultPosition, wxDefaultSize, 0 );
	sbOptionsSizer->Add( m_GetListBrowser, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerUpper->Add( sbOptionsSizer, 3, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbAddToListSelectionSizer;
	sbAddToListSelectionSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Add to list:") ), wxVERTICAL );
	
	m_AddLocationField = new wxCheckBox( this, wxID_ANY, _("Component location"), wxDefaultPosition, wxDefaultSize, 0 );
	sbAddToListSelectionSizer->Add( m_AddLocationField, 0, wxALL, 5 );
	
	wxStaticBoxSizer* sbFixedFieldsSizer;
	sbFixedFieldsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("System Fields:") ), wxVERTICAL );
	
	m_AddDatasheetField = new wxCheckBox( this, wxID_ANY, _("Datasheet"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFixedFieldsSizer->Add( m_AddDatasheetField, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_AddFootprintField = new wxCheckBox( this, wxID_ANY, _("Footprint"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFixedFieldsSizer->Add( m_AddFootprintField, 0, wxALL|wxEXPAND, 5 );
	
	
	sbAddToListSelectionSizer->Add( sbFixedFieldsSizer, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbUsersFiledsSizer;
	sbUsersFiledsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Users fields:") ), wxVERTICAL );
	
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
	
	
	sbAddToListSelectionSizer->Add( sbUsersFiledsSizer, 0, wxEXPAND|wxTOP, 5 );
	
	m_AddAllFields = new wxCheckBox( this, wxID_ANY, _("All existing user fields"), wxDefaultPosition, wxDefaultSize, 0 );
	sbAddToListSelectionSizer->Add( m_AddAllFields, 0, wxALL, 5 );
	
	
	bRightSizer->Add( sbAddToListSelectionSizer, 1, wxEXPAND, 5 );
	
	
	bSizerUpper->Add( bRightSizer, 2, wxALL|wxEXPAND, 5 );
	
	
	bMainSizer->Add( bSizerUpper, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bMainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	// Connect Events
	m_OutputFormCtrl->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_BUILD_BOM_BASE::OnRadioboxSelectFormatSelected ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BUILD_BOM_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BUILD_BOM_BASE::OnOkClick ), NULL, this );
}

DIALOG_BUILD_BOM_BASE::~DIALOG_BUILD_BOM_BASE()
{
	// Disconnect Events
	m_OutputFormCtrl->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_BUILD_BOM_BASE::OnRadioboxSelectFormatSelected ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BUILD_BOM_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BUILD_BOM_BASE::OnOkClick ), NULL, this );
	
}
