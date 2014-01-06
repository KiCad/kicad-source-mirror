///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct  8 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_export_idf_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXPORT_IDF3_BASE::DIALOG_EXPORT_IDF3_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerIDFFile;
	bSizerIDFFile = new wxBoxSizer( wxVERTICAL );
	
	m_txtBrdFile = new wxStaticText( this, wxID_ANY, wxT("IDF Board file"), wxDefaultPosition, wxDefaultSize, 0 );
	m_txtBrdFile->Wrap( -1 );
	bSizerIDFFile->Add( m_txtBrdFile, 0, wxALL, 5 );
	
	m_filePickerIDF = new wxFilePickerCtrl( this, wxID_ANY, wxEmptyString, wxT("Select a board file"), wxT("*.emn"), wxDefaultPosition, wxDefaultSize, wxFLP_OVERWRITE_PROMPT|wxFLP_SAVE|wxFLP_USE_TEXTCTRL );
	m_filePickerIDF->SetMinSize( wxSize( 420,30 ) );
	
	bSizerIDFFile->Add( m_filePickerIDF, 0, wxALL, 5 );
	
	m_chkThou = new wxCheckBox( this, wxID_ANY, wxT("unit: THOU"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerIDFFile->Add( m_chkThou, 0, wxALL, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bSizerIDFFile->Add( m_sdbSizer1, 1, wxEXPAND, 5 );
	
	
	this->SetSizer( bSizerIDFFile );
	this->Layout();
	
	this->Centre( wxBOTH );
}

DIALOG_EXPORT_IDF3_BASE::~DIALOG_EXPORT_IDF3_BASE()
{
}
