///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_export_3Dfiles_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXPORT_3DFILE_BASE::DIALOG_EXPORT_3DFILE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxVERTICAL );
	
	bUpperSizer->SetMinSize( wxSize( 450,-1 ) ); 
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Wrml main file filename:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bUpperSizer->Add( m_staticText1, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_filePicker = new wxFilePickerCtrl( this, wxID_ANY, wxEmptyString, _("Save VRML Board File"), wxT("*.wrl"), wxDefaultPosition, wxDefaultSize, wxFLP_DEFAULT_STYLE|wxFLP_SAVE );
	bUpperSizer->Add( m_filePicker, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Wrml 3D footprints shapes subdir:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	bUpperSizer->Add( m_staticText3, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_SubdirNameCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bUpperSizer->Add( m_SubdirNameCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bSizer1->Add( bUpperSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxString m_rbSelectUnitsChoices[] = { _("Inch"), _("mm"), _("Meter") };
	int m_rbSelectUnitsNChoices = sizeof( m_rbSelectUnitsChoices ) / sizeof( wxString );
	m_rbSelectUnits = new wxRadioBox( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, m_rbSelectUnitsNChoices, m_rbSelectUnitsChoices, 1, wxRA_SPECIFY_COLS );
	m_rbSelectUnits->SetSelection( 0 );
	bLowerSizer->Add( m_rbSelectUnits, 1, wxALL|wxEXPAND, 5 );
	
	wxString m_rb3DFilesOptionChoices[] = { _("Copy 3D Shapes Files in Subdir"), _("Use Absolute Path in Wrml File ") };
	int m_rb3DFilesOptionNChoices = sizeof( m_rb3DFilesOptionChoices ) / sizeof( wxString );
	m_rb3DFilesOption = new wxRadioBox( this, wxID_ANY, _("3D Shapes Files Option:"), wxDefaultPosition, wxDefaultSize, m_rb3DFilesOptionNChoices, m_rb3DFilesOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_rb3DFilesOption->SetSelection( 0 );
	bLowerSizer->Add( m_rb3DFilesOption, 1, wxALL|wxEXPAND, 5 );
	
	bSizer1->Add( bLowerSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer1->Add( m_staticline1, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	bSizer1->Add( m_sdbSizer1, 0, wxEXPAND|wxALL, 5 );
	
	this->SetSizer( bSizer1 );
	this->Layout();
	
	// Connect Events
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_3DFILE_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_3DFILE_BASE::OnOkClick ), NULL, this );
}

DIALOG_EXPORT_3DFILE_BASE::~DIALOG_EXPORT_3DFILE_BASE()
{
	// Disconnect Events
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_3DFILE_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_3DFILE_BASE::OnOkClick ), NULL, this );
}
