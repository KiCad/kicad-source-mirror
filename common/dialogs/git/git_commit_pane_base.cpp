///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "git_commit_pane_base.h"

///////////////////////////////////////////////////////////////////////////

GIT_COMMIT_PANE_BASE::GIT_COMMIT_PANE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                                            long style, const wxString& name ) :
        wxPanel( parent, id, pos, size, style, name )
{
    wxBoxSizer* bMainSizer;
    bMainSizer = new wxBoxSizer( wxVERTICAL );

    m_emptyLabel = new wxStaticText( this, wxID_ANY, _( "No changes" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_emptyLabel->Wrap( -1 );
    bMainSizer->Add( m_emptyLabel, 0, wxALL, 8 );

    m_checklist = new FILE_CHECKLIST_CTRL( this, wxID_ANY );
    bMainSizer->Add( m_checklist, 1, wxALL | wxEXPAND, 4 );

    m_message = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
    m_message->SetMinSize( wxSize( -1, 60 ) );

    bMainSizer->Add( m_message, 0, wxEXPAND | wxLEFT | wxRIGHT, 4 );

    m_amendCheckbox = new wxCheckBox( this, wxID_ANY, _( "Amend last commit" ), wxDefaultPosition, wxDefaultSize, 0 );
    bMainSizer->Add( m_amendCheckbox, 0, wxLEFT | wxRIGHT | wxTOP, 8 );

    wxBoxSizer* bCommitSizer;
    bCommitSizer = new wxBoxSizer( wxHORIZONTAL );

    m_commitOnlyButton = new wxButton( this, wxID_ANY, _( "Commit" ), wxDefaultPosition, wxDefaultSize, 0 );
    bCommitSizer->Add( m_commitOnlyButton, 1, wxALL, 4 );

    m_commitButton = new wxButton( this, wxID_ANY, _( "Commit and Push" ), wxDefaultPosition, wxDefaultSize, 0 );
    bCommitSizer->Add( m_commitButton, 1, wxALL, 4 );


    bMainSizer->Add( bCommitSizer, 0, wxEXPAND, 0 );

    wxBoxSizer* bActionSizer;
    bActionSizer = new wxBoxSizer( wxHORIZONTAL );

    m_pullButton = new wxButton( this, wxID_ANY, _( "Get Changes" ), wxDefaultPosition, wxDefaultSize, 0 );
    bActionSizer->Add( m_pullButton, 1, wxALL, 4 );

    m_advancedButton = new wxButton( this, wxID_ANY, _( "Advanced..." ), wxDefaultPosition, wxDefaultSize, 0 );
    bActionSizer->Add( m_advancedButton, 1, wxALL, 4 );


    bMainSizer->Add( bActionSizer, 0, wxEXPAND, 0 );


    this->SetSizer( bMainSizer );
    this->Layout();
}

GIT_COMMIT_PANE_BASE::~GIT_COMMIT_PANE_BASE()
{
}
