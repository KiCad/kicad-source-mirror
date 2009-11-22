///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "gerbview_dialog_display_options_frame_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DISPLAY_OPTIONS_BASE::DIALOG_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bDialogSizer;
	bDialogSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_OptDisplayLinesChoices[] = { _("Sketch"), _("Filled") };
	int m_OptDisplayLinesNChoices = sizeof( m_OptDisplayLinesChoices ) / sizeof( wxString );
	m_OptDisplayLines = new wxRadioBox( this, wxID_ANY, _("Lines:"), wxDefaultPosition, wxDefaultSize, m_OptDisplayLinesNChoices, m_OptDisplayLinesChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayLines->SetSelection( 1 );
	bLeftSizer->Add( m_OptDisplayLines, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_OptDisplayFlashedItemsChoices[] = { _("Sketch"), _("Filled") };
	int m_OptDisplayFlashedItemsNChoices = sizeof( m_OptDisplayFlashedItemsChoices ) / sizeof( wxString );
	m_OptDisplayFlashedItems = new wxRadioBox( this, wxID_ANY, _("Spots:"), wxDefaultPosition, wxDefaultSize, m_OptDisplayFlashedItemsNChoices, m_OptDisplayFlashedItemsChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayFlashedItems->SetSelection( 1 );
	bLeftSizer->Add( m_OptDisplayFlashedItems, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_OptDisplayPolygonsChoices[] = { _("Sketch"), _("Filled") };
	int m_OptDisplayPolygonsNChoices = sizeof( m_OptDisplayPolygonsChoices ) / sizeof( wxString );
	m_OptDisplayPolygons = new wxRadioBox( this, wxID_ANY, _("Polygons:"), wxDefaultPosition, wxDefaultSize, m_OptDisplayPolygonsNChoices, m_OptDisplayPolygonsChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayPolygons->SetSelection( 1 );
	bLeftSizer->Add( m_OptDisplayPolygons, 0, wxALL|wxEXPAND, 5 );
	
	bUpperSizer->Add( bLeftSizer, 1, wxEXPAND, 5 );
	
	
	bUpperSizer->Add( 20, 20, 0, 0, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_ShowPageLimitsChoices[] = { _("Full size. Do not show page limits"), _("Full size"), _("Size A4"), _("Size A3"), _("Size A2"), _("Size A"), _("Size B"), _("Size C") };
	int m_ShowPageLimitsNChoices = sizeof( m_ShowPageLimitsChoices ) / sizeof( wxString );
	m_ShowPageLimits = new wxRadioBox( this, wxID_ANY, _("Show Page Limits:"), wxDefaultPosition, wxDefaultSize, m_ShowPageLimitsNChoices, m_ShowPageLimitsChoices, 1, wxRA_SPECIFY_COLS );
	m_ShowPageLimits->SetSelection( 0 );
	bRightSizer->Add( m_ShowPageLimits, 0, wxALL|wxEXPAND, 5 );
	
	
	bRightSizer->Add( 20, 20, 0, 0, 5 );
	
	m_OptDisplayDCodes = new wxCheckBox( this, wxID_ANY, _("Show D codes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayDCodes->SetValue(true);
	
	bRightSizer->Add( m_OptDisplayDCodes, 0, wxALL, 5 );
	
	bUpperSizer->Add( bRightSizer, 1, wxEXPAND, 5 );
	
	bDialogSizer->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bDialogSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	bDialogSizer->Add( m_sdbSizer1, 0, wxEXPAND|wxALL, 5 );
	
	this->SetSizer( bDialogSizer );
	this->Layout();
	
	// Connect Events
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnOKBUttonClick ), NULL, this );
}

DIALOG_DISPLAY_OPTIONS_BASE::~DIALOG_DISPLAY_OPTIONS_BASE()
{
	// Disconnect Events
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnOKBUttonClick ), NULL, this );
}
