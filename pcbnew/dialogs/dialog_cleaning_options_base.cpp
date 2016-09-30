///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_cleaning_options_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_CLEANING_OPTIONS_BASE::DIALOG_CLEANING_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxVERTICAL );
	
	m_cleanShortCircuitOpt = new wxCheckBox( this, wxID_ANY, _("Delete &track segments connecting different nets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cleanShortCircuitOpt->SetToolTip( _("remove track segments connecting nodes belonging to different nets (short circuit)") );
	
	bSizerUpper->Add( m_cleanShortCircuitOpt, 0, wxALL, 5 );
	
	m_cleanViasOpt = new wxCheckBox( this, wxID_ANY, _("&Delete redundant vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cleanViasOpt->SetToolTip( _("remove vias on through hole pads and superimposed vias") );
	
	bSizerUpper->Add( m_cleanViasOpt, 0, wxALL, 5 );
	
	m_mergeSegmOpt = new wxCheckBox( this, wxID_ANY, _("&Merge overlapping segments"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mergeSegmOpt->SetToolTip( _("merge aligned track segments, and remove null segments") );
	
	bSizerUpper->Add( m_mergeSegmOpt, 0, wxALL, 5 );
	
	m_deleteUnconnectedOpt = new wxCheckBox( this, wxID_ANY, _("Delete &dangling tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_deleteUnconnectedOpt->SetToolTip( _("delete tracks having at least one dangling end") );
	
	bSizerUpper->Add( m_deleteUnconnectedOpt, 0, wxALL, 5 );
	
	
	bSizerMain->Add( bSizerUpper, 1, wxEXPAND|wxALL, 5 );
	
	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerMain->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
}

DIALOG_CLEANING_OPTIONS_BASE::~DIALOG_CLEANING_OPTIONS_BASE()
{
}
