///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_constraint_value_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_CONSTRAINT_VALUE_BASE::DIALOG_CONSTRAINT_VALUE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bValueSizer;
	bValueSizer = new wxBoxSizer( wxHORIZONTAL );

	m_valueLabel = new wxStaticText( this, wxID_ANY, _("Value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_valueLabel->Wrap( -1 );
	bValueSizer->Add( m_valueLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_valueCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bValueSizer->Add( m_valueCtrl, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_valueUnits = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_valueUnits->Wrap( -1 );
	bValueSizer->Add( m_valueUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bMainSizer->Add( bValueSizer, 0, wxEXPAND|wxALL, 5 );

	m_drivingCtrl = new wxCheckBox( this, wxID_ANY, _("Driving (locks the geometry)"), wxDefaultPosition, wxDefaultSize, 0 );
	bMainSizer->Add( m_drivingCtrl, 0, wxALL, 8 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_CONSTRAINT_VALUE_BASE::~DIALOG_CONSTRAINT_VALUE_BASE()
{
}
