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

#include "command_pcb_export_pdf.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_pcb_pdf.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <string_utils.h>
#include <wx/crt.h>

#include <locale_io.h>

#define ARG_MODE_SEPARATE "--mode-separate"
#define ARG_MODE_MULTIPAGE "--mode-multipage"

CLI::PCB_EXPORT_PDF_COMMAND::PCB_EXPORT_PDF_COMMAND() : PCB_EXPORT_BASE_COMMAND( "pdf" )
{
    m_argParser.add_description( UTF8STDSTR( _( "Generate PDF from a list of layers" ) ) );

    addLayerArg( true );
    addDrawingSheetArg();
    addDefineArg();

    m_argParser.add_argument( ARG_MODE_SEPARATE )
            .help( UTF8STDSTR( _( "Plot the layers to individual PDF files" ) ) )
            .flag();

    m_argParser.add_argument( ARG_MODE_MULTIPAGE )
            .help( UTF8STDSTR( _( "Plot the layers to a single PDF file with multiple pages" ) ) )
            .flag();

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

    m_argParser.add_argument( "--cl", ARG_COMMON_LAYERS )
            .default_value( std::string() )
            .help( UTF8STDSTR(
                    _( "Layers to include on each plot, comma separated list of untranslated "
                       "layer names to include such as "
                       "F.Cu,B.Cu" ) ) )
            .metavar( "COMMON_LAYER_LIST" );

    m_argParser.add_argument( ARG_PLOT_INVISIBLE_TEXT )
            .help( UTF8STDSTR( _( ARG_PLOT_INVISIBLE_TEXT_DESC ) ) )
            .flag();
}


int CLI::PCB_EXPORT_PDF_COMMAND::doPerform( KIWAY& aKiway )
{
    int baseExit = PCB_EXPORT_BASE_COMMAND::doPerform( aKiway );

    if( baseExit != EXIT_CODES::OK )
        return baseExit;

    std::unique_ptr<JOB_EXPORT_PCB_PDF> pdfJob( new JOB_EXPORT_PCB_PDF() );

    pdfJob->m_filename = m_argInput;
    pdfJob->SetOutputPath( m_argOutput );
    pdfJob->m_drawingSheet = m_argDrawingSheet;
    pdfJob->SetVarOverrides( m_argDefineVars );

    if( !wxFile::Exists( pdfJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    pdfJob->m_plotFootprintValues = !m_argParser.get<bool>( ARG_EXCLUDE_VALUE );
    pdfJob->m_plotRefDes = !m_argParser.get<bool>( ARG_EXCLUDE_REFDES );
    pdfJob->m_plotInvisibleText = m_argParser.get<bool>( ARG_PLOT_INVISIBLE_TEXT );

    pdfJob->m_plotDrawingSheet = m_argParser.get<bool>( ARG_INCLUDE_BORDER_TITLE );

    pdfJob->m_mirror = m_argParser.get<bool>( ARG_MIRROR );
    pdfJob->m_blackAndWhite = m_argParser.get<bool>( ARG_BLACKANDWHITE );
    pdfJob->m_colorTheme = From_UTF8( m_argParser.get<std::string>( ARG_THEME ).c_str() );
    pdfJob->m_negative = m_argParser.get<bool>( ARG_NEGATIVE );

    pdfJob->m_sketchPadsOnFabLayers = m_argParser.get<bool>( ARG_SKETCH_PADS_ON_FAB_LAYERS );
    pdfJob->m_hideDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_HIDE_DNP_FPS_ON_FAB_LAYERS );
    pdfJob->m_sketchDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_SKETCH_DNP_FPS_ON_FAB_LAYERS );
    pdfJob->m_crossoutDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_CROSSOUT_DNP_FPS_ON_FAB_LAYERS );

    int drillShape = m_argParser.get<int>( ARG_DRILL_SHAPE_OPTION );
    pdfJob->m_drillShapeOption = static_cast<JOB_EXPORT_PCB_PDF::DRILL_MARKS>( drillShape );

    pdfJob->m_printMaskLayer = m_selectedLayers;

    bool argModeMulti = m_argParser.get<bool>( ARG_MODE_MULTIPAGE );
    bool argModeSeparate = m_argParser.get<bool>( ARG_MODE_SEPARATE );

    if( argModeMulti && argModeSeparate )
    {
        wxFprintf( stderr, _( "Cannot use more than one mode flag\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    wxString layers = From_UTF8( m_argParser.get<std::string>( ARG_COMMON_LAYERS ).c_str() );
    bool     blah = false;
    pdfJob->m_printMaskLayersToIncludeOnAllLayers = convertLayerStringList( layers, blah );

    if( argModeMulti )
    {
        pdfJob->m_pdfGenMode = JOB_EXPORT_PCB_PDF::GEN_MODE::ONE_PAGE_PER_LAYER_ONE_FILE;
    }
    else if( argModeSeparate )
    {
        pdfJob->m_pdfGenMode = JOB_EXPORT_PCB_PDF::GEN_MODE::ALL_LAYERS_SEPARATE_FILE;
    }

    if( pdfJob->m_pdfGenMode == JOB_EXPORT_PCB_PDF::GEN_MODE::ALL_LAYERS_ONE_FILE )
    {
        wxFprintf( stdout,
                wxT( "\033[33;1m%s\033[0m\n" ),
                _( "This command has deprecated behavior as of KiCad 9.0, the default behavior of this command will change in a future release." ) );

        wxFprintf( stdout, wxT( "\033[33;1m%s\033[0m\n" ),
                _( "The new behavior will match --mode-separate" ) );

        wxFprintf( stdout, wxT( "\033[33;1m%s\033[0m\n" ),
                   _( "The behavior with neither --mode flags will no longer exist" ) );
    }

    LOCALE_IO dummy;    // Switch to "C" locale
    int exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, pdfJob.get() );

    return exitCode;
}
