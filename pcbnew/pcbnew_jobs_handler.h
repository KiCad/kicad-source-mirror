/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PCBNEW_JOBS_HANDLER_H
#define PCBNEW_JOBS_HANDLER_H

#include <jobs/job_dispatcher.h>
#include <pcb_plot_params.h>
#include <reporter.h>

class JOB_EXPORT_PCB_GERBER;
class JOB_FP_EXPORT_SVG;
class FOOTPRINT;

class PCBNEW_JOBS_HANDLER : public JOB_DISPATCHER, REPORTER
{
public:
    PCBNEW_JOBS_HANDLER();
    int JobExportStep( JOB* aJob );
    int JobExportSvg( JOB* aJob );
    int JobExportDxf( JOB* aJob );
    int JobExportPdf( JOB* aJob );
    int JobExportGerber( JOB* aJob );
    int JobExportGerbers( JOB* aJob );
    int JobExportDrill( JOB* aJob );
    int JobExportPos( JOB* aJob );
    int JobExportFpUpgrade( JOB* aJob );
    int JobExportFpSvg( JOB* aJob );

    /*
     * REPORTER INTERFACE
     */
    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;

    bool HasMessage() const override { return false; }

private:
    void populateGerberPlotOptionsFromJob( PCB_PLOT_PARAMS&       aPlotOpts,
                                           JOB_EXPORT_PCB_GERBER* aJob );
    int  doFpExportSvg( JOB_FP_EXPORT_SVG* aSvgJob, const FOOTPRINT* aFootprint );
};

#endif