///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_edit_line_style_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_EDIT_LINE_STYLE_BASE, DIALOG_SHIM )
	EVT_BUTTON( idColorBtn, DIALOG_EDIT_LINE_STYLE_BASE::_wxFB_onColorButtonClicked )
	EVT_BUTTON( wxID_APPLY, DIALOG_EDIT_LINE_STYLE_BASE::_wxFB_resetDefaults )
END_EVENT_TABLE()

DIALOG_EDIT_LINE_STYLE_BASE::DIALOG_EDIT_LINE_STYLE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* dlgBorderSizer;
	dlgBorderSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("General") ), wxVERTICAL );
	
	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticWidth1 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("&Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticWidth1->Wrap( -1 );
	bSizer31->Add( m_staticWidth1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_lineWidth = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 2,-1 ), 0 );
	bSizer31->Add( m_lineWidth, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticWidthUnits = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("millimeter"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticWidthUnits->Wrap( -1 );
	bSizer31->Add( m_staticWidthUnits, 1, wxALIGN_CENTER_VERTICAL|wxALL, 1 );
	
	
	sbSizer1->Add( bSizer31, 4, wxALL|wxEXPAND, 1 );
	
	wxBoxSizer* bColorSizer;
	bColorSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText5 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("C&olor:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	bColorSizer->Add( m_staticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_colorButton = new wxBitmapButton( sbSizer1->GetStaticBox(), idColorBtn, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	bColorSizer->Add( m_colorButton, 1, wxALL, 5 );
	
	
	bColorSizer->Add( 0, 0, 1, 0, 1 );
	
	
	sbSizer1->Add( bColorSizer, 3, wxEXPAND|wxALL, 1 );
	
	
	bSizer7->Add( sbSizer1, 2, wxALL, 5 );
	
	wxString m_lineStyleChoices[] = { _("Solid"), _("Dashed"), _("Dotted"), _("Dash-Dot") };
	int m_lineStyleNChoices = sizeof( m_lineStyleChoices ) / sizeof( wxString );
	m_lineStyle = new wxRadioBox( this, wxID_ANY, _("&Pen Style"), wxDefaultPosition, wxDefaultSize, m_lineStyleNChoices, m_lineStyleChoices, 4, wxRA_SPECIFY_ROWS );
	m_lineStyle->SetSelection( 0 );
	bSizer7->Add( m_lineStyle, 1, wxALL, 5 );
	
	
	dlgBorderSizer->Add( bSizer7, 0, wxEXPAND, 5 );
	
	
	dlgBorderSizer->Add( 0, 0, 0, wxALL|wxEXPAND, 1 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Apply = new wxButton( this, wxID_APPLY );
	m_sdbSizer1->AddButton( m_sdbSizer1Apply );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	dlgBorderSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );
	
	
	mainSizer->Add( dlgBorderSizer, 1, wxALL|wxEXPAND, 12 );
	
	
	this->SetSizer( mainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
}

DIALOG_EDIT_LINE_STYLE_BASE::~DIALOG_EDIT_LINE_STYLE_BASE()
{
}
