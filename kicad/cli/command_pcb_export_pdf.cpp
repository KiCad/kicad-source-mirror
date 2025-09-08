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

#include "command_pcb_export_pdf.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_pcb_pdf.h"
#include <kiface_base.h>
#include <string_utils.h>
#include <wx/crt.h>

#define ARG_MODE_SEPARATE "--mode-separate"
#define ARG_MODE_MULTIPAGE "--mode-multipage"
#define ARG_MODE_SINGLE "--mode-single"

CLI::PCB_EXPORT_PDF_COMMAND::PCB_EXPORT_PDF_COMMAND() :
        PCB_EXPORT_BASE_COMMAND( "pdf", false, true )
{
    m_argParser.add_description( UTF8STDSTR( _( "Generate PDF from a list of layers" ) ) );

    addLayerArg();
    addCommonLayersArg();
    addDrawingSheetArg();
    addDefineArg();

    m_argParser.add_argument( "-m", ARG_MIRROR )
            .help( UTF8STDSTR( _( "Mirror the board (useful for trying to show bottom layers)" ) ) )
            .flag();

    m_argParser.add_argument( "--erd", ARG_EXCLUDE_REFDES )
            .help( UTF8STDSTR( _( "Exclude the reference designator text" ) ) )
            .flag();

    m_argParser.add_argument( "--ev", ARG_EXCLUDE_VALUE )
            .help( UTF8STDSTR( _( "Exclude the value text" ) ) )
            .flag();

    m_argParser.add_argument( "--ibt", ARG_INCLUDE_BORDER_TITLE )
            .help( UTF8STDSTR( _( "Include the border and title block" ) ) )
            .flag();

    m_argParser.add_argument( ARG_SUBTRACT_SOLDERMASK )
            .help( UTF8STDSTR( _( "Subtract soldermask from silkscreen" ) ) )
            .flag();

    m_argParser.add_argument( "--sp", ARG_SKETCH_PADS_ON_FAB_LAYERS )
            .help( UTF8STDSTR( _( ARG_SKETCH_PADS_ON_FAB_LAYERS_DESC ) ) )
            .flag();

    m_argParser.add_argument( "--hdnp", ARG_HIDE_DNP_FPS_ON_FAB_LAYERS )
            .help( UTF8STDSTR( _( ARG_HIDE_DNP_FPS_ON_FAB_LAYERS_DESC ) ) )
            .flag();

    m_argParser.add_argument( "--sdnp", ARG_SKETCH_DNP_FPS_ON_FAB_LAYERS )
            .help( UTF8STDSTR( _( ARG_SKETCH_DNP_FPS_ON_FAB_LAYERS_DESC ) ) )
            .flag();

    m_argParser.add_argument( "--cdnp", ARG_CROSSOUT_DNP_FPS_ON_FAB_LAYERS )
            .help( UTF8STDSTR( _( ARG_CROSSOUT_DNP_FPS_ON_FAB_LAYERS_DESC ) ) )
            .flag();

    m_argParser.add_argument( ARG_NEGATIVE_SHORT, ARG_NEGATIVE )
            .help( UTF8STDSTR( _( ARG_NEGATIVE_DESC ) ) )
            .flag();

    m_argParser.add_argument( ARG_BLACKANDWHITE )
            .help( UTF8STDSTR( _( ARG_BLACKANDWHITE_DESC ) ) )
            .flag();

    m_argParser.add_argument( "-t", ARG_THEME )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Color theme to use (will default to PCB Editor settings)" ) ) )
            .metavar( "THEME_NAME" );

    m_argParser.add_argument( ARG_DRILL_SHAPE_OPTION )
            .help( UTF8STDSTR( _( ARG_DRILL_SHAPE_OPTION_DESC ) ) )
            .scan<'i', int>()
            .default_value( 2 );

    m_argParser.add_argument( DEPRECATED_ARG_PLOT_INVISIBLE_TEXT )
            .help( UTF8STDSTR( _( DEPRECATED_ARG_PLOT_INVISIBLE_TEXT_DESC ) ) )
            .flag();

    m_argParser.add_argument( ARG_MODE_SINGLE )
            .help( UTF8STDSTR(
                    _( "Generates a single file with the output arg path acting as the complete "
                       "directory and filename path. COMMON_LAYER_LIST does not function in this "
                       "mode. Instead LAYER_LIST controls all layers plotted." ) ) )
            .flag();

    m_argParser.add_argument( ARG_MODE_SEPARATE )
            .help( UTF8STDSTR( _( "Plot the layers to individual PDF files" ) ) )
            .flag();

    m_argParser.add_argument( ARG_MODE_MULTIPAGE )
            .help( UTF8STDSTR( _( "Plot the layers to a single PDF file with multiple pages" ) ) )
            .flag();

    m_argParser.add_argument( ARG_SCALE )
            .help( UTF8STDSTR( _( ARG_SCALE_DESC ) ) )
            .scan<'g', double>()
            .default_value( 1.0 )
            .metavar( "SCALE" );

    m_argParser.add_argument( ARG_BACKGROUND_COLOR )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( ARG_BACKGROUND_COLOR_DESC ) ) )
            .metavar( "COLOR" );

    m_argParser.add_argument( ARG_CHECK_ZONES )
            .help( UTF8STDSTR( _( ARG_CHECK_ZONES_DESC ) ) )
            .flag();
}


