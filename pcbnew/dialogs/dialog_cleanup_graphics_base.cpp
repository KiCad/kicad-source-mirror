///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
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
	bSizerUpper->Add( m_deleteRedundantOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerMain->Add( bSizerUpper, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxVERTICAL );

	bLowerSizer->SetMinSize( wxSize( 660,250 ) );
	staticChangesLabel = new wxStaticText( this, wxID_ANY, _("Changes To Be Applied:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticChangesLabel->Wrap( -1 );
	bLowerSizer->Add( staticChangesLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_changesDataView = new wxDataViewCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER );
	bLowerSizer->Add( m_changesDataView, 1, wxALL|wxEXPAND, 5 );


	bSizerMain->Add( bLowerSizer, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

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
	m_changesDataView->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_CLEANUP_GRAPHICS_BASE::OnSelectItem ), NULL, this );
	m_changesDataView->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_CLEANUP_GRAPHICS_BASE::OnLeftDClickItem ), NULL, this );
}

DIALOG_CLEANUP_GRAPHICS_BASE::~DIALOG_CLEANUP_GRAPHICS_BASE()
{
	// Disconnect Events
	m_createRectanglesOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_GRAPHICS_BASE::OnCheckBox ), NULL, this );
	m_deleteRedundantOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CLEANUP_GRAPHICS_BASE::OnCheckBox ), NULL, this );
	m_changesDataView->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_CLEANUP_GRAPHICS_BASE::OnSelectItem ), NULL, this );
	m_changesDataView->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_CLEANUP_GRAPHICS_BASE::OnLeftDClickItem ), NULL, this );

}
