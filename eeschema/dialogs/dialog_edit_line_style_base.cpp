///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov  9 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_edit_line_style_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_EDIT_LINE_STYLE_BASE, DIALOG_SHIM )
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
	
	m_lineWidth = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_lineWidth->HasFlag( wxTE_MULTILINE ) )
	{
	m_lineWidth->SetMaxLength( 6 );
	}
	#else
	m_lineWidth->SetMaxLength( 6 );
	#endif
	bSizer31->Add( m_lineWidth, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticWidthUnits = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("millimeter"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticWidthUnits->Wrap( -1 );
	bSizer31->Add( m_staticWidthUnits, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	sbSizer1->Add( bSizer31, 0, wxALL|wxEXPAND, 1 );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText5 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("C&olor:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	bSizer5->Add( m_staticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_colorPicker = new wxColourPickerCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxColour( 0, 0, 0 ), wxDefaultPosition, wxDefaultSize, wxCLRP_DEFAULT_STYLE );
	bSizer5->Add( m_colorPicker, 0, wxALL, 5 );
	
	
	bSizer5->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	sbSizer1->Add( bSizer5, 0, wxEXPAND|wxALL, 1 );
	
	
	bSizer7->Add( sbSizer1, 3, wxALL|wxEXPAND, 5 );
	
	wxString m_lineStyleChoices[] = { _("Solid"), _("Dashed"), _("Dotted"), _("Dash-Dot") };
	int m_lineStyleNChoices = sizeof( m_lineStyleChoices ) / sizeof( wxString );
	m_lineStyle = new wxRadioBox( this, wxID_ANY, _("&Pen Style"), wxDefaultPosition, wxDefaultSize, m_lineStyleNChoices, m_lineStyleChoices, 4, wxRA_SPECIFY_ROWS );
	m_lineStyle->SetSelection( 1 );
	bSizer7->Add( m_lineStyle, 2, wxALL|wxEXPAND, 5 );
	
	
	dlgBorderSizer->Add( bSizer7, 1, wxEXPAND, 5 );
	
	
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
