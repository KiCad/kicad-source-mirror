///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_cleanup_graphics_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_CLEANUP_GRAPHICS_BASE::DIALOG_CLEANUP_GRAPHICS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxVERTICAL );

	m_createRectanglesOpt = new wxCheckBox( this, wxID_ANY, _("Merge lines into rectangles"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerUpper->Add( m_createRectanglesOpt, 0, wxALL, 5 );

	m_deleteRedundantOpt = new wxCheckBox( this, wxID_ANY, _("Delete redundant graphics"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerUpper->Add( m_deleteRedundantOpt, 0, wxALL, 5 );

	m_mergePadsOpt = new wxCheckBox( this, wxID_ANY, _("Merge overlapping graphics into pads"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerUpper->Add( m_mergePadsOpt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerMargins;
	bSizerMargins = new wxBoxSizer( wxVERTICAL );

	m_nettieHint = new wxStaticText( this, wxID_ANY, _("(Pads which appear in a Net Tie pad group will not be considered for merging.)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_nettieHint->Wrap( -1 );
	bSizerMargins->Add( m_nettieHint, 0, wxLEFT, 25 );


	bSizerUpper->Add( bSizerMargins, 1, wxEXPAND|wxALL, 3 );

	m_fixBoardOutlines = new wxCheckBox( this, wxID_ANY, _("Fix discontinuities in board outlines"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerUpper->Add( m_fixBoardOutlines, 0, wxALL, 5 );

	m_toleranceSizer = new wxBoxSizer( wxHORIZONTAL );

	m_toleranceLabel = new wxStaticText( this, wxID_ANY, _("Tolerance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_toleranceLabel->Wrap( -1 );
	m_toleranceSizer->Add( m_toleranceLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 25 );

	m_toleranceCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_toleranceSizer->Add( m_toleranceCtrl, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_toleranceUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_toleranceUnits->Wrap( -1 );
	m_toleranceSizer->Add( m_toleranceUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizerUpper->Add( m_toleranceSizer, 1, wxEXPAND, 5 );


	bSizerMain->Add( bSizerUpper, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxVERTICAL );

	bLowerSizer->SetMinSize( wxSize( 660,250 ) );
	staticChangesLabel = new wxStaticText( this, wxID_ANY, _("Changes to be applied:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticChangesLabel->Wrap( -1 );
	bLowerSizer->Add( staticChangesLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_changesDataView = new wxDataViewCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER );
	bLowerSizer->Add( m_changesDataView, 1, wxALL|wxEXPAND, 5 );


	bSizerMain->Add( bLowerSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerMain->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_createRectanglesOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_GRAPHICS_BASE::OnCheckBox ), NULL, this );
	m_deleteRedundantOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_GRAPHICS_BASE::OnCheckBox ), NULL, this );
	m_mergePadsOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_GRAPHICS_BASE::OnCheckBox ), NULL, this );
	m_fixBoardOutlines->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_GRAPHICS_BASE::OnCheckBox ), NULL, this );
	m_changesDataView->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_CLEANUP_GRAPHICS_BASE::OnSelectItem ), NULL, this );
	m_changesDataView->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_CLEANUP_GRAPHICS_BASE::OnLeftDClickItem ), NULL, this );
}

DIALOG_CLEANUP_GRAPHICS_BASE::~DIALOG_CLEANUP_GRAPHICS_BASE()
{
	// Disconnect Events
	m_createRectanglesOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_GRAPHICS_BASE::OnCheckBox ), NULL, this );
	m_deleteRedundantOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_GRAPHICS_BASE::OnCheckBox ), NULL, this );
	m_mergePadsOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_GRAPHICS_BASE::OnCheckBox ), NULL, this );
	m_fixBoardOutlines->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_GRAPHICS_BASE::OnCheckBox ), NULL, this );
	m_changesDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_CLEANUP_GRAPHICS_BASE::OnSelectItem ), NULL, this );
	m_changesDataView->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_CLEANUP_GRAPHICS_BASE::OnLeftDClickItem ), NULL, this );

}
