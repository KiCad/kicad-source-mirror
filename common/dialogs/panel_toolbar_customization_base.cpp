///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

#include "panel_toolbar_customization_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_TOOLBAR_CUSTOMIZATION_BASE::PANEL_TOOLBAR_CUSTOMIZATION_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_customToolbars = new wxCheckBox( this, wxID_ANY, _("Customize toolbars"), wxDefaultPosition, wxDefaultSize, 0 );
	bPanelSizer->Add( m_customToolbars, 0, wxALL, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bPanelSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	wxFlexGridSizer* m_customizeControls;
	m_customizeControls = new wxFlexGridSizer( 1, 3, 0, 0 );
	m_customizeControls->AddGrowableCol( 0 );
	m_customizeControls->AddGrowableCol( 2 );
	m_customizeControls->AddGrowableRow( 0 );
	m_customizeControls->SetFlexibleDirection( wxHORIZONTAL );
	m_customizeControls->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	wxArrayString m_tbChoiceChoices;
	m_tbChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_tbChoiceChoices, 0 );
	m_tbChoice->SetSelection( 0 );
	bSizer8->Add( m_tbChoice, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );

	m_toolbarTree = new wxTreeCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE|wxTR_EDIT_LABELS|wxTR_HIDE_ROOT|wxTR_NO_LINES );
	bSizer11->Add( m_toolbarTree, 1, wxALL|wxEXPAND, 5 );


	bSizer10->Add( bSizer11, 1, wxEXPAND, 5 );


	bSizer8->Add( bSizer10, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerToolbarBtns;
	bSizerToolbarBtns = new wxBoxSizer( wxHORIZONTAL );

	m_btnToolDelete = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerToolbarBtns->Add( m_btnToolDelete, 0, wxALL, 5 );

	m_btnToolMoveUp = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerToolbarBtns->Add( m_btnToolMoveUp, 0, wxBOTTOM|wxLEFT|wxTOP, 5 );

	m_btnToolMoveDown = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerToolbarBtns->Add( m_btnToolMoveDown, 0, wxALL, 5 );


	bSizerToolbarBtns->Add( 20, 0, 0, wxEXPAND, 5 );

	m_insertButton = new SPLIT_BUTTON( this, wxID_ANY, _( "Insert separator" ), wxDefaultPosition );
	bSizerToolbarBtns->Add( m_insertButton, 0, wxALL, 5 );


	bSizer8->Add( bSizerToolbarBtns, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


	m_customizeControls->Add( bSizer8, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );


	bSizer6->Add( 0, 0, 1, wxEXPAND, 5 );

	m_btnAddTool = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer6->Add( m_btnAddTool, 0, wxALL, 5 );


	bSizer6->Add( 0, 0, 1, wxEXPAND, 5 );


	m_customizeControls->Add( bSizer6, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	m_actionsList = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_NO_HEADER|wxLC_REPORT|wxLC_SINGLE_SEL );
	bSizer9->Add( m_actionsList, 0, wxALL|wxEXPAND, 5 );


	m_customizeControls->Add( bSizer9, 1, wxEXPAND, 5 );


	bPanelSizer->Add( m_customizeControls, 1, wxEXPAND, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::OnUpdateUI ) );
	m_customToolbars->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::onCustomizeTbCb ), NULL, this );
	m_tbChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::onTbChoiceSelect ), NULL, this );
	m_toolbarTree->Connect( wxEVT_COMMAND_TREE_BEGIN_LABEL_EDIT, wxTreeEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::onTreeBeginLabelEdit ), NULL, this );
	m_toolbarTree->Connect( wxEVT_COMMAND_TREE_END_LABEL_EDIT, wxTreeEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::onTreeEndLabelEdit ), NULL, this );
	m_btnToolDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::onToolDelete ), NULL, this );
	m_btnToolMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::onToolMoveUp ), NULL, this );
	m_btnToolMoveDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::onToolMoveDown ), NULL, this );
	m_btnAddTool->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::onBtnAddAction ), NULL, this );
	m_actionsList->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::onListItemActivated ), NULL, this );
}

PANEL_TOOLBAR_CUSTOMIZATION_BASE::~PANEL_TOOLBAR_CUSTOMIZATION_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::OnUpdateUI ) );
	m_customToolbars->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::onCustomizeTbCb ), NULL, this );
	m_tbChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::onTbChoiceSelect ), NULL, this );
	m_toolbarTree->Disconnect( wxEVT_COMMAND_TREE_BEGIN_LABEL_EDIT, wxTreeEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::onTreeBeginLabelEdit ), NULL, this );
	m_toolbarTree->Disconnect( wxEVT_COMMAND_TREE_END_LABEL_EDIT, wxTreeEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::onTreeEndLabelEdit ), NULL, this );
	m_btnToolDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::onToolDelete ), NULL, this );
	m_btnToolMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::onToolMoveUp ), NULL, this );
	m_btnToolMoveDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::onToolMoveDown ), NULL, this );
	m_btnAddTool->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::onBtnAddAction ), NULL, this );
	m_actionsList->Disconnect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( PANEL_TOOLBAR_CUSTOMIZATION_BASE::onListItemActivated ), NULL, this );

}
