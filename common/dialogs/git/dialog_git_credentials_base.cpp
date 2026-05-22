///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_git_credentials_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GIT_CREDENTIALS_BASE::DIALOG_GIT_CREDENTIALS_BASE( wxWindow* parent, wxWindowID id, const wxString& title,
                                                          const wxPoint& pos, const wxSize& size, long style ) :
        DIALOG_SHIM( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxSize( 400, -1 ), wxDefaultSize );

    wxBoxSizer* m_mainSizer;
    m_mainSizer = new wxBoxSizer( wxVERTICAL );

    m_promptLabel = new wxStaticText( this, wxID_ANY, _( "Authentication is required to access:" ), wxDefaultPosition,
                                      wxDefaultSize, 0 );
    m_promptLabel->Wrap( -1 );
    m_mainSizer->Add( m_promptLabel, 0, wxALL, 5 );

    m_urlLabel = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    m_urlLabel->Wrap( -1 );
    m_mainSizer->Add( m_urlLabel, 0, wxBOTTOM | wxEXPAND | wxLEFT | wxRIGHT, 5 );

    wxString m_authChoiceChoices[] = { _( "Username and password" ), _( "SSH key" ) };
    int      m_authChoiceNChoices = sizeof( m_authChoiceChoices ) / sizeof( wxString );
    m_authChoice = new wxRadioBox( this, wxID_ANY, _( "Authentication" ), wxDefaultPosition, wxDefaultSize,
                                   m_authChoiceNChoices, m_authChoiceChoices, 1, wxRA_SPECIFY_ROWS );
    m_authChoice->SetSelection( 0 );
    m_mainSizer->Add( m_authChoice, 0, wxALL | wxEXPAND, 5 );

    wxFlexGridSizer* fgSizer1;
    fgSizer1 = new wxFlexGridSizer( 4, 3, 5, 0 );
    fgSizer1->AddGrowableCol( 1 );
    fgSizer1->SetFlexibleDirection( wxBOTH );
    fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    m_userLabel = new wxStaticText( this, wxID_ANY, _( "Username:" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_userLabel->Wrap( -1 );
    fgSizer1->Add( m_userLabel, 0, wxALL, 5 );

    m_userCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    fgSizer1->Add( m_userCtrl, 0, wxEXPAND, 5 );


    fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

    m_passLabel = new wxStaticText( this, wxID_ANY, _( "Password / token:" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_passLabel->Wrap( -1 );
    fgSizer1->Add( m_passLabel, 0, wxALL, 5 );

    m_passCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
    fgSizer1->Add( m_passCtrl, 0, wxEXPAND, 5 );


    fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

    m_keyLabel = new wxStaticText( this, wxID_ANY, _( "SSH key:" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_keyLabel->Wrap( -1 );
    fgSizer1->Add( m_keyLabel, 0, wxALL, 5 );

    m_keyPicker = new wxFilePickerCtrl( this, wxID_ANY, wxEmptyString, _( "Select SSH private key" ), _( "*" ),
                                        wxDefaultPosition, wxDefaultSize, wxFLP_DEFAULT_STYLE );
    fgSizer1->Add( m_keyPicker, 0, wxEXPAND, 5 );


    fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

    m_saveCheck = new wxCheckBox( this, wxID_ANY, _( "Save credentials" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_saveCheck->SetValue( true );
    fgSizer1->Add( m_saveCheck, 0, wxALL, 5 );

    m_sdbSizer1 = new wxStdDialogButtonSizer();
    m_sdbSizer1OK = new wxButton( this, wxID_OK );
    m_sdbSizer1->AddButton( m_sdbSizer1OK );
    m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
    m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
    m_sdbSizer1->Realize();

    fgSizer1->Add( m_sdbSizer1, 1, wxALL | wxEXPAND, 5 );


    m_mainSizer->Add( fgSizer1, 1, wxALL | wxEXPAND, 5 );


    this->SetSizer( m_mainSizer );
    this->Layout();
    m_mainSizer->Fit( this );

    this->Centre( wxBOTH );

    // Connect Events
    m_authChoice->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED,
                           wxCommandEventHandler( DIALOG_GIT_CREDENTIALS_BASE::OnConnTypeChanged ), NULL, this );
}

DIALOG_GIT_CREDENTIALS_BASE::~DIALOG_GIT_CREDENTIALS_BASE()
{
}
