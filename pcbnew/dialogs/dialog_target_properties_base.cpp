///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct  8 2012)
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
	fgSizer->Add( m_staticTextSize, 0, wxALL, 5 );
	
	m_MireWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_MireWidthCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_staticTextSizeUnits = new wxStaticText( this, wxID_ANY, wxT("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeUnits->Wrap( -1 );
	fgSizer->Add( m_staticTextSizeUnits, 0, wxALL, 5 );
	
	m_staticTextThickness = new wxStaticText( this, wxID_ANY, wxT("Thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextThickness->Wrap( -1 );
	fgSizer->Add( m_staticTextThickness, 0, wxALL, 5 );
	
	m_MireSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_MireSizeCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_staticTextThicknessUnits = new wxStaticText( this, wxID_ANY, wxT("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextThicknessUnits->Wrap( -1 );
	fgSizer->Add( m_staticTextThicknessUnits, 0, wxALL, 5 );
	
	m_staticTextShape = new wxStaticText( this, wxID_ANY, wxT("Shape"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextShape->Wrap( -1 );
	fgSizer->Add( m_staticTextShape, 0, wxALL, 5 );
	
	wxString m_MireShapeChoices[] = { wxT("+"), wxT("X") };
	int m_MireShapeNChoices = sizeof( m_MireShapeChoices ) / sizeof( wxString );
	m_MireShape = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_MireShapeNChoices, m_MireShapeChoices, 0 );
	m_MireShape->SetSelection( 0 );
	fgSizer->Add( m_MireShape, 0, wxALL|wxEXPAND, 5 );
	
	
	fgSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizerUpper->Add( fgSizer, 1, wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizerUpper, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
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
