///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_eeschema_options_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_EESCHEMA_OPTIONS_BASE, wxDialog )
	EVT_CHOICE( wxID_ANY, DIALOG_EESCHEMA_OPTIONS_BASE::_wxFB_OnChooseUnits )
END_EVENT_TABLE()

DIALOG_EESCHEMA_OPTIONS_BASE::DIALOG_EESCHEMA_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 7, 3, 0, 0 );
	fgSizer1->AddGrowableCol( 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->AddGrowableCol( 2 );
	fgSizer1->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Mesurement &units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizer1->Add( m_staticText2, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	wxArrayString m_choiceUnitsChoices;
	m_choiceUnits = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnitsChoices, 0 );
	m_choiceUnits->SetSelection( 0 );
	fgSizer1->Add( m_choiceUnits, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 3 );
	
	m_staticText3 = new wxStaticText( this, wxID_ANY, _("&Grid size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	fgSizer1->Add( m_staticText3, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	wxArrayString m_choiceGridSizeChoices;
	m_choiceGridSize = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceGridSizeChoices, 0 );
	m_choiceGridSize->SetSelection( 0 );
	fgSizer1->Add( m_choiceGridSize, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	m_staticGridUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticGridUnits->Wrap( -1 );
	fgSizer1->Add( m_staticGridUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, _("Default &line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	fgSizer1->Add( m_staticText5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_spinLineWidth = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, 0, 100, 0 );
	fgSizer1->Add( m_spinLineWidth, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	m_staticLineWidthUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticLineWidthUnits->Wrap( -1 );
	fgSizer1->Add( m_staticLineWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticText7 = new wxStaticText( this, wxID_ANY, _("Default text &size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	fgSizer1->Add( m_staticText7, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_spinTextSize = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, 0, 1000, 0 );
	fgSizer1->Add( m_spinTextSize, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	m_staticTextSizeUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeUnits->Wrap( -1 );
	fgSizer1->Add( m_staticTextSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Repeat draw item &horizontal displacement:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizer1->Add( m_staticText9, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_spinRepeatHorizontal = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, 1, 2000, 1 );
	fgSizer1->Add( m_spinRepeatHorizontal, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	m_staticRepeatXUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticRepeatXUnits->Wrap( -1 );
	fgSizer1->Add( m_staticRepeatXUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticText12 = new wxStaticText( this, wxID_ANY, _("Repeat draw item &vertical displacement:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	fgSizer1->Add( m_staticText12, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_spinRepeatVertical = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, 1, 2000, 1 );
	fgSizer1->Add( m_spinRepeatVertical, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	m_staticRepeatYUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticRepeatYUnits->Wrap( -1 );
	fgSizer1->Add( m_staticRepeatYUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticText16 = new wxStaticText( this, wxID_ANY, _("&Repeat label increment:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	fgSizer1->Add( m_staticText16, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_spinRepeatLabel = new wxSpinCtrl( this, wxID_ANY, wxT("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, -100, 100, 1 );
	fgSizer1->Add( m_spinRepeatLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 3 );
	
	bSizer3->Add( fgSizer1, 0, wxALIGN_CENTER|wxEXPAND, 0 );
	
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );
	
	m_checkShowGrid = new wxCheckBox( this, wxID_ANY, _("Show g&rid"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bSizer2->Add( m_checkShowGrid, 0, wxALL|wxEXPAND, 3 );
	
	m_checkShowHiddenPins = new wxCheckBox( this, wxID_ANY, _("Show hi&dden pins"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bSizer2->Add( m_checkShowHiddenPins, 0, wxALL|wxEXPAND, 3 );
	
	m_checkAutoPan = new wxCheckBox( this, wxID_ANY, _("Enable automatic &panning"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bSizer2->Add( m_checkAutoPan, 0, wxALL|wxEXPAND, 3 );
	
	m_checkAnyOrientation = new wxCheckBox( this, wxID_ANY, _("Allow buses and wires to be placed in any &orientation"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bSizer2->Add( m_checkAnyOrientation, 0, wxALL|wxEXPAND, 3 );
	
	m_checkPageLimits = new wxCheckBox( this, wxID_ANY, _("Show p&age limits"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bSizer2->Add( m_checkPageLimits, 0, wxALL|wxEXPAND, 3 );
	
	bSizer3->Add( bSizer2, 0, wxEXPAND, 0 );
	
	
	bSizer3->Add( 0, 0, 1, wxALL|wxEXPAND, 10 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	bSizer3->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 0 );
	
	mainSizer->Add( bSizer3, 1, wxALL|wxEXPAND, 12 );
	
	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
	
	this->Centre( wxBOTH );
}

DIALOG_EESCHEMA_OPTIONS_BASE::~DIALOG_EESCHEMA_OPTIONS_BASE()
{
}
