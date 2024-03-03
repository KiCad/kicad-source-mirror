///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_zone_manager_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_ZONE_MANAGER_BASE::DIALOG_ZONE_MANAGER_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	m_MainBoxSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );

	m_sizerTop = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );

	m_viewZonesOverview = new wxDataViewCtrl( this, VIEW_ZONE_TABLE, wxDefaultPosition, wxSize( -1,240 ), wxDV_HORIZ_RULES|wxDV_ROW_LINES|wxDV_SINGLE|wxDV_VERT_RULES );
	m_viewZonesOverview->SetMinSize( wxSize( -1,240 ) );

	bSizer7->Add( m_viewZonesOverview, 1, wxALL|wxEXPAND, 0 );

	m_sizerZoneOP = new wxBoxSizer( wxHORIZONTAL );

	m_btnMoveUp = new STD_BITMAP_BUTTON( this, BTN_MOVE_UP, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_btnMoveUp->SetToolTip( _("Top zone has the highest priority. When a zone is inside another zone, if its priority is higher, its outlines are removed from the other zone.") );

	m_sizerZoneOP->Add( m_btnMoveUp, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_btnMoveDown = new STD_BITMAP_BUTTON( this, BTN_MOVE_DOWN, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_btnMoveDown->SetToolTip( _("Top zone has the highest priority. When a zone is inside another zone, if its priority is higher, its outlines are removed from the other zone.") );

	m_sizerZoneOP->Add( m_btnMoveDown, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	m_sizerZoneOP->Add( 0, 0, 0, wxEXPAND, 5 );

	m_checkName = new wxCheckBox( this, CHECK_NAME, _("Name"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkName->SetValue(true);
	m_sizerZoneOP->Add( m_checkName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_checkNet = new wxCheckBox( this, CHECK_NET, _("Net"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkNet->SetValue(true);
	m_sizerZoneOP->Add( m_checkNet, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_filterCtrl = new wxSearchCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	#ifndef __WXMAC__
	m_filterCtrl->ShowSearchButton( true );
	#endif
	m_filterCtrl->ShowCancelButton( true );
	m_sizerZoneOP->Add( m_filterCtrl, 1, wxEXPAND, 5 );


	bSizer7->Add( m_sizerZoneOP, 0, wxEXPAND|wxLEFT|wxTOP, 5 );


	m_sizerTop->Add( bSizer7, 1, wxALL|wxEXPAND, 5 );


	bSizer12->Add( m_sizerTop, 1, wxEXPAND, 5 );

	m_sizerProperties = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Properties") ), wxVERTICAL );


	bSizer12->Add( m_sizerProperties, 0, wxALL|wxEXPAND, 5 );


	m_MainBoxSizer->Add( bSizer12, 1, wxEXPAND, 0 );

	wxBoxSizer* bSizer61;
	bSizer61 = new wxBoxSizer( wxHORIZONTAL );

	m_checkRepour = new wxCheckBox( this, wxID_ANY, _("Repour"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer61->Add( m_checkRepour, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );


	bSizer61->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerApply = new wxButton( this, wxID_APPLY );
	m_sdbSizer->AddButton( m_sdbSizerApply );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizer61->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	m_MainBoxSizer->Add( bSizer61, 0, wxEXPAND, 5 );


	this->SetSizer( m_MainBoxSizer );
	this->Layout();
	m_MainBoxSizer->Fit( this );

	// Connect Events
	m_viewZonesOverview->Connect( wxEVT_CHAR, wxKeyEventHandler( DIALOG_ZONE_MANAGER_BASE::OnTableChar ), NULL, this );
	m_viewZonesOverview->Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_ZONE_MANAGER_BASE::OnTableCharHook ), NULL, this );
	m_viewZonesOverview->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_ZONE_MANAGER_BASE::OnDataViewCtrlSelectionChanged ), NULL, this );
	m_viewZonesOverview->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( DIALOG_ZONE_MANAGER_BASE::OnViewZonesOverviewOnLeftUp ), NULL, this );
	m_filterCtrl->Connect( wxEVT_COMMAND_SEARCHCTRL_CANCEL_BTN, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnFilterCtrlCancel ), NULL, this );
	m_filterCtrl->Connect( wxEVT_COMMAND_SEARCHCTRL_SEARCH_BTN, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnFilterCtrlSearch ), NULL, this );
	m_filterCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnFilterCtrlTextChange ), NULL, this );
	m_filterCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnFilterCtrlEnter ), NULL, this );
	m_checkRepour->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnRepourCheck ), NULL, this );
	m_sdbSizerApply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnButtonApplyClick ), NULL, this );
}

DIALOG_ZONE_MANAGER_BASE::~DIALOG_ZONE_MANAGER_BASE()
{
	// Disconnect Events
	m_viewZonesOverview->Disconnect( wxEVT_CHAR, wxKeyEventHandler( DIALOG_ZONE_MANAGER_BASE::OnTableChar ), NULL, this );
	m_viewZonesOverview->Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_ZONE_MANAGER_BASE::OnTableCharHook ), NULL, this );
	m_viewZonesOverview->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_ZONE_MANAGER_BASE::OnDataViewCtrlSelectionChanged ), NULL, this );
	m_viewZonesOverview->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( DIALOG_ZONE_MANAGER_BASE::OnViewZonesOverviewOnLeftUp ), NULL, this );
	m_filterCtrl->Disconnect( wxEVT_COMMAND_SEARCHCTRL_CANCEL_BTN, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnFilterCtrlCancel ), NULL, this );
	m_filterCtrl->Disconnect( wxEVT_COMMAND_SEARCHCTRL_SEARCH_BTN, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnFilterCtrlSearch ), NULL, this );
	m_filterCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnFilterCtrlTextChange ), NULL, this );
	m_filterCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnFilterCtrlEnter ), NULL, this );
	m_checkRepour->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnRepourCheck ), NULL, this );
	m_sdbSizerApply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnButtonApplyClick ), NULL, this );

}
