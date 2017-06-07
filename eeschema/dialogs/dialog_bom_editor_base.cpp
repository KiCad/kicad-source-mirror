///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Feb 19 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_bom_editor_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_BOM_EDITOR_BASE::DIALOG_BOM_EDITOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bHorizontalSizer;
	bHorizontalSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer61;
	bSizer61 = new wxBoxSizer( wxVERTICAL );
	
	m_panel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	m_splitter1 = new wxSplitterWindow( m_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	m_splitter1->Connect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_BOM_EDITOR_BASE::m_splitter1OnIdle ), NULL, this );
	m_splitter1->SetMinimumPaneSize( 120 );
	
	m_leftPanel = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( m_leftPanel, wxID_ANY, _("Options") ), wxVERTICAL );
	
	m_groupComponentsBox = new wxCheckBox( sbSizer1->GetStaticBox(), OPT_GROUP_COMPONENTS, _("Group components"), wxDefaultPosition, wxDefaultSize, 0 );
	m_groupComponentsBox->SetValue(true); 
	m_groupComponentsBox->SetToolTip( _("Group components together based on common properties") );
	
	sbSizer1->Add( m_groupComponentsBox, 0, wxALL|wxEXPAND, 5 );
	
	m_regroupComponentsButton = new wxButton( sbSizer1->GetStaticBox(), wxID_ANY, _("Regroup components"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_regroupComponentsButton, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer6->Add( sbSizer1, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* m_fieldListSizer;
	m_fieldListSizer = new wxStaticBoxSizer( new wxStaticBox( m_leftPanel, wxID_ANY, _("Fields") ), wxVERTICAL );
	
	m_columnListCtrl = new wxDataViewListCtrl( m_fieldListSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_columnListCtrl->SetMinSize( wxSize( -1,250 ) );
	
	m_fieldListSizer->Add( m_columnListCtrl, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizer9->Add( m_fieldListSizer, 1, wxEXPAND, 5 );
	
	
	bSizer6->Add( bSizer9, 5, wxEXPAND, 5 );
	
	
	m_leftPanel->SetSizer( bSizer6 );
	m_leftPanel->Layout();
	bSizer6->Fit( m_leftPanel );
	m_panel4 = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );
	
	m_bomView = new wxDataViewCtrl( m_panel4, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE|wxDV_ROW_LINES|wxDV_VERT_RULES );
	m_bomView->SetMinSize( wxSize( 450,250 ) );
	
	bSizer5->Add( m_bomView, 1, wxALL|wxEXPAND, 5 );
	
	
	m_panel4->SetSizer( bSizer5 );
	m_panel4->Layout();
	bSizer5->Fit( m_panel4 );
	m_splitter1->SplitVertically( m_leftPanel, m_panel4, 231 );
	bSizer7->Add( m_splitter1, 1, wxEXPAND, 5 );
	
	
	m_panel->SetSizer( bSizer7 );
	m_panel->Layout();
	bSizer7->Fit( m_panel );
	bSizer61->Add( m_panel, 1, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bSizer71;
	bSizer71 = new wxBoxSizer( wxHORIZONTAL );
	
	m_applyChangesButton = new wxButton( this, wxID_ANY, _("Apply Changes"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer71->Add( m_applyChangesButton, 0, wxALL, 5 );
	
	m_revertChangesButton = new wxButton( this, wxID_ANY, _("Revert Changes"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer71->Add( m_revertChangesButton, 0, wxALL, 5 );
	
	
	bSizer71->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_closeButton = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer71->Add( m_closeButton, 0, wxALL, 5 );
	
	
	bSizer61->Add( bSizer71, 0, wxEXPAND, 5 );
	
	
	bHorizontalSizer->Add( bSizer61, 1, wxEXPAND, 5 );
	
	
	this->SetSizer( bHorizontalSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_BOM_EDITOR_BASE::OnDialogClosed ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_BOM_EDITOR_BASE::OnUpdateUI ) );
	m_groupComponentsBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOM_EDITOR_BASE::OnGroupComponentsToggled ), NULL, this );
	m_regroupComponentsButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_EDITOR_BASE::OnRegroupComponents ), NULL, this );
	m_columnListCtrl->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_BOM_EDITOR_BASE::OnColumnItemToggled ), NULL, this );
	m_bomView->Connect( wxEVT_COMMAND_DATAVIEW_COLUMN_REORDERED, wxDataViewEventHandler( DIALOG_BOM_EDITOR_BASE::OnBomColumReordered ), NULL, this );
	m_bomView->Connect( wxEVT_COMMAND_DATAVIEW_COLUMN_SORTED, wxDataViewEventHandler( DIALOG_BOM_EDITOR_BASE::OnBomColumnSorted ), NULL, this );
	m_bomView->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler( DIALOG_BOM_EDITOR_BASE::OnTableItemActivated ), NULL, this );
	m_bomView->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler( DIALOG_BOM_EDITOR_BASE::OnTableItemContextMenu ), NULL, this );
	m_bomView->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_EDITING_DONE, wxDataViewEventHandler( DIALOG_BOM_EDITOR_BASE::OnTableValueChanged ), NULL, this );
	m_bomView->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_BOM_EDITOR_BASE::OnSelectionChanged ), NULL, this );
	m_applyChangesButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_EDITOR_BASE::OnApplyFieldChanges ), NULL, this );
	m_revertChangesButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_EDITOR_BASE::OnRevertFieldChanges ), NULL, this );
	m_closeButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_EDITOR_BASE::OnCloseButton ), NULL, this );
}

DIALOG_BOM_EDITOR_BASE::~DIALOG_BOM_EDITOR_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_BOM_EDITOR_BASE::OnDialogClosed ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_BOM_EDITOR_BASE::OnUpdateUI ) );
	m_groupComponentsBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOM_EDITOR_BASE::OnGroupComponentsToggled ), NULL, this );
	m_regroupComponentsButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_EDITOR_BASE::OnRegroupComponents ), NULL, this );
	m_columnListCtrl->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_BOM_EDITOR_BASE::OnColumnItemToggled ), NULL, this );
	m_bomView->Disconnect( wxEVT_COMMAND_DATAVIEW_COLUMN_REORDERED, wxDataViewEventHandler( DIALOG_BOM_EDITOR_BASE::OnBomColumReordered ), NULL, this );
	m_bomView->Disconnect( wxEVT_COMMAND_DATAVIEW_COLUMN_SORTED, wxDataViewEventHandler( DIALOG_BOM_EDITOR_BASE::OnBomColumnSorted ), NULL, this );
	m_bomView->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler( DIALOG_BOM_EDITOR_BASE::OnTableItemActivated ), NULL, this );
	m_bomView->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler( DIALOG_BOM_EDITOR_BASE::OnTableItemContextMenu ), NULL, this );
	m_bomView->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_EDITING_DONE, wxDataViewEventHandler( DIALOG_BOM_EDITOR_BASE::OnTableValueChanged ), NULL, this );
	m_bomView->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_BOM_EDITOR_BASE::OnSelectionChanged ), NULL, this );
	m_applyChangesButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_EDITOR_BASE::OnApplyFieldChanges ), NULL, this );
	m_revertChangesButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_EDITOR_BASE::OnRevertFieldChanges ), NULL, this );
	m_closeButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOM_EDITOR_BASE::OnCloseButton ), NULL, this );
	
}
