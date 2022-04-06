///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
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

	wxBoxSizer* dlgBorderSizer;
	dlgBorderSizer = new wxBoxSizer( wxVERTICAL );

	m_checkBorder = new wxCheckBox( this, wxID_ANY, _("Border"), wxDefaultPosition, wxDefaultSize, 0 );
	dlgBorderSizer->Add( m_checkBorder, 0, wxTOP|wxRIGHT|wxLEFT, 3 );

	wxBoxSizer* bSizerLineWidth;
	bSizerLineWidth = new wxBoxSizer( wxHORIZONTAL );

	m_widthLabel = new wxStaticText( this, wxID_ANY, _("Border width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_widthLabel->Wrap( -1 );
	bSizerLineWidth->Add( m_widthLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );

	m_widthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLineWidth->Add( m_widthCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );

	m_widthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_widthUnits->Wrap( -1 );
	bSizerLineWidth->Add( m_widthUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 3 );


	dlgBorderSizer->Add( bSizerLineWidth, 0, wxEXPAND, 5 );

	m_helpLabel = new wxStaticText( this, wxID_ANY, _("Set border width to 0 to use schematic's default symbol line width."), wxDefaultPosition, wxDefaultSize, 0 );
	m_helpLabel->Wrap( 320 );
	dlgBorderSizer->Add( m_helpLabel, 0, wxALL, 3 );


	dlgBorderSizer->Add( 0, 3, 0, 0, 5 );

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

	m_colorSwatch = new COLOR_SWATCH( bSizerFill->GetStaticBox(), FILLED_WITH_COLOR, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_colorSwatch, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizerFill->Add( gbSizer1, 1, wxEXPAND|wxBOTTOM, 5 );


	dlgBorderSizer->Add( bSizerFill, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	dlgBorderSizer->Add( 0, 3, 0, 0, 5 );

	m_privateCheckbox = new wxCheckBox( this, wxID_ANY, _("Private to Symbol Editor"), wxDefaultPosition, wxDefaultSize, 0 );
	dlgBorderSizer->Add( m_privateCheckbox, 0, wxALL, 3 );


	dlgBorderSizer->Add( 0, 8, 1, wxEXPAND, 5 );

	m_checkApplyToAllUnits = new wxCheckBox( this, wxID_ANY, _("Common to all &units in symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	dlgBorderSizer->Add( m_checkApplyToAllUnits, 0, wxALL, 3 );

	m_checkApplyToAllConversions = new wxCheckBox( this, wxID_ANY, _("Common to all body &styles (De Morgan)"), wxDefaultPosition, wxDefaultSize, 0 );
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
