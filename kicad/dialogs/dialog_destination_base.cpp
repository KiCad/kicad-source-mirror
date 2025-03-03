///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

#include "dialog_destination_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DESTINATION_BASE::DIALOG_DESTINATION_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 4, 2, 5, 5 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->AddGrowableRow( 3 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_textArchiveDesc = new wxStaticText( this, wxID_ANY, _("Description:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textArchiveDesc->Wrap( -1 );
	fgSizer1->Add( m_textArchiveDesc, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlDescription = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textCtrlDescription, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_textArchiveFormat = new wxStaticText( this, wxID_ANY, _("Format:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textArchiveFormat->Wrap( -1 );
	fgSizer1->Add( m_textArchiveFormat, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceArchiveformatChoices;
	m_choiceArchiveformat = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceArchiveformatChoices, 0 );
	m_choiceArchiveformat->SetSelection( 0 );
	m_choiceArchiveformat->SetMinSize( wxSize( 100,-1 ) );

	fgSizer1->Add( m_choiceArchiveformat, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_textOutputPath = new wxStaticText( this, wxID_ANY, _("Destination path:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOutputPath->Wrap( -1 );
	fgSizer1->Add( m_textOutputPath, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxHORIZONTAL );

	m_textCtrlOutputPath = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlOutputPath->SetMinSize( wxSize( 350,-1 ) );

	bSizer16->Add( m_textCtrlOutputPath, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_buttonOutputPath = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer16->Add( m_buttonOutputPath, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer1->Add( bSizer16, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText10 = new wxStaticText( this, wxID_ANY, _("Include jobs:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	fgSizer1->Add( m_staticText10, 0, wxTOP, 5 );

	wxArrayString m_includeJobsChoices;
	m_includeJobs = new wxCheckListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_includeJobsChoices, 0 );
	m_includeJobs->SetMinSize( wxSize( 300,200 ) );

	fgSizer1->Add( m_includeJobs, 1, wxEXPAND, 5 );


	bSizerMain->Add( fgSizer1, 1, wxEXPAND|wxALL, 10 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerMain->Add( m_sdbSizer1, 0, wxBOTTOM|wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_buttonOutputPath->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESTINATION_BASE::onOutputPathBrowseClicked ), NULL, this );
}

DIALOG_DESTINATION_BASE::~DIALOG_DESTINATION_BASE()
{
	// Disconnect Events
	m_buttonOutputPath->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESTINATION_BASE::onOutputPathBrowseClicked ), NULL, this );

}
