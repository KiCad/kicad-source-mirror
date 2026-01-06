///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

#include "dialog_zone_manager_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_ZONE_MANAGER_BASE::DIALOG_ZONE_MANAGER_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	m_MainBoxSizer = new wxBoxSizer( wxVERTICAL );

	m_sizerTop = new wxBoxSizer( wxHORIZONTAL );

	m_splitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE );
	m_splitter->SetSashGravity( 0.3 );
	m_splitter->Connect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_ZONE_MANAGER_BASE::m_splitterOnIdle ), NULL, this );

	m_listPanel = new wxPanel( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* listPanelSizer;
	listPanelSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* m_leftColumn;
	m_leftColumn = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* searchSizer;
	searchSizer = new wxBoxSizer( wxHORIZONTAL );

	m_filterCtrl = new wxSearchCtrl( m_listPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	#ifndef __WXMAC__
	m_filterCtrl->ShowSearchButton( true );
	#endif
	m_filterCtrl->ShowCancelButton( true );
	searchSizer->Add( m_filterCtrl, 1, wxALIGN_CENTER_VERTICAL, 1 );


	searchSizer->Add( 10, 0, 0, wxEXPAND, 5 );

	m_checkName = new wxCheckBox( m_listPanel, wxID_ANY, _("Name"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkName->SetValue(true);
	searchSizer->Add( m_checkName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_checkNet = new wxCheckBox( m_listPanel, wxID_ANY, _("Net"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkNet->SetValue(true);
	searchSizer->Add( m_checkNet, 0, wxALIGN_CENTER_VERTICAL, 5 );


	m_leftColumn->Add( searchSizer, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );

	m_viewZonesOverview = new wxDataViewCtrl( m_listPanel, wxID_ANY, wxDefaultPosition, wxSize( -1,240 ), wxDV_HORIZ_RULES|wxDV_SINGLE|wxDV_VERT_RULES );
	m_viewZonesOverview->SetMinSize( wxSize( -1,240 ) );

	m_leftColumn->Add( m_viewZonesOverview, 1, wxEXPAND, 5 );

	m_sizerZoneOP = new wxBoxSizer( wxHORIZONTAL );

	m_btnMoveUp = new STD_BITMAP_BUTTON( m_listPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_btnMoveUp->SetToolTip( _("Top zone has the highest priority. When a zone is inside another zone, if its priority is higher, its outlines are removed from the other zone.") );

	m_sizerZoneOP->Add( m_btnMoveUp, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_btnMoveDown = new STD_BITMAP_BUTTON( m_listPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_btnMoveDown->SetToolTip( _("Top zone has the highest priority. When a zone is inside another zone, if its priority is higher, its outlines are removed from the other zone.") );

	m_sizerZoneOP->Add( m_btnMoveDown, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	m_leftColumn->Add( m_sizerZoneOP, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	listPanelSizer->Add( m_leftColumn, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	m_listPanel->SetSizer( listPanelSizer );
	m_listPanel->Layout();
	listPanelSizer->Fit( m_listPanel );
	m_zonePanel = new wxPanel( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* zonePanelSizer;
	zonePanelSizer = new wxBoxSizer( wxVERTICAL );

	m_rightColumn = new wxBoxSizer( wxVERTICAL );

	m_sizerProperties = new wxBoxSizer( wxVERTICAL );


	m_rightColumn->Add( m_sizerProperties, 0, wxEXPAND, 5 );

	m_sizerPreview = new wxBoxSizer( wxVERTICAL );


	m_rightColumn->Add( m_sizerPreview, 1, wxEXPAND|wxBOTTOM, 5 );


	zonePanelSizer->Add( m_rightColumn, 1, wxEXPAND|wxBOTTOM, 5 );


	m_zonePanel->SetSizer( zonePanelSizer );
	m_zonePanel->Layout();
	zonePanelSizer->Fit( m_zonePanel );
	m_splitter->SplitVertically( m_listPanel, m_zonePanel, 520 );
	m_sizerTop->Add( m_splitter, 1, wxEXPAND, 5 );


	m_MainBoxSizer->Add( m_sizerTop, 1, wxEXPAND, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_MainBoxSizer->Add( m_staticline1, 0, wxEXPAND, 5 );

	m_sizerBottom = new wxBoxSizer( wxHORIZONTAL );

	m_checkRepour = new wxCheckBox( this, wxID_ANY, _("Refill zones"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkRepour->SetToolTip( _("Refill zones after changes made on board") );

	m_sizerBottom->Add( m_checkRepour, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	m_sizerBottom->Add( 100, 0, 1, wxEXPAND, 5 );

	m_updateDisplayedZones = new wxButton( this, wxID_ANY, _("Update Displayed Zones"), wxDefaultPosition, wxDefaultSize, 0 );
	m_updateDisplayedZones->SetToolTip( _("Update filled areas shown in dialog, according to the new current settings") );

	m_sizerBottom->Add( m_updateDisplayedZones, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_sizerBottom->Add( m_sdbSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );


	m_MainBoxSizer->Add( m_sizerBottom, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	this->SetSizer( m_MainBoxSizer );
	this->Layout();
	m_MainBoxSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_ZONE_MANAGER_BASE::onDialogResize ) );
	m_filterCtrl->Connect( wxEVT_COMMAND_SEARCHCTRL_CANCEL_BTN, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnFilterCtrlCancel ), NULL, this );
	m_filterCtrl->Connect( wxEVT_COMMAND_SEARCHCTRL_SEARCH_BTN, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnFilterCtrlSearch ), NULL, this );
	m_filterCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnFilterCtrlTextChange ), NULL, this );
	m_filterCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnFilterCtrlEnter ), NULL, this );
	m_viewZonesOverview->Connect( wxEVT_CHAR, wxKeyEventHandler( DIALOG_ZONE_MANAGER_BASE::OnTableChar ), NULL, this );
	m_viewZonesOverview->Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_ZONE_MANAGER_BASE::OnTableCharHook ), NULL, this );
	m_viewZonesOverview->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_ZONE_MANAGER_BASE::OnDataViewCtrlSelectionChanged ), NULL, this );
	m_viewZonesOverview->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( DIALOG_ZONE_MANAGER_BASE::OnViewZonesOverviewOnLeftUp ), NULL, this );
	m_btnMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnMoveUpClick ), NULL, this );
	m_btnMoveDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnMoveDownClick ), NULL, this );
	m_updateDisplayedZones->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnUpdateDisplayedZonesClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnOk ), NULL, this );
}

