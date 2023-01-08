///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/color_swatch.h"

#include "dialog_lib_shape_properties_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_LIB_SHAPE_PROPERTIES_BASE, DIALOG_SHIM )
	EVT_CHECKBOX( wxID_ANY, DIALOG_LIB_SHAPE_PROPERTIES_BASE::_wxFB_onBorderChecked )
	EVT_RADIOBUTTON( NO_FILL, DIALOG_LIB_SHAPE_PROPERTIES_BASE::_wxFB_onFill )
	EVT_RADIOBUTTON( FILLED_SHAPE, DIALOG_LIB_SHAPE_PROPERTIES_BASE::_wxFB_onFill )
	EVT_RADIOBUTTON( FILLED_WITH_BG_BODYCOLOR, DIALOG_LIB_SHAPE_PROPERTIES_BASE::_wxFB_onFill )
	EVT_RADIOBUTTON( FILLED_WITH_COLOR, DIALOG_LIB_SHAPE_PROPERTIES_BASE::_wxFB_onFill )
END_EVENT_TABLE()

DIALOG_LIB_SHAPE_PROPERTIES_BASE::DIALOG_LIB_SHAPE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bTop;
	bTop = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftCol;
	bLeftCol = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer2;
	gbSizer2 = new wxGridBagSizer( 3, 0 );
	gbSizer2->SetFlexibleDirection( wxBOTH );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_checkBorder = new wxCheckBox( this, wxID_ANY, _("Border"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_checkBorder, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), wxTOP|wxRIGHT|wxLEFT, 5 );

	m_borderWidthLabel = new wxStaticText( this, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthLabel->Wrap( -1 );
	gbSizer2->Add( m_borderWidthLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_borderWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer7->Add( m_borderWidthCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_borderWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthUnits->Wrap( -1 );
	bSizer7->Add( m_borderWidthUnits, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 3 );

	m_borderColorLabel = new wxStaticText( this, wxID_ANY, _("Color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderColorLabel->Wrap( -1 );
	bSizer7->Add( m_borderColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );


	bSizer7->Add( 5, 0, 0, 0, 5 );

	m_panelBorderColor = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer22;
	bSizer22 = new wxBoxSizer( wxVERTICAL );

	m_borderColorSwatch = new COLOR_SWATCH( m_panelBorderColor, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer22->Add( m_borderColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panelBorderColor->SetSizer( bSizer22 );
	m_panelBorderColor->Layout();
	bSizer22->Fit( m_panelBorderColor );
	bSizer7->Add( m_panelBorderColor, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	gbSizer2->Add( bSizer7, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_borderStyleLabel = new wxStaticText( this, wxID_ANY, _("Style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderStyleLabel->Wrap( -1 );
	gbSizer2->Add( m_borderStyleLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_borderStyleCombo = new wxBitmapComboBox( this, wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_borderStyleCombo->SetMinSize( wxSize( 240,-1 ) );

	gbSizer2->Add( m_borderStyleCombo, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_helpLabel = new wxStaticText( this, wxID_ANY, _("Set border width to 0 to use schematic's default symbol line width."), wxDefaultPosition, wxDefaultSize, 0 );
	m_helpLabel->Wrap( 320 );
	gbSizer2->Add( m_helpLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 2 ), wxTOP|wxRIGHT|wxLEFT, 5 );


	bLeftCol->Add( gbSizer2, 1, wxEXPAND, 5 );


	bTop->Add( bLeftCol, 1, wxEXPAND|wxRIGHT, 30 );

	wxBoxSizer* bRightCol;
	bRightCol = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* bSizerFill;
	bSizerFill = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Fill Style") ), wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 3, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_rbFillNone = new wxRadioButton( bSizerFill->GetStaticBox(), NO_FILL, _("Do not fill"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	gbSizer1->Add( m_rbFillNone, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_rbFillOutline = new wxRadioButton( bSizerFill->GetStaticBox(), FILLED_SHAPE, _("Fill with body outline color"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_rbFillOutline, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_rbFillBackground = new wxRadioButton( bSizerFill->GetStaticBox(), FILLED_WITH_BG_BODYCOLOR, _("Fill with body background color"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_rbFillBackground, wxGBPosition( 2, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_rbFillCustom = new wxRadioButton( bSizerFill->GetStaticBox(), FILLED_WITH_COLOR, _("Fill with:"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_rbFillCustom, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_fillColorSwatch = new COLOR_SWATCH( bSizerFill->GetStaticBox(), FILLED_WITH_COLOR, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_fillColorSwatch, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizerFill->Add( gbSizer1, 1, wxEXPAND|wxBOTTOM, 5 );


	bRightCol->Add( bSizerFill, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	bTop->Add( bRightCol, 1, wxEXPAND|wxLEFT, 30 );


	mainSizer->Add( bTop, 1, wxALL|wxEXPAND, 10 );

	wxGridSizer* bBottom;
	bBottom = new wxGridSizer( 0, 2, 0, 60 );

	m_privateCheckbox = new wxCheckBox( this, wxID_ANY, _("Private to Symbol Editor"), wxDefaultPosition, wxDefaultSize, 0 );
	bBottom->Add( m_privateCheckbox, 0, wxALL, 5 );

	m_checkApplyToAllUnits = new wxCheckBox( this, wxID_ANY, _("Common to all &units in symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	bBottom->Add( m_checkApplyToAllUnits, 0, wxALL, 5 );


	bBottom->Add( 0, 0, 1, wxEXPAND, 5 );

	m_checkApplyToAllConversions = new wxCheckBox( this, wxID_ANY, _("Common to all body &styles (De Morgan)"), wxDefaultPosition, wxDefaultSize, 0 );
	bBottom->Add( m_checkApplyToAllConversions, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	mainSizer->Add( bBottom, 0, wxEXPAND|wxALL, 10 );

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
