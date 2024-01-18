///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_plugin_settings_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PLUGIN_SETTINGS_BASE::PANEL_PLUGIN_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerServer;
	sbSizerServer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("KiCad API") ), wxVERTICAL );

	m_staticText3 = new wxStaticText( sbSizerServer->GetStaticBox(), wxID_ANY, _("When the KiCad API is enabled, plugins and other software running on this computer can connect to KiCad."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	sbSizerServer->Add( m_staticText3, 0, wxALL, 5 );

	m_cbEnableApi = new wxCheckBox( sbSizerServer->GetStaticBox(), wxID_ANY, _("Enable KiCad API"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbEnableApi->SetToolTip( _("Enable the KiCad API.  Doing so will allow third-party software running on your computer to access KiCad.") );

	sbSizerServer->Add( m_cbEnableApi, 0, wxALL, 5 );

	m_stApiStatus = new wxStaticText( sbSizerServer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_stApiStatus->Wrap( -1 );
	sbSizerServer->Add( m_stApiStatus, 0, wxALL, 5 );


	bSizer8->Add( sbSizerServer, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerPython;
	sbSizerPython = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Python Interpreter") ), wxVERTICAL );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText2 = new wxStaticText( sbSizerPython->GetStaticBox(), wxID_ANY, _("Path to Python interpreter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bSizer4->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_pickerPythonInterpreter = new wxFilePickerCtrl( sbSizerPython->GetStaticBox(), wxID_ANY, wxEmptyString, _("Select the path to a Python interpreter"), _("*.*"), wxDefaultPosition, wxDefaultSize, wxFLP_DEFAULT_STYLE|wxFLP_USE_TEXTCTRL );
	bSizer4->Add( m_pickerPythonInterpreter, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_btnDetectAutomatically = new wxButton( sbSizerPython->GetStaticBox(), wxID_ANY, _("Detect Automatically"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_btnDetectAutomatically, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	sbSizerPython->Add( bSizer4, 0, wxEXPAND, 5 );

	m_stPythonStatus = new wxStaticText( sbSizerPython->GetStaticBox(), wxID_ANY, _("No Python interpreter chosen; external Python plugins will not be available"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stPythonStatus->Wrap( -1 );
	m_stPythonStatus->SetToolTip( _("Python interpreter status") );

	sbSizerPython->Add( m_stPythonStatus, 0, wxALL, 5 );


	bSizer8->Add( sbSizerPython, 0, wxALL|wxEXPAND, 5 );


	bSizer8->Add( 0, 0, 1, wxEXPAND, 5 );


	bPanelSizer->Add( bSizer8, 1, wxEXPAND, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );

	// Connect Events
	m_cbEnableApi->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_PLUGIN_SETTINGS_BASE::OnEnableApiChecked ), NULL, this );
	m_pickerPythonInterpreter->Connect( wxEVT_COMMAND_FILEPICKER_CHANGED, wxFileDirPickerEventHandler( PANEL_PLUGIN_SETTINGS_BASE::OnPythonInterpreterChanged ), NULL, this );
	m_btnDetectAutomatically->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PLUGIN_SETTINGS_BASE::OnBtnDetectAutomaticallyClicked ), NULL, this );
}

PANEL_PLUGIN_SETTINGS_BASE::~PANEL_PLUGIN_SETTINGS_BASE()
{
	// Disconnect Events
	m_cbEnableApi->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_PLUGIN_SETTINGS_BASE::OnEnableApiChecked ), NULL, this );
	m_pickerPythonInterpreter->Disconnect( wxEVT_COMMAND_FILEPICKER_CHANGED, wxFileDirPickerEventHandler( PANEL_PLUGIN_SETTINGS_BASE::OnPythonInterpreterChanged ), NULL, this );
	m_btnDetectAutomatically->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PLUGIN_SETTINGS_BASE::OnBtnDetectAutomaticallyClicked ), NULL, this );

}
