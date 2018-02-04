///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Feb  6 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_update_fields_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_UPDATE_FIELDS_BASE::DIALOG_UPDATE_FIELDS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* m_mainSizer;
	m_mainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_infoLabel = new wxStaticText( this, wxID_ANY, _("Select fields to update:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_infoLabel->Wrap( -1 );
	m_mainSizer->Add( m_infoLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxArrayString m_fieldsBoxChoices;
	m_fieldsBox = new wxCheckListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fieldsBoxChoices, wxLB_NEEDED_SB );
	m_mainSizer->Add( m_fieldsBox, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* m_selBtnSizer;
	m_selBtnSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_selAllBtn = new wxButton( this, wxID_ANY, _("Select All"), wxDefaultPosition, wxDefaultSize, 0 );
	m_selBtnSizer->Add( m_selAllBtn, 1, wxALL|wxEXPAND, 5 );
	
	m_selNoneBtn = new wxButton( this, wxID_ANY, _("Select None"), wxDefaultPosition, wxDefaultSize, 0 );
	m_selBtnSizer->Add( m_selNoneBtn, 1, wxALL|wxEXPAND, 5 );
	
	
	m_mainSizer->Add( m_selBtnSizer, 0, wxEXPAND, 5 );
	
	m_removeExtraBox = new wxCheckBox( this, wxID_ANY, _("Remove extra fields"), wxDefaultPosition, wxDefaultSize, 0 );
	m_removeExtraBox->SetToolTip( _("Removes fields that do not occur in the original library symbols") );
	
	m_mainSizer->Add( m_removeExtraBox, 0, wxALL, 5 );
	
	m_omitEmpty = new wxCheckBox( this, wxID_ANY, _("Omit empty fields"), wxDefaultPosition, wxDefaultSize, 0 );
	m_omitEmpty->SetToolTip( _("Do not clear existing entries if library field is empty") );
	
	m_mainSizer->Add( m_omitEmpty, 0, wxALL, 5 );
	
	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_mainSizer->Add( m_staticline, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	m_mainSizer->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	
	this->SetSizer( m_mainSizer );
	this->Layout();
	m_mainSizer->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_selAllBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FIELDS_BASE::onSelectAll ), NULL, this );
	m_selNoneBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FIELDS_BASE::onSelectNone ), NULL, this );
}

DIALOG_UPDATE_FIELDS_BASE::~DIALOG_UPDATE_FIELDS_BASE()
{
	// Disconnect Events
	m_selAllBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FIELDS_BASE::onSelectAll ), NULL, this );
	m_selNoneBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_UPDATE_FIELDS_BASE::onSelectNone ), NULL, this );
	
}
