///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version May 21 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
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
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("General"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	m_staticText1->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	dlgBorderSizer->Add( m_staticText1, 0, wxALIGN_LEFT, 3 );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer3->Add( 12, 0, 0, wxEXPAND, 3 );
	
	m_staticWidth = new wxStaticText( this, wxID_ANY, _("&Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticWidth->Wrap( -1 );
	bSizer3->Add( m_staticWidth, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	bSizer3->Add( 0, 0, 1, wxEXPAND, 3 );
	
	m_textWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_textWidth, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticWidthUnits->Wrap( -1 );
	bSizer3->Add( m_staticWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	dlgBorderSizer->Add( bSizer3, 0, wxALL|wxEXPAND, 0 );
	
	m_staticTextSharing = new wxStaticText( this, ID_M_STATICTEXTSHARING, _("Sharing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSharing->Wrap( -1 );
	m_staticTextSharing->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	dlgBorderSizer->Add( m_staticTextSharing, 0, wxTOP|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer4->Add( 12, 0, 0, wxEXPAND, 3 );
	
	m_checkApplyToAllUnits = new wxCheckBox( this, wxID_ANY, _("Common to all &units in component"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_checkApplyToAllUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	dlgBorderSizer->Add( bSizer4, 0, wxALL|wxEXPAND, 0 );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer5->Add( 12, 0, 0, wxEXPAND, 3 );
	
	m_checkApplyToAllConversions = new wxCheckBox( this, wxID_ANY, _("Common to all body &styles (DeMorgan)"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_checkApplyToAllConversions, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	dlgBorderSizer->Add( bSizer5, 0, wxEXPAND, 3 );
	
	
	dlgBorderSizer->Add( 0, 0, 0, wxALL|wxEXPAND, 10 );
	
	m_staticText4 = new wxStaticText( this, wxID_ANY, _("Fill Style"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	m_staticText4->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	dlgBorderSizer->Add( m_staticText4, 0, wxALIGN_LEFT|wxBOTTOM, 3 );
	
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer6->Add( 12, 0, 0, wxEXPAND, 3 );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	m_radioFillNone = new wxRadioButton( this, wxID_ANY, _("Do &not fill"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_radioFillNone->SetValue( true ); 
	bSizer7->Add( m_radioFillNone, 0, wxALL, 3 );
	
	m_radioFillForeground = new wxRadioButton( this, wxID_ANY, _("Fill &foreground"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_radioFillForeground, 0, wxALL, 3 );
	
	m_radioFillBackground = new wxRadioButton( this, wxID_ANY, _("Fill &background"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_radioFillBackground, 0, wxALL, 3 );
	
	
	bSizer6->Add( bSizer7, 0, wxEXPAND, 0 );
	
	
	dlgBorderSizer->Add( bSizer6, 1, wxALL|wxEXPAND, 0 );
	
	
	dlgBorderSizer->Add( 0, 0, 0, wxALL|wxEXPAND, 10 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	dlgBorderSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 0 );
	
	
	mainSizer->Add( dlgBorderSizer, 1, wxALL|wxEXPAND, 12 );
	
	
	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
	
	this->Centre( wxBOTH );
}

DIALOG_LIB_EDIT_DRAW_ITEM_BASE::~DIALOG_LIB_EDIT_DRAW_ITEM_BASE()
{
}
