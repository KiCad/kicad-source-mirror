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

#include <startwizard.h>
#include <startwizard_provider.h>
#include <kiface_base.h>
#include <wx/wizard.h>
#include <pgm_base.h>
#include <startwizard_provider_pcm.h>

#ifdef KICAD_USE_SENTRY
#include <startwizard_provider_datacollection.h>
#endif

#include <wx/statline.h>
#include <wx/wx.h>


class STARTWIZARD_PAGE : public wxWizardPageSimple
{
public:
    STARTWIZARD_PAGE( wxWizard* aParent, const wxString& aPageTitle ) : wxWizardPageSimple( aParent )
    {
        m_mainSizer = new wxBoxSizer( wxVERTICAL );

        wxStaticText* pageTitle = new wxStaticText( this, -1, wxString::Format( _( "Setup: %s" ), aPageTitle ) );
        pageTitle->SetFont(
                wxFont( 14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD ) );
        m_mainSizer->Add( pageTitle, 0, wxALIGN_CENTRE | wxALL, 5 );

        wxStaticLine* pageDivider = new wxStaticLine( this, wxID_ANY, wxDefaultPosition,
                                                      wxDefaultSize, wxLI_HORIZONTAL );
        m_mainSizer->Add( pageDivider, 0, wxEXPAND | wxALL, 5 );
    }

    void AddContent(wxPanel* aContent)
    {
        m_mainSizer->Add( aContent );

        SetSizerAndFit( m_mainSizer );
    }

private:
    wxBoxSizer* m_mainSizer;
};


STARTWIZARD::STARTWIZARD( KIWAY* aKiway ) :
    m_kiway( aKiway ),
    m_wizard( nullptr )
{
}


void STARTWIZARD::CheckAndRun( wxWindow* aParent )
{
    KIFACE* kiface = nullptr;

    std::vector<std::unique_ptr<STARTWIZARD_PROVIDER>> providers;

#ifdef KICAD_USE_SENTRY
    providers.push_back( std::make_unique<STARTWIZARD_PROVIDER_DATACOLLECTION>() );
#endif
    providers.push_back( std::make_unique<STARTWIZARD_PROVIDER_PCM>() );

    kiface = m_kiway->KiFACE( KIWAY::FACE_SCH );
    kiface->GetStartupProviders( providers );

    kiface = m_kiway->KiFACE( KIWAY::FACE_PCB );
    kiface->GetStartupProviders( providers );

    if( providers.size() == 0 )
        return;

    bool wizardRequired = std::any_of( providers.cbegin(), providers.cend(),
                                        []( const std::unique_ptr<STARTWIZARD_PROVIDER>& aProvider ) -> bool
                                        {
                                            return aProvider->NeedsUserInput();
                                        } );

    if( !wizardRequired )
        return;


    Pgm().HideSplash();

    m_wizard = new wxWizard( aParent );

    wxWizardPageSimple* firstPage = nullptr;
    wxWizardPageSimple* lastPage = nullptr;
    for( std::unique_ptr<STARTWIZARD_PROVIDER>& provider : providers )
    {
        if( !provider->NeedsUserInput() )
            continue;

        STARTWIZARD_PAGE* page = new STARTWIZARD_PAGE( m_wizard, provider->GetPageName() );
        wxPanel*          panel = provider->GetWizardPanel( page );
        page->AddContent( panel );

        if( firstPage == nullptr )
            firstPage = page;

        if( lastPage != nullptr )
        {
            lastPage->Chain( page );
        }

        lastPage = page;
    }

    m_wizard->SetPageSize( m_wizard->GetPageSize() * 1.5 );

    bool finished = m_wizard->RunWizard( firstPage );

    if (finished)
    {
        for( std::unique_ptr<STARTWIZARD_PROVIDER>& provider : providers )
        {
            if( !provider->NeedsUserInput() )
                continue;

            provider->Finish();
        }
    }

    m_wizard->Destroy();
    m_wizard = nullptr;
}
