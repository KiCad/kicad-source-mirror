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

#include <dialogs/dialog_rc_job.h>
#include <jobs/job_pcb_drc.h>

class DIALOG_DRC_JOB_CONFIG : public DIALOG_RC_JOB
{
public:
    DIALOG_DRC_JOB_CONFIG( wxWindow* parent, JOB_PCB_DRC* aJob );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    JOB_PCB_DRC* m_drcJob;
};