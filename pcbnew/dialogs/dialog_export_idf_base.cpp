///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov  6 2013)
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
	
	bSizerIDFFile->SetMinSize( wxSize( 500,-1 ) ); 
	m_txtBrdFile = new wxStaticText( this, wxID_ANY, _("IDF board file"), wxDefaultPosition, wxDefaultSize, 0 );
	m_txtBrdFile->Wrap( -1 );
	bSizerIDFFile->Add( m_txtBrdFile, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_filePickerIDF = new wxFilePickerCtrl( this, wxID_ANY, wxEmptyString, _("Select an IDF export filename"), wxT("*.emn"), wxDefaultPosition, wxSize( 450,-1 ), wxFLP_OVERWRITE_PROMPT|wxFLP_SAVE|wxFLP_USE_TEXTCTRL );
	bSizerIDFFile->Add( m_filePickerIDF, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxString m_rbUnitSelectionChoices[] = { _("Millimeters"), _("Mils") };
	int m_rbUnitSelectionNChoices = sizeof( m_rbUnitSelectionChoices ) / sizeof( wxString );
	m_rbUnitSelection = new wxRadioBox( this, wxID_ANY, _("IDF File Units"), wxDefaultPosition, wxDefaultSize, m_rbUnitSelectionNChoices, m_rbUnitSelectionChoices, 1, wxRA_SPECIFY_COLS );
	m_rbUnitSelection->SetSelection( 0 );
	bSizerIDFFile->Add( m_rbUnitSelection, 0, wxALL, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerIDFFile->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bSizerIDFFile->Add( m_sdbSizer1, 0, wxALIGN_RIGHT, 5 );
	
	
	this->SetSizer( bSizerIDFFile );
	this->Layout();
	
	this->Centre( wxBOTH );
}

DIALOG_EXPORT_IDF3_BASE::~DIALOG_EXPORT_IDF3_BASE()
{
}
