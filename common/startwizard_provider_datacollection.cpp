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

#include "startwizard_provider_datacollection.h"

#include <app_monitor.h>

#include "pgm_base.h"
#include <settings/common_settings.h>


wxPanel* STARTWIZARD_PROVIDER_DATACOLLECTION::GetWizardPanel( wxWindow* aParent )
{
    m_model = std::make_shared<PANEL_DATA_COLLECTION_MODEL>();
    return new PANEL_DATA_COLLECTION( m_model, aParent );
}


void STARTWIZARD_PROVIDER_DATACOLLECTION::Finish()
{
    APP_MONITOR::SENTRY::Instance()->SetSentryOptIn( m_model->m_enableSentry );

    Pgm().GetCommonSettings()->m_DoNotShowAgain.data_collection_prompt = true;
}


bool STARTWIZARD_PROVIDER_DATACOLLECTION::NeedsUserInput() const
{
    return !Pgm().GetCommonSettings()->m_DoNotShowAgain.data_collection_prompt;
}
