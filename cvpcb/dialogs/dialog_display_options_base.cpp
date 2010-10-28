///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_display_options_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxString m_EdgesDisplayOptionChoices[] = { _("Line"), _("Filled"), _("Sketch") };
	int m_EdgesDisplayOptionNChoices = sizeof( m_EdgesDisplayOptionChoices ) / sizeof( wxString );
	m_EdgesDisplayOption = new wxRadioBox( this, ID_EDGE_SELECT, _("Edges:"), wxDefaultPosition, wxDefaultSize, m_EdgesDisplayOptionNChoices, m_EdgesDisplayOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_EdgesDisplayOption->SetSelection( 0 );
	bUpperSizer->Add( m_EdgesDisplayOption, 1, wxALL|wxEXPAND, 5 );
	
	wxString m_TextDisplayOptionChoices[] = { _("Line"), _("Filled"), _("Sketch") };
	int m_TextDisplayOptionNChoices = sizeof( m_TextDisplayOptionChoices ) / sizeof( wxString );
	m_TextDisplayOption = new wxRadioBox( this, ID_TEXT_SELECT, _("Texts:"), wxDefaultPosition, wxDefaultSize, m_TextDisplayOptionNChoices, m_TextDisplayOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_TextDisplayOption->SetSelection( 0 );
	bUpperSizer->Add( m_TextDisplayOption, 1, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Pads:") ), wxVERTICAL );
	
	m_IsShowPadFill = new wxCheckBox( this, ID_PADFILL_OPT, _("Fill &pad"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbSizer1->Add( m_IsShowPadFill, 0, wxALL|wxEXPAND, 5 );
	
	m_IsShowPadNum = new wxCheckBox( this, wxID_ANY, _("Show pad &number"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbSizer1->Add( m_IsShowPadNum, 0, wxALL|wxEXPAND, 5 );
	
	bUpperSizer->Add( sbSizer1, 1, wxEXPAND|wxALL, 5 );
	
	bSizerMain->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Apply = new wxButton( this, wxID_APPLY );
	m_sdbSizer1->AddButton( m_sdbSizer1Apply );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	bSizerMain->Add( m_sdbSizer1, 0, wxEXPAND|wxALL, 5 );
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	// Connect Events
	m_sdbSizer1Apply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnApplyClick ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnOkClick ), NULL, this );
}

DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::~DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE()
{
	// Disconnect Events
	m_sdbSizer1Apply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnApplyClick ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnOkClick ), NULL, this );
}
