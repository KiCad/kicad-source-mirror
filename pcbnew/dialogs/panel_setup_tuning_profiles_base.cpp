///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

#include "panel_setup_tuning_profiles_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_TUNING_PROFILES_BASE::PANEL_SETUP_TUNING_PROFILES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 2, 1, 0, 0 );
	fgSizer1->AddGrowableCol( 0 );
	fgSizer1->AddGrowableRow( 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	m_tuningProfiles = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	fgSizer1->Add( m_tuningProfiles, 1, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bSizer91;
	bSizer91 = new wxBoxSizer( wxHORIZONTAL );

	m_addTuningProfileButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer91->Add( m_addTuningProfileButton, 0, wxBOTTOM|wxLEFT, 5 );


	bSizer91->Add( 20, 0, 1, wxEXPAND, 5 );

	m_removeTuningProfileButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer91->Add( m_removeTuningProfileButton, 0, wxBOTTOM|wxLEFT, 5 );


	fgSizer1->Add( bSizer91, 1, wxTOP, 5 );


	this->SetSizer( fgSizer1 );
	this->Layout();

	// Connect Events
	m_addTuningProfileButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TUNING_PROFILES_BASE::OnAddTuningProfileClick ), NULL, this );
	m_removeTuningProfileButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TUNING_PROFILES_BASE::OnRemoveTuningProfileClick ), NULL, this );
}

PANEL_SETUP_TUNING_PROFILES_BASE::~PANEL_SETUP_TUNING_PROFILES_BASE()
{
}
