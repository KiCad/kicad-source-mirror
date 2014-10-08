///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  6 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_erc_listbox.h"

#include "dialog_erc_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_ERC_BASE, DIALOG_SHIM )
	EVT_CLOSE( DIALOG_ERC_BASE::_wxFB_OnCloseErcDialog )
	EVT_LISTBOX( ID_MAKER_HTMLLISTBOX, DIALOG_ERC_BASE::_wxFB_OnLeftClickMarkersList )
	EVT_LISTBOX_DCLICK( ID_MAKER_HTMLLISTBOX, DIALOG_ERC_BASE::_wxFB_OnLeftDblClickMarkersList )
	EVT_BUTTON( ID_ERASE_DRC_MARKERS, DIALOG_ERC_BASE::_wxFB_OnEraseDrcMarkersClick )
	EVT_BUTTON( ID_ERC_CMP, DIALOG_ERC_BASE::_wxFB_OnErcCmpClick )
	EVT_BUTTON( wxID_CANCEL, DIALOG_ERC_BASE::_wxFB_OnButtonCloseClick )
	EVT_BUTTON( ID_RESET_MATRIX, DIALOG_ERC_BASE::_wxFB_OnResetMatrixClick )
END_EVENT_TABLE()

DIALOG_ERC_BASE::DIALOG_ERC_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
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
	sdiagSizer = new wxStaticBoxSizer( new wxStaticBox( m_PanelERC, wxID_ANY, _("ERC Report:") ), wxVERTICAL );
	
	wxGridSizer* gSizeDiag;
	gSizeDiag = new wxGridSizer( 3, 2, 0, 0 );
	
	m_ErcTotalErrorsText = new wxStaticText( m_PanelERC, wxID_ANY, _("Total:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
	m_ErcTotalErrorsText->Wrap( -1 );
	gSizeDiag->Add( m_ErcTotalErrorsText, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TotalErrCount = new wxTextCtrl( m_PanelERC, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	gSizeDiag->Add( m_TotalErrCount, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_WarnErcErrorsText = new wxStaticText( m_PanelERC, wxID_ANY, _("Warnings:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
	m_WarnErcErrorsText->Wrap( -1 );
	gSizeDiag->Add( m_WarnErcErrorsText, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_LastWarningCount = new wxTextCtrl( m_PanelERC, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	gSizeDiag->Add( m_LastWarningCount, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_LastErrCountText = new wxStaticText( m_PanelERC, wxID_ANY, _("Errors:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
	m_LastErrCountText->Wrap( -1 );
	gSizeDiag->Add( m_LastErrCountText, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_LastErrCount = new wxTextCtrl( m_PanelERC, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	gSizeDiag->Add( m_LastErrCount, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sdiagSizer->Add( gSizeDiag, 0, 0, 5 );
	
	m_WriteResultOpt = new wxCheckBox( m_PanelERC, wxID_ANY, _("Create ERC file report"), wxDefaultPosition, wxDefaultSize, 0 );
	sdiagSizer->Add( m_WriteResultOpt, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	
	bupperSizer->Add( sdiagSizer, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP, 5 );
	
	wxBoxSizer* bSizerMessages;
	bSizerMessages = new wxBoxSizer( wxVERTICAL );
	
	m_titleMessages = new wxStaticText( m_PanelERC, wxID_ANY, _("Messages:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_titleMessages->Wrap( -1 );
	bSizerMessages->Add( m_titleMessages, 0, wxRIGHT|wxLEFT, 5 );
	
	m_MessagesList = new wxTextCtrl( m_PanelERC, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_MessagesList->SetMaxLength( 0 ); 
	bSizerMessages->Add( m_MessagesList, 1, wxEXPAND|wxLEFT, 5 );
	
	
	bupperSizer->Add( bSizerMessages, 1, wxEXPAND, 5 );
	
	
	bercSizer->Add( bupperSizer, 0, wxALL|wxEXPAND, 5 );
	
	m_textMarkers = new wxStaticText( m_PanelERC, wxID_ANY, _("Error list:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textMarkers->Wrap( -1 );
	bercSizer->Add( m_textMarkers, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_MarkersList = new ERC_HTML_LISTBOX( m_PanelERC, ID_MAKER_HTMLLISTBOX, wxDefaultPosition, wxDefaultSize, 0, NULL, 0|wxSIMPLE_BORDER ); 
	m_MarkersList->SetMinSize( wxSize( 500,250 ) );
	
	bercSizer->Add( m_MarkersList, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bbuttonsSizer;
	bbuttonsSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttondelmarkers = new wxButton( m_PanelERC, ID_ERASE_DRC_MARKERS, _("&Delete Markers"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttondelmarkers, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonERC = new wxButton( m_PanelERC, ID_ERC_CMP, _("&Run"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonERC->SetDefault(); 
	bbuttonsSizer->Add( m_buttonERC, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonClose = new wxButton( m_PanelERC, wxID_CANCEL, _("&Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonClose, 0, wxALL|wxEXPAND, 5 );
	
	
	bercSizer->Add( bbuttonsSizer, 0, wxALIGN_RIGHT, 5 );
	
	
	m_PanelERC->SetSizer( bercSizer );
	m_PanelERC->Layout();
	bercSizer->Fit( m_PanelERC );
	m_NoteBook->AddPage( m_PanelERC, _("ERC"), true );
	m_PanelERCOptions = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* m_panelMatrixSizer;
	m_panelMatrixSizer = new wxBoxSizer( wxVERTICAL );
	
	m_ResetOptButton = new wxButton( m_PanelERCOptions, ID_RESET_MATRIX, _("Reset"), wxDefaultPosition, wxDefaultSize, 0 );
	m_panelMatrixSizer->Add( m_ResetOptButton, 0, wxALIGN_RIGHT|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_matrixPanel = new wxPanel( m_PanelERCOptions, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelMatrixSizer->Add( m_matrixPanel, 1, wxEXPAND | wxALL, 5 );
	
	
	m_PanelERCOptions->SetSizer( m_panelMatrixSizer );
	m_PanelERCOptions->Layout();
	m_panelMatrixSizer->Fit( m_PanelERCOptions );
	m_NoteBook->AddPage( m_PanelERCOptions, _("Options"), false );
	
	bSizer1->Add( m_NoteBook, 1, wxEXPAND | wxALL, 5 );
	
	
	this->SetSizer( bSizer1 );
	this->Layout();
}

DIALOG_ERC_BASE::~DIALOG_ERC_BASE()
{
}
