///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov  6 2013)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_target_properties_base.h"

///////////////////////////////////////////////////////////////////////////

TARGET_PROPERTIES_DIALOG_EDITOR_BASE::TARGET_PROPERTIES_DIALOG_EDITOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer;
	fgSizer = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer->AddGrowableCol( 1 );
	fgSizer->SetFlexibleDirection( wxBOTH );
	fgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextSize = new wxStaticText( this, wxID_ANY, wxT("Size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSize->Wrap( -1 );
	fgSizer->Add( m_staticTextSize, 0, wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TargetSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_TargetSizeCtrl, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextSizeUnits = new wxStaticText( this, wxID_ANY, wxT("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeUnits->Wrap( -1 );
	fgSizer->Add( m_staticTextSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_staticTextThickness = new wxStaticText( this, wxID_ANY, wxT("Thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextThickness->Wrap( -1 );
	fgSizer->Add( m_staticTextThickness, 0, wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TargetThicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_TargetThicknessCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_staticTextThicknessUnits = new wxStaticText( this, wxID_ANY, wxT("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextThicknessUnits->Wrap( -1 );
	fgSizer->Add( m_staticTextThicknessUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_staticTextShape = new wxStaticText( this, wxID_ANY, wxT("Shape"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextShape->Wrap( -1 );
	fgSizer->Add( m_staticTextShape, 0, wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_TargetShapeChoices[] = { wxT("+"), wxT("X") };
	int m_TargetShapeNChoices = sizeof( m_TargetShapeChoices ) / sizeof( wxString );
	m_TargetShape = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_TargetShapeNChoices, m_TargetShapeChoices, 0 );
	m_TargetShape->SetSelection( 0 );
	fgSizer->Add( m_TargetShape, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizer->Add( 0, 0, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerUpper->Add( fgSizer, 1, wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizerUpper, 1, wxEXPAND, 5 );
	
	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizerButts = new wxStdDialogButtonSizer();
	m_sdbSizerButtsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButts->AddButton( m_sdbSizerButtsOK );
	m_sdbSizerButtsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButts->AddButton( m_sdbSizerButtsCancel );
	m_sdbSizerButts->Realize();
	
	bSizerMain->Add( m_sdbSizerButts, 0, wxALIGN_RIGHT|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_sdbSizerButtsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TARGET_PROPERTIES_DIALOG_EDITOR_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerButtsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TARGET_PROPERTIES_DIALOG_EDITOR_BASE::OnOkClick ), NULL, this );
}

TARGET_PROPERTIES_DIALOG_EDITOR_BASE::~TARGET_PROPERTIES_DIALOG_EDITOR_BASE()
{
	// Disconnect Events
	m_sdbSizerButtsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TARGET_PROPERTIES_DIALOG_EDITOR_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerButtsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TARGET_PROPERTIES_DIALOG_EDITOR_BASE::OnOkClick ), NULL, this );
	
}
