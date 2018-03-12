///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
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
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Fields to Update:") ), wxVERTICAL );
	
	wxArrayString m_fieldsBoxChoices;
	m_fieldsBox = new wxCheckListBox( sbSizer2->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fieldsBoxChoices, wxLB_NEEDED_SB );
	m_fieldsBox->SetMinSize( wxSize( -1,150 ) );
	
	sbSizer2->Add( m_fieldsBox, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* m_selBtnSizer;
	m_selBtnSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_selAllBtn = new wxButton( sbSizer2->GetStaticBox(), wxID_ANY, _("Select All"), wxDefaultPosition, wxDefaultSize, 0 );
	m_selBtnSizer->Add( m_selAllBtn, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_selNoneBtn = new wxButton( sbSizer2->GetStaticBox(), wxID_ANY, _("Select None"), wxDefaultPosition, wxDefaultSize, 0 );
	m_selBtnSizer->Add( m_selNoneBtn, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	sbSizer2->Add( m_selBtnSizer, 0, wxEXPAND, 5 );
	
	
	bUpperSizer->Add( sbSizer2, 1, wxEXPAND|wxTOP|wxLEFT, 10 );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options:") ), wxVERTICAL );
	
	m_removeExtraBox = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Remove fields not in library"), wxDefaultPosition, wxDefaultSize, 0 );
	m_removeExtraBox->SetToolTip( _("Removes fields that do not occur in the original library symbols") );
	
	sbSizer1->Add( m_removeExtraBox, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_resetEmpty = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Reset fields which are empty in library"), wxDefaultPosition, wxDefaultSize, 0 );
	m_resetEmpty->SetToolTip( _("Do not clear existing entries if library field is empty") );
	
	sbSizer1->Add( m_resetEmpty, 0, wxALL, 5 );
	
	
	sbSizer1->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_resetVisibility = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Reset field visibilities"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_resetVisibility, 0, wxALL, 5 );
	
	m_resetSizeAndStyle = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Reset field text sizes and styles"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_resetSizeAndStyle, 0, wxALL, 5 );
	
	m_resetPosition = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Reset field positions"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_resetPosition, 0, wxALL, 5 );
	
	
	bUpperSizer->Add( sbSizer1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );
	
	
	m_mainSizer->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
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
