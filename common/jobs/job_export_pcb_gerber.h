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

#ifndef JOB_EXPORT_PCB_GERBER_H
#define JOB_EXPORT_PCB_GERBER_H

#include <kicommon.h>
#include <layer_ids.h>
#include <lseq.h>
#include <jobs/job_export_pcb_plot.h>

class KICOMMON_API JOB_EXPORT_PCB_GERBER : public JOB_EXPORT_PCB_PLOT
{
public:
    JOB_EXPORT_PCB_GERBER( const std::string& aType );
    JOB_EXPORT_PCB_GERBER();
    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;

    bool m_includeNetlistAttributes;
    bool m_useX2Format;
    bool m_disableApertureMacros;
    bool m_useProtelFileExtension;

    int m_precision;
};

#endif
