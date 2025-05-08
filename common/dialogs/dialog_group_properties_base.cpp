///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

#include "dialog_group_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GROUP_PROPERTIES_BASE::DIALOG_GROUP_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_nameLabel = new wxStaticText( this, wxID_ANY, _("Group name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_nameLabel->Wrap( -1 );
	fgSizer1->Add( m_nameLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_nameCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_nameCtrl, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_libraryLinkLabel = new wxStaticText( this, wxID_ANY, _("Library link:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_libraryLinkLabel->Wrap( -1 );
	fgSizer1->Add( m_libraryLinkLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_libraryLink = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_libraryLink, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );


	bSizerUpper->Add( fgSizer1, 0, wxEXPAND, 5 );

	m_locked = new wxCheckBox( this, wxID_ANY, _("Locked"), wxDefaultPosition, wxDefaultSize, 0 );
	m_locked->SetToolTip( _("Prevents group from being moved on canvas") );

	bSizerUpper->Add( m_locked, 0, wxALL, 5 );

	m_membersLabel = new wxStaticText( this, wxID_ANY, _("Group members:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_membersLabel->Wrap( -1 );
	bSizerUpper->Add( m_membersLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_membersList = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_membersList->SetMinSize( wxSize( 360,200 ) );

	bSizerUpper->Add( m_membersList, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bMembershipButtons;
	bMembershipButtons = new wxBoxSizer( wxHORIZONTAL );

	m_bpAddMember = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bMembershipButtons->Add( m_bpAddMember, 0, wxALL, 5 );


	bMembershipButtons->Add( 10, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_bpRemoveMember = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bMembershipButtons->Add( m_bpRemoveMember, 0, wxALL, 5 );


	bSizerUpper->Add( bMembershipButtons, 0, wxBOTTOM|wxEXPAND, 5 );


	bSizerMain->Add( bSizerUpper, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerMain->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_GROUP_PROPERTIES_BASE::onClose ) );
	m_membersList->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_GROUP_PROPERTIES_BASE::OnMemberSelected ), NULL, this );
	m_bpAddMember->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GROUP_PROPERTIES_BASE::OnAddMember ), NULL, this );
	m_bpRemoveMember->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GROUP_PROPERTIES_BASE::OnRemoveMember ), NULL, this );
}

DIALOG_GROUP_PROPERTIES_BASE::~DIALOG_GROUP_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_GROUP_PROPERTIES_BASE::onClose ) );
	m_membersList->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_GROUP_PROPERTIES_BASE::OnMemberSelected ), NULL, this );
	m_bpAddMember->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GROUP_PROPERTIES_BASE::OnAddMember ), NULL, this );
	m_bpRemoveMember->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GROUP_PROPERTIES_BASE::OnRemoveMember ), NULL, this );

}
