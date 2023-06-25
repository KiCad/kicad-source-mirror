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

#include "startwizard_provider_pcm.h"

#include <settings/common_settings.h>

#include "dialogs/panel_pcm_startwizard_base.h"
#include "settings/kicad_settings.h"
#include "pgm_kicad.h"

class PANEL_PCM_STARTWIZARD : public PANEL_PCM_STARTWIZARD_BASE
{
public:
    PANEL_PCM_STARTWIZARD( std::shared_ptr<PANEL_PCM_STARTWIZARD_MODEL> aModel,
                           wxWindow*                                    aParent ) :
            PANEL_PCM_STARTWIZARD_BASE( aParent ),
            m_model( aModel ){};


    bool TransferDataFromWindow() override
    {
        m_model->m_autoUpdateCheck = m_cbAutoUpdate->GetValue();

        return true;
    }

    bool TransferDataToWindow() override { return true; }

private:
    std::shared_ptr<PANEL_PCM_STARTWIZARD_MODEL> m_model;
};


STARTWIZARD_PROVIDER_PCM::STARTWIZARD_PROVIDER_PCM() :
        STARTWIZARD_PROVIDER( wxT( "Plugin & Content Manager" ) )
{
}


bool STARTWIZARD_PROVIDER_PCM::NeedsUserInput() const
{
    COMMON_SETTINGS* commonSettings = PgmTop().GetCommonSettings();
    return !commonSettings->m_DoNotShowAgain.update_check_prompt;
}


wxPanel* STARTWIZARD_PROVIDER_PCM::GetWizardPanel( wxWindow* aParent )
{
    m_model = std::make_shared<PANEL_PCM_STARTWIZARD_MODEL>();
    return new PANEL_PCM_STARTWIZARD( m_model, aParent );
}


void STARTWIZARD_PROVIDER_PCM::Finish()
{
    COMMON_SETTINGS* commonSettings = PgmTop().GetCommonSettings();
    KICAD_SETTINGS* settings = static_cast<KICAD_SETTINGS*>( PgmTop().PgmSettings() );

    if( !commonSettings->m_DoNotShowAgain.update_check_prompt )
    {
        if( m_model->m_autoUpdateCheck )
        {
            settings->m_KiCadUpdateCheck = true;
            settings->m_PcmUpdateCheck = true;
        }
        else
        {
            settings->m_KiCadUpdateCheck = false;
            settings->m_PcmUpdateCheck = false;
        }
    }
}
