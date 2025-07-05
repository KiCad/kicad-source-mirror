/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <build_version.h>
#include <confirm.h>
#include <kiface_base.h>
#include <pgm_base.h>
#include <wx/statline.h>
#include <wx/wx.h>
#include <wx/wizard.h>
#include <startwizard/startwizard.h>
#include <startwizard/startwizard_provider.h>
#include <startwizard/startwizard_provider_libraries.h>
#include <startwizard/startwizard_provider_privacy.h>
#include <startwizard/startwizard_provider_settings.h>
#include <wx/hyperlink.h>


class STARTWIZARD_PAGE : public wxWizardPageSimple
{
public:
    STARTWIZARD_PAGE( wxWizard* aParent, const wxString& aPageTitle ) :
            wxWizardPageSimple( aParent )
    {
        m_mainSizer = new wxBoxSizer( wxVERTICAL );

        wxStaticText* pageTitle = new wxStaticText( this, -1, aPageTitle );
        pageTitle->SetFont(
                wxFont( 14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD ) );
        m_mainSizer->Add( pageTitle, 0, wxALIGN_CENTRE | wxALL, 5 );

        wxStaticLine* pageDivider = new wxStaticLine( this, wxID_ANY, wxDefaultPosition,
                                                      wxDefaultSize, wxLI_HORIZONTAL );
        m_mainSizer->Add( pageDivider, 0, wxEXPAND | wxALL, 5 );
    }

    void AddContent( wxPanel* aContent )
    {
        m_mainSizer->Add( aContent );

        SetSizerAndFit( m_mainSizer );
    }

private:
    wxBoxSizer* m_mainSizer;
};


class STARTWIZARD_WELCOME_PAGE : public wxWizardPageSimple
{
public:
    STARTWIZARD_WELCOME_PAGE( wxWizard* aParent ) : wxWizardPageSimple( aParent )
    {
        m_mainSizer = new wxBoxSizer( wxVERTICAL );

        wxStaticText* pageTitle = new wxStaticText(
                this, -1, wxString::Format( _( "Welcome to KiCad %s" ), GetMajorMinorVersion() ) );
        pageTitle->SetFont(
                wxFont( 14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD ) );
        m_mainSizer->Add( pageTitle, 0, wxALIGN_CENTRE | wxALL, 5 );

        wxStaticLine* pageDivider = new wxStaticLine( this, wxID_ANY, wxDefaultPosition,
                                                      wxDefaultSize, wxLI_HORIZONTAL );
        m_mainSizer->Add( pageDivider, 0, wxEXPAND | wxALL, 5 );

        m_welcomeText = new wxStaticText( this, -1,
            _( "KiCad is starting for the first time, or some of its configuration files are missing.\n\n"
               "Let's take a moment to configure some basic settings.  You can always modify these "
               "settings later by opening the Preferences dialog." ) );
        m_mainSizer->Add( m_welcomeText, 0, wxEXPAND | wxALL, 5 );

        wxBoxSizer* helpSizer = new wxBoxSizer( wxHORIZONTAL );
        wxStaticText* helpLabel = new wxStaticText( this, -1, _( "For help, please visit " ) );
        wxString docsUrl = wxString::Format( "https://go.kicad.org/docs/%s", GetMajorMinorVersion() );
        wxHyperlinkCtrl* helpLink = new wxHyperlinkCtrl( this, -1, wxT( "docs.kicad.org" ), docsUrl );
        helpSizer->Add( helpLabel, 0, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, 5 );
        helpSizer->Add( helpLink, 0, wxEXPAND | wxRIGHT | wxTOP | wxBOTTOM, 5 );
        m_mainSizer->Add( helpSizer, 0, wxEXPAND, 5 );

        SetSizerAndFit( m_mainSizer );
    }

    void SetWrap( int aWidth ) const
    {
        m_welcomeText->Wrap( aWidth );
    }

private:
    wxBoxSizer* m_mainSizer;
    wxStaticText* m_welcomeText;
};


STARTWIZARD::STARTWIZARD() :
    m_wizard( nullptr )
{
}


STARTWIZARD::~STARTWIZARD()
{
}


STARTWIZARD_PROVIDER* STARTWIZARD::GetProvider( const wxString& aName )
{
    if( auto it = std::ranges::find_if( m_providers,
                                        [&]( const std::unique_ptr<STARTWIZARD_PROVIDER>& aProvider )
                                        {
                                            return aProvider->Name() == aName;
                                        } ); it != m_providers.end() )
    {
        return it->get();
    }

    return nullptr;
}


void STARTWIZARD::CheckAndRun( wxWindow* aParent )
{
    m_providers.clear();

    m_providers.push_back( std::make_unique<STARTWIZARD_PROVIDER_SETTINGS>() );
    m_providers.push_back( std::make_unique<STARTWIZARD_PROVIDER_LIBRARIES>() );
    m_providers.push_back( std::make_unique<STARTWIZARD_PROVIDER_PRIVACY>() );
    //providers.push_back( std::make_unique<STARTWIZARD_PROVIDER_LOOK_AND_FEEL>() );

    if( m_providers.size() == 0 )
        return;

    bool wizardRequired = std::ranges::any_of( std::as_const( m_providers ),
                                               []( const std::unique_ptr<STARTWIZARD_PROVIDER>& aProvider ) -> bool
                                               {
                                                   return aProvider->NeedsUserInput();
                                               } );

    if( !wizardRequired )
        return;

    Pgm().HideSplash();

    m_wizard = new wxWizard( aParent, wxID_ANY, _( "KiCad Setup" ) );

    STARTWIZARD_WELCOME_PAGE* firstPage = new STARTWIZARD_WELCOME_PAGE( m_wizard );
    wxWizardPageSimple* lastPage = nullptr;
    wxSize minPageSize;

    for( std::unique_ptr<STARTWIZARD_PROVIDER>& provider : m_providers )
    {
        if( !provider->NeedsUserInput() )
            continue;

        STARTWIZARD_PAGE* page = new STARTWIZARD_PAGE( m_wizard, provider->GetPageName() );
        wxPanel*          panel = provider->GetWizardPanel( page, this );
        page->AddContent( panel );

        if( lastPage != nullptr )
        {
            lastPage->Chain( page );
        }
        else
        {
            firstPage->Chain( page );
        }

        lastPage = page;

        wxSize size = page->GetSizer()->CalcMin();

        if( size.x > minPageSize.x )
            minPageSize.x = size.x;

        if( size.y > minPageSize.y )
            minPageSize.y = size.y;
    }

    m_wizard->SetPageSize( minPageSize + wxSize( 10, 10 ) );
    firstPage->SetWrap( minPageSize.x );

    m_wizard->Bind( wxEVT_WIZARD_CANCEL,
            [&]( wxWizardEvent& aEvt )
            {
                if( IsOK( aParent, _( "Are you sure?  If you cancel KiCad setup, default settings "
                                      "will be chosen for you." ) ) )
                {
                    for( std::unique_ptr<STARTWIZARD_PROVIDER>& provider : m_providers )
                    {
                        provider->ApplyDefaults();
                    }
                }
                else
                {
                    aEvt.Veto();
                }
            } );

    if( m_wizard->RunWizard( firstPage ) )
    {
        for( std::unique_ptr<STARTWIZARD_PROVIDER>& provider : m_providers )
        {
            if( !provider->NeedsUserInput() )
                continue;

            provider->Finish();
        }
    }

    m_wizard->Destroy();
    m_wizard = nullptr;
}
