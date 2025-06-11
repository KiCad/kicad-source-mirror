///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_setup_annotation_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_ANNOTATION_BASE::PANEL_SETUP_ANNOTATION_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftColumn;
	bLeftColumn = new wxBoxSizer( wxVERTICAL );

	m_checkReuseRefdes = new wxCheckBox( this, wxID_ANY, _("Allow reference reuse"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkReuseRefdes->SetValue(true);
	m_checkReuseRefdes->SetToolTip( _("Allow reusing references from removed components") );

	bLeftColumn->Add( m_checkReuseRefdes, 0, wxLEFT|wxTOP, 12 );


	bLeftColumn->Add( 0, 5, 0, wxEXPAND, 5 );

	m_unitsLabel = new wxStaticText( this, wxID_ANY, _("Units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitsLabel->Wrap( -1 );
	bLeftColumn->Add( m_unitsLabel, 0, wxLEFT|wxRIGHT, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftColumn->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Symbol unit notation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	bSizer5->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxString m_choiceSeparatorRefIdChoices[] = { _("A"), _(".A"), _("-A"), _("_A"), _(".1"), _("-1"), _("_1") };
	int m_choiceSeparatorRefIdNChoices = sizeof( m_choiceSeparatorRefIdChoices ) / sizeof( wxString );
	m_choiceSeparatorRefId = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceSeparatorRefIdNChoices, m_choiceSeparatorRefIdChoices, 0 );
	m_choiceSeparatorRefId->SetSelection( 0 );
	bSizer5->Add( m_choiceSeparatorRefId, 1, wxEXPAND|wxRIGHT, 5 );


	bLeftColumn->Add( bSizer5, 0, wxEXPAND|wxTOP, 5 );


	bLeftColumn->Add( 0, 0, 1, wxEXPAND, 5 );


	bPanelSizer->Add( bLeftColumn, 0, wxEXPAND, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_SETUP_ANNOTATION_BASE::~PANEL_SETUP_ANNOTATION_BASE()
{
}
