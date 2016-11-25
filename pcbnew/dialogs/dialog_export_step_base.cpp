///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_export_step_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXPORT_STEP_BASE::DIALOG_EXPORT_STEP_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerSTEPFile;
	bSizerSTEPFile = new wxBoxSizer( wxVERTICAL );
	
	m_txtBrdFile = new wxStaticText( this, wxID_ANY, _("STEP File name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_txtBrdFile->Wrap( -1 );
	bSizerSTEPFile->Add( m_txtBrdFile, 0, wxALL, 5 );
	
	m_filePickerSTEP = new wxFilePickerCtrl( this, wxID_ANY, wxEmptyString, _("Select a STEP export filename"), wxT("*.stp,*.step"), wxDefaultPosition, wxSize( 450,-1 ), wxFLP_OVERWRITE_PROMPT|wxFLP_SAVE|wxFLP_USE_TEXTCTRL );
	bSizerSTEPFile->Add( m_filePickerSTEP, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerSTEPFile->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText6 = new wxStaticText( this, wxID_ANY, _("STEP coordinates origin options:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	m_staticText6->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	bSizer7->Add( m_staticText6, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	fgSizer2->Add( 0, 0, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_cbDrillOrigin = new wxCheckBox( this, wxID_ANY, _("Drill and plot axis origin"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbDrillOrigin->SetToolTip( _("Use the auxiliary axis origin (used in plot and drill geneation) as STEP coordinates origin.") );
	
	fgSizer2->Add( m_cbDrillOrigin, 0, wxALL, 5 );
	
	
	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_cbAuxOrigin = new wxCheckBox( this, wxID_ANY, _("Grid origin"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbAuxOrigin->SetToolTip( _("Use the grid origin as STEP coordinates origin.") );
	
	fgSizer2->Add( m_cbAuxOrigin, 0, wxALL, 5 );
	
	
	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_cbUserOrigin = new wxCheckBox( this, wxID_ANY, _("User defined origin"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbUserOrigin->SetToolTip( _("Use this option if you want to define a specific coordinate origin value.") );
	
	fgSizer2->Add( m_cbUserOrigin, 0, wxALL, 5 );
	
	
	bSizer7->Add( fgSizer2, 1, wxEXPAND, 5 );
	
	
	bSizer2->Add( bSizer7, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("User defined origin:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	m_staticText2->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	bSizer3->Add( m_staticText2, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	fgSizer1->Add( m_staticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	wxString m_STEP_OrgUnitChoiceChoices[] = { _("mm"), _("inch") };
	int m_STEP_OrgUnitChoiceNChoices = sizeof( m_STEP_OrgUnitChoiceChoices ) / sizeof( wxString );
	m_STEP_OrgUnitChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_STEP_OrgUnitChoiceNChoices, m_STEP_OrgUnitChoiceChoices, 0 );
	m_STEP_OrgUnitChoice->SetSelection( 0 );
	fgSizer1->Add( m_STEP_OrgUnitChoice, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_staticText3 = new wxStaticText( this, wxID_ANY, _("X Position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	fgSizer1->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_STEP_Xorg = new wxTextCtrl( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_STEP_Xorg->HasFlag( wxTE_MULTILINE ) )
	{
	m_STEP_Xorg->SetMaxLength( 8 );
	}
	#else
	m_STEP_Xorg->SetMaxLength( 8 );
	#endif
	fgSizer1->Add( m_STEP_Xorg, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_staticText4 = new wxStaticText( this, wxID_ANY, _("Y Position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	fgSizer1->Add( m_staticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_STEP_Yorg = new wxTextCtrl( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_STEP_Yorg->HasFlag( wxTE_MULTILINE ) )
	{
	m_STEP_Yorg->SetMaxLength( 8 );
	}
	#else
	m_STEP_Yorg->SetMaxLength( 8 );
	#endif
	fgSizer1->Add( m_STEP_Yorg, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );
	
	
	bSizer3->Add( fgSizer1, 1, wxEXPAND, 5 );
	
	
	bSizer2->Add( bSizer3, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText7 = new wxStaticText( this, wxID_ANY, _("Other options:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	m_staticText7->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	bSizer8->Add( m_staticText7, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	fgSizer3->Add( 0, 0, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_cbRemoveVirtual = new wxCheckBox( this, wxID_ANY, _("Ignore Virtual Components"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_cbRemoveVirtual, 0, wxALL, 5 );
	
	
	bSizer8->Add( fgSizer3, 1, wxEXPAND, 5 );
	
	
	bSizer2->Add( bSizer8, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerSTEPFile->Add( bSizer2, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerSTEPFile->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerSTEPFile->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	
	this->SetSizer( bSizerSTEPFile );
	this->Layout();
	bSizerSTEPFile->Fit( this );
	
	this->Centre( wxBOTH );
}

DIALOG_EXPORT_STEP_BASE::~DIALOG_EXPORT_STEP_BASE()
{
}