DIALOG_ZONE_MANAGER_BASE::~DIALOG_ZONE_MANAGER_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_ZONE_MANAGER_BASE::onDialogResize ) );
	m_filterCtrl->Disconnect( wxEVT_COMMAND_SEARCHCTRL_CANCEL_BTN, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnFilterCtrlCancel ), NULL, this );
	m_filterCtrl->Disconnect( wxEVT_COMMAND_SEARCHCTRL_SEARCH_BTN, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnFilterCtrlSearch ), NULL, this );
	m_filterCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnFilterCtrlTextChange ), NULL, this );
	m_filterCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnFilterCtrlEnter ), NULL, this );
	m_viewZonesOverview->Disconnect( wxEVT_CHAR, wxKeyEventHandler( DIALOG_ZONE_MANAGER_BASE::OnTableChar ), NULL, this );
	m_viewZonesOverview->Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_ZONE_MANAGER_BASE::OnTableCharHook ), NULL, this );
	m_viewZonesOverview->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_ZONE_MANAGER_BASE::OnDataViewCtrlSelectionChanged ), NULL, this );
	m_viewZonesOverview->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( DIALOG_ZONE_MANAGER_BASE::OnViewZonesOverviewOnLeftUp ), NULL, this );
	m_btnMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnMoveUpClick ), NULL, this );
	m_btnMoveDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnMoveDownClick ), NULL, this );
	m_updateDisplayedZones->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnUpdateDisplayedZonesClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ZONE_MANAGER_BASE::OnOk ), NULL, this );

}
