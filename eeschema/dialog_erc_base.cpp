///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_erc_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_ERC_BASE, wxDialog )
	EVT_BUTTON( ID_ERC_CMP, DIALOG_ERC_BASE::_wxFB_OnErcCmpClick )
	EVT_BUTTON( ID_ERASE_DRC_MARKERS, DIALOG_ERC_BASE::_wxFB_OnEraseDrcMarkersClick )
	EVT_BUTTON( wxID_CANCEL, DIALOG_ERC_BASE::_wxFB_OnCancelClick )
	EVT_BUTTON( ID_RESET_MATRIX, DIALOG_ERC_BASE::_wxFB_OnResetMatrixClick )
END_EVENT_TABLE()

DIALOG_ERC_BASE::DIALOG_ERC_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );
	
	m_NoteBook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_PanelERC = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bercSizer;
	bercSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sdiagSizer;
	sdiagSizer = new wxStaticBoxSizer( new wxStaticBox( m_PanelERC, wxID_ANY, _("Erc File Report:") ), wxVERTICAL );
	
	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 3, 2, 0, 0 );
	
	m_ErcTotalErrorsText = new wxStaticText( m_PanelERC, wxID_ANY, _("Total Errors Count:  "), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
	m_ErcTotalErrorsText->Wrap( -1 );
	gSizer1->Add( m_ErcTotalErrorsText, 0, wxALL, 5 );
	
	m_TotalErrCount = new wxStaticText( m_PanelERC, wxID_ANY, _("0000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TotalErrCount->Wrap( -1 );
	gSizer1->Add( m_TotalErrCount, 0, wxALL, 5 );
	
	m_WarnErcErrorsText = new wxStaticText( m_PanelERC, wxID_ANY, _("Warnings Count:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
	m_WarnErcErrorsText->Wrap( -1 );
	gSizer1->Add( m_WarnErcErrorsText, 0, wxALL, 5 );
	
	m_LastWarningCount = new wxStaticText( m_PanelERC, wxID_ANY, _("0000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LastWarningCount->Wrap( -1 );
	gSizer1->Add( m_LastWarningCount, 0, wxALL, 5 );
	
	m_LastErrCountText = new wxStaticText( m_PanelERC, wxID_ANY, _("Errors Count:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
	m_LastErrCountText->Wrap( -1 );
	gSizer1->Add( m_LastErrCountText, 0, wxALL, 5 );
	
	m_LastErrCount = new wxStaticText( m_PanelERC, wxID_ANY, _("0000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LastErrCount->Wrap( -1 );
	gSizer1->Add( m_LastErrCount, 0, wxALL, 5 );
	
	sdiagSizer->Add( gSizer1, 0, 0, 5 );
	
	bupperSizer->Add( sdiagSizer, 0, 0, 5 );
	
	
	bupperSizer->Add( 10, 10, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bbuttonsSizer;
	bbuttonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonERC = new wxButton( m_PanelERC, ID_ERC_CMP, _("&Test Erc"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonERC, 0, wxALL|wxEXPAND, 5 );
	
	m_buttondelmarkers = new wxButton( m_PanelERC, ID_ERASE_DRC_MARKERS, _("&Del Markers"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttondelmarkers, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonClose = new wxButton( m_PanelERC, wxID_CANCEL, _("&Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonClose, 0, wxALL|wxEXPAND, 5 );
	
	bupperSizer->Add( bbuttonsSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bercSizer->Add( bupperSizer, 0, wxEXPAND, 5 );
	
	m_WriteResultOpt = new wxCheckBox( m_PanelERC, wxID_ANY, _("Write ERC report"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bercSizer->Add( m_WriteResultOpt, 0, wxALL, 5 );
	
	m_staticline2 = new wxStaticLine( m_PanelERC, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bercSizer->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );
	
	m_textMessage = new wxStaticText( m_PanelERC, wxID_ANY, _("Messages:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textMessage->Wrap( -1 );
	bercSizer->Add( m_textMessage, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_MessagesList = new wxTextCtrl( m_PanelERC, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_MessagesList->SetMinSize( wxSize( 580,300 ) );
	
	bercSizer->Add( m_MessagesList, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_PanelERC->SetSizer( bercSizer );
	m_PanelERC->Layout();
	bercSizer->Fit( m_PanelERC );
	m_NoteBook->AddPage( m_PanelERC, _("ERC"), true );
	m_PanelERCOptions = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_PanelMatrixSizer = new wxBoxSizer( wxVERTICAL );
	
	m_ResetOptButton = new wxButton( m_PanelERCOptions, ID_RESET_MATRIX, _("Reset"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PanelMatrixSizer->Add( m_ResetOptButton, 0, wxALL, 5 );
	
	m_staticline1 = new wxStaticLine( m_PanelERCOptions, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_PanelMatrixSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_MatrixSizer = new wxBoxSizer( wxVERTICAL );
	
	m_PanelMatrixSizer->Add( m_MatrixSizer, 1, wxEXPAND, 5 );
	
	m_PanelERCOptions->SetSizer( m_PanelMatrixSizer );
	m_PanelERCOptions->Layout();
	m_PanelMatrixSizer->Fit( m_PanelERCOptions );
	m_NoteBook->AddPage( m_PanelERCOptions, _("Options"), false );
	
	bSizer1->Add( m_NoteBook, 1, wxEXPAND | wxALL, 5 );
	
	this->SetSizer( bSizer1 );
	this->Layout();
}

DIALOG_ERC_BASE::~DIALOG_ERC_BASE()
{
}
