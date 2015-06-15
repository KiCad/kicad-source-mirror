///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_drclistbox.h"

#include "dialog_drc_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DRC_CONTROL_BASE::DIALOG_DRC_CONTROL_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* m_MainSizer;
	m_MainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* m_CommandSizer;
	m_CommandSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbSizerOptions;
	sbSizerOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options:") ), wxHORIZONTAL );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgMinValuesSizer;
	fgMinValuesSizer = new wxFlexGridSizer( 4, 2, 0, 0 );
	fgMinValuesSizer->AddGrowableCol( 1 );
	fgMinValuesSizer->SetFlexibleDirection( wxHORIZONTAL );
	fgMinValuesSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_ClearanceTitle = new wxStaticText( this, wxID_ANY, _("Clearance"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ClearanceTitle->Wrap( -1 );
	fgMinValuesSizer->Add( m_ClearanceTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_SetClearance = new wxTextCtrl( this, wxID_ANY, _("By Netclass"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SetClearance->SetMaxLength( 0 ); 
	m_SetClearance->Enable( false );
	
	fgMinValuesSizer->Add( m_SetClearance, 0, wxALL|wxEXPAND, 5 );
	
	m_TrackMinWidthTitle = new wxStaticText( this, wxID_ANY, _("Min track width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackMinWidthTitle->Wrap( -1 );
	m_TrackMinWidthTitle->SetToolTip( _("Enter the minimum acceptable value for a track width") );
	
	fgMinValuesSizer->Add( m_TrackMinWidthTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_SetTrackMinWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SetTrackMinWidthCtrl->SetMaxLength( 0 ); 
	fgMinValuesSizer->Add( m_SetTrackMinWidthCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_ViaMinTitle = new wxStaticText( this, wxID_ANY, _("Min via size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaMinTitle->Wrap( -1 );
	m_ViaMinTitle->SetHelpText( _("Enter the minimum acceptable diameter for a standard via") );
	
	fgMinValuesSizer->Add( m_ViaMinTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_SetViaMinSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SetViaMinSizeCtrl->SetMaxLength( 0 ); 
	fgMinValuesSizer->Add( m_SetViaMinSizeCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_MicroViaMinTitle = new wxStaticText( this, wxID_ANY, _("Min uVia size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MicroViaMinTitle->Wrap( -1 );
	m_MicroViaMinTitle->SetToolTip( _("Enter the minimum acceptable diameter for a micro via") );
	
	fgMinValuesSizer->Add( m_MicroViaMinTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_SetMicroViakMinSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SetMicroViakMinSizeCtrl->SetMaxLength( 0 ); 
	fgMinValuesSizer->Add( m_SetMicroViakMinSizeCtrl, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer7->Add( fgMinValuesSizer, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* ReportFileSizer;
	ReportFileSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Create Report File") ), wxHORIZONTAL );
	
	m_CreateRptCtrl = new wxCheckBox( this, ID_CHECKBOX_RPT_FILE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_CreateRptCtrl->SetToolTip( _("Enable writing report to this file") );
	
	ReportFileSizer->Add( m_CreateRptCtrl, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_RptFilenameCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_RptFilenameCtrl->SetMaxLength( 0 ); 
	m_RptFilenameCtrl->SetToolTip( _("Enter the report filename") );
	m_RptFilenameCtrl->SetMinSize( wxSize( 180,-1 ) );
	
	ReportFileSizer->Add( m_RptFilenameCtrl, 1, wxALL|wxEXPAND, 5 );
	
	m_BrowseButton = new wxButton( this, ID_BUTTON_BROWSE_RPT_FILE, _("..."), wxDefaultPosition, wxSize( 50,-1 ), 0 );
	ReportFileSizer->Add( m_BrowseButton, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer7->Add( ReportFileSizer, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	
	sbSizerOptions->Add( bSizer7, 1, wxEXPAND, 5 );
	
	
	m_CommandSizer->Add( sbSizerOptions, 1, 0, 5 );
	
	wxBoxSizer* bSizerMessages;
	bSizerMessages = new wxBoxSizer( wxVERTICAL );
	
	m_staticText6 = new wxStaticText( this, wxID_ANY, _("Messages:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	bSizerMessages->Add( m_staticText6, 0, wxRIGHT|wxLEFT, 5 );
	
	m_Messages = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxTE_MULTILINE|wxTE_READONLY );
	m_Messages->SetMinSize( wxSize( 220,-1 ) );
	
	bSizerMessages->Add( m_Messages, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	
	m_CommandSizer->Add( bSizerMessages, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );
	
	m_buttonRunDRC = new wxButton( this, ID_STARTDRC, _("Start DRC"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonRunDRC->SetDefault(); 
	m_buttonRunDRC->SetToolTip( _("Start the Design Rule Checker") );
	
	bSizer11->Add( m_buttonRunDRC, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonListUnconnected = new wxButton( this, ID_LIST_UNCONNECTED, _("List Unconnected"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonListUnconnected->SetToolTip( _("List unconnected pads or tracks") );
	
	bSizer11->Add( m_buttonListUnconnected, 0, wxALL|wxEXPAND, 5 );
	
	m_DeleteAllButton = new wxButton( this, ID_DELETE_ALL, _("Delete All Markers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DeleteAllButton->SetToolTip( _("Delete every marker") );
	
	bSizer11->Add( m_DeleteAllButton, 0, wxALL|wxEXPAND, 5 );
	
	m_DeleteCurrentMarkerButton = new wxButton( this, wxID_ANY, _("Delete Current Marker"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DeleteCurrentMarkerButton->SetToolTip( _("Delete the marker selected in the list box below") );
	
	bSizer11->Add( m_DeleteCurrentMarkerButton, 0, wxEXPAND|wxALL, 5 );
	
	
	m_CommandSizer->Add( bSizer11, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	m_MainSizer->Add( m_CommandSizer, 0, wxALL|wxEXPAND, 5 );
	
	m_staticTextErrMsg = new wxStaticText( this, wxID_ANY, _("Error Messages:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextErrMsg->Wrap( -1 );
	m_MainSizer->Add( m_staticTextErrMsg, 0, wxALL, 5 );
	
	m_Notebook = new wxNotebook( this, ID_NOTEBOOK1, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelClearanceListBox = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizeClearanceBox;
	bSizeClearanceBox = new wxBoxSizer( wxVERTICAL );
	
	m_ClearanceListBox = new DRCLISTBOX( m_panelClearanceListBox, ID_CLEARANCE_LIST, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	m_ClearanceListBox->SetToolTip( _("MARKERs, double click any to go there in PCB, right click for popup menu") );
	m_ClearanceListBox->SetMinSize( wxSize( -1,80 ) );
	
	bSizeClearanceBox->Add( m_ClearanceListBox, 1, wxALL|wxEXPAND, 5 );
	
	
	m_panelClearanceListBox->SetSizer( bSizeClearanceBox );
	m_panelClearanceListBox->Layout();
	bSizeClearanceBox->Fit( m_panelClearanceListBox );
	m_Notebook->AddPage( m_panelClearanceListBox, _("Problems / Markers"), true );
	m_panelUnconnectedBox = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerUnconnectedBox;
	bSizerUnconnectedBox = new wxBoxSizer( wxVERTICAL );
	
	m_UnconnectedListBox = new DRCLISTBOX( m_panelUnconnectedBox, ID_UNCONNECTED_LIST, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	m_UnconnectedListBox->SetToolTip( _("A list of unconnected pads, right click for popup menu") );
	
	bSizerUnconnectedBox->Add( m_UnconnectedListBox, 1, wxALL|wxEXPAND, 5 );
	
	
	m_panelUnconnectedBox->SetSizer( bSizerUnconnectedBox );
	m_panelUnconnectedBox->Layout();
	bSizerUnconnectedBox->Fit( m_panelUnconnectedBox );
	m_Notebook->AddPage( m_panelUnconnectedBox, _("Unconnected"), false );
	
	m_MainSizer->Add( m_Notebook, 1, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	m_MainSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );
	
	// Connect Events
	m_CreateRptCtrl->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnReportCheckBoxClicked ), NULL, this );
	m_BrowseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnButtonBrowseRptFileClick ), NULL, this );
	m_buttonRunDRC->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnStartdrcClick ), NULL, this );
	m_buttonListUnconnected->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnListUnconnectedClick ), NULL, this );
	m_DeleteAllButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnDeleteAllClick ), NULL, this );
	m_DeleteCurrentMarkerButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnDeleteOneClick ), NULL, this );
	m_Notebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_DRC_CONTROL_BASE::OnChangingMarkerList ), NULL, this );
	m_ClearanceListBox->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnLeftDClickClearance ), NULL, this );
	m_ClearanceListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnMarkerSelectionEvent ), NULL, this );
	m_ClearanceListBox->Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnRightUpClearance ), NULL, this );
	m_UnconnectedListBox->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnLeftDClickUnconnected ), NULL, this );
	m_UnconnectedListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnUnconnectedSelectionEvent ), NULL, this );
	m_UnconnectedListBox->Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnRightUpUnconnected ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnOkClick ), NULL, this );
}

DIALOG_DRC_CONTROL_BASE::~DIALOG_DRC_CONTROL_BASE()
{
	// Disconnect Events
	m_CreateRptCtrl->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnReportCheckBoxClicked ), NULL, this );
	m_BrowseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnButtonBrowseRptFileClick ), NULL, this );
	m_buttonRunDRC->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnStartdrcClick ), NULL, this );
	m_buttonListUnconnected->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnListUnconnectedClick ), NULL, this );
	m_DeleteAllButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnDeleteAllClick ), NULL, this );
	m_DeleteCurrentMarkerButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnDeleteOneClick ), NULL, this );
	m_Notebook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_DRC_CONTROL_BASE::OnChangingMarkerList ), NULL, this );
	m_ClearanceListBox->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnLeftDClickClearance ), NULL, this );
	m_ClearanceListBox->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnMarkerSelectionEvent ), NULL, this );
	m_ClearanceListBox->Disconnect( wxEVT_RIGHT_UP, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnRightUpClearance ), NULL, this );
	m_UnconnectedListBox->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnLeftDClickUnconnected ), NULL, this );
	m_UnconnectedListBox->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnUnconnectedSelectionEvent ), NULL, this );
	m_UnconnectedListBox->Disconnect( wxEVT_RIGHT_UP, wxMouseEventHandler( DIALOG_DRC_CONTROL_BASE::OnRightUpUnconnected ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRC_CONTROL_BASE::OnOkClick ), NULL, this );
	
}
