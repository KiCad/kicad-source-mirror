///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug 12 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_regulator_data_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EDITOR_DATA_BASE::DIALOG_EDITOR_DATA_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
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

	m_staticTextVref = new wxStaticText( this, wxID_ANY, _("Vref:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextVref->Wrap( -1 );
	fgSizerPrms->Add( m_staticTextVref, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_textCtrlVref = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPrms->Add( m_textCtrlVref, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );

	m_staticTextVrefUnit = new wxStaticText( this, wxID_ANY, _("Volt"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextVrefUnit->Wrap( -1 );
	fgSizerPrms->Add( m_staticTextVrefUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_staticTextType = new wxStaticText( this, wxID_ANY, _("Type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextType->Wrap( -1 );
	fgSizerPrms->Add( m_staticTextType, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxString m_choiceRegTypeChoices[] = { _("Separate sense pin"), _("3 terminals regulator") };
	int m_choiceRegTypeNChoices = sizeof( m_choiceRegTypeChoices ) / sizeof( wxString );
	m_choiceRegType = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceRegTypeNChoices, m_choiceRegTypeChoices, 0 );
	m_choiceRegType->SetSelection( 0 );
	fgSizerPrms->Add( m_choiceRegType, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );


	fgSizerPrms->Add( 0, 0, 1, wxEXPAND, 5 );

	m_RegulIadjTitle = new wxStaticText( this, wxID_ANY, _("Iadj:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RegulIadjTitle->Wrap( -1 );
	fgSizerPrms->Add( m_RegulIadjTitle, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_RegulIadjValue = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPrms->Add( m_RegulIadjValue, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );

	m_IadjUnitLabel = new wxStaticText( this, wxID_ANY, _("uA"), wxDefaultPosition, wxDefaultSize, 0 );
	m_IadjUnitLabel->Wrap( -1 );
	fgSizerPrms->Add( m_IadjUnitLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bSizerMain->Add( fgSizerPrms, 0, wxEXPAND, 5 );


	bSizerMain->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline, 0, wxEXPAND, 5 );

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
	m_choiceRegType->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_EDITOR_DATA_BASE::OnRegTypeSelection ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDITOR_DATA_BASE::OnOKClick ), NULL, this );
}

DIALOG_EDITOR_DATA_BASE::~DIALOG_EDITOR_DATA_BASE()
{
	// Disconnect Events
	m_choiceRegType->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_EDITOR_DATA_BASE::OnRegTypeSelection ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDITOR_DATA_BASE::OnOKClick ), NULL, this );

}
