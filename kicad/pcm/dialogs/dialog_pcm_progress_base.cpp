///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_pcm_progress_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PCM_PROGRESS_BASE::DIALOG_PCM_PROGRESS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 400,300 ), wxDefaultSize );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	m_panelDownload = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( m_panelDownload, wxID_ANY, _("Download Progress") ), wxVERTICAL );

	m_downloadText = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Waiting..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_downloadText->Wrap( -1 );
	sbSizer1->Add( m_downloadText, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_downloadGauge = new wxGauge( sbSizer1->GetStaticBox(), wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL|wxGA_SMOOTH );
	m_downloadGauge->SetValue( 0 );
	sbSizer1->Add( m_downloadGauge, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_panelDownload->SetSizer( sbSizer1 );
	m_panelDownload->Layout();
	sbSizer1->Fit( m_panelDownload );
	bSizer1->Add( m_panelDownload, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_panel2 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( m_panel2, wxID_ANY, _("Overall Progress") ), wxVERTICAL );

	m_overallGauge = new wxGauge( sbSizer2->GetStaticBox(), wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL );
	m_overallGauge->SetValue( 0 );
	sbSizer2->Add( m_overallGauge, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizer3->Add( sbSizer2, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( m_panel2, wxID_ANY, _("Details") ), wxVERTICAL );

	m_reporter = new WX_HTML_REPORT_BOX( sbSizer3->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxHW_SCROLLBAR_AUTO|wxHSCROLL|wxVSCROLL );
	sbSizer3->Add( m_reporter, 1, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );


	bSizer3->Add( sbSizer3, 1, wxEXPAND|wxTOP, 5 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );


	bSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	m_buttonCancel = new wxButton( m_panel2, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_buttonCancel, 0, wxALL, 5 );


	bSizer2->Add( 20, 0, 0, wxEXPAND, 5 );

	m_buttonClose = new wxButton( m_panel2, wxID_OK, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );

	m_buttonClose->SetDefault();
	m_buttonClose->Enable( false );

	bSizer2->Add( m_buttonClose, 0, wxALL, 5 );


	bSizer3->Add( bSizer2, 0, wxEXPAND, 5 );


	m_panel2->SetSizer( bSizer3 );
	m_panel2->Layout();
	bSizer3->Fit( m_panel2 );
	bSizer1->Add( m_panel2, 1, wxEXPAND | wxALL, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();

	// Connect Events
	m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_PROGRESS_BASE::OnCancelClicked ), NULL, this );
	m_buttonClose->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_PROGRESS_BASE::OnCloseClicked ), NULL, this );
}

DIALOG_PCM_PROGRESS_BASE::~DIALOG_PCM_PROGRESS_BASE()
{
	// Disconnect Events
	m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_PROGRESS_BASE::OnCancelClicked ), NULL, this );
	m_buttonClose->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCM_PROGRESS_BASE::OnCloseClicked ), NULL, this );

}
