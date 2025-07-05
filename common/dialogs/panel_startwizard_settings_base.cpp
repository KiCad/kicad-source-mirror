///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

#include "panel_startwizard_settings_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_STARTWIZARD_SETTINGS_BASE::PANEL_STARTWIZARD_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	m_sizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	m_staticText2 = new wxStaticText( this, wxID_ANY, _("How would you like to configure KiCad?"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bSizer6->Add( m_staticText2, 0, wxALL|wxEXPAND, 5 );

	m_btnPrevVer = new wxRadioButton( this, wxID_ANY, _("Import settings from a previous version at:"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( m_btnPrevVer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );


	bSizer5->Add( 20, 0, 0, wxEXPAND, 5 );

	m_cbPath = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	bSizer5->Add( m_cbPath, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_btnCustomPath = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_btnCustomPath->SetToolTip( _("Choose a different path") );

	bSizer5->Add( m_btnCustomPath, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bSizer6->Add( bSizer5, 1, wxEXPAND, 5 );

	m_lblPathError = new wxStaticText( this, wxID_ANY, _("The selected path does not contain valid KiCad settings!"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblPathError->Wrap( -1 );
	m_lblPathError->SetForegroundColour( wxColour( 255, 43, 0 ) );
	m_lblPathError->Hide();

	bSizer6->Add( m_lblPathError, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

	m_btnUseDefaults = new wxRadioButton( this, wxID_ANY, _("Start with default settings"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( m_btnUseDefaults, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


	m_sizer->Add( bSizer6, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10 );


	this->SetSizer( m_sizer );
	this->Layout();

	// Connect Events
	m_btnPrevVer->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_STARTWIZARD_SETTINGS_BASE::OnPrevVerSelected ), NULL, this );
	m_cbPath->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( PANEL_STARTWIZARD_SETTINGS_BASE::OnPathChanged ), NULL, this );
	m_cbPath->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_STARTWIZARD_SETTINGS_BASE::OnPathDefocused ), NULL, this );
	m_cbPath->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( PANEL_STARTWIZARD_SETTINGS_BASE::OnPathChanged ), NULL, this );
	m_btnCustomPath->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_STARTWIZARD_SETTINGS_BASE::OnChoosePath ), NULL, this );
	m_btnUseDefaults->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_STARTWIZARD_SETTINGS_BASE::OnDefaultSelected ), NULL, this );
}

PANEL_STARTWIZARD_SETTINGS_BASE::~PANEL_STARTWIZARD_SETTINGS_BASE()
{
	// Disconnect Events
	m_btnPrevVer->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_STARTWIZARD_SETTINGS_BASE::OnPrevVerSelected ), NULL, this );
	m_cbPath->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( PANEL_STARTWIZARD_SETTINGS_BASE::OnPathChanged ), NULL, this );
	m_cbPath->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_STARTWIZARD_SETTINGS_BASE::OnPathDefocused ), NULL, this );
	m_cbPath->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( PANEL_STARTWIZARD_SETTINGS_BASE::OnPathChanged ), NULL, this );
	m_btnCustomPath->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_STARTWIZARD_SETTINGS_BASE::OnChoosePath ), NULL, this );
	m_btnUseDefaults->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_STARTWIZARD_SETTINGS_BASE::OnDefaultSelected ), NULL, this );

}
