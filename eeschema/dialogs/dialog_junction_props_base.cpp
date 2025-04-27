///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/color_swatch.h"

#include "dialog_junction_props_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_JUNCTION_PROPS_BASE::DIALOG_JUNCTION_PROPS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 3, 5, 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextDiameter = new wxStaticText( this, wxID_ANY, _("Diameter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDiameter->Wrap( -1 );
	fgSizer2->Add( m_staticTextDiameter, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_textCtrlDiameter = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_textCtrlDiameter, 0, wxEXPAND, 5 );

	m_staticTextDiameterUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDiameterUnits->Wrap( -1 );
	fgSizer2->Add( m_staticTextDiameterUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_staticTextColor = new wxStaticText( this, wxID_ANY, _("Color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextColor->Wrap( -1 );
	fgSizer2->Add( m_staticTextColor, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_panel1 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxVERTICAL );

	m_colorSwatch = new COLOR_SWATCH( m_panel1, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer21->Add( m_colorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panel1->SetSizer( bSizer21 );
	m_panel1->Layout();
	bSizer21->Fit( m_panel1 );
	fgSizer2->Add( m_panel1, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizer2->Add( fgSizer2, 1, wxALL|wxEXPAND, 10 );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	m_helpLabel1 = new wxStaticText( this, wxID_ANY, _("Set diameter to 0 to use schematic's junction dot size."), wxDefaultPosition, wxDefaultSize, 0 );
	m_helpLabel1->Wrap( 333 );
	bMargins->Add( m_helpLabel1, 0, wxBOTTOM|wxRIGHT, 5 );

	m_helpLabel2 = new wxStaticText( this, wxID_ANY, _("Clear color to use Schematic Editor colors."), wxDefaultPosition, wxDefaultSize, 0 );
	m_helpLabel2->Wrap( -1 );
	bMargins->Add( m_helpLabel2, 0, wxBOTTOM|wxRIGHT, 5 );


	bSizer2->Add( bMargins, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerApply = new wxButton( this, wxID_APPLY );
	m_sdbSizer->AddButton( m_sdbSizerApply );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizer2->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	this->SetSizer( bSizer2 );
	this->Layout();
	bSizer2->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_sdbSizerApply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_JUNCTION_PROPS_BASE::resetDefaults ), NULL, this );
}

DIALOG_JUNCTION_PROPS_BASE::~DIALOG_JUNCTION_PROPS_BASE()
{
	// Disconnect Events
	m_sdbSizerApply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_JUNCTION_PROPS_BASE::resetDefaults ), NULL, this );

}
