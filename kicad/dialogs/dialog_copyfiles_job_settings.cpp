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

#include "dialog_copyfiles_job_settings.h"
#include <jobs/job_special_copyfiles.h>


DIALOG_COPYFILES_JOB_SETTINGS::DIALOG_COPYFILES_JOB_SETTINGS( wxWindow* aParent,
                                                              JOB_SPECIAL_COPYFILES* aJob ) :
        DIALOG_COPYFILES_JOB_SETTINGS_BASE( aParent ),
        m_job( aJob )
{
    m_textCtrlSource->SetValidator( wxTextValidator( wxFILTER_EMPTY ) );

    SetupStandardButtons();
}


bool DIALOG_COPYFILES_JOB_SETTINGS::TransferDataToWindow()
{
    m_textCtrlSource->SetValue( m_job->m_source );
    m_textCtrlDest->SetValue( m_job->m_dest );
    m_cbGenerateError->SetValue( m_job->m_generateErrorOnNoCopy );
    m_cbOverwrite->SetValue( m_job->m_overwriteDest );
    return true;
}


bool DIALOG_COPYFILES_JOB_SETTINGS::TransferDataFromWindow()
{
    m_job->m_source = m_textCtrlSource->GetValue();
    m_job->m_dest = m_textCtrlDest->GetValue();
    m_job->m_generateErrorOnNoCopy = m_cbGenerateError->GetValue();
    m_job->m_overwriteDest = m_cbOverwrite->GetValue();
    return true;
}
