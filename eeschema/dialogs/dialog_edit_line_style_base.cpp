///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version v3.8.0)
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
	mainSizer = new wxBoxSizer( wxVERTICAL );


	mainSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizerGeneral;
	fgSizerGeneral = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerGeneral->AddGrowableCol( 1 );
	fgSizerGeneral->SetFlexibleDirection( wxBOTH );
	fgSizerGeneral->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextWidth = new wxStaticText( this, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextWidth->Wrap( -1 );
	fgSizerGeneral->Add( m_staticTextWidth, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );

	m_lineWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_lineWidth->SetMinSize( wxSize( 80,-1 ) );

	fgSizerGeneral->Add( m_lineWidth, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );

	m_staticWidthUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticWidthUnits->Wrap( -1 );
	m_staticWidthUnits->SetMinSize( wxSize( 40,-1 ) );

	fgSizerGeneral->Add( m_staticWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );

	m_staticTextColor = new wxStaticText( this, wxID_ANY, _("Color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextColor->Wrap( -1 );
	fgSizerGeneral->Add( m_staticTextColor, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_colorButton = new wxBitmapButton( this, idColorBtn, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );

	m_colorButton->SetBitmap( wxNullBitmap );
	fgSizerGeneral->Add( m_colorButton, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 3 );


	fgSizerGeneral->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextStyle = new wxStaticText( this, wxID_ANY, _("Style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStyle->Wrap( -1 );
	fgSizerGeneral->Add( m_staticTextStyle, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_typeCombo = new wxBitmapComboBox( this, wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_typeCombo->SetMinSize( wxSize( 140,-1 ) );

	fgSizerGeneral->Add( m_typeCombo, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );


	mainSizer->Add( fgSizerGeneral, 3, wxEXPAND, 5 );


	mainSizer->Add( 0, 0, 1, wxALL|wxEXPAND, 1 );

	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline, 0, wxEXPAND | wxALL, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerApply = new wxButton( this, wxID_APPLY );
	m_sdbSizer->AddButton( m_sdbSizerApply );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	mainSizer->Add( m_sdbSizer, 0, wxALL|wxALIGN_RIGHT, 5 );


	this->SetSizer( mainSizer );
	this->Layout();

	this->Centre( wxBOTH );
}

DIALOG_EDIT_LINE_STYLE_BASE::~DIALOG_EDIT_LINE_STYLE_BASE()
{
}
