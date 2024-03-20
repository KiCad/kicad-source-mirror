///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_regulator_form_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_REGULATOR_FORM_BASE::DIALOG_REGULATOR_FORM_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	bSizerMain->SetMinSize( wxSize( 300,-1 ) );
	wxFlexGridSizer* fgSizerPrms;
	fgSizerPrms = new wxFlexGridSizer( 4, 3, 0, 0 );
	fgSizerPrms->AddGrowableCol( 1 );
	fgSizerPrms->AddGrowableRow( 0 );
	fgSizerPrms->SetFlexibleDirection( wxHORIZONTAL );
	fgSizerPrms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextName = new wxStaticText( this, wxID_ANY, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextName->Wrap( -1 );
	fgSizerPrms->Add( m_staticTextName, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_textCtrlName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPrms->Add( m_textCtrlName, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );


	fgSizerPrms->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextVref = new wxStaticText( this, wxID_ANY, _("Vref (min/typ/max):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextVref->Wrap( -1 );
	fgSizerPrms->Add( m_staticTextVref, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_vrefMinVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_vrefMinVal, 0, wxALL, 5 );

	m_vrefTypVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_vrefTypVal, 0, wxALL, 5 );

	m_vrefMaxVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_vrefMaxVal, 0, wxALL, 5 );


	fgSizerPrms->Add( bSizer2, 1, wxEXPAND, 5 );

	m_staticTextVrefUnit = new wxStaticText( this, wxID_ANY, _("Volt"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextVrefUnit->Wrap( -1 );
	fgSizerPrms->Add( m_staticTextVrefUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_staticTextType = new wxStaticText( this, wxID_ANY, _("Type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextType->Wrap( -1 );
	fgSizerPrms->Add( m_staticTextType, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxString m_choiceRegTypeChoices[] = { _("Standard Type"), _("3 Terminal Type") };
	int m_choiceRegTypeNChoices = sizeof( m_choiceRegTypeChoices ) / sizeof( wxString );
	m_choiceRegType = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceRegTypeNChoices, m_choiceRegTypeChoices, 0 );
	m_choiceRegType->SetSelection( 0 );
	fgSizerPrms->Add( m_choiceRegType, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );


	fgSizerPrms->Add( 0, 0, 1, wxEXPAND, 5 );

	m_RegulIadjTitle = new wxStaticText( this, wxID_ANY, _("Iadj (typ/max):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RegulIadjTitle->Wrap( -1 );
	fgSizerPrms->Add( m_RegulIadjTitle, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );


	bSizer3->Add( 0, 0, 1, 0, 5 );

	m_iadjTypVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_iadjTypVal, 0, wxALL, 5 );

	m_iadjMaxVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_iadjMaxVal, 0, wxALL, 5 );


	fgSizerPrms->Add( bSizer3, 1, wxEXPAND, 5 );

	m_labelUnitIadj = new wxStaticText( this, wxID_ANY, _("uA"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelUnitIadj->Wrap( -1 );
	fgSizerPrms->Add( m_labelUnitIadj, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bSizerMain->Add( fgSizerPrms, 0, wxEXPAND, 5 );


	bSizerMain->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerMain->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_choiceRegType->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_REGULATOR_FORM_BASE::OnRegTypeSelection ), NULL, this );
}

DIALOG_REGULATOR_FORM_BASE::~DIALOG_REGULATOR_FORM_BASE()
{
	// Disconnect Events
	m_choiceRegType->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_REGULATOR_FORM_BASE::OnRegTypeSelection ), NULL, this );

}
