/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pcbnew_jobs_handler.h"
#include <jobs/job_fp_export_svg.h>
#include <jobs/job_fp_upgrade.h>
#include <jobs/job_export_pcb_gerber.h>
#include <jobs/job_export_pcb_gerbers.h>
#include <jobs/job_export_pcb_drill.h>
#include <jobs/job_export_pcb_dxf.h>
#include <jobs/job_export_pcb_pdf.h>
#include <jobs/job_export_pcb_pos.h>
#include <jobs/job_export_pcb_svg.h>
#include <jobs/job_export_pcb_step.h>
#include <cli/exit_codes.h>
#include <plotters/plotter_dxf.h>
#include <plotters/plotter_gerber.h>
#include <plotters/plotters_pslike.h>
#include <exporters/place_file_exporter.h>
#include <exporters/step/exporter_step.h>
#include "gerber_placefile_writer.h"
#include <pgm_base.h>
#include <pcbplot.h>
#include <board_design_settings.h>
#include <pad.h>
#include <pcbnew_settings.h>
#include <wx/crt.h>
#include <wx/dir.h>
#include <pcb_plot_svg.h>
#include <gendrill_Excellon_writer.h>
#include <gendrill_gerber_writer.h>
#include <wildcards_and_files_ext.h>
#include <plugins/kicad/pcb_plugin.h>
#include <gerber_jobfile_writer.h>

#include "pcbnew_scripting_helpers.h"


PCBNEW_JOBS_HANDLER::PCBNEW_JOBS_HANDLER()
{
    Register( "step",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportStep, this, std::placeholders::_1 ) );
    Register( "svg", std::bind( &PCBNEW_JOBS_HANDLER::JobExportSvg, this, std::placeholders::_1 ) );
    Register( "dxf", std::bind( &PCBNEW_JOBS_HANDLER::JobExportDxf, this, std::placeholders::_1 ) );
    Register( "pdf", std::bind( &PCBNEW_JOBS_HANDLER::JobExportPdf, this, std::placeholders::_1 ) );
    Register( "gerber",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportGerber, this, std::placeholders::_1 ) );
    Register( "gerbers",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportGerbers, this, std::placeholders::_1 ) );
    Register( "drill",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportDrill, this, std::placeholders::_1 ) );
    Register( "pos", std::bind( &PCBNEW_JOBS_HANDLER::JobExportPos, this, std::placeholders::_1 ) );
    Register( "fpupgrade",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportFpUpgrade, this, std::placeholders::_1 ) );
    Register( "fpsvg",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportFpSvg, this, std::placeholders::_1 ) );
}


