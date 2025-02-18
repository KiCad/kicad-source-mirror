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

#include <dialogs/dialog_executecommand_job_settings.h>
#include <jobs/job_special_execute.h>

DIALOG_EXECUTECOMMAND_JOB_SETTINGS::DIALOG_EXECUTECOMMAND_JOB_SETTINGS( wxWindow* aParent,
                                                                        JOB_SPECIAL_EXECUTE* aJob ) :
        DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE( aParent ),
        m_job( aJob )
{
    SetupStandardButtons();

    finishDialogSettings();
}


bool DIALOG_EXECUTECOMMAND_JOB_SETTINGS::TransferDataFromWindow()
{
    m_job->m_command = m_textCtrlCommand->GetValue();
    m_job->m_ignoreExitcode = m_cbIgnoreExitCode->GetValue();
    m_job->m_recordOutput = m_cbRecordOutput->GetValue();
    m_job->SetConfiguredOutputPath( m_textCtrlOutputPath->GetValue() );

    return true;
}


bool DIALOG_EXECUTECOMMAND_JOB_SETTINGS::TransferDataToWindow()
{
    m_textCtrlCommand->SetValue( m_job->m_command );
    m_cbIgnoreExitCode->SetValue( m_job->m_ignoreExitcode );
    m_cbRecordOutput->SetValue( m_job->m_recordOutput );

    m_textCtrlOutputPath->SetValue( m_job->GetConfiguredOutputPath() );
    m_textCtrlOutputPath->Enable( m_cbRecordOutput->GetValue() );

    return true;
}


void DIALOG_EXECUTECOMMAND_JOB_SETTINGS::OnRecordOutputClicked( wxCommandEvent& aEvent )
{
    m_textCtrlOutputPath->Enable( m_cbRecordOutput->GetValue() );
}