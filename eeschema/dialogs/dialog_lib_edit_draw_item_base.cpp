///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_lib_edit_draw_item_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LIB_EDIT_DRAW_ITEM_BASE::DIALOG_LIB_EDIT_DRAW_ITEM_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* dlgBorderSizer;
	dlgBorderSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );
	
	m_widthLabel = new wxStaticText( this, wxID_ANY, _("Line Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_widthLabel->Wrap( -1 );
	bSizer3->Add( m_widthLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_widthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_widthCtrl, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_widthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_widthUnits->Wrap( -1 );
	bSizer3->Add( m_widthUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 3 );
	
	
	dlgBorderSizer->Add( bSizer3, 0, wxEXPAND|wxBOTTOM, 10 );
	
	m_checkApplyToAllUnits = new wxCheckBox( this, wxID_ANY, _("Common to all &units in component"), wxDefaultPosition, wxDefaultSize, 0 );
	dlgBorderSizer->Add( m_checkApplyToAllUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_checkApplyToAllConversions = new wxCheckBox( this, wxID_ANY, _("Common to all body &styles (DeMorgan)"), wxDefaultPosition, wxDefaultSize, 0 );
	dlgBorderSizer->Add( m_checkApplyToAllConversions, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	dlgBorderSizer->Add( 0, 0, 0, wxEXPAND|wxTOP, 5 );
	
	wxString m_fillCtrlChoices[] = { _("Do not fill"), _("Fill foreground"), _("Fill background") };
	int m_fillCtrlNChoices = sizeof( m_fillCtrlChoices ) / sizeof( wxString );
	m_fillCtrl = new wxRadioBox( this, wxID_ANY, _("Fill Style"), wxDefaultPosition, wxDefaultSize, m_fillCtrlNChoices, m_fillCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_fillCtrl->SetSelection( 0 );
	dlgBorderSizer->Add( m_fillCtrl, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	dlgBorderSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 0 );
	
	
	mainSizer->Add( dlgBorderSizer, 1, wxALL|wxEXPAND, 10 );
	
	
	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
	
	this->Centre( wxBOTH );
}

DIALOG_LIB_EDIT_DRAW_ITEM_BASE::~DIALOG_LIB_EDIT_DRAW_ITEM_BASE()
{
}
