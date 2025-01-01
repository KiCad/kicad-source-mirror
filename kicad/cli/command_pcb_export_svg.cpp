/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

#include "command_pcb_export_svg.h"
#include <cli/exit_codes.h>
#include "command_pcb_export_base.h"
#include "jobs/job_export_pcb_svg.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <string_utils.h>
#include <regex>
#include <wx/crt.h>


#define ARG_EXCLUDE_DRAWING_SHEET "--exclude-drawing-sheet"
#define ARG_PAGE_SIZE "--page-size-mode"
#define ARG_MODE_NEW "--mode-new"

CLI::PCB_EXPORT_SVG_COMMAND::PCB_EXPORT_SVG_COMMAND() : PCB_EXPORT_BASE_COMMAND( "svg" )
{
    m_argParser.add_description( UTF8STDSTR( _( "Generate SVG outputs of a given layer list" ) ) );

    addLayerArg( true );
    addDrawingSheetArg();
    addDefineArg();

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

    m_argParser.add_argument( ARG_EXCLUDE_DRAWING_SHEET )
            .help( UTF8STDSTR( _( "No drawing sheet" ) ) )
            .flag();

    m_argParser.add_argument( ARG_DRILL_SHAPE_OPTION )
            .help( UTF8STDSTR( _( ARG_DRILL_SHAPE_OPTION_DESC ) ) )
            .scan<'i', int>()
            .default_value( 2 )
            .metavar( "SHAPE_OPTION" );

    m_argParser.add_argument( "--cl", ARG_COMMON_LAYERS )
            .default_value( std::string() )
            .help( UTF8STDSTR(
                    _( "Layers to include on each plot, comma separated list of untranslated "
                       "layer names to include such as "
                       "F.Cu,B.Cu" ) ) )
            .metavar( "COMMON_LAYER_LIST" );

    m_argParser.add_argument( ARG_MODE_NEW )
            .help( UTF8STDSTR(
                    _( "Opt into the new behavior which means output path is a directory, a file "
                       "per layer is generated and the common layers arg becomes available. " ) ) )
            .flag();


    m_argParser.add_argument( ARG_PLOT_INVISIBLE_TEXT )
            .help( UTF8STDSTR( _( ARG_PLOT_INVISIBLE_TEXT_DESC ) ) )
            .flag();
}


int CLI::PCB_EXPORT_SVG_COMMAND::doPerform( KIWAY& aKiway )
{
    int baseExit = PCB_EXPORT_BASE_COMMAND::doPerform( aKiway );
    if( baseExit != EXIT_CODES::OK )
        return baseExit;

    std::unique_ptr<JOB_EXPORT_PCB_SVG> svgJob( new JOB_EXPORT_PCB_SVG() );

    svgJob->m_mirror = m_argParser.get<bool>( ARG_MIRROR );
    svgJob->m_blackAndWhite = m_argParser.get<bool>( ARG_BLACKANDWHITE );
    svgJob->m_pageSizeMode = m_argParser.get<int>( ARG_PAGE_SIZE );
    svgJob->m_negative = m_argParser.get<bool>( ARG_NEGATIVE );
    svgJob->m_sketchPadsOnFabLayers = m_argParser.get<bool>( ARG_SKETCH_PADS_ON_FAB_LAYERS );
    svgJob->m_hideDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_HIDE_DNP_FPS_ON_FAB_LAYERS );
    svgJob->m_sketchDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_SKETCH_DNP_FPS_ON_FAB_LAYERS );
    svgJob->m_crossoutDNPFPsOnFabLayers = m_argParser.get<bool>( ARG_CROSSOUT_DNP_FPS_ON_FAB_LAYERS );
    int drillShape = m_argParser.get<int>( ARG_DRILL_SHAPE_OPTION );
    svgJob->m_drillShapeOption = static_cast<JOB_EXPORT_PCB_SVG::DRILL_MARKS>( drillShape );
    svgJob->m_drawingSheet = m_argDrawingSheet;
    svgJob->m_plotInvisibleText = m_argParser.get<bool>( ARG_PLOT_INVISIBLE_TEXT );

    svgJob->m_filename = m_argInput;
    svgJob->SetOutputPath( m_argOutput );
    svgJob->m_colorTheme = From_UTF8( m_argParser.get<std::string>( ARG_THEME ).c_str() );
    svgJob->m_plotDrawingSheet = !m_argParser.get<bool>( ARG_EXCLUDE_DRAWING_SHEET );
    svgJob->SetVarOverrides( m_argDefineVars );

    wxString layers = From_UTF8( m_argParser.get<std::string>( ARG_COMMON_LAYERS ).c_str() );
    bool     blah = false;
    svgJob->m_printMaskLayersToIncludeOnAllLayers = convertLayerStringList( layers, blah );

    if( !wxFile::Exists( svgJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    svgJob->m_printMaskLayer = m_selectedLayers;

    if( m_argParser.get<bool>( ARG_MODE_NEW ) )
        svgJob->m_genMode = JOB_EXPORT_PCB_SVG::GEN_MODE::NEW;
    else
        svgJob->m_genMode = JOB_EXPORT_PCB_SVG::GEN_MODE::DEPRECATED;

    if( svgJob->m_genMode == JOB_EXPORT_PCB_SVG::GEN_MODE::DEPRECATED )
    {
        wxFprintf( stdout, wxT( "\033[33;1m%s\033[0m\n" ),
                   _( "This command has deprecated behavior as of KiCad 9.0, the default behavior "
                      "of this command will change in a future release." ) );

        wxFprintf( stdout, wxT( "\033[33;1m%s\033[0m\n" ),
                   _( "The new behavior will match --mode-new" ) );
    }

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, svgJob.get() );

    return exitCode;
}
