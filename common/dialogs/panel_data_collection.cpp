/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <dialogs/panel_data_collection.h>

#include <advanced_config.h>
#include <bitmaps.h>
#include <dialog_shim.h>
#include <gal/dpi_scaling.h>
#include <kiface_base.h>
#include <kiplatform/policy.h>
#include <kiplatform/ui.h>
#include <pgm_base.h>
#include <policy_keys.h>
#include <id.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>


PANEL_DATA_COLLECTION::PANEL_DATA_COLLECTION( wxWindow* aParent ) :
        PANEL_DATA_COLLECTION_BASE( aParent )
{
}


bool PANEL_DATA_COLLECTION::TransferDataToWindow()
{
    applySettingsToPanel();

    KIPLATFORM::POLICY::PBOOL policyState =
            KIPLATFORM::POLICY::GetPolicyBool( POLICY_KEY_DATACOLLECTION );
    if( policyState != KIPLATFORM::POLICY::PBOOL::NOT_CONFIGURED )
    {
        Disable();
    }

    m_sentryUid->SetValue( Pgm().GetSentryId() );

    return true;
}


bool PANEL_DATA_COLLECTION::TransferDataFromWindow()
{
    Pgm().SetSentryOptIn( m_cbOptIn->GetValue() );

    return true;
}


void PANEL_DATA_COLLECTION::ResetPanel()
{
    applySettingsToPanel();
}


void PANEL_DATA_COLLECTION::applySettingsToPanel()
{
    m_cbOptIn->SetValue( Pgm().IsSentryOptedIn() );
}


void PANEL_DATA_COLLECTION::OnResetIdClick( wxCommandEvent& aEvent )
{
    Pgm().ResetSentryId();
    m_sentryUid->SetValue( Pgm().GetSentryId() );
}