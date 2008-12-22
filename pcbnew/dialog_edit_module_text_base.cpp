///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_edit_module_text_base.h"

///////////////////////////////////////////////////////////////////////////

DialogEditModuleText_base::DialogEditModuleText_base( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_ModuleInfoText = new wxStaticText( this, wxID_ANY, _("Module %s (%s) orient %.1f"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ModuleInfoText->Wrap( -1 );
	bMainSizer->Add( m_ModuleInfoText, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxALL|wxEXPAND, 5 );
	
	m_TextDataTitle = new wxStaticText( this, wxID_ANY, _("Reference:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextDataTitle->Wrap( -1 );
	bMainSizer->Add( m_TextDataTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Name = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bMainSizer->Add( m_Name, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );
	
	m_SizeXTitle = new wxStaticText( this, wxID_ANY, _("Size X"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXTitle->Wrap( -1 );
	bSizer3->Add( m_SizeXTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_TxtSizeCtrlX = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_TxtSizeCtrlX, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_SizeYTitle = new wxStaticText( this, wxID_ANY, _("Size Y"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeYTitle->Wrap( -1 );
	bSizer3->Add( m_SizeYTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_TxtSizeCtrlY = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_TxtSizeCtrlY, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_PosXTitle = new wxStaticText( this, wxID_ANY, _("Offset X"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PosXTitle->Wrap( -1 );
	bSizer3->Add( m_PosXTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_TxtPosCtrlX = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_TxtPosCtrlX, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_PosYTitle = new wxStaticText( this, wxID_ANY, _("Offset Y"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PosYTitle->Wrap( -1 );
	bSizer3->Add( m_PosYTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_TxtPosCtrlY = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_TxtPosCtrlY, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	bSizer2->Add( bSizer3, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );
	
	m_WidthTitle = new wxStaticText( this, wxID_ANY, _("Thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	m_WidthTitle->Wrap( -1 );
	bSizer4->Add( m_WidthTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_TxtWidthCtlr = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_TxtWidthCtlr, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	wxString m_OrientChoices[] = { _("horizontal"), _("vertical") };
	int m_OrientNChoices = sizeof( m_OrientChoices ) / sizeof( wxString );
	m_Orient = new wxRadioBox( this, wxID_ANY, _("Orientation"), wxDefaultPosition, wxDefaultSize, m_OrientNChoices, m_OrientChoices, 1, wxRA_SPECIFY_COLS );
	m_Orient->SetSelection( 0 );
	bSizer4->Add( m_Orient, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_ShowChoices[] = { _("Visible"), _("Invisible") };
	int m_ShowNChoices = sizeof( m_ShowChoices ) / sizeof( wxString );
	m_Show = new wxRadioBox( this, wxID_ANY, _("Display"), wxDefaultPosition, wxDefaultSize, m_ShowNChoices, m_ShowChoices, 1, wxRA_SPECIFY_COLS );
	m_Show->SetSelection( 0 );
	bSizer4->Add( m_Show, 0, wxALL|wxEXPAND, 5 );
	
	bSizer2->Add( bSizer4, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );
	
	wxString m_StyleChoices[] = { _("Normal"), _("Italic") };
	int m_StyleNChoices = sizeof( m_StyleChoices ) / sizeof( wxString );
	m_Style = new wxRadioBox( this, wxID_ANY, _("Style"), wxDefaultPosition, wxDefaultSize, m_StyleNChoices, m_StyleChoices, 1, wxRA_SPECIFY_COLS );
	m_Style->SetSelection( 0 );
	bSizer5->Add( m_Style, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer5->Add( 10, 10, 0, 0, 5 );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonOK->SetDefault(); 
	m_buttonOK->SetForegroundColour( wxColour( 202, 0, 0 ) );
	
	bSizer5->Add( m_buttonOK, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_buttonCANCEL = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonCANCEL->SetForegroundColour( wxColour( 0, 0, 220 ) );
	
	bSizer5->Add( m_buttonCANCEL, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer2->Add( bSizer5, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	bMainSizer->Add( bSizer2, 1, wxEXPAND, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DialogEditModuleText_base::OnInitDialog ) );
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogEditModuleText_base::OnOkClick ), NULL, this );
	m_buttonCANCEL->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogEditModuleText_base::OnCancelClick ), NULL, this );
}

DialogEditModuleText_base::~DialogEditModuleText_base()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DialogEditModuleText_base::OnInitDialog ) );
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogEditModuleText_base::OnOkClick ), NULL, this );
	m_buttonCANCEL->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogEditModuleText_base::OnCancelClick ), NULL, this );
}
