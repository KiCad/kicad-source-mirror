///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_multichannel_generate_rule_areas_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS_BASE::DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxSize( -1,-1 ) );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer3->AddGrowableCol( 0 );
	fgSizer3->AddGrowableRow( 0 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_sourceNotebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_sourceNotebook->SetMinSize( wxSize( 600,400 ) );

	m_panel1 = new wxPanel( m_sourceNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_sourceNotebook->AddPage( m_panel1, _("Sheets"), false );
	m_panel2 = new wxPanel( m_sourceNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_sourceNotebook->AddPage( m_panel2, _("Component Classes"), false );

	fgSizer3->Add( m_sourceNotebook, 1, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );


	fgSizer3->Add( bSizer4, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );

	m_cbReplaceExisting = new wxCheckBox( this, wxID_ANY, _("Replace existing placement rule areas"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer13->Add( m_cbReplaceExisting, 0, wxALL, 5 );

	m_cbGroupItems = new wxCheckBox( this, wxID_ANY, _("Group footprints with their placement rule areas"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer13->Add( m_cbGroupItems, 0, wxALL, 5 );


	fgSizer3->Add( bSizer13, 1, wxEXPAND, 5 );

	wxBoxSizer* bottomButtonsSizer;
	bottomButtonsSizer = new wxBoxSizer( wxHORIZONTAL );


	bottomButtonsSizer->Add( 10, 0, 1, 0, 5 );

	m_sdbSizerStdButtons = new wxStdDialogButtonSizer();
	m_sdbSizerStdButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsOK );
	m_sdbSizerStdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsCancel );
	m_sdbSizerStdButtons->Realize();

	bottomButtonsSizer->Add( m_sdbSizerStdButtons, 0, wxALL, 5 );


	fgSizer3->Add( bottomButtonsSizer, 0, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( fgSizer3 );
	this->Layout();
	fgSizer3->Fit( this );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS_BASE::OnInitDlg ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS_BASE::OnUpdateUI ) );
	m_sourceNotebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS_BASE::OnNotebookPageChanged ), NULL, this );
}

DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS_BASE::~DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS_BASE::OnInitDlg ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS_BASE::OnUpdateUI ) );
	m_sourceNotebook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS_BASE::OnNotebookPageChanged ), NULL, this );

}
