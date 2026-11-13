///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "constraint_rules_list_base.h"

///////////////////////////////////////////////////////////////////////////

CONSTRAINT_RULES_LIST_BASE::CONSTRAINT_RULES_LIST_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	m_list = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxSize( -1,140 ), wxLC_REPORT|wxLC_SINGLE_SEL );
	m_list->SetToolTip( _("The rules for this constraint, in the order they are read. Select one to load it into the form below.") );

	mainSizer->Add( m_list, 1, wxBOTTOM|wxEXPAND, 4 );

	m_emptyLabel = new wxStaticText( this, wxID_ANY, _("No rules yet."), wxDefaultPosition, wxDefaultSize, 0 );
	m_emptyLabel->Wrap( -1 );
	mainSizer->Add( m_emptyLabel, 0, wxBOTTOM|wxLEFT, 4 );

	wxBoxSizer* tableButtons;
	tableButtons = new wxBoxSizer( wxHORIZONTAL );

	m_upBtn = new wxButton( this, wxID_ANY, _("Move Up"), wxDefaultPosition, wxDefaultSize, 0 );
	m_upBtn->SetToolTip( _("Move the selected rule one place earlier. Rules are read from top to bottom and the last one that matches decides the value, so a rule lower down wins.") );

	tableButtons->Add( m_upBtn, 0, wxRIGHT, 4 );

	m_downBtn = new wxButton( this, wxID_ANY, _("Move Down"), wxDefaultPosition, wxDefaultSize, 0 );
	m_downBtn->SetToolTip( _("Move the selected rule one place later. Rules are read from top to bottom and the last one that matches decides the value, so a rule lower down wins.") );

	tableButtons->Add( m_downBtn, 0, wxRIGHT, 4 );

	m_duplicateBtn = new wxButton( this, wxID_ANY, _("Duplicate"), wxDefaultPosition, wxDefaultSize, 0 );
	m_duplicateBtn->SetToolTip( _("Copy the selected rule and place the copy after it.") );

	tableButtons->Add( m_duplicateBtn, 0, wxRIGHT, 4 );

	m_removeBtn = new wxButton( this, wxID_ANY, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
	m_removeBtn->SetToolTip( _("Delete the selected rule.") );

	tableButtons->Add( m_removeBtn, 0, wxRIGHT, 4 );


	mainSizer->Add( tableButtons, 0, wxBOTTOM, 4 );

	m_editorHost = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_editorSizer = new wxBoxSizer( wxVERTICAL );


	m_editorHost->SetSizer( m_editorSizer );
	m_editorHost->Layout();
	m_editorSizer->Fit( m_editorHost );
	mainSizer->Add( m_editorHost, 0, wxEXPAND|wxTOP, 8 );

	wxBoxSizer* formButtons;
	formButtons = new wxBoxSizer( wxHORIZONTAL );

	m_primaryBtn = new wxButton( this, wxID_ANY, _("Add Rule"), wxDefaultPosition, wxDefaultSize, 0 );
	formButtons->Add( m_primaryBtn, 0, wxRIGHT, 4 );

	m_newBtn = new wxButton( this, wxID_ANY, _("New Rule"), wxDefaultPosition, wxDefaultSize, 0 );
	m_newBtn->SetToolTip( _("Empty the form so another rule can be written. Anything typed and not added is discarded.") );

	formButtons->Add( m_newBtn, 0, wxRIGHT, 4 );


	mainSizer->Add( formButtons, 0, wxBOTTOM|wxTOP, 6 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
}

CONSTRAINT_RULES_LIST_BASE::~CONSTRAINT_RULES_LIST_BASE()
{
}
