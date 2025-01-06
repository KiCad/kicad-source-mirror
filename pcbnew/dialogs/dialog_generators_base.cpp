///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_generators_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GENERATORS_BASE::DIALOG_GENERATORS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* m_MainSizer;
	m_MainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );

	m_Notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_Notebook->SetMinSize( wxSize( 640,-1 ) );

	m_panelPage1 = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerPage1;
	bSizerPage1 = new wxBoxSizer( wxVERTICAL );

	bSizerPage1->SetMinSize( wxSize( -1,320 ) );
	m_dataview1 = new wxDataViewCtrl( m_panelPage1, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_dataview1->SetToolTip( _("Click on items to highlight them on the board.") );

	bSizerPage1->Add( m_dataview1, 1, wxEXPAND|wxALL, 5 );


	bSizerPage1->Add( 0, 8, 0, wxEXPAND, 5 );


	m_panelPage1->SetSizer( bSizerPage1 );
	m_panelPage1->Layout();
	bSizerPage1->Fit( m_panelPage1 );
	m_Notebook->AddPage( m_panelPage1, _("Generators A (%s)"), false );

	bSizer13->Add( m_Notebook, 1, wxEXPAND, 10 );


	m_MainSizer->Add( bSizer13, 1, wxEXPAND, 5 );

	m_sizerButtons = new wxBoxSizer( wxHORIZONTAL );

	m_rebuildSelected = new wxButton( this, wxID_ANY, _("Rebuild Selected"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizerButtons->Add( m_rebuildSelected, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_rebuildThisType = new wxButton( this, wxID_ANY, _("Rebuild this type"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rebuildThisType->Hide();

	m_sizerButtons->Add( m_rebuildThisType, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );


	m_sizerButtons->Add( 0, 8, 1, wxEXPAND, 5 );

	m_rebuildAll = new wxButton( this, wxID_ANY, _("Rebuild All"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizerButtons->Add( m_rebuildAll, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_sizerButtons->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	m_MainSizer->Add( m_sizerButtons, 0, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_ACTIVATE, wxActivateEventHandler( DIALOG_GENERATORS_BASE::OnActivateDlg ) );
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_GENERATORS_BASE::OnClose ) );
	m_Notebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_GENERATORS_BASE::OnChangingNotebookPage ), NULL, this );
	m_rebuildSelected->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERATORS_BASE::OnRebuildSelectedClick ), NULL, this );
	m_rebuildThisType->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERATORS_BASE::OnRebuildTypeClick ), NULL, this );
	m_rebuildAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERATORS_BASE::OnRebuildAllClick ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERATORS_BASE::OnCancelClick ), NULL, this );
}

DIALOG_GENERATORS_BASE::~DIALOG_GENERATORS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_ACTIVATE, wxActivateEventHandler( DIALOG_GENERATORS_BASE::OnActivateDlg ) );
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_GENERATORS_BASE::OnClose ) );
	m_Notebook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_GENERATORS_BASE::OnChangingNotebookPage ), NULL, this );
	m_rebuildSelected->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERATORS_BASE::OnRebuildSelectedClick ), NULL, this );
	m_rebuildThisType->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERATORS_BASE::OnRebuildTypeClick ), NULL, this );
	m_rebuildAll->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERATORS_BASE::OnRebuildAllClick ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENERATORS_BASE::OnCancelClick ), NULL, this );

}