int CLI::PCB_EXPORT_PDF_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_PDF> pdfJob( new JOB_EXPORT_PCB_PDF() );

    pdfJob->m_filename = m_argInput;
    pdfJob->SetConfiguredOutputPath( m_argOutput );
    pdfJob->m_drawingSheet = m_argDrawingSheet;
    pdfJob->SetVarOverrides( m_argDefineVars );

    if( !wxFile::Exists( pdfJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    pdfJob->m_plotFootprintValues = !m_argParser.get<bool>( ARG_EXCLUDE_VALUE );
    pdfJob->m_plotRefDes = !m_argParser.get<bool>( ARG_EXCLUDE_REFDES );

    pdfJob->m_plotDrawingSheet = m_argParser.get<bool>( ARG_INCLUDE_BORDER_TITLE );
    pdfJob->m_subtractSolderMaskFromSilk = m_argParser.get<bool>( ARG_SUBTRACT_SOLDERMASK );

    if( m_argParser.get<bool>( DEPRECATED_ARG_PLOT_INVISIBLE_TEXT ) )
        wxFprintf( stdout, DEPRECATED_ARG_PLOT_INVISIBLE_TEXT_WARNING );

    pdfJob->m_mirror = m_argParser.get<bool>( ARG_MIRROR );
    pdfJob->m_blackAndWhite = m_argParser.get<bool>( ARG_BLACKANDWHITE );
    pdfJob->m_colorTheme = From_UTF8( m_argParser.get<std::string>( ARG_THEME ).c_str() );
    pdfJob->m_negative = m_argParser.get<bool>( ARG_NEGATIVE );
    pdfJob->m_scale = m_argParser.get<double>( ARG_SCALE );
    pdfJob->m_checkZonesBeforePlot = m_argParser.get<bool>( ARG_CHECK_ZONES );

    pdfJob->m_sketchPadsOnFabLayers = m_argParser.get<bool>( ARG_SKETCH_PADS_ON_FAB_LAYERS );
    pdfJob->m_hideDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_HIDE_DNP_FPS_ON_FAB_LAYERS );
    pdfJob->m_sketchDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_SKETCH_DNP_FPS_ON_FAB_LAYERS );
    pdfJob->m_crossoutDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_CROSSOUT_DNP_FPS_ON_FAB_LAYERS );

    wxString bgColor = From_UTF8( m_argParser.get<std::string>( ARG_BACKGROUND_COLOR ).c_str() );
    if( bgColor != wxEmptyString )
        pdfJob->m_pdfBackgroundColor = bgColor;

    int drillShape = m_argParser.get<int>( ARG_DRILL_SHAPE_OPTION );
    pdfJob->m_drillShapeOption = static_cast<DRILL_MARKS>( drillShape );

    bool argModeMulti = m_argParser.get<bool>( ARG_MODE_MULTIPAGE );
    bool argModeSeparate = m_argParser.get<bool>( ARG_MODE_SEPARATE );
    bool argModeSingle = m_argParser.get<bool>( ARG_MODE_SINGLE );

    if( argModeMulti && argModeSeparate )
    {
        wxFprintf( stderr, _( "Cannot use more than one mode flag\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    pdfJob->m_argLayers = From_UTF8( m_argParser.get<std::string>( ARG_LAYERS ).c_str() );
    pdfJob->m_argCommonLayers = From_UTF8( m_argParser.get<std::string>( ARG_COMMON_LAYERS ).c_str() );

    if( argModeMulti )
    {
        pdfJob->m_pdfGenMode = JOB_EXPORT_PCB_PDF::GEN_MODE::ONE_PAGE_PER_LAYER_ONE_FILE;
        pdfJob->m_pdfSingle = true;
    }
    else if( argModeSeparate )
        pdfJob->m_pdfGenMode = JOB_EXPORT_PCB_PDF::GEN_MODE::ALL_LAYERS_SEPARATE_FILE;
    else if( argModeSingle )
        pdfJob->m_pdfGenMode = JOB_EXPORT_PCB_PDF::GEN_MODE::ALL_LAYERS_ONE_FILE;

    return aKiway.ProcessJob( KIWAY::FACE_PCB, pdfJob.get() );
}
