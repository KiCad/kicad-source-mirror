///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_footprint_checker_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FOOTPRINT_CHECKER_BASE::DIALOG_FOOTPRINT_CHECKER_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxVERTICAL );

	bUpperSizer->SetMinSize( wxSize( 660,250 ) );
	m_markersDataView = new wxDataViewCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER );
	bUpperSizer->Add( m_markersDataView, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSeveritySizer;
	bSeveritySizer = new wxBoxSizer( wxHORIZONTAL );

	m_showLabel = new wxStaticText( this, wxID_ANY, _("Show:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_showLabel->Wrap( -1 );
	bSeveritySizer->Add( m_showLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_showAll = new wxCheckBox( this, wxID_ANY, _("All"), wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_showAll, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSeveritySizer->Add( 35, 0, 0, wxEXPAND, 5 );

	m_showErrors = new wxCheckBox( this, wxID_ANY, _("Errors"), wxDefaultPosition, wxDefaultSize, 0 );
	m_showErrors->SetValue(true);
	bSeveritySizer->Add( m_showErrors, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_errorsBadge = new NUMBER_BADGE( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_errorsBadge, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 25 );

	m_showWarnings = new wxCheckBox( this, wxID_ANY, _("Warnings"), wxDefaultPosition, wxDefaultSize, 0 );
	m_showWarnings->SetValue(true);
	bSeveritySizer->Add( m_showWarnings, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_warningsBadge = new NUMBER_BADGE( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_warningsBadge, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 25 );

	m_showExclusions = new wxCheckBox( this, wxID_ANY, _("Exclusions"), wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_showExclusions, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_exclusionsBadge = new NUMBER_BADGE( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_exclusionsBadge, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 25 );


	bSeveritySizer->Add( 5, 0, 1, wxEXPAND, 5 );


	bSizer9->Add( bSeveritySizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bUpperSizer->Add( bSizer9, 0, wxEXPAND|wxTOP, 3 );


	bSizerMain->Add( bUpperSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxHORIZONTAL );

	m_DeleteCurrentMarkerButton = new wxButton( this, wxID_ANY, _("Delete Marker"), wxDefaultPosition, wxDefaultSize, 0 );
	bLowerSizer->Add( m_DeleteCurrentMarkerButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_DeleteAllMarkersButton = new wxButton( this, wxID_ANY, _("Delete All Markers"), wxDefaultPosition, wxDefaultSize, 0 );
	bLowerSizer->Add( m_DeleteAllMarkersButton, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bLowerSizer->Add( m_sdbSizer, 1, wxEXPAND|wxALL, 5 );


	bSizerMain->Add( bLowerSizer, 0, wxEXPAND|wxLEFT, 10 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnClose ) );
	m_markersDataView->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnSelectItem ), NULL, this );
	m_markersDataView->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnLeftDClickItem ), NULL, this );
	m_showAll->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnSeverity ), NULL, this );
	m_showErrors->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnSeverity ), NULL, this );
	m_showWarnings->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnSeverity ), NULL, this );
	m_showExclusions->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnSeverity ), NULL, this );
	m_DeleteCurrentMarkerButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnDeleteOneClick ), NULL, this );
	m_DeleteAllMarkersButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnDeleteAllClick ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnRunChecksClick ), NULL, this );
}

DIALOG_FOOTPRINT_CHECKER_BASE::~DIALOG_FOOTPRINT_CHECKER_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnClose ) );
	m_markersDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnSelectItem ), NULL, this );
	m_markersDataView->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnLeftDClickItem ), NULL, this );
	m_showAll->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnSeverity ), NULL, this );
	m_showErrors->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnSeverity ), NULL, this );
	m_showWarnings->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnSeverity ), NULL, this );
	m_showExclusions->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnSeverity ), NULL, this );
	m_DeleteCurrentMarkerButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnDeleteOneClick ), NULL, this );
	m_DeleteAllMarkersButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnDeleteAllClick ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnRunChecksClick ), NULL, this );

}
