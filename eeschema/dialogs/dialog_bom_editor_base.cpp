///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr  1 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_bom_editor_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_BOM_EDITOR_BASE, DIALOG_SHIM )
	EVT_CLOSE( DIALOG_BOM_EDITOR_BASE::_wxFB_OnBomEditorClosed )
	EVT_CHECKBOX( OPT_GROUP_COMPONENTS, DIALOG_BOM_EDITOR_BASE::_wxFB_OnGroupComponentsToggled )
	EVT_BUTTON( ID_BUTTON_REGROUP, DIALOG_BOM_EDITOR_BASE::_wxFB_OnRegroupComponents )
	EVT_BUTTON( ID_BUTTON_REVERT_CHANGES, DIALOG_BOM_EDITOR_BASE::_wxFB_OnRevertFieldChanges )
	EVT_DATAVIEW_ITEM_VALUE_CHANGED( wxID_ANY, DIALOG_BOM_EDITOR_BASE::_wxFB_OnColumnItemToggled )
	EVT_BUTTON( wxID_BOM_BUTTON_EXPORT, DIALOG_BOM_EDITOR_BASE::_wxFB_OnExportBOM )
	EVT_DATAVIEW_COLUMN_REORDERED( wxID_ANY, DIALOG_BOM_EDITOR_BASE::_wxFB_OnBomColumReordered )
	EVT_DATAVIEW_COLUMN_SORTED( wxID_ANY, DIALOG_BOM_EDITOR_BASE::_wxFB_OnBomColumnSorted )
	EVT_DATAVIEW_ITEM_EDITING_DONE( wxID_ANY, DIALOG_BOM_EDITOR_BASE::_wxFB_OnTableValueChanged )
	EVT_DATAVIEW_SELECTION_CHANGED( wxID_ANY, DIALOG_BOM_EDITOR_BASE::_wxFB_OnSelectionChanged )
END_EVENT_TABLE()

DIALOG_BOM_EDITOR_BASE::DIALOG_BOM_EDITOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bHorizontalSizer;
	bHorizontalSizer = new wxBoxSizer( wxVERTICAL );
	
	m_panel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	m_splitter1 = new wxSplitterWindow( m_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	m_splitter1->Connect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_BOM_EDITOR_BASE::m_splitter1OnIdle ), NULL, this );
	
	m_leftPanel = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( m_leftPanel, wxID_ANY, _("Options") ), wxVERTICAL );
	
	m_groupComponentsBox = new wxCheckBox( sbSizer1->GetStaticBox(), OPT_GROUP_COMPONENTS, _("Group components"), wxDefaultPosition, wxDefaultSize, 0 );
	m_groupComponentsBox->SetValue(true); 
	m_groupComponentsBox->SetToolTip( _("Group components together based on common properties") );
	
	sbSizer1->Add( m_groupComponentsBox, 0, wxALL|wxEXPAND, 5 );
	
	m_regroupComponentsButton = new wxButton( sbSizer1->GetStaticBox(), ID_BUTTON_REGROUP, _("Regroup components"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_regroupComponentsButton, 0, wxALL|wxEXPAND, 5 );
	
	m_reloadTableButton = new wxButton( sbSizer1->GetStaticBox(), ID_BUTTON_REVERT_CHANGES, _("Revert all changes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_reloadTableButton->Enable( false );
	m_reloadTableButton->SetToolTip( _("Reload table (reverts component field changes)") );
	
	sbSizer1->Add( m_reloadTableButton, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer6->Add( sbSizer1, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* m_fieldListSizer;
	m_fieldListSizer = new wxStaticBoxSizer( new wxStaticBox( m_leftPanel, wxID_ANY, _("Fields") ), wxVERTICAL );
	
	m_columnListCtrl = new wxDataViewListCtrl( m_fieldListSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_fieldListSizer->Add( m_columnListCtrl, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizer9->Add( m_fieldListSizer, 1, wxEXPAND, 5 );
	
	
	bSizer6->Add( bSizer9, 5, wxEXPAND, 5 );
	
	
	bSizer6->Add( 0, 0, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( m_leftPanel, wxID_ANY, _("Export options") ), wxVERTICAL );
	
	m_includeProjectData = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_BOM_OPT_INC_PRJ_DATA, _("Include project data"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeProjectData->SetValue(true); 
	m_includeProjectData->SetToolTip( _("Include project information in BOM file") );
	
	sbSizer2->Add( m_includeProjectData, 0, wxALL|wxEXPAND, 5 );
	
	m_showRowNumbers = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_BOM_OPT_SHOW_ROW_NUMS, _("Show row numbers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_showRowNumbers->SetValue(true); 
	m_showRowNumbers->SetToolTip( _("Show BOM row numbers ") );
	
	sbSizer2->Add( m_showRowNumbers, 0, wxALL|wxEXPAND, 5 );
	
	
	sbSizer2->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer13->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_exportButton = new wxButton( sbSizer2->GetStaticBox(), wxID_BOM_BUTTON_EXPORT, _("Export BOM"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer13->Add( m_exportButton, 0, wxALL, 5 );
	
	
	sbSizer2->Add( bSizer13, 0, wxEXPAND, 5 );
	
	
	bSizer6->Add( sbSizer2, 0, wxEXPAND, 5 );
	
	
	m_leftPanel->SetSizer( bSizer6 );
	m_leftPanel->Layout();
	bSizer6->Fit( m_leftPanel );
	m_panel4 = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );
	
	m_bomView = new wxDataViewCtrl( m_panel4, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE|wxDV_ROW_LINES|wxDV_VERT_RULES );
	m_bomView->SetMinSize( wxSize( 250,250 ) );
	
	bSizer5->Add( m_bomView, 1, wxALL|wxEXPAND, 5 );
	
	
	m_panel4->SetSizer( bSizer5 );
	m_panel4->Layout();
	bSizer5->Fit( m_panel4 );
	m_splitter1->SplitVertically( m_leftPanel, m_panel4, 231 );
	bSizer7->Add( m_splitter1, 1, wxEXPAND, 5 );
	
	
	m_panel->SetSizer( bSizer7 );
	m_panel->Layout();
	bSizer7->Fit( m_panel );
	bHorizontalSizer->Add( m_panel, 1, wxEXPAND | wxALL, 5 );
	
	
	this->SetSizer( bHorizontalSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
}

DIALOG_BOM_EDITOR_BASE::~DIALOG_BOM_EDITOR_BASE()
{
}
