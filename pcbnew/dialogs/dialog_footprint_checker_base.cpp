///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
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


	bSizerMain->Add( bUpperSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxHORIZONTAL );

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
	m_DeleteAllMarkersButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnDeleteAllClick ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINT_CHECKER_BASE::OnRunChecksClick ), NULL, this );

}
