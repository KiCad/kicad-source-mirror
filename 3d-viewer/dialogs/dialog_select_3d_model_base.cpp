///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_select_3d_model_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SELECT_3D_MODEL_BASE::DIALOG_SELECT_3D_MODEL_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );

	bSizerUpper->SetMinSize( wxSize( -1,400 ) );
	m_splitterWin = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	m_splitterWin->SetSashGravity( 0.35 );
	m_splitterWin->Connect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_SELECT_3D_MODEL_BASE::m_splitterWinOnIdle ), NULL, this );

	m_panelLeft = new wxPanel( m_splitterWin, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxVERTICAL );

	m_FileTree = new wxGenericDirCtrl( m_panelLeft, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxDIRCTRL_3D_INTERNAL|wxDIRCTRL_EDIT_LABELS|wxDIRCTRL_SHOW_FILTERS|wxBORDER_DEFAULT, wxEmptyString, 0 );

	m_FileTree->ShowHidden( false );
	m_FileTree->SetMinSize( wxSize( 300,-1 ) );

	bSizerLeft->Add( m_FileTree, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );


	bSizerLeft->Add( bSizer7, 0, wxEXPAND, 5 );


	m_panelLeft->SetSizer( bSizerLeft );
	m_panelLeft->Layout();
	bSizerLeft->Fit( m_panelLeft );
	m_pane3Dviewer = new wxPanel( m_splitterWin, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Sizer3Dviewer = new wxBoxSizer( wxVERTICAL );


	m_pane3Dviewer->SetSizer( m_Sizer3Dviewer );
	m_pane3Dviewer->Layout();
	m_Sizer3Dviewer->Fit( m_pane3Dviewer );
	m_splitterWin->SplitVertically( m_panelLeft, m_pane3Dviewer, 300 );
	bSizerUpper->Add( m_splitterWin, 1, wxEXPAND, 5 );


	bSizerMain->Add( bSizerUpper, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerLower;
	bSizerLower = new wxBoxSizer( wxHORIZONTAL );

	m_stDirChoice = new wxStaticText( this, wxID_ANY, _("Available paths:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stDirChoice->Wrap( -1 );
	bSizerLower->Add( m_stDirChoice, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxArrayString m_dirChoicesChoices;
	m_dirChoices = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dirChoicesChoices, 0 );
	m_dirChoices->SetSelection( 0 );
	bSizerLower->Add( m_dirChoices, 1, wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_cfgPathsButt = new wxButton( this, wxID_ANY, _("Configure Paths"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLower->Add( m_cfgPathsButt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerMain->Add( bSizerLower, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );

	m_EmbedModelCb = new wxCheckBox( this, wxID_ANY, _("Embed model"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( m_EmbedModelCb, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bSizer6->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizer6->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	bSizerMain->Add( bSizer6, 0, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_FileTree->Connect( wxEVT_DIRCTRL_FILEACTIVATED, wxCommandEventHandler( DIALOG_SELECT_3D_MODEL_BASE::OnFileActivated ), NULL, this );
	m_FileTree->Connect( wxEVT_DIRCTRL_SELECTIONCHANGED, wxCommandEventHandler( DIALOG_SELECT_3D_MODEL_BASE::OnSelectionChanged ), NULL, this );
	m_dirChoices->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SELECT_3D_MODEL_BASE::SetRootDir ), NULL, this );
	m_cfgPathsButt->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SELECT_3D_MODEL_BASE::Cfg3DPaths ), NULL, this );
}

DIALOG_SELECT_3D_MODEL_BASE::~DIALOG_SELECT_3D_MODEL_BASE()
{
	// Disconnect Events
	m_FileTree->Disconnect( wxEVT_DIRCTRL_FILEACTIVATED, wxCommandEventHandler( DIALOG_SELECT_3D_MODEL_BASE::OnFileActivated ), NULL, this );
	m_FileTree->Disconnect( wxEVT_DIRCTRL_SELECTIONCHANGED, wxCommandEventHandler( DIALOG_SELECT_3D_MODEL_BASE::OnSelectionChanged ), NULL, this );
	m_dirChoices->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SELECT_3D_MODEL_BASE::SetRootDir ), NULL, this );
	m_cfgPathsButt->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SELECT_3D_MODEL_BASE::Cfg3DPaths ), NULL, this );

}
