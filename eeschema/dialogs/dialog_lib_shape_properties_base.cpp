///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_lib_shape_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LIB_SHAPE_PROPERTIES_BASE::DIALOG_LIB_SHAPE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* dlgBorderSizer;
	dlgBorderSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerLineWidth;
	bSizerLineWidth = new wxBoxSizer( wxHORIZONTAL );

	m_widthLabel = new wxStaticText( this, wxID_ANY, _("Line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_widthLabel->Wrap( -1 );
	bSizerLineWidth->Add( m_widthLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );

	m_widthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLineWidth->Add( m_widthCtrl, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3 );

	m_widthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_widthUnits->Wrap( -1 );
	bSizerLineWidth->Add( m_widthUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 3 );


	dlgBorderSizer->Add( bSizerLineWidth, 0, wxEXPAND, 5 );

	wxString m_fillCtrlChoices[] = { _("Do not fill"), _("Fill with body outline color"), _("Fill with body background color") };
	int m_fillCtrlNChoices = sizeof( m_fillCtrlChoices ) / sizeof( wxString );
	m_fillCtrl = new wxRadioBox( this, wxID_ANY, _("Fill Style"), wxDefaultPosition, wxDefaultSize, m_fillCtrlNChoices, m_fillCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_fillCtrl->SetSelection( 0 );
	dlgBorderSizer->Add( m_fillCtrl, 0, wxEXPAND|wxTOP|wxBOTTOM, 10 );

	m_checkApplyToAllUnits = new wxCheckBox( this, wxID_ANY, _("Common to all &units in symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	dlgBorderSizer->Add( m_checkApplyToAllUnits, 0, wxALL, 3 );

	m_checkApplyToAllConversions = new wxCheckBox( this, wxID_ANY, _("Common to all body &styles (DeMorgan)"), wxDefaultPosition, wxDefaultSize, 0 );
	dlgBorderSizer->Add( m_checkApplyToAllConversions, 0, wxALL, 3 );


	mainSizer->Add( dlgBorderSizer, 1, wxALL|wxEXPAND, 10 );

	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	mainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_LIB_SHAPE_PROPERTIES_BASE::~DIALOG_LIB_SHAPE_PROPERTIES_BASE()
{
}
