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

#include "pcbnew_jobs_handler.h"
#include <jobs/job_export_pcb_gerber.h>
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
#include <pcbnew_settings.h>
#include <wx/crt.h>
#include <pcb_plot_svg.h>
#include <gendrill_Excellon_writer.h>
#include <gendrill_gerber_writer.h>
#include <wildcards_and_files_ext.h>

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
    Register( "drill",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportDrill, this, std::placeholders::_1 ) );
    Register( "pos",
              std::bind( &PCBNEW_JOBS_HANDLER::JobExportPos, this, std::placeholders::_1 ) );
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
    params.m_includeExcludedBom = aStepJob->m_includeExcludedBom;
    params.m_minDistance = aStepJob->m_minDistance;
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

    PDF_PLOTTER* plotter = (PDF_PLOTTER*) StartPlotBoard(
            brd, &plotOpts, UNDEFINED_LAYER, aPdfJob->m_outputFile, wxEmptyString, wxEmptyString );

    if( plotter )
    {
        PlotBoardLayers( brd, plotter, aPdfJob->m_printMaskLayer.SeqStackupBottom2Top(), plotOpts );
        PlotInteractiveLayer( brd, plotter );
        plotter->EndPlot();
    }

    delete plotter;

    return CLI::EXIT_CODES::OK;
}


int PCBNEW_JOBS_HANDLER::JobExportGerber( JOB* aJob )
{
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
    plotOpts.SetFormat( PLOT_FORMAT::GERBER );

    plotOpts.SetPlotFrameRef( aGerberJob->m_plotBorderTitleBlocks );
    plotOpts.SetPlotValue( aGerberJob->m_plotFootprintValues );
    plotOpts.SetPlotReference( aGerberJob->m_plotRefDes );

    plotOpts.SetLayerSelection( aGerberJob->m_printMaskLayer );

    plotOpts.SetSubtractMaskFromSilk( aGerberJob->m_subtractSolderMaskFromSilk );
    // Always disable plot pad holes
    plotOpts.SetDrillMarksType( DRILL_MARKS::NO_DRILL_SHAPE );

    plotOpts.SetDisableGerberMacros( aGerberJob->m_disableApertureMacros );
    plotOpts.SetUseGerberX2format( aGerberJob->m_useX2Format );
    plotOpts.SetIncludeGerberNetlistInfo( aGerberJob->m_includeNetlistAttributes );

    plotOpts.SetGerberPrecision( aGerberJob->m_precision );

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

    delete plotter;

    return CLI::EXIT_CODES::OK;
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
        excellonWriter->SetFormat( aDrillJob->m_drillUnits
                                          == JOB_EXPORT_PCB_DRILL::DRILL_UNITS::MILLIMETERS,
                                  zeroFmt, precision.m_Lhs, precision.m_Rhs );
        excellonWriter->SetOptions( aDrillJob->m_excellonMirrorY, aDrillJob->m_excellonMinimalHeader,
                                   offset, aDrillJob->m_excellonCombinePTHNPTH );
        excellonWriter->SetRouteModeForOvalHoles( aDrillJob->m_excellonOvalDrillRoute );
        excellonWriter->SetMapFileFormat( mapFormat );

        excellonWriter->CreateDrillandMapFilesSet( aDrillJob->m_outputDir, true,
                                                  aDrillJob->m_generateMap, nullptr );
    }
    else if( aDrillJob->m_format == JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::GERBER )
    {
        GERBER_WRITER* gerberWriter = dynamic_cast<GERBER_WRITER*>( drillWriter.get() );
        // Set gerber precision: only 5 or 6 digits for mantissa are allowed
        // (SetFormat() accept 5 or 6, and any other value set the precision to 5)
        // the integer part precision is always 4, and units always mm
        gerberWriter->SetFormat( aDrillJob->m_gerberPrecision );
        gerberWriter->SetOptions( offset );
        gerberWriter->SetMapFileFormat( mapFormat );

        gerberWriter->CreateDrillandMapFilesSet( aDrillJob->m_outputDir, true,
                                                aDrillJob->m_generateMap, nullptr );
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

    if( aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::ASCII || aPosJob->m_format == JOB_EXPORT_PCB_POS::FORMAT::CSV )
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

        PLACE_FILE_EXPORTER exporter( brd, aPosJob->m_units == JOB_EXPORT_PCB_POS::UNITS::MILLIMETERS,
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
