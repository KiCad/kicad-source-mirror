///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_startwizard_libraries_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_STARTWIZARD_LIBRARIES_BASE::PANEL_STARTWIZARD_LIBRARIES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_stIntro = new wxStaticText( this, wxID_ANY, _("KiCad comes with a large set of symbol and footprint libraries maintained by the KiCad librarian team.  You may also create your own libraries, and install third-party ones from the Plugin and Content Manager or other sources.\n\nLibrary tables are the configuration files which list the libraries to be loaded. Global libraries are available in every project, and you may also add project-specific libraries if desired."), wxDefaultPosition, wxDefaultSize, 0 );
	m_stIntro->Wrap( 540 );
	bSizer8->Add( m_stIntro, 0, wxALL|wxEXPAND, 5 );

	m_stRequiredTablesLabel = new wxStaticText( this, wxID_ANY, _("<b>The following global library tables need to be created:</b>"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stRequiredTablesLabel->SetLabelMarkup( _("<b>The following global library tables need to be created:</b>") );
	m_stRequiredTablesLabel->Wrap( -1 );
	bSizer8->Add( m_stRequiredTablesLabel, 0, wxALL|wxEXPAND, 5 );

	m_stRequiredTables = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stRequiredTables->SetLabelMarkup( _("dummy") );
	m_stRequiredTables->Wrap( -1 );
	bSizer8->Add( m_stRequiredTables, 0, wxALL, 5 );

	m_stQuery = new wxStaticText( this, wxID_ANY, _("How would you like KiCad to create them?"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stQuery->Wrap( 400 );
	bSizer8->Add( m_stQuery, 0, wxALL, 5 );

	m_rbDefaultTables = new wxRadioButton( this, wxID_ANY, _("Start with the built-in KiCad libraries (recommended)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbDefaultTables->SetValue( true );
	bSizer8->Add( m_rbDefaultTables, 0, wxALL, 5 );

	m_rbImport = new wxRadioButton( this, wxID_ANY, _("Import tables from the previous version"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbImport->SetToolTip( _("Library tables will be imported from the version you selected on the previous step.  Any tables that cannot be imported will be created with the default KiCad libraries.") );

	bSizer8->Add( m_rbImport, 0, wxALL, 5 );

	m_rbBlankTables = new wxRadioButton( this, wxID_ANY, _("Start with no libraries"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_rbBlankTables, 0, wxALL, 5 );


	bPanelSizer->Add( bSizer8, 1, wxEXPAND, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
}

PANEL_STARTWIZARD_LIBRARIES_BASE::~PANEL_STARTWIZARD_LIBRARIES_BASE()
{
}
