///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_update_symbol_fields_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_UPDATE_SYMBOL_FIELDS_BASE::DIALOG_UPDATE_SYMBOL_FIELDS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	m_newIdSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* m_parentSymbolLabel;
	m_parentSymbolLabel = new wxStaticText( this, wxID_ANY, _("Parent symbol:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_parentSymbolLabel->Wrap( -1 );
	m_newIdSizer->Add( m_parentSymbolLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_parentSymbolReadOnly = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_parentSymbolReadOnly->Enable( false );

	m_newIdSizer->Add( m_parentSymbolReadOnly, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	m_mainSizer->Add( m_newIdSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerUpdate;
	bSizerUpdate = new wxBoxSizer( wxHORIZONTAL );

	m_updateFieldsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Update/reset Fields") ), wxVERTICAL );

	wxArrayString m_fieldsBoxChoices;
	m_fieldsBox = new wxCheckListBox( m_updateFieldsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fieldsBoxChoices, wxLB_NEEDED_SB );
	m_fieldsBox->SetMinSize( wxSize( -1,120 ) );

	m_updateFieldsSizer->Add( m_fieldsBox, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* m_selBtnSizer;
	m_selBtnSizer = new wxBoxSizer( wxHORIZONTAL );

	m_selAllBtn = new wxButton( m_updateFieldsSizer->GetStaticBox(), wxID_ANY, _("Select All"), wxDefaultPosition, wxDefaultSize, 0 );
	m_selBtnSizer->Add( m_selAllBtn, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_selNoneBtn = new wxButton( m_updateFieldsSizer->GetStaticBox(), wxID_ANY, _("Select None"), wxDefaultPosition, wxDefaultSize, 0 );
	m_selBtnSizer->Add( m_selNoneBtn, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_updateFieldsSizer->Add( m_selBtnSizer, 0, wxEXPAND, 5 );


	bSizerUpdate->Add( m_updateFieldsSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 8 );

	m_updateOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Update Options") ), wxVERTICAL );

	m_removeExtraBox = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Remove fields if not in parent symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	m_removeExtraBox->SetToolTip( _("Removes fields that do not occur in the original library symbols") );

	m_updateOptionsSizer->Add( m_removeExtraBox, 0, wxBOTTOM|wxRIGHT, 4 );

	m_resetEmptyFields = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Reset fields if empty in parent symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	m_updateOptionsSizer->Add( m_resetEmptyFields, 0, wxBOTTOM|wxRIGHT, 4 );


	m_updateOptionsSizer->Add( 0, 15, 0, wxEXPAND, 5 );

	m_resetFieldText = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Update/reset field text"), wxDefaultPosition, wxDefaultSize, 0 );
	m_resetFieldText->SetValue(true);
	m_updateOptionsSizer->Add( m_resetFieldText, 0, wxBOTTOM|wxRIGHT, 4 );

	m_resetFieldVisibilities = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Update/reset field visibilities"), wxDefaultPosition, wxDefaultSize, 0 );
	m_resetFieldVisibilities->SetValue(true);
	m_updateOptionsSizer->Add( m_resetFieldVisibilities, 0, wxBOTTOM|wxRIGHT, 4 );

	m_resetFieldEffects = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Update/reset field sizes and styles"), wxDefaultPosition, wxDefaultSize, 0 );
	m_resetFieldEffects->SetValue(true);
	m_updateOptionsSizer->Add( m_resetFieldEffects, 0, wxBOTTOM|wxRIGHT, 4 );

	m_resetFieldPositions = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Update/reset field positions"), wxDefaultPosition, wxDefaultSize, 0 );
	m_resetFieldPositions->SetValue(true);
	m_updateOptionsSizer->Add( m_resetFieldPositions, 0, wxBOTTOM|wxRIGHT, 4 );


	bSizerUpdate->Add( m_updateOptionsSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 8 );


	m_mainSizer->Add( bSizerUpdate, 0, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_mainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( m_mainSizer );
	this->Layout();
	m_mainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_selAllBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_SYMBOL_FIELDS_BASE::onSelectAll ), NULL, this );
	m_selNoneBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_SYMBOL_FIELDS_BASE::onSelectNone ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_SYMBOL_FIELDS_BASE::onOkButtonClicked ), NULL, this );
}

DIALOG_UPDATE_SYMBOL_FIELDS_BASE::~DIALOG_UPDATE_SYMBOL_FIELDS_BASE()
{
	// Disconnect Events
	m_selAllBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_SYMBOL_FIELDS_BASE::onSelectAll ), NULL, this );
	m_selNoneBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_SYMBOL_FIELDS_BASE::onSelectNone ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_SYMBOL_FIELDS_BASE::onOkButtonClicked ), NULL, this );

}
