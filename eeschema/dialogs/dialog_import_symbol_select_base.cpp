///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_import_symbol_select_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_IMPORT_SYMBOL_SELECT_BASE::DIALOG_IMPORT_SYMBOL_SELECT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 600,450 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bContentSizer;
	bContentSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );

	m_searchCtrl = new wxSearchCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifndef __WXMAC__
	m_searchCtrl->ShowSearchButton( true );
	#endif
	m_searchCtrl->ShowCancelButton( false );
	bLeftSizer->Add( m_searchCtrl, 0, wxEXPAND|wxBOTTOM, 5 );

	m_symbolList = new wxDataViewListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_SINGLE );
	bLeftSizer->Add( m_symbolList, 1, wxEXPAND, 5 );

	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_selectAllButton = new wxButton( this, wxID_ANY, _("Select All"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_selectAllButton, 0, wxALL, 5 );

	m_selectNoneButton = new wxButton( this, wxID_ANY, _("Select None"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_selectNoneButton, 0, wxALL, 5 );


	bLeftSizer->Add( bButtonsSizer, 0, wxEXPAND|wxTOP, 5 );


	bContentSizer->Add( bLeftSizer, 2, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bUnitSizer;
	bUnitSizer = new wxBoxSizer( wxHORIZONTAL );

	m_unitLabel = new wxStaticText( this, wxID_ANY, _("Unit:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitLabel->Wrap( -1 );
	bUnitSizer->Add( m_unitLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxArrayString m_unitChoiceChoices;
	m_unitChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_unitChoiceChoices, 0 );
	m_unitChoice->SetSelection( 0 );
	m_unitChoice->SetMinSize( wxSize( 100,-1 ) );

	bUnitSizer->Add( m_unitChoice, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bRightSizer->Add( bUnitSizer, 0, wxEXPAND|wxBOTTOM, 5 );

	m_previewPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_previewSizer = new wxBoxSizer( wxVERTICAL );


	m_previewPanel->SetSizer( m_previewSizer );
	m_previewPanel->Layout();
	m_previewSizer->Fit( m_previewPanel );
	bRightSizer->Add( m_previewPanel, 1, wxEXPAND, 5 );


	bContentSizer->Add( bRightSizer, 3, wxEXPAND|wxALL, 5 );


	bMainSizer->Add( bContentSizer, 1, wxEXPAND, 5 );

	m_statusLine = new wxStaticText( this, wxID_ANY, _("0 symbols selected"), wxDefaultPosition, wxDefaultSize, 0 );
	m_statusLine->Wrap( -1 );
	bMainSizer->Add( m_statusLine, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_searchCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_IMPORT_SYMBOL_SELECT_BASE::OnFilterTextChanged ), NULL, this );
	m_symbolList->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_IMPORT_SYMBOL_SELECT_BASE::OnSymbolSelected ), NULL, this );
	m_selectAllButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SYMBOL_SELECT_BASE::OnSelectAll ), NULL, this );
	m_selectNoneButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SYMBOL_SELECT_BASE::OnSelectNone ), NULL, this );
	m_unitChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_SYMBOL_SELECT_BASE::OnUnitChanged ), NULL, this );
}

DIALOG_IMPORT_SYMBOL_SELECT_BASE::~DIALOG_IMPORT_SYMBOL_SELECT_BASE()
{
	// Disconnect Events
	m_searchCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_IMPORT_SYMBOL_SELECT_BASE::OnFilterTextChanged ), NULL, this );
	m_symbolList->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_IMPORT_SYMBOL_SELECT_BASE::OnSymbolSelected ), NULL, this );
	m_selectAllButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SYMBOL_SELECT_BASE::OnSelectAll ), NULL, this );
	m_selectNoneButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SYMBOL_SELECT_BASE::OnSelectNone ), NULL, this );
	m_unitChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_SYMBOL_SELECT_BASE::OnUnitChanged ), NULL, this );

}
