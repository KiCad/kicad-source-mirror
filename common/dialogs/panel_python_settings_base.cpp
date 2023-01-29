///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_python_settings_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PYTHON_SETTINGS_BASE::PANEL_PYTHON_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Python Interpreter") ), wxVERTICAL );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText2 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Path to Python interpreter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bSizer4->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_pickerPythonInterpreter = new wxFilePickerCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, _("Select the path to a Python interpreter"), _("*.*"), wxDefaultPosition, wxDefaultSize, wxFLP_DEFAULT_STYLE|wxFLP_USE_TEXTCTRL );
	bSizer4->Add( m_pickerPythonInterpreter, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_btnDetectAutomatically = new wxButton( sbSizer1->GetStaticBox(), wxID_ANY, _("Detect Automatically"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_btnDetectAutomatically, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	sbSizer1->Add( bSizer4, 0, wxEXPAND, 5 );

	m_stPythonStatus = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("No Python interpreter chosen; external Python plugins will not be available"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stPythonStatus->Wrap( -1 );
	m_stPythonStatus->SetToolTip( _("Python interpreter status") );

	sbSizer1->Add( m_stPythonStatus, 0, wxALL, 5 );


	bSizer8->Add( sbSizer1, 0, wxALL|wxEXPAND, 5 );


	bSizer8->Add( 0, 0, 1, wxEXPAND, 5 );


	bPanelSizer->Add( bSizer8, 1, wxEXPAND, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );

	// Connect Events
	m_pickerPythonInterpreter->Connect( wxEVT_COMMAND_FILEPICKER_CHANGED, wxFileDirPickerEventHandler( PANEL_PYTHON_SETTINGS_BASE::OnPythonInterpreterChanged ), NULL, this );
	m_btnDetectAutomatically->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PYTHON_SETTINGS_BASE::OnBtnDetectAutomaticallyClicked ), NULL, this );
}

PANEL_PYTHON_SETTINGS_BASE::~PANEL_PYTHON_SETTINGS_BASE()
{
	// Disconnect Events
	m_pickerPythonInterpreter->Disconnect( wxEVT_COMMAND_FILEPICKER_CHANGED, wxFileDirPickerEventHandler( PANEL_PYTHON_SETTINGS_BASE::OnPythonInterpreterChanged ), NULL, this );
	m_btnDetectAutomatically->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PYTHON_SETTINGS_BASE::OnBtnDetectAutomaticallyClicked ), NULL, this );

}
