/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

#include "command_pcb_export_svg.h"
#include <cli/exit_codes.h>
#include "command_pcb_export_base.h"
#include "jobs/job_export_pcb_svg.h"
#include <kiface_base.h>
#include <string_utils.h>
#include <regex>
#include <wx/crt.h>


#define ARG_EXCLUDE_DRAWING_SHEET "--exclude-drawing-sheet"
#define ARG_PAGE_SIZE "--page-size-mode"
#define ARG_FIT_PAGE_TO_BOARD "--fit-page-to-board"
#define ARG_MODE_SINGLE "--mode-single"
#define ARG_MODE_MULTI "--mode-multi"

CLI::PCB_EXPORT_SVG_COMMAND::PCB_EXPORT_SVG_COMMAND() :
        PCB_EXPORT_BASE_COMMAND( "svg", false, true )
{
    m_argParser.add_description( UTF8STDSTR( _( "Generate SVG outputs of a given layer list" ) ) );

    addLayerArg();
    addCommonLayersArg();
    addDrawingSheetArg();
    addDefineArg();

    m_argParser.add_argument( ARG_SUBTRACT_SOLDERMASK )
            .help( UTF8STDSTR( _( "Subtract soldermask from silkscreen" ) ) )
            .flag();

    m_argParser.add_argument( "-m", ARG_MIRROR )
            .help( UTF8STDSTR( _( "Mirror the board (useful for trying to show bottom layers)" ) ) )
            .flag();

    m_argParser.add_argument( "-t", ARG_THEME )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Color theme to use (will default to PCB editor settings)" ) ) )
            .metavar( "THEME_NAME" );

    m_argParser.add_argument( ARG_NEGATIVE_SHORT, ARG_NEGATIVE )
            .help( UTF8STDSTR( _( ARG_NEGATIVE_DESC ) ) )
            .flag();

    m_argParser.add_argument( ARG_BLACKANDWHITE )
            .help( UTF8STDSTR( _( ARG_BLACKANDWHITE_DESC ) ) )
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

    m_argParser.add_argument( ARG_PAGE_SIZE )
            .help( UTF8STDSTR( _( "Set page sizing mode (0 = page with frame and title block, 1 = "
                                "current page size, 2 = board area only)" ) ) )
            .scan<'i', int>()
            .default_value( 0 )
            .metavar( "MODE" );

    m_argParser.add_argument( ARG_FIT_PAGE_TO_BOARD )
            .help( UTF8STDSTR( _( "Fit the page to the board" ) ) )
            .flag();

    m_argParser.add_argument( ARG_EXCLUDE_DRAWING_SHEET )
            .help( UTF8STDSTR( _( "No drawing sheet" ) ) )
            .flag();

    m_argParser.add_argument( ARG_DRILL_SHAPE_OPTION )
            .help( UTF8STDSTR( _( ARG_DRILL_SHAPE_OPTION_DESC ) ) )
            .scan<'i', int>()
            .default_value( 2 )
            .metavar( "SHAPE_OPTION" );

    m_argParser.add_argument( ARG_MODE_SINGLE )
            .help( UTF8STDSTR(
                    _( "Generates a single file with the output arg path acting as the complete "
                       "directory and filename path. COMMON_LAYER_LIST does not function in this "
                       "mode. Instead LAYER_LIST controls all layers plotted." ) ) )
            .flag();

    m_argParser.add_argument( ARG_MODE_MULTI )
            .help( UTF8STDSTR( _( "Generates one or more files with behavior similar to the KiCad "
                                  "GUI plotting. The given output path specifies a directory in "
                                  "which files may be output." ) ) )
            .flag();

    m_argParser.add_argument( DEPRECATED_ARG_PLOT_INVISIBLE_TEXT )
            .help( UTF8STDSTR( _( DEPRECATED_ARG_PLOT_INVISIBLE_TEXT_DESC ) ) )
            .flag();

    m_argParser.add_argument( ARG_SCALE )
            .help( UTF8STDSTR( _( ARG_SCALE_DESC ) ) )
            .scan<'g', double>()
            .default_value( 1.0 )
            .metavar( "SCALE" );

    m_argParser.add_argument( ARG_CHECK_ZONES )
            .help( UTF8STDSTR( _( ARG_CHECK_ZONES_DESC ) ) )
            .flag();
}


