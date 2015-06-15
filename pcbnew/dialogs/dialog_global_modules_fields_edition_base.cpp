///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_global_modules_fields_edition_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GLOBAL_MODULES_FIELDS_EDITION_BASE::DIALOG_GLOBAL_MODULES_FIELDS_EDITION_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Footprint Fields") ), wxVERTICAL );
	
	m_ReferenceOpt = new wxCheckBox( this, wxID_ANY, _("Reference designator"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_ReferenceOpt, 0, wxALL|wxEXPAND, 5 );
	
	m_ValueOpt = new wxCheckBox( this, wxID_ANY, _("Value"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_ValueOpt, 0, wxALL|wxEXPAND, 5 );
	
	m_OtherFields = new wxCheckBox( this, wxID_ANY, _("User defined"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_OtherFields, 0, wxALL|wxEXPAND, 5 );
	
	
	bLeftSizer->Add( sbSizer1, 1, wxBOTTOM|wxEXPAND|wxRIGHT, 5 );
	
	m_staticTextFilter = new wxStaticText( this, wxID_ANY, _("Footprint Filter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFilter->Wrap( -1 );
	m_staticTextFilter->SetToolTip( _("A string to filter footprints to edit.\nIf not void, footprint names should match this filter.\nA filter can be something like SM* (case insensitive)") );
	
	bLeftSizer->Add( m_staticTextFilter, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ModuleFilter = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ModuleFilter->SetMaxLength( 0 ); 
	bLeftSizer->Add( m_ModuleFilter, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	
	bSizerUpper->Add( bLeftSizer, 2, wxEXPAND, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizerSettings;
	sbSizerSettings = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Current Text Dimensions") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizerCurrSettings;
	fgSizerCurrSettings = new wxFlexGridSizer( 3, 3, 0, 0 );
	fgSizerCurrSettings->AddGrowableCol( 1 );
	fgSizerCurrSettings->SetFlexibleDirection( wxBOTH );
	fgSizerCurrSettings->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	fgSizerCurrSettings->Add( m_staticText3, 0, wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_SizeX_Value = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeX_Value->SetMaxLength( 0 ); 
	fgSizerCurrSettings->Add( m_SizeX_Value, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_SizeXunit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXunit->Wrap( -1 );
	fgSizerCurrSettings->Add( m_SizeXunit, 0, wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText6 = new wxStaticText( this, wxID_ANY, _("Height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	fgSizerCurrSettings->Add( m_staticText6, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT|wxALIGN_RIGHT, 5 );
	
	m_SizeY_Value = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeY_Value->SetMaxLength( 0 ); 
	fgSizerCurrSettings->Add( m_SizeY_Value, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_SizeYunit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeYunit->Wrap( -1 );
	fgSizerCurrSettings->Add( m_SizeYunit, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizerCurrSettings->Add( m_staticText9, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_TicknessValue = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TicknessValue->SetMaxLength( 0 ); 
	fgSizerCurrSettings->Add( m_TicknessValue, 0, wxALL|wxEXPAND, 5 );
	
	m_Ticknessunit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Ticknessunit->Wrap( -1 );
	fgSizerCurrSettings->Add( m_Ticknessunit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	sbSizerSettings->Add( fgSizerCurrSettings, 1, wxEXPAND, 5 );
	
	
	bRightSizer->Add( sbSizerSettings, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	
	bSizerUpper->Add( bRightSizer, 3, wxEXPAND, 5 );
	
	
	bMainSizer->Add( bSizerUpper, 1, wxALL|wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_sdbSizerButtons = new wxStdDialogButtonSizer();
	m_sdbSizerButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsOK );
	m_sdbSizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsCancel );
	m_sdbSizerButtons->Realize();
	
	bMainSizer->Add( m_sdbSizerButtons, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	// Connect Events
	m_sdbSizerButtonsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_MODULES_FIELDS_EDITION_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_MODULES_FIELDS_EDITION_BASE::OnOKClick ), NULL, this );
}

DIALOG_GLOBAL_MODULES_FIELDS_EDITION_BASE::~DIALOG_GLOBAL_MODULES_FIELDS_EDITION_BASE()
{
	// Disconnect Events
	m_sdbSizerButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_MODULES_FIELDS_EDITION_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_MODULES_FIELDS_EDITION_BASE::OnOKClick ), NULL, this );
	
}
