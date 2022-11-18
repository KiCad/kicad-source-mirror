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
#include <jobs/job_export_sch_bom.h>
#include <jobs/job_export_sch_netlist.h>
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
#include <erc.h>
#include <wildcards_and_files_ext.h>

#include <settings/settings_manager.h>

#include <netlist.h>
#include <netlist_exporter_base.h>
#include <netlist_exporter_orcadpcb2.h>
#include <netlist_exporter_cadstar.h>
#include <netlist_exporter_spice.h>
#include <netlist_exporter_spice_model.h>
#include <netlist_exporter_kicad.h>
#include <netlist_exporter_xml.h>


EESCHEMA_JOBS_HANDLER::EESCHEMA_JOBS_HANDLER()
{
    Register( "bom",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobExportBom, this, std::placeholders::_1 ) );
    Register( "netlist",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobExportNetlist, this, std::placeholders::_1 ) );
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


REPORTER& EESCHEMA_JOBS_HANDLER::Report( const wxString& aText, SEVERITY aSeverity )
{
    if( aSeverity == RPT_SEVERITY_ERROR )
        wxFprintf( stderr, aText );
    else
        wxPrintf( aText );

    return *this;
}


int EESCHEMA_JOBS_HANDLER::JobExportPdf( JOB* aJob )
{
    JOB_EXPORT_SCH_PDF* aPdfJob = dynamic_cast<JOB_EXPORT_SCH_PDF*>( aJob );

    SCHEMATIC* sch = EESCHEMA_HELPERS::LoadSchematic( aPdfJob->m_filename, SCH_IO_MGR::SCH_KICAD );

    if( sch == nullptr )
    {
        wxFprintf( stderr, _( "Failed to load schematic file\n" ) );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

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

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::JobExportSvg( JOB* aJob )
{
    JOB_EXPORT_SCH_SVG* aSvgJob = dynamic_cast<JOB_EXPORT_SCH_SVG*>( aJob );

    SCHEMATIC* sch = EESCHEMA_HELPERS::LoadSchematic( aSvgJob->m_filename, SCH_IO_MGR::SCH_KICAD );

    if( sch == nullptr )
    {
        wxFprintf( stderr, _( "Failed to load schematic file\n" ) );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

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

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::JobExportNetlist( JOB* aJob )
{
    JOB_EXPORT_SCH_NETLIST* aNetJob = dynamic_cast<JOB_EXPORT_SCH_NETLIST*>( aJob );

    SCHEMATIC* sch = EESCHEMA_HELPERS::LoadSchematic( aNetJob->m_filename, SCH_IO_MGR::SCH_KICAD );

    if( sch == nullptr )
    {
        wxFprintf( stderr, _( "Failed to load schematic file\n" ) );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    // Annotation warning check
    SCH_REFERENCE_LIST referenceList;
    sch->GetSheets().GetSymbols( referenceList );
    if( referenceList.GetCount() > 0 )
    {
        if( referenceList.CheckAnnotation( []( ERCE_T, const wxString&, SCH_REFERENCE*, SCH_REFERENCE* ) {} ) > 0 )
        {
            wxPrintf( _( "Warning: schematic has annotation errors, please use the schematic editor to fix them\n" ) );
        }
    }

    // Test duplicate sheet names:
    ERC_TESTER erc( sch );

    if( erc.TestDuplicateSheetNames( false ) > 0 )
    {
        wxPrintf( _( "Warning: duplicate sheet names.\n" ) );
    }


    std::unique_ptr<NETLIST_EXPORTER_BASE> helper;

    wxString fileExt;

    switch( aNetJob->format )
    {
    case JOB_EXPORT_SCH_NETLIST::FORMAT::KICADSEXPR:
        fileExt = NetlistFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_KICAD>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::ORCADPCB2:
        fileExt = OrCadPcb2NetlistFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_ORCADPCB2>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::CADSTAR:
        fileExt = CadstarNetlistFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_CADSTAR>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::SPICE:
        fileExt = SpiceFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_SPICE>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::SPICEMODEL:
        fileExt = SpiceFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_SPICE_MODEL>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::KICADXML:
        fileExt = wxS( "xml" );
        helper = std::make_unique<NETLIST_EXPORTER_XML>( sch );
        break;
    default:
        wxFprintf( stderr, _( "Unknown netlist format.\n" ) );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }


    if( aNetJob->m_outputFile.IsEmpty() )
    {
        wxFileName fn = sch->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( fileExt );

        aNetJob->m_outputFile = fn.GetFullName();
    }

    bool res = helper->WriteNetlist( aNetJob->m_outputFile, 0, *this );

    if(!res)
    {
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::JobExportBom( JOB* aJob )
{
    JOB_EXPORT_SCH_BOM* aNetJob = dynamic_cast<JOB_EXPORT_SCH_BOM*>( aJob );

    SCHEMATIC* sch = EESCHEMA_HELPERS::LoadSchematic( aNetJob->m_filename, SCH_IO_MGR::SCH_KICAD );

    if( sch == nullptr )
    {
        wxFprintf( stderr, _( "Failed to load schematic file\n" ) );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    // Annotation warning check
    SCH_REFERENCE_LIST referenceList;
    sch->GetSheets().GetSymbols( referenceList );
    if( referenceList.GetCount() > 0 )
    {
        if( referenceList.CheckAnnotation(
                    []( ERCE_T, const wxString&, SCH_REFERENCE*, SCH_REFERENCE* )
                    {
                    } )
            > 0 )
        {
            wxPrintf( _( "Warning: schematic has annotation errors, please use the schematic "
                         "editor to fix them\n" ) );
        }
    }

    // Test duplicate sheet names:
    ERC_TESTER erc( sch );

    if( erc.TestDuplicateSheetNames( false ) > 0 )
    {
        wxPrintf( _( "Warning: duplicate sheet names.\n" ) );
    }

    if( aNetJob->format == JOB_EXPORT_SCH_BOM::FORMAT::XML )
    {
        std::unique_ptr<NETLIST_EXPORTER_XML> xmlNetlist =
                std::make_unique<NETLIST_EXPORTER_XML>( sch );

        wxString fileExt = wxS( "xml" );

        if( aNetJob->m_outputFile.IsEmpty() )
        {
            wxFileName fn = sch->GetFileName();
            fn.SetName( fn.GetName() + "-bom" );
            fn.SetExt( fileExt );

            aNetJob->m_outputFile = fn.GetFullName();
        }

        bool res = xmlNetlist->WriteNetlist( aNetJob->m_outputFile, GNL_OPT_BOM, *this );

        if( !res )
        {
            return CLI::EXIT_CODES::ERR_UNKNOWN;
        }

        return CLI::EXIT_CODES::OK;
    }

    return CLI::EXIT_CODES::ERR_UNKNOWN;
}