int PCBNEW_JOBS_HANDLER::JobExportStep( JOB* aJob )
{
    JOB_EXPORT_PCB_STEP* aStepJob = dynamic_cast<JOB_EXPORT_PCB_STEP*>( aJob );

    if( aStepJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    if( aJob->IsCli() )
        wxPrintf( _( "Loading board\n" ) );

    BOARD* brd = LoadBoard( aStepJob->m_filename );

    if( aStepJob->m_outputFile.IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( wxS( "step" ) );

        aStepJob->m_outputFile = fn.GetFullName();
    }

    EXPORTER_STEP_PARAMS params;
    params.m_exportTracks = aStepJob->m_exportTracks;
    params.m_includeUnspecified = aStepJob->m_includeUnspecified;
    params.m_includeDNP = aStepJob->m_includeDNP;
    params.m_BoardOutlinesChainingEpsilon = aStepJob->m_BoardOutlinesChainingEpsilon;
    params.m_overwrite = aStepJob->m_overwrite;
    params.m_substModels = aStepJob->m_substModels;
    params.m_origin = VECTOR2D( aStepJob->m_xOrigin, aStepJob->m_yOrigin );
    params.m_useDrillOrigin = aStepJob->m_useDrillOrigin;
    params.m_useGridOrigin = aStepJob->m_useGridOrigin;
    params.m_boardOnly = aStepJob->m_boardOnly;

    EXPORTER_STEP stepExporter( brd, params );
    stepExporter.m_outputFile = aStepJob->m_outputFile;

    if( !stepExporter.Export() )
    {
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportSvg( JOB* aJob )
{
    JOB_EXPORT_PCB_SVG* aSvgJob = dynamic_cast<JOB_EXPORT_PCB_SVG*>( aJob );

    if( aSvgJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    PCB_PLOT_SVG_OPTIONS svgPlotOptions;
    svgPlotOptions.m_blackAndWhite = aSvgJob->m_blackAndWhite;
    svgPlotOptions.m_colorTheme = aSvgJob->m_colorTheme;
    svgPlotOptions.m_outputFile = aSvgJob->m_outputFile;
    svgPlotOptions.m_mirror = aSvgJob->m_mirror;
    svgPlotOptions.m_pageSizeMode = aSvgJob->m_pageSizeMode;
    svgPlotOptions.m_printMaskLayer = aSvgJob->m_printMaskLayer;
    svgPlotOptions.m_plotFrame = aSvgJob->m_plotDrawingSheet;

    if( aJob->IsCli() )
        wxPrintf( _( "Loading board\n" ) );

    BOARD* brd = LoadBoard( aSvgJob->m_filename );

    if( aJob->IsCli() )
    {
        if( PCB_PLOT_SVG::Plot( brd, svgPlotOptions ) )
            wxPrintf( _( "Successfully created svg file" ) );
        else
            wxPrintf( _( "Error creating svg file" ) );
    }

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportDxf( JOB* aJob )
{
    JOB_EXPORT_PCB_DXF* aDxfJob = dynamic_cast<JOB_EXPORT_PCB_DXF*>( aJob );

    if( aDxfJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    if( aJob->IsCli() )
        wxPrintf( _( "Loading board\n" ) );

    BOARD* brd = LoadBoard( aDxfJob->m_filename );

    if( aDxfJob->m_outputFile.IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( GetDefaultPlotExtension( PLOT_FORMAT::DXF ) );

        aDxfJob->m_outputFile = fn.GetFullName();
    }

    PCB_PLOT_PARAMS plotOpts;
    plotOpts.SetFormat( PLOT_FORMAT::DXF );

    plotOpts.SetDXFPlotPolygonMode( aDxfJob->m_plotGraphicItemsUsingContours );

    if( aDxfJob->m_dxfUnits == JOB_EXPORT_PCB_DXF::DXF_UNITS::MILLIMETERS )
    {
        plotOpts.SetDXFPlotUnits( DXF_UNITS::MILLIMETERS );
    }
    else
    {
        plotOpts.SetDXFPlotUnits( DXF_UNITS::INCHES );
    }

    plotOpts.SetPlotFrameRef( aDxfJob->m_plotBorderTitleBlocks );
    plotOpts.SetPlotValue( aDxfJob->m_plotFootprintValues );
    plotOpts.SetPlotReference( aDxfJob->m_plotRefDes );
    plotOpts.SetLayerSelection( aDxfJob->m_printMaskLayer );

    DXF_PLOTTER* plotter = (DXF_PLOTTER*) StartPlotBoard(
            brd, &plotOpts, UNDEFINED_LAYER, aDxfJob->m_outputFile, wxEmptyString, wxEmptyString );

    if( plotter )
    {
        PlotBoardLayers( brd, plotter, aDxfJob->m_printMaskLayer.SeqStackupBottom2Top(), plotOpts );
        plotter->EndPlot();
    }

    delete plotter;

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportPdf( JOB* aJob )
{
    JOB_EXPORT_PCB_PDF* aPdfJob = dynamic_cast<JOB_EXPORT_PCB_PDF*>( aJob );

    if( aPdfJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    if( aJob->IsCli() )
        wxPrintf( _( "Loading board\n" ) );

    BOARD* brd = LoadBoard( aPdfJob->m_filename );

    if( aPdfJob->m_outputFile.IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( GetDefaultPlotExtension( PLOT_FORMAT::PDF ) );

        aPdfJob->m_outputFile = fn.GetFullName();
    }

    PCB_PLOT_PARAMS plotOpts;
    plotOpts.SetFormat( PLOT_FORMAT::PDF );

    plotOpts.SetPlotFrameRef( aPdfJob->m_plotBorderTitleBlocks );
    plotOpts.SetPlotValue( aPdfJob->m_plotFootprintValues );
    plotOpts.SetPlotReference( aPdfJob->m_plotRefDes );

    plotOpts.SetLayerSelection( aPdfJob->m_printMaskLayer );

    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    plotOpts.SetColorSettings( mgr.GetColorSettings( aPdfJob->m_colorTheme ) );
    plotOpts.SetBlackAndWhite( aPdfJob->m_blackAndWhite );

    PDF_PLOTTER* plotter = (PDF_PLOTTER*) StartPlotBoard(
            brd, &plotOpts, UNDEFINED_LAYER, aPdfJob->m_outputFile, wxEmptyString, wxEmptyString );

    if( plotter )
    {
        PlotBoardLayers( brd, plotter, aPdfJob->m_printMaskLayer.SeqStackupBottom2Top(), plotOpts );
        PlotInteractiveLayer( brd, plotter, plotOpts );
        plotter->EndPlot();
    }

    delete plotter;

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportGerbers( JOB* aJob )
{
    int                     exitCode = CLI::EXIT_CODES::OK;
    JOB_EXPORT_PCB_GERBERS* aGerberJob = dynamic_cast<JOB_EXPORT_PCB_GERBERS*>( aJob );

    if( aGerberJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    if( aJob->IsCli() )
        wxPrintf( _( "Loading board\n" ) );

    BOARD*          brd = LoadBoard( aGerberJob->m_filename );
    PCB_PLOT_PARAMS boardPlotOptions = brd->GetPlotOptions();
    LSET                  plotOnAllLayersSelection = boardPlotOptions.GetPlotOnAllLayersSelection();
    GERBER_JOBFILE_WRITER jobfile_writer( brd );

    wxString fileExt;

    if( aGerberJob->m_useBoardPlotParams )
    {
        aGerberJob->m_printMaskLayer = boardPlotOptions.GetLayerSelection();
        aGerberJob->m_layersIncludeOnAll = boardPlotOptions.GetPlotOnAllLayersSelection();
    }
    else
    {
        // default to the board enabled layers
        if( aGerberJob->m_printMaskLayer == 0 )
            aGerberJob->m_printMaskLayer = brd->GetEnabledLayers();

        if( aGerberJob->m_layersIncludeOnAllSet )
            aGerberJob->m_layersIncludeOnAll = plotOnAllLayersSelection;
    }

    for( LSEQ seq = aGerberJob->m_printMaskLayer.UIOrder(); seq; ++seq )
    {
        LSEQ plotSequence;

        // Base layer always gets plotted first.
        plotSequence.push_back( *seq );

        // Now all the "include on all" layers
        for( LSEQ seqAll = aGerberJob->m_layersIncludeOnAll.UIOrder(); seqAll; ++seqAll )
        {
            // Don't plot the same layer more than once;
            if( find( plotSequence.begin(), plotSequence.end(), *seqAll ) != plotSequence.end() )
                continue;

            plotSequence.push_back( *seqAll );
        }

        // Pick the basename from the board file
        wxFileName fn( brd->GetFileName() );
        PCB_LAYER_ID layer = *seq;
        fileExt = GetGerberProtelExtension( layer );

        PCB_PLOT_PARAMS plotOpts;

        if( aGerberJob->m_useBoardPlotParams )
            plotOpts = boardPlotOptions;
        else
            populateGerberPlotOptionsFromJob( plotOpts, aGerberJob );

        BuildPlotFileName( &fn, aGerberJob->m_outputFile, brd->GetLayerName( layer ), fileExt );
        wxString fullname = fn.GetFullName();

        jobfile_writer.AddGbrFile( layer, fullname );

        // We are feeding it one layer at the start here to silence a logic check
        GERBER_PLOTTER* plotter = (GERBER_PLOTTER*) StartPlotBoard(
                brd, &plotOpts, layer, fn.GetFullPath(), wxEmptyString, wxEmptyString );

        if( plotter )
        {
            wxPrintf( _( "Plotted to '%s'.\n" ), fn.GetFullPath() );
            PlotBoardLayers( brd, plotter, plotSequence, plotOpts );
            plotter->EndPlot();
        }
        else
        {
            wxFprintf( stderr, _( "Failed to plot to '%s'.\n" ), fn.GetFullPath() );
            exitCode = CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
        }

        delete plotter;
    }

    wxFileName fn( aGerberJob->m_filename );
    // Build gerber job file from basename
    BuildPlotFileName( &fn, aGerberJob->m_outputFile, wxT( "job" ), GerberJobFileExtension );
    jobfile_writer.CreateJobFile( fn.GetFullPath() );

    return exitCode;
}


void PCBNEW_JOBS_HANDLER::populateGerberPlotOptionsFromJob( PCB_PLOT_PARAMS&       aPlotOpts,
                                                            JOB_EXPORT_PCB_GERBER* aJob )
{
    aPlotOpts.SetFormat( PLOT_FORMAT::GERBER );

    aPlotOpts.SetPlotFrameRef( aJob->m_plotBorderTitleBlocks );
    aPlotOpts.SetPlotValue( aJob->m_plotFootprintValues );
    aPlotOpts.SetPlotReference( aJob->m_plotRefDes );

    aPlotOpts.SetSubtractMaskFromSilk( aJob->m_subtractSolderMaskFromSilk );

    // Always disable plot pad holes
    aPlotOpts.SetDrillMarksType( DRILL_MARKS::NO_DRILL_SHAPE );

    aPlotOpts.SetDisableGerberMacros( aJob->m_disableApertureMacros );
    aPlotOpts.SetUseGerberX2format( aJob->m_useX2Format );
    aPlotOpts.SetIncludeGerberNetlistInfo( aJob->m_includeNetlistAttributes );
    aPlotOpts.SetUseAuxOrigin( aJob->m_useAuxOrigin );

    aPlotOpts.SetGerberPrecision( aJob->m_precision );
}


int PCBNEW_JOBS_HANDLER::JobExportGerber( JOB* aJob )
{
    int                    exitCode = CLI::EXIT_CODES::OK;
    JOB_EXPORT_PCB_GERBER* aGerberJob = dynamic_cast<JOB_EXPORT_PCB_GERBER*>( aJob );

    if( aGerberJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    if( aJob->IsCli() )
        wxPrintf( _( "Loading board\n" ) );

    BOARD* brd = LoadBoard( aGerberJob->m_filename );

    if( aGerberJob->m_outputFile.IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( GetDefaultPlotExtension( PLOT_FORMAT::GERBER ) );

        aGerberJob->m_outputFile = fn.GetFullName();
    }

    PCB_PLOT_PARAMS plotOpts;
    populateGerberPlotOptionsFromJob( plotOpts, aGerberJob );
    plotOpts.SetLayerSelection( aGerberJob->m_printMaskLayer );

    // We are feeding it one layer at the start here to silence a logic check
    GERBER_PLOTTER* plotter = (GERBER_PLOTTER*) StartPlotBoard(
            brd, &plotOpts, aGerberJob->m_printMaskLayer.Seq().front(), aGerberJob->m_outputFile,
            wxEmptyString, wxEmptyString );

    if( plotter )
    {
        PlotBoardLayers( brd, plotter, aGerberJob->m_printMaskLayer.SeqStackupBottom2Top(),
                         plotOpts );
        plotter->EndPlot();
    }
    else
    {
        wxFprintf( stderr, _( "Failed to plot to '%s'.\n" ), aGerberJob->m_outputFile );
        exitCode = CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    delete plotter;

    return exitCode;
}

static DRILL_PRECISION precisionListForInches( 2, 4 );
static DRILL_PRECISION precisionListForMetric( 3, 3 );


int PCBNEW_JOBS_HANDLER::JobExportDrill( JOB* aJob )
{
    JOB_EXPORT_PCB_DRILL* aDrillJob = dynamic_cast<JOB_EXPORT_PCB_DRILL*>( aJob );

    if( aDrillJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    if( aJob->IsCli() )
        wxPrintf( _( "Loading board\n" ) );

    BOARD* brd = LoadBoard( aDrillJob->m_filename );

    std::unique_ptr<GENDRILL_WRITER_BASE> drillWriter;

    if( aDrillJob->m_format == JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::EXCELLON )
    {
        drillWriter = std::make_unique<EXCELLON_WRITER>( brd );
    }
    else
    {
        drillWriter = std::make_unique<GERBER_WRITER>( brd );
    }

    VECTOR2I offset;

    if( aDrillJob->m_drillOrigin == JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN::ABSOLUTE )
        offset = VECTOR2I( 0, 0 );
    else
        offset = brd->GetDesignSettings().GetAuxOrigin();

    PLOT_FORMAT mapFormat = PLOT_FORMAT::PDF;

    switch( aDrillJob->m_mapFormat )
    {
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::POSTSCRIPT: mapFormat = PLOT_FORMAT::POST; break;
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::GERBER_X2: mapFormat = PLOT_FORMAT::GERBER; break;
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::DXF: mapFormat = PLOT_FORMAT::DXF; break;
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::SVG: mapFormat = PLOT_FORMAT::SVG; break;
    default:
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::PDF: mapFormat = PLOT_FORMAT::PDF; break;
    }

    if( aDrillJob->m_format == JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::EXCELLON )
    {
        EXCELLON_WRITER::ZEROS_FMT zeroFmt;
        switch( aDrillJob->m_zeroFormat )
        {
        case JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::KEEP_ZEROS:
            zeroFmt = EXCELLON_WRITER::KEEP_ZEROS;
            break;
        case JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::SUPPRESS_LEADING:
            zeroFmt = EXCELLON_WRITER::SUPPRESS_LEADING;
            break;
        case JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::SUPPRESS_TRAILING:
            zeroFmt = EXCELLON_WRITER::SUPPRESS_TRAILING;
            break;
        case JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::DECIMAL:
        default:
            zeroFmt = EXCELLON_WRITER::DECIMAL_FORMAT;
            break;
        }

        DRILL_PRECISION precision;

        if( aDrillJob->m_drillUnits == JOB_EXPORT_PCB_DRILL::DRILL_UNITS::INCHES )
            precision = precisionListForInches;
        else
            precision = precisionListForMetric;

        EXCELLON_WRITER* excellonWriter = dynamic_cast<EXCELLON_WRITER*>( drillWriter.get() );

        if( excellonWriter == nullptr )
            return CLI::EXIT_CODES::ERR_UNKNOWN;

        excellonWriter->SetFormat( aDrillJob->m_drillUnits
                                          == JOB_EXPORT_PCB_DRILL::DRILL_UNITS::MILLIMETERS,
                                   zeroFmt, precision.m_Lhs, precision.m_Rhs );
        excellonWriter->SetOptions( aDrillJob->m_excellonMirrorY,
                                    aDrillJob->m_excellonMinimalHeader,
                                    offset, aDrillJob->m_excellonCombinePTHNPTH );
        excellonWriter->SetRouteModeForOvalHoles( aDrillJob->m_excellonOvalDrillRoute );
        excellonWriter->SetMapFileFormat( mapFormat );

        if( !excellonWriter->CreateDrillandMapFilesSet( aDrillJob->m_outputDir, true,
                                                        aDrillJob->m_generateMap, this ) )
        {
            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
        }
    }
    else if( aDrillJob->m_format == JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::GERBER )
    {
        GERBER_WRITER* gerberWriter = dynamic_cast<GERBER_WRITER*>( drillWriter.get() );

        if( gerberWriter == nullptr )
            return CLI::EXIT_CODES::ERR_UNKNOWN;

        // Set gerber precision: only 5 or 6 digits for mantissa are allowed
        // (SetFormat() accept 5 or 6, and any other value set the precision to 5)
        // the integer part precision is always 4, and units always mm
        gerberWriter->SetFormat( aDrillJob->m_gerberPrecision );
        gerberWriter->SetOptions( offset );
        gerberWriter->SetMapFileFormat( mapFormat );

        if( !gerberWriter->CreateDrillandMapFilesSet( aDrillJob->m_outputDir, true,
                                                      aDrillJob->m_generateMap, this ) )
        {
            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
        }
    }

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportPos( JOB* aJob )
{
    JOB_EXPORT_PCB_POS* aPosJob = dynamic_cast<JOB_EXPORT_PCB_POS*>( aJob );

    if( aPosJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    if( aJob->IsCli() )
        wxPrintf( _( "Loading board\n" ) );

    BOARD* brd = LoadBoard( aPosJob->m_filename );

    if( aPosJob->m_outputFile.IsEmpty() )
    {
        wxFileName fn = brd->GetFileName();
        fn.SetName( fn.GetName() );

        if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::ASCII )
            fn.SetExt( FootprintPlaceFileExtension );
        else if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::CSV )
            fn.SetExt( CsvFileExtension );
        else if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::GERBER )
            fn.SetExt( GerberFileExtension );

        aPosJob->m_outputFile = fn.GetFullName();
    }

    if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::ASCII
      || aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::CSV )
    {
        FILE* file = nullptr;
        file = wxFopen( aPosJob->m_outputFile, wxS( "wt" ) );

        if( file == nullptr )
            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;

        std::string         data;

        bool frontSide = aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::FRONT
                         || aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BOTH;

        bool backSide = aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BACK
                         || aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BOTH;

        PLACE_FILE_EXPORTER exporter( brd,
                                      aPosJob->m_units == JOB_EXPORT_PCB_POS::UNITS::MILLIMETERS,
                                      aPosJob->m_smdOnly, aPosJob->m_excludeFootprintsWithTh,
                                      frontSide, backSide,
                                      aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::CSV,
                                      aPosJob->m_useDrillPlaceFileOrigin,
                                      aPosJob->m_negateBottomX );
        data = exporter.GenPositionData();

        fputs( data.c_str(), file );
        fclose( file );
    }
    else if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::GERBER )
    {
        PLACEFILE_GERBER_WRITER exporter( brd );

        PCB_LAYER_ID gbrLayer = F_Cu;

        if( aPosJob->m_side == JOB_EXPORT_PCB_POS::SIDE::BACK )
            gbrLayer = B_Cu;

        exporter.CreatePlaceFile( aPosJob->m_outputFile, gbrLayer, aPosJob->m_gerberBoardEdge );
    }

    return CLI::EXIT_CODES::OK;
}

extern FOOTPRINT* try_load_footprint( const wxFileName& aFileName, IO_MGR::PCB_FILE_T aFileType,
                                      const wxString& aName );


int PCBNEW_JOBS_HANDLER::JobExportFpUpgrade( JOB* aJob )
{
    JOB_FP_UPGRADE* upgradeJob = dynamic_cast<JOB_FP_UPGRADE*>( aJob );

    if( upgradeJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    if( aJob->IsCli() )
        wxPrintf( _( "Loading footprint library\n" ) );

    if( !upgradeJob->m_outputLibraryPath.IsEmpty() )
    {
        if( wxFile::Exists( upgradeJob->m_outputLibraryPath ) ||
            wxDir::Exists( upgradeJob->m_outputLibraryPath) )
        {
            wxFprintf( stderr, _( "Output path must not conflict with existing path\n" ) );
            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
        }
    }

    PCB_PLUGIN  pcb_io( CTL_FOR_LIBRARY );
    FP_CACHE   fpLib( &pcb_io, upgradeJob->m_libraryPath );

    try
    {
        fpLib.Load();
    }
    catch(...)
    {
        wxFprintf( stderr, _( "Unable to load library\n" ) );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    bool shouldSave = upgradeJob->m_force;

    for( const auto& footprint : fpLib.GetFootprints() )
    {
        if( footprint.second->GetFootprint()->GetFileFormatVersionAtLoad() < SEXPR_BOARD_FILE_VERSION )
        {
            shouldSave = true;
        }
    }

    if( shouldSave )
    {
        wxPrintf( _( "Saving footprint library\n" ) );

        try
        {
            if( !upgradeJob->m_outputLibraryPath.IsEmpty() )
            {
                fpLib.SetPath( upgradeJob->m_outputLibraryPath );
            }

            fpLib.Save();
        }
        catch( ... )
        {
            wxFprintf( stderr, _( "Unable to save library\n" ) );
            return CLI::EXIT_CODES::ERR_UNKNOWN;
        }
    }
    else
    {
        wxPrintf( _( "Footprint library was not updated\n" ) );
    }

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportFpSvg( JOB* aJob )
{
    JOB_FP_EXPORT_SVG* svgJob = dynamic_cast<JOB_FP_EXPORT_SVG*>( aJob );

    if( svgJob == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    if( aJob->IsCli() )
        wxPrintf( _( "Loading footprint library\n" ) );

    PCB_PLUGIN pcb_io( CTL_FOR_LIBRARY );
    FP_CACHE   fpLib( &pcb_io, svgJob->m_libraryPath );

    try
    {
        fpLib.Load();
    }
    catch( ... )
    {
        wxFprintf( stderr, _( "Unable to load library\n" ) );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    if( !svgJob->m_outputDirectory.IsEmpty() && !wxDir::Exists( svgJob->m_outputDirectory ) )
    {
        wxFileName::Mkdir( svgJob->m_outputDirectory );
    }

    int exitCode = CLI::EXIT_CODES::OK;

    // Just plot all the symbols we can
    FP_CACHE_FOOTPRINT_MAP& footprintMap = fpLib.GetFootprints();

    bool singleFpPlotted = false;
    for( FP_CACHE_FOOTPRINT_MAP::iterator it = footprintMap.begin(); it != footprintMap.end();
         ++it )
    {
        const FOOTPRINT* fp = it->second->GetFootprint();
        if( !svgJob->m_footprint.IsEmpty() )
        {
            if( fp->GetFPID().GetLibItemName().wx_str() != svgJob->m_footprint )
            {
                // skip until we find the right footprint
                continue;
            }
            else
            {
                singleFpPlotted = true;
            }
        }

        exitCode = doFpExportSvg( svgJob, fp );
        if( exitCode != CLI::EXIT_CODES::OK )
            break;
    }

    if( !svgJob->m_footprint.IsEmpty() && !singleFpPlotted )
        wxFprintf( stderr, _( "The given footprint could not be found to export." ) );

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::doFpExportSvg( JOB_FP_EXPORT_SVG* aSvgJob, const FOOTPRINT* aFootprint )
{
    // the hack for now is we create fake boards containing the footprint and plot the board
    // until we refactor better plot api later
    std::unique_ptr<BOARD> brd;
    brd.reset( CreateEmptyBoard() );

    FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( aFootprint->Clone() );

    if( fp == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    fp->SetLink( niluuid );
    fp->SetFlags( IS_NEW );
    fp->SetParent( brd.get() );

    for( PAD* pad : fp->Pads() )
    {
        pad->SetLocalRatsnestVisible( false );
        pad->SetNetCode( 0 );
    }

    fp->SetOrientation( ANGLE_0 );
    fp->SetPosition( VECTOR2I( 0, 0 ) );

    brd->Add( fp, ADD_MODE::INSERT, true );

    wxFileName outputFile;
    outputFile.SetPath( aSvgJob->m_outputDirectory );
    outputFile.SetName( aFootprint->GetFPID().GetLibItemName().wx_str() );
    outputFile.SetExt( SVGFileExtension );

    wxPrintf( _( "Plotting footprint '%s' to '%s'\n" ),
              aFootprint->GetFPID().GetLibItemName().wx_str(), outputFile.GetFullPath() );


    PCB_PLOT_SVG_OPTIONS svgPlotOptions;
    svgPlotOptions.m_blackAndWhite = aSvgJob->m_blackAndWhite;
    svgPlotOptions.m_colorTheme = aSvgJob->m_colorTheme;
    svgPlotOptions.m_outputFile = outputFile.GetFullPath();
    svgPlotOptions.m_mirror = false;
    svgPlotOptions.m_pageSizeMode = 2; // board bounding box
    svgPlotOptions.m_printMaskLayer = aSvgJob->m_printMaskLayer;
    svgPlotOptions.m_plotFrame = false;

    if( !PCB_PLOT_SVG::Plot( brd.get(), svgPlotOptions ) )
        wxFprintf( stderr, _( "Error creating svg file" ) );


    return CLI::EXIT_CODES::OK;
}


REPORTER& PCBNEW_JOBS_HANDLER::Report( const wxString& aText, SEVERITY aSeverity )
{
    if( aSeverity == RPT_SEVERITY_ERROR )
        wxFprintf( stderr, wxS( "%s\n" ), aText );
    else
        wxPrintf( wxS( "%s\n" ), aText );

    return *this;
}