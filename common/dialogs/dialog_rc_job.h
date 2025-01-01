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

#pragma once

#include <jobs/job_rc.h>
#include <dialogs/dialog_rc_job_base.h>

class KICOMMON_API DIALOG_RC_JOB : public DIALOG_RC_JOB_BASE
{
public:
    DIALOG_RC_JOB( wxWindow* aParent, JOB_RC* aJob, const wxString& aTitle );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnFormatChoice( wxCommandEvent& aEvent ) override;

protected:
    JOB_RC::OUTPUT_FORMAT getSelectedFormat();
    void                  setSelectedFormat( JOB_RC::OUTPUT_FORMAT aFormat );

    JOB_RC* m_job;
};