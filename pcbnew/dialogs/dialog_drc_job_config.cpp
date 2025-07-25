/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/dialog_drc_job_config.h>

DIALOG_DRC_JOB_CONFIG::DIALOG_DRC_JOB_CONFIG( wxWindow* parent, JOB_PCB_DRC* aJob ) :
        DIALOG_RC_JOB( parent, aJob, _( "DRC Job Settings" ) ),
        m_drcJob( aJob )
{
    SetTitle( aJob->GetSettingsDialogTitle() );
	SetMinSize( GetBestSize() );
}


bool DIALOG_DRC_JOB_CONFIG::TransferDataToWindow()
{
    if( !DIALOG_RC_JOB::TransferDataToWindow() )
        return false;

    m_cbAllTrackViolations->SetValue( m_drcJob->m_reportAllTrackErrors );
    m_cbSchParity->SetValue( m_drcJob->m_parity );
    m_cbRefillZones->SetValue( m_drcJob->m_refillZones );
    return true;
}


bool DIALOG_DRC_JOB_CONFIG::TransferDataFromWindow()
{
    if( !DIALOG_RC_JOB::TransferDataFromWindow() )
        return false;

    m_drcJob->m_reportAllTrackErrors = m_cbAllTrackViolations->GetValue();
    m_drcJob->m_parity = m_cbSchParity->GetValue();
    m_drcJob->m_refillZones = m_cbRefillZones->GetValue();
    return true;
}