///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov 23 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_global_sym_lib_table_config_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG_BASE::DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxHORIZONTAL );

	bSizer2 = new wxBoxSizer( wxVERTICAL );

	m_staticText1 = new wxStaticText( this, wxID_ANY, _("KiCad has been run for the first time using the new symbol library table for accessing symbol libraries.  In order for KiCad to access symbol libraries, you must configure your global symbol library table.  Please select from one of the options below.  If you are not sure which option to select, please use the default selection."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( 500 );
	bSizer2->Add( m_staticText1, 0, wxALL, 5 );


	bSizer2->Add( 0, 0, 0, wxALL|wxEXPAND, 5 );

	m_defaultRb = new wxRadioButton( this, wxID_ANY, _("Copy default global symbol library table (recommended)"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_defaultRb->SetValue( true );
	m_defaultRb->SetToolTip( _("Select this option if you not sure about configuring the global symbol library table") );

	bSizer2->Add( m_defaultRb, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_customRb = new wxRadioButton( this, wxID_ANY, _("Copy custom global symbol library table"), wxDefaultPosition, wxDefaultSize, 0 );
	m_customRb->SetToolTip( _("Select this option to copy a symbol library table file other than the default") );

	bSizer2->Add( m_customRb, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_emptyRb = new wxRadioButton( this, wxID_ANY, _("Create an empty global symbol library table"), wxDefaultPosition, wxDefaultSize, 0 );
	m_emptyRb->SetToolTip( _("Select this option to define symbol libraries in project specific library tables") );

	bSizer2->Add( m_emptyRb, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizer2->Add( 0, 0, 0, wxALL|wxEXPAND, 5 );

	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Select global symbol library table file:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bSizer2->Add( m_staticText2, 0, wxALL, 5 );

	m_filePicker1 = new wxFilePickerCtrl( this, wxID_ANY, wxEmptyString, _("Select a file"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxFLP_DEFAULT_STYLE|wxFLP_FILE_MUST_EXIST|wxFLP_OPEN );
	m_filePicker1->Enable( false );

	bSizer2->Add( m_filePicker1, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer2->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1->Realize();

	bSizer2->Add( m_sdbSizer1, 1, wxEXPAND, 5 );


	bSizer1->Add( bSizer2, 1, wxALL|wxEXPAND, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();
	bSizer1->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_defaultRb->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG_BASE::onUpdateDefaultSelection ), NULL, this );
	m_filePicker1->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG_BASE::onUpdateFilePicker ), NULL, this );
}

DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG_BASE::~DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG_BASE()
{
	// Disconnect Events
	m_defaultRb->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG_BASE::onUpdateDefaultSelection ), NULL, this );
	m_filePicker1->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG_BASE::onUpdateFilePicker ), NULL, this );

}
