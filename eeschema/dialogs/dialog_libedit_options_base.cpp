///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_libedit_options_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LIBEDIT_OPTIONS_BASE::DIALOG_LIBEDIT_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bOptionsSizer;
	bOptionsSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* p1mainSizer;
	p1mainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer;
	fgSizer = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer->AddGrowableCol( 0 );
	fgSizer->AddGrowableCol( 1 );
	fgSizer->AddGrowableCol( 2 );
	fgSizer->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText3 = new wxStaticText( this, wxID_ANY, _("&Grid size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	fgSizer->Add( m_staticText3, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	wxArrayString m_choiceGridSizeChoices;
	m_choiceGridSize = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceGridSizeChoices, 0 );
	m_choiceGridSize->SetSelection( 0 );
	fgSizer->Add( m_choiceGridSize, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxALL, 3 );
	
	m_staticGridUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticGridUnits->Wrap( -1 );
	fgSizer->Add( m_staticGridUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, _("Default &line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	fgSizer->Add( m_staticText5, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_spinLineWidth = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, 1, 100, 6 );
	fgSizer->Add( m_spinLineWidth, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	m_staticLineWidthUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticLineWidthUnits->Wrap( -1 );
	fgSizer->Add( m_staticLineWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_staticText52 = new wxStaticText( this, wxID_ANY, _("Default pin length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText52->Wrap( -1 );
	fgSizer->Add( m_staticText52, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 3 );
	
	m_spinPinLength = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, 50, 1000, 200 );
	fgSizer->Add( m_spinPinLength, 0, wxALIGN_CENTER|wxALL|wxEXPAND, 3 );
	
	m_staticPinLengthUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticPinLengthUnits->Wrap( -1 );
	fgSizer->Add( m_staticPinLengthUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 3 );
	
	m_staticText7 = new wxStaticText( this, wxID_ANY, _("Default pin num &size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	fgSizer->Add( m_staticText7, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_spinPinNumSize = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, 0, 1000, 0 );
	fgSizer->Add( m_spinPinNumSize, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	m_staticTextSizeUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeUnits->Wrap( -1 );
	fgSizer->Add( m_staticTextSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Default pin &name size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizer->Add( m_staticText9, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_spinPinNameSize = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, -5000, 5000, 0 );
	fgSizer->Add( m_spinPinNameSize, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	m_staticRepeatXUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticRepeatXUnits->Wrap( -1 );
	fgSizer->Add( m_staticRepeatXUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_staticText11 = new wxStaticText( this, wxID_ANY, _("Repeat draw item &horizontal displacement:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	fgSizer->Add( m_staticText11, 0, wxALL, 5 );
	
	m_spinRepeatHorizontal = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -1000, 1000, 0 );
	fgSizer->Add( m_spinRepeatHorizontal, 0, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText12 = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	fgSizer->Add( m_staticText12, 0, wxALL, 5 );
	
	m_staticText13 = new wxStaticText( this, wxID_ANY, _("Repeat draw item &vertical displacement:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText13->Wrap( -1 );
	fgSizer->Add( m_staticText13, 0, wxALL, 5 );
	
	m_spinRepeatVertical = new wxSpinCtrl( this, wxID_ANY, wxT("100"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -1000, 1000, 0 );
	fgSizer->Add( m_spinRepeatVertical, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText14 = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	fgSizer->Add( m_staticText14, 0, wxALL, 5 );
	
	m_staticText15 = new wxStaticText( this, wxID_ANY, _("Repeat pin displacement"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	fgSizer->Add( m_staticText15, 0, wxALL, 5 );
	
	wxString m_choicePinDisplacementChoices[] = { _("100"), _("50") };
	int m_choicePinDisplacementNChoices = sizeof( m_choicePinDisplacementChoices ) / sizeof( wxString );
	m_choicePinDisplacement = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choicePinDisplacementNChoices, m_choicePinDisplacementChoices, 0 );
	m_choicePinDisplacement->SetSelection( 0 );
	fgSizer->Add( m_choicePinDisplacement, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText16 = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	fgSizer->Add( m_staticText16, 0, wxALL, 5 );
	
	m_staticText17 = new wxStaticText( this, wxID_ANY, _("&Repeat label increment:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText17->Wrap( -1 );
	fgSizer->Add( m_staticText17, 0, wxALL, 5 );
	
	m_spinRepeatLabel = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10, 10, 1 );
	fgSizer->Add( m_spinRepeatLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	fgSizer->Add( 0, 0, 0, 0, 5 );
	
	
	bSizer3->Add( fgSizer, 0, wxALIGN_CENTER|wxEXPAND, 0 );
	
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );
	
	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer2->Add( m_staticline3, 0, wxEXPAND | wxALL, 5 );
	
	m_checkShowGrid = new wxCheckBox( this, wxID_ANY, _("Show gr&id"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_checkShowGrid, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 3 );
	
	
	bSizer3->Add( bSizer2, 0, wxEXPAND, 0 );
	
	
	p1mainSizer->Add( bSizer3, 1, wxALL|wxEXPAND, 6 );
	
	
	bOptionsSizer->Add( p1mainSizer, 1, wxEXPAND, 5 );
	
	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bOptionsSizer->Add( m_staticline2, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bOptionsSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 6 );
	
	
	mainSizer->Add( bOptionsSizer, 1, wxEXPAND, 12 );
	
	
	this->SetSizer( mainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
}

DIALOG_LIBEDIT_OPTIONS_BASE::~DIALOG_LIBEDIT_OPTIONS_BASE()
{
}
