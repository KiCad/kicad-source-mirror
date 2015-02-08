///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_display_options_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbSizerDrawMode;
	sbSizerDrawMode = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Draw options") ), wxVERTICAL );
	
	m_EdgesDisplayOption = new wxCheckBox( this, wxID_ANY, _("Graphic items sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerDrawMode->Add( m_EdgesDisplayOption, 0, wxALL, 5 );
	
	m_TextDisplayOption = new wxCheckBox( this, wxID_ANY, _("Texts sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerDrawMode->Add( m_TextDisplayOption, 0, wxALL, 5 );
	
	m_ShowPadSketch = new wxCheckBox( this, ID_PADFILL_OPT, _("Pad sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerDrawMode->Add( m_ShowPadSketch, 0, wxEXPAND|wxALL, 5 );
	
	m_ShowPadNum = new wxCheckBox( this, wxID_ANY, _("Show pad &number"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerDrawMode->Add( m_ShowPadNum, 0, wxALL|wxEXPAND, 5 );
	
	
	bUpperSizer->Add( sbSizerDrawMode, 1, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* sbSizerViewOpt;
	sbSizerViewOpt = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Pan and Zoom") ), wxVERTICAL );
	
	m_IsZoomNoCenter = new wxCheckBox( this, wxID_ANY, _("Do not center and warp cusor on zoom"), wxDefaultPosition, wxDefaultSize, 0 );
	m_IsZoomNoCenter->SetToolTip( _("Keep the cursor at its current location when zooming") );
	
	sbSizerViewOpt->Add( m_IsZoomNoCenter, 0, wxEXPAND|wxALL, 5 );
	
	m_IsMiddleButtonPan = new wxCheckBox( this, wxID_ANY, _("Use middle mouse button to pan"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerViewOpt->Add( m_IsMiddleButtonPan, 0, wxEXPAND|wxALL, 5 );
	
	m_IsMiddleButtonPanLimited = new wxCheckBox( this, wxID_ANY, _("Limit panning to scroll size"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerViewOpt->Add( m_IsMiddleButtonPanLimited, 0, wxALL|wxEXPAND, 5 );
	
	
	bUpperSizer->Add( sbSizerViewOpt, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizerMain->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Apply = new wxButton( this, wxID_APPLY );
	m_sdbSizer1->AddButton( m_sdbSizer1Apply );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bSizerMain->Add( m_sdbSizer1, 0, wxEXPAND|wxALL, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	// Connect Events
	m_IsMiddleButtonPan->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnMiddleBtnPanEnbl ), NULL, this );
	m_sdbSizer1Apply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnApplyClick ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnOkClick ), NULL, this );
}

DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::~DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE()
{
	// Disconnect Events
	m_IsMiddleButtonPan->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnMiddleBtnPanEnbl ), NULL, this );
	m_sdbSizer1Apply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnApplyClick ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE::OnOkClick ), NULL, this );
	
}
