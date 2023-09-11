///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_rescue_each_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_RESCUE_EACH_BASE::DIALOG_RESCUE_EACH_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_htmlPrompt = new HTML_WINDOW( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	m_htmlPrompt->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	m_htmlPrompt->SetMinSize( wxSize( -1,80 ) );

	bSizer8->Add( m_htmlPrompt, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_titleSymbols = new wxStaticText( this, wxID_ANY, _("Symbols to update:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_titleSymbols->Wrap( -1 );
	bSizer8->Add( m_titleSymbols, 0, wxEXPAND|wxLEFT|wxRIGHT, 10 );


	bSizer8->Add( 0, 2, 0, wxEXPAND, 5 );

	m_ListOfConflicts = new wxDataViewListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_ListOfConflicts, 2, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	m_titleInstances = new wxStaticText( this, wxID_ANY, _("Instances of this symbol:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_titleInstances->Wrap( -1 );
	bSizer8->Add( m_titleInstances, 0, wxEXPAND|wxLEFT|wxRIGHT, 10 );


	bSizer8->Add( 0, 2, 0, wxEXPAND, 5 );

	m_ListOfInstances = new wxDataViewListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_ListOfInstances, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );


	bSizerMain->Add( bSizer8, 3, wxEXPAND, 5 );

	wxBoxSizer* bSizerPreviews;
	bSizerPreviews = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeftPreview;
	bSizerLeftPreview = new wxBoxSizer( wxVERTICAL );

	m_previewOldLabel = new wxStaticText( this, wxID_ANY, _("Cached Symbol:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_previewOldLabel->Wrap( -1 );
	bSizerLeftPreview->Add( m_previewOldLabel, 0, 0, 5 );

	m_previewOldPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_previewOldPanel->SetMinSize( wxSize( 350,-1 ) );

	m_SizerOldPanel = new wxBoxSizer( wxVERTICAL );


	m_previewOldPanel->SetSizer( m_SizerOldPanel );
	m_previewOldPanel->Layout();
	m_SizerOldPanel->Fit( m_previewOldPanel );
	bSizerLeftPreview->Add( m_previewOldPanel, 1, wxEXPAND | wxALL, 5 );


	bSizerPreviews->Add( bSizerLeftPreview, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerRightPreview;
	bSizerRightPreview = new wxBoxSizer( wxVERTICAL );

	m_previewNewLabel = new wxStaticText( this, wxID_ANY, _("Library Symbol:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_previewNewLabel->Wrap( -1 );
	bSizerRightPreview->Add( m_previewNewLabel, 0, 0, 5 );

	m_previewNewPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_previewNewPanel->SetMinSize( wxSize( 350,-1 ) );

	m_SizerNewPanel = new wxBoxSizer( wxVERTICAL );


	m_previewNewPanel->SetSizer( m_SizerNewPanel );
	m_previewNewPanel->Layout();
	m_SizerNewPanel->Fit( m_previewNewPanel );
	bSizerRightPreview->Add( m_previewNewPanel, 1, wxEXPAND | wxALL, 5 );


	bSizerPreviews->Add( bSizerRightPreview, 1, wxEXPAND, 5 );


	bSizerMain->Add( bSizerPreviews, 2, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	m_btnNeverShowAgain = new wxButton( this, wxID_ANY, _("Never Show Again"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_btnNeverShowAgain, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );


	bSizer5->Add( 0, 0, 1, wxEXPAND, 5 );

	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();

	bSizer5->Add( m_stdButtons, 0, wxALL|wxEXPAND, 5 );


	bSizerMain->Add( bSizer5, 0, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_RESCUE_EACH_BASE::OnDialogResize ) );
	m_ListOfConflicts->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_RESCUE_EACH_BASE::OnConflictSelect ), NULL, this );
	m_btnNeverShowAgain->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_RESCUE_EACH_BASE::OnNeverShowClick ), NULL, this );
	m_stdButtonsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_RESCUE_EACH_BASE::OnCancelClick ), NULL, this );
}

DIALOG_RESCUE_EACH_BASE::~DIALOG_RESCUE_EACH_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_RESCUE_EACH_BASE::OnDialogResize ) );
	m_ListOfConflicts->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_RESCUE_EACH_BASE::OnConflictSelect ), NULL, this );
	m_btnNeverShowAgain->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_RESCUE_EACH_BASE::OnNeverShowClick ), NULL, this );
	m_stdButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_RESCUE_EACH_BASE::OnCancelClick ), NULL, this );

}
