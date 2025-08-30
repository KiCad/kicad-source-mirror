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

#ifndef EESCHEMA_JOBS_HANDLER_H
#define EESCHEMA_JOBS_HANDLER_H

#include <jobs/job_dispatcher.h>
#include <wx/string.h>

class SCH_RENDER_SETTINGS;
class KIWAY;
class SCHEMATIC;
class JOB_SYM_EXPORT_SVG;
class LIB_SYMBOL;
class DS_PROXY_VIEW_ITEM;

/**
 * Handle Eeschema job dispatches.
 */
class EESCHEMA_JOBS_HANDLER : public JOB_DISPATCHER
{
public:
    EESCHEMA_JOBS_HANDLER( KIWAY* aKiway );
    int JobExportBom( JOB* aJob );
    int JobExportPythonBom( JOB* aJob );
    int JobExportNetlist( JOB* aJob );
    int JobExportPlot( JOB* aJob );
    int JobSchErc( JOB* aJob );
    int JobSymUpgrade( JOB* aJob );
    int JobSymExportSvg( JOB* aJob );
    int JobUpgrade( JOB* aJob );

    /**
     * Configure the SCH_RENDER_SETTINGS object with the correct data to be used with plotting.
     *
     * It's sort of a kludge due to the plotter depending on this object normally managed by
     * the frame and canvas.
     *
     * @param aRenderSettings The object to populate with working settings.
     * @param aTheme The theme to take color data from to stick into render settings, can be
     *        left blank for default.
     * @param aSch The schematic to further copy settings from to be put into aRenderSettings.
     */
    void InitRenderSettings( SCH_RENDER_SETTINGS* aRenderSettings, const wxString& aTheme,
                             SCHEMATIC* aSch,
                             const wxString& aDrawingSheetOverride = wxEmptyString );

private:
    SCHEMATIC* getSchematic( const wxString& aPath );

    int doSymExportSvg( JOB_SYM_EXPORT_SVG* aSvgJob, SCH_RENDER_SETTINGS* aRenderSettings,
                        LIB_SYMBOL* symbol );

    DS_PROXY_VIEW_ITEM* getDrawingSheetProxyView( SCHEMATIC* aSch );

    SCHEMATIC* m_cliSchematic;
};

#endif
