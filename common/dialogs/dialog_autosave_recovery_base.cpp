///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_autosave_recovery_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_AUTOSAVE_RECOVERY_BASE::DIALOG_AUTOSAVE_RECOVERY_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_explanation = new wxStaticText( this, wxID_ANY, _("KiCad found auto-saved changes from a previous session. This usually means KiCad closed unexpectedly while you had unsaved work."), wxDefaultPosition, wxDefaultSize, 0 );
	m_explanation->Wrap( 560 );
	bMainSizer->Add( m_explanation, 0, wxALL|wxEXPAND, 10 );

	m_fileList = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_HRULES|wxLC_REPORT|wxLC_VRULES );
	bMainSizer->Add( m_fileList, 1, wxALL|wxEXPAND, 10 );

	wxBoxSizer* bButtonSizer;
	bButtonSizer = new wxBoxSizer( wxHORIZONTAL );

	m_btnRestore = new wxButton( this, wxID_ANY, _("Restore auto-saved"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btnRestore->SetToolTip( _("Replaces your saved file with the auto-saved version. The previous saved content is discarded.") );

	bButtonSizer->Add( m_btnRestore, 0, wxALL, 5 );

	m_btnKeepCurrent = new wxButton( this, wxID_ANY, _("Keep current"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btnKeepCurrent->SetToolTip( _("Keeps your saved file as-is and discards the auto-saved changes.") );

	bButtonSizer->Add( m_btnKeepCurrent, 0, wxALL, 5 );

	m_btnKeepBoth = new wxButton( this, wxID_ANY, _("Keep both"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btnKeepBoth->SetToolTip( _("Keeps your saved file and writes the auto-saved version alongside it as a separate timestamped file you can review later.") );

	bButtonSizer->Add( m_btnKeepBoth, 0, wxALL, 5 );


	bButtonSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_btnCancel = new wxButton( this, wxID_CANCEL, _("Cancel open"), wxDefaultPosition, wxDefaultSize, 0 );

	m_btnCancel->SetDefault();
	m_btnCancel->SetToolTip( _("Closes this dialog without making any changes. The auto-saved files stay on disk and will be offered again next time you open the project.") );

	bButtonSizer->Add( m_btnCancel, 0, wxALL, 5 );


	bMainSizer->Add( bButtonSizer, 0, wxALL|wxEXPAND, 10 );


	this->SetSizer( bMainSizer );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_btnRestore->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_AUTOSAVE_RECOVERY_BASE::OnRestore ), NULL, this );
	m_btnKeepCurrent->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_AUTOSAVE_RECOVERY_BASE::OnKeepCurrent ), NULL, this );
	m_btnKeepBoth->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_AUTOSAVE_RECOVERY_BASE::OnKeepBoth ), NULL, this );
	m_btnCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_AUTOSAVE_RECOVERY_BASE::OnCancel ), NULL, this );
}

DIALOG_AUTOSAVE_RECOVERY_BASE::~DIALOG_AUTOSAVE_RECOVERY_BASE()
{
	// Disconnect Events
	m_btnRestore->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_AUTOSAVE_RECOVERY_BASE::OnRestore ), NULL, this );
	m_btnKeepCurrent->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_AUTOSAVE_RECOVERY_BASE::OnKeepCurrent ), NULL, this );
	m_btnKeepBoth->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_AUTOSAVE_RECOVERY_BASE::OnKeepBoth ), NULL, this );
	m_btnCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_AUTOSAVE_RECOVERY_BASE::OnCancel ), NULL, this );

}
