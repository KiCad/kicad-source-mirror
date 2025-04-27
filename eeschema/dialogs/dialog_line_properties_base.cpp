///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/color_swatch.h"

#include "dialog_line_properties_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_LINE_PROPERTIES_BASE, DIALOG_SHIM )
	EVT_BUTTON( wxID_APPLY, DIALOG_LINE_PROPERTIES_BASE::_wxFB_resetDefaults )
END_EVENT_TABLE()

DIALOG_LINE_PROPERTIES_BASE::DIALOG_LINE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 3, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextWidth = new wxStaticText( this, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextWidth->Wrap( -1 );
	gbSizer1->Add( m_staticTextWidth, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_lineWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_lineWidth->SetMinSize( wxSize( 146,-1 ) );

	gbSizer1->Add( m_lineWidth, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticWidthUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticWidthUnits->Wrap( -1 );
	m_staticWidthUnits->SetMinSize( wxSize( 40,-1 ) );

	gbSizer1->Add( m_staticWidthUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );

	m_staticTextColor = new wxStaticText( this, wxID_ANY, _("Color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextColor->Wrap( -1 );
	gbSizer1->Add( m_staticTextColor, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );

	m_panelColor = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxVERTICAL );

	m_colorSwatch = new COLOR_SWATCH( m_panelColor, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer21->Add( m_colorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panelColor->SetSizer( bSizer21 );
	m_panelColor->Layout();
	bSizer21->Fit( m_panelColor );
	gbSizer1->Add( m_panelColor, wxGBPosition( 0, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_staticTextStyle = new wxStaticText( this, wxID_ANY, _("Style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStyle->Wrap( -1 );
	gbSizer1->Add( m_staticTextStyle, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_typeCombo = new wxBitmapComboBox( this, wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_typeCombo->SetMinSize( wxSize( 240,-1 ) );

	gbSizer1->Add( m_typeCombo, wxGBPosition( 1, 1 ), wxGBSpan( 1, 4 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->AddGrowableCol( 1 );

	mainSizer->Add( gbSizer1, 0, wxEXPAND|wxALL, 10 );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	m_helpLabel1 = new wxStaticText( this, wxID_ANY, _("Set width to 0 to use schematic's default line width."), wxDefaultPosition, wxDefaultSize, 0 );
	m_helpLabel1->Wrap( -1 );
	bMargins->Add( m_helpLabel1, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_helpLabel2 = new wxStaticText( this, wxID_ANY, _("Clear color to use Schematic Editor colors."), wxDefaultPosition, wxDefaultSize, 0 );
	m_helpLabel2->Wrap( -1 );
	bMargins->Add( m_helpLabel2, 0, wxBOTTOM|wxRIGHT, 5 );


	mainSizer->Add( bMargins, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerApply = new wxButton( this, wxID_APPLY );
	m_sdbSizer->AddButton( m_sdbSizerApply );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	mainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_LINE_PROPERTIES_BASE::~DIALOG_LINE_PROPERTIES_BASE()
{
}
