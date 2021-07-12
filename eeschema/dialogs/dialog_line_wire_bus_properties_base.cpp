///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/color_swatch.h"

#include "dialog_line_wire_bus_properties_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_LINE_WIRE_BUS_PROPERTIES_BASE, DIALOG_SHIM )
	EVT_BUTTON( wxID_APPLY, DIALOG_LINE_WIRE_BUS_PROPERTIES_BASE::_wxFB_resetDefaults )
END_EVENT_TABLE()

DIALOG_LINE_WIRE_BUS_PROPERTIES_BASE::DIALOG_LINE_WIRE_BUS_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizerGeneral;
	fgSizerGeneral = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerGeneral->AddGrowableCol( 1 );
	fgSizerGeneral->SetFlexibleDirection( wxBOTH );
	fgSizerGeneral->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextWidth = new wxStaticText( this, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextWidth->Wrap( -1 );
	fgSizerGeneral->Add( m_staticTextWidth, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 3 );

	m_lineWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	fgSizerGeneral->Add( m_lineWidth, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 3 );

	m_staticWidthUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticWidthUnits->Wrap( -1 );
	m_staticWidthUnits->SetMinSize( wxSize( 40,-1 ) );

	fgSizerGeneral->Add( m_staticWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 3 );

	m_staticTextColor = new wxStaticText( this, wxID_ANY, _("Color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextColor->Wrap( -1 );
	fgSizerGeneral->Add( m_staticTextColor, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_panel1 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	m_colorSwatch = new COLOR_SWATCH( m_panel1, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_colorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panel1->SetSizer( bSizer2 );
	m_panel1->Layout();
	bSizer2->Fit( m_panel1 );
	fgSizerGeneral->Add( m_panel1, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 2 );


	fgSizerGeneral->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextStyle = new wxStaticText( this, wxID_ANY, _("Style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStyle->Wrap( -1 );
	fgSizerGeneral->Add( m_staticTextStyle, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_typeCombo = new wxBitmapComboBox( this, wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_typeCombo->SetMinSize( wxSize( 240,-1 ) );

	fgSizerGeneral->Add( m_typeCombo, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 3 );


	mainSizer->Add( fgSizerGeneral, 1, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 10 );

	m_helpLabel1 = new wxStaticText( this, wxID_ANY, _("Set width to 0 to use Schematic Editor line widths."), wxDefaultPosition, wxDefaultSize, 0 );
	m_helpLabel1->Wrap( 333 );
	mainSizer->Add( m_helpLabel1, 0, wxRIGHT|wxLEFT, 10 );

	m_helpLabel2 = new wxStaticText( this, wxID_ANY, _("Set color to transparent to use Schematic Editor colors."), wxDefaultPosition, wxDefaultSize, 0 );
	m_helpLabel2->Wrap( -1 );
	mainSizer->Add( m_helpLabel2, 0, wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

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
	mainSizer->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_LINE_WIRE_BUS_PROPERTIES_BASE::~DIALOG_LINE_WIRE_BUS_PROPERTIES_BASE()
{
}
