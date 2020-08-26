///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/infobar.h"

#include "dialog_erc_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_ERC_BASE::DIALOG_ERC_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	m_infoBar = new WX_INFOBAR( this );
	m_infoBar->SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
	m_infoBar->SetEffectDuration( 500 );
	bSizer1->Add( m_infoBar, 0, wxEXPAND, 5 );

	wxBoxSizer* bercSizer;
	bercSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerMessages;
	bSizerMessages = new wxBoxSizer( wxVERTICAL );

	m_titleMessages = new wxStaticText( this, wxID_ANY, _("Messages:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_titleMessages->Wrap( -1 );
	m_titleMessages->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizerMessages->Add( m_titleMessages, 0, wxRIGHT|wxLEFT, 10 );

	m_MessagesList = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_MessagesList->SetMinSize( wxSize( 180,110 ) );

	bSizerMessages->Add( m_MessagesList, 1, wxEXPAND|wxLEFT, 5 );


	bupperSizer->Add( bSizerMessages, 1, wxEXPAND|wxBOTTOM, 5 );


	bercSizer->Add( bupperSizer, 1, wxEXPAND|wxTOP|wxRIGHT, 5 );

	m_textMarkers = new wxStaticText( this, wxID_ANY, _("Violations:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textMarkers->Wrap( -1 );
	m_textMarkers->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bercSizer->Add( m_textMarkers, 0, wxTOP|wxRIGHT|wxLEFT, 10 );

	m_markerDataView = new wxDataViewCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER );
	m_markerDataView->SetToolTip( _("Click on items to highlight them on the board.") );
	m_markerDataView->SetMinSize( wxSize( -1,200 ) );

	bercSizer->Add( m_markerDataView, 2, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSeveritySizer;
	bSeveritySizer = new wxBoxSizer( wxHORIZONTAL );

	m_showLabel = new wxStaticText( this, wxID_ANY, _("Show:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_showLabel->Wrap( -1 );
	bSeveritySizer->Add( m_showLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_showAll = new wxCheckBox( this, wxID_ANY, _("All"), wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_showAll, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSeveritySizer->Add( 35, 0, 0, wxEXPAND, 5 );

	m_showErrors = new wxCheckBox( this, wxID_ANY, _("Errors"), wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_showErrors, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_errorsBadge = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_errorsBadge->SetMinSize( wxSize( 20,20 ) );

	bSeveritySizer->Add( m_errorsBadge, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 25 );

	m_showWarnings = new wxCheckBox( this, wxID_ANY, _("Warnings"), wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_showWarnings, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_warningsBadge = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_warningsBadge->SetMinSize( wxSize( 20,20 ) );

	bSeveritySizer->Add( m_warningsBadge, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 25 );

	m_showExclusions = new wxCheckBox( this, wxID_ANY, _("Exclusions"), wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_showExclusions, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_exclusionsBadge = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_exclusionsBadge, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 25 );


	bSeveritySizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_saveReport = new wxButton( this, wxID_ANY, _("Save..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSeveritySizer->Add( m_saveReport, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bercSizer->Add( bSeveritySizer, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	bSizer1->Add( bercSizer, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 8 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer1->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_buttondelmarkers = new wxButton( this, ID_ERASE_DRC_MARKERS, _("Delete Markers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonsSizer->Add( m_buttondelmarkers, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 8 );


	m_buttonsSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	m_buttonsSizer->Add( m_sdbSizer1, 0, wxEXPAND|wxALL, 5 );


	bSizer1->Add( m_buttonsSizer, 0, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();
	bSizer1->Fit( this );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_ERC_BASE::OnCloseErcDialog ) );
	m_markerDataView->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler( DIALOG_ERC_BASE::OnERCItemDClick ), NULL, this );
	m_markerDataView->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler( DIALOG_ERC_BASE::OnERCItemRClick ), NULL, this );
	m_markerDataView->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_ERC_BASE::OnERCItemSelected ), NULL, this );
	m_showAll->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSeverity ), NULL, this );
	m_showErrors->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSeverity ), NULL, this );
	m_showWarnings->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSeverity ), NULL, this );
	m_showExclusions->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSeverity ), NULL, this );
	m_saveReport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSaveReport ), NULL, this );
	m_buttondelmarkers->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnEraseDrcMarkersClick ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnButtonCloseClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnRunERCClick ), NULL, this );
}

DIALOG_ERC_BASE::~DIALOG_ERC_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_ERC_BASE::OnCloseErcDialog ) );
	m_markerDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler( DIALOG_ERC_BASE::OnERCItemDClick ), NULL, this );
	m_markerDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler( DIALOG_ERC_BASE::OnERCItemRClick ), NULL, this );
	m_markerDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_ERC_BASE::OnERCItemSelected ), NULL, this );
	m_showAll->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSeverity ), NULL, this );
	m_showErrors->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSeverity ), NULL, this );
	m_showWarnings->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSeverity ), NULL, this );
	m_showExclusions->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSeverity ), NULL, this );
	m_saveReport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnSaveReport ), NULL, this );
	m_buttondelmarkers->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnEraseDrcMarkersClick ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnButtonCloseClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ERC_BASE::OnRunERCClick ), NULL, this );

}
