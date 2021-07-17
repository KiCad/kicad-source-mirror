///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/color_swatch.h"

#include "dialog_shape_properties_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_SHAPE_PROPERTIES_BASE, DIALOG_SHIM )
	EVT_BUTTON( wxID_APPLY, DIALOG_SHAPE_PROPERTIES_BASE::_wxFB_resetDefaults )
END_EVENT_TABLE()

DIALOG_SHAPE_PROPERTIES_BASE::DIALOG_SHAPE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizerGeneral;
	fgSizerGeneral = new wxFlexGridSizer( 0, 3, 7, 0 );
	fgSizerGeneral->AddGrowableCol( 1 );
	fgSizerGeneral->SetFlexibleDirection( wxBOTH );
	fgSizerGeneral->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lineWidthLabel = new wxStaticText( this, wxID_ANY, _("Line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthLabel->Wrap( -1 );
	fgSizerGeneral->Add( m_lineWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 3 );

	m_lineWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	fgSizerGeneral->Add( m_lineWidthCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 3 );

	m_lineWidthUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthUnits->Wrap( -1 );
	m_lineWidthUnits->SetMinSize( wxSize( 40,-1 ) );

	fgSizerGeneral->Add( m_lineWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 3 );

	m_lineColorLabel = new wxStaticText( this, wxID_ANY, _("Line color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineColorLabel->Wrap( -1 );
	fgSizerGeneral->Add( m_lineColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_panel1 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	m_lineColorSwatch = new COLOR_SWATCH( m_panel1, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_lineColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panel1->SetSizer( bSizer2 );
	m_panel1->Layout();
	bSizer2->Fit( m_panel1 );
	fgSizerGeneral->Add( m_panel1, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 2 );


	fgSizerGeneral->Add( 0, 0, 1, wxEXPAND, 5 );

	m_lineStyleLabel = new wxStaticText( this, wxID_ANY, _("Line style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineStyleLabel->Wrap( -1 );
	fgSizerGeneral->Add( m_lineStyleLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_lineStyleCombo = new wxBitmapComboBox( this, wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_lineStyleCombo->SetMinSize( wxSize( 240,-1 ) );

	fgSizerGeneral->Add( m_lineStyleCombo, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 3 );


	fgSizerGeneral->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizerGeneral->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	fgSizerGeneral->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizerGeneral->Add( 0, 0, 1, wxEXPAND, 5 );

	m_filledCtrl = new wxCheckBox( this, wxID_ANY, _("Filled shape"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerGeneral->Add( m_filledCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerGeneral->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizerGeneral->Add( 0, 0, 1, wxEXPAND, 5 );

	m_fillColorLabel = new wxStaticText( this, wxID_ANY, _("Fill color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fillColorLabel->Wrap( -1 );
	fgSizerGeneral->Add( m_fillColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_panel11 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxVERTICAL );

	m_fillColorSwatch = new COLOR_SWATCH( m_panel11, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer21->Add( m_fillColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panel11->SetSizer( bSizer21 );
	m_panel11->Layout();
	bSizer21->Fit( m_panel11 );
	fgSizerGeneral->Add( m_panel11, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	mainSizer->Add( fgSizerGeneral, 1, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 10 );

	m_helpLabel1 = new wxStaticText( this, wxID_ANY, _("Set line width to 0 to use Schematic Editor line widths."), wxDefaultPosition, wxDefaultSize, 0 );
	m_helpLabel1->Wrap( 333 );
	mainSizer->Add( m_helpLabel1, 0, wxTOP|wxRIGHT|wxLEFT, 10 );

	m_helpLabel2 = new wxStaticText( this, wxID_ANY, _("Set line color to transparent to use Schematic Editor colors."), wxDefaultPosition, wxDefaultSize, 0 );
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

DIALOG_SHAPE_PROPERTIES_BASE::~DIALOG_SHAPE_PROPERTIES_BASE()
{
}