int CLI::PCB_EXPORT_SVG_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_SVG> svgJob( new JOB_EXPORT_PCB_SVG() );

    svgJob->m_mirror = m_argParser.get<bool>( ARG_MIRROR );
    svgJob->m_blackAndWhite = m_argParser.get<bool>( ARG_BLACKANDWHITE );
    svgJob->m_negative = m_argParser.get<bool>( ARG_NEGATIVE );
    svgJob->m_sketchPadsOnFabLayers = m_argParser.get<bool>( ARG_SKETCH_PADS_ON_FAB_LAYERS );
    svgJob->m_hideDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_HIDE_DNP_FPS_ON_FAB_LAYERS );
    svgJob->m_sketchDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_SKETCH_DNP_FPS_ON_FAB_LAYERS );
    svgJob->m_crossoutDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_CROSSOUT_DNP_FPS_ON_FAB_LAYERS );
    int drillShape = m_argParser.get<int>( ARG_DRILL_SHAPE_OPTION );
    svgJob->m_drillShapeOption = static_cast<DRILL_MARKS>( drillShape );
    svgJob->m_drawingSheet = m_argDrawingSheet;
    svgJob->m_subtractSolderMaskFromSilk = m_argParser.get<bool>( ARG_SUBTRACT_SOLDERMASK );
    svgJob->m_scale = m_argParser.get<double>( ARG_SCALE );
    svgJob->m_checkZonesBeforePlot = m_argParser.get<bool>( ARG_CHECK_ZONES );

    if( m_argParser.get<bool>( DEPRECATED_ARG_PLOT_INVISIBLE_TEXT ) )
        wxFprintf( stdout, DEPRECATED_ARG_PLOT_INVISIBLE_TEXT_WARNING );

    svgJob->m_fitPageToBoard = m_argParser.get<bool>( ARG_FIT_PAGE_TO_BOARD );

    // legacy compat, should eliminate this arg eventually
    int legacyPageSizeMode = m_argParser.get<int>( ARG_PAGE_SIZE );

    if( legacyPageSizeMode == 0 )
    {
        svgJob->m_plotDrawingSheet = true;
    }
    else if( legacyPageSizeMode == 1 )
    {
        svgJob->m_plotDrawingSheet = false;
    }
    else if( legacyPageSizeMode == 2 )
    {
        svgJob->m_fitPageToBoard = true;
        svgJob->m_plotDrawingSheet = false;
    }

    svgJob->m_filename = m_argInput;
    svgJob->SetConfiguredOutputPath( m_argOutput );
    svgJob->m_colorTheme = From_UTF8( m_argParser.get<std::string>( ARG_THEME ).c_str() );
    svgJob->m_plotDrawingSheet = !m_argParser.get<bool>( ARG_EXCLUDE_DRAWING_SHEET );
    svgJob->SetVarOverrides( m_argDefineVars );

    svgJob->m_argLayers = From_UTF8( m_argParser.get<std::string>( ARG_LAYERS ).c_str() );
    svgJob->m_argCommonLayers = From_UTF8( m_argParser.get<std::string>( ARG_COMMON_LAYERS ).c_str() );

    if( !wxFile::Exists( svgJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    if( m_argParser.get<bool>( ARG_MODE_MULTI ) )
        svgJob->m_genMode = JOB_EXPORT_PCB_SVG::GEN_MODE::MULTI;
    else if( m_argParser.get<bool>( ARG_MODE_SINGLE ) )
        svgJob->m_genMode = JOB_EXPORT_PCB_SVG::GEN_MODE::SINGLE;
    else if( svgJob->m_genMode == JOB_EXPORT_PCB_SVG::GEN_MODE::SINGLE )
    {
        // TODO remove this block when changing the default for JOB_EXPORT_PCB_SVG::m_genMode
        wxFprintf( stdout, wxT( "\033[33;1m%s\033[0m\n" ),
                   _( "This command has deprecated behavior as of KiCad 9.0, the default behavior "
                      "of this command will change in a future release." ) );

        wxFprintf( stdout, wxT( "\033[33;1m%s\033[0m\n" ),
                   _( "The new behavior will match --mode-multi" ) );
    }

    return aKiway.ProcessJob( KIWAY::FACE_PCB, svgJob.get() );
}
