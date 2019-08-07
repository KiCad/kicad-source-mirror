///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_erc_listbox.h"

#include "dialog_erc_base.h"

///////////////////////////////////////////////////////////////////////////

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
	gSizeDiag = new wxGridSizer( 3, 2, 5, 5 );
	
	m_ErcTotalErrorsText = new wxStaticText( sdiagSizer->GetStaticBox(), wxID_ANY, _("Total:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ErcTotalErrorsText->Wrap( -1 );
	gSizeDiag->Add( m_ErcTotalErrorsText, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TotalErrCount = new wxTextCtrl( sdiagSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	gSizeDiag->Add( m_TotalErrCount, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_WarnErcErrorsText = new wxStaticText( sdiagSizer->GetStaticBox(), wxID_ANY, _("Warnings:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_WarnErcErrorsText->Wrap( -1 );
	gSizeDiag->Add( m_WarnErcErrorsText, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_LastWarningCount = new wxTextCtrl( sdiagSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	gSizeDiag->Add( m_LastWarningCount, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_LastErrCountText = new wxStaticText( sdiagSizer->GetStaticBox(), wxID_ANY, _("Errors:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LastErrCountText->Wrap( -1 );
	gSizeDiag->Add( m_LastErrCountText, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_LastErrCount = new wxTextCtrl( sdiagSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	gSizeDiag->Add( m_LastErrCount, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sdiagSizer->Add( gSizeDiag, 0, wxALL|wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( sdiagSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sdiagSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_WriteResultOpt = new wxCheckBox( sdiagSizer->GetStaticBox(), wxID_ANY, _("Create ERC file report"), wxDefaultPosition, wxDefaultSize, 0 );
	sdiagSizer->Add( m_WriteResultOpt, 0, wxALL|wxBOTTOM|wxRIGHT|wxTOP, 5 );
	
	
	bupperSizer->Add( sdiagSizer, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	wxBoxSizer* bSizerMessages;
	bSizerMessages = new wxBoxSizer( wxVERTICAL );
	
	m_titleMessages = new wxStaticText( m_PanelERC, wxID_ANY, _("Messages:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_titleMessages->Wrap( -1 );
	m_titleMessages->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	bSizerMessages->Add( m_titleMessages, 0, wxRIGHT|wxLEFT, 12 );
	
	m_MessagesList = new wxTextCtrl( m_PanelERC, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_MessagesList->SetMinSize( wxSize( 180,-1 ) );
	
	bSizerMessages->Add( m_MessagesList, 1, wxEXPAND|wxBOTTOM|wxLEFT, 5 );
	
	
	bupperSizer->Add( bSizerMessages, 1, wxBOTTOM|wxEXPAND|wxRIGHT|wxTOP, 3 );
	
	
	bercSizer->Add( bupperSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 8 );
	
	m_textMarkers = new wxStaticText( m_PanelERC, wxID_ANY, _("Error List:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textMarkers->Wrap( -1 );
	m_textMarkers->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	bercSizer->Add( m_textMarkers, 0, wxLEFT|wxRIGHT, 20 );
	
	m_MarkersList = new ERC_HTML_LISTFRAME( m_PanelERC, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO|wxBORDER_SIMPLE );
	bercSizer->Add( m_MarkersList, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 8 );
	
	
	m_PanelERC->SetSizer( bercSizer );
	m_PanelERC->Layout();
	bercSizer->Fit( m_PanelERC );
	m_NoteBook->AddPage( m_PanelERC, _("ERC"), true );
	m_PanelERCOptions = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* m_panelMatrixSizer;
	m_panelMatrixSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( m_PanelERCOptions, wxID_ANY, _("Label to Label Connections") ), wxVERTICAL );
	
	m_cbTestSimilarLabels = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_ANY, _("Test similar labels"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTestSimilarLabels->SetToolTip( _("Similar labels are labels (inside a sheet) which differs only by upper/lower case") );
	
	sbSizer2->Add( m_cbTestSimilarLabels, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_cbTestUniqueGlbLabels = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_ANY, _("Test single instances of global labels"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTestUniqueGlbLabels->SetToolTip( _("Global labels are used to connect signals across the full hierarchy.\nThey are expected to be at least two labels with the same name.") );
	
	sbSizer2->Add( m_cbTestUniqueGlbLabels, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	
	m_panelMatrixSizer->Add( sbSizer2, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( m_PanelERCOptions, wxID_ANY, _("Pin to Pin Connections") ), wxVERTICAL );
	
	m_matrixPanel = new wxPanel( sbSizer3->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	sbSizer3->Add( m_matrixPanel, 1, wxEXPAND | wxALL, 5 );
	
	
	m_panelMatrixSizer->Add( sbSizer3, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer31;
	sbSizer31 = new wxStaticBoxSizer( new wxStaticBox( m_PanelERCOptions, wxID_ANY, _("Bus Connections") ), wxVERTICAL );
	
	m_cbCheckBusToNetConflicts = new wxCheckBox( sbSizer31->GetStaticBox(), wxID_ANY, _("Check that bus wires are not connected to hierarchical net pins and vice versa"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer31->Add( m_cbCheckBusToNetConflicts, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_cbCheckBusToBusConflicts = new wxCheckBox( sbSizer31->GetStaticBox(), wxID_ANY, _("Check that bus-to-bus connections have shared members"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer31->Add( m_cbCheckBusToBusConflicts, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_cbCheckBusEntries = new wxCheckBox( sbSizer31->GetStaticBox(), wxID_ANY, _("Check that nets are members of buses they graphically connect to"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer31->Add( m_cbCheckBusEntries, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_cbCheckBusDriverConflicts = new wxCheckBox( sbSizer31->GetStaticBox(), wxID_ANY, _("Check buses for conflicting drivers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer31->Add( m_cbCheckBusDriverConflicts, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	m_panelMatrixSizer->Add( sbSizer31, 0, wxALL|wxEXPAND, 5 );
	
	
	m_PanelERCOptions->SetSizer( m_panelMatrixSizer );
	m_PanelERCOptions->Layout();
	m_panelMatrixSizer->Fit( m_PanelERCOptions );
	m_NoteBook->AddPage( m_PanelERCOptions, _("Options"), false );
	
	bSizer1->Add( m_NoteBook, 1, wxEXPAND | wxALL, 5 );
	
	m_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttondelmarkers = new wxButton( this, ID_ERASE_DRC_MARKERS, _("Delete Markers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonsSizer->Add( m_buttondelmarkers, 0, wxALL|wxEXPAND, 5 );
	
	m_ResetOptButton = new wxButton( this, ID_RESET_MATRIX, _("Reset to Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonsSizer->Add( m_ResetOptButton, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	m_buttonsSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	m_buttonsSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer1->Add( m_buttonsSizer, 0, wxEXPAND|wxLEFT, 10 );
	
	
	this->SetSizer( bSizer1 );
	this->Layout();
	bSizer1->Fit( this );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_ERC_BASE::OnCloseErcDialog ) );
	m_NoteBook->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_ERC_BASE::OnUpdateUI ), NULL, this );
	m_MarkersList->Connect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( DIALOG_ERC_BASE::OnLeftClickMarkersList ), NULL, this );
	m_MarkersList->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_ERC_BASE::OnLeftDblClickMarkersList ), NULL, this );
	m_buttondelmarkers->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnEraseDrcMarkersClick ), NULL, this );
	m_ResetOptButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnResetMatrixClick ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnButtonCloseClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnErcCmpClick ), NULL, this );
}

DIALOG_ERC_BASE::~DIALOG_ERC_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_ERC_BASE::OnCloseErcDialog ) );
	m_NoteBook->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_ERC_BASE::OnUpdateUI ), NULL, this );
	m_MarkersList->Disconnect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( DIALOG_ERC_BASE::OnLeftClickMarkersList ), NULL, this );
	m_MarkersList->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_ERC_BASE::OnLeftDblClickMarkersList ), NULL, this );
	m_buttondelmarkers->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnEraseDrcMarkersClick ), NULL, this );
	m_ResetOptButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnResetMatrixClick ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnButtonCloseClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnErcCmpClick ), NULL, this );
	
}
