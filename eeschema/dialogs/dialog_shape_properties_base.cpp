///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/color_swatch.h"
#include "widgets/wx_infobar.h"

#include "dialog_shape_properties_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_SHAPE_PROPERTIES_BASE, DIALOG_SHIM )
	EVT_CHECKBOX( wxID_ANY, DIALOG_SHAPE_PROPERTIES_BASE::_wxFB_onBorderChecked )
	EVT_CHECKBOX( wxID_ANY, DIALOG_SHAPE_PROPERTIES_BASE::_wxFB_onFillChecked )
END_EVENT_TABLE()

DIALOG_SHAPE_PROPERTIES_BASE::DIALOG_SHAPE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	m_infoBar = new WX_INFOBAR( this );
	m_infoBar->SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
	m_infoBar->SetEffectDuration( 500 );
	m_infoBar->Hide();

	mainSizer->Add( m_infoBar, 0, wxBOTTOM|wxEXPAND, 5 );

	m_textEntrySizer = new wxGridBagSizer( 3, 3 );
	m_textEntrySizer->SetFlexibleDirection( wxBOTH );
	m_textEntrySizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	m_textEntrySizer->SetEmptyCellSize( wxSize( 36,-1 ) );

	m_borderCheckbox = new wxCheckBox( this, wxID_ANY, _("Border"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textEntrySizer->Add( m_borderCheckbox, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), wxBOTTOM, 2 );

	m_borderWidthLabel = new wxStaticText( this, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthLabel->Wrap( -1 );
	m_textEntrySizer->Add( m_borderWidthLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_borderWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer7->Add( m_borderWidthCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_borderWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthUnits->Wrap( -1 );
	bSizer7->Add( m_borderWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );

	m_borderColorLabel = new wxStaticText( this, wxID_ANY, _("Color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderColorLabel->Wrap( -1 );
	bSizer7->Add( m_borderColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );


	bSizer7->Add( 5, 0, 0, 0, 5 );

	m_panelBorderColor = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer23;
	bSizer23 = new wxBoxSizer( wxVERTICAL );

	m_borderColorSwatch = new COLOR_SWATCH( m_panelBorderColor, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer23->Add( m_borderColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panelBorderColor->SetSizer( bSizer23 );
	m_panelBorderColor->Layout();
	bSizer23->Fit( m_panelBorderColor );
	bSizer7->Add( m_panelBorderColor, 0, wxALIGN_CENTER_VERTICAL, 5 );


	m_textEntrySizer->Add( bSizer7, wxGBPosition( 1, 1 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );

	m_borderStyleLabel = new wxStaticText( this, wxID_ANY, _("Style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderStyleLabel->Wrap( -1 );
	m_textEntrySizer->Add( m_borderStyleLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_borderStyleCombo = new wxBitmapComboBox( this, wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_borderStyleCombo->SetMinSize( wxSize( 240,-1 ) );

	m_textEntrySizer->Add( m_borderStyleCombo, wxGBPosition( 2, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_filledCtrl = new wxCheckBox( this, wxID_ANY, _("Filled shape"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textEntrySizer->Add( m_filledCtrl, wxGBPosition( 0, 5 ), wxGBSpan( 1, 2 ), wxRIGHT, 80 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );

	m_fillColorLabel = new wxStaticText( this, wxID_ANY, _("Fill color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fillColorLabel->Wrap( -1 );
	bSizer8->Add( m_fillColorLabel, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_panelFillColor = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer211;
	bSizer211 = new wxBoxSizer( wxVERTICAL );

	m_fillColorSwatch = new COLOR_SWATCH( m_panelFillColor, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer211->Add( m_fillColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panelFillColor->SetSizer( bSizer211 );
	m_panelFillColor->Layout();
	bSizer211->Fit( m_panelFillColor );
	bSizer8->Add( m_panelFillColor, 0, wxALIGN_CENTER_VERTICAL, 5 );


	m_textEntrySizer->Add( bSizer8, wxGBPosition( 1, 5 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );


	m_textEntrySizer->AddGrowableCol( 3 );

	mainSizer->Add( m_textEntrySizer, 0, wxEXPAND|wxALL, 10 );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );

	m_helpLabel1 = new wxStaticText( this, wxID_ANY, _("Set border width to 0 to use schematic's default line width."), wxDefaultPosition, wxDefaultSize, 0 );
	m_helpLabel1->Wrap( -1 );
	bSizer12->Add( m_helpLabel1, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 10 );

	m_helpLabel2 = new wxStaticText( this, wxID_ANY, _("Clear colors to use Schematic Editor colors."), wxDefaultPosition, wxDefaultSize, 0 );
	m_helpLabel2->Wrap( -1 );
	bSizer12->Add( m_helpLabel2, 0, wxBOTTOM|wxRIGHT|wxLEFT, 10 );


	mainSizer->Add( bSizer12, 0, wxBOTTOM|wxEXPAND, 5 );

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

DIALOG_SHAPE_PROPERTIES_BASE::~DIALOG_SHAPE_PROPERTIES_BASE()
{
}
