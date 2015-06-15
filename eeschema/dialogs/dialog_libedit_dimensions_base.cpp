///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_libedit_dimensions_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_LIBEDIT_DIMENSIONS_BASE, wxDialog )
	EVT_BUTTON( wxID_ANY, DIALOG_LIBEDIT_DIMENSIONS_BASE::_wxFB_OnSaveSetupClick )
	EVT_BUTTON( wxID_CANCEL, DIALOG_LIBEDIT_DIMENSIONS_BASE::_wxFB_OnCancelClick )
	EVT_BUTTON( wxID_OK, DIALOG_LIBEDIT_DIMENSIONS_BASE::_wxFB_OnOkClick )
END_EVENT_TABLE()

DIALOG_LIBEDIT_DIMENSIONS_BASE::DIALOG_LIBEDIT_DIMENSIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 7, 3, 0, 0 );
	fgSizer1->AddGrowableCol( 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->AddGrowableCol( 2 );
	fgSizer1->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText3 = new wxStaticText( this, wxID_ANY, _("&Grid size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	fgSizer1->Add( m_staticText3, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	wxString m_choiceGridSizeChoices[] = { _("50"), _("25"), _("10"), _("5"), _("2"), _("1") };
	int m_choiceGridSizeNChoices = sizeof( m_choiceGridSizeChoices ) / sizeof( wxString );
	m_choiceGridSize = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceGridSizeNChoices, m_choiceGridSizeChoices, 0 );
	m_choiceGridSize->SetSelection( 0 );
	fgSizer1->Add( m_choiceGridSize, 0, wxALL|wxEXPAND, 3 );
	
	m_staticGridUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticGridUnits->Wrap( -1 );
	fgSizer1->Add( m_staticGridUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, _("Current graphic &line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	fgSizer1->Add( m_staticText5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_CurrentGraphicLineThicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_CurrentGraphicLineThicknessCtrl->SetMaxLength( 0 ); 
	fgSizer1->Add( m_CurrentGraphicLineThicknessCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_staticLineWidthUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticLineWidthUnits->Wrap( -1 );
	fgSizer1->Add( m_staticLineWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticText7 = new wxStaticText( this, wxID_ANY, _("Current graphic text &size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	fgSizer1->Add( m_staticText7, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_CurrentGraphicTextSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_CurrentGraphicTextSizeCtrl->SetMaxLength( 0 ); 
	fgSizer1->Add( m_CurrentGraphicTextSizeCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_staticTextSizeUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeUnits->Wrap( -1 );
	fgSizer1->Add( m_staticTextSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Repeat draw item &horizontal displacement:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizer1->Add( m_staticText9, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	wxString m_choiceRepeatHorizontalChoices[] = { _("50"), _("25") };
	int m_choiceRepeatHorizontalNChoices = sizeof( m_choiceRepeatHorizontalChoices ) / sizeof( wxString );
	m_choiceRepeatHorizontal = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceRepeatHorizontalNChoices, m_choiceRepeatHorizontalChoices, 0 );
	m_choiceRepeatHorizontal->SetSelection( 0 );
	fgSizer1->Add( m_choiceRepeatHorizontal, 0, wxALL|wxEXPAND, 5 );
	
	m_staticRepeatXUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticRepeatXUnits->Wrap( -1 );
	fgSizer1->Add( m_staticRepeatXUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticText12 = new wxStaticText( this, wxID_ANY, _("Repeat draw item &vertical displacement:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	fgSizer1->Add( m_staticText12, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	wxString m_choiceRepeatVerticalChoices[] = { _("50"), _("25") };
	int m_choiceRepeatVerticalNChoices = sizeof( m_choiceRepeatVerticalChoices ) / sizeof( wxString );
	m_choiceRepeatVertical = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceRepeatVerticalNChoices, m_choiceRepeatVerticalChoices, 0 );
	m_choiceRepeatVertical->SetSelection( 0 );
	fgSizer1->Add( m_choiceRepeatVertical, 0, wxALL|wxEXPAND, 5 );
	
	m_staticRepeatYUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticRepeatYUnits->Wrap( -1 );
	fgSizer1->Add( m_staticRepeatYUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticText15 = new wxStaticText( this, wxID_ANY, _("Current &pin lenght:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	fgSizer1->Add( m_staticText15, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_CurrentPinLenghtCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_CurrentPinLenghtCtrl->SetMaxLength( 0 ); 
	fgSizer1->Add( m_CurrentPinLenghtCtrl, 0, wxEXPAND|wxALL, 5 );
	
	m_staticText161 = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText161->Wrap( -1 );
	fgSizer1->Add( m_staticText161, 0, wxALL, 5 );
	
	m_CurrentPinNameSizeText = new wxStaticText( this, wxID_ANY, _("Current pin name size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CurrentPinNameSizeText->Wrap( -1 );
	fgSizer1->Add( m_CurrentPinNameSizeText, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_CurrentPinNameSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_CurrentPinNameSizeCtrl->SetMaxLength( 0 ); 
	fgSizer1->Add( m_CurrentPinNameSizeCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText18 = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	fgSizer1->Add( m_staticText18, 0, wxALL, 5 );
	
	m_CurrentPinNumberSizeText = new wxStaticText( this, wxID_ANY, _("Current pin number size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CurrentPinNumberSizeText->Wrap( -1 );
	fgSizer1->Add( m_CurrentPinNumberSizeText, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_CurrentPinNumberSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_CurrentPinNumberSizeCtrl->SetMaxLength( 0 ); 
	fgSizer1->Add( m_CurrentPinNumberSizeCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText20 = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText20->Wrap( -1 );
	fgSizer1->Add( m_staticText20, 0, wxALL, 5 );
	
	m_staticText16 = new wxStaticText( this, wxID_ANY, _("&Repeat pin number increment:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	fgSizer1->Add( m_staticText16, 0, wxALL|wxALIGN_CENTER_VERTICAL, 3 );
	
	m_spinRepeatLabel = new wxSpinCtrl( this, wxID_ANY, wxT("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, 0, 10, 1 );
	fgSizer1->Add( m_spinRepeatLabel, 0, wxALL|wxEXPAND, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 3 );
	
	
	mainSizer->Add( fgSizer1, 0, wxEXPAND, 0 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bSizerBttons;
	bSizerBttons = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonSave = new wxButton( this, wxID_ANY, _("Save as Default"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBttons->Add( m_buttonSave, 0, wxALL, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bSizerBttons->Add( m_sdbSizer1, 0, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	
	
	mainSizer->Add( bSizerBttons, 0, wxALIGN_RIGHT, 5 );
	
	
	this->SetSizer( mainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
}

DIALOG_LIBEDIT_DIMENSIONS_BASE::~DIALOG_LIBEDIT_DIMENSIONS_BASE()
{
}
