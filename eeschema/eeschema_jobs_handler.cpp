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

#include "eeschema_jobs_handler.h"
#include <cli/exit_codes.h>
#include <jobs/job_export_sch_pdf.h>
#include <jobs/job_export_sch_svg.h>
#include <pgm_base.h>
#include <sch_plotter.h>
#include <schematic.h>
#include <wx/crt.h>
#include <memory>
#include <connection_graph.h>
#include "eeschema_helpers.h"
#include <sch_painter.h>

#include <settings/settings_manager.h>


EESCHEMA_JOBS_HANDLER::EESCHEMA_JOBS_HANDLER()
{
    Register( "pdf",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobExportPdf, this, std::placeholders::_1 ) );
    Register( "svg",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobExportSvg, this, std::placeholders::_1 ) );
}


void EESCHEMA_JOBS_HANDLER::InitRenderSettings( KIGFX::SCH_RENDER_SETTINGS* aRenderSettings,
                                                const wxString& aTheme, SCHEMATIC* aSch )
{
    COLOR_SETTINGS* cs = Pgm().GetSettingsManager().GetColorSettings( aTheme );
    aRenderSettings->LoadColors( cs );

    aRenderSettings->SetDefaultPenWidth( aSch->Settings().m_DefaultLineWidth );
    aRenderSettings->m_LabelSizeRatio = aSch->Settings().m_LabelSizeRatio;
    aRenderSettings->m_TextOffsetRatio = aSch->Settings().m_TextOffsetRatio;
    aRenderSettings->m_PinSymbolSize = aSch->Settings().m_PinSymbolSize;

    aRenderSettings->SetDashLengthRatio( aSch->Settings().m_DashedLineDashRatio );
    aRenderSettings->SetGapLengthRatio( aSch->Settings().m_DashedLineGapRatio );
}


int EESCHEMA_JOBS_HANDLER::JobExportPdf( JOB* aJob )
{
    JOB_EXPORT_SCH_PDF* aPdfJob = dynamic_cast<JOB_EXPORT_SCH_PDF*>( aJob );

    SCHEMATIC* sch = EESCHEMA_HELPERS::LoadSchematic( aPdfJob->m_filename, SCH_IO_MGR::SCH_KICAD );
    std::unique_ptr<KIGFX::SCH_RENDER_SETTINGS> renderSettings =
            std::make_unique<KIGFX::SCH_RENDER_SETTINGS>();
    InitRenderSettings( renderSettings.get(), aPdfJob->m_colorTheme, sch );

    std::unique_ptr<SCH_PLOTTER> schPlotter = std::make_unique<SCH_PLOTTER>( sch );

    SCH_PLOT_SETTINGS settings;
    settings.m_plotAll = true;
    settings.m_plotDrawingSheet = aPdfJob->m_plotDrawingSheet;
    settings.m_blackAndWhite = aPdfJob->m_blackAndWhite;
    settings.m_theme = aPdfJob->m_colorTheme;
    settings.m_useBackgroundColor = aPdfJob->m_useBackgroundColor;
    settings.m_pageSizeSelect = PAGE_SIZE_AUTO;
    settings.m_outputFile = aPdfJob->m_outputFile;

    schPlotter->Plot( PLOT_FORMAT::PDF, settings, renderSettings.get(), nullptr );

    return 0;
}


int EESCHEMA_JOBS_HANDLER::JobExportSvg( JOB* aJob )
{
    JOB_EXPORT_SCH_SVG* aSvgJob = dynamic_cast<JOB_EXPORT_SCH_SVG*>( aJob );

    SCHEMATIC* sch = EESCHEMA_HELPERS::LoadSchematic( aSvgJob->m_filename, SCH_IO_MGR::SCH_KICAD );
    std::unique_ptr<KIGFX::SCH_RENDER_SETTINGS> renderSettings =
            std::make_unique<KIGFX::SCH_RENDER_SETTINGS>();
    InitRenderSettings( renderSettings.get(), aSvgJob->m_colorTheme, sch );

    std::unique_ptr<SCH_PLOTTER> schPlotter = std::make_unique<SCH_PLOTTER>( sch );

    SCH_PLOT_SETTINGS settings;
    settings.m_plotAll = true;
    settings.m_plotDrawingSheet = true;
    settings.m_blackAndWhite = aSvgJob->m_blackAndWhite;
    settings.m_theme = aSvgJob->m_colorTheme;
    settings.m_pageSizeSelect = PAGE_SIZE_AUTO;
    settings.m_outputDirectory = aSvgJob->m_outputDirectory;
    settings.m_useBackgroundColor = aSvgJob->m_useBackgroundColor;

    schPlotter->Plot( PLOT_FORMAT::SVG, settings, renderSettings.get(), nullptr );

    return 0;
}