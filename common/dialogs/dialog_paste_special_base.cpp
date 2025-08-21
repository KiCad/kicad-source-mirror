///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_paste_special_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PASTE_SPECIAL_BASE::DIALOG_PASTE_SPECIAL_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* m_mainSizer;
	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* optionsSizer;
	optionsSizer = new wxBoxSizer( wxVERTICAL );

	wxString m_optionsChoices[] = { _("Assign unique reference designators to pasted symbols"), _("Keep existing reference designators, even if they are duplicated"), _("Clear reference designators on all pasted symbols") };
	int m_optionsNChoices = sizeof( m_optionsChoices ) / sizeof( wxString );
	m_options = new wxRadioBox( this, wxID_ANY, _("Reference Designators"), wxDefaultPosition, wxDefaultSize, m_optionsNChoices, m_optionsChoices, 1, wxRA_SPECIFY_COLS );
	m_options->SetSelection( 1 );
	optionsSizer->Add( m_options, 0, wxALL, 5 );

	m_clearNetsCB = new wxCheckBox( this, wxID_ANY, _("Clear net assignments"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearNetsCB->SetToolTip( _("Remove the net information from all connected items before pasting") );

	optionsSizer->Add( m_clearNetsCB, 0, wxALL, 5 );


	m_mainSizer->Add( optionsSizer, 1, wxALL|wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_mainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( m_mainSizer );
	this->Layout();
	m_mainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_options->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PASTE_SPECIAL_BASE::onRadioBoxEvent ), NULL, this );
}

DIALOG_PASTE_SPECIAL_BASE::~DIALOG_PASTE_SPECIAL_BASE()
{
	// Disconnect Events
	m_options->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PASTE_SPECIAL_BASE::onRadioBoxEvent ), NULL, this );

}
