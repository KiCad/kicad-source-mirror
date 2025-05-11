/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
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

#ifndef JOB_EXPORT_PCB_PS_H
#define JOB_EXPORT_PCB_PS_H

#include <kicommon.h>
#include <kicommon.h>
#include <layer_ids.h>
#include <lseq.h>
#include <jobs/job_export_pcb_plot.h>


class KICOMMON_API JOB_EXPORT_PCB_PS : public JOB_EXPORT_PCB_PLOT
{
public:
    JOB_EXPORT_PCB_PS();
    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;

    enum class GEN_MODE
    {
        SINGLE,
        MULTI
    };

    GEN_MODE  m_genMode;
    double    m_trackWidthCorrection;
    double    m_XScaleAdjust;
    double    m_YScaleAdjust;
    bool      m_forceA4;
    bool      m_useGlobalSettings;
};

#endif
