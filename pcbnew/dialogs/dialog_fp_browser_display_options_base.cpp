///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_fp_browser_display_options_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FP_BROWSER_DISPLAY_OPTIONS_BASE::DIALOG_FP_BROWSER_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbSizerDrawMode;
	sbSizerDrawMode = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Drawing Options") ), wxVERTICAL );
	
	m_EdgesDisplayOption = new wxCheckBox( sbSizerDrawMode->GetStaticBox(), wxID_ANY, _("Graphic items sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerDrawMode->Add( m_EdgesDisplayOption, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_TextDisplayOption = new wxCheckBox( sbSizerDrawMode->GetStaticBox(), wxID_ANY, _("Texts sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerDrawMode->Add( m_TextDisplayOption, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_ShowPadSketch = new wxCheckBox( sbSizerDrawMode->GetStaticBox(), ID_PADFILL_OPT, _("Pad sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerDrawMode->Add( m_ShowPadSketch, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	
	sbSizerDrawMode->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_ShowPadNum = new wxCheckBox( sbSizerDrawMode->GetStaticBox(), wxID_ANY, _("Show pad &numbers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerDrawMode->Add( m_ShowPadNum, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bUpperSizer->Add( sbSizerDrawMode, 1, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* sbSizerViewOpt;
	sbSizerViewOpt = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Auto-zoom") ), wxVERTICAL );
	
	m_autoZoomOption = new wxCheckBox( sbSizerViewOpt->GetStaticBox(), wxID_ANY, _("Zoom to fit when changing footprint"), wxDefaultPosition, wxDefaultSize, 0 );
	m_autoZoomOption->SetValue(true); 
	sbSizerViewOpt->Add( m_autoZoomOption, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bUpperSizer->Add( sbSizerViewOpt, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizerMain->Add( bUpperSizer, 1, wxEXPAND|wxALL, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerApply = new wxButton( this, wxID_APPLY );
	m_sdbSizer->AddButton( m_sdbSizerApply );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerMain->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
	// Connect Events
	m_sdbSizerApply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_BROWSER_DISPLAY_OPTIONS_BASE::OnApplyClick ), NULL, this );
}

DIALOG_FP_BROWSER_DISPLAY_OPTIONS_BASE::~DIALOG_FP_BROWSER_DISPLAY_OPTIONS_BASE()
{
	// Disconnect Events
	m_sdbSizerApply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_BROWSER_DISPLAY_OPTIONS_BASE::OnApplyClick ), NULL, this );
	
}
