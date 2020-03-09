///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_color_settings_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_COLOR_SETTINGS_BASE::PANEL_COLOR_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bControlSizer;
	bControlSizer = new wxBoxSizer( wxHORIZONTAL );

	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Theme:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	bControlSizer->Add( m_staticText9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxString m_cbThemeChoices[] = { _("User") };
	int m_cbThemeNChoices = sizeof( m_cbThemeChoices ) / sizeof( wxString );
	m_cbTheme = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbThemeNChoices, m_cbThemeChoices, 0 );
	m_cbTheme->SetSelection( 0 );
	m_cbTheme->SetMinSize( wxSize( 150,-1 ) );

	bControlSizer->Add( m_cbTheme, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_btnSave = new wxButton( this, wxID_ANY, _("&Save"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btnSave->Enable( false );
	m_btnSave->SetToolTip( _("Save the active color theme") );

	bControlSizer->Add( m_btnSave, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_btnNew = new wxButton( this, wxID_ANY, _("&New"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btnNew->SetToolTip( _("Create a new color theme based on the current one") );

	bControlSizer->Add( m_btnNew, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_btnRename = new wxButton( this, wxID_ANY, _("Rename"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btnRename->Enable( false );
	m_btnRename->SetToolTip( _("The \"User\" theme cannot be renamed") );

	bControlSizer->Add( m_btnRename, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bControlSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_btnReset = new wxButton( this, wxID_ANY, _("&Reset to Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btnReset->SetToolTip( _("Reset all colors in this theme to the KiCad defaults") );

	bControlSizer->Add( m_btnReset, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_btnOpenFolder = new wxButton( this, wxID_ANY, _("Open Theme Folder"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btnOpenFolder->SetToolTip( _("Open the folder containing color themes") );

	bControlSizer->Add( m_btnOpenFolder, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	m_mainSizer->Add( bControlSizer, 0, wxEXPAND|wxRIGHT, 10 );

	m_panelThemeProperties = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelThemeProperties->Hide();

	m_sizerThemeProperties = new wxBoxSizer( wxHORIZONTAL );

	m_staticText6 = new wxStaticText( m_panelThemeProperties, wxID_ANY, _("Theme name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	m_sizerThemeProperties->Add( m_staticText6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_txtThemeName = new wxTextCtrl( m_panelThemeProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_txtThemeName->HasFlag( wxTE_MULTILINE ) )
	{
	m_txtThemeName->SetMaxLength( 48 );
	}
	#else
	m_txtThemeName->SetMaxLength( 48 );
	#endif
	m_txtThemeName->SetToolTip( _("Name of the theme") );
	m_txtThemeName->SetMinSize( wxSize( 200,-1 ) );

	m_sizerThemeProperties->Add( m_txtThemeName, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	m_sizerThemeProperties->Add( 24, 0, 0, wxEXPAND, 5 );

	m_staticText7 = new wxStaticText( m_panelThemeProperties, wxID_ANY, _("Filename:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	m_sizerThemeProperties->Add( m_staticText7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_txtFilename = new wxTextCtrl( m_panelThemeProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_txtFilename->SetToolTip( _("Filename to save the theme to (must end in .json)") );
	m_txtFilename->SetMinSize( wxSize( 200,-1 ) );

	m_sizerThemeProperties->Add( m_txtFilename, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_lblThemePropertiesError = new wxStaticText( m_panelThemeProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_lblThemePropertiesError->Wrap( -1 );
	m_lblThemePropertiesError->SetForegroundColour( wxColour( 255, 0, 0 ) );

	m_sizerThemeProperties->Add( m_lblThemePropertiesError, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	m_panelThemeProperties->SetSizer( m_sizerThemeProperties );
	m_panelThemeProperties->Layout();
	m_sizerThemeProperties->Fit( m_panelThemeProperties );
	m_mainSizer->Add( m_panelThemeProperties, 0, wxEXPAND | wxALL, 0 );

	m_colorsMainSizer = new wxBoxSizer( wxHORIZONTAL );

	m_colorsListWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_colorsListWindow->SetScrollRate( 5, 5 );
	m_colorsListWindow->SetMinSize( wxSize( 240,-1 ) );

	m_colorsGridSizer = new wxFlexGridSizer( 0, 2, 0, 0 );
	m_colorsGridSizer->AddGrowableCol( 0 );
	m_colorsGridSizer->SetFlexibleDirection( wxHORIZONTAL );
	m_colorsGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_NONE );


	m_colorsListWindow->SetSizer( m_colorsGridSizer );
	m_colorsListWindow->Layout();
	m_colorsGridSizer->Fit( m_colorsListWindow );
	m_colorsMainSizer->Add( m_colorsListWindow, 0, wxEXPAND | wxALL, 5 );


	m_mainSizer->Add( m_colorsMainSizer, 1, wxEXPAND, 5 );


	this->SetSizer( m_mainSizer );
	this->Layout();

	// Connect Events
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_COLOR_SETTINGS_BASE::OnSize ) );
	m_cbTheme->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnThemeChanged ), NULL, this );
	m_btnSave->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnBtnSaveClicked ), NULL, this );
	m_btnNew->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnBtnNewClicked ), NULL, this );
	m_btnRename->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnBtnRenameClicked ), NULL, this );
	m_btnReset->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnBtnResetClicked ), NULL, this );
	m_btnOpenFolder->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnBtnOpenThemeFolderClicked ), NULL, this );
	m_txtThemeName->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnThemeNameChanged ), NULL, this );
	m_txtFilename->Connect( wxEVT_CHAR, wxKeyEventHandler( PANEL_COLOR_SETTINGS_BASE::OnFilenameChar ), NULL, this );
	m_txtFilename->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnFilenameChanged ), NULL, this );
}

PANEL_COLOR_SETTINGS_BASE::~PANEL_COLOR_SETTINGS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_COLOR_SETTINGS_BASE::OnSize ) );
	m_cbTheme->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnThemeChanged ), NULL, this );
	m_btnSave->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnBtnSaveClicked ), NULL, this );
	m_btnNew->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnBtnNewClicked ), NULL, this );
	m_btnRename->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnBtnRenameClicked ), NULL, this );
	m_btnReset->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnBtnResetClicked ), NULL, this );
	m_btnOpenFolder->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnBtnOpenThemeFolderClicked ), NULL, this );
	m_txtThemeName->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnThemeNameChanged ), NULL, this );
	m_txtFilename->Disconnect( wxEVT_CHAR, wxKeyEventHandler( PANEL_COLOR_SETTINGS_BASE::OnFilenameChar ), NULL, this );
	m_txtFilename->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnFilenameChanged ), NULL, this );

}